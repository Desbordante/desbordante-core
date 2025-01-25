/** \file
 * \brief Spider algorithm
 *
 * Spider algorithm class definition
 */
#pragma once
#include <vector>

#include "algorithms/ind/ind_algorithm.h"
#include "config/equal_nulls/type.h"
#include "config/error/type.h"
#include "config/mem_limit/type.h"
#include "config/thread_number/type.h"
#include "model/table/column_domain.h"

namespace algos {
namespace cind {
class Cind;
}  // namespace cind

///
/// \brief disk-backed unary inclusion dependency mining algorithm
///
/// \note modification(1): writing to disk is only triggered
///       when the algorithm encounters memory limit
///
/// \note modification(2): algorithm mines AINDs (unary)
///
class Spider final : public INDAlgorithm {
public:
    /// timing information for algorithm stages
    struct StageTimings {
        size_t load;    /**< time taken for the data loading */
        size_t compute; /**< time taken for the inds computing */
        size_t total;   /**< total time taken for all stages */
    };

private:
    /* configuration stage fields */
    config::EqNullsType is_null_equal_null_;
    config::ThreadNumType threads_num_;
    config::MemLimitMBType mem_limit_mb_;
    config::ErrorType max_ind_error_;

    /* execution stage fields */
    std::shared_ptr<std::vector<model::ColumnDomain>> domains_; /*< loaded data */
    StageTimings timings_;                                      /*< timings info */

    void MakeLoadOptsAvailable();
    void LoadINDAlgorithmDataInternal() final;
    void MakeExecuteOptsAvailable() final;

    /* execution stage functions */
    void MineINDs();
    void MineAINDs();

    unsigned long long ExecuteInternal() final;
    void ResetINDAlgorithmState() final;

public:
    explicit Spider();

    /// get information about stage timings
    StageTimings const& GetStageTimings() const noexcept {
        return timings_;
    }

private:
    friend class cind::Cind;
};

}  // namespace algos
