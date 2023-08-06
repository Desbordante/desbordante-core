# pragma once

#include <map>
#include <set>
#include <unordered_set>
#include <vector>

#include "../algorithm.h"
#include "attribute_pair.h"
#include "attribute_set.h"
#include "canonical_od.h"
#include "data/DataFrame.h"
#include "timers/Timer.h"

namespace algos::fastod {

class Fastod/*: public Algorithm*/ {
private:
    const long time_limit_;
    bool is_complete_ = true;
    int level_;
    double error_rate_threshold_ = -1;
    int od_count_ = 0, fd_count_ = 0, ocd_count_ = 0;

    std::vector<CanonicalOD> result_;
    std::vector<std::set<AttributeSet>> context_in_each_level_;
    std::map<AttributeSet, AttributeSet> cc_;
    std::map<AttributeSet, std::unordered_set<AttributePair>> cs_;

    AttributeSet schema_;
    const DataFrame& data_;

    Timer timer_;

    // void PrintState() const noexcept;

    bool IsTimeUp() const noexcept;
    
    void CCPut(const AttributeSet& key, const AttributeSet& attribute_set) noexcept;
    void CCPut(const AttributeSet& key, int attribute) noexcept;
    const AttributeSet& CCGet(const AttributeSet& key) noexcept;
    void CSPut(const AttributeSet& key, const AttributePair& value) noexcept;
    std::unordered_set<AttributePair>& CSGet(const AttributeSet& key) noexcept;

    void Initialize() noexcept;

    void ComputeODs() noexcept;
    void PruneLevels() noexcept;
    void CalculateNextLevel() noexcept;

public:
    Fastod(const DataFrame& data, long time_limit, double error_rate_threshold) noexcept;
    Fastod(const DataFrame& data, long time_limit) noexcept;

    void PrintStatistics() const noexcept;
    bool IsComplete() const noexcept;
    std::vector<CanonicalOD> Discover() noexcept;
};

} // namespace algos::fatod
