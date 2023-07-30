#include <sstream>
#include "data/SchemaValue.h"
#include "value_pair.h"

using namespace algos::fastod;

ValuePair::ValuePair(SchemaValue first, SchemaValue second) noexcept : pair_(std::make_pair(first, second)) {}

SchemaValue ValuePair::GetFirst() const noexcept {
    return this->pair_.first;
}

SchemaValue ValuePair::GetSecond() const noexcept {
    return this->pair_.second;
}

std::string ValuePair::ToString() const noexcept {
    std::stringstream ss;

    ss << "DataAndIndex{first=" << GetFirst().ToString() << ", second=" << GetSecond().ToString() << "}";

    return ss.str();
}

namespace algos::fastod {

bool operator<(const ValuePair& x, const ValuePair& y) {
    return x.GetFirst() < y.GetSecond();
}

} // namespace algos::fastod
