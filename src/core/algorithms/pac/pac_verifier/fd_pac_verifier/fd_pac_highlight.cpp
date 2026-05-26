#include "core/algorithms/pac/pac_verifier/fd_pac_verifier/fd_pac_highlight.h"

#include <cstddef>
#include <iterator>
#include <sstream>
#include <utility>
#include <vector>

#include "core/algorithms/pac/model/tuple.h"
#include "core/algorithms/pac/pac_verifier/util/tuple_pair.h"

namespace algos::pac_verifier {
std::vector<std::pair<std::size_t, std::size_t>> FDPACHighlight::RowIndices() const {
    std::vector<std::pair<std::size_t, std::size_t>> result;
    result.reserve(std::distance(begin_, end_));
    for (auto it = begin_; it != end_; ++it) {
        result.emplace_back(it->first_idx, it->second_idx);
        result.emplace_back(it->second_idx, it->first_idx);
    }
    return result;
}

std::vector<FDPACHighlight::RawPair> FDPACHighlight::RawData() const {
    auto make_raw_data = [this](std::size_t idx) {
        return LhsRhsData{(*lhs_tuples_)[idx], (*rhs_tuples_)[idx]};
    };

    return TransformPairs<RawPair>([&make_raw_data](TuplePair const& p) {
        return std::make_pair(make_raw_data(p.first_idx), make_raw_data(p.second_idx));
    });
}

std::vector<FDPACHighlight::StringPair> FDPACHighlight::StringData() const {
    auto make_string_data = [this](std::size_t idx) {
        using namespace pac::model;

        return LhsRhsString{TupleToString((*lhs_tuples_)[idx], *lhs_types_),
                            TupleToString((*rhs_tuples_)[idx], *rhs_types_)};
    };

    return TransformPairs<StringPair>([&make_string_data](TuplePair const& p) {
        return std::make_pair(make_string_data(p.first_idx), make_string_data(p.second_idx));
    });
}

std::string FDPACHighlight::ToString() const {
    auto string_pairs = StringData();
    std::ostringstream result;
    result << '[';
    for (auto it = string_pairs.begin(); it != string_pairs.end(); ++it) {
        if (it != string_pairs.begin()) {
            result << ", ";
        }
        auto const& [first_str, second_str] = *it;
        result << "({" << first_str.first << " -> " << first_str.second << "}, {"
               << second_str.first << " -> " << second_str.second << "})";
    }
    result << ']';
    return result.str();
}
}  // namespace algos::pac_verifier
