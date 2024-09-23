#pragma once

#include "algorithms/fd/pli_based_fd_algorithm.h"
#include "algorithms/fd/tane/model/lattice_level.h"
#include "config/error/type.h"
#include "model/table/column_data.h"
#include "model/table/column_layout_relation_data.h"
#include "model/table/position_list_index.h"

namespace algos::tane {

class TaneCommon : public PliBasedFDAlgorithm {
protected:
    config::ErrorType max_fd_error_;
    config::ErrorType max_ucc_error_;

private:
    void ResetStateFd() final {}

    void Prune(model::LatticeLevel* level);
    void ComputeDependencies(model::LatticeLevel* level);
    unsigned long long ExecuteInternal() final;
    virtual config::ErrorType CalculateZeroAryFdError(ColumnData const* rhs) = 0;
    virtual config::ErrorType CalculateFdError(model::PositionListIndex const* lhs_pli,
                                               model::PositionListIndex const* joint_pli) = 0;
    static double CalculateUccError(model::PositionListIndex const* pli,
                                    ColumnLayoutRelationData const* relation_data);
    void RegisterAndCountFd(Vertical const& lhs, Column const* rhs);

public:
    TaneCommon(std::optional<ColumnLayoutRelationDataManager> relation_manager = std::nullopt);
};

}  // namespace algos::tane
