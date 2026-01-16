#pragma once

#include <cstddef>
#include <string>

#include "core/algorithms/cfd/cfdfinder/model/pattern/entry.h"

namespace algos::cfdfinder {

class VariableEntry final : public Entry {
public:
    inline bool Matches([[maybe_unused]] size_t value) const override final {
        return true;
    }

    bool operator==(Entry const& other) const override final {
        return dynamic_cast<VariableEntry const*>(&other) != nullptr;
    }

    bool operator!=(Entry const& other) const {
        return !(*this == other);
    }

    size_t Hash() const override {
        return 0x9e3779b9;
    }

    bool IsConstant() const override {
        return false;
    }

    std::string ToString([[maybe_unused]] InvertedClusterMap const& cluster_map) const override {
        return std::string(kWildCard);
    }
};
}  // namespace algos::cfdfinder
