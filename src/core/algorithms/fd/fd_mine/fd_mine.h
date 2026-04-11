#pragma once

#include <set>

#include <boost/dynamic_bitset.hpp>
#include <boost/unordered_map.hpp>

#include "core/algorithms/fd/multi_attr_rhs_fd_storage.h"
#include "core/algorithms/partition_only_algorithm.h"
#include "core/config/max_lhs/type.h"

namespace algos {

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

    MultiAttrRhsFdStorage::OwningPointer fd_storage_;

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

    MultiAttrRhsFdStorage::OwningPointer GetFdStorage() {
        return fd_storage_;
    }
};

}  // namespace algos
