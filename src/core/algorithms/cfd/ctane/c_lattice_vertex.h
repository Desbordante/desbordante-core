#pragma once

#include <optional>
#include <vector>

#include "core/algorithms/cfd/ctane/tuple_pattern.h"
#include "core/algorithms/cfd/model/cfd_relation_data.h"
#include "core/algorithms/cfd/model/cfd_types.h"
#include "core/algorithms/cfd/model/partition_tidlist.h"

class CLatticeVertex {
private:
    algos::cfd::CFDRelationData const* relation_ = nullptr;
    algos::cfd::TuplePattern tuple_pattern_;
    std::optional<algos::cfd::PartitionTIdList> partition_;
    algos::cfd::Itemset rhs_candidates_;
    std::vector<CLatticeVertex const*> parents_;

public:
    explicit CLatticeVertex(algos::cfd::CFDRelationData const* relation,
                            algos::cfd::TuplePattern tuple_pattern);

    auto const& GetColumnIndices() const {
        return GetTuplePattern().GetColumnIndices();
    }

    algos::cfd::CFDRelationData const* GetRelation() const;
    algos::cfd::TuplePattern const& GetTuplePattern() const;
    auto const& GetPatternValues() const;
    algos::cfd::PartitionTIdList const* GetPositionListIndex() const;
    std::vector<CLatticeVertex const*>& GetParents();
    std::vector<CLatticeVertex const*> const& GetParents() const;
    algos::cfd::Itemset& GetRhsCandidates();
    algos::cfd::Itemset const& GetRhsCandidates() const;
    void SetRhsCandidates(algos::cfd::Itemset candidates);
    void SetPositionListIndex(algos::cfd::PartitionTIdList partition);

    bool operator>(CLatticeVertex const& rhs) const;
    bool operator<(CLatticeVertex const& rhs) const;

    static bool Comparator(CLatticeVertex* v_1, CLatticeVertex* v_2);

    bool ComesBeforeAndSharePrefixWith(CLatticeVertex const& rhs) const;

    static bool RhsCandidateComparator(algos::cfd::Item lhs, algos::cfd::Item rhs);
    static algos::cfd::Itemset IntersectRhsCandidates(algos::cfd::Itemset const& lhs,
                                                      algos::cfd::Itemset const& rhs);
};
