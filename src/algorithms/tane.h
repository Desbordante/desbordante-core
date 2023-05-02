#pragma once

#include <string>

#include "algorithms/options/error/type.h"
#include "algorithms/options/max_lhs/type.h"
#include "algorithms/pli_based_fd_algorithm.h"
#include "model/relation_data.h"
#include "util/position_list_index.h"

namespace algos {

class Tane : public PliBasedFDAlgorithm {
private:
    void RegisterOptions();
    void MakeExecuteOptsAvailable() final;

    void ResetStateFd() final;
    unsigned long long ExecuteInternal() final;

public:
    config::ErrorType max_fd_error_;
    config::ErrorType max_ucc_error_;
    config::MaxLhsType max_lhs_;

    int count_of_fd_ = 0;
    int count_of_ucc_ = 0;
    long apriori_millis_ = 0;

    Tane();

    static double CalculateZeroAryFdError(ColumnData const* rhs,
                                          ColumnLayoutRelationData const* relation_data);
    static double CalculateFdError(util::PositionListIndex const* lhs_pli,
                                   util::PositionListIndex const* joint_pli,
                                   ColumnLayoutRelationData const* relation_data);
    static double CalculateUccError(util::PositionListIndex const* pli,
                                    ColumnLayoutRelationData const* relation_data);

    //static double round(double error) { return ((int)(error * 32768) + 1)/ 32768.0; }

    void RegisterFd(Vertical const& lhs, Column const* rhs,
                    double error, RelationalSchema const* schema);
    // void RegisterFd(Vertical const* lhs, Column const* rhs, double error, RelationalSchema const* schema);
    void RegisterUcc(Vertical const& key, double error, RelationalSchema const* schema);
};

}  // namespace algos
