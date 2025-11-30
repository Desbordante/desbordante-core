#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/od/order/dependency_checker.h"
#include "core/algorithms/od/order/list_lattice.h"
#include "core/algorithms/od/order/order_utility.h"
#include "core/algorithms/od/order/sorted_partitions.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_layout_typed_relation_data.h"

namespace algos::order {

class Order : public Algorithm {
public:
    using SortedPartitions = std::unordered_map<Node, SortedPartition, ListHash>;
    using TypedRelation = model::ColumnLayoutTypedRelationData;

    config::InputTable input_table_;
    std::unique_ptr<TypedRelation> typed_relation_;
    SortedPartitions sorted_partitions_;
    std::vector<AttributeList> single_attributes_;
    CandidateSets previous_candidate_sets_;
    CandidateSets candidate_sets_;
    OrderDependencies valid_;
    OrderDependencies merge_invalidated_;
    std::unique_ptr<ListLattice> lattice_;

    void RegisterOptions();
    void LoadDataInternal() override;
    void ResetState() override;
    void PruneSingleEqClassPartitions();
    void CreateSingleColumnSortedPartitions();
    void CreateSortedPartitionsFromSingletons(AttributeList const& attr_list);
    bool HasValidPrefix(AttributeList const& lhs, AttributeList const& rhs) const;
    ValidityType CheckCandidateValidity(AttributeList const& lhs, AttributeList const& rhs);
    void ComputeDependencies(ListLattice::LatticeLevel const& lattice_level);
    std::vector<AttributeList> Extend(AttributeList const& lhs, AttributeList const& rhs) const;
    bool IsMinimal(AttributeList const& a) const;
    bool ExtendedRhsIsPrunable(AttributeList const& lhs, AttributeList const& extended_rhs) const;
    void UpdateCandidateSets();
    void MergePrune();
    void PrintValidOD();
    unsigned long long ExecuteInternal() final;

public:
    OrderDependencies const& GetValidODs() const {
        return valid_;
    }

    Order();
};

}  // namespace algos::order
