#include <algorithm>
#include <atomic>
#include <memory>
#include <meta>
#include <source_location>
#include <unordered_map>

#include <windows.h>

#include <ddraw.h>
#include <winnt.h>

#include "core/utils/annotations.h"
#include "core/utils/error.h"
#include "core/utils/iat_patcher.h"
#include "core/utils/log.h"
#include "core/utils/meta.h"
#include "core/utils/narrow.h"
#include "core/utils/trampoline.h"

using namespace std::literals;

extern "C"
{

[[= gems::IATPatch]] void WINAPI AcquireDDThreadLock();
[[= gems::IATPatch]] void WINAPI ReleaseDDThreadLock();
[[= gems::IATPatch]] ::HRESULT WINAPI DirectDrawEnumerateA(::LPDDENUMCALLBACKA, ::LPVOID);
[[= gems::IATPatch]] ::HRESULT WINAPI DirectDrawEnumerateW(::LPDDENUMCALLBACKW, ::LPVOID);
[[= gems::IATPatch]] ::HRESULT WINAPI DirectDrawEnumerateExA(::LPDDENUMCALLBACKEXA, ::LPVOID, ::DWORD);
[[= gems::IATPatch]] ::HRESULT WINAPI DirectDrawEnumerateExW(::LPDDENUMCALLBACKEXW, ::LPVOID, ::DWORD);
[[= gems::IATPatch]] ::HRESULT WINAPI DirectDrawCreateClipper(::DWORD, ::LPDIRECTDRAWCLIPPER *, ::IUnknown *);
[[= gems::IATPatch]] ::HRESULT WINAPI DirectDrawCreate(::GUID *, ::LPDIRECTDRAW *, ::IUnknown *);
[[= gems::IATPatch]] ::HRESULT WINAPI DirectDrawCreateEx(::GUID *, ::LPVOID *, REFIID, ::IUnknown *);
[[= gems::IATPatch]] ::HRESULT WINAPI D3DParseUnknownCommand(::LPVOID, ::LPVOID *);
[[= gems::IATPatch]] ::HRESULT WINAPI GetSurfaceFromDC(::HDC, ::LPDIRECTDRAWSURFACE4 *, ::HDC *);
[[= gems::IATPatch]] ::HRESULT WINAPI DllCanUnloadNow();
[[= gems::IATPatch]] ::HRESULT WINAPI DllGetClassObject(REFCLSID, REFIID, void **);
[[= gems::IATPatch]] ::HRESULT WINAPI CompleteCreateSysmemSurface();
[[= gems::IATPatch]] ::HRESULT WINAPI DDGetAttachedSurfaceLcl(::DWORD, ::DWORD, ::DWORD);
[[= gems::IATPatch]] ::HRESULT WINAPI DDInternalLock(::DWORD, ::DWORD);
[[= gems::IATPatch]] ::HRESULT WINAPI DDInternalUnlock(::DWORD);
[[= gems::IATPatch]] ::HRESULT WINAPI DSoundHelp(::DWORD, ::DWORD, ::DWORD);
[[= gems::IATPatch]] ::HRESULT WINAPI GetDDSurfaceLocal();
[[= gems::IATPatch]] ::HRESULT WINAPI GetOLEThunkData();
[[= gems::IATPatch]] ::HRESULT WINAPI RegisterSpecialCase();
[[= gems::IATPatch]] ::HRESULT WINAPI SetAppCompatData();
}

namespace gems::impl
{

auto g_kernel32 = ::HMODULE{};
auto g_proxied_ddraw = ::HMODULE{};
auto g_iat_entries = std::unordered_map<std::string_view, void *>{};
auto g_system_dll_functions = std::unordered_map<std::string_view, void *>{};
void *g_get_proc_address = {};

typedef struct _LDR_DLL_LOADED_NOTIFICATION_DATA
{
    ULONG Flags;                  // Reserved.
    PCUNICODE_STRING FullDllName; // The full path name of the DLL module.
    PCUNICODE_STRING BaseDllName; // The base file name of the DLL module.
    PVOID DllBase;                // A pointer to the base address for the DLL in memory.
    ULONG SizeOfImage;            // The size of the DLL image, in bytes.
} LDR_DLL_LOADED_NOTIFICATION_DATA, *PLDR_DLL_LOADED_NOTIFICATION_DATA;

typedef struct _LDR_DLL_UNLOADED_NOTIFICATION_DATA
{
    ULONG Flags;                  // Reserved.
    PCUNICODE_STRING FullDllName; // The full path name of the DLL module.
    PCUNICODE_STRING BaseDllName; // The base file name of the DLL module.
    PVOID DllBase;                // A pointer to the base address for the DLL in memory.
    ULONG SizeOfImage;            // The size of the DLL image, in bytes.
} LDR_DLL_UNLOADED_NOTIFICATION_DATA, *PLDR_DLL_UNLOADED_NOTIFICATION_DATA;

typedef union _LDR_DLL_NOTIFICATION_DATA
{
    LDR_DLL_LOADED_NOTIFICATION_DATA Loaded;
    LDR_DLL_UNLOADED_NOTIFICATION_DATA Unloaded;
} LDR_DLL_NOTIFICATION_DATA, *PLDR_DLL_NOTIFICATION_DATA;

::FARPROC WINAPI GetProcAddress(::HMODULE hModule, ::LPCSTR lpProcName)
{
    auto module_name = std::string(MAX_PATH, '\0');
    const auto length = ::GetModuleFileName(hModule, std::ranges::data(module_name), std::ranges::size(module_name));
    module_name.resize(length);

    log("GetProcAddress({} {})", module_name, lpProcName);

    ensure(g_proxied_ddraw != ::HMODULE{}, "proxied ddraw not set");

    if (hModule == g_proxied_ddraw)
    {
        log("routing {} to proxy", lpProcName);

        const auto lookup = g_iat_entries.find(lpProcName);
        ensure(lookup != std::ranges::cend(g_iat_entries), "{} missing from lookup", lpProcName);

        return reinterpret_cast<::FARPROC>(lookup->second);
    }

    return kernel32::GetProcAddress(
        reinterpret_cast<decltype(&::GetProcAddress)>(g_get_proc_address), hModule, lpProcName);
}

VOID CALLBACK LdrDllNotification(::ULONG NotificationReason, const LDR_DLL_NOTIFICATION_DATA *NotificationData, ::PVOID)
{
    switch (NotificationReason)
    {
        case 1:
        {
            const auto full_dll_name = narrow(NotificationData->Loaded.FullDllName);

            log("dll loaded: {}", full_dll_name);

            const auto module = ::LoadLibrary(full_dll_name.c_str());
            ensure(module != NULL, "failed to load module");

            {
                const auto res = gems::impl::patch_out_module(module, "ddraw.dll", g_iat_entries);
                if (!res)
                {
                    log("failed to patch {}: {}", full_dll_name, res.error());
                }
            }

            {
                const auto res = gems::impl::patch_out_module(
                    module,
                    "kernel32.dll",
                    {{"GetProcAddress", reinterpret_cast<void *>(&gems::impl::GetProcAddress)}});
                if (!res)
                {
                    log("failed to patch {}: {}", full_dll_name, res.error());
                }
            }

            break;
        }
        case 2:
        {
            log("dll unloaded: {}", narrow(NotificationData->Unloaded.FullDllName));
            break;
        }
    }
}

auto try_load_iat_hooks(std::source_location loc = std::source_location::current())
{
    static auto completed = std::atomic<bool>{false};
    if (completed.exchange(true))
    {
        return;
    }

    const auto ntdll = ::LoadLibrary("ntdll.dll");
    ensure(ntdll != NULL, "failed to load ntdll");

    using PLDR_DLL_NOTIFICATION_FUNCTION = VOID(CALLBACK *)(ULONG, const LDR_DLL_NOTIFICATION_DATA *, PVOID);
    using LdrRegisterDllNotificationFn = int(CALLBACK *)(::ULONG, PLDR_DLL_NOTIFICATION_FUNCTION, ::PVOID, ::PVOID *);

    const auto LdrRegisterDllNotification =
        reinterpret_cast<LdrRegisterDllNotificationFn>(::GetProcAddress(ntdll, "LdrRegisterDllNotification"));
    ensure(LdrRegisterDllNotification != nullptr, "failed to find LdrRegisterDllNotification");

    log("registering loader callback");
    void *cookie = {};
    ensure(
        LdrRegisterDllNotification(0u, &gems::impl::LdrDllNotification, nullptr, &cookie) == 0,
        "could not register callback");

    g_kernel32 = load_system_dll("kernel32.dll");
    g_get_proc_address = reinterpret_cast<void *>(::GetProcAddress(g_kernel32, "GetProcAddress"));

    log("loading iat hooks, called from: {}", loc.function_name());

    const auto system_ddraw = load_system_dll("ddraw.dll");
    log("loaded system ddraw.dll: {}", static_cast<void *>(system_ddraw));

    static constexpr auto proxy_functions =
        std::define_static_array(find_functions_with_annotations<^^gems, ^^IATProxyAnnotation>());
    static constexpr auto to_patch_functions =
        std::define_static_array(find_functions_with_annotations<^^::, ^^IATPatchAnnotation>());

    template for (constexpr auto func : to_patch_functions)
    {
        constexpr auto func_name = std::meta::identifier_of(func);

        const auto func_name_str = std::string(std::ranges::data(func_name), std::ranges::size(func_name));
        auto *addr = reinterpret_cast<void *>(::GetProcAddress(system_ddraw, func_name_str.c_str()));

        g_system_dll_functions[func_name] = addr;

        if constexpr (
            std::ranges::contains(proxy_functions, func_name, [](const auto e) { return std::meta::identifier_of(e); }))
        {
            constexpr auto proxy_func_meta =
                std::ranges::find(proxy_functions, func_name, [](const auto e) { return std::meta::identifier_of(e); });

            using ArgsTupleType = GetArgs<typename[:std::meta::type_of(func):]>::type;
            constexpr auto trampoline = build_trampoline<ArgsTupleType>(func, *proxy_func_meta);

            addr = reinterpret_cast<void *>(&[:trampoline:]);
        }

        ensure(addr, "{} not found", func_name);
        g_iat_entries[func_name] = addr;
    }

    log("found {} iat entries to patch", std::ranges::size(g_iat_entries));

    iat_patcher("ddraw.dll", g_iat_entries);
    iat_patcher("kernel32.dll", {{"GetProcAddress", reinterpret_cast<void *>(&gems::impl::GetProcAddress)}});
}
}

extern "C"
{

[[= gems::IATPatch]] void WINAPI AcquireDDThreadLock()
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("AcquireDDThreadLock");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    reinterpret_cast<decltype(&AcquireDDThreadLock)>(orig_func->second)();
}

[[= gems::IATPatch]] void WINAPI ReleaseDDThreadLock()
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("ReleaseDDThreadLock");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    reinterpret_cast<decltype(&ReleaseDDThreadLock)>(orig_func->second)();
}

[[= gems::IATPatch]] ::HRESULT WINAPI DirectDrawEnumerateA(::LPDDENUMCALLBACKA a, ::LPVOID b)
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("DirectDrawEnumerateA");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    return reinterpret_cast<decltype(&DirectDrawEnumerateA)>(orig_func->second)(a, b);
}

[[= gems::IATPatch]] ::HRESULT WINAPI DirectDrawEnumerateW(::LPDDENUMCALLBACKW a, ::LPVOID b)
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("DirectDrawEnumerateW");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    return reinterpret_cast<decltype(&DirectDrawEnumerateW)>(orig_func->second)(a, b);
}

[[= gems::IATPatch]] ::HRESULT WINAPI DirectDrawEnumerateExA(::LPDDENUMCALLBACKEXA a, ::LPVOID b, ::DWORD c)
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("DirectDrawEnumerateExA");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    return reinterpret_cast<decltype(&DirectDrawEnumerateExA)>(orig_func->second)(a, b, c);
}

[[= gems::IATPatch]] ::HRESULT WINAPI DirectDrawEnumerateExW(::LPDDENUMCALLBACKEXW a, ::LPVOID b, ::DWORD c)
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("DirectDrawEnumerateExW");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    return reinterpret_cast<decltype(&DirectDrawEnumerateExW)>(orig_func->second)(a, b, c);
}

[[= gems::IATPatch]] ::HRESULT WINAPI DirectDrawCreateClipper(::DWORD a, ::LPDIRECTDRAWCLIPPER *b, ::IUnknown *c)
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("DirectDrawCreateClipper");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    return reinterpret_cast<decltype(&DirectDrawCreateClipper)>(orig_func->second)(a, b, c);
}

[[= gems::IATPatch]] ::HRESULT WINAPI DirectDrawCreate(::GUID *a, ::LPDIRECTDRAW *b, ::IUnknown *c)
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("DirectDrawCreate");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    return reinterpret_cast<decltype(&DirectDrawCreate)>(orig_func->second)(a, b, c);
}

[[= gems::IATPatch]] ::HRESULT WINAPI DirectDrawCreateEx(::GUID *a, ::LPVOID *b, REFIID c, ::IUnknown *d)
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("DirectDrawCreateEx");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    return reinterpret_cast<decltype(&DirectDrawCreateEx)>(orig_func->second)(a, b, c, d);
}

[[= gems::IATPatch]] ::HRESULT WINAPI D3DParseUnknownCommand(::LPVOID a, ::LPVOID *b)
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("D3DParseUnknownCommand");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    return reinterpret_cast<decltype(&D3DParseUnknownCommand)>(orig_func->second)(a, b);
}

[[= gems::IATPatch]] ::HRESULT WINAPI GetSurfaceFromDC(::HDC a, ::LPDIRECTDRAWSURFACE4 *b, ::HDC *c)
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("GetSurfaceFromDC");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    return reinterpret_cast<decltype(&GetSurfaceFromDC)>(orig_func->second)(a, b, c);
}

[[= gems::IATPatch]] ::HRESULT WINAPI DllCanUnloadNow()
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("DllCanUnloadNow");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    return reinterpret_cast<decltype(&DllCanUnloadNow)>(orig_func->second)();
}

[[= gems::IATPatch]] ::HRESULT WINAPI DllGetClassObject(REFCLSID a, REFIID b, void **c)
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("DllGetClassObject");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    return reinterpret_cast<decltype(&DllGetClassObject)>(orig_func->second)(a, b, c);
}

[[= gems::IATPatch]] ::HRESULT WINAPI CompleteCreateSysmemSurface()
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("CompleteCreateSysmemSurface");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    return reinterpret_cast<decltype(&CompleteCreateSysmemSurface)>(orig_func->second)();
}

[[= gems::IATPatch]] ::HRESULT WINAPI DDGetAttachedSurfaceLcl(::DWORD a, ::DWORD b, ::DWORD c)
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("DDGetAttachedSurfaceLcl");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    return reinterpret_cast<decltype(&DDGetAttachedSurfaceLcl)>(orig_func->second)(a, b, c);
}

[[= gems::IATPatch]] ::HRESULT WINAPI DDInternalLock(::DWORD a, ::DWORD b)
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("DDInternalLock");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    return reinterpret_cast<decltype(&DDInternalLock)>(orig_func->second)(a, b);
}

[[= gems::IATPatch]] ::HRESULT WINAPI DDInternalUnlock(::DWORD a)
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("DDInternalUnlock");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    return reinterpret_cast<decltype(&DDInternalUnlock)>(orig_func->second)(a);
}

[[= gems::IATPatch]] ::HRESULT WINAPI DSoundHelp(::DWORD a, ::DWORD b, ::DWORD c)
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("DSoundHelp");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    return reinterpret_cast<decltype(&DSoundHelp)>(orig_func->second)(a, b, c);
}

[[= gems::IATPatch]] ::HRESULT WINAPI GetDDSurfaceLocal()
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("GetDDSurfaceLocal");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    return reinterpret_cast<decltype(&GetDDSurfaceLocal)>(orig_func->second)();
}

[[= gems::IATPatch]] ::HRESULT WINAPI GetOLEThunkData()
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("GetOLEThunkData");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    return reinterpret_cast<decltype(&GetOLEThunkData)>(orig_func->second)();
}

[[= gems::IATPatch]] ::HRESULT WINAPI RegisterSpecialCase()
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("RegisterSpecialCase");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    return reinterpret_cast<decltype(&RegisterSpecialCase)>(orig_func->second)();
}

[[= gems::IATPatch]] ::HRESULT WINAPI SetAppCompatData()
{
    gems::impl::try_load_iat_hooks();

    const auto orig_func = gems::impl::g_system_dll_functions.find("SetAppCompatData");
    gems::ensure(orig_func != std::ranges::cend(gems::impl::g_system_dll_functions), "could not find iat entry");

    return reinterpret_cast<decltype(&SetAppCompatData)>(orig_func->second)();
}

::BOOL WINAPI DllMain(::HMODULE module, ::DWORD reason, void *)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        ::DisableThreadLibraryCalls(module);
        gems::log("DllMain called for ddraw.dll");

        gems::impl::g_proxied_ddraw = module;
    }
    return TRUE;
}
}
