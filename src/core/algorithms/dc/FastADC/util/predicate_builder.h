#pragma once

#include <memory>
#include <stddef.h>
#include <utility>
#include <vector>

#include "core/algorithms/dc/FastADC/model/predicate.h"
#include "core/algorithms/dc/FastADC/providers/index_provider.h"

namespace algos::fastadc {

/**
 * @brief Constructs the predicate space for a given table and organizes predicates into categories.
 *
 * Generates set of predicates based on the provided table's columns. It evaluates each pair of
 * columns to determine if meaningful predicates can be formed. It also categorizes them into four
 * groups: numeric predicates on single columns, numeric predicates across columns, string
 * predicates on single columns, and string predicates across columns. These groups will later be
 * used for Evidence set building. Additionally, the builder constructs inverse maps and mutex maps
 * to efficiently navigate between predicates and their relationships.
 */
class PredicateBuilder {
private:
    bool IsJoinable(model::TypedColumnData const& c1, model::TypedColumnData const& c2);
    bool IsComparable(model::TypedColumnData const& c1, model::TypedColumnData const& c2);
    void AddAndCategorizePredicate(ColumnOperand const& c1, ColumnOperand const& c2,
                                   bool comparable);
    void ProcessColumnPair(size_t i, size_t j, std::vector<model::TypedColumnData> const& input);
    void CategorizeLastPredicate(bool comparable);

    /**
     * Fills internal predicate vectors with predicates.
     *
     * Goes through every pair of columns in the table, checks
     * whether we can form menaningful predicates with the current pair
     * by measuring how data in these columns are similar to each other.
     */
    void BuildAndCategorizePredicates(std::vector<model::TypedColumnData> const& input);
    void BuildMutexMap();
    void BuildInverseMap();
    size_t PredIdx(PredicatePtr const& p);

    bool allow_cross_columns_;
    double minimum_shared_value_;
    double comparable_threshold_;

    PredicatesVector predicates_;
    PredicatesVector num_single_column_predicates_;
    PredicatesVector num_cross_column_predicates_;
    PredicatesVector str_single_column_predicates_;
    PredicatesVector str_cross_column_predicates_;

    // i (predicate num from PredicateIndexProvider) -> predicates with the same column pair as i
    std::vector<PredicateBitset> mutex_map_;
    // i -> index of predicate having inverse operator to predicate i
    std::vector<size_t> inverse_map_;

public:
    std::shared_ptr<PredicateIndexProvider> predicate_index_provider;
    PredicateProvider* predicate_provider;

    PredicateBuilder(PredicateBuilder const& other) = delete;
    PredicateBuilder& operator=(PredicateBuilder const& other) = delete;
    PredicateBuilder(PredicateBuilder&& other) noexcept = default;
    PredicateBuilder& operator=(PredicateBuilder&& other) noexcept = default;

    /**
     * Constructs a PredicateBuilder with specified configuration parameters.
     *
     * @param allow_cross_columns Determines whether predicates can be formed between different
     * columns.
     *
     * @param minimum_shared_value The minimum percentage of shared values required for two columns
     * to be considered joinable. This parameter is used to assess the similarity between columns
     * based on the overlap in their values. A higher value indicates a stricter criterion for
     * joinability, requiring a greater overlap in the values present in the two columns.
     * It's expressed as a fraction between 0 and 1.
     *
     * @param comparable_threshold The threshold used to determine whether two columns are
     * comparable based on their data characteristic. This value will be compared with the ratio of
     * the columns average values. Columns with a ratio exceeding this threshold are considered
     * sufficiently similar to be comparable, allowing for predicates that involve comparison
     * operators (<, <=, >, >=) to be generated between them.
     * It's expressed as a fraction between 0 and 1.
     */
    PredicateBuilder(PredicateProvider* predicate_provider,
                     std::shared_ptr<PredicateIndexProvider> predicate_index_provider,
                     bool allow_cross_columns, double minimum_shared_value = 0.3,
                     double comparable_threshold = 0.1);

    // TODO: can we pass just a vector of TypedColumnData, or there should be another table
    // representation?
    void BuildPredicateSpace(std::vector<model::TypedColumnData> const& input);

    PredicatesVector const& GetNumSingleColumnPredicates() const noexcept {
        return num_single_column_predicates_;
    }

    PredicatesVector const& GetNumCrossColumnPredicates() const noexcept {
        return num_cross_column_predicates_;
    }

    PredicatesVector const& GetStrSingleColumnPredicates() const noexcept {
        return str_single_column_predicates_;
    }

    PredicatesVector const& GetStrCrossColumnPredicates() const noexcept {
        return str_cross_column_predicates_;
    }

    PredicatesVector const& GetPredicates() const noexcept {
        return predicates_;
    }

    size_t PredicateCount() const noexcept {
        return predicates_.size();
    }

    std::vector<PredicateBitset>&& TakeMutexMap() noexcept {
        return std::move(mutex_map_);
    }

    std::vector<size_t>&& TakeInverseMap() noexcept {
        return std::move(inverse_map_);
    }
};

}  // namespace algos::fastadc
