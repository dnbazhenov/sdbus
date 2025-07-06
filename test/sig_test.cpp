
#include <sdbus/sdbus.hpp>

#include <gtest/gtest.h>

using namespace sdbus;

static constexpr auto sig(auto v)
{
    return traits<decltype(v)>::sig;
}

TEST(Signature, BasicSig)
{
    EXPECT_EQ(traits<bool>::sig, "b");
    EXPECT_EQ(traits<uint8_t>::sig, "y");
    EXPECT_EQ(traits<int16_t>::sig, "n");
    EXPECT_EQ(traits<uint16_t>::sig, "q");
    EXPECT_EQ(traits<int32_t>::sig, "i");
    EXPECT_EQ(traits<uint32_t>::sig, "u");
    EXPECT_EQ(traits<int64_t>::sig, "x");
    EXPECT_EQ(traits<uint64_t>::sig, "t");
    EXPECT_EQ(traits<double>::sig, "d");
}

TEST(Signature, StringSig)
{
    EXPECT_EQ(traits<char*>::sig, "s");
    EXPECT_EQ(traits<const char*>::sig, "s");
}

TEST(Signature, As)
{
    defctx ctx(nullptr);
    bool b = false;
    int i = 0;
    std::string s;
    objpath o;

    EXPECT_EQ(sig(as<int>(b)), "i");
    EXPECT_EQ(sig(as<bool>(i)), "b");
    EXPECT_EQ(sig(as<uint8_t>(i)), "y");
    EXPECT_EQ(sig(as<int16_t>(i)), "n");
    EXPECT_EQ(sig(as<uint16_t>(i)), "q");
    EXPECT_EQ(sig(as<int32_t>(i)), "i");
    EXPECT_EQ(sig(as<uint32_t>(i)), "u");
    EXPECT_EQ(sig(as<int64_t>(i)), "x");
    EXPECT_EQ(sig(as<uint64_t>(i)), "t");
    EXPECT_EQ(sig(as<double>(i)), "d");
    EXPECT_EQ(sig(as<objpath>(s)), "o");
    EXPECT_EQ(sig(as<std::string>(o)), "s");
}
