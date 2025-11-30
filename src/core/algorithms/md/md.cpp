#include "core/algorithms/md/md.h"

#include <ranges>

#include "core/model/table/column.h"
#include "core/util/get_preallocated_vector.h"

namespace model {

MD::MD(std::shared_ptr<RelationalSchema const> left_schema,
       std::shared_ptr<RelationalSchema const> right_schema,
       std::shared_ptr<std::vector<md::ColumnMatch> const> column_matches,
       std::vector<md::LhsColumnSimilarityClassifier> lhs,
       md::ColumnSimilarityClassifier rhs) noexcept
    : left_schema_(std::move(left_schema)),
      right_schema_(std::move(right_schema)),
      column_matches_(std::move(column_matches)),
      lhs_(std::move(lhs)),
      rhs_(rhs) {}

std::string MD::ToStringFull() const {
    std::stringstream ss;
    ss << "[";
    for (auto const& classifier : lhs_) {
        // if (classifier.GetDecisionBoundary() == 0.0) continue;
        auto const& column_match = (*column_matches_)[classifier.GetColumnMatchIndex()];
        ss << " ";
        ss << column_match.name << "(" << left_schema_->GetName() << ":"
           << left_schema_->GetColumn(column_match.left_col_index)->GetName() << ", "
           << right_schema_->GetName() << ":"
           << right_schema_->GetColumn(column_match.right_col_index)->GetName()
           << ")>=" << classifier.GetDecisionBoundary() << " ";
        auto disp = classifier.GetMaxDisprovedBound();
        if (disp.has_value()) {
            ss << "(>" << disp.value() << ") ";
        }
        ss << "|";
    }
    ss.seekp(-1, std::stringstream::cur);
    ss << "] -> ";
    auto const& classifier = rhs_;
    auto const& column_match = (*column_matches_)[classifier.GetColumnMatchIndex()];
    ss << column_match.name << "(" << left_schema_->GetName() << ":"
       << left_schema_->GetColumn(column_match.left_col_index)->GetName() << ", "
       << right_schema_->GetName() << ":"
       << right_schema_->GetColumn(column_match.right_col_index)->GetName()
       << ")>=" << classifier.GetDecisionBoundary();
    return ss.str();
}

std::string MD::ToStringShort() const {
    std::stringstream ss;
    ss << "[";
    for (auto const& classifier : lhs_) {
        model::md::DecisionBoundary const decision_boundary = classifier.GetDecisionBoundary();
        if (decision_boundary != 0.0) {
            ss << decision_boundary;
        }
        ss << ",";
    }
    ss.seekp(-1, std::stringstream::cur);
    ss << "]->";
    ss << rhs_.GetColumnMatchIndex() << "@" << rhs_.GetDecisionBoundary();
    return ss.str();
}

std::string MD::ToStringActiveLhsOnly() const {
    std::stringstream ss;
    ss << "[";
    bool any_active = false;
    for (auto const& classifier : lhs_) {
        md::DecisionBoundary const decision_boundary = classifier.GetDecisionBoundary();
        if (decision_boundary == 0.0) continue;
        any_active = true;
        auto const& column_match = (*column_matches_)[classifier.GetColumnMatchIndex()];
        ss << " ";
        ss << column_match.name << "("
           << left_schema_->GetColumn(column_match.left_col_index)->GetName() << ", "
           << right_schema_->GetColumn(column_match.right_col_index)->GetName()
           << ")>=" << decision_boundary << " ";
        ss << "|";
    }
    if (any_active) ss.seekp(-1, std::stringstream::cur);
    ss << "] -> ";
    auto const& classifier = rhs_;
    auto const& column_match = (*column_matches_)[classifier.GetColumnMatchIndex()];
    ss << column_match.name << "("
       << left_schema_->GetColumn(column_match.left_col_index)->GetName() << ", "
       << right_schema_->GetColumn(column_match.right_col_index)->GetName()
       << ")>=" << classifier.GetDecisionBoundary();
    return ss.str();
}

std::vector<md::DecisionBoundary> MD::GetLhsDecisionBounds() const {
    std::vector<md::DecisionBoundary> decision_bounds;
    decision_bounds.reserve(lhs_.size());
    std::ranges::transform(
            lhs_, std::back_inserter(decision_bounds),
            [](md::LhsColumnSimilarityClassifier const& lhs) { return lhs.GetDecisionBoundary(); });
    return decision_bounds;
}

std::pair<Index, md::DecisionBoundary> MD::GetRhs() const noexcept {
    return {rhs_.GetColumnMatchIndex(), rhs_.GetDecisionBoundary()};
}

ColumnMatchDescription MD::GetColumnMatchDescription(model::Index index) const {
    auto const& [left_col_index, right_col_index, column_match_name] = (*column_matches_)[index];
    return {{left_schema_->GetColumn(left_col_index)->GetName(), left_col_index},
            {right_schema_->GetColumn(right_col_index)->GetName(), right_col_index},
            column_match_name};
}

MDDescription MD::GetDescription() const {
    std::vector<LhsSimilarityClassifierDesctription> lhs_description =
            util::GetPreallocatedVector<LhsSimilarityClassifierDesctription>(lhs_.size());
    for (md::LhsColumnSimilarityClassifier const& lhs_classifier : lhs_) {
        lhs_description.push_back({GetColumnMatchDescription(lhs_classifier.GetColumnMatchIndex()),
                                   lhs_classifier.GetDecisionBoundary(),
                                   lhs_classifier.GetMaxDisprovedBound()});
    }
    return {left_schema_->GetName(),
            right_schema_->GetName(),
            std::move(lhs_description),
            {GetColumnMatchDescription(rhs_.GetColumnMatchIndex()), rhs_.GetDecisionBoundary()}};
}

}  // namespace model
