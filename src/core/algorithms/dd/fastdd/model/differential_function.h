#pragma once

#include <string>

#include "core/algorithms/dd/dd.h"
#include "core/algorithms/dd/fastdd/model/operator.h"
#include "core/model/table/column.h"

namespace algos::dd {

class DifferentialFunction {
private:
    model::DFConstraint constraint_;
    Column const* column_;

public:
    DifferentialFunction(model::DFConstraint constraint, Column const* column)
        : constraint_(constraint), column_(column) {}

    bool operator==(DifferentialFunction const& other) const = default;

    model::DFConstraint GetConstraint() const {
        return constraint_;
    }

    Column const* GetColumn() const {
        return column_;
    }

    std::string ToString() const {
        return column_->GetName() + constraint_.ToString();
    }
};

}  // namespace algos::dd
