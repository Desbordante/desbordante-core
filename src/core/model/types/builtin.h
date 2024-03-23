#pragma once

#include <algorithm>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/serialization/strong_typedef.hpp>
#include <enum.h>

namespace model {

/* Aliases below are used to describe types containing in ColumnData and a ColumnData type itself.
 */

/* 'Real' types */

using Int = int64_t; /* Type of any integer value that fits into int64 */

namespace details {
BOOST_STRONG_TYPEDEF(std::string, Placeholder);
}  // namespace details

using BigInt = details::Placeholder; /* Type of an integer that don't fit into Int */
using Double = double; /* Fixed-precision floating point value; also we need type for values
                        * with arbitrary precision analogous to BigInt */
using String = std::string;
using Date = boost::gregorian::date; /* Date in the range from 1400-Jan-01 to 9999-Dec-31 */

static_assert(sizeof(Int) == sizeof(Double));

/* Dummy types */

/* Value is equal to "NULL". Also is used to describe ColumnData with undefined
 * type, i.e. columns containing only null and empty values */
struct Null {
    constexpr static std::string_view kValue = "NULL";
};

class Empty {}; /* Empty value */

class Mixed {}; /* Dummy type only to describe columns with more than one type */

/* All types that ColumnData can contain */
using AllValueTypes = std::tuple<Int, Double, BigInt, String, Null, Empty>;

/* Describes the type of ColumnData in a runtime and types of values hold by it.
 * Maybe we need to use separate enums to describe column types and value types to
 * avoid confusion.
 */
// clang-format off
BETTER_ENUM(TypeId, char,
    kInt = 0,   /* Except for nulls and empties column contains only ints
                 * (fixed-precision integer value) */
    kDouble,    /* Except for nulls and empties column contains only doubles
                 * (fixed-precision floating point value) */
    kBigInt,    /* Except for nulls and empties column contains only big ints
                 * (arbitrary-precision integer value) */
    kString,    /* Except for nulls and empties column contains only strings
                 * (string value, sequence of characters) */
    kDate,      /* Column contains only valid dates in the range 1400-Jan-01 to 9999-Dec-31
                 * (class date from boost::gregorian)  */
    kNull,      /* Column contains only nulls ("NULL" value) */
    kEmpty,     /* Column contains only empties ("" value) */
    kUndefined, /* Column contains only nulls and empties */
    kMixed      /* Except for nulls and empties column contains more than one type */
);

// clang-format on

template <typename T>
struct TypeConverter {};

template <>
struct TypeConverter<Int> {
    inline static constexpr auto kConvert = [](std::string const& v) {
        return static_cast<Int>(std::stoll(v));
    };
};

template <>
struct TypeConverter<Double> {
    inline static constexpr auto kConvert = [](std::string const& v) { return std::stold(v); };
};

template <>
struct TypeConverter<BigInt> {
    inline static constexpr auto kConvert = [](std::string& v) { return BigInt(std::move(v)); };
};

template <>
struct TypeConverter<String> {
    inline static constexpr auto kConvert = [](std::string& v) { return std::move(v); };
};

template <>
struct TypeConverter<Null> {
    inline static constexpr auto kConvert = [](std::string const& v) {
        if (v != Null::kValue) {
            throw std::invalid_argument("Cannot convert v to Null value");
        }
        return Null();
    };
};

template <>
struct TypeConverter<Empty> {
    inline static constexpr auto kConvert = [](std::string const& v) {
        if (!v.empty()) {
            throw std::invalid_argument("Cannot convert v to Empty value");
        }
        return Empty();
    };
};

enum class CompareResult { kLess = -1, kGreater = 1, kEqual = 0, kNotEqual = 2 };

namespace detail {

template <typename T>
struct TupleMaxAlign {};

template <typename... Ts>
struct TupleMaxAlign<std::tuple<Ts...>> {
    static constexpr size_t kValue = std::max({alignof(Ts)...});
};

}  // namespace detail

inline constexpr size_t kTypesMaxAlignment = detail::TupleMaxAlign<AllValueTypes>::kValue;

}  // namespace model

/* Should be outside the namespace */
BETTER_ENUMS_DECLARE_STD_HASH(model::TypeId)
