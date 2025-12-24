#pragma once

#include <sstream>
#include <string>
#include <type_traits>

#include <magic_enum/magic_enum.hpp>

#include "core/util/enum_to_str.h"

namespace util {

template <typename EnumType>
    requires std::is_enum_v<EnumType>
static std::string EnumToAvailableValues() {
    std::stringstream avail_values;
    constexpr auto& enum_values = magic_enum::enum_values<EnumType>();

    avail_values << '[';
    for (size_t i = 0; i < enum_values.size(); ++i) {
        avail_values << EnumToStr(enum_values[i]);
        if (i < enum_values.size() - 1) {
            avail_values << '|';
        }
    }
    avail_values << ']';

    return avail_values.str();
}

}  // namespace util
