#pragma once

#include <numeric>

#include "dc/FastADC/model/evidence.h"
#include "util/logger.h"

namespace algos::fastadc {

/**
 * EvidenceSet is vector of Evidences with extra methods
 */
class EvidenceSet {
public:
    void Reserve(size_t n) {
        evidences_.reserve(n);
    }

    template <typename... Args>
    void EmplaceBack(Args&&... args) {
        evidences_.emplace_back(std::forward<Args>(args)...);
    }

    size_t Size() const {
        return evidences_.size();
    }

    uint64_t GetTotalCount() const {
        return std::accumulate(
                evidences_.begin(), evidences_.end(), 0UL,
                [](uint64_t total, Evidence const& evidence) { return total + evidence.count; });
    }

    Evidence const& operator[](size_t index) const {
        return evidences_[index];
    }

    Evidence& operator[](size_t index) {
        return evidences_[index];
    }

    // NOLINTBEGIN(readability-identifier-naming)
    auto begin() const {
        return evidences_.begin();
    }

    auto end() const {
        return evidences_.end();
    }

    auto begin() {
        return evidences_.begin();
    }

    auto end() {
        return evidences_.end();
    }

    // NOLINTEND(readability-identifier-naming)

private:
    std::vector<Evidence> evidences_;
};

}  // namespace algos::fastadc
