#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/cfd/cfdfinder/types/inverted_cluster_maps.h"

namespace algos::cfdfinder {
class Entry {
protected:
    inline static constexpr std::string_view kNullRepresentation = "null";
    inline static constexpr std::string_view kWildCard = "_";

public:
    virtual ~Entry() = default;

    virtual inline int GetOrderRank() const = 0;
    virtual size_t Hash() const = 0;
    virtual bool operator==(Entry const& other) const = 0;
    virtual bool operator<(Entry const& other) const = 0;
    virtual bool IsConstantType() const = 0;
    virtual std::string ToString(InvertedClusterMap const& cluster_map) const = 0;
};

}  // namespace algos::cfdfinder
