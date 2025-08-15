#pragma once

#include "algorithms/mde/hymde/cover_calculation/lattice/lhs_specialization.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lhs.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/node_base.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"

namespace algos::hymde::cover_calculation::lattice {
class SupportNode : public NodeBase<SupportNode> {
    bool is_unsupported_ = false;
public:
    using Specialization = LhsSpecialization;

    void MarkUnsupported() noexcept {
        is_unsupported_ = true;
    }

    static PathToNode const& GetLhs(Specialization::Unspecialized lhs) noexcept {
        return lhs;
    }

    bool ContainsGeneralizationOf(Specialization::Unspecialized) const noexcept {
        return is_unsupported_;
    }

    SupportNode* AddOneUnchecked(model::Index next_node_offset, RecordClassifierValueId rcv_id) {
        return AddOneUncheckedBase(next_node_offset, rcv_id);
    }

    SupportNode(std::size_t children_number) : NodeBase<SupportNode>(children_number) {}
};
}  // namespace algos::hymde::cover_calculation::lattice
