
#include <sdbus/sdbus.hpp>

#include <gtest/gtest.h>

static constexpr auto sig(auto v)
{
    return sdbus::traits<decltype(v)>::sig;
}

struct sdbus_deleter
{
    static void operator()(sd_bus* bus)
    {
        sd_bus_unref(bus);
    }
};

struct sdbus_msg_deleter
{
    static void operator()(sd_bus_message* msg)
    {
        sd_bus_message_unref(msg);
    }
};

using sdbus_ptr = std::unique_ptr<sd_bus, sdbus_deleter>;
using sdbus_msg = std::unique_ptr<sd_bus_message, sdbus_msg_deleter>;

static sdbus_ptr create_dbus()
{
    sd_bus* bus;
    sd_bus_default(&bus);
    return sdbus_ptr{bus};
}

static sdbus_msg create_msg(const sdbus_ptr& bus, uint8_t type)
{
    sd_bus_message* msg;
    sd_bus_message_new(bus.get(), &msg, type);
    return sdbus_msg{msg};
}

struct ReadWrite : public testing::Test
{

    static void SetUpTestSuite()
    {
        s_bus = create_dbus();
    }

    static void TearDownTestSuite()
    {
        s_bus.reset();
    }

    void SetUp() override
    {
        _msg = create_msg(s_bus, SD_BUS_MESSAGE_METHOD_CALL);
    }

    void TearDown() override
    {
        _msg.reset();
    }

    sd_bus_message* msg() const
    {
        return _msg.get();
    }

  private:
    static inline sdbus_ptr s_bus;
    sdbus_msg _msg;
};

TEST_F(ReadWrite, BasicConstants)
{
    sdbus::defctx ctx(msg());

    int32_t i;
    uint32_t u;
    int64_t x;
    uint64_t t;
    double d;
    bool b;

    // integer constants
    EXPECT_TRUE(sdbus::write(ctx, -5) == sdbus::errc::success);
    EXPECT_TRUE(sdbus::write(ctx, 6u) == sdbus::errc::success);
    EXPECT_TRUE(sdbus::write(ctx, -7l) == sdbus::errc::success);
    EXPECT_TRUE(sdbus::write(ctx, 8ul) == sdbus::errc::success);
    EXPECT_TRUE(sdbus::write(ctx, -9ll) == sdbus::errc::success);
    EXPECT_TRUE(sdbus::write(ctx, 10ull) == sdbus::errc::success);
    // double constants
    EXPECT_TRUE(sdbus::write(ctx, 1.2) == sdbus::errc::success);
    // boolean constants
    EXPECT_TRUE(sdbus::write(ctx, false) == sdbus::errc::success);
    EXPECT_TRUE(sdbus::write(ctx, true) == sdbus::errc::success);

    sd_bus_message_seal(msg(), 100, 0);

    EXPECT_TRUE(sdbus::read(ctx, i) == sdbus::errc::success);
    EXPECT_EQ(i, -5);
    EXPECT_TRUE(sdbus::read(ctx, u) == sdbus::errc::success);
    EXPECT_EQ(u, 6u);
    EXPECT_TRUE(sdbus::read(ctx, x) == sdbus::errc::success);
    EXPECT_EQ(x, -7l);
    EXPECT_TRUE(sdbus::read(ctx, t) == sdbus::errc::success);
    EXPECT_EQ(t, 8ul);
    EXPECT_TRUE(sdbus::read(ctx, x) == sdbus::errc::success);
    EXPECT_EQ(x, -9ll);
    EXPECT_TRUE(sdbus::read(ctx, t) == sdbus::errc::success);
    EXPECT_EQ(t, 10ull);
    EXPECT_TRUE(sdbus::read(ctx, d) == sdbus::errc::success);
    EXPECT_EQ(d, 1.2);
    EXPECT_TRUE(sdbus::read(ctx, b) == sdbus::errc::success);
    EXPECT_EQ(b, false);
    EXPECT_TRUE(sdbus::read(ctx, b) == sdbus::errc::success);
    EXPECT_EQ(b, true);
}

TEST_F(ReadWrite, BasicTypes)
{
    sdbus::defctx ctx(msg());

    uint8_t y = 20, y2;
    int16_t n = -21, n2;
    uint16_t q = 22, q2;
    int32_t i = -23, i2;
    uint32_t u = 24, u2;
    int64_t x = -25, x2;
    uint64_t t = 26, t2;
    double d = -27.0, d2;
    bool b = true, b2;

    EXPECT_TRUE(sdbus::write(ctx, y) == sdbus::errc::success);
    EXPECT_TRUE(sdbus::write(ctx, n) == sdbus::errc::success);
    EXPECT_TRUE(sdbus::write(ctx, q) == sdbus::errc::success);
    EXPECT_TRUE(sdbus::write(ctx, i) == sdbus::errc::success);
    EXPECT_TRUE(sdbus::write(ctx, u) == sdbus::errc::success);
    EXPECT_TRUE(sdbus::write(ctx, x) == sdbus::errc::success);
    EXPECT_TRUE(sdbus::write(ctx, t) == sdbus::errc::success);
    EXPECT_TRUE(sdbus::write(ctx, d) == sdbus::errc::success);
    EXPECT_TRUE(sdbus::write(ctx, b) == sdbus::errc::success);

    sd_bus_message_seal(msg(), 100, 0);

    EXPECT_TRUE(sdbus::read(ctx, y2) == sdbus::errc::success);
    EXPECT_EQ(y, y2);
    EXPECT_TRUE(sdbus::read(ctx, n2) == sdbus::errc::success);
    EXPECT_EQ(n, n2);
    EXPECT_TRUE(sdbus::read(ctx, q2) == sdbus::errc::success);
    EXPECT_EQ(q, q2);
    EXPECT_TRUE(sdbus::read(ctx, i2) == sdbus::errc::success);
    EXPECT_EQ(i, i2);
    EXPECT_TRUE(sdbus::read(ctx, u2) == sdbus::errc::success);
    EXPECT_EQ(u, u2);
    EXPECT_TRUE(sdbus::read(ctx, x2) == sdbus::errc::success);
    EXPECT_EQ(x, x2);
    EXPECT_TRUE(sdbus::read(ctx, t2) == sdbus::errc::success);
    EXPECT_EQ(t, t2);
    EXPECT_TRUE(sdbus::read(ctx, d2) == sdbus::errc::success);
    EXPECT_EQ(d, d2);
    EXPECT_TRUE(sdbus::read(ctx, b2) == sdbus::errc::success);
    EXPECT_EQ(b, b2);
}

TEST_F(ReadWrite, AsBasic)
{
    sdbus::defctx ctx(msg());

    uint8_t y = 0xF8, y2;
    int16_t n = static_cast<int16_t>(0xF7F8), n2;
    uint16_t q = 0xF7F8, q2;
    int32_t i = 0xF5F6F7F8, i2;
    uint32_t u = 0xF5F6F7F8, u2;
    uint64_t t = 0xF1F2F3F4F5F6F7F8;
    double d;
    bool b;

    EXPECT_TRUE(sdbus::write(ctx, sdbus::as<bool>(t)) == sdbus::errc::success);
    EXPECT_TRUE(sdbus::write(ctx, sdbus::as<uint8_t>(t)) == sdbus::errc::success);
    EXPECT_TRUE(sdbus::write(ctx, sdbus::as<int16_t>(t)) == sdbus::errc::success);
    EXPECT_TRUE(sdbus::write(ctx, sdbus::as<uint16_t>(t)) == sdbus::errc::success);
    EXPECT_TRUE(sdbus::write(ctx, sdbus::as<int32_t>(t)) == sdbus::errc::success);
    EXPECT_TRUE(sdbus::write(ctx, sdbus::as<uint32_t>(t)) == sdbus::errc::success);
    EXPECT_TRUE(sdbus::write(ctx, sdbus::as<double>(505)) == sdbus::errc::success);

    sd_bus_message_seal(msg(), 100, 0);

    EXPECT_TRUE(sdbus::read(ctx, b) == sdbus::errc::success);
    EXPECT_EQ(b, true);
    EXPECT_TRUE(sdbus::read(ctx, y2) == sdbus::errc::success);
    EXPECT_EQ(y, y2);
    EXPECT_TRUE(sdbus::read(ctx, n2) == sdbus::errc::success);
    EXPECT_EQ(n, n2);
    EXPECT_TRUE(sdbus::read(ctx, q2) == sdbus::errc::success);
    EXPECT_EQ(q, q2);
    EXPECT_TRUE(sdbus::read(ctx, i2) == sdbus::errc::success);
    EXPECT_EQ(i, i2);
    EXPECT_TRUE(sdbus::read(ctx, u2) == sdbus::errc::success);
    EXPECT_EQ(u, u2);
    EXPECT_TRUE(sdbus::read(ctx, d) == sdbus::errc::success);
    EXPECT_EQ(d, 505);
}
