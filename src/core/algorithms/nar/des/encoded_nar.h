#pragma once

#include "algorithms/nar/nar.h"
#include "encoded_value_range.h"
#include "model/table/column_layout_typed_relation_data.h"

namespace algos::des {

class EncodedNAR {
private:
    using TypedRelation = model::ColumnLayoutTypedRelationData;
    using FeatureDomains = std::vector<std::shared_ptr<model::ValueRange>> const;
    double implication_sign_pos_ = -1;
    std::vector<EncodedValueRange> encoded_value_ranges_ = std::vector<EncodedValueRange>();

    model::NARQualities qualities_;
    bool qualities_consistent_ = false;
    double& GetElementAtIndex(size_t index) const;
public:
    size_t VectorSize() const;
    size_t FeatureCount() const;
    double& operator[](size_t index);
    double const& operator[](size_t index) const;

    model::NARQualities const& GetQualities() const;
    model::NAR SetQualities(FeatureDomains& domains, TypedRelation const* typed_relation);

    model::NAR Decode(FeatureDomains& domains) const;
    EncodedNAR(FeatureDomains& domains, TypedRelation const* typed_relation);
    EncodedNAR(size_t feature_count);

};

}  // namespace algos::des
