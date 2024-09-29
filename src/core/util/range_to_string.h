#pragma once

#include <ostream>
#include <ranges>
#include <sstream>
#include <string>

namespace util {

template <typename T>
concept Printable = requires(std::ostream& sstream, T const& t) { sstream << t; };

template <std::ranges::input_range Range>
    requires Printable<std::ranges::range_value_t<Range>>
std::string RangeToString(Range const& range) {
    std::ostringstream sstream;
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
