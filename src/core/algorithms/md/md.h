#pragma once

#include <memory>

#include "algorithms/md/column_match.h"
#include "algorithms/md/column_similarity_classifier.h"
#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/lhs_column_similarity_classifier.h"
#include "model/index.h"
#include "model/table/relational_schema.h"

namespace model {

// Based on the definition given in the article titled "Efficient Discovery of
// Matching Dependencies" by Philipp Schirmer, Thorsten Papenbrock, Ioannis
// Koumarelas, and Felix Naumann.
class MD {
private:
    std::shared_ptr<RelationalSchema const> left_schema_;
    std::shared_ptr<RelationalSchema const> right_schema_;
    std::shared_ptr<std::vector<md::ColumnMatch> const> column_matches_;
    std::vector<md::LhsColumnSimilarityClassifier> lhs_;
    md::ColumnSimilarityClassifier rhs_;

public:
    MD(std::shared_ptr<RelationalSchema const> left_schema,
       std::shared_ptr<RelationalSchema const> right_schema,
       std::shared_ptr<std::vector<md::ColumnMatch> const> column_matches,
       std::vector<md::LhsColumnSimilarityClassifier> lhs,
       md::ColumnSimilarityClassifier rhs) noexcept;

    [[nodiscard]] std::string ToStringFull() const;
    [[nodiscard]] std::string ToStringShort() const;
    [[nodiscard]] std::string ToStringActiveLhsOnly() const;
    [[nodiscard]] std::vector<md::DecisionBoundary> GetLhsDecisionBounds() const;
    [[nodiscard]] std::pair<Index, md::DecisionBoundary> GetRhs() const noexcept;
};

}  // namespace model
