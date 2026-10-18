#pragma once

// The proxy defines these APIs rather than importing them from wsock32.dll.
#pragma push_macro("WINSOCK_API_LINKAGE")
#undef WINSOCK_API_LINKAGE
#define WINSOCK_API_LINKAGE
#include <winsock.h>
#pragma pop_macro("WINSOCK_API_LINKAGE")

#include <nspapi.h>
#include <windows.h>
#include <winnt.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <meta>
#include <mutex>
#include <ranges>
#include <source_location>
#include <string>
#include <string_view>
#include <unordered_map>

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

// These public stubs are exported explicitly so the proxy keeps real exports to bind;
// the shared wsock32.def still pins the legacy ordinals.
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI __WSAFDIsSet(::SOCKET, ::fd_set *);
[[= gems::IATPatch]] __declspec(dllexport) ::SOCKET WINAPI accept(::SOCKET, ::sockaddr *, int *);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI bind(::SOCKET, const ::sockaddr *, int);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI closesocket(::SOCKET);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI connect(::SOCKET, const ::sockaddr *, int);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI ioctlsocket(::SOCKET, long, ::u_long *);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI getpeername(::SOCKET, ::sockaddr *, int *);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI getsockname(::SOCKET, ::sockaddr *, int *);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI getsockopt(::SOCKET, int, int, char *, int *);
[[= gems::IATPatch]] __declspec(dllexport) ::u_long WINAPI htonl(::u_long);
[[= gems::IATPatch]] __declspec(dllexport) ::u_short WINAPI htons(::u_short);
[[= gems::IATPatch]] __declspec(dllexport) unsigned long WINAPI inet_addr(const char *);
[[= gems::IATPatch]] __declspec(dllexport) char *WINAPI inet_ntoa(::in_addr);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI listen(::SOCKET, int);
[[= gems::IATPatch]] __declspec(dllexport) ::u_long WINAPI ntohl(::u_long);
[[= gems::IATPatch]] __declspec(dllexport) ::u_short WINAPI ntohs(::u_short);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI recv(::SOCKET, char *, int, int);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI recvfrom(::SOCKET, char *, int, int, ::sockaddr *, int *);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI
select(int, ::fd_set *, ::fd_set *, ::fd_set *, const ::timeval *);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI send(::SOCKET, const char *, int, int);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI sendto(::SOCKET, const char *, int, int, const ::sockaddr *, int);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI setsockopt(::SOCKET, int, int, const char *, int);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI shutdown(::SOCKET, int);
[[= gems::IATPatch]] __declspec(dllexport) ::SOCKET WINAPI socket(int, int, int);
[[= gems::IATPatch]] __declspec(dllexport) ::hostent *WINAPI gethostbyaddr(const char *, int, int);
[[= gems::IATPatch]] __declspec(dllexport) ::hostent *WINAPI gethostbyname(const char *);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI gethostname(char *, int);
[[= gems::IATPatch]] __declspec(dllexport) ::servent *WINAPI getservbyport(int, const char *);
[[= gems::IATPatch]] __declspec(dllexport) ::servent *WINAPI getservbyname(const char *, const char *);
[[= gems::IATPatch]] __declspec(dllexport) ::protoent *WINAPI getprotobynumber(int);
[[= gems::IATPatch]] __declspec(dllexport) ::protoent *WINAPI getprotobyname(const char *);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI WSAStartup(::WORD, ::LPWSADATA);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI WSACleanup();
[[= gems::IATPatch]] __declspec(dllexport) void WINAPI WSASetLastError(int);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI WSAGetLastError();
[[= gems::IATPatch]] __declspec(dllexport) ::BOOL WINAPI WSAIsBlocking();
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI WSAUnhookBlockingHook();
[[= gems::IATPatch]] __declspec(dllexport) ::FARPROC WINAPI WSASetBlockingHook(::FARPROC);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI WSACancelBlockingCall();
[[= gems::IATPatch]] __declspec(dllexport) ::HANDLE WINAPI
WSAAsyncGetServByName(::HWND, ::u_int, const char *, const char *, char *, int);
[[= gems::IATPatch]] __declspec(dllexport) ::HANDLE WINAPI
WSAAsyncGetServByPort(::HWND, ::u_int, int, const char *, char *, int);
[[= gems::IATPatch]] __declspec(dllexport) ::HANDLE WINAPI
WSAAsyncGetProtoByName(::HWND, ::u_int, const char *, char *, int);
[[= gems::IATPatch]] __declspec(dllexport) ::HANDLE WINAPI WSAAsyncGetProtoByNumber(::HWND, ::u_int, int, char *, int);
[[= gems::IATPatch]] __declspec(dllexport) ::HANDLE WINAPI
WSAAsyncGetHostByName(::HWND, ::u_int, const char *, char *, int);
[[= gems::IATPatch]] __declspec(dllexport) ::HANDLE WINAPI
WSAAsyncGetHostByAddr(::HWND, ::u_int, const char *, int, int, char *, int);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI WSACancelAsyncRequest(::HANDLE);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI WSAAsyncSelect(::SOCKET, ::HWND, ::u_int, long);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI WSARecvEx(::SOCKET, char *, int, int *);
[[= gems::IATPatch]] __declspec(dllexport) ::BOOL WINAPI
    TransmitFile(::SOCKET, ::HANDLE, ::DWORD, ::DWORD, ::LPOVERLAPPED, ::LPTRANSMIT_FILE_BUFFERS, ::DWORD);
[[= gems::IATPatch]] __declspec(dllexport) ::BOOL WINAPI
    AcceptEx(::SOCKET, ::SOCKET, ::PVOID, ::DWORD, ::DWORD, ::DWORD, ::LPDWORD, ::LPOVERLAPPED);
[[= gems::IATPatch]] __declspec(dllexport) void WINAPI
GetAcceptExSockaddrs(::PVOID, ::DWORD, ::DWORD, ::DWORD, ::sockaddr **, ::LPINT, ::sockaddr **, ::LPINT);
[[= gems::IATPatch]] __declspec(dllexport) ::INT WINAPI EnumProtocolsA(::LPINT, ::LPVOID, ::LPDWORD);
[[= gems::IATPatch]] __declspec(dllexport) ::INT WINAPI GetAddressByNameA(
    ::DWORD,
    ::LPGUID,
    ::LPSTR,
    ::LPINT,
    ::DWORD,
    ::LPSERVICE_ASYNC_INFO,
    ::LPVOID,
    ::LPDWORD,
    ::LPSTR,
    ::LPDWORD);
[[= gems::IATPatch]] __declspec(dllexport) ::INT WINAPI GetTypeByNameA(::LPSTR, ::LPGUID);
[[= gems::IATPatch]] __declspec(dllexport) ::INT WINAPI GetNameByTypeA(::LPGUID, ::LPSTR, ::DWORD);
[[= gems::IATPatch]] __declspec(dllexport) ::INT WINAPI
    SetServiceA(::DWORD, ::DWORD, ::DWORD, ::LPSERVICE_INFOA, ::LPSERVICE_ASYNC_INFO, ::LPDWORD);
[[= gems::IATPatch]] __declspec(dllexport) ::INT WINAPI
    GetServiceA(::DWORD, ::LPGUID, ::LPSTR, ::DWORD, ::LPVOID, ::LPDWORD, ::LPSERVICE_ASYNC_INFO);
[[= gems::IATPatch]] __declspec(dllexport) ::INT WINAPI EnumProtocolsW(::LPINT, ::LPVOID, ::LPDWORD);
[[= gems::IATPatch]] __declspec(dllexport) ::INT WINAPI GetAddressByNameW(
    ::DWORD,
    ::LPGUID,
    ::LPWSTR,
    ::LPINT,
    ::DWORD,
    ::LPSERVICE_ASYNC_INFO,
    ::LPVOID,
    ::LPDWORD,
    ::LPWSTR,
    ::LPDWORD);
[[= gems::IATPatch]] __declspec(dllexport) ::INT WINAPI GetTypeByNameW(::LPWSTR, ::LPGUID);
[[= gems::IATPatch]] __declspec(dllexport) ::INT WINAPI GetNameByTypeW(::LPGUID, ::LPWSTR, ::DWORD);
[[= gems::IATPatch]] __declspec(dllexport) ::INT WINAPI
    SetServiceW(::DWORD, ::DWORD, ::DWORD, ::LPSERVICE_INFOW, ::LPSERVICE_ASYNC_INFO, ::LPDWORD);
[[= gems::IATPatch]] __declspec(dllexport) ::INT WINAPI
    GetServiceW(::DWORD, ::LPGUID, ::LPWSTR, ::DWORD, ::LPVOID, ::LPDWORD, ::LPSERVICE_ASYNC_INFO);
[[= gems::IATPatch]] __declspec(dllexport) int WINAPI WEP();
}

namespace gems::impl
{

auto g_kernel32 = ::HMODULE{};
auto g_proxied_wsock32 = ::HMODULE{};
auto g_iat_entries = std::unordered_map<std::string_view, void *>{};
auto g_iat_ordinals = IATOrdinalNames{};
auto g_system_dll_functions = std::unordered_map<std::string_view, void *>{};
void *g_get_proc_address = {};
auto g_mutex = std::mutex{};

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
    const auto real_get_proc_address = reinterpret_cast<decltype(&::GetProcAddress)>(g_get_proc_address);

    // Ordinals occupy the low word, not a readable string.
    if (reinterpret_cast<std::uintptr_t>(lpProcName) <= 0xffffu)
    {
        return real_get_proc_address(hModule, lpProcName);
    }
    log("real GetProcAddress: {} {}", (void *)real_get_proc_address, lpProcName);

    ensure(g_proxied_wsock32 != ::HMODULE{}, "proxied wsock32 not set");

    if (hModule == g_proxied_wsock32)
    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        const auto lookup = g_iat_entries.find(lpProcName);
        if (lookup != std::ranges::cend(g_iat_entries))
        {
            return reinterpret_cast<::FARPROC>(lookup->second);
        }
    }

    return real_get_proc_address(hModule, lpProcName);
}

VOID CALLBACK LdrDllNotification(::ULONG NotificationReason, const LDR_DLL_NOTIFICATION_DATA *NotificationData, ::PVOID)
{
    log("loader notification start");

    switch (NotificationReason)
    {
        case 1:
        {
            const auto full_dll_name = narrow(NotificationData->Loaded.FullDllName);

            log("dll loaded: {}", full_dll_name);

            const auto module = ::LoadLibrary(full_dll_name.c_str());
            ensure(module != NULL, "failed to load module");

            {
                const auto res = gems::impl::patch_out_module(module, "wsock32.dll", g_iat_entries, g_iat_ordinals);
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

    log("loader notification done");
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

    if (!g_get_proc_address)
    {
        g_kernel32 = load_system_dll("kernel32.dll");
        g_get_proc_address = reinterpret_cast<void *>(::GetProcAddress(g_kernel32, "GetProcAddress"));
    }

    log("loading iat hooks, called from: {}", loc.function_name());

    const auto system_wsock32 = load_system_dll("wsock32.dll");
    log("loaded system wsock32.dll: {}", static_cast<void *>(system_wsock32));

    static constexpr auto proxy_functions =
        std::define_static_array(find_functions_with_annotations<^^gems, ^^IATProxyAnnotation>());
    static constexpr auto to_patch_functions =
        std::define_static_array(find_functions_with_annotations<^^::, ^^IATPatchAnnotation>());

    template for (constexpr auto func : to_patch_functions)
    {
        constexpr auto func_name = std::meta::identifier_of(func);

        const auto func_name_str = std::string(std::ranges::data(func_name), std::ranges::size(func_name));
        auto *addr = reinterpret_cast<void *>(::GetProcAddress(system_wsock32, func_name_str.c_str()));

        if (addr == nullptr)
        {
            log("{} not found in system wsock32.dll; skipping iat hook", func_name);
            continue;
        }

        log("patching: {}", func_name);
        g_system_dll_functions[func_name] = addr;

        if constexpr (
            std::ranges::contains(proxy_functions, func_name, [](const auto e) { return std::meta::identifier_of(e); }))
        {
            constexpr auto proxy_func_meta =
                std::ranges::find(proxy_functions, func_name, [](const auto e) { return std::meta::identifier_of(e); });

            using ArgsTupleType = GetArgs<typename[:std::meta::type_of(func):]>::type;
            gems::original_function<&[:func:]> = reinterpret_cast<decltype(&[:func:])>(addr);
            constexpr auto trampoline =
                build_trampoline<ArgsTupleType>(func, *proxy_func_meta, ^^gems::original_trampoline);

            addr = reinterpret_cast<void *>(&[:trampoline:]);
        }

        ensure(addr, "{} not found", func_name);
        g_iat_entries[func_name] = addr;
    }

    g_iat_ordinals = build_ordinal_iat_names(g_proxied_wsock32, g_iat_entries);

    log("found {} iat entries to patch", std::ranges::size(g_iat_entries));

    iat_patcher("wsock32.dll", g_iat_entries, g_iat_ordinals);
    iat_patcher("kernel32.dll", {{"GetProcAddress", reinterpret_cast<void *>(&gems::impl::GetProcAddress)}});
}
}

extern "C"
{

[[= gems::IATPatch]] int WINAPI __WSAFDIsSet(::SOCKET s, ::fd_set *set)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("__WSAFDIsSet");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&__WSAFDIsSet)>(orig_func->second)(s, set);
}

[[= gems::IATPatch]] ::SOCKET WINAPI accept(::SOCKET s, ::sockaddr *addr, int *addrlen)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("accept");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&accept)>(orig_func->second)(s, addr, addrlen);
}

[[= gems::IATPatch]] int WINAPI bind(::SOCKET s, const ::sockaddr *name, int namelen)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("bind");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&bind)>(orig_func->second)(s, name, namelen);
}

[[= gems::IATPatch]] int WINAPI closesocket(::SOCKET s)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("closesocket");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&closesocket)>(orig_func->second)(s);
}

[[= gems::IATPatch]] int WINAPI connect(::SOCKET s, const ::sockaddr *name, int namelen)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("connect");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&connect)>(orig_func->second)(s, name, namelen);
}

[[= gems::IATPatch]] int WINAPI ioctlsocket(::SOCKET s, long cmd, ::u_long *argp)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("ioctlsocket");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&ioctlsocket)>(orig_func->second)(s, cmd, argp);
}

[[= gems::IATPatch]] int WINAPI getpeername(::SOCKET s, ::sockaddr *name, int *namelen)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("getpeername");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&getpeername)>(orig_func->second)(s, name, namelen);
}

[[= gems::IATPatch]] int WINAPI getsockname(::SOCKET s, ::sockaddr *name, int *namelen)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("getsockname");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&getsockname)>(orig_func->second)(s, name, namelen);
}

[[= gems::IATPatch]] int WINAPI getsockopt(::SOCKET s, int level, int optname, char *optval, int *optlen)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("getsockopt");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&getsockopt)>(orig_func->second)(s, level, optname, optval, optlen);
}

[[= gems::IATPatch]] ::u_long WINAPI htonl(::u_long hostlong)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("htonl");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&htonl)>(orig_func->second)(hostlong);
}

[[= gems::IATPatch]] ::u_short WINAPI htons(::u_short hostshort)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("htons");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&htons)>(orig_func->second)(hostshort);
}

[[= gems::IATPatch]] unsigned long WINAPI inet_addr(const char *cp)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("inet_addr");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&inet_addr)>(orig_func->second)(cp);
}

[[= gems::IATPatch]] char *WINAPI inet_ntoa(::in_addr in)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("inet_ntoa");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&inet_ntoa)>(orig_func->second)(in);
}

[[= gems::IATPatch]] int WINAPI listen(::SOCKET s, int backlog)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("listen");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&listen)>(orig_func->second)(s, backlog);
}

[[= gems::IATPatch]] ::u_long WINAPI ntohl(::u_long netlong)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("ntohl");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&ntohl)>(orig_func->second)(netlong);
}

[[= gems::IATPatch]] ::u_short WINAPI ntohs(::u_short netshort)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("ntohs");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&ntohs)>(orig_func->second)(netshort);
}

[[= gems::IATPatch]] int WINAPI recv(::SOCKET s, char *buf, int len, int flags)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("recv");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&recv)>(orig_func->second)(s, buf, len, flags);
}

[[= gems::IATPatch]] int WINAPI recvfrom(::SOCKET s, char *buf, int len, int flags, ::sockaddr *from, int *fromlen)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("recvfrom");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&recvfrom)>(orig_func->second)(s, buf, len, flags, from, fromlen);
}

[[= gems::IATPatch]] int WINAPI
select(int nfds, ::fd_set *readfds, ::fd_set *writefds, ::fd_set *exceptfds, const ::timeval *timeout)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("select");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&select)>(orig_func->second)(nfds, readfds, writefds, exceptfds, timeout);
}

[[= gems::IATPatch]] int WINAPI send(::SOCKET s, const char *buf, int len, int flags)
{
    gems::log("this send");

    auto target = decltype(&::send){};
    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);
        gems::impl::try_load_iat_hooks();

        const auto entry = gems::impl::g_iat_entries.find("send");
        gems::ensure(entry != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
        target = reinterpret_cast<decltype(target)>(entry->second);
    }

    return target(s, buf, len, flags);
}

[[= gems::IATPatch]] int WINAPI sendto(::SOCKET s, const char *buf, int len, int flags, const ::sockaddr *to, int tolen)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("sendto");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&sendto)>(orig_func->second)(s, buf, len, flags, to, tolen);
}

[[= gems::IATPatch]] int WINAPI setsockopt(::SOCKET s, int level, int optname, const char *optval, int optlen)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("setsockopt");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&setsockopt)>(orig_func->second)(s, level, optname, optval, optlen);
}

[[= gems::IATPatch]] int WINAPI shutdown(::SOCKET s, int how)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("shutdown");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&shutdown)>(orig_func->second)(s, how);
}

[[= gems::IATPatch]] ::SOCKET WINAPI socket(int af, int type, int protocol)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("socket");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&socket)>(orig_func->second)(af, type, protocol);
}

[[= gems::IATPatch]] ::hostent *WINAPI gethostbyaddr(const char *addr, int len, int type)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("gethostbyaddr");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&gethostbyaddr)>(orig_func->second)(addr, len, type);
}

[[= gems::IATPatch]] ::hostent *WINAPI gethostbyname(const char *name)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("gethostbyname");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&gethostbyname)>(orig_func->second)(name);
}

[[= gems::IATPatch]] int WINAPI gethostname(char *name, int namelen)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("gethostname");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&gethostname)>(orig_func->second)(name, namelen);
}

[[= gems::IATPatch]] ::servent *WINAPI getservbyport(int port, const char *proto)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("getservbyport");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&getservbyport)>(orig_func->second)(port, proto);
}

[[= gems::IATPatch]] ::servent *WINAPI getservbyname(const char *name, const char *proto)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("getservbyname");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&getservbyname)>(orig_func->second)(name, proto);
}

[[= gems::IATPatch]] ::protoent *WINAPI getprotobynumber(int number)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("getprotobynumber");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&getprotobynumber)>(orig_func->second)(number);
}

[[= gems::IATPatch]] ::protoent *WINAPI getprotobyname(const char *name)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("getprotobyname");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&getprotobyname)>(orig_func->second)(name);
}

[[= gems::IATPatch]] int WINAPI WSAStartup(::WORD wVersionRequested, ::LPWSADATA lpWSAData)
{
    auto startup_func = decltype(&::WSAStartup){};

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        const auto entry = gems::impl::g_iat_entries.find("WSAStartup");
        gems::ensure(entry != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
        startup_func = reinterpret_cast<decltype(startup_func)>(entry->second);
    }

    return startup_func(wVersionRequested, lpWSAData);
}

[[= gems::IATPatch]] int WINAPI WSACleanup()
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("WSACleanup");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&WSACleanup)>(orig_func->second)();
}

[[= gems::IATPatch]] void WINAPI WSASetLastError(int iError)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("WSASetLastError");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    reinterpret_cast<decltype(&WSASetLastError)>(orig_func->second)(iError);
}

[[= gems::IATPatch]] int WINAPI WSAGetLastError()
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("WSAGetLastError");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&WSAGetLastError)>(orig_func->second)();
}

[[= gems::IATPatch]] ::BOOL WINAPI WSAIsBlocking()
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("WSAIsBlocking");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&WSAIsBlocking)>(orig_func->second)();
}

[[= gems::IATPatch]] int WINAPI WSAUnhookBlockingHook()
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("WSAUnhookBlockingHook");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&WSAUnhookBlockingHook)>(orig_func->second)();
}

[[= gems::IATPatch]] ::FARPROC WINAPI WSASetBlockingHook(::FARPROC lpBlockFunc)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("WSASetBlockingHook");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&WSASetBlockingHook)>(orig_func->second)(lpBlockFunc);
}

[[= gems::IATPatch]] int WINAPI WSACancelBlockingCall()
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("WSACancelBlockingCall");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&WSACancelBlockingCall)>(orig_func->second)();
}

[[= gems::IATPatch]] ::HANDLE WINAPI
WSAAsyncGetServByName(::HWND hWnd, ::u_int wMsg, const char *name, const char *proto, char *buf, int buflen)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("WSAAsyncGetServByName");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&WSAAsyncGetServByName)>(orig_func->second)(hWnd, wMsg, name, proto, buf, buflen);
}

[[= gems::IATPatch]] ::HANDLE WINAPI
WSAAsyncGetServByPort(::HWND hWnd, ::u_int wMsg, int port, const char *proto, char *buf, int buflen)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("WSAAsyncGetServByPort");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&WSAAsyncGetServByPort)>(orig_func->second)(hWnd, wMsg, port, proto, buf, buflen);
}

[[= gems::IATPatch]] ::HANDLE WINAPI
WSAAsyncGetProtoByName(::HWND hWnd, ::u_int wMsg, const char *name, char *buf, int buflen)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("WSAAsyncGetProtoByName");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&WSAAsyncGetProtoByName)>(orig_func->second)(hWnd, wMsg, name, buf, buflen);
}

[[= gems::IATPatch]] ::HANDLE WINAPI
WSAAsyncGetProtoByNumber(::HWND hWnd, ::u_int wMsg, int number, char *buf, int buflen)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("WSAAsyncGetProtoByNumber");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&WSAAsyncGetProtoByNumber)>(orig_func->second)(hWnd, wMsg, number, buf, buflen);
}

[[= gems::IATPatch]] ::HANDLE WINAPI
WSAAsyncGetHostByName(::HWND hWnd, ::u_int wMsg, const char *name, char *buf, int buflen)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("WSAAsyncGetHostByName");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&WSAAsyncGetHostByName)>(orig_func->second)(hWnd, wMsg, name, buf, buflen);
}

[[= gems::IATPatch]] ::HANDLE WINAPI
WSAAsyncGetHostByAddr(::HWND hWnd, ::u_int wMsg, const char *addr, int len, int type, char *buf, int buflen)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("WSAAsyncGetHostByAddr");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&WSAAsyncGetHostByAddr)>(
        orig_func->second)(hWnd, wMsg, addr, len, type, buf, buflen);
}

[[= gems::IATPatch]] int WINAPI WSACancelAsyncRequest(::HANDLE hAsyncTaskHandle)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("WSACancelAsyncRequest");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&WSACancelAsyncRequest)>(orig_func->second)(hAsyncTaskHandle);
}

[[= gems::IATPatch]] int WINAPI WSAAsyncSelect(::SOCKET s, ::HWND hWnd, ::u_int wMsg, long lEvent)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("WSAAsyncSelect");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&WSAAsyncSelect)>(orig_func->second)(s, hWnd, wMsg, lEvent);
}

[[= gems::IATPatch]] int WINAPI WSARecvEx(::SOCKET s, char *buf, int len, int *flags)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("WSARecvEx");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&WSARecvEx)>(orig_func->second)(s, buf, len, flags);
}

[[= gems::IATPatch]] ::BOOL WINAPI TransmitFile(
    ::SOCKET hSocket,
    ::HANDLE hFile,
    ::DWORD nNumberOfBytesToWrite,
    ::DWORD nNumberOfBytesPerSend,
    ::LPOVERLAPPED lpOverlapped,
    ::LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers,
    ::DWORD dwReserved)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("TransmitFile");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&TransmitFile)>(orig_func->second)(
        hSocket, hFile, nNumberOfBytesToWrite, nNumberOfBytesPerSend, lpOverlapped, lpTransmitBuffers, dwReserved);
}

[[= gems::IATPatch]] ::BOOL WINAPI AcceptEx(
    ::SOCKET sListenSocket,
    ::SOCKET sAcceptSocket,
    ::PVOID lpOutputBuffer,
    ::DWORD dwReceiveDataLength,
    ::DWORD dwLocalAddressLength,
    ::DWORD dwRemoteAddressLength,
    ::LPDWORD lpdwBytesReceived,
    ::LPOVERLAPPED lpOverlapped)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("AcceptEx");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&AcceptEx)>(orig_func->second)(
        sListenSocket,
        sAcceptSocket,
        lpOutputBuffer,
        dwReceiveDataLength,
        dwLocalAddressLength,
        dwRemoteAddressLength,
        lpdwBytesReceived,
        lpOverlapped);
}

[[= gems::IATPatch]] void WINAPI GetAcceptExSockaddrs(
    ::PVOID lpOutputBuffer,
    ::DWORD dwReceiveDataLength,
    ::DWORD dwLocalAddressLength,
    ::DWORD dwRemoteAddressLength,
    ::sockaddr **LocalSockaddr,
    ::LPINT LocalSockaddrLength,
    ::sockaddr **RemoteSockaddr,
    ::LPINT RemoteSockaddrLength)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("GetAcceptExSockaddrs");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    reinterpret_cast<decltype(&GetAcceptExSockaddrs)>(orig_func->second)(
        lpOutputBuffer,
        dwReceiveDataLength,
        dwLocalAddressLength,
        dwRemoteAddressLength,
        LocalSockaddr,
        LocalSockaddrLength,
        RemoteSockaddr,
        RemoteSockaddrLength);
}

[[= gems::IATPatch]] ::INT WINAPI
EnumProtocolsA(::LPINT lpiProtocols, ::LPVOID lpProtocolBuffer, ::LPDWORD lpdwBufferLength)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("EnumProtocolsA");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&EnumProtocolsA)>(
        orig_func->second)(lpiProtocols, lpProtocolBuffer, lpdwBufferLength);
}

[[= gems::IATPatch]] ::INT WINAPI GetAddressByNameA(
    ::DWORD dwNameSpace,
    ::LPGUID lpServiceType,
    ::LPSTR lpServiceName,
    ::LPINT lpiProtocols,
    ::DWORD dwResolution,
    ::LPSERVICE_ASYNC_INFO lpServiceAsyncInfo,
    ::LPVOID lpCsaddrBuffer,
    ::LPDWORD lpdwBufferLength,
    ::LPSTR lpAliasBuffer,
    ::LPDWORD lpdwAliasBufferLength)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("GetAddressByNameA");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&GetAddressByNameA)>(orig_func->second)(
        dwNameSpace,
        lpServiceType,
        lpServiceName,
        lpiProtocols,
        dwResolution,
        lpServiceAsyncInfo,
        lpCsaddrBuffer,
        lpdwBufferLength,
        lpAliasBuffer,
        lpdwAliasBufferLength);
}

[[= gems::IATPatch]] ::INT WINAPI GetTypeByNameA(::LPSTR lpServiceName, ::LPGUID lpServiceType)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("GetTypeByNameA");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&GetTypeByNameA)>(orig_func->second)(lpServiceName, lpServiceType);
}

[[= gems::IATPatch]] ::INT WINAPI GetNameByTypeA(::LPGUID lpServiceType, ::LPSTR lpServiceName, ::DWORD dwNameLength)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("GetNameByTypeA");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&GetNameByTypeA)>(orig_func->second)(lpServiceType, lpServiceName, dwNameLength);
}

[[= gems::IATPatch]] ::INT WINAPI SetServiceA(
    ::DWORD dwNameSpace,
    ::DWORD dwOperation,
    ::DWORD dwFlags,
    ::LPSERVICE_INFOA lpServiceInfo,
    ::LPSERVICE_ASYNC_INFO lpServiceAsyncInfo,
    ::LPDWORD lpdwStatusFlags)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("SetServiceA");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&SetServiceA)>(
        orig_func->second)(dwNameSpace, dwOperation, dwFlags, lpServiceInfo, lpServiceAsyncInfo, lpdwStatusFlags);
}

[[= gems::IATPatch]] ::INT WINAPI GetServiceA(
    ::DWORD dwNameSpace,
    ::LPGUID lpGuid,
    ::LPSTR lpServiceName,
    ::DWORD dwProperties,
    ::LPVOID lpBuffer,
    ::LPDWORD lpdwBufferSize,
    ::LPSERVICE_ASYNC_INFO lpServiceAsyncInfo)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("GetServiceA");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&GetServiceA)>(orig_func->second)(
        dwNameSpace, lpGuid, lpServiceName, dwProperties, lpBuffer, lpdwBufferSize, lpServiceAsyncInfo);
}

[[= gems::IATPatch]] ::INT WINAPI
EnumProtocolsW(::LPINT lpiProtocols, ::LPVOID lpProtocolBuffer, ::LPDWORD lpdwBufferLength)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("EnumProtocolsW");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&EnumProtocolsW)>(
        orig_func->second)(lpiProtocols, lpProtocolBuffer, lpdwBufferLength);
}

[[= gems::IATPatch]] ::INT WINAPI GetAddressByNameW(
    ::DWORD dwNameSpace,
    ::LPGUID lpServiceType,
    ::LPWSTR lpServiceName,
    ::LPINT lpiProtocols,
    ::DWORD dwResolution,
    ::LPSERVICE_ASYNC_INFO lpServiceAsyncInfo,
    ::LPVOID lpCsaddrBuffer,
    ::LPDWORD lpdwBufferLength,
    ::LPWSTR lpAliasBuffer,
    ::LPDWORD lpdwAliasBufferLength)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("GetAddressByNameW");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&GetAddressByNameW)>(orig_func->second)(
        dwNameSpace,
        lpServiceType,
        lpServiceName,
        lpiProtocols,
        dwResolution,
        lpServiceAsyncInfo,
        lpCsaddrBuffer,
        lpdwBufferLength,
        lpAliasBuffer,
        lpdwAliasBufferLength);
}

[[= gems::IATPatch]] ::INT WINAPI GetTypeByNameW(::LPWSTR lpServiceName, ::LPGUID lpServiceType)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("GetTypeByNameW");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&GetTypeByNameW)>(orig_func->second)(lpServiceName, lpServiceType);
}

[[= gems::IATPatch]] ::INT WINAPI GetNameByTypeW(::LPGUID lpServiceType, ::LPWSTR lpServiceName, ::DWORD dwNameLength)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("GetNameByTypeW");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&GetNameByTypeW)>(orig_func->second)(lpServiceType, lpServiceName, dwNameLength);
}

[[= gems::IATPatch]] ::INT WINAPI SetServiceW(
    ::DWORD dwNameSpace,
    ::DWORD dwOperation,
    ::DWORD dwFlags,
    ::LPSERVICE_INFOW lpServiceInfo,
    ::LPSERVICE_ASYNC_INFO lpServiceAsyncInfo,
    ::LPDWORD lpdwStatusFlags)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("SetServiceW");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&SetServiceW)>(
        orig_func->second)(dwNameSpace, dwOperation, dwFlags, lpServiceInfo, lpServiceAsyncInfo, lpdwStatusFlags);
}

[[= gems::IATPatch]] ::INT WINAPI GetServiceW(
    ::DWORD dwNameSpace,
    ::LPGUID lpGuid,
    ::LPWSTR lpServiceName,
    ::DWORD dwProperties,
    ::LPVOID lpBuffer,
    ::LPDWORD lpdwBufferSize,
    ::LPSERVICE_ASYNC_INFO lpServiceAsyncInfo)
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("GetServiceW");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&GetServiceW)>(orig_func->second)(
        dwNameSpace, lpGuid, lpServiceName, dwProperties, lpBuffer, lpdwBufferSize, lpServiceAsyncInfo);
}

[[= gems::IATPatch]] int WINAPI WEP()
{
    auto orig_func = std::ranges::cend(gems::impl::g_iat_entries);

    {
        auto lock = std::scoped_lock(gems::impl::g_mutex);

        gems::impl::try_load_iat_hooks();

        orig_func = gems::impl::g_iat_entries.find("WEP");
        gems::ensure(orig_func != std::ranges::cend(gems::impl::g_iat_entries), "could not find iat entry");
    }

    return reinterpret_cast<decltype(&WEP)>(orig_func->second)();
}

::BOOL WINAPI DllMain(::HMODULE module, ::DWORD reason, void *)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        ::DisableThreadLibraryCalls(module);
        gems::log("DllMain called for wsock32.dll");

        gems::impl::g_proxied_wsock32 = module;
    }
    return TRUE;
}
}
