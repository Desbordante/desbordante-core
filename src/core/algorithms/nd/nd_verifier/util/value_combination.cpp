#include "algorithms/nd/nd_verifier/util/value_combination.h"

#include <memory>
#include <ostream>
#include <sstream>
#include <string>

#include "builtin.h"
#include "model/types/create_type.h"
#include "model/types/mixed_type.h"
#include "type.h"

namespace algos::nd_verifier::util {

using namespace model;

bool ValueCombination::CompareValues(ValueCombination::TypedValue a,
                                     ValueCombination::TypedValue b) {
    if (a.first != b.first) {
        return false;
    }
    if (a.second == nullptr && b.second == nullptr) {
        return true;
    } else if (a.second == nullptr || b.second == nullptr) {
        return false;
    }
    auto type = CreateType(a.first, false);
    if (a.first == +(TypeId::kMixed)) {
        if (MixedType::RetrieveTypeId(a.second) != MixedType::RetrieveTypeId(b.second)) {
            return false;
        }
    }
    return type->Compare(a.second, b.second) == CompareResult::kEqual;
}

std::string ValueCombination::ToString() const {
    std::stringstream sstream;
    sstream << '(';
    for (auto pt{typed_data_.begin()}; pt != typed_data_.end(); ++pt) {
        if (pt != typed_data_.begin()) {
            sstream << ", ";
        }
        if (pt->second != nullptr) {
            auto type = CreateType(pt->first, false);
            sstream << type->ValueToString(pt->second);
        }
    }
    sstream << ')';
    return sstream.str();
}

std::ostream& operator<<(std::ostream& os, ValueCombination const& vc) {
    os << vc.ToString();
    return os;
}

}  // namespace algos::nd_verifier::util
