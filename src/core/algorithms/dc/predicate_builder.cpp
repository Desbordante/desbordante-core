#include "predicate_builder.h"

#include <easylogging++.h>

#include "index_provider.h"
#include "operator.h"
#include "predicate_provider.h"
#include "typed_column_data_value_differences.h"

namespace model {

PredicateBuilder::PredicateBuilder(bool allow_different_columns, double minimum_shared_value,
                                   double comparable_threshold)
    : allow_cross_columns_(allow_different_columns),
      minimum_shared_value_(minimum_shared_value),
      comparable_threshold_(comparable_threshold) {
    PredicateProvider::CreateInstance();
    PredicateIndexProvider::CreateInstance();
}

PredicateBuilder::~PredicateBuilder() {
    PredicateProvider::ClearInstance();
    PredicateIndexProvider::ClearInstance();
}

void PredicateBuilder::BuildPredicateSpace(std::vector<TypedColumnData> const& input) {
    BuildAndCategorizePredicates(input);

    // Populate global PredicateIndexProvider with built predicates.
    PredicateIndexProvider::GetInstance()->AddAll(predicates_);
    BuildMutexMap();
    BuildInverseMap();

    LOG(DEBUG) <<" [Predicate] Predicate space size: " <<predicates_.size();
}

static size_t PredIdx(PredicatePtr const& p) {
    return PredicateIndexProvider::GetInstance()->GetIndex(p);
}

void PredicateBuilder::BuildMutexMap() {
    mutex_map_.resize(predicates_.size());

    for (auto const& p1 : predicates_) {
        for (auto const& p2 : predicates_) {
            if (p2->HasSameOperandsAs(p1)) mutex_map_[PredIdx(p1)].set(PredIdx(p2));
        }
    }
}

void PredicateBuilder::BuildInverseMap() {
    inverse_map_.resize(predicates_.size());

    for (auto const& p1 : predicates_) {
        inverse_map_[PredIdx(p1)] = PredIdx(p1->GetInverse());
    }
}

void PredicateBuilder::BuildAndCategorizePredicates(std::vector<TypedColumnData> const& input) {
    size_t columns_num = input.size();
    predicates_.reserve((columns_num * (columns_num + 1)) / 2);

    for (size_t i = 0; i < columns_num; ++i) {
        for (size_t j = i; j < columns_num; ++j) {
            ProcessColumnPair(i, j, input);
        }
    }
}

void PredicateBuilder::ProcessColumnPair(size_t i, size_t j,
                                         std::vector<TypedColumnData> const& input) {
    bool joinable = IsJoinable(input[i], input[j]);
    bool comparable = IsComparable(input[i], input[j]);

    if (joinable || comparable) {
        AddAndCategorizePredicate(ColumnOperand(input[i].GetColumn(), true),
                                  ColumnOperand(input[j].GetColumn(), false), comparable);
    }
}

void PredicateBuilder::CategorizeLastPredicate(bool comparable) {
    auto const& pred = predicates_.back();

    if (comparable) {
        if (pred->IsCrossColumn())
            num_cross_column_predicates_.push_back(pred);
        else
            num_single_column_predicates_.push_back(pred);
    } else {
        if (pred->IsCrossColumn())
            str_cross_column_predicates_.push_back(pred);
        else
            str_single_column_predicates_.push_back(pred);
    }
}

void PredicateBuilder::AddAndCategorizePredicate(ColumnOperand const& left,
                                                 ColumnOperand const& right, bool comparable) {
    for (auto op : kAllOperatorTypes) {
        bool should_add_predicate =
                comparable || op == OperatorType::kEqual || op == OperatorType::kUnequal;

        if (should_add_predicate) {
            predicates_.push_back(GetPredicate(Operator(op), left, right));
            CategorizeLastPredicate(comparable);
        }
    }
}

bool PredicateBuilder::IsJoinable(TypedColumnData const& c1, TypedColumnData const& c2) {
    if (!allow_cross_columns_) return c1.GetColumn() == c2.GetColumn();

    if (c1.GetTypeId() != c2.GetTypeId()) return false;

    // FIXME: diff > epsilon?
    return GetSharedPercentage(c1, c2) > minimum_shared_value_;
}

bool PredicateBuilder::IsComparable(TypedColumnData const& c1, TypedColumnData const& c2) {
    if (!allow_cross_columns_) return c1.GetColumn() == c2.GetColumn() && (c1.IsNumeric());

    if (c1.GetTypeId() != c2.GetTypeId()) return false;

    // FIXME: diff > epsilon?
    return c1.IsNumeric() && GetAverageRatio(c1, c2) > comparable_threshold_;
}

}  // namespace model
