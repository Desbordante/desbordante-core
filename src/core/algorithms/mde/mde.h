#include <format>
#include <functional>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "model/table/relational_schema.h"
#include "model/types/type.h"

namespace model::mde {
using InitializedObjectPtr = std::byte*;
using UninitializedStoragePtr = std::byte*;

using Record = std::vector<std::string>;

// Partitioning function (PF): table value -> value
template <typename T>
using PartitioningFunction = std::function<T(Record const&)>;

class IValuePartitioner {
public:
    virtual void GetPartitioningValue(Record const& record,
                                      UninitializedStoragePtr left_partitioning_value) const = 0;

    virtual std::string ToString(RelationalSchema const& schema) const = 0;
};

// Comparison function: two values -> result
template <typename Res, typename L, typename R>
using ComparisonFunction = std::function<Res(L const&, R const&)>;

class IPartitionValueComparer {
public:
    virtual void ComparePartitionValues(InitializedObjectPtr left_partition_value,
                                        InitializedObjectPtr right_partition_value,
                                        UninitializedStoragePtr result) const = 0;

    virtual std::string ToString() const = 0;
};

// Comparison function result order: lt(result1, result2)
template <typename Res>
using ResultOrder = std::function<bool(Res const&, Res const&)>;

class ITotalOrder {
public:
    // Defines a total order on comparison function results.
    virtual bool AreResultsOrdered(InitializedObjectPtr result1,
                                   InitializedObjectPtr result2) const = 0;
    // Total decision boundary: matches all in the table
    // Universal decision boundary: matches all possible values
    // This method is used to distinguish between the two
    virtual bool IsLeastElement(InitializedObjectPtr result) const = 0;

    virtual std::string ToString() const = 0;
};

// Record match: (PF1, PF2, Comp, ResOrder)
class RecordMatch {
    std::unique_ptr<model::Type> left_value_partitioner_type_;
    std::unique_ptr<model::Type> right_value_partitioner_type_;
    std::unique_ptr<model::Type> comparison_result_type_;

    std::unique_ptr<IValuePartitioner const> left_value_partitioner_;
    std::unique_ptr<IValuePartitioner const> right_value_partitioner_;
    std::unique_ptr<IPartitionValueComparer const> partition_value_comparer_;
    std::unique_ptr<ITotalOrder const> result_order_;

public:
    // needed from type: size, destructor
    // partitioning values need to be hashable and eq-comparable
    // types: partitioning value (left, right), result comparison type

    // invariant: partition value comparer accepts the types returned in L and R value partitioners
    // invariant: order accepts values of comparer's type

    // Tagging known stuff, but allowing anything.
    IValuePartitioner const& GetLeftValuePartitioner() const noexcept {
        return *left_value_partitioner_;
    }

    IValuePartitioner const& GetRightValuePartitioner() const noexcept {
        return right_value_partitioner_ == nullptr ? *left_value_partitioner_
                                                   : *right_value_partitioner_;
    }

    IPartitionValueComparer const& GetPartitionValueComparer() const noexcept {
        return *partition_value_comparer_;
    }

    ITotalOrder const& GetResultOrder() const noexcept {
        return *result_order_;
    }

    // example output: <ComparisonFunction>(<table1>::<PF1>, <table2>::<PF2>) >= <value>
    // example output: <ComparisonFunction>(<PF1>(<table1>), <PF2>(<table2>)) <= <value>
    // example output: <ComparisonFunction>(<PF1>(<table1>), <PF2>(<table2>)) follows <value>
    //                 in order "<order>"
    // example output: <PF1> on table "<title1>" compared with <PF2> on table "<title2>" using
    //                 <ComparisonFunction> follows <value> in order "<order>"
    // example output: distance(USState(trips), USState(trips)) >= 500km
    virtual std::string MakeClassifierString(RelationalSchema const& left_schema,
                                             RelationalSchema const& right_schema,
                                             std::string_view fmt,
                                             InitializedObjectPtr decision_boundary) const = 0;
};

// Decision boundary: result of comparison function Comp(PF1(p), PF2(q))
// Matching classifier: (RecMatch, DecBound)
class MatchingClassifier {
public:
    RecordMatch const& GetRecordMatch() const noexcept;
    InitializedObjectPtr GetDecisionBoundary() const noexcept;

    // Default format here: <value> <order> <ComparisonFunction>(<PF1>(<table1>), <PF2>(<table2>))
    std::string ToString(RelationalSchema const& left_schema,
                         RelationalSchema const& right_schema) const;
};

// LHS: set of classifiers
// RHS: single classifier

// (index, bound)
class MD {
    std::shared_ptr<RelationalSchema> left_schema_;
    std::shared_ptr<RelationalSchema> right_schema_;
    std::vector<std::shared_ptr<MatchingClassifier>> lhs_;
    std::shared_ptr<MatchingClassifier> rhs_;

public:
    ;
};

// Infeasible LHS: no records could possibly be matched
// Implication order for dependencies: a <= b => a implies b independent of data
// Triviality: holds on any dataset
// Cardinality: number of LHS classifiers
// Support: number of pairs matched by LHS (part of info)
// Disjointness: can a classifier with same RecMatch as in RHS be in LHS?
// Bound cutoff: replace everything below with universal bound
// LHS boundary subset: allow RecMatch to have only some DecBounds in LHS (search space cut)
// Record match can be in LHS, RHS, both, with disjointness requirement or without
}  // namespace model::mde
