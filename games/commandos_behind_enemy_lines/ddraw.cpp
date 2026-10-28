#include <array>
#include <cstdint>
#include <cstring>
#include <string>

#include <windows.h>

#include <ddraw.h>

#include "core/utils/auto_writer.h"
#include "core/utils/log.h"

// Ctrl+Enter switches Comandos.exe to a window, but the picture freezes.
// The present function (0x596220) converts the 16-bit back buffer into a 32-bit
// temp surface, then copies it to the primary surface only when fullscreen:
//
//   0x59639e  cmp dword ptr [esi+0x3c0], 0   ; fullscreen flag
//   0x5963a5  jz  0x5963c5                   ; windowed: skip the Blt
//
// We replace those 9 bytes with a jump to our own present. It does the Blt in
// both modes. In windowed mode the target is the window's client area.

namespace gems
{

namespace
{

constexpr auto patch_rva = std::uintptr_t{0x19639e};
constexpr auto resume_rva = std::uintptr_t{0x1963c5};
constexpr auto original_bytes = std::array<std::uint8_t, 9>{0x83, 0xbe, 0xc0, 0x03, 0x00, 0x00, 0x00, 0x74, 0x1e};

constexpr auto game_window = 0x204;
constexpr auto game_primary = 0x25c;
constexpr auto game_fullscreen = 0x3c0;

auto g_system_ddraw = ::HMODULE{};

auto client_rect_on_screen(::HWND window, ::RECT &rect) -> bool
{
    if (::GetClientRect(window, &rect) == 0 || rect.right <= 0 || rect.bottom <= 0)
    {
        return false;
    }

    ::MapWindowPoints(window, nullptr, reinterpret_cast<::POINT *>(&rect), 2);
    return true;
}

void __stdcall present(std::uint8_t *game, ::IDirectDrawSurface *frame)
{
    auto *primary = *reinterpret_cast<::IDirectDrawSurface **>(game + game_primary);
    if (primary == nullptr || frame == nullptr)
    {
        return;
    }

    auto client = ::RECT{};
    auto *target = static_cast<::RECT *>(nullptr);

    if (*reinterpret_cast<int *>(game + game_fullscreen) == 0)
    {
        if (!client_rect_on_screen(*reinterpret_cast<::HWND *>(game + game_window), client))
        {
            return;
        }
        target = &client;
    }

    if (primary->Blt(target, frame, nullptr, DDBLT_WAIT, nullptr) == DDERR_SURFACELOST)
    {
        primary->Restore();
    }
}

// Machine code, so the stub builds the same with MinGW and MSVC.
// At the patch site esi is the game object and [esp+0x24] is the temp surface.
auto build_stub(std::uintptr_t resume) -> void *
{
    auto *stub = static_cast<std::uint8_t *>(
        ::VirtualAlloc(nullptr, 32, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (stub == nullptr)
    {
        return nullptr;
    }

    auto code = std::array<std::uint8_t, 18>{
        0x8b, 0x44, 0x24, 0x24, // mov eax, [esp+0x24]
        0x50,                   // push eax
        0x56,                   // push esi
        0xe8, 0, 0, 0, 0,       // call present
        0xe9, 0, 0, 0, 0,       // jmp resume
    };

    const auto base = reinterpret_cast<std::uintptr_t>(stub);
    const auto call_rel = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&present) - (base + 11));
    const auto jmp_rel = static_cast<std::uint32_t>(resume - (base + 16));
    std::memcpy(code.data() + 7, &call_rel, 4);
    std::memcpy(code.data() + 12, &jmp_rel, 4);
    std::memcpy(stub, code.data(), code.size());

    return stub;
}

void patch_present()
{
    const auto exe = reinterpret_cast<std::uintptr_t>(::GetModuleHandleA(nullptr));
    auto *site = reinterpret_cast<std::uint8_t *>(exe + patch_rva);

    if (std::memcmp(site, original_bytes.data(), original_bytes.size()) != 0)
    {
        log("present patch skipped: bytes at {} do not match, wrong Comandos.exe?", static_cast<void *>(site));
        return;
    }

    auto *stub = build_stub(exe + resume_rva);
    if (stub == nullptr)
    {
        log("present patch skipped: VirtualAlloc failed");
        return;
    }

    const auto jmp_rel =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(stub) - (reinterpret_cast<std::uintptr_t>(site) + 5));

    {
        const auto writer = AutoWriter{site, original_bytes.size()};
        site[0] = 0xe9;
        std::memcpy(site + 1, &jmp_rel, 4);
        std::memset(site + 5, 0x90, original_bytes.size() - 5);
    }

    ::FlushInstructionCache(::GetCurrentProcess(), site, original_bytes.size());
    log("present patched at {} -> {}", static_cast<void *>(site), stub);
}

template <class T>
auto system_function(const char *name) -> T
{
    return reinterpret_cast<T>(reinterpret_cast<void *>(::GetProcAddress(g_system_ddraw, name)));
}

}

}

extern "C"
{

::HRESULT WINAPI DirectDrawCreate(::GUID *guid, ::LPDIRECTDRAW *dd, ::IUnknown *outer)
{
    static const auto real = gems::system_function<decltype(&DirectDrawCreate)>("DirectDrawCreate");
    return real(guid, dd, outer);
}

::HRESULT WINAPI DirectDrawCreateEx(::GUID *guid, ::LPVOID *dd, REFIID iid, ::IUnknown *outer)
{
    static const auto real = gems::system_function<decltype(&DirectDrawCreateEx)>("DirectDrawCreateEx");
    return real(guid, dd, iid, outer);
}

::HRESULT WINAPI DirectDrawCreateClipper(::DWORD flags, ::LPDIRECTDRAWCLIPPER *clipper, ::IUnknown *outer)
{
    static const auto real = gems::system_function<decltype(&DirectDrawCreateClipper)>("DirectDrawCreateClipper");
    return real(flags, clipper, outer);
}

::HRESULT WINAPI DirectDrawEnumerateA(::LPDDENUMCALLBACKA callback, ::LPVOID context)
{
    static const auto real = gems::system_function<decltype(&DirectDrawEnumerateA)>("DirectDrawEnumerateA");
    return real(callback, context);
}

::HRESULT WINAPI DirectDrawEnumerateW(::LPDDENUMCALLBACKW callback, ::LPVOID context)
{
    static const auto real = gems::system_function<decltype(&DirectDrawEnumerateW)>("DirectDrawEnumerateW");
    return real(callback, context);
}

::HRESULT WINAPI DirectDrawEnumerateExA(::LPDDENUMCALLBACKEXA callback, ::LPVOID context, ::DWORD flags)
{
    static const auto real = gems::system_function<decltype(&DirectDrawEnumerateExA)>("DirectDrawEnumerateExA");
    return real(callback, context, flags);
}

::HRESULT WINAPI DirectDrawEnumerateExW(::LPDDENUMCALLBACKEXW callback, ::LPVOID context, ::DWORD flags)
{
    static const auto real = gems::system_function<decltype(&DirectDrawEnumerateExW)>("DirectDrawEnumerateExW");
    return real(callback, context, flags);
}

::BOOL WINAPI DllMain(::HINSTANCE, ::DWORD reason, ::LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        auto path = std::string(MAX_PATH, '\0');
        const auto length = ::GetSystemDirectoryA(path.data(), MAX_PATH);
        path.resize(length);
        path += "\\ddraw.dll";

        gems::g_system_ddraw = ::LoadLibraryA(path.c_str());
        if (gems::g_system_ddraw == nullptr)
        {
            gems::log("failed to load {}", path);
            return FALSE;
        }

        gems::log("loaded {}", path);
        gems::patch_present();
    }

    return TRUE;
}
}
