#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "condition_miners/cind_miner.h"
#include "core/algorithms/algorithm.h"
#include "core/algorithms/cind/types.h"
#include "core/algorithms/ind/spider/spider.h"

namespace algos::cind {
class CindAlgorithm final : public Algorithm {
private:
    struct StageTimings {
        std::uint64_t load{0};
        std::uint64_t compute{0};
        std::uint64_t total{0};
    };

    std::unique_ptr<Spider> spider_algo_;
    std::unique_ptr<CindMiner> cind_miner_;
    AlgoType algo_type_{AlgoType::kPliCind};
    StageTimings timings_{};

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

public:
    CindAlgorithm();

    [[nodiscard]] std::uint64_t TimeTaken() const noexcept {
        return timings_.total;
    }

    [[nodiscard]] auto const& AINDList() const noexcept {
        return spider_algo_->INDList();
    }

    [[nodiscard]] auto const& CINDList() const noexcept {
        return cind_miner_->CINDList();
    }
};
}  // namespace algos::cind
