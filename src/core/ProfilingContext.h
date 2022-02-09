#pragma once

#include <iostream>
#include <random>
#include <string>

#include "AgreeSetSample.h"
#include "CacheEvictionMethod.h"
#include "CachingMethod.h"
#include "Configuration.h"
#include "custom/CustomRandom.h"
#include "PartialFD.h"
#include "PartialKey.h"
#include "DependencyConsumer.h"

namespace util {

//forward declaration
class PLICache;

template<class Value>
class VerticalMap;

}

//Dependency Consumer?
class ProfilingContext : public DependencyConsumer {
private:
    Configuration configuration_;
    std::unique_ptr<util::PLICache> pli_cache_;
    std::unique_ptr<util::VerticalMap<util::AgreeSetSample>> agree_set_samples_;     //unique_ptr?
    ColumnLayoutRelationData* relation_data_;
    std::mt19937 random_;
    CustomRandom custom_random_;

    util::AgreeSetSample const* CreateColumnFocusedSample(
        Vertical const& focus, util::PositionListIndex const* restriction_pli, double boost_factor);

public:
    enum class ObjectToCache {
        kPli,
        kAs
    };

    ProfilingContext(Configuration configuration, ColumnLayoutRelationData* relation_data,
                     std::function<void(PartialKey const&)> const& ucc_consumer,
                     std::function<void(PartialFD const&)> const& fd_consumer,
                     CachingMethod const& caching_method,
                     CacheEvictionMethod const& eviction_method, double caching_method_value);

    // Non-const as RandomGenerator state gets changed
    util::AgreeSetSample const* CreateFocusedSample(Vertical const& focus, double boost_factor);
    std::shared_ptr<util::AgreeSetSample const> GetAgreeSetSample(Vertical const& focus) const;
    util::PLICache* GetPliCache() { return pli_cache_.get(); }
    bool IsAgreeSetSamplesEmpty() const { return agree_set_samples_ == nullptr; }
    RelationalSchema const* GetSchema() const { return relation_data_->GetSchema(); }

    Configuration const& GetConfiguration() const { return configuration_; }
    ColumnLayoutRelationData const* GetColumnLayoutRelationData() const { return relation_data_; }
    util::PLICache const* GetPliCache() const { return pli_cache_.get(); }

    // get int in range [0, upper_bound) from the uniform distribution
    // int NextInt(int upper_bound) { return std::uniform_int_distribution<int>{0, upper_bound}(random_); }
    int NextInt(int upper_bound) { return custom_random_.NextInt(upper_bound); }
    double NextDouble() { return custom_random_.NextDouble(); }

    ~ProfilingContext();

    static double GetMaximumEntropy(ColumnLayoutRelationData const* cd1);
    static double GetMinEntropy(ColumnLayoutRelationData const* cd1);
    static double GetMedianEntropy(ColumnLayoutRelationData const* relation_data);
    static double GetMedianInvertedEntropy(ColumnLayoutRelationData const* relation_data);
    static double GetMeanEntropy(ColumnLayoutRelationData const* relation_data);
    static double GetMedianGini(ColumnLayoutRelationData const* relation_data);
private:
    static double GetMedianValue(std::vector<double>&& values, std::string const& measure_name);
    static double SetMaximumEntropy(ColumnLayoutRelationData const* relation_data,
                                    CachingMethod const& caching_method);
};
