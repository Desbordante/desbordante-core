#include "core/algorithms/pac/model/tuple_type.h"

#include <iterator>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "core/model/types/builtin.h"

namespace pac::model {
::model::CompareResult TupleType::CompareBytes(std::size_t const type_num, std::byte const* x,
                                               std::byte const* y) {
    assert(type_num < types_.size());

    if (x == y) {
        return ::model::CompareResult::kEqual;
    }
    if (x == nullptr) {
        return ::model::CompareResult::kLess;
    }
    if (y == nullptr) {
        return ::model::CompareResult::kGreater;
    }
    return types_[type_num]->Compare(x, y);
}

std::string TupleType::ValueToString(Tuple const& value) const {
    assert(value.size() == types_.size());

    if (types_.size() == 1) {
        return ByteToString(0, value.front());
    }
    std::ostringstream oss;
    oss << '{';
    for (std::size_t i = 0; i < types_.size(); ++i) {
        if (i > 0) {
            oss << ", ";
        }
        oss << ByteToString(i, value[i]);
    }
    oss << '}';
    return oss.str();
}

std::string StringValueToString(std::vector<std::string> const& strings) {
    using namespace std::string_literals;

    if (strings.size() == 1) {
        return strings.front();
    }
    return std::accumulate(std::next(strings.begin()), strings.end(), "{"s + strings.front(),
                           [](std::string&& acc, std::string const& val) {
                               return std::move(acc) + ", "s + val;
                           }) +
           "}"s;
}
}  // namespace pac::model
