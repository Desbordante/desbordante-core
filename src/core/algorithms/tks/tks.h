#pragma once

#include <filesystem>
#include <memory>
#include <queue>

#include "core/algorithms/algorithm.h"
#include "PatternTKS.hpp"
#include "TKSEngine.hpp"

namespace algos::tks {

/* Sequential Pattern Mining algorithm using TKS approach.
 * Discovers top-K frequent sequential patterns in sequence databases in SPMF format.
 * 
 * SPMF format:
 * - Items are positive integers separated by spaces
 * - "-1" separates itemsets within a sequence
 * - "-2" marks end of sequence
 * - Lines starting with #, %, @ are comments
 */
class TKS : public Algorithm {
private:
    std::filesystem::path input_file_path_;
    
    int k_ = 10;
    int min_pattern_length_ = 1;
    int max_pattern_length_ = 1000;
    int max_gap_ = INT_MAX;
    bool show_sequence_identifiers_ = false;
    
    std::unique_ptr<TKSEngine> engine_;
    std::priority_queue<PatternTKS> patterns_;
    
    void RegisterOptions();
    void ResetState() final;
    
    void LoadDataInternal() final;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() final;

public:
    TKS();
    
    std::priority_queue<PatternTKS> const& GetPatterns() const noexcept {
        return patterns_;
    }
    
    size_t GetPatternCount() const noexcept {
        return patterns_.size();
    }
};

} 
