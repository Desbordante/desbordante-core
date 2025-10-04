#pragma once

#include <ranges>
#include <sstream>
#include <string>

namespace util {

template <typename T>
concept Printable = requires(std::stringstream& sstream, T& t) { sstream << t; };

template <std::ranges::input_range Range>
    requires Printable<std::ranges::range_value_t<Range>>
std::string RangeToString(Range const& range) {
    std::stringstream sstream;
    sstream << '[';
    for (auto pt{range.begin()}; pt != range.end(); ++pt) {
        if (pt != range.begin()) {
            sstream << ", ";
        }
        sstream << *pt;
    }
    sstream << ']';
    return sstream.str();
}

}  // namespace util
