#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <tuple>
#include <vector>

#include "algorithms/mde/decision_boundaries/decision_boundary.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"
#include "algorithms/mde/mde.h"
#include "algorithms/mde/record_match.h"
#include "model/index.h"
#include "model/table/relational_schema.h"
#include "model/types/type.h"
#include "util/get_preallocated_vector.h"

namespace algos::hymde {
// A bunch of MDEs in the search space have a ton of common elements, and minimal covers can grow
// exponentially, so size of each dependency matters.
struct RecordClassifierSpecification {
    model::Index record_match_index;
    RecordClassifierValueId rcv_id;

    RecordClassifierSpecification(model::Index record_match_index, RecordClassifierValueId rcv_id)
        : record_match_index(record_match_index), rcv_id(rcv_id) {}

    std::tuple<model::Index, RecordClassifierValueId> ToTuple() const noexcept {
        return {record_match_index, rcv_id};
    }
};

struct LhsSpecification {
    std::vector<RecordClassifierSpecification> lhs;
    std::size_t support;

    LhsSpecification(std::vector<RecordClassifierSpecification> lhs, std::size_t support)
        : lhs(std::move(lhs)), support(support) {}
};

struct SameLhsMDEsSpecification {
    LhsSpecification lhs_spec;
    std::vector<RecordClassifierSpecification> rhss;

    SameLhsMDEsSpecification(LhsSpecification lhs_spec,
                             std::vector<RecordClassifierSpecification> rhss)
        : lhs_spec(std::move(lhs_spec)), rhss(std::move(rhss)) {}
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
    model::mde::RecordMatch record_match;
    // TODO: all decision boundaries have the same type.
    std::vector<std::shared_ptr<model::mde::decision_boundaries::DecisionBoundary>>
            decision_boundaries;
};

// Constructed search space superset specification and minimal cover of dependencies of the search
// space(!) that hold
class CompactMDEStorage {
    using FullLhsSpec = std::pair<std::vector<model::mde::RecordClassifier>, std::size_t>;

    std::shared_ptr<RelationalSchema> left_schema_;
    std::shared_ptr<RelationalSchema> right_schema_;
    // Record matches are not repeated often, even if the comparison function is.
    // The thing that has exponential size is the number of MDEs, compressing record matches is
    // pointless.
    // bound list for each record match
    std::vector<SearchSpaceFactorSpecification> search_space_specification_;
    std::vector<SameLhsMDEsSpecification> mde_specifications_;

    model::mde::RecordClassifier ToClassifier(RecordClassifierSpecification const& spec) const {
        SearchSpaceFactorSpecification const& factor_spec =
                search_space_specification_[spec.record_match_index];
        return {factor_spec.record_match, factor_spec.decision_boundaries[spec.rcv_id]};
    }

    std::vector<model::mde::RecordClassifier> ToLhs(
            std::vector<RecordClassifierSpecification> const& lhs_specification) const {
        std::vector<model::mde::RecordClassifier> lhs =
                util::GetPreallocatedVector<model::mde::RecordClassifier>(lhs_specification.size());
        for (RecordClassifierSpecification const& cls_spec : lhs_specification) {
            lhs.push_back(ToClassifier(cls_spec));
        }
        return lhs;
    }

public:
    CompactMDEStorage(std::shared_ptr<RelationalSchema> left_schema,
                      std::shared_ptr<RelationalSchema> right_schema,
                      std::vector<SearchSpaceFactorSpecification> search_space_specification,
                      std::vector<SameLhsMDEsSpecification> mde_specifications)
        : left_schema_(std::move(left_schema)),
          right_schema_(std::move(right_schema)),
          search_space_specification_(std::move(search_space_specification)),
          mde_specifications_(std::move(mde_specifications)) {}

    bool DefinedOnSameTable() const noexcept {
        return left_schema_ == right_schema_;
    }

    std::string GetLeftTableName() const {
        return left_schema_->GetName();
    }

    std::string GetRightTableName() const {
        return right_schema_->GetName();
    }

    std::vector<SearchSpaceFactorSpecification> const& GetSearchSpaceSpecification()
            const noexcept {
        return search_space_specification_;
    }

    std::vector<SameLhsMDEsSpecification> const& GetMdeSpecifications() const noexcept {
        return mde_specifications_;
    }

    std::vector<FullLhsSpec> GetLhsInfo() {
        std::vector<FullLhsSpec> lhs_specs =
                util::GetPreallocatedVector<FullLhsSpec>(mde_specifications_.size());
        for (auto const& [lhs_spec, _] : mde_specifications_) {
            lhs_specs.emplace_back(ToLhs(lhs_spec.lhs), lhs_spec.support);
        }
        return lhs_specs;
    }

    std::vector<model::mde::MDE> GetAll() const {
        std::string left_table = left_schema_->GetName();
        std::string right_table = right_schema_->GetName();
        std::vector<model::mde::MDE> mdes =
                util::GetPreallocatedVector<model::mde::MDE>(mde_specifications_.size());
        for (SameLhsMDEsSpecification const& mdes_spec : mde_specifications_) {
            std::vector<model::mde::RecordClassifier> lhs = ToLhs(mdes_spec.lhs_spec.lhs);
            for (RecordClassifierSpecification const& rhs : mdes_spec.rhss) {
                mdes.emplace_back(left_table, right_table, lhs, ToClassifier(rhs));
            }
        }
        return mdes;
    }

    // Operations: get an MDE, iterate through MDs
};
}  // namespace algos::hymde
