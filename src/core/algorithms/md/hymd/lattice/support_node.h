#pragma once

#include "core/algorithms/md/hymd/column_classifier_value_id.h"
#include "core/algorithms/md/hymd/lattice/lhs_specialization.h"
#include "core/algorithms/md/hymd/lattice/node_base.h"
#include "core/algorithms/md/hymd/md_lhs.h"

namespace algos::hymd::lattice {
class SupportNode : public NodeBase<SupportNode> {
public:
    using Specialization = LhsSpecialization;

    bool is_unsupported = false;

    static MdLhs const& GetLhs(Specialization::Unspecialized lhs) noexcept {
        return lhs;
    }

    bool ContainsGeneralizationOf(Specialization::Unspecialized) const noexcept {
        return is_unsupported;
    }

    SupportNode* AddOneUnchecked(model::Index next_node_offset, ColumnClassifierValueId ccv_id) {
        return AddOneUncheckedBase(next_node_offset, ccv_id);
    }

    SupportNode(std::size_t children_number) : NodeBase<SupportNode>(children_number) {}
};
}  // namespace algos::hymd::lattice
