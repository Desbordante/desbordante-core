#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/algorithms/pac/util/columns_utils.h"
#include "core/config/indices/type.h"
#include "core/model/table/vertical.h"

namespace model {
/// @brief An Unique Column Combination Probabilistic Approximate Constraint on column set X
/// specifies that
///   Pr(dist(t_i[A_l], t_j[A_l]) <= eps) <= delta for each A_l in X
class UCCPAC {
private:
    double epsilon_;
    double delta_;

    config::IndicesType column_indices_;
    std::vector<std::string> column_names_;

    std::string StringStem(std::string_view const arg) const;

public:
    UCCPAC() = default;

    UCCPAC(const UCCPAC&) = default;
    UCCPAC(UCCPAC&&) = default;
    UCCPAC& operator=(const UCCPAC&) = default;
    UCCPAC& operator=(UCCPAC&&) = default;

    ~UCCPAC() = default;

    UCCPAC(config::IndicesType&& column_indices, std::vector<std::string>&& column_names,
           double epsilon, double delta)
        : epsilon_(epsilon),
          delta_(delta),
          column_indices_(std::move(column_indices)),
          column_names_(std::move(column_names)) {}

    UCCPAC(Vertical&& columns, double epsilon, double delta)
        : UCCPAC(columns.GetColumnIndicesAsVector(), pac::util::GetColumnNames(columns), epsilon,
                 delta) {}

    double GetEpsilon() const {
        return epsilon_;
    }

    double GetDelta() const {
        return delta_;
    }

    config::IndicesType const& GetColumnIndices() const {
        return column_indices_;
    }

    std::vector<std::string> const& GetColumnNames() const {
        return column_names_;
    }

    std::string ToShortString() const;
    std::string ToLongString() const;
    bool operator==(UCCPAC const&) const;
};
}  // namespace model
