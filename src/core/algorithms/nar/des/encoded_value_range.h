#pragma once

#include "algorithms/nar/value_range.h"
#include "model/table/column_layout_typed_relation_data.h"
#include "rng.h"

namespace algos::des {

class EncodedValueRange {
public:
    static size_t constexpr kFieldCount = 4;
    double permutation;
    double threshold;
    double bound1;
    double bound2;

    // TODO: remove code duplication here
    double& operator[](size_t index) {
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

    double const& operator[](size_t index) const {
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

    std::shared_ptr<model::ValueRange> Decode(std::shared_ptr<model::ValueRange> domain) const {
        using namespace model;
        switch (domain->GetTypeId()) {
            case TypeId::kInt: {
                std::shared_ptr<IntValueRange> int_domain =
                        std::static_pointer_cast<IntValueRange>(domain);
                Int span = int_domain->upper_bound - int_domain->lower_bound;
                Int resulting_lower = int_domain->lower_bound + span * this->bound1;
                Int resulting_upper = int_domain->lower_bound + span * this->bound2;
                if (resulting_lower > resulting_upper) {
                    std::swap(resulting_lower, resulting_upper);
                }
                return std::make_shared<IntValueRange>(
                        IntValueRange(resulting_lower, resulting_upper));
            }
            case TypeId::kDouble: {
                std::shared_ptr<DoubleValueRange> double_domain =
                        std::static_pointer_cast<DoubleValueRange>(domain);
                Double span = double_domain->upper_bound - double_domain->lower_bound;
                Double resulting_lower = double_domain->lower_bound + span * this->bound1;
                Double resulting_upper = double_domain->lower_bound + span * this->bound2;
                if (resulting_lower > resulting_upper) {
                    std::swap(resulting_lower, resulting_upper);
                }
                return std::make_shared<DoubleValueRange>(
                        DoubleValueRange(resulting_lower, resulting_upper));
            }
            case TypeId::kString:
            case TypeId::kMixed: {
                std::shared_ptr<StringValueRange> string_domain =
                        std::static_pointer_cast<StringValueRange>(domain);
                std::vector<String> string_vector = string_domain->domain;
                size_t span = string_vector.size();
                // upper_bound is not used, resulting NARs bind categorical values with a single
                // value.
                String result;
                if (bound1 == 1.0) {
                    result = string_vector.back();
                } else {
                    result = string_vector[(size_t)(span * this->bound1)];
                }
                return std::make_shared<StringValueRange>(StringValueRange(result));
            }
            default:
                throw std::invalid_argument(
                        std::string("ValueRange has invalid type_id in function: ") + __func__);
        }
    }

    EncodedValueRange(RNG& rng) {
        permutation = rng.Next();
        threshold = rng.Next();
        bound1 = rng.Next();
        bound2 = rng.Next();
    }
};

}  // namespace algos::des
