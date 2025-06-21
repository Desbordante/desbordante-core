#pragma once

#include "algorithms/mde/hymde/cover_calculation/lattice/mde.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/mde_lhs.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/mde_specialization.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/multi_mde.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/node_base.h"
#include "algorithms/mde/hymde/cover_calculation/lattice/rhs.h"
#include "algorithms/mde/hymde/cover_calculation/mde_element.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"

namespace algos::hymde::cover_calculation::lattice {
class MdeNode : public NodeBase<MdeNode> {
public:
    using Specialization = MdeSpecialization;

    Rhs rhs;

    static MdeLhs const& GetLhs(auto const& mde) noexcept {
        return mde.lhs;
    }

    bool ContainsGeneralizationOf(MdeElement mde_rhs) const noexcept {
        return rhs[mde_rhs.index] >= mde_rhs.rcv_id;
    }

    bool ContainsGeneralizationOf(Specialization::Unspecialized const& mde) const noexcept {
        if (rhs.IsEmpty()) return false;
        return ContainsGeneralizationOf(mde.rhs);
    }

    bool ContainsGeneralizationOf(MultiMde& mde) const noexcept {
        if (rhs.IsEmpty()) return false;
        return mde.rhss.CheckEnabled([this](MdeElement const& mde_element) {
            return ContainsGeneralizationOf(mde_element);
        });
    }

    void SetRhs(utility::ExclusionList<MdeElement> rhss) {
        SetRhs(rhs, rhss);
    }

    static void SetRhs(Rhs& node_rhs, utility::ExclusionList<MdeElement> rhss) {
        rhss.ForEachEnabled([&node_rhs](MdeElement const& rhs) { SetRhs(node_rhs, rhs); });
    }

    void SetRhs(MdeElement mde_rhs) {
        SetRhs(rhs, mde_rhs);
    }

    static void SetRhs(Rhs& node_rhs, MdeElement rhs) {
        node_rhs.Set(rhs.index, rhs.rcv_id);
    }

    MdeNode* AddOneUnchecked(model::Index offset, RecordClassifierValueId rcv_id,
                             std::size_t record_matches_number) {
        return AddOneUncheckedBase(offset, rcv_id, record_matches_number);
    }

    MdeNode(std::size_t record_matches_number, std::size_t children_number)
        : NodeBase<MdeNode>(children_number), rhs(record_matches_number) {}

    explicit MdeNode(std::size_t record_matches_number, Rhs rhs)
        : NodeBase<MdeNode>(record_matches_number), rhs(std::move(rhs)) {}
};
}  // namespace algos::hymde::cover_calculation::lattice
