#ifndef sdbus_READ_HPP_
#define sdbus_READ_HPP_

#include <sdbus/concepts.hpp>
#include <sdbus/property.hpp>

namespace sdbus
{

static errc read(sd_bus_message* msg, auto& v)
{
    defctx ctx(msg);
    return read(ctx, v);
}

static errc read(subctx& ctx, auto& v)
{
    using type = std::remove_reference_t<decltype(v)>;
    return traits<type>::read_value(ctx, v);
}

static errc read(subctx& ctx, auto&& v)
{
    using type = std::remove_reference_t<decltype(v)>;
    return traits<type>::read_value(ctx, v);
}

struct reader_base
{
    virtual errc read_value(subctx&) = 0;
};

template <typename T>
errc read_string(subctx& ctx, T& v)
{
    string_read_helper rh(v);
    const char* cstr;
    auto ec = read_basic_impl(ctx, rh.sig(), &cstr);
    if (is_error(ec))
    {
        return ec;
    }
    try
    {
        rh = cstr;
    }
    catch (...)
    {
        return errc::no_memory;
    }
    return errc::success;
}

template <typename T>
struct simple_reader : reader_base
{
    constexpr simple_reader(T& v) : _v{v}
    {}

    errc read_value(subctx& ctx) override
    {
        return traits<T>::read_value(ctx, _v);
    }

  private:
    T& _v;
};

template <typename T>
struct variant_reader : simple_reader<T>
{
    using simple_reader<T>::simple_reader;
};

struct variant_reader_base : reader_base
{
    using callback_t = bool (variant_reader_base::*)(subctx&, const char*, errc&);
    using callbacks_t = std::span<const callback_t>;

    variant_reader_base(const callbacks_t& callbacks) : _callbacks(callbacks)
    {}

    errc read_value(subctx& ctx) override;

  private:
    const callbacks_t& _callbacks;
};

template <concepts::Variant T>
struct variant_reader<T> : variant_reader_base
{
    template <typename V>
    bool read_one(subctx& ctx, const char* sig, errc& ec)
    {
        if (sig != traits<V>::sig)
        {
            return false;
        }

        T v;

        ec = read(ctx, v);
        if (no_error(ec))
        {
            try
            {
                _v = std::move(v);
            }
            catch (std::bad_alloc&)
            {
                ec = errc::no_memory;
            }
            catch (...)
            {
                ec = errc::bad_exception;
            }
        }

        return true;
    }

    template <typename F>
    struct callbacks_holder;

    template <typename... Ts>
    struct callbacks_holder<std::variant<Ts...>>
    {
        static constexpr auto cb = {static_cast<callback_t>(&variant_reader<T>::read_one<Ts>)...};
    };

    variant_reader(T& v) : variant_reader_base(callbacks_holder<T>::cb), _v(v)
    {}

  private:
    T& _v;
};

template <typename T>
errc read_variant(subctx& ctx, T& v)
{
    auto rdr = variant_reader<T>(v);
    return read_variant_impl(ctx, rdr);
}

template <typename T>
struct dict_entry_reader : reader_base
{
    using T1 = typename T::first_type;
    using T2 = typename T::second_type;

    constexpr dict_entry_reader(T& v) : _v{v}
    {}

    errc read_value(subctx& ctx) override
    {
        auto ec = read(ctx, _v.first);
        if (no_error(ec))
        {
            ec = read(ctx, _v.second);
        }
        return ec;
    }

  private:
    T& _v;
};

template <typename T>
errc read_dict_entry(subctx& ctx, T& v)
{
    auto r = dict_entry_reader<T>(v);
    return read_dict_entry_impl(ctx, r);
}

template <typename T>
struct item_reader : simple_reader<T>
{
    using simple_reader<T>::simple_reader;
};

template <typename T>
struct value_type_traits
{
    using type = T;
};
template <typename T1, typename T2>
struct value_type_traits<std::pair<const T1, T2>>
{
    using type = std::pair<T1, T2>;
};

template <typename T, typename V = typename T::value_type>
struct container_reader : reader_base
{
    using value_type = typename value_type_traits<V>::type;

    container_reader(T& container) : _container(container)
    {}

    errc read_value(subctx& ctx) override
    {
        if (ctx.index() == _container.size())
        {
            return errc::out_of_space;
        }
        auto& v = _container[ctx.index()];
        return traits<value_type>::read_value(ctx, v);
    }

  private:
    T& _container;
};

template <concepts::Emplaceable T, typename V>
struct container_reader<T, V> : reader_base
{
    using value_type = typename value_type_traits<V>::type;

    container_reader(T& container) : _container(container)
    {}

    errc read_value(subctx& ctx) override
    {
        value_type _value;
        auto ec = traits<value_type>::read_value(ctx, _value);
        if (no_error(ec))
        {
            try
            {
                if constexpr (concepts::HasEmplace<T>)
                {
                    _container.emplace(std::move(_value));
                }
                else
                {
                    _container.emplace(_container.end(), std::move(_value));
                }
            }
            catch (std::bad_alloc&)
            {
                ec = errc::no_memory;
            }
            catch (...)
            {
                ec = errc::bad_exception;
            }
        }
        return ec;
    }

  private:
    T& _container;
};

template <concepts::Container T>
struct item_reader<T> : container_reader<T>
{
    using container_reader<T>::container_reader;
};

template <typename C, typename V>
struct item_reader<array_of_helper<C, V>> : container_reader<C, V>
{
    constexpr item_reader(array_of_helper<C, V>& v) : container_reader<C, V>(v.ref)
    {}
};

struct dict_reader_base : reader_base
{
    errc read_value(subctx& ctx) override;

  protected:
    virtual errc read_entry(subctx& ctx, const char*) = 0;
};

template <typename T>
struct dict_reader : dict_reader_base
{
    dict_reader(T& v) : _compound(v)
    {}

  protected:
    errc read_entry(subctx& ctx, const char* name)
    {
        auto _descs = make_descs();
        auto iter = std::ranges::find_if(
            _descs, [&](const auto& item) -> bool { return item.name() == name; });

        if (iter == _descs.end())
        {
            return errc::unknown_property;
        }
        return iter->read_value(ctx, _compound);
    }

  private:
    static constexpr auto make_descs()
    {
        using props = typename T::dict_t;
        return property_desc_maker<props>::make_descs();
    }

  private:
    T& _compound;
};

template <concepts::Dict T>
struct item_reader<T> : dict_reader<T>
{
    using dict_reader<T>::dict_reader;
};

template <typename T>
static errc read_array(subctx& ctx, T& v)
{
    item_reader<T> r(v);
    return read_array_impl(ctx, r);
}
} // namespace sdbus

#endif // sdbus_READ_HPP_
