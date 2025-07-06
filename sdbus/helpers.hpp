#ifndef sdbus_HELPERS_HPP_
#define sdbus_HELPERS_HPP_

#include <sdbus/forwards.hpp>

#include <string_view>

namespace sdbus
{

template <typename C, typename F, typename Ref = F&, typename CRef = const F&>
struct property_ptr
{
    using container_type = C;
    using value_type = F;
    using reference = Ref;
    using const_reference = CRef;

    constexpr property_ptr(value_type container_type::* ptr) : mptr{ptr}
    {}

    value_type container_type::* mptr;
};

template <typename T>
struct basic_objpath : T
{
    using T::T;
    using T::operator=;
};

using objpath = basic_objpath<std::string>;

template <typename T, typename C, typename F>
struct as_mptr_helper
{
    constexpr as_mptr_helper(F C::* ptr) : ptr{ptr}
    {}

    F C::* ptr;
};

template <typename T, typename F>
struct as_helper
{
    using target_type = T;
    using source_type = F;
    constexpr as_helper(F& v) : ref(v)
    {}
    F& ref;
};

template <typename T, typename F>
struct as_helper<T, const F>
{
    using target_type = T;
    using source_type = F;
    const F& ref;
};

template <typename T, typename F>
static constexpr auto as(F& f)
{
    return as_helper<T, F>(f);
}

template <typename T, typename F>
static constexpr auto as(const F& f)
{
    return as_helper<T, const F>(f);
}

template <typename T, typename C, typename F>
static constexpr auto as(F C::* p)
{
    return property_ptr<C, F, as_helper<T, F>, as_helper<T, const F>>(p);
}

template <typename T>
static constexpr auto as_path(T& v)
{
    return as_helper<basic_objpath<T>, T>(v);
}

template <typename T>
static constexpr auto as_path(const T& v)
{
    return as_helper<basic_objpath<T>, const T>(v);
}

template <typename C, typename F>
static constexpr auto as_path(F C::* p)
{
    return property_ptr<C, F, as_helper<basic_objpath<F>, F>, as_helper<basic_objpath<F>, const F>>(
        p);
}

template <typename T>
struct variant_tag
{
    using type = T;
};

template <typename T>
static constexpr auto as_variant(T& v)
{
    return as_helper<variant_tag<T>, T>(v);
}

template <typename T>
static constexpr auto as_variant(const T& v)
{
    return as_helper<variant_tag<T>, const T>(v);
}

template <typename C, typename F>
static constexpr auto as_variant(F C::* p)
{
    return property_ptr<C, F, as_helper<variant_tag<F>, F>, as_helper<variant_tag<F>, const F>>(p);
}

template <typename T>
struct basic_read_helper
{
    using base_type = std::decay_t<T>;
    using reference = base_type&;

    constexpr basic_read_helper(T& r) : _ref(r)
    {}

    constexpr reference ref() const
    {
        return _ref;
    }

    constexpr void commit()
    {}

  private:
    T& _ref;
};

template <typename T, typename F>
struct basic_read_helper<as_helper<T, F>>
{
    using base_type = std::decay_t<T>;
    using reference = base_type&;

    constexpr basic_read_helper(as_helper<T, F> h) : _ref(h.ref)
    {}

    constexpr reference ref() const
    {
        return _value;
    }

    constexpr void commit()
    {
        _ref = static_cast<F>(_value);
    }

  private:
    T _value;
    F& _ref;
};

template <typename T>
struct basic_write_helper
{
    using base_type = std::decay_t<T>;
    using reference = const base_type&;

    constexpr basic_write_helper(T& r) : _ref(r)
    {}

    constexpr reference ref() const
    {
        return _ref;
    }

  private:
    const T& _ref;
};

template <typename T, typename F>
struct basic_write_helper<const as_helper<T, F>>
{
    using base_type = std::decay_t<T>;
    using reference = const base_type&;

    constexpr basic_write_helper(as_helper<T, F> h) : _value(static_cast<T>(h.ref))
    {}

    constexpr reference ref() const
    {
        return _value;
    }

  private:
    T _value;
};

template <typename T>
struct string_read_helper
{
    constexpr explicit string_read_helper(T& v) : _ref(v)
    {}
    constexpr const char* sig() const
    {
        return traits<T>::sig;
    }
    void operator=(const char* data) const
    {
        _ref = data;
    }

  private:
    T& _ref;
};

template <typename T, typename F>
struct string_read_helper<as_helper<T, F>>
{
    constexpr explicit string_read_helper(as_helper<T, F>& v) : _ref(v.ref)
    {}
    constexpr const char* sig() const
    {
        return traits<T>::sig;
    }
    void operator=(const char* data) const
    {
        _ref = data;
    }

  private:
    F& _ref;
};

template <typename T>
struct string_write_helper
{
    constexpr explicit string_write_helper(const T& v) : _data(std::string_view(v).data())
    {}
    constexpr const char* sig() const
    {
        return traits<T>::sig;
    }
    constexpr const char* data() const
    {
        return _data;
    }

  private:
    const char* _data;
};

template <typename T, typename F>
struct string_write_helper<as_helper<T, F>>
{
    constexpr explicit string_write_helper(const as_helper<T, F>& wh) :
        _data(std::string_view(wh.ref).data())
    {}
    constexpr const char* sig() const
    {
        return traits<T>::sig;
    }
    constexpr const char* data() const
    {
        return _data;
    }

  private:
    const char* _data;
};

template <typename C, typename V>
struct array_of_helper
{
    C& ref;
};

template <typename V, typename C>
static constexpr auto as_array_of(C&& v)
{
    using type = std::remove_reference_t<C>;
    using value_type = typename type::value_type;
    return array_of_helper<type, as_helper<V, value_type>>(std::forward<C>(v));
}

} // namespace sdbus

#endif // sdbus_HELPERS_HPP_
