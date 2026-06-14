#include "core/algorithms/pac/pac_verifier/ucc_pac_verifier/ucc_pac_highlight.h"

#include <cstddef>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "core/algorithms/pac/pac_verifier/util/tuple_pair.h"

using namespace pac::model;

namespace algos::pac_verifier {
std::vector<std::pair<std::size_t, std::size_t>> UCCPACHighlight::RowIndices() const {
    return TransformPairs<std::pair<std::size_t, std::size_t>>(
            [](TuplePair const& pair) { return std::make_pair(pair.first_idx, pair.second_idx); });
}

std::vector<UCCPACHighlight::RawPair> UCCPACHighlight::RawData() const {
    return TransformPairs<RawPair>([this](TuplePair const& pair) {
        return std::make_pair((*tuples_)[pair.first_idx], (*tuples_)[pair.second_idx]);
    });
}

std::vector<UCCPACHighlight::StringPair> UCCPACHighlight::StringData() const {
    auto val_to_str = [this](std::size_t const idx) {
        return tuple_type_->ValueToString((*tuples_)[idx]);
    };
    return TransformPairs<StringPair>([&val_to_str](TuplePair const& pair) {
        return std::make_pair(val_to_str(pair.first_idx), val_to_str(pair.second_idx));
    });
}

std::string UCCPACHighlight::ToString() const {
    auto string_pairs = StringData();
    std::ostringstream result;
    result << '[';
    for (auto it = string_pairs.begin(); it != string_pairs.end(); ++it) {
        if (it != string_pairs.begin()) {
            result << ", ";
        }
        auto const& [first_str, second_str] = *it;
        result << '(' << first_str << ", " << second_str << ')';
    }
    result << ']';
    return result.str();
}
}  // namespace algos::pac_verifier
