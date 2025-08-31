#ifndef sdbus_SIG_STRING_HPP_
#define sdbus_SIG_STRING_HPP_

#include <algorithm>
#include <cstring>
#include <string_view>

namespace sdbus
{

template <size_t Nm>
struct sig_string
{
    using value_type = char;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = value_type*;
    using const_iterator = const value_type*;

    constexpr sig_string() = default;
    constexpr sig_string(const char (&arr)[Nm]) noexcept
    {
        std::copy_n(arr, Nm, value);
    }
    constexpr sig_string(const char* s, size_t size) noexcept
    {
        if (size >= Nm)
        {
            size = Nm - 1;
        }
        std::copy_n(s, size, value);
        value[size] = 0;
    }
    constexpr iterator begin() noexcept
    {
        return value;
    }
    constexpr const_iterator begin() const noexcept
    {
        return value;
    }
    constexpr iterator end() noexcept
    {
        return value + Nm;
    }
    constexpr const_iterator end() const noexcept
    {
        return value + Nm;
    }
    constexpr const_pointer c_str() const noexcept
    {
        return value;
    }
    constexpr pointer data() noexcept
    {
        return value;
    }
    constexpr const_pointer data() const noexcept
    {
        return value;
    }
    constexpr std::string_view sv() const noexcept
    {
        return {value, Nm - 1};
    }
    constexpr operator std::string_view() const noexcept
    {
        return std::string_view(data(), Nm - 1);
    }
    constexpr operator const char*() const
    {
        return value;
    }
    constexpr bool empty() const
    {
        return Nm == 1;
    }
    char value[Nm];
};

template <size_t N, size_t M>
constexpr sig_string<N + M - 1> operator+(const sig_string<N>& lhs, sig_string<M> rhs)
{
    sig_string<N + M - 1> result;

    std::copy_n(lhs.value, N - 1, result.value);
    std::copy_n(rhs.value, M, result.value + N - 1);

    return result;
}

template <size_t N, size_t M>
constexpr sig_string<N + M - 1> operator+(const sig_string<N>& lhs, const char (&rhs)[M])
{
    sig_string<N + M - 1> result;

    std::copy_n(lhs.value, N - 1, result.value);
    std::copy_n(rhs, M, result.value + N - 1);

    return result;
}

template <size_t N, size_t M>
constexpr sig_string<N + M - 1> operator+(const char (&lhs)[N], const sig_string<M>& rhs)
{
    sig_string<N + M - 1> result;

    std::copy_n(lhs, N - 1, result.value);
    std::copy_n(rhs.value, M, result.value + N - 1);

    return result;
}

template <size_t N, size_t M>
constexpr bool operator==(const sig_string<N>& lhs, const sig_string<M>& rhs)
{
    if constexpr (N != M)
    {
        return false;
    }

    return std::string_view(lhs.data(), N) == std::string_view(rhs.data(), M);
}

template <size_t N>
static inline bool operator==(const sig_string<N>& lhs, const char* n)
{
    return !std::strcmp(n, lhs.data());
}

template <size_t N>
static inline bool operator==(const char* n, const sig_string<N>& lhs)
{
    return !std::strcmp(n, lhs.data());
}

namespace string_literals
{
template <sig_string A>
constexpr auto operator""_ss()
{
    return A;
}
} // namespace string_literals

} // namespace sdbus

#endif // sdbus_SIG_STRING_HPP_
