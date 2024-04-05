#pragma once

#include "model/table/column.h"
#include "model/table/vertical.h"

using WeightType = unsigned;

class ND {
private:
    Vertical lhs_;
    Vertical rhs_;
    WeightType weight_;

public:
    ND(Vertical const& lhs, Vertical const& rhs, WeightType weight)
        : lhs_(lhs), rhs_(rhs), weight_(weight) {}

    Vertical const& GetLhs() const {
        return lhs_;
    }

    Vertical const& GetRhs() const {
        return rhs_;
    }

    WeightType GetWeight() const {
        return weight_;
    }

    std::vector<model::ColumnIndex> GetLhsIndices() const {
        return lhs_.GetColumnIndicesAsVector();
    }

    std::vector<model::ColumnIndex> GetRhsIndices() const {
        return rhs_.GetColumnIndicesAsVector();
    }

    std::string ToShortString() const;

    std::string ToLongString() const;
};
