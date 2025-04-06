#pragma once

#include <string>

#include "core/algorithms/dd/fastdd/model/operator.h"
#include "core/model/table/column.h"

namespace algos::dd {

class DifferentialFunction {
private:
    Operator operator_;
    double threshold_;
    Column const* column_;

public:
    DifferentialFunction(Operator op, double threshold, Column const* column)
        : operator_(op), threshold_(threshold), column_(column) {}

    Operator GetOperator() const {
        return operator_;
    }

    double GetThreshold() const {
        return threshold_;
    }

    Column const* GetColumn() const {
        return column_;
    }

    std::string ToString() const {
        return "[ " + column_->GetName() + " " + (operator_ == Operator::kGreater ? ">" : "<=") +
               std::to_string(threshold_) + " ]";
    }
};

}  // namespace algos::dd
