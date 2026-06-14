#pragma once

#include <string>
#include <vector>

#include "core/algorithms/pac/util/columns_utils.h"
#include "core/config/indices/type.h"
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
    std::vector<double> lhs_Deltas_;

    config::IndicesType lhs_indices_;
    std::vector<std::string> lhs_column_names_;
    config::IndicesType rhs_indices_;
    std::vector<std::string> rhs_column_names_;

public:
    FDPAC() = default;

    FDPAC(FDPAC const&) = default;
    FDPAC(FDPAC&&) = default;
    FDPAC& operator=(FDPAC const&) = default;
    FDPAC& operator=(FDPAC&&) = default;

    FDPAC(config::IndicesType&& lhs_indices, std::vector<std::string>&& lhs_column_names,
          config::IndicesType&& rhs_indices, std::vector<std::string>&& rhs_column_names,
          std::vector<double> const& lhs_Deltas, std::vector<double>&& epsilons, double delta)
        : epsilons_(std::move(epsilons)),
          delta_(delta),
          lhs_Deltas_(lhs_Deltas),
          lhs_indices_(std::move(lhs_indices)),
          lhs_column_names_(std::move(lhs_column_names)),
          rhs_indices_(std::move(rhs_indices)),
          rhs_column_names_(std::move(rhs_column_names)) {}

    FDPAC(Vertical&& lhs, Vertical&& rhs, std::vector<double> const& lhs_Deltas,
          std::vector<double>&& epsilons, double delta)
        : FDPAC(lhs.GetColumnIndicesAsVector(), pac::util::GetColumnNames(lhs),
                rhs.GetColumnIndicesAsVector(), pac::util::GetColumnNames(rhs),
                std::move(lhs_Deltas), std::move(epsilons), delta) {}

    std::vector<double> const& GetEpsilons() const {
        return epsilons_;
    }

    double GetDelta() const {
        return delta_;
    }

    std::vector<double> const& GetLhsDeltas() const {
        return lhs_Deltas_;
    }

    config::IndicesType const& GetLhsColumnIndices() const {
        return lhs_indices_;
    }

    std::vector<std::string> const& GetLhsColumnNames() const {
        return lhs_column_names_;
    }

    config::IndicesType const& GetRhsColumnIndices() const {
        return rhs_indices_;
    }

    std::vector<std::string> const& GetRhsColumnNames() const {
        return rhs_column_names_;
    }

    std::string ToShortString() const;
    std::string ToLongString() const;
    bool operator==(FDPAC const&) const;
};
}  // namespace model
