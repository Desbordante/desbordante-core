#pragma once

#include <cstddef>
#include <memory>

#include "algorithms/nar/value_range.h"
#include "rng.h"

namespace algos {
namespace des {
class RNG;
}  // namespace des
}  // namespace algos

namespace algos::des {

class EncodedValueRange {
public:
    static size_t constexpr kFieldCount = 4;
    double permutation;
    double threshold;
    double bound1;
    double bound2;

    double& operator[](size_t index);
    double const& operator[](size_t index) const;

    template <typename T, typename RangeT>
    std::shared_ptr<RangeT> DecodeTypedValueRange(
            std::shared_ptr<model::ValueRange> const& domain) const;

    std::shared_ptr<model::ValueRange> Decode(
            std::shared_ptr<model::ValueRange> const& domain) const;

    explicit EncodedValueRange(RNG& rng);
};

}  // namespace algos::des
