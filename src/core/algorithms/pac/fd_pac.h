#pragma once

#include <cstdlib>
#include <string>
#include <vector>

#include "core/model/table/vertical.h"

namespace model {
/// @brief A Functional Dependency Probabilistic Approximate Constraint X -> Y, with given Delta_l,
/// specifies that
///     if |t_i[A_l] - t_j[A_l]| <= Delta_l for each A_l in X,
///     then Pr(|t_i[B_l] - t_j[b_l]| <= eps_l) >= delta for each B_l in Y
class FDPAC {
private:
    std::vector<double> epsilons_;
    double delta_;
    Vertical lhs_;
    Vertical rhs_;
    std::vector<double> lhs_Deltas_;

public:
    FDPAC() = default;

    FDPAC(FDPAC const&) = default;
    FDPAC(FDPAC&&) = default;
    FDPAC& operator=(FDPAC const&) = default;
    FDPAC& operator=(FDPAC&&) = default;

    FDPAC(Vertical&& lhs, Vertical&& rhs, std::vector<double>&& lhs_Deltas,
          std::vector<double>&& epsilons, double delta)
        : epsilons_(std::move(epsilons)),
          delta_(delta),
          lhs_(std::move(lhs)),
          rhs_(std::move(rhs)),
          lhs_Deltas_(std::move(lhs_Deltas)) {}

    std::vector<double> const& GetEpsilons() const {
        return epsilons_;
    }

    double GetDelta() const {
        return delta_;
    }

    std::vector<double> const& GetLhsDeltas() const {
        return lhs_Deltas_;
    }

    Vertical const& GetLhs() const {
        return lhs_;
    }

    Vertical const& GetRhs() const {
        return rhs_;
    }

    std::string ToShortString() const;
    std::string ToLongString() const;
    bool operator==(FDPAC const&) const;
};
}  // namespace model
