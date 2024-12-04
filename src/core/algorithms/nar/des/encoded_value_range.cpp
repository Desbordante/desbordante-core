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
    T resulting_lower = typed_domain->lower_bound + span * this->bound1;
    T resulting_upper = typed_domain->lower_bound + span * this->bound2;
    if (resulting_lower > resulting_upper) {
        std::swap(resulting_lower, resulting_upper);
    }
    return std::make_shared<RangeT>(resulting_lower, resulting_upper);
}

template <>
std::shared_ptr<model::StringValueRange>
EncodedValueRange::DecodeTypedValueRange<model::String, model::StringValueRange>(
        std::shared_ptr<model::ValueRange> const& domain) const {
    using namespace model;
    auto string_domain = std::static_pointer_cast<StringValueRange>(domain);
    std::vector<String> string_vector = string_domain->domain;
    size_t span = string_vector.size();
    // upper_bound is not used, resulting NARs bind categorical values with a single
    // value.
    String result;
    if (bound1 == 1.0) {
        result = string_vector.back();
    } else {
        result = string_vector[span * this->bound1];
    }
    return std::make_shared<StringValueRange>(result);
}

std::shared_ptr<model::ValueRange> EncodedValueRange::Decode(
        std::shared_ptr<model::ValueRange> const& domain) const {
    using namespace model;
    auto domain_type_id = domain->GetTypeId();
    switch (domain_type_id) {
        case TypeId::kInt: {
            return DecodeTypedValueRange<Int, IntValueRange>(domain);
        }
        case TypeId::kDouble: {
            return DecodeTypedValueRange<Double, DoubleValueRange>(domain);
        }
        case TypeId::kString: {
            return DecodeTypedValueRange<String, StringValueRange>(domain);
        }
        default:
            throw std::invalid_argument(std::string("ValueRange has invalid type_id: ") +
                                        domain_type_id._to_string() +
                                        std::string(" in function: ") + __func__);
    }
}

EncodedValueRange::EncodedValueRange(RNG& rng)
    : permutation(rng.Next()), threshold(rng.Next()), bound1(rng.Next()), bound2(rng.Next()) {}

}  // namespace algos::des
