#pragma once

#include <set>

#include <boost/dynamic_bitset.hpp>
#include <boost/unordered_map.hpp>

#include "core/algorithms/fd/table_mask_pair_fd_view.h"
#include "core/algorithms/partition_only_algorithm.h"
#include "core/config/max_lhs/type.h"

namespace algos::fd {

class FdMine : public PartitionOnlyAlgorithm {
private:
    config::MaxLhsType max_lhs_;

    std::set<boost::dynamic_bitset<>> candidate_set_;
    boost::unordered_map<boost::dynamic_bitset<>, std::unordered_set<boost::dynamic_bitset<>>>
            eq_set_;
    boost::unordered_map<boost::dynamic_bitset<>, boost::dynamic_bitset<>> fd_set_;
    boost::unordered_map<boost::dynamic_bitset<>, boost::dynamic_bitset<>> final_fd_set_;
    std::set<boost::dynamic_bitset<>> key_set_;
    boost::unordered_map<boost::dynamic_bitset<>, boost::dynamic_bitset<>> closure_;
    boost::unordered_map<boost::dynamic_bitset<>, std::shared_ptr<model::PositionListIndex const>>
            plis_;
    boost::dynamic_bitset<> relation_indices_;

    TableMaskPairFdView::OwningPointer fd_view_;

    void ComputeNonTrivialClosure(boost::dynamic_bitset<> const& xi);
    void ObtainFDandKey(boost::dynamic_bitset<> const& xi);
    void ObtainEqSet();
    void PruneCandidates();
    void GenerateNextLevelCandidates();
    void Reconstruct();
    void Display();

    void ResetState() final;

    void MakeExecuteOptsAvailable() final;
    unsigned long long ExecuteInternal() final;

public:
    FdMine();

    TableMaskPairFdView::OwningPointer GetFds() {
        return fd_view_;
    }
};

}  // namespace algos::fd
