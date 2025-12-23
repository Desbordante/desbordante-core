#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <ind/spider/spider.h>

#include "algorithms/algorithm.h"
#include "cind/types.h"
#include "condition_miners/cind_miner.h"

namespace algos::cind {
class CindAlgorithm final : public Algorithm {
public:
    explicit CindAlgorithm(std::vector<std::string_view> phase_names = {});

    [[nodiscard]] std::uint64_t TimeTaken() const noexcept {
        return timings_.total;
    }

    [[nodiscard]] auto const& AINDList() const noexcept {
        return spider_algo_->INDList();
    }

    [[nodiscard]] auto const& CINDList() const noexcept {
        return cind_miner_->CINDList();
    }

private:
    /// timing information for algorithm stages
    struct StageTimings {
        std::uint64_t load{0};    /**< time taken for the data loading */
        std::uint64_t compute{0}; /**< time taken for the inds computing */
        std::uint64_t total{0};   /**< total time taken for all stages */
    };

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
    std::unique_ptr<Spider> spider_algo_;
    std::unique_ptr<CindMiner> cind_miner_;
    AlgoType algo_type_{AlgoType::pli_cind};

    StageTimings timings_{};
};
}  // namespace algos::cind
