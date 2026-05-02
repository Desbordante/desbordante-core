#include "core/algorithms/pac/ucc_pac.h"

#include <sstream>
#include <string>
#include <string_view>

namespace model {
std::string UCCPAC::StringStem(std::string_view const arg) const {
    std::ostringstream oss;
    oss << "Pr(dist(" << arg << ") ≤ " << epsilon_ << ") ≤ " << delta_;
    return oss.str();
}

std::string UCCPAC::ToShortString() const {
    return StringStem(columns_.ToString());
}

std::string UCCPAC::ToLongString() const {
    std::ostringstream oss;
    oss << "UCC PAC " << StringStem("x") << " on columns " << columns_.ToString();
    return oss.str();
}

bool UCCPAC::operator==(UCCPAC const& other) const {
    constexpr static auto kThreshold = 1e-12;

    return std::abs(epsilon_ - other.epsilon_) < kThreshold &&
           std::abs(delta_ - other.delta_) < kThreshold && columns_ == other.columns_;
}
}  // namespace model
