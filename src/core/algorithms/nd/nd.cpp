#include "core/algorithms/nd/nd.h"

#include <algorithm>

#include "core/algorithms/nd/util/get_vertical_names.h"
#include "core/util/range_to_string.h"

namespace model {

[[nodiscard]] std::vector<std::string> ND::GetLhsNames() const {
    return algos::nd::util::GetVerticalNames(GetLhs());
}

[[nodiscard]] std::vector<std::string> ND::GetRhsNames() const {
    return algos::nd::util::GetVerticalNames(GetRhs());
}

std::string ND::ToShortString() const {
    using namespace util;
    return RangeToString(GetLhsIndices()) + " -> " + RangeToString(GetRhsIndices());
}

std::string ND::ToLongString() const {
    using namespace util;
    return RangeToString(GetLhsNames()) + " -" + std::to_string(GetWeight()) + "-> " +
           RangeToString(GetRhsNames());
}

[[nodiscard]] std::tuple<std::vector<std::string>, std::vector<std::string>, WeightType>
ND::ToNameTuple() const {
    return std::make_tuple(GetLhsNames(), GetRhsNames(), GetWeight());
}

}  // namespace model
