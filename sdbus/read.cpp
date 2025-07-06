#include <systemd/sd-bus.h>

#include <sdbus/read.hpp>
#include <sdbus/traits.hpp>

namespace sdbus
{

errc read_basic_impl(subctx& ctx, const char* type, void* v)
{
    return sdbus_errc(sd_bus_message_read_basic(ctx.msg(), type[0], v));
}

errc read_basic(subctx& ctx, bool& v)
{
    int32_t tmp;
    auto ec = read_basic_impl(ctx, basic_signature<bool>(), &tmp);
    if (no_error(ec))
    {
        v = static_cast<bool>(tmp);
    }
    return ec;
}

errc read_basic(subctx& ctx, uint8_t& v)
{
    return read_basic_impl(ctx, basic_signature<uint8_t>(), &v);
}

errc read_basic(subctx& ctx, int16_t& v)
{
    return read_basic_impl(ctx, basic_signature<int16_t>(), &v);
}

errc read_basic(subctx& ctx, uint16_t& v)
{
    return read_basic_impl(ctx, basic_signature<uint16_t>(), &v);
}

errc read_basic(subctx& ctx, int32_t& v)
{
    return read_basic_impl(ctx, basic_signature<int32_t>(), &v);
}

errc read_basic(subctx& ctx, uint32_t& v)
{
    return read_basic_impl(ctx, basic_signature<uint32_t>(), &v);
}

errc read_basic(subctx& ctx, int64_t& v)
{
    return read_basic_impl(ctx, basic_signature<int64_t>(), &v);
}

errc read_basic(subctx& ctx, long long& v)
{
    return read_basic_impl(ctx, basic_signature<long long>(), &v);
}

errc read_basic(subctx& ctx, uint64_t& v)
{
    return read_basic_impl(ctx, basic_signature<uint64_t>(), &v);
}

errc read_basic(subctx& ctx, unsigned long long& v)
{
    return read_basic_impl(ctx, basic_signature<unsigned long long>(), &v);
}

errc read_basic(subctx& ctx, double& v)
{
    return read_basic_impl(ctx, basic_signature<double>(), &v);
}

errc read_array_impl(subctx& ctx, reader_base& rdr)
{
    auto msg = ctx.msg();
    auto ec = sdbus_errc(sd_bus_message_enter_container(msg, SD_BUS_TYPE_ARRAY, nullptr));
    if (no_error(ec))
    {
        auto index = 0;

        for (;;)
        {
            auto ret = sd_bus_message_at_end(msg, 0);
            if (ret < 0)
            {
                return errc::read_error;
            }
            if (ret > 0)
            {
                break;
            }

            subctx ictx(index++, ctx);
            ec = rdr.read_value(ictx);
            if (is_error(ec))
            {
                ec = ictx.error(ec);
                if (is_error(ec))
                {
                    break;
                }
                else
                {
                    sd_bus_message_skip(msg, nullptr);
                }
            }
        }

        if (sd_bus_message_exit_container(msg) < 0)
        {
            return errc::read_error;
        }
    }

    return ec;
}

errc read_variant_impl(subctx& ctx, reader_base& rdr)
{
    auto msg = ctx.msg();
    auto ec = sdbus_errc(sd_bus_message_enter_container(msg, SD_BUS_TYPE_VARIANT, nullptr));
    if (no_error(ec))
    {
        ec = rdr.read_value(ctx);
        if (is_error(ec))
        {
            sd_bus_message_skip(msg, nullptr);
        }
        if (sd_bus_message_exit_container(msg) < 0)
        {
            return errc::read_error;
        }
    }

    return ec;
}

errc read_dict_entry_impl(subctx& ctx, reader_base& rdr)
{
    auto msg = ctx.msg();
    auto ec = sdbus_errc(sd_bus_message_enter_container(msg, SD_BUS_TYPE_DICT_ENTRY, nullptr));
    if (no_error(ec))
    {
        ec = rdr.read_value(ctx);
        if (ec == errc::unknown_property)
        {
            ec = errc::success;
        }
        if (sd_bus_message_exit_container(msg) < 0)
        {
            return errc::read_error;
        }
    }

    return ec;
}

errc variant_reader_base::read_value(subctx& ctx)
{
    const char* sig = sd_bus_message_get_signature(ctx.msg(), 0);

    for (const auto& cb : _callbacks)
    {
        errc ec = errc::success;

        if ((this->*cb)(ctx, sig, ec))
        {
            return ec;
        }
    }

    return errc::bad_variant;
}

errc dict_reader_base::read_value(subctx& ctx)
{
    auto msg = ctx.msg();
    auto ec = sdbus_errc(sd_bus_message_enter_container(msg, SD_BUS_TYPE_DICT_ENTRY, nullptr));
    if (no_error(ec))
    {
        const char* name;
        ec = read(ctx, name);
        if (no_error(ec))
        {
            ec = read_entry(ctx, name);
        }
        if (ec == errc::unknown_property)
        {
            sd_bus_message_skip(msg, 0);
            ec = errc::success;
        }
        if (sd_bus_message_exit_container(msg) < 0)
        {
            return errc::read_error;
        }
    }

    return ec;
}

} // namespace sdbus
