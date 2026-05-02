#include "core/algorithms/pac/pac_verifier/ucc_pac_verifier/ucc_pac_highlight.h"

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "core/algorithms/pac/model/tuple.h"

using namespace pac::model;

namespace algos::pac_verifier {
std::vector<UCCPACHighlight::RawPair> UCCPACHighlight::RawData() const {
    return TransformPairs<RawPair>(
            [](Tuple const& a, Tuple const& b) { return std::make_pair(a, b); });
}

std::vector<UCCPACHighlight::StringPair> UCCPACHighlight::StringData() const {
    return TransformPairs<StringPair>([this](Tuple const& a, Tuple const& b) {
        return std::make_pair(tuple_type_->ValueToString(a), tuple_type_->ValueToString(b));
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
