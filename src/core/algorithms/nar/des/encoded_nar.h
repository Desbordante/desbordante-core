#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "core/algorithms/nar/des/encoded_value_range.h"
#include "core/algorithms/nar/des/rng.h"
#include "core/algorithms/nar/nar.h"
#include "core/algorithms/nar/value_range.h"
#include "core/model/table/column_layout_typed_relation_data.h"

namespace algos::des {
using model::NAR;

class EncodedNAR {
private:
    using TypedRelation = model::ColumnLayoutTypedRelationData;
    using FeatureDomains = std::vector<std::shared_ptr<model::ValueRange>> const;

    double implication_sign_pos_ = -1;
    std::vector<EncodedValueRange> encoded_value_ranges_{};

    model::NARQualities qualities_;
    bool qualities_consistent_ = false;

public:
    std::size_t VectorSize() const;
    std::size_t FeatureCount() const;
    double& operator[](std::size_t index);
    double const& operator[](std::size_t index) const;

    model::NARQualities const& GetQualities() const;
    NAR SetQualities(FeatureDomains& domains, TypedRelation const* typed_relation, RNG& rng);

    NAR Decode(FeatureDomains& domains, RNG& rng) const;
    EncodedNAR(FeatureDomains& domains, TypedRelation const* typed_relation, RNG& rng);
    EncodedNAR(std::size_t feature_count, RNG& rng);
};

}  // namespace algos::des
