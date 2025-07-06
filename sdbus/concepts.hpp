#ifndef sdbus_CONCEPTS_HPP_
#define sdbus_CONCEPTS_HPP_

#include <concepts>
#include <cstdint>
#include <string>
#include <tuple>
#include <variant>

namespace sdbus::concepts
{

template <typename T>
concept Basic =
    std::same_as<T, bool> || std::same_as<T, int16_t> || std::same_as<T, int32_t> ||
    std::same_as<T, int64_t> || std::same_as<T, long long> || std::same_as<T, uint8_t> ||
    std::same_as<T, uint16_t> || std::same_as<T, uint32_t> || std::same_as<T, uint64_t> ||
    std::same_as<T, unsigned long long> || std::same_as<T, double>;

template <typename T>
concept String = std::convertible_to<T, std::string_view> && std::assignable_from<T&, const char*>;

template <typename T>
concept WriteOnlyString =
    std::convertible_to<T, std::string_view> && !std::assignable_from<T&, const char*>;

template <typename T>
concept GenericContainer = requires {
    typename T::value_type;
    typename T::iterator;
    typename T::const_iterator;
};

template <typename T>
concept Container = GenericContainer<T> && !String<T>;

template <typename T>
concept HasEmplaceBack = requires(T t) {
    { t.emplace(t.end(), std::declval<typename T::value_type>) };
};

template <typename T>
concept HasEmplace = requires(T t) {
    { t.emplace(std::declval<typename T::value_type>) };
};

template <typename T>
concept Emplaceable = HasEmplaceBack<T> || HasEmplace<T>;

template <typename T>
concept DictEntry = requires(T t) {
    typename T::first_type;
    typename T::second_type;
    { t.first } -> std::same_as<typename T::first_type&>;
    { t.second } -> std::same_as<typename T::second_type&>;
    requires Basic<std::decay_t<typename T::first_type>> ||
                 String<std::decay_t<typename T::first_type>>;
};

template <typename T>
concept Variant = requires {
    { std::variant_size<T>::value };
};

template <typename T>
concept Property = requires {
    { T::name };
    { T::pointer };
    { &T::read_value };
    { &T::write_value };
};

template <typename T>
concept Dict = requires { requires Property<std::tuple_element_t<0, typename T::dict_t>>; };

} // namespace sdbus::concepts

#endif // sdbus_CONCEPTS_HPP_
