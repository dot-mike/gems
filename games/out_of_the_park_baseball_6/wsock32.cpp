#include <algorithm>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>

#pragma push_macro("WINSOCK_API_LINKAGE")
#undef WINSOCK_API_LINKAGE
#define WINSOCK_API_LINKAGE
#include <winsock.h>
#pragma pop_macro("WINSOCK_API_LINKAGE")

#include <windows.h>

#include <tlhelp32.h>

#include "core/utils/annotations.h"
#include "core/utils/formatter.h"
#include "core/utils/iat_patcher.h"
#include "core/utils/log.h"

namespace gems
{
namespace impl
{

enum class RecvAction
{
    BUILD_RESPONSE,
    EMPTY,
    NORMAL,
};

auto g_recv_action = RecvAction::NORMAL;

auto machine_secret() -> std::optional<std::string>
{
    const auto snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, ::GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        log("CreateToolhelp32Snapshot failed: {}", ::GetLastError());
        return std::nullopt;
    }

    auto module = ::MODULEENTRY32{};
    module.dwSize = sizeof(module);

    if (!::Module32First(snapshot, &module))
    {
        log("Module32First failed: {}", ::GetLastError());
        return std::nullopt;
    }

    do
    {
        const auto path_view = std::string_view(module.szExePath);
        if (path_view.contains("\\Temp\\"))
        {
            using FuncSig = char *(__cdecl *)();
            const auto func = reinterpret_cast<FuncSig>(module.modBaseAddr + 0x7D51F);
            const auto *encoded = func();

            auto res = std::string(std::strlen(encoded), '\0');
            auto res_index = std::size_t{};

            const int shift = encoded[0] - '0';

            while (*++encoded)
            {
                const int digit = (*encoded - '0' - shift + 10) % 10;
                res[res_index++] = char('0' + digit);
            }

            res.resize(res_index);

            return res;
        }
    } while (::Module32Next(snapshot, &module));

    return std::nullopt;
}

auto to_base33(std::uint32_t value) -> std::string
{
    constexpr char alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVW";
    auto reversed = std::string(7, '\0');
    auto count = 0u;

    do
    {
        reversed[count++] = alphabet[value % 33];
        value /= 33;
    } while (value != 0);

    auto res = std::string{};
    while (count != 0)
    {
        res += reversed[--count];
    }

    return res;
}

auto build_license(
    std::string_view machine_secret,
    std::uint32_t vendor_id,
    std::uint32_t product_id,
    std::string_view security_text,
    std::string_view header) -> std::optional<std::string>
{
    auto machine_id = 0u;
    const char *end = std::ranges::data(machine_secret) + std::ranges::size(machine_secret);

    const auto parsed = std::from_chars(std::ranges::data(machine_secret), end, machine_id, 10);

    if (parsed.ec != std::errc{} || parsed.ptr != end)
    {
        return std::nullopt;
    }

    auto license = std::string{};
    license.reserve(40);
    license.append(header);

    auto checksum = 0u;

    const auto append_field = [&](std::uint32_t value)
    {
        const auto encoded = to_base33(value);
        license += encoded;
        checksum += std::ranges::fold_left(encoded, 0, std::plus<>{});
    };

    const auto append_separator = [&]
    {
        license.push_back('Z');
        checksum += 'Z';
    };

    append_field(machine_id);
    append_separator();
    append_field(vendor_id);
    append_separator();
    append_field(product_id);

    const auto checksum_str = to_base33(checksum);

    license.push_back('Z');
    license += checksum_str;
    license += security_text;

    return license;
}

}

[[= IATProxy]] __declspec(dllexport) ::SOCKET WINAPI socket(decltype(&::socket) orig, int af, int type, int protocol)
{
    log("socket({}, {}, {})", af, type, protocol);

    const auto res = orig(af, type, protocol);

    log("socket res: {}", res);

    return res;
}

[[= IATProxy]] unsigned long WINAPI inet_addr(decltype(&::inet_addr) orig, const char *cp)
{
    log("inet_addr({})", cp);

    const auto res = orig(cp);

    log("inet_addr res: {}", res);

    return res;
}

[[= IATProxy]] ::hostent *WINAPI gethostbyname(decltype(&::gethostbyname) orig, const char *name)
{
    log("gethostbyname({})", name);

    const auto res = orig(name);

    if (res)
    {
        log("gethostbyname res: {}", *res);
    }
    else
    {
        log("gethostbyname res: <null>");
    }

    return res;
}

[[= IATProxy]] char *WINAPI inet_ntoa(decltype(&::inet_ntoa) orig, ::in_addr in)
{
    log("inet_ntoa({})", in);

    const auto res = orig(in);

    log("inet_ntoa res: {}", res);

    return res;
}

// [[= IATProxy]] int WINAPI connect(decltype(&::connect) orig, ::SOCKET s, const ::sockaddr *name, int namelen)
// {
//     log("connect({}, {}, {})", s, *name, namelen);
//
//     const auto res = orig(s, name, namelen);
//
//     log("connect res: {}", res);
//
//     return res;
// }

[[= IATProxy]] int WINAPI send(decltype(&::send) orig, ::SOCKET s, const char *buf, int len, int flags)
{
    log("send({}, {}, {})", s, len, flags);
    log("{}", buf);

    const auto buf_view = std::string_view(buf, len);
    if (buf_view.contains("POST /vtcp/scripts/rq2/getlicense.asp HTTP/1.0"))
    {
        impl::g_recv_action = impl::RecvAction::BUILD_RESPONSE;
    }

    const auto res = orig(s, buf, len, flags);

    log("send res: {}", res);

    return res;
}

[[= IATProxy]] int WINAPI recv(decltype(&::recv) orig, ::SOCKET s, char *buf, int len, int flags)
{
    log("recv({}, {}, {})", s, len, flags);

    auto res = int{};

    if (impl::g_recv_action == impl::RecvAction::EMPTY)
    {
        impl::g_recv_action = impl::RecvAction::NORMAL;
        res = 0;
    }
    else if (impl::g_recv_action == impl::RecvAction::BUILD_RESPONSE)
    {
        const auto secret = impl::machine_secret();
        log("secret: {}", secret ? *secret : "<empty>");

        const auto license = impl::build_license(*secret, 0x507u, 0x0D9F64FDu, "09c34t3f", "AA0AB");
        log("license: {}", license ? *license : "<failed>");

        const auto content = std::format("return-status=ok&vtpLicenseData={}", *license);

        auto response = std::string{};
        response += "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/plain\r\n";
        response += std::format("Content-Length: {}\r\n", std::ranges::size(content));
        response += "Connection: close\r\n";
        response += "\r\n";
        response += content;

        const auto min_len = std::min(static_cast<std::size_t>(len), response.size());
        std::memcpy(buf, response.data(), min_len);
        impl::g_recv_action = impl::RecvAction::EMPTY;

        res = min_len;
    }
    else
    {
        res = orig(s, buf, len, flags);
    }

    log("{}", buf);
    log("recv res: {}", res);

    return res;
}

[[= IATProxy]] __declspec(dllexport) int WINAPI
WSAStartup(decltype(&::WSAStartup) orig, ::WORD wVersionRequested, ::LPWSADATA lpWSAData)
{
    log("WSAStartup({}, {} {})", wVersionRequested, lpWSAData->wVersion, lpWSAData->wHighVersion);

    const auto res = orig(wVersionRequested, lpWSAData);

    log("WSAStartup res: {}", res);

    return res;
}

[[= IATProxy]] ::HANDLE WINAPI WSAAsyncGetHostByName(
    decltype(&::WSAAsyncGetHostByName) orig,
    ::HWND hWnd,
    ::u_int wMsg,
    const char *name,
    char *buf,
    int buflen)
{
    log("WSAAsyncGetHostByName({}, {}, {}, {}, {})",
        static_cast<void *>(hWnd),
        wMsg,
        name,
        static_cast<void *>(buf),
        buflen);

    const auto res = orig(hWnd, wMsg, name, buf, buflen);
    log("WSAAsyncGetHostByName res: {}", res);

    return res;
}

}

#include "core/dll_main/wsock32/wsock32_main.h"
