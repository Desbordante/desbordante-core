#pragma once

#include <map>

#include "core/algorithms/fd/afd_measure.h"
#include "core/algorithms/fd/pli_based_afd_algorithm.h"
#include "core/algorithms/fd/tane/model/lattice_vertex.h"
#include "core/config/error/type.h"
#include "core/model/table/column_data.h"
#include "core/model/table/column_layout_relation_data.h"
#include "core/model/table/position_list_index.h"

namespace algos {

class Tane final : public PliBasedAFDAlgorithm {
protected:
    config::ErrorType max_fd_error_;
    config::ErrorType max_ucc_error_;
    model::AfdMeasure afd_measure_;

private:
    using LatticeLevel = std::map<boost::dynamic_bitset<>, std::unique_ptr<model::LatticeVertex>>;

    void ResetStateFd() final {}

    LatticeLevel GenerateLevel1(model::LatticeVertex const* empty_vertex);
    void Prune(LatticeLevel& level);
    void ComputeDependencies(LatticeLevel& level);
    static LatticeLevel GenerateNextLevel(LatticeLevel& level);
    void ExecuteInternal() final;
    void MakeExecuteOptsAvailableFDInternal() final;
    config::ErrorType CalculateZeroAryFdError(ColumnData const* rhs);
    config::ErrorType CalculateFdError(model::PLIWS const* lhs_pli, model::PLIWS const* rhs_pli,
                                       model::PLIWS const* joint_pli);
    static double CalculateUccError(model::PositionListIndex const* pli,
                                    ColumnLayoutRelationData const* relation_data);

public:
    Tane();
};

}  // namespace algos
