#pragma once

#include <optional>

#include "core/algorithms/ind/faida/inclusion_testing/hyperloglog.h"

namespace algos::faida {

class HLLData {
private:
    std::optional<hll::HyperLogLog> hll_;

public:
    HLLData() : hll_(std::nullopt) {}

    std::optional<hll::HyperLogLog>& GetHll() {
        return hll_;
    }

    std::optional<hll::HyperLogLog> const& GetHll() const {
        return hll_;
    }

    void SetHll(hll::HyperLogLog hll) {
        hll_ = std::move(hll);
    }

    bool IsIncludedIn(HLLData const& other) const {
        if (!hll_.has_value()) {
            return true;
        } else if (!other.GetHll().has_value()) {
            return false;
        } else {
            return hll_->is_included_in(other.GetHll().value());
        }
    }
};

}  // namespace algos::faida
