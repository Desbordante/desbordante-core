#include "core/algorithms/pac/fd_pac.h"

#include <sstream>
#include <string>
#include <vector>

namespace model {
std::string FDPAC::ToLongString() const {
    auto doubles_to_str = [](std::vector<double> const& doubles) {
        if (doubles.size() == 1) {
            return std::to_string(doubles.front());
        }

        std::ostringstream res;
        res << '{';
        for (auto it = doubles.begin(); it != doubles.end(); ++it) {
            if (it != doubles.begin()) {
                res << ", ";
            }
            res << *it;
        }
        res << '}';
        return res.str();
    };

    std::ostringstream oss;
    oss << "FD PAC d(" << lhs_.ToString() << ") ≤ " << doubles_to_str(lhs_Deltas_) << "}) => Pr(d("
        << rhs_.ToString() << ") ≤ " << doubles_to_str(GetEpsilons()) << ") ≥ " << GetDelta();
    return oss.str();
}
}  // namespace model
