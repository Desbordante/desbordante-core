#pragma once

#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include "model/table/column.h"
#include "model/table/vertical.h"
#include "table/column_index.h"

namespace model {

using WeightType = unsigned int;

class ND {
private:
    Vertical lhs_;
    Vertical rhs_;
    WeightType weight_;

public:
    ND(Vertical const& lhs, Vertical const& rhs, WeightType weight)
        : lhs_(lhs), rhs_(rhs), weight_(weight) {}

    [[nodiscard]] Vertical const& GetLhs() const {
        return lhs_;
    }

    [[nodiscard]] Vertical const& GetRhs() const {
        return rhs_;
    }

    [[nodiscard]] std::vector<std::string> GetLhsNames() const;

    [[nodiscard]] std::vector<std::string> GetRhsNames() const;

    [[nodiscard]] WeightType GetWeight() const {
        return weight_;
    }

    [[nodiscard]] std::vector<ColumnIndex> GetLhsIndices() const {
        return lhs_.GetColumnIndicesAsVector();
    }

    [[nodiscard]] std::vector<ColumnIndex> GetRhsIndices() const {
        return rhs_.GetColumnIndicesAsVector();
    }

    [[nodiscard]] std::string ToShortString() const;

    [[nodiscard]] std::string ToLongString() const;

    [[nodiscard]] std::tuple<std::vector<std::string>, std::vector<std::string>, WeightType>
    ToNameTuple() const;

    bool operator==(ND const& other) const = default;
    bool operator!=(ND const& other) const = default;
};

}  // namespace model
