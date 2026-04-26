#pragma once

#include <string>
#include <string_view>

#include "core/model/table/vertical.h"

namespace model {
/// @brief An Unique Column Combination Probabilistic Approximate Constraint on column set X
/// specifies that
///   Pr(dist(t_i[A_l], t_j[A_l]) <= eps) <= delta for each A_l in X
class UCCPAC {
private:
    double epsilon_;
    double delta_;
    Vertical columns_;

	std::string StringStem(std::string_view const arg) const;

public:
    UCCPAC() = default;

    UCCPAC(const UCCPAC&) = default;
    UCCPAC(UCCPAC&&) = default;
    UCCPAC& operator=(const UCCPAC&) = default;
    UCCPAC& operator=(UCCPAC&&) = default;

    ~UCCPAC() = default;

    UCCPAC(Vertical&& columns, double epsilon, double delta)
        : epsilon_(epsilon), delta_(delta), columns_(std::move(columns)) {}

    double GetEpsilon() const {
        return epsilon_;
    }

    double GetDelta() const {
        return delta_;
    }

    Vertical const& GetColumns() const {
        return columns_;
    }

    std::string ToShortString() const;
    std::string ToLongString() const;
    bool operator==(UCCPAC const&) const;
};
}  // namespace model
