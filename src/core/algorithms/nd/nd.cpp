#include "algorithms/nd/nd.h"

#include "algorithms/nd/nd_verifier/util/vector_to_string.h"
#include "algorithms/nd/util/get_vertical_names.h"

namespace model {

[[nodiscard]] std::vector<std::string> ND::GetLhsNames() const {
    return algos::nd::util::GetVerticalNames(GetLhs());
}

[[nodiscard]] std::vector<std::string> ND::GetRhsNames() const {
    return algos::nd::util::GetVerticalNames(GetRhs());
}

std::string ND::ToShortString() const {
    using namespace algos::nd_verifier::util;
    return VectorToString(GetLhsIndices()) + " -> " + VectorToString(GetRhsIndices());
}

std::string ND::ToLongString() const {
    using namespace algos::nd_verifier::util;
    return VectorToString(GetLhsNames()) + " -" + std::to_string(GetWeight()) + "-> " +
           VectorToString(GetRhsNames());
}

[[nodiscard]] std::tuple<std::vector<std::string>, std::vector<std::string>, WeightType>
ND::ToNameTuple() const {
    return std::make_tuple(GetLhsNames(), GetRhsNames(), GetWeight());
}

}  // namespace model
