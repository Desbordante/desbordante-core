#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "algorithms/mde/hymde/record_classifier_value_id.h"
#include "model/index.h"
#include "model/table/relational_schema.h"
#include "model/types/type.h"

namespace algos::hymde {
// A bunch of MDEs in the search space have a ton of common elements, and minimal covers can grow
// exponentially, so size of each dependency matters.
struct RecordClassifierSpecification {
    model::Index record_match_index;
    RecordClassifierValueId rcv_id_;
};

struct MDESpecification {
    std::vector<RecordClassifierSpecification> lhs_;
    RecordClassifierSpecification rhs_;
};

// If a record match is present in both LHS and RHS, LHS bounds may be selected further
// Search spaces are defined by its elements. Each contains a record match and a bound list.
// Refined further with LHS subsets? Proto-search space?
// LHS components and RHS components.
// Theory: all record matches, some are selected as LHS, some are selected as RHS, those that are
// both can have LHS selection
// As far as compressed result storage is concerned, LHS selection is not important.
// Reformulation: we need to encode a minimal cover of MDEs.
// This can be considered as encoding a set of MDEs with common record matches and bound sets.
// However, size of decision bound type can be smaller than an index. Then what? We can determine
// size of decision bound using its type, which is available in record match specification.
// Okay, let's not be fanatical and just use the (index, index) approach for now
struct SearchSpaceFactorSpecification {
    // std::shared_ptr<RecordMatch> record_match;
    std::function<void(RecordClassifierValueId, std::byte*)> decision_bound_map_;
};

class CompactMDEStorage {
    std::shared_ptr<RelationalSchema> left_schema_;
    std::shared_ptr<RelationalSchema> right_schema_;
    // Record matches are not repeated often, even if the comparison function is.
    // The thing that has exponential size is the number of MDEs, compressing record matches is
    // pointless.
    // bound list for each record match
    // size is not needed, no point in vectors
    // Types are stored in record matches.
    // all record matches
    // not search space! Info about record matches and decision bounds
    std::vector<SearchSpaceFactorSpecification> search_space_specification_;
    // MDEs stored as:
    // - LHS: list of (RM index, bound index)
    // - RHS: (RM index, bound index)
    std::vector<MDESpecification> mde_specifications_;

    // Operations: get an MDE, get all MDEs, iterate through MDs
};
}  // namespace algos::hymde
