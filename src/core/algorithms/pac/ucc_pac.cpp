#include "core/algorithms/pac/ucc_pac.h"

#include <sstream>
#include <string>
#include <string_view>

#include "core/algorithms/pac/util/columns_utils.h"

namespace model {
std::string UCCPAC::StringStem(std::string_view const arg) const {
    std::ostringstream oss;
    oss << "Pr(dist(" << arg << ") ≤ " << epsilon_ << ") ≤ " << delta_;
    return oss.str();
}

std::string UCCPAC::ToShortString() const {
    return StringStem(pac::util::ColumnNamesToString(column_names_));
}

std::string UCCPAC::ToLongString() const {
    std::ostringstream oss;
    oss << "UCC PAC " << StringStem("x") << " on columns "
        << pac::util::ColumnNamesToString(column_names_);
    return oss.str();
}

bool UCCPAC::operator==(UCCPAC const& other) const {
    constexpr static auto kThreshold = 1e-12;

    return std::abs(epsilon_ - other.epsilon_) < kThreshold &&
           std::abs(delta_ - other.delta_) < kThreshold && column_indices_ == other.column_indices_;
}
}  // namespace model
