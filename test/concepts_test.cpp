
#include <sdbus/sdbus.hpp>

#include <deque>
#include <list>
#include <queue>
#include <stack>

#include <gtest/gtest.h>

using namespace sdbus;

struct non_dict_s
{
    int a;
};

struct dict_s
{
    int a;
    using dict_t = std::tuple<property<"a", &dict_s::a>>;
};

using prop = property<"a", &dict_s::a>;

struct pairlike
{
    using first_type = int;
    using second_type = std::string;
    int first;
    std::string second;
};

struct non_pairlike1
{
    int first;
    std::string second;
};

struct non_pairlike2
{
    using first_type = int;
    using second_type = std::string;
};

TEST(Concepts, Basic)
{
    EXPECT_TRUE(concepts::Basic<bool>);
    EXPECT_TRUE(concepts::Basic<uint8_t>);
    EXPECT_TRUE(concepts::Basic<int16_t>);
    EXPECT_TRUE(concepts::Basic<uint16_t>);
    EXPECT_TRUE(concepts::Basic<int32_t>);
    EXPECT_TRUE(concepts::Basic<uint32_t>);
    EXPECT_TRUE(concepts::Basic<int64_t>);
    EXPECT_TRUE(concepts::Basic<uint64_t>);
    EXPECT_TRUE(concepts::Basic<double>);
    EXPECT_FALSE(concepts::Basic<char*>);
    EXPECT_FALSE(concepts::Basic<const char*>);
    EXPECT_FALSE(concepts::Basic<std::string_view>);
    EXPECT_FALSE(concepts::Basic<std::string>);
}

TEST(Concepts, String)
{
    EXPECT_FALSE(concepts::String<char*>);
    EXPECT_TRUE(concepts::String<const char*>);
    EXPECT_TRUE(concepts::String<std::string_view>);
    EXPECT_TRUE(concepts::String<std::string>);
    EXPECT_TRUE(concepts::String<std::pmr::string>);
    EXPECT_TRUE(concepts::WriteOnlyString<char*>);
    EXPECT_FALSE(concepts::WriteOnlyString<const char*>);
    EXPECT_FALSE(concepts::WriteOnlyString<std::string_view>);
    EXPECT_FALSE(concepts::WriteOnlyString<std::string>);
    EXPECT_FALSE(concepts::WriteOnlyString<std::pmr::string>);
}

TEST(Concepts, Variant)
{
    EXPECT_TRUE(concepts::Variant<std::variant<>>);
    EXPECT_TRUE(concepts::Variant<std::variant<int>>);
    EXPECT_FALSE(concepts::Variant<bool>);
    EXPECT_FALSE(concepts::Variant<uint8_t>);
    EXPECT_FALSE(concepts::Variant<int16_t>);
    EXPECT_FALSE(concepts::Variant<uint16_t>);
    EXPECT_FALSE(concepts::Variant<int32_t>);
    EXPECT_FALSE(concepts::Variant<uint32_t>);
    EXPECT_FALSE(concepts::Variant<int64_t>);
    EXPECT_FALSE(concepts::Variant<uint64_t>);
    EXPECT_FALSE(concepts::Variant<double>);
    EXPECT_FALSE(concepts::Variant<char*>);
    EXPECT_FALSE(concepts::Variant<const char*>);
    EXPECT_FALSE(concepts::Variant<std::string_view>);
    EXPECT_FALSE(concepts::Variant<std::string>);
}

TEST(Concepts, DictEntry)
{
    EXPECT_TRUE((concepts::DictEntry<std::pair<int, double>>));
    EXPECT_TRUE(concepts::DictEntry<pairlike>);
    EXPECT_FALSE((concepts::DictEntry<std::pair<non_dict_s, double>>));
    EXPECT_FALSE(concepts::DictEntry<non_pairlike1>);
    EXPECT_FALSE(concepts::DictEntry<non_pairlike2>);
}

TEST(Concepts, Array)
{
    EXPECT_TRUE(concepts::Container<std::vector<int>>);
    EXPECT_TRUE((concepts::Container<std::array<int, 1>>));
    EXPECT_TRUE(concepts::Container<std::list<int>>);
    EXPECT_TRUE(concepts::Container<std::deque<int>>);
    EXPECT_TRUE((concepts::Container<std::map<int, double>>));
    EXPECT_FALSE(concepts::Container<std::queue<int>>);
    EXPECT_FALSE(concepts::Container<std::stack<int>>);
}

TEST(Concepts, Dict)
{
    EXPECT_TRUE(concepts::Dict<dict_s>);
    EXPECT_FALSE(concepts::Dict<non_dict_s>);
}
