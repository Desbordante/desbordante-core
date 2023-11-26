#include "ind.h"

#include <sstream>

namespace model {

std::string IND::ToString() const {
    std::stringstream ss;
    ss << GetLhs().ToString() << " -> " << GetRhs().ToString();
    return ss.str();
}

}  // namespace model
