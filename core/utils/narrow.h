#pragma once

#include <ranges>
#include <string>
#include <string_view>

#include <windows.h>

#include "core/utils/log.h"

namespace gems
{
namespace impl
{
typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;

}

inline auto narrow(std::wstring_view str) -> std::string
{
    auto length = ::WideCharToMultiByte(
        CP_UTF8, WC_NO_BEST_FIT_CHARS, std::ranges::data(str), std::ranges::size(str), nullptr, 0, nullptr, nullptr);

    if (length == 0)
    {
        log("failed to get string length");
        return {};
    }

    auto res = std::string(length, '\0');

    length = ::WideCharToMultiByte(
        CP_UTF8,
        WC_NO_BEST_FIT_CHARS,
        std::ranges::data(str),
        std::ranges::size(str),
        std::ranges::data(res),
        std::ranges::size(res),
        nullptr,
        nullptr);

    if (length == 0)
    {
        log("failed to narrow string");
        return {};
    }

    return res;
}

inline auto narrow(const impl::UNICODE_STRING *str) -> std::string
{
    return narrow(std::wstring_view{str->Buffer, str->Length / sizeof(WCHAR)});
}

}
