#include "algorithms/nar/des/encoded_value_range.h"

namespace algos::des {

// TODO: remove code duplication here
double& EncodedValueRange::operator[](size_t index) {
    switch (index) {
        case 0:
            return permutation;
        case 1:
            return threshold;
        case 2:
            return bound1;
        case 3:
            return bound2;
        default:
            throw std::out_of_range("Index out of range for value range.");
    }
}

double const& EncodedValueRange::operator[](size_t index) const {
    switch (index) {
        case 0:
            return permutation;
        case 1:
            return threshold;
        case 2:
            return bound1;
        case 3:
            return bound2;
        default:
            throw std::out_of_range("Index out of range for value range.");
    }
}

template <typename T, typename RangeT>
std::shared_ptr<RangeT> EncodedValueRange::DecodeTypedValueRange(
        std::shared_ptr<model::ValueRange> const& domain) const {
    auto typed_domain = std::static_pointer_cast<RangeT>(domain);
    T span = typed_domain->upper_bound - typed_domain->lower_bound;
    T decoded_lower = typed_domain->lower_bound + span * bound1;
    T decoded_upper = typed_domain->lower_bound + span * bound2;

    auto [resulting_lower, resulting_upper] = std::minmax(decoded_lower, decoded_upper);
    return std::make_shared<RangeT>(resulting_lower, resulting_upper);
}

template <>
std::shared_ptr<model::StringValueRange>
EncodedValueRange::DecodeTypedValueRange<model::String, model::StringValueRange>(
        std::shared_ptr<model::ValueRange> const& domain_ptr) const {
    using namespace model;
    auto string_domain_ptr = std::static_pointer_cast<StringValueRange>(domain_ptr);
    std::vector<String> const& string_domain = string_domain_ptr->domain;
    if (string_domain.empty()) {
        throw std::logic_error("String domain is empty, cannot decode value range.");
    }
    size_t span = string_domain.size();
    // upper_bound is not used, resulting NARs bind categorical values with a single
    // value.
    size_t index = std::clamp(bound1 * span, 0.0, span - 1.0);
    return std::make_shared<StringValueRange>(string_domain[index]);
}

std::shared_ptr<model::ValueRange> EncodedValueRange::Decode(
        std::shared_ptr<model::ValueRange> const& domain) const {
    using namespace model;
    auto domain_type_id = domain->GetTypeId();
    switch (domain_type_id) {
        case TypeId::kString:
            return DecodeTypedValueRange<String, StringValueRange>(domain);
        case TypeId::kDouble:
            return DecodeTypedValueRange<Double, NumericValueRange<Double>>(domain);
        case TypeId::kInt:
            return DecodeTypedValueRange<Int, NumericValueRange<Int>>(domain);
        default:
            throw std::invalid_argument(std::string("ValueRange has invalid type_id: ") +
                                        domain_type_id._to_string() +
                                        std::string(" in function: ") + __func__);
    }
}

EncodedValueRange::EncodedValueRange(RNG& rng)
    : permutation(rng.Next()), threshold(rng.Next()), bound1(rng.Next()), bound2(rng.Next()) {}

}  // namespace algos::des
