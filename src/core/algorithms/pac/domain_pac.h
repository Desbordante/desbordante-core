#pragma once

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "algorithms/pac/model/idomain.h"
#include "algorithms/pac/pac.h"
#include "table/vertical.h"

namespace model {
/// @brief Given an ordered domain D on an attribute, a Domain PAC specifies that all attribute
/// values fall within @c epsilon of D with at least probability @c delta.
class DomainPAC : public PAC {
private:
    std::shared_ptr<pac::model::IDomain> domain_;
    Vertical columns_;

    std::string StringStem(std::string const& arg) const {
        std::ostringstream oss;
        oss << "Pr(" << arg << " ∈ " << domain_->ToString() << "±" << epsilon_ << ") ≥ " << delta_;
        return oss.str();
    }

public:
    DomainPAC(double epsilon, double delta, std::shared_ptr<pac::model::IDomain> domain,
              Vertical const& columns)
        : PAC(epsilon, delta), domain_(std::move(domain)), columns_(columns) {}

    DomainPAC(DomainPAC const&) = default;
    DomainPAC(DomainPAC&&) = default;
    DomainPAC& operator=(DomainPAC const&) = default;
    DomainPAC& operator=(DomainPAC&&) = default;

    Vertical const& GetColumns() const {
        return columns_;
    }

    std::vector<std::string> GetColumnNames() const {
        std::vector<std::string> result;
        std::ranges::transform(columns_.GetColumns(), std::back_inserter(result),
                               std::mem_fn(&Column::GetName));
        return result;
    }

    pac::model::IDomain const& GetDomain() const {
        return *domain_;
    }

    std::shared_ptr<pac::model::IDomain> GetDomainPtr() {
        return domain_;
    }

    virtual std::string ToShortString() const override {
        return StringStem(columns_.ToString());
    }

    virtual std::string ToLongString() const override {
        std::ostringstream oss;
        oss << "Domain PAC " << StringStem("x") << " on columns " << columns_.ToString();
        return oss.str();
    }
};
}  // namespace model
