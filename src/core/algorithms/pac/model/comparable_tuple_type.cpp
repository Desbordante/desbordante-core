#include "algorithms/pac/model/comparable_tuple_type.h"

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

namespace pac::model {
std::string ComparableTupleType::ValueToString(Tuple const& value) const {
    assert(value.size() >= types_.size());

    if (types_.size() == 1) {
        return types_.front()->ValueToString(value.front());
    }
    std::ostringstream oss;
    oss << '{';
    for (std::size_t i = 0; i < types_.size(); ++i) {
        if (i > 0) {
            oss << ", ";
        }
        oss << types_[i]->ValueToString(value[i]);
    }
    oss << '}';
    return oss.str();
}
}  // namespace pac::model
