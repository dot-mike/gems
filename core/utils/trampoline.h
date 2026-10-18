#pragma once

#include <meta>
#include <tuple>
#include <vector>

#include <windows.h>

#include "core/utils/log.h"

namespace gems
{

template <class T>
struct GetArgs;

template <class R, class... Args>
struct GetArgs<R WINAPI(Args...)>
{
    using type = std::tuple<Args...>;
};

template <auto Orig, auto Proxy, class... Args>
auto WINAPI trampoline(Args... args)
{
    return Proxy(Orig, args...);
}

template <auto Orig>
inline auto original_function = decltype(Orig){};

template <auto Orig, auto Proxy, class... Args>
auto WINAPI original_trampoline(Args... args)
{
    return Proxy(original_function<Orig>, args...);
}

template <class ArgsTuple>
consteval auto build_trampoline(
    std::meta::info orig,
    std::meta::info proxy,
    std::meta::info trampoline_template = ^^trampoline) -> std::meta::info
{
    auto args_meta = std::meta::template_arguments_of(std::meta::dealias(^^ArgsTuple));
    args_meta.insert(std::ranges::begin(args_meta), proxy);
    args_meta.insert(std::ranges::begin(args_meta), orig);

    return std::meta::substitute(trampoline_template, args_meta);
}

}
