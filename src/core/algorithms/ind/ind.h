#pragma once

#include <memory>
#include <string>

#include "model/table/column_combination.h"

namespace model {

// Inclusion dependency is a relation between attributes of tables
// that indicates possible Primary Key–Foreign Key references.
class IND {
private:
    std::shared_ptr<ColumnCombination> lhs_;
    std::shared_ptr<ColumnCombination> rhs_;

public:
    IND(std::shared_ptr<ColumnCombination> lhs, std::shared_ptr<ColumnCombination> rhs)
        : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    ColumnCombination const& GetLhs() const {
        return *lhs_;
    }

    ColumnCombination const& GetRhs() const {
        return *rhs_;
    }

    std::string ToString() const;
};

}  // namespace model
