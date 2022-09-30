#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

#include "AC.h"
#include "ColumnLayoutTypedRelationData.h"
#include "Primitive.h"
#include "Types.h"

namespace algos {

class ACAlgorithm : public algos::Primitive {
public:
    struct RangesCollection {
        RangesCollection(std::unique_ptr<model::Type> num_type, std::vector<std::byte*>&& ranges,
                         size_t lhs_i, size_t rhs_i)
            : num_type(std::move(num_type)),
              ranges(std::move(ranges)),
              column_indices(lhs_i, rhs_i) {}

        std::unique_ptr<model::Type> num_type;
        std::vector<std::byte*> ranges;
        std::pair<size_t, size_t> column_indices;
    };

private:
    using TypedRelation = model::ColumnLayoutTypedRelationData;
    using Constraints = std::vector<std::unique_ptr<model::AC>>;

    std::vector<std::byte*> disjunctive_ranges_;
    std::vector<Constraints> alg_constraints_;
    std::vector<RangesCollection> ranges_;
    std::unique_ptr<TypedRelation> typed_relation_;
    char bin_operation_;
    model::INumericType* num_type_;
    double fuzziness_;
    double weight_;
    size_t bumps_limit_;
    double p_fuzz_;
    size_t iterations_limit;
    std::string pairing_rule_;
    bool test_mode_;

    std::vector<std::byte*> Sampling(std::vector<model::TypedColumnData> const& data_, size_t lhs_i,
                                     size_t rhs_i, size_t k_bumps_, size_t i);
    std::vector<model::AC*> FindAllConstraints(std::vector<model::TypedColumnData> const& data_,
                                               size_t lhs_i, size_t rhs_i);
    void ConstructDisjunctiveRanges(Constraints&, std::vector<std::byte*>&);
    void RestrictRangesAmount(std::vector<std::byte*>&);

public:
    struct Config {
        std::filesystem::path data{}; /* Path to input file */
        char separator = ',';         /* Separator for csv */
        bool has_header = true;       /* Indicates if input file has header */
        char bin_operation = '+';
        double fuzziness = 0.15;      /* Fraction of exceptional records */
        double p_fuzz = 0.9;          /* Probability of not exceeding fuzziness ratio
                                          by ratio of exceptional records */
        double weight = 0.05;         /* between 0 and 1. Closer to 0 - many short intervals.
                                         To 1 - small number of long intervals */
        size_t bumps_limit = 5;       /* to remove limit: pass value of 0 */
        size_t iterations_limit = 10; /* limit for iterations in Sampling() */
        std::string pairing_rule = "trivial";
        bool test_mode = false;
    };

    explicit ACAlgorithm(Config const& config)
        : Primitive(config.data, config.separator, config.has_header,
                    std::vector<std::string_view>()),
          typed_relation_(std::move(TypedRelation::CreateFrom(input_generator_, true))),
          bin_operation_(config.bin_operation),
          fuzziness_(config.fuzziness),
          weight_(config.weight),
          bumps_limit_(config.bumps_limit),
          p_fuzz_(config.p_fuzz),
          iterations_limit(config.iterations_limit),
          pairing_rule_(config.pairing_rule),
          test_mode_(config.test_mode) {}

    ~ACAlgorithm() = default;
    size_t CalculateSampleSize(size_t k_bumps_) const;
    inline const std::vector<RangesCollection>& GetRangesCollection() const {
        return ranges_;
    }
    inline const RangesCollection& GetRangesByColumns(size_t lhs_i, size_t rhs_i) {
        auto res =
            std::find_if(ranges_.begin(), ranges_.end(), [lhs_i, rhs_i](RangesCollection& r) {
                return (r.column_indices.first == lhs_i && r.column_indices.second == rhs_i);
            });
        return *res;
    }
    void PrintRanges(std::vector<model::TypedColumnData> const& data) const;
    unsigned long long Execute() override;
};

}  // namespace algos
