#ifndef sdbus_FORWARDS_HPP_
#define sdbus_FORWARDS_HPP_

#include <sdbus/error_code.hpp>
#include <sdbus/sig_string.hpp>

#include <cstdint>

namespace sdbus
{

struct subctx;

template <typename T>
struct default_traits
{
    static constexpr auto sig = sig_string("");

    static errc read_value(subctx&, T&)
    {
        static_assert(false, "Type-specific traits not defined.");
        return errc::read_error;
    }

    static errc write_value(subctx&, const T&)
    {
        static_assert(false, "Type-specific traits not defined.");
        return errc::write_error;
    }
};

template <typename T>
struct traits : default_traits<T>
{};

struct reader_base;
struct writer_base;

errc sdbus_errc(int ret);

errc read_basic_impl(subctx&, const char*, void*);
errc read_variant_impl(subctx&, reader_base&);
errc read_dict_entry_impl(subctx&, reader_base&);
errc read_array_impl(subctx&, reader_base&);

errc write_basic_impl(subctx&, const char*, const void*);
errc write_variant_impl(subctx&, const char*, writer_base&);
errc write_dict_entry_impl(subctx&, const char*, writer_base&);
errc write_array_impl(subctx&, const char*, size_t, writer_base&);

errc read_basic(subctx&, bool&);
errc read_basic(subctx&, uint8_t&);
errc read_basic(subctx&, int16_t&);
errc read_basic(subctx&, int16_t&);
errc read_basic(subctx&, uint16_t&);
errc read_basic(subctx&, int32_t&);
errc read_basic(subctx&, uint32_t&);
errc read_basic(subctx&, int64_t&);
errc read_basic(subctx&, uint64_t&);
errc read_basic(subctx&, long long&);
errc read_basic(subctx&, unsigned long long&);
errc read_basic(subctx&, double&);

errc write_basic(subctx&, const bool&);
errc write_basic(subctx&, const uint8_t&);
errc write_basic(subctx&, const int16_t&);
errc write_basic(subctx&, const int16_t&);
errc write_basic(subctx&, const uint16_t&);
errc write_basic(subctx&, const int32_t&);
errc write_basic(subctx&, const uint32_t&);
errc write_basic(subctx&, const int64_t&);
errc write_basic(subctx&, const uint64_t&);
errc write_basic(subctx&, const long long&);
errc write_basic(subctx&, const unsigned long long&);
errc write_basic(subctx&, const double&);

static errc read(subctx& ctx, auto& v);
static errc read(subctx& ctx, auto&& v);

template <typename T>
static errc write(subctx& ctx, const T& v);

template <typename T>
static errc read_string(subctx& ctx, T& v);

template <typename T>
static errc write_string(subctx& ctx, const T& v);

template <typename T>
static errc read_variant(subctx& ctx, T& v);

template <typename T>
static errc write_variant(subctx& ctx, const T& v);

template <typename T>
static errc read_array(subctx& ctx, T& v);

template <typename T>
static errc write_array(subctx& ctx, const T& v);

} // namespace sdbus

#endif /* sdbus_FORWARDS_HPP_ */
