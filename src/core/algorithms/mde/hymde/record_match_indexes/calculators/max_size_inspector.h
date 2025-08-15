#pragma once

#include <algorithm>
#include <cstddef>

namespace algos::hymde::record_match_indexes::calculators {
class SingleMaxSizeInspector {
    std::size_t max_size_ = 0;

public:
    void InspectOld(auto&&...) const noexcept {}

    void InspectNew(auto&& v, auto&&) noexcept {
        std::size_t const size = v.size();
        if (size > max_size_) max_size_ = size;
    }

    std::size_t GetMaxSize() const noexcept {
        return max_size_;
    }
};

class PairMaxSizeInspector {
    std::size_t max_size_left_ = 0;
    std::size_t max_size_right_ = 0;

public:
    void InspectOldLeft(auto&&...) const noexcept {}

    void InspectOldRight(auto&&...) const noexcept {}

    void InspectNewLeft(auto&& v, auto&&) noexcept {
        std::size_t const size = v.size();
        if (size > max_size_left_) max_size_left_ = size;
    }

    void InspectNewRight(auto&& v, auto&&) noexcept {
        std::size_t const size = v.size();
        if (size > max_size_right_) max_size_right_ = size;
    }

    std::size_t GetMaxSize() const noexcept {
        return std::min(max_size_left_, max_size_right_);
    }
};
}  // namespace algos::hymde::record_match_indexes::calculators
