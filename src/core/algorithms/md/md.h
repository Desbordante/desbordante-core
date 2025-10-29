#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "algorithms/md/column_match.h"
#include "algorithms/md/column_similarity_classifier.h"
#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/lhs_column_similarity_classifier.h"
#include "model/index.h"
#include "model/table/relational_schema.h"

class RelationalSchema;

namespace model {
namespace md {
struct ColumnMatch;
}  // namespace md

struct ColumnDescription {
    std::string column_name;
    Index column_index;
};

struct ColumnMatchDescription {
    ColumnDescription left_column_description;
    ColumnDescription right_column_description;
    std::string column_match_name;
};

struct LhsSimilarityClassifierDesctription {
    ColumnMatchDescription column_match_description;
    md::DecisionBoundary decision_boundary;
    std::optional<md::DecisionBoundary> max_invalid_bound;
};

struct RhsSimilarityClassifierDesctription {
    ColumnMatchDescription column_match_description;
    md::DecisionBoundary decision_boundary;
};

struct MDDescription {
    std::string left_table_name;
    std::string right_table_name;
    std::vector<LhsSimilarityClassifierDesctription> lhs;
    RhsSimilarityClassifierDesctription rhs;
};

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
    // TODO: add support

    [[nodiscard]] ColumnMatchDescription GetColumnMatchDescription(Index index) const;

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

    [[nodiscard]] std::shared_ptr<RelationalSchema const> const& GetLeftSchema() const {
        return left_schema_;
    }

    [[nodiscard]] std::shared_ptr<RelationalSchema const> const& GetRightSchema() const {
        return right_schema_;
    }

    [[nodiscard]] std::shared_ptr<std::vector<md::ColumnMatch> const> const& GetColumnMatches()
            const {
        return column_matches_;
    }

    [[nodiscard]] std::vector<md::LhsColumnSimilarityClassifier> const& GetLhs() const {
        return lhs_;
    }

    [[nodiscard]] bool SingleTable() const noexcept {
        return left_schema_ == right_schema_;
    }

    [[nodiscard]] MDDescription GetDescription() const;
};

}  // namespace model
