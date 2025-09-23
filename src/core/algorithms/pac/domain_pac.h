#pragma once

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "core/model/table/vertical.h"

namespace model {
/// @brief Given an ordered domain D on an attribute, a Domain PAC specifies that all attribute
/// values fall within @c epsilon of D with at least probability @c delta.
class DomainPAC {
private:
    double epsilon_;
    double delta_;
    std::string domain_name_;
    Vertical columns_;

    std::string StringStem(std::string const& arg) const {
        std::ostringstream oss;
        oss << "Pr(" << arg << " ∈ " << domain_name_ << "±" << GetEpsilon() << ") ≥ " << GetDelta();
        return oss.str();
    }

public:
    DomainPAC(double epsilon, double delta, std::string const& domain_name, Vertical const& columns)
        : epsilon_(epsilon), delta_(delta), domain_name_(domain_name), columns_(columns) {}

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

    Vertical const& GetColumns() const {
        return columns_;
    }

    std::vector<std::string> GetColumnNames() const {
        std::vector<std::string> result;
        result.reserve(columns_.GetArity());
        std::ranges::transform(columns_.GetColumns(), std::back_inserter(result),
                               std::mem_fn(&Column::GetName));
        return result;
    }

    std::string const& GetDomainName() const {
        return domain_name_;
    }

    std::string ToShortString() const {
        return StringStem(columns_.ToString());
    }

    std::string ToLongString() const {
        std::ostringstream oss;
        oss << "Domain PAC " << StringStem("x") << " on columns " << columns_.ToString();
        return oss.str();
    }

    bool operator==(DomainPAC const& other) const {
        constexpr static auto kThreshold = 1e-12;

        return std::abs(epsilon_ - other.epsilon_) < kThreshold &&
               std::abs(delta_ - other.delta_) < kThreshold && domain_name_ == other.domain_name_ &&
               columns_ == other.columns_;
    }
};
}  // namespace model
