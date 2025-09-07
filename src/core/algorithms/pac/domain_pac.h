#pragma once

#include <sstream>

#include "algorithms/pac/model/domain.h"
#include "algorithms/pac/pac.h"
#include "table/vertical.h"

namespace model {
/// @brief Given an ordered domain D on an attribute, a Domain PAC specifies that all attribute
/// values fall within @c epsilon of D with at least probability @c delta.
class DomainPAC : public PAC {
private:
    pac::model::Domain domain_;
    Vertical columns_;

    std::string StringStem(std::string const& arg) const {
        std::ostringstream oss;
        oss << "Pr(" << arg << " ∈ " << domain_.ToString() << "±" << epsilon_ << ") ≥ " << delta_;
        return oss.str();
    }

public:
    DomainPAC(double epsilon, double delta, pac::model::Domain&& domain, Vertical const& columns)
        : PAC(epsilon, delta), domain_(std::move(domain)), columns_(columns) {}

    Vertical const& GetColumns() const {
        return columns_;
    }

    pac::model::Domain const& GetDomain() const {
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
