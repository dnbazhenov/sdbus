#ifndef sdbus_ERROR_HPP_
#define sdbus_ERROR_HPP_

#include <system_error>

namespace sdbus
{

enum class error
{
    success,
    invalid_type,
    invalid_enum,
    invalid_enum_string,
    invalid_property,
    unknown_property,
    read_error,
    write_error,
    no_memory,
    out_of_space,
    bad_variant,
    bad_exception,
};

using errc = error;

enum class condition
{
    critical_error = 1,
    non_critical_error,
};

static constexpr const std::error_category& error_cat()
{
    extern const std::error_category& error_cat_ref;
    return error_cat_ref;
}

static constexpr const std::error_category& condition_cat()
{
    extern const std::error_category& condition_cat_ref;
    return condition_cat_ref;
}

static constexpr bool is_error(errc ec)
{
    return ec != error::success;
}

static constexpr bool no_error(errc ec)
{
    return ec == error::success;
}

} // namespace sdbus

namespace std
{

template <>
struct is_error_code_enum<sdbus::error> : std::true_type
{};

template <>
struct is_error_condition_enum<sdbus::condition> : std::true_type
{};

static constexpr error_code make_error_code(sdbus::error e) noexcept
{
    return error_code(static_cast<int>(e), sdbus::error_cat());
}

static constexpr error_condition make_error_condtition(sdbus::condition e) noexcept
{
    return error_condition(static_cast<int>(e), sdbus::condition_cat());
}

} // namespace std

#endif // sdbus_ERROR_HPP_
