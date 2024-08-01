#pragma once

#include "algorithms/md/hymd/column_classifier_value_id.h"
#include "algorithms/md/hymd/lattice/md.h"
#include "algorithms/md/hymd/lattice/md_specialization.h"
#include "algorithms/md/hymd/lattice/multi_md.h"
#include "algorithms/md/hymd/lattice/node_base.h"
#include "algorithms/md/hymd/lattice/rhs.h"
#include "algorithms/md/hymd/utility/exclusion_list.h"
#include "model/index.h"

namespace algos::hymd::lattice {
class MdNode : public NodeBase<MdNode> {
public:
    using Specialization = MdSpecialization;

    Rhs rhs;

    static MdLhs const& GetLhs(auto const& md) noexcept {
        return md.lhs;
    }

    bool ContainsGeneralizationOf(MdElement md_rhs) const noexcept {
        return rhs[md_rhs.index] >= md_rhs.ccv_id;
    }

    bool ContainsGeneralizationOf(Specialization::Unspecialized const& md) const noexcept {
        if (rhs.IsEmpty()) return false;
        return ContainsGeneralizationOf(md.rhs);
    }

    bool ContainsGeneralizationOf(MultiMd& md) const noexcept {
        if (rhs.IsEmpty()) return false;
        return md.rhss.CheckEnabled([this](MdElement const& md_element) {
            return ContainsGeneralizationOf(md_element);
        });
    }

    void SetRhs(utility::ExclusionList<MdElement> rhss) {
        SetRhs(rhs, rhss);
    }

    static void SetRhs(Rhs& node_rhs, utility::ExclusionList<MdElement> rhss) {
        rhss.ForEachEnabled([&node_rhs](MdElement const& rhs) { SetRhs(node_rhs, rhs); });
    }

    void SetRhs(MdElement md_rhs) {
        SetRhs(rhs, md_rhs);
    }

    static void SetRhs(Rhs& node_rhs, MdElement rhs) {
        node_rhs.Set(rhs.index, rhs.ccv_id);
    }

    MdNode* AddOneUnchecked(model::Index child_array_index, ColumnClassifierValueId ccv_id) {
        return AddOneUncheckedBase(child_array_index, ccv_id, rhs.size);
    }

    MdNode(std::size_t attributes_num, std::size_t children_number)
        : NodeBase<MdNode>(children_number), rhs(attributes_num) {}

    explicit MdNode(Rhs rhs) : NodeBase<MdNode>(rhs.size), rhs(std::move(rhs)) {}
};
}  // namespace algos::hymd::lattice
