#pragma once

#include <sstream>
#include <string>

namespace util {

template <typename BetterEnumType>
static std::string EnumToAvailableValues() {
    std::stringstream avail_values;

    avail_values << '[';

    for (auto const& name : BetterEnumType::_names()) {
        avail_values << name << '|';
    }

    avail_values.seekp(-1, std::stringstream::cur);
    avail_values << ']';

    return avail_values.str();
}

}  // namespace util
