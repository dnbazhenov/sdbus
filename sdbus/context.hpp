#ifndef sdbus_CONTEXT_HPP_
#define sdbus_CONTEXT_HPP_

#include <systemd/sd-bus.h>

#include <sdbus/error_code.hpp>

#include <functional>

namespace sdbus
{

struct path_entry
{
    enum type_t
    {
        none,
        array_item,
        dict_entry,
    };

    type_t type = none;
    union
    {
        const char* entry;
        size_t index;
    };
    const path_entry& parent;

    constexpr path_entry() : parent(*this)
    {}
    constexpr path_entry(const path_entry&) = default;
    constexpr path_entry(size_t index, const path_entry& parent) :
        type{array_item}, index{index}, parent{parent}
    {}
    constexpr path_entry(const char* entry, const path_entry& parent) :
        type{dict_entry}, entry{entry}, parent{parent}
    {}
    const char* to_str() const;
};

struct defctx;
struct subctx
{
    constexpr subctx(const subctx&) = default;
    constexpr subctx(defctx& ctx) : _context{ctx}
    {}
    constexpr subctx(size_t index, const subctx& parent) :
        _context{parent._context}, _path{index, parent._path}
    {}
    constexpr subctx(const char* entry, const subctx& parent) :
        _context{parent._context}, _path{entry, parent._path}
    {}
    constexpr sd_bus_message* msg() const;
    constexpr errc error(errc ec) const;
    constexpr const path_entry& path() const
    {
        return _path;
    }
    constexpr path_entry::type_t path_type() const
    {
        return _path.type;
    }
    constexpr size_t index() const
    {
        return _path.index;
    }
    constexpr const char* entry() const
    {
        return _path.entry;
    }
    const char* path_str() const
    {
        return _path.to_str();
    }

  protected:
    defctx& _context;
    const path_entry _path;
};

struct defctx : subctx
{
    using error_callback = std::function<errc(const subctx&, errc)>;

    constexpr defctx(sd_bus_message* msg, error_callback&& error_cb = {}) :
        subctx{*this}, _msg{msg}, _error(std::move(error_cb))
    {}
    constexpr sd_bus_message* msg() const
    {
        return _msg;
    }
    constexpr errc error(errc ec) const
    {
        return error(*this, ec);
    }
    constexpr errc error(const subctx& ctx, errc ec) const
    {
        if (_error)
        {
            return _error(ctx, ec);
        }

        return ec;
    }

  private:
    sd_bus_message* _msg;
    error_callback _error;
};

constexpr sd_bus_message* subctx::msg() const
{
    return _context.msg();
}

constexpr errc subctx::error(errc ec) const
{
    return _context.error(*this, ec);
}

} // namespace sdbus

#endif /* sdbus_CONTEXT_HPP_ */
