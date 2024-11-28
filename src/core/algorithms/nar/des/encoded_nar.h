#pragma once

#include "algorithms/nar/nar.h"
#include "encoded_value_range.h"
#include "model/table/column_layout_typed_relation_data.h"

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
    size_t VectorSize() const;
    size_t FeatureCount() const;
    double& operator[](size_t index);
    double const& operator[](size_t index) const;

    model::NARQualities const& GetQualities() const;
    NAR SetQualities(FeatureDomains& domains, TypedRelation const* typed_relation, RNG& rng);

    NAR Decode(FeatureDomains& domains, RNG& rng) const;
    EncodedNAR(FeatureDomains& domains, TypedRelation const* typed_relation, RNG& rng);
    EncodedNAR(size_t feature_count, RNG& rng);
};

}  // namespace algos::des
