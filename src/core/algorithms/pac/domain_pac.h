#pragma once

#include <cstddef>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

#include "core/algorithms/pac/util/columns_utils.h"
#include "core/config/column_index/type.h"
#include "core/model/table/vertical.h"

namespace model {
/// @brief Given an ordered domain D on an attribute, a Domain PAC specifies that all attribute
/// values fall within @c epsilon of D with at least probability @c delta.
class DomainPAC {
private:
    double epsilon_;
    double delta_;
    std::string domain_name_;

    std::vector<config::IndexType> column_indices_;
    std::vector<std::string> column_names_;

    std::string StringStem(std::string const& arg) const {
        std::ostringstream oss;
        oss << "Pr(" << arg << " ∈ " << domain_name_ << "±" << GetEpsilon() << ") ≥ " << GetDelta();
        return oss.str();
    }

public:
    DomainPAC(double epsilon, double delta, std::string const& domain_name,
              std::vector<config::IndexType>&& column_indices,
              std::vector<std::string>&& column_names)
        : epsilon_(epsilon),
          delta_(delta),
          domain_name_(domain_name),
          column_indices_(std::move(column_indices)),
          column_names_(std::move(column_names)) {}

    DomainPAC(double epsilon, double delta, std::string const& domain_name, Vertical const& columns)
        : DomainPAC(epsilon, delta, domain_name, columns.GetColumnIndicesAsVector(),
                    pac::util::GetColumnNames(columns)) {}

    DomainPAC(DomainPAC const&) = default;
    DomainPAC(DomainPAC&&) = default;
    DomainPAC& operator=(DomainPAC const&) = default;
    DomainPAC& operator=(DomainPAC&&) = default;

    double GetEpsilon() const {
        return epsilon_;
    }

    double GetDelta() const {
        return delta_;
    }

    std::vector<config::IndexType> const& GetColumnIndices() const {
        return column_indices_;
    }

    std::vector<std::string> const& GetColumnNames() const {
        return column_names_;
    }

    std::string const& GetDomainName() const {
        return domain_name_;
    }

    std::string ToShortString() const {
        return StringStem(pac::util::ColumnNamesToString(column_names_));
    }

    std::string ToLongString() const {
        std::ostringstream oss;
        oss << "Domain PAC " << StringStem("x") << " on columns "
            << pac::util::ColumnNamesToString(column_names_);
        return oss.str();
    }

    bool operator==(DomainPAC const& other) const {
        constexpr static auto kThreshold = 1e-12;

        return std::abs(epsilon_ - other.epsilon_) < kThreshold &&
               std::abs(delta_ - other.delta_) < kThreshold && domain_name_ == other.domain_name_ &&
               column_indices_ == other.column_indices_;
    }
};
}  // namespace model
