#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "core/algorithms/pac/pac.h"
#include "core/model/table/relational_schema.h"
#include "core/model/table/vertical.h"

namespace model {
/// @brief A Functional Dependency Probabilistic Approximate Constraint X -> Y, with given Delta_l,
/// specifies that
///		if |t_i[A_l] - t_j[A_l]| <= Delta_l for each A_l in X,
/// 	then Pr(|t_i[B_l] - t_j[b_l]| <= eps_l) >= delta for each B_l in Y
class FDPAC : public PAC {
private:
    Vertical lhs_;
    Vertical rhs_;
    std::vector<double> lhs_Deltas_;

public:
    FDPAC() = default;

    FDPAC(FDPAC const&) = default;
    FDPAC(FDPAC&&) = default;
    FDPAC& operator=(FDPAC const&) = default;
    FDPAC& operator=(FDPAC&&) = default;

    FDPAC(std::shared_ptr<RelationalSchema const> rel_schema, Vertical&& lhs, Vertical&& rhs,
          std::vector<double>&& lhs_Deltas, std::vector<double>&& epsilons, double delta)
        : PAC(std::move(rel_schema), std::move(epsilons), {delta}),
          lhs_(std::move(lhs)),
          rhs_(std::move(rhs)),
          lhs_Deltas_(std::move(lhs_Deltas)) {}

    Vertical const& GetLhs() const {
        return lhs_;
    }

    Vertical const& GetRhs() const {
        return rhs_;
    }

    std::vector<double> const& GetLhsDeltas() const {
        return lhs_Deltas_;
    }

    std::string ToShortString() const override {
        std::ostringstream oss;
        oss << lhs_.ToString() << " -(" << GetEpsilon() << ", " << GetDelta() << ")-> "
            << rhs_.ToString();
        return oss.str();
    }

	std::string ToLongString() const override;
};
}  // namespace model
