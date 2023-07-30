#include <sstream>
#include "data_and_index.h"

using namespace algos::fastod;

DataAndIndex::DataAndIndex(int data, int index) noexcept : pair_(std::make_pair(data, index)) {}

std::string DataAndIndex::ToString() const noexcept {
    std::stringstream ss;

    ss << "DataAndIndex{data=" << this->pair_.first << ", index=" << this->pair_.second << "}";

    return ss.str();
}

namespace algos::fastod {

bool operator<(DataAndIndex const& x, DataAndIndex const& y) {
    return x.pair_.first - y.pair_.first;
}

} // namespace algos::fastod
