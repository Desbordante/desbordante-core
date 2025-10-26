#pragma once
#include <ind/spider/spider.h>
#include <memory>

#include "algorithms/algorithm.h"
#include "cind/condition_type.h"
#include "condition_miners/cind_miner.h"

namespace algos::cind {
class CindAlgorithm final : public Algorithm {
    /// timing information for algorithm stages
    struct StageTimings {
        size_t load;    /**< time taken for the data loading */
        size_t compute; /**< time taken for the inds computing */
        size_t total;   /**< total time taken for all stages */
    };

public:
    explicit CindAlgorithm(std::vector<std::string_view> phase_names = {});

    unsigned long long TimeTaken() const {
        return timings_.total;
    }

    std::list<model::IND> const& AINDList() const noexcept {
        return spider_algo_->INDList();
    }

    std::list<CIND> const& CINDList() const noexcept {
        return cind_miner_->CINDList();
    }

private:
    void LoadDataInternal() final;
    unsigned long long ExecuteInternal() final;
    void ResetState() final;

    void CreateSpiderAlgo();
    void RegisterSpiderOptions();
    void CreateCindMinerAlgo();
    void RegisterCindMinerOptions();
    void MakeExecuteOptsAvailable() final;
    bool SetExternalOption(std::string_view option_name, boost::any const& value) final;
    void AddSpecificNeededOptions(
            std::unordered_set<std::string_view>& previous_options) const final;

private:
    // algos
    std::unique_ptr<Spider> spider_algo_;
    std::unique_ptr<CindMiner> cind_miner_;
    AlgoType algo_type_{AlgoType::pli_cind};

    StageTimings timings_;
};

}  // namespace algos::cind