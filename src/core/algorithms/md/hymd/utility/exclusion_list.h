#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "model/index.h"
#include "util/bitset_utils.h"

namespace algos::hymd::utility {
template <typename T>
class ExclusionList {
    std::vector<T> const& elements_;
    boost::dynamic_bitset<>& enabled_;

    ExclusionList(std::vector<T> const& elements, boost::dynamic_bitset<>& enabled)
        : elements_(elements), enabled_(enabled) {}

public:
    static ExclusionList Create(std::vector<T> const& elements, boost::dynamic_bitset<>& enabled) {
        enabled.resize(elements.size(), true);
        return {elements, enabled};
    }

    void Reset() {
        enabled_.set(0, Size(), true);
    }

    template <typename Function>
    bool CheckEnabled(Function func) {
        bool nothing_left = true;
        util::ForEachIndex(enabled_, [this, &nothing_left, &func](model::Index index) {
            bool disable = func(elements_[index]);
            if (disable) {
                enabled_.reset(index);
            } else {
                nothing_left = false;
            }
        });
        return nothing_left;
    }

    template <typename Function>
    void ForEachEnabled(Function func) const {
        util::ForEachIndex(enabled_, [this, &func](model::Index index) { func(elements_[index]); });
    }

    std::size_t Size() const noexcept {
        return enabled_.size();
    }

    std::vector<T> const& GetElements() const noexcept {
        return elements_;
    }

    boost::dynamic_bitset<>& GetEnabled() noexcept {
        return enabled_;
    }
};
}  // namespace algos::hymd::utility
