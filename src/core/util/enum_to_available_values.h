#pragma once

#include <sstream>
#include <string>
#include <type_traits>

#include <magic_enum/magic_enum.hpp>

namespace util {

template <typename EnumType>
    requires std::is_enum_v<EnumType>
static std::string EnumToAvailableValues() {
    std::stringstream avail_values;

    constexpr auto& enum_names = magic_enum::enum_names<EnumType>();

    avail_values << '[';
    for (size_t i = 0; i < enum_names.size(); ++i) {
        avail_values << enum_names[i];
        if (i < enum_names.size() - 1) {
            avail_values << '|';
        }
    }
    avail_values << ']';

    return avail_values.str();
}

}  // namespace util
