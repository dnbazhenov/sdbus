#ifndef sdbus_TRAITS_HPP_
#define sdbus_TRAITS_HPP_

#include <sdbus/concepts.hpp>
#include <sdbus/helpers.hpp>

namespace sdbus
{

template <typename T>
consteval auto basic_signature()
{
    if constexpr (std::same_as<T, bool>)
    {
        return sig_string("b");
    }
    else if constexpr (std::same_as<T, uint8_t>)
    {
        return sig_string("y");
    }
    else if constexpr (std::same_as<T, int16_t>)
    {
        return sig_string("n");
    }
    else if constexpr (std::same_as<T, uint16_t>)
    {
        return sig_string("q");
    }
    else if constexpr (std::same_as<T, int32_t>)
    {
        return sig_string("i");
    }
    else if constexpr (std::same_as<T, uint32_t>)
    {
        return sig_string("u");
    }
    else if constexpr (std::same_as<T, int64_t>)
    {
        return sig_string("x");
    }
    else if constexpr (std::same_as<T, long long>)
    {
        return sig_string("x");
    }
    else if constexpr (std::same_as<T, uint64_t>)
    {
        return sig_string("t");
    }
    else if constexpr (std::same_as<T, unsigned long long>)
    {
        return sig_string("t");
    }
    else if constexpr (std::same_as<T, double>)
    {
        return sig_string("d");
    }
    else
    {
        static_assert(false, "Type-specific signature is not defined.");
    }
}

template <typename T>
struct default_basic_traits
{
    static constexpr auto sig = basic_signature<T>();

    template <typename F>
    static errc read_value(subctx& ctx, F&& v)
    {
        basic_read_helper rh(v);
        auto ec = read_basic(ctx, rh.ref());

        if (ec == errc::success)
        {
            rh.commit();
        }

        return ec;
    }

    template <typename F>
    static errc write_value(subctx& ctx, F&& v)
    {
        basic_write_helper wh(std::forward<F>(v));
        return write_basic(ctx, wh.ref());
    }
};

template <concepts::Basic T>
struct default_traits<T> : default_basic_traits<T>
{};

template <typename T>
consteval auto string_signature()
{
    return sig_string("s");
}

template <typename T>
struct default_string_traits
{
    static constexpr auto sig = string_signature<T>();

    template <typename F>
    static errc read_value(subctx& ctx, F&& v)
    {
        return read_string(ctx, std::forward<F>(v));
    }

    template <typename F>
    static errc write_value(subctx& ctx, F&& v)
    {
        return write_string(ctx, std::forward<F>(v));
    }
};

template <concepts::String T>
struct default_traits<T> : default_string_traits<T>
{};

template <typename T>
struct default_traits<basic_objpath<T>> : default_string_traits<basic_objpath<T>>
{
    static constexpr auto sig = sig_string("o");
};

template <typename T>
struct default_write_only_string_traits : default_string_traits<T>
{
    template <typename F>
    static errc read_value(subctx& ctx, F&& v)
    {
        static_assert(false, "Can't read write-only string.");
        return errc::read_error;
    }
};

template <concepts::WriteOnlyString T>
struct default_traits<T> : default_write_only_string_traits<T>
{};

template <typename T>
struct default_variant_traits
{
    static constexpr auto sig = sig_string("v");

    template <typename F>
    static errc read_value(subctx& ctx, F&& v)
    {
        return read_variant(ctx, std::forward<F>(v));
    }

    template <typename F>
    static errc write_value(subctx& ctx, F&& v)
    {
        return write_variant(ctx, std::forward<F>(v));
    }
};

template <concepts::Variant T>
struct default_traits<T> : default_variant_traits<T>
{};

template <typename T>
struct default_array_traits
{
    static constexpr auto sig = "a" + traits<typename T::value_type>::sig;

    template <typename F>
    static errc read_value(subctx& ctx, F&& v)
    {
        return read_array(ctx, std::forward<F>(v));
    }

    template <typename F>
    static errc write_value(subctx& ctx, F&& v)
    {
        return write_array(ctx, std::forward<F>(v));
    }
};

template <concepts::Container T>
struct default_traits<T> : default_array_traits<T>
{};

template <typename T>
struct default_dict_entry_traits
{
    using first_type = typename T::first_type;
    using second_type = typename T::second_type;

    static constexpr auto sig = "{" + traits<first_type>::sig + traits<second_type>::sig + "}";

    static errc read_value(subctx& ctx, T& v)
    {
        return read_dict_entry(ctx, v);
    }

    static errc write_value(subctx& ctx, const T& v)
    {
        return write_dict_entry(ctx, v);
    }
};

template <concepts::DictEntry T>
struct default_traits<T> : default_dict_entry_traits<T>
{};

template <typename T>
struct default_dict_traits
{
    static constexpr auto sig = sig_string("a{sv}");

    static errc read_value(subctx& ctx, T& v)
    {
        return read_array(ctx, v);
    }

    static errc write_value(subctx& ctx, const T& v)
    {
        return write_array(ctx, v);
    }
};

template <concepts::Dict T>
struct default_traits<T> : default_dict_traits<T>
{};

template <typename T, typename F>
struct default_traits<as_helper<T, F>>
{
    static constexpr auto sig = traits<T>::sig;

    static errc read_value(subctx& ctx, as_helper<T, F> v)
    {
        return traits<T>::read_value(ctx, v);
    }

    static errc write_value(subctx& ctx, const as_helper<T, F>& v)
    {
        return traits<T>::write_value(ctx, v);
    }
};

template <typename T>
struct default_traits<variant_tag<T>>
{
    static constexpr auto sig = traits<T>::sig;

    template <typename U, typename F>
    static errc read_value(subctx& ctx, as_helper<U, F>& v)
    {
        return read_variant(ctx, v.ref);
    }

    template <typename U, typename F>
    static errc write_value(subctx& ctx, const as_helper<U, F>& v)
    {
        return write_variant(ctx, v.ref);
    }
};

template <typename C, typename V>
struct default_traits<array_of_helper<C, V>> : default_array_traits<C>
{};

} // namespace sdbus

#endif /* sdbus_TRAITS_HPP_ */
