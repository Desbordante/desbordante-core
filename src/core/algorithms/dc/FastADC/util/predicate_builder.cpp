#include "core/algorithms/dc/FastADC/util/predicate_builder.h"

#include <array>
#include <assert.h>
#include <bitset>

#include "core/algorithms/dc/FastADC/misc/typed_column_data_value_differences.h"
#include "core/algorithms/dc/FastADC/model/column_operand.h"
#include "core/algorithms/dc/FastADC/model/operator.h"
#include "core/algorithms/dc/FastADC/model/predicate.h"
#include "core/algorithms/dc/FastADC/providers/index_provider.h"
#include "core/algorithms/dc/FastADC/providers/predicate_provider.h"
#include "core/model/table/typed_column_data.h"
#include "core/model/types/builtin.h"
#include "core/util/logger.h"

namespace algos::fastadc {

PredicateBuilder::PredicateBuilder(PredicateProvider* predicate_provider,
                                   std::shared_ptr<PredicateIndexProvider> predicate_index_provider,
                                   bool allow_cross_columns, double minimum_shared_value,
                                   double comparable_threshold)
    : allow_cross_columns_(allow_cross_columns),
      minimum_shared_value_(minimum_shared_value),
      comparable_threshold_(comparable_threshold),
      predicate_index_provider(predicate_index_provider),
      predicate_provider(predicate_provider) {
    assert(predicate_index_provider);
    assert(predicate_provider);
}

void PredicateBuilder::BuildPredicateSpace(std::vector<model::TypedColumnData> const& input) {
    BuildAndCategorizePredicates(input);

    predicate_index_provider->AddAll(predicates_);
    BuildMutexMap();
    BuildInverseMap();

    LOG_DEBUG(" [Predicate] Predicate space size: {}", predicates_.size());
}

size_t PredicateBuilder::PredIdx(PredicatePtr const& p) {
    return predicate_index_provider->GetIndex(p);
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
        inverse_map_[PredIdx(p1)] = PredIdx(p1->GetInverse(predicate_provider));
    }
}

void PredicateBuilder::BuildAndCategorizePredicates(
        std::vector<model::TypedColumnData> const& input) {
    size_t columns_num = input.size();
    predicates_.reserve((columns_num * (columns_num + 1)) / 2);

    for (size_t i = 0; i < columns_num; ++i) {
        for (size_t j = i; j < columns_num; ++j) {
            ProcessColumnPair(i, j, input);
        }
    }
}

void PredicateBuilder::ProcessColumnPair(size_t i, size_t j,
                                         std::vector<model::TypedColumnData> const& input) {
    bool joinable = IsJoinable(input[i], input[j]);
    bool comparable = IsComparable(input[i], input[j]);

    if (joinable || comparable) {
        ColumnOperand t_col_op(input[i].GetColumn(), ColumnOperandTuple::t);
        ColumnOperand s_col_op(input[j].GetColumn(), ColumnOperandTuple::s);
        AddAndCategorizePredicate(t_col_op, s_col_op, comparable);
    }
}

void PredicateBuilder::CategorizeLastPredicate(bool comparable) {
    auto const& pred = predicates_.back();

    if (comparable) {
        if (pred->IsCrossColumn()) {
            num_cross_column_predicates_.push_back(pred);
        } else {
            num_single_column_predicates_.push_back(pred);
        }
    } else {
        if (pred->IsCrossColumn()) {
            str_cross_column_predicates_.push_back(pred);
        } else {
            str_single_column_predicates_.push_back(pred);
        }
    }
}

void PredicateBuilder::AddAndCategorizePredicate(ColumnOperand const& left,
                                                 ColumnOperand const& right, bool comparable) {
    for (auto op : kAllOperatorTypes) {
        bool should_add_predicate =
                comparable || op == OperatorType::kEqual || op == OperatorType::kUnequal;

        if (should_add_predicate) {
            predicates_.push_back(predicate_provider->GetPredicate(Operator(op), left, right));
            CategorizeLastPredicate(comparable);
        }
    }
}

bool PredicateBuilder::IsJoinable(model::TypedColumnData const& c1,
                                  model::TypedColumnData const& c2) {
    if (!allow_cross_columns_) return c1.GetColumn() == c2.GetColumn();

    if (c1.GetTypeId() != c2.GetTypeId()) return false;

    // FIXME: diff > epsilon?
    return GetSharedPercentage(c1, c2) > minimum_shared_value_;
}

bool PredicateBuilder::IsComparable(model::TypedColumnData const& c1,
                                    model::TypedColumnData const& c2) {
    if (!allow_cross_columns_) return c1.GetColumn() == c2.GetColumn() && (c1.IsNumeric());

    if (c1.GetTypeId() != c2.GetTypeId()) return false;

    // FIXME: diff > epsilon?
    return c1.IsNumeric() && GetAverageRatio(c1, c2) > comparable_threshold_;
}

}  // namespace algos::fastadc
