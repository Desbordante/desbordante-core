#include "core/algorithms/pac/fd_pac.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace {
std::string DoublesToString(std::vector<double> const& doubles) {
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
}  // namespace

namespace model {
std::string FDPAC::ToShortString() const {
    std::ostringstream oss;
    oss << lhs_.ToString() << " -(" << DoublesToString(epsilons_) << ", " << delta_ << ")-> "
        << rhs_.ToString();
    return oss.str();
}

std::string FDPAC::ToLongString() const {
    std::ostringstream oss;
    oss << "FD PAC d(" << lhs_.ToString() << ") ≤ " << DoublesToString(lhs_Deltas_) << "}) => Pr(d("
        << rhs_.ToString() << ") ≤ " << DoublesToString(epsilons_) << ") ≥ " << delta_;
    return oss.str();
}

bool FDPAC::operator==(FDPAC const& other) const {
    constexpr static auto kThreshold = 1e-12;
    auto approx_equal = [](double a, double b) { return std::abs(a - b) < kThreshold; };

    auto epsilons_equal = std::ranges::equal(epsilons_, other.epsilons_, approx_equal);
    auto lhs_deltas_equal = std::ranges::equal(lhs_Deltas_, other.lhs_Deltas_, approx_equal);
    return epsilons_equal && approx_equal(delta_, other.delta_) && lhs_deltas_equal &&
           lhs_ == other.lhs_ && rhs_ == other.rhs_;
}
}  // namespace model
