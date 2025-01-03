#include "encoded_nar.h"

#include "algorithms/nar/value_range.h"
#include "model/types/types.h"

namespace algos::des {

using model::NAR;

size_t EncodedNAR::VectorSize() const {
    return encoded_value_ranges_.size() * EncodedValueRange::kFieldCount + 1;
}

size_t EncodedNAR::FeatureCount() const {
    return encoded_value_ranges_.size();
}

// TODO: remove code duplication here
double& EncodedNAR::operator[](size_t index) {
    qualities_consistent_ = false;
    if (index == 0) {
        return implication_sign_pos_;
    }
    index--;
    size_t feature = index / EncodedValueRange::kFieldCount;
    size_t feature_field = index % EncodedValueRange::kFieldCount;
    return encoded_value_ranges_[feature][feature_field];
}

double const& EncodedNAR::operator[](size_t index) const {
    if (index == 0) {
        return implication_sign_pos_;
    }
    index--;
    size_t feature = index / EncodedValueRange::kFieldCount;
    size_t feature_field = index % EncodedValueRange::kFieldCount;
    return encoded_value_ranges_[feature][feature_field];
}

NAR EncodedNAR::SetQualities(FeatureDomains& domains, TypedRelation const* typed_relation,
                             RNG& rng) {
    NAR this_decoded = Decode(domains, rng);
    this_decoded.SetQualities(typed_relation);
    qualities_ = this_decoded.GetQualities();
    qualities_consistent_ = true;
    return this_decoded;
}

model::NARQualities const& EncodedNAR::GetQualities() const {
    if (!qualities_consistent_) {
        throw std::logic_error("Getting uninitialized qualities from NAR.");
    }
    return qualities_;
}

NAR EncodedNAR::Decode(FeatureDomains& domains, RNG& rng) const {
    NAR resulting_nar;
    std::vector<size_t> feature_order(encoded_value_ranges_.size());
    std::iota(std::begin(feature_order), std::end(feature_order), 0);
    auto compare_by_permutation = [this](size_t a, size_t b) {
        return encoded_value_ranges_[a].permutation > encoded_value_ranges_[b].permutation;
    };
    std::ranges::sort(feature_order, compare_by_permutation);

    // Implication sign comes after EncodedValueRange with this index.
    size_t implication_sign_after = implication_sign_pos_ * (encoded_value_ranges_.size() - 1);
    size_t processed_features = 0;
    for (size_t feature_index : feature_order) {
        EncodedValueRange const& encoded_feature = encoded_value_ranges_[feature_index];
        if (encoded_feature.threshold < rng.Next()) {
            ++processed_features;
            continue;
        }
        auto const& domain = domains.at(feature_index);
        auto const& decoded = encoded_feature.Decode(domain);
        if (processed_features > implication_sign_after) {
            resulting_nar.InsertInCons(feature_index, decoded);
        } else {
            resulting_nar.InsertInAnte(feature_index, decoded);
        }
        ++processed_features;
    }
    return resulting_nar;
}

EncodedNAR::EncodedNAR(FeatureDomains& domains, TypedRelation const* typed_relation, RNG& rng)
    : implication_sign_pos_(rng.Next()) {
    size_t feature_count = domains.size();
    encoded_value_ranges_.reserve(feature_count);
    std::generate_n(std::back_inserter(encoded_value_ranges_), feature_count,
                    [&rng]() { return EncodedValueRange(rng); });
    SetQualities(domains, typed_relation, rng);
}

EncodedNAR::EncodedNAR(size_t feature_count, RNG& rng) : implication_sign_pos_(rng.Next()) {
    encoded_value_ranges_.reserve(feature_count);
    std::generate_n(std::back_inserter(encoded_value_ranges_), feature_count,
                    [&rng]() { return EncodedValueRange(rng); });
}

}  // namespace algos::des
