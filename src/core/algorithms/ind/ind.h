#pragma once

#include <memory>
#include <string>

#include "model/table/column_combination.h"
#include "model/table/relational_schema.h"
#include "model/table/vertical.h"

namespace model {

// Inclusion dependency is a relation between attributes of tables
// that indicates possible Primary Key–Foreign Key references.
class IND {
private:
    std::shared_ptr<ColumnCombination> lhs_;
    std::shared_ptr<ColumnCombination> rhs_;
    std::shared_ptr<std::vector<RelationalSchema>> schemas_;

public:
    IND(std::shared_ptr<ColumnCombination> lhs, std::shared_ptr<ColumnCombination> rhs,
        std::shared_ptr<std::vector<RelationalSchema>> schemas)
        : lhs_(std::move(lhs)), rhs_(std::move(rhs)), schemas_(std::move(schemas)) {}

    ColumnCombination const& GetLhs() const {
        return *lhs_;
    }

    ColumnCombination const& GetRhs() const {
        return *rhs_;
    }

    bool StartsWith(IND const& other) const noexcept {
        return GetLhs().StartsWith(other.GetLhs()) && GetRhs().StartsWith(other.GetRhs());
    }

    ArityIndex GetArity() const noexcept {
        return lhs_->GetArity();
    }

    std::string ToShortString() const;

    std::string ToLongString() const;
};

}  // namespace model
