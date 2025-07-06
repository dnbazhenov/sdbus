#include <sdbus/traits.hpp>
#include <sdbus/write.hpp>

#include <cerrno>

namespace sdbus
{

errc write_basic_impl(subctx& ctx, const char* type, const void* v)
{
    return sdbus_errc(sd_bus_message_append_basic(ctx.msg(), type[0], v));
}

errc write_basic(subctx& ctx, const bool& v)
{
    int tmp = v;
    return write_basic_impl(ctx, basic_signature<bool>(), &tmp);
}

errc write_basic(subctx& ctx, const uint8_t& v)
{
    return write_basic_impl(ctx, basic_signature<uint8_t>(), &v);
}

errc write_basic(subctx& ctx, const int16_t& v)
{
    return write_basic_impl(ctx, basic_signature<int16_t>(), &v);
}

errc write_basic(subctx& ctx, const uint16_t& v)
{
    return write_basic_impl(ctx, basic_signature<uint16_t>(), &v);
}

errc write_basic(subctx& ctx, const int32_t& v)
{
    return write_basic_impl(ctx, basic_signature<int32_t>(), &v);
}

errc write_basic(subctx& ctx, const uint32_t& v)
{
    return write_basic_impl(ctx, basic_signature<uint32_t>(), &v);
}

errc write_basic(subctx& ctx, const int64_t& v)
{
    return write_basic_impl(ctx, basic_signature<int64_t>(), &v);
}

errc write_basic(subctx& ctx, const long long& v)
{
    return write_basic_impl(ctx, basic_signature<long long>(), &v);
}

errc write_basic(subctx& ctx, const uint64_t& v)
{
    return write_basic_impl(ctx, basic_signature<uint64_t>(), &v);
}

errc write_basic(subctx& ctx, const unsigned long long& v)
{
    return write_basic_impl(ctx, basic_signature<unsigned long long>(), &v);
}

errc write_basic(subctx& ctx, const double& v)
{
    return write_basic_impl(ctx, basic_signature<double>(), &v);
}

errc write_variant_impl(subctx& ctx, const char* sig, writer_base& writer)
{
    auto msg = ctx.msg();
    auto ec = sdbus_errc(sd_bus_message_open_container(msg, SD_BUS_TYPE_VARIANT, sig));
    if (no_error(ec))
    {
        ec = writer.write_value(ctx);
        sd_bus_message_close_container(msg);
    }
    return ec;
}

errc write_dict_entry_impl(subctx& ctx, const char* sig, writer_base& writer)
{
    auto msg = ctx.msg();
    auto ec = sdbus_errc(sd_bus_message_open_container(msg, SD_BUS_TYPE_DICT_ENTRY, sig));
    if (no_error(ec))
    {
        ec = writer.write_value(ctx);
        sd_bus_message_close_container(msg);
    }
    return ec;
}

errc write_array_impl(subctx& ctx, const char* sig, size_t size, writer_base& writer)
{
    auto msg = ctx.msg();
    auto ec = sdbus_errc(sd_bus_message_open_container(msg, SD_BUS_TYPE_ARRAY, sig));
    if (no_error(ec))
    {
        auto index = 0;

        while (size--)
        {
            subctx ictx(index++, ctx);
            ec = writer.write_value(ictx);
            if (is_error(ec))
            {
                break;
            }
        }
        sd_bus_message_close_container(msg);
    }
    return ec;
}

} // namespace sdbus
