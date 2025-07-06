#ifndef sdbus_WRITE_HPP_
#define sdbus_WRITE_HPP_

#include <sdbus/concepts.hpp>
#include <sdbus/property.hpp>

namespace sdbus
{

template <typename T>
static errc write(sd_bus_message* msg, const T& v)
{
    defctx ctx(msg);
    return write(ctx, v);
}

template <typename T>
static errc write(subctx& ctx, const T& v)
{
    using type = std::decay_t<const T>;
    return traits<type>::write_value(ctx, v);
}

template <typename T>
static errc write_string(subctx& ctx, const T& v)
{
    string_write_helper wh(v);
    return write_basic_impl(ctx, wh.sig(), wh.data());
}

struct writer_base
{
    virtual errc write_value(subctx&) = 0;
};

template <typename T>
struct simple_writer : writer_base
{
    constexpr simple_writer(const T& v) : _v{v}
    {}

    const char* signature() const
    {
        return traits<T>::sig;
    }

    errc write_value(subctx& ctx) override
    {
        return traits<T>::write_value(ctx, _v);
    }

  private:
    const T& _v;
};

template <typename T>
struct variant_writer : simple_writer<T>
{
    using simple_writer<T>::simple_writer;
};

template <concepts::Variant T>
struct variant_writer<T> : writer_base
{
    variant_writer(const T& v) : _value(v)
    {}

    const char* signature() const
    {
        return std::visit(
            [](const auto& v) -> const char* {
                using type = std::decay_t<decltype(v)>;
                return traits<type>::sig;
            },
            _value);
    }

    errc write_value(subctx& ctx) override
    {
        return std::visit(
            [&](const auto& v) -> errc {
                using type = std::decay_t<decltype(v)>;
                if constexpr (traits<type>::sig == sig_string(""))
                {
                    return errc::invalid_type;
                }
                else
                {
                    return write(ctx, v);
                }
            },
            _value);
    }

  private:
    const T& _value;
};

template <typename T>
errc write_variant(subctx& ctx, const T& v)
{
    auto w = variant_writer<T>(v);
    return write_variant_impl(ctx, w.signature(), w);
}

template <typename T>
struct dict_entry_writer : writer_base
{
    using T1 = typename T::first_type;
    using T2 = typename T::second_type;

    static constexpr auto sig = traits<T1>::sig + traits<T2>::sig;

    constexpr dict_entry_writer(const T& v) : _v{v}
    {}

    const char* signature() const
    {
        return sig;
    }

    errc write_value(subctx& ctx) override
    {
        auto ec = write(ctx, _v.first);
        if (no_error(ec))
        {
            ec = write(ctx, _v.second);
        }
        return ec;
    }

  private:
    const T& _v;
};

template <typename T>
errc write_dict_entry(subctx& ctx, const T& v)
{
    auto w = dict_entry_writer<T>(v);
    return write_dict_entry_impl(ctx, w.signature(), w);
}

template <typename T>
struct item_writer : simple_writer<T>
{
    using simple_writer<T>::simple_writer;
};

template <typename T>
struct container_writer_traits
{
    using item_type = typename T::value_type;
    using const_iterator = typename T::const_iterator;
};

template <typename T, typename V = typename T::value_type>
struct container_writer : writer_base
{
    using item_type = V;
    using iterator = typename T::const_iterator;

    container_writer(const T& v) : _pos(v.begin()), _size(v.size())
    {}

    const char* signature() const
    {
        return static_cast<const char*>(traits<item_type>::sig);
    }

    size_t size() const
    {
        return _size;
    }

    errc write_value(subctx& ctx)
    {
        return write<item_type>(ctx, *_pos++);
    }

  private:
    iterator _pos;
    size_t _size;
};

template <concepts::Container T>
struct item_writer<T> : container_writer<T>
{
    using container_writer<T>::container_writer;
};

template <typename C, typename V>
struct item_writer<array_of_helper<C, V>> : container_writer<C, V>
{
    constexpr item_writer(array_of_helper<C, V>& v) : container_writer<C, V>(v.ref)
    {}
};

struct dict_writer_base : writer_base
{
    const char* signature() const
    {
        return "{sv}";
    }
};

template <typename T>
struct dict_writer : dict_writer_base
{
    dict_writer(const T& v) : _compound(v), _descs(make_descs())
    {}

    size_t size() const
    {
        return _descs.size();
    }

  protected:
    errc write_value(subctx& ctx) override
    {
        const auto& desc = _descs[ctx.index()];
        auto ec = write(ctx, desc.name());
        if (no_error(ec))
        {
            ec = desc.write_value(ctx, _compound);
        }

        return ec;
    }

  private:
    static constexpr auto make_descs()
    {
        using props = typename T::dict_t;
        return property_desc_maker<props>::make_descs();
    }

  private:
    const T& _compound;
    property_descs<T> _descs;
};

template <concepts::Dict T>
struct item_writer<T> : dict_writer<T>
{
    using dict_writer<T>::dict_writer;
};

template <typename T>
static errc write_array(subctx& ctx, const T& v)
{
    item_writer<T> w(v);
    return write_array_impl(ctx, w.signature(), w.size(), w);
}

} // namespace sdbus

#endif /* sdbus_WRITE_HPP_ */
