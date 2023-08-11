#pragma once

#include <filesystem>
#include <set>

#include <boost/unordered_map.hpp>

#include "algorithms/fd/pli_based_fd_algorithm.h"
#include "model/table/column_combination.h"
#include "model/table/column_layout_relation_data.h"
#include "model/table/position_list_index.h"
#include "model/table/vertical.h"

namespace algos {

class Fd_mine : public PliBasedFDAlgorithm {
private:
    const RelationalSchema* schema_;

    std::set<dynamic_bitset<>> candidate_set_;
    boost::unordered_map<dynamic_bitset<>, std::unordered_set<dynamic_bitset<>>> eq_set_;
    boost::unordered_map<dynamic_bitset<>, dynamic_bitset<>> fd_set_;
    boost::unordered_map<dynamic_bitset<>, dynamic_bitset<>> final_fd_set_;
    std::set<dynamic_bitset<>> key_set_;
    boost::unordered_map<dynamic_bitset<>, dynamic_bitset<>> closure_;
    boost::unordered_map<dynamic_bitset<>, std::shared_ptr<model::PositionListIndex const>> plis_;
    dynamic_bitset<> relation_indices_;

    void ComputeNonTrivialClosure(dynamic_bitset<> const& xi);
    void ObtainFDandKey(dynamic_bitset<> const& xi);
    void ObtainEqSet();
    void PruneCandidates();
    void GenerateNextLevelCandidates();
    void Reconstruct();
    void Display();

    void ResetStateFd() final;
    unsigned long long ExecuteInternal() override;

public:
    Fd_mine();
};

}  // namespace algos
