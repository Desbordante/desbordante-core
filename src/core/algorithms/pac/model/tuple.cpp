#include "core/algorithms/pac/model/tuple.h"

#include <cassert>
#include <string>
#include <vector>

#include "core/model/types/type.h"

namespace pac::model {
std::string TupleToString(Tuple const& tp, std::vector<::model::Type const*> const& types) {
    assert(tp.size() == types.size());

    if (tp.size() == 1) {
        return types.front()->ValueToString(tp.front());
    }

    std::ostringstream result;
    result << '[';
    for (std::size_t i = 0; i < tp.size(); ++i) {
        if (i > 0) {
            result << ", ";
        }
        result << types[i]->ValueToString(tp[i]);
    }
    result << ']';
    return result.str();
}
}  // namespace pac::model
