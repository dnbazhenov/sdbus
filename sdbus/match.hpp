#pragma once

#include <sdbus/sig_string.hpp>

#include <string_view>

namespace sdbus
{

constexpr auto signal() noexcept
{
    return sig_string("type='signal',");
}

template <size_t M>
constexpr auto sender(const sig_string<M> s) noexcept
{
    return sig_string("sender='") + s + sig_string("',");
}

template <size_t M>
constexpr auto path(const sig_string<M> s) noexcept
{
    return sig_string("path='") + s + sig_string("',");
}

template <size_t M>
constexpr auto path_namespace(const sig_string<M> s) noexcept
{
    return sig_string("path_namespace='") + s + sig_string("',");
}

template <size_t M>
constexpr auto interface(const sig_string<M> s) noexcept
{
    return sig_string("interface='") + s + sig_string("',");
}

template <size_t M>
constexpr auto member(const sig_string<M> s) noexcept
{
    return sig_string("member='") + s + sig_string("',");
}

template <size_t M>
constexpr auto arg0(const sig_string<M> s) noexcept
{
    return sig_string("arg0='") + s + sig_string("',");
}

} // namespace sdbus
