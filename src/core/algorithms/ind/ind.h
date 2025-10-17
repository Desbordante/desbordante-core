#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "config/error/type.h"
#include "model/table/column_combination.h"
#include "model/table/relational_schema.h"
#include "model/table/vertical.h"
#include "table/arity_index.h"

class RelationalSchema;

namespace model {

///
/// Inclusion dependency is a relation between attributes of tables
/// that indicates possible Primary Key–Foreign Key references.
///
/// \note It also stores an error threshold in the interval [0, 1], where
///       lower value indicate a higher degree of IND satisfaction.
///
class IND {
private:
    std::shared_ptr<ColumnCombination> lhs_;
    std::shared_ptr<ColumnCombination> rhs_;
    std::shared_ptr<std::vector<RelationalSchema>> schemas_;
    config::ErrorType error_;

public:
    IND(std::shared_ptr<ColumnCombination> lhs, std::shared_ptr<ColumnCombination> rhs,
        std::shared_ptr<std::vector<RelationalSchema>> schemas, config::ErrorType error = 0.0)
        : lhs_(std::move(lhs)), rhs_(std::move(rhs)), schemas_(std::move(schemas)), error_(error) {}

    ColumnCombination const& GetLhs() const {
        return *lhs_;
    }

    ColumnCombination const& GetRhs() const {
        return *rhs_;
    }

    std::shared_ptr<std::vector<RelationalSchema>> const& GetSchemas() const {
        return schemas_;
    }

    ///
    /// Get the error threshold at which AIND holds.
    ///
    /// \note Informally, this is the proportion of values in the dataset stream
    ///       that would need to be removed (independently of their occurrences)
    ///       in order to produce a modified dataset stream where the IND holds.
    ///
    config::ErrorType GetError() const {
        return error_;
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
