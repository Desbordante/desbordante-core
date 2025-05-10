#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "algorithms/mde/decision_boundaries/decision_boundary.h"
#include "algorithms/mde/hymde/lowest_rc_value_id.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_lr_map.h"
#include "algorithms/mde/hymde/record_match_indexes/upper_set_index.h"
#include "algorithms/mde/hymde/record_match_indexes/value_matrix.h"
#include "model/index.h"
#include "util/get_preallocated_vector.h"

namespace algos::hymde::record_match_indexes {

// Calculator outputs:
// 1) decision boundaries (output only)
// 2) total dec. bound. is universal? (output only)
// 3) RCV ID LR map (heavy use)
// 4) value matrix (heavy use)
// 5) upper set index (heavy use)
// 6) lpli cluster overlaps with upper set index and gives max comp.res.? (sampling variation)
// 7) comp. function is symmetric + plis are identical? (sampling)

// Structure:
// 1) output only data (1, 2 + useful RMs)
// 2) indexes (3, 4)
// 3) LR map (5)
// 4) (reordered) PartitionIndex
// 5) sampling help assertions (6, 7)

struct ClassifierValues {
    using SearchSpaceDecisionBoundaries =
            std::vector<std::shared_ptr<model::mde::decision_boundaries::DecisionBoundary>>;

    // Always has at least one value at the front: the total decision boundary
    SearchSpaceDecisionBoundaries values;
    bool total_decision_boundary_is_universal_;

    bool MaxIsTotal() const noexcept {
        return values.size() == 1;
    }
};

struct Indexes {
    ValueMatrix value_matrix;
    UpperSetIndex upper_set_index;
};

struct SearchSpaceComponentSpecification {
    ClassifierValues classifier_values;
    RcvIdLRMap rcv_id_lr_map;

    static RcvIdLRMap MakeLhsCCVIdsInfo(std::vector<RecordClassifierValueId>&& lhs_ids,
                                        std::size_t const values_size) {
        std::vector<RecordClassifierValueId> cm_map =
                util::GetPreallocatedVector<RecordClassifierValueId>(values_size);
        auto next = lhs_ids.begin(), prev = next++, end = lhs_ids.end();
        cm_map.insert(cm_map.end(), lhs_ids.front(), kLowestRCValueId);
        model::Index cur_i = 0;
        for (; next != end; ++prev, ++next, ++cur_i) {
            cm_map.insert(cm_map.end(), *next - *prev, cur_i);
        }
        cm_map.insert(cm_map.end(), values_size - *prev, cur_i);
        return {std::move(lhs_ids), std::move(cm_map)};
    }

    SearchSpaceComponentSpecification(ClassifierValues classifier_values_tmp,
                                      std::vector<RecordClassifierValueId> lhs_ids)
        : classifier_values(std::move(classifier_values_tmp)),
          rcv_id_lr_map(MakeLhsCCVIdsInfo(std::move(lhs_ids), classifier_values.values.size())) {}
};

struct ComponentStructureAssertions {
    // The following must be true:
    // 1) number of values for both tables is the same (n)
    // 2) if m is the max comparison value index for the column match the following holds:
    //      \forall i \in range(n) value_matrix[i][i] == m
    // 3) \forall i \in range(n) left_pli[i] \subset upper_set_index[i][m]
    // This corresponds to column matches matching columns to themselves and equal values being
    // considered to have similarity of 1.0
    bool assume_overlap_lpli_cluster_max_;

    // TODO: figure this out!
    // In case of 1 table, we are going to compare records (6, 7) and (7, 6), which will give the
    // same results, right? So we can skip records with index greater than... There is some
    // interplay with equality and how records are partitioned here, figure it out.
    bool assume_record_symmetric_;
};

struct ComponentHandlingInfo {
    SearchSpaceComponentSpecification search_space_component;
    Indexes comparison_indexes;
    ComponentStructureAssertions structure_assertions;

    ComponentHandlingInfo(SearchSpaceComponentSpecification search_space_component,
                          Indexes comparison_indexes,
                          ComponentStructureAssertions structure_assertions)
        : search_space_component(std::move(search_space_component)),
          comparison_indexes(std::move(comparison_indexes)),
          structure_assertions(structure_assertions) {}
};
}  // namespace algos::hymde::record_match_indexes
