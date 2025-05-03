#pragma once

#include <memory>         // for shared_ptr
#include <optional>       // for optional, nullopt
#include <set>            // for set
#include <unordered_set>  // for unordered_set

#include <boost/dynamic_bitset/dynamic_bitset.hpp>  // for dynamic_bitset
#include <boost/unordered/unordered_map.hpp>        // for unordered_map

#include "algorithms/fd/pli_based_fd_algorithm.h"  // for PliBasedFDAlgorithm

class RelationalSchema;

namespace model {
class PositionListIndex;
}

namespace algos {

class FdMine : public PliBasedFDAlgorithm {
private:
    RelationalSchema const* schema_;

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

    void ComputeNonTrivialClosure(boost::dynamic_bitset<> const& xi);
    void ObtainFDandKey(boost::dynamic_bitset<> const& xi);
    void ObtainEqSet();
    void PruneCandidates();
    void GenerateNextLevelCandidates();
    void Reconstruct();
    void Display();

    void ResetStateFd() final;
    unsigned long long ExecuteInternal() override;

public:
    FdMine(std::optional<ColumnLayoutRelationDataManager> relation_manager = std::nullopt);
};

}  // namespace algos
