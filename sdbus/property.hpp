#ifndef sdbus_PROPERTY_HPP_
#define sdbus_PROPERTY_HPP_

#include <sdbus/context.hpp>
#include <sdbus/helpers.hpp>

#include <span>
#include <string_view>

namespace sdbus
{

template <sig_string Name, property_ptr Tptr>
struct property
{
    static constexpr auto pointer = Tptr.mptr;
    static constexpr auto name = Name;

    using container = decltype(Tptr)::container_type;
    using reference = decltype(Tptr)::reference;
    using const_reference = decltype(Tptr)::const_reference;

    static errc read_value(subctx& ctx, container& compound)
    {
        reference value = compound.*pointer;
        return read_variant(ctx, value);
    }

    static errc write_value(subctx& ctx, const container& compound)
    {
        const_reference value{compound.*pointer};
        return write_variant(ctx, value);
    }
};

template <typename T>
struct property_desc
{
    using container_type = T;

    template <typename Property>
    constexpr property_desc(Property&&) :
        _read_fn(&Property::read_value), _write_fn(&Property::write_value), _name(Property::name)
    {}

    errc read_value(subctx& ctx, container_type& compound) const
    {
        return _read_fn(ctx, compound);
    }

    errc write_value(subctx& ctx, const container_type& compound) const
    {
        return _write_fn(ctx, compound);
    }

    constexpr std::string_view name() const noexcept
    {
        return _name;
    }

  private:
    errc (*_read_fn)(subctx&, container_type&);
    errc (*_write_fn)(subctx&, const container_type&);
    std::string_view _name;
};

template <typename T>
using property_descs = std::span<const property_desc<T>>;

template <typename T>
struct property_desc_maker
{};

template <typename T, typename... Ts>
struct property_desc_maker<std::tuple<T, Ts...>>
{
    static constexpr auto make_descs()
    {
        using container_type = typename T::container;
        return std::array<property_desc<container_type>, sizeof...(Ts) + 1>{T(), Ts()...};
    }
};

} // namespace sdbus

#endif /* sdbus_PROPERTY_HPP_ */
