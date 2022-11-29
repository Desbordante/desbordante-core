#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

#include "algorithms/legacy_primitive.h"
#include "algorithms/primitive.h"
#include "model/ac.h"
#include "model/column_layout_typed_relation_data.h"
#include "types.h"

namespace algos {

class ACAlgorithm : public LegacyPrimitive {
public:
    enum class Binop : char { Plus = '+', Minus = '-', Multiplication = '*', Division = '/' };
    enum class PairingRule { Trivial };

    /* A set of intervals for a specific pair of columns */
    struct RangesCollection {
        RangesCollection(std::unique_ptr<model::INumericType> num_type,
                         std::vector<std::byte const*>&& ranges, size_t lhs_i, size_t rhs_i)
            : column_indices(lhs_i, rhs_i),
              num_type(std::move(num_type)),
              ranges(std::move(ranges)) {}
        /* Column indexes, the first for the column whose values were the left operand
         * for binop_, the second for the right */
        std::pair<size_t, size_t> column_indices;
        /* Columns type */
        std::unique_ptr<model::INumericType> num_type;
        /* border values of the intervals. Even element --
         * left border. Odd -- right */
        std::vector<std::byte const*> ranges;
    };

private:
    using TypedRelation = model::ColumnLayoutTypedRelationData;
    using Constraints = std::vector<std::unique_ptr<model::AC>>;

    std::vector<std::byte*> disjunctive_ranges_;
    std::vector<Constraints> alg_constraints_;
    std::vector<RangesCollection> ranges_;
    std::unique_ptr<TypedRelation> typed_relation_;
    Binop bin_operation_;
    model::INumericType::NumericBinop binop_pointer_ = nullptr;
    std::unique_ptr<model::INumericType> num_type_;
    double fuzziness_;
    double weight_;
    size_t bumps_limit_;
    double p_fuzz_;
    size_t iterations_limit;
    std::string pairing_rule_;
    bool test_mode_;

    Binop InitializeBinop(char bin_operation);
    void InvokeBinop(std::byte const* l, std::byte const* r, std::byte* res) {
        std::invoke(binop_pointer_, num_type_, l, r, res);
    }
    std::vector<std::byte const*> SamplingIteration(std::vector<model::TypedColumnData> const& data,
                                                    size_t lhs_i, size_t rhs_i, double probability,
                                                    Constraints& constraints);
    std::vector<std::byte const*> Sampling(std::vector<model::TypedColumnData> const& data,
                                           size_t lhs_i, size_t rhs_i, size_t k_bumps);
    std::vector<std::byte const*> ConstructDisjunctiveRanges(Constraints&) const;
    void RestrictRangesAmount(std::vector<std::byte const*>&) const;

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
    };

    explicit ACAlgorithm(Config const& config, bool test_mode = false)
        : LegacyPrimitive(config.data, config.separator, config.has_header,
                    std::vector<std::string_view>()),
          typed_relation_(TypedRelation::CreateFrom(*input_generator_, true)),
          fuzziness_(config.fuzziness),
          weight_(config.weight),
          bumps_limit_(config.bumps_limit),
          p_fuzz_(config.p_fuzz),
          iterations_limit(config.iterations_limit),
          pairing_rule_(config.pairing_rule),
          test_mode_(test_mode) {

        bin_operation_ = InitializeBinop(config.bin_operation);
    }

    size_t CalculateSampleSize(size_t k_bumps) const;
    std::vector<RangesCollection> const& GetRangesCollections() const {
        return ranges_;
    }
    RangesCollection const& GetRangesByColumns(size_t lhs_i, size_t rhs_i) const;
    void PrintRanges(std::vector<model::TypedColumnData> const& data) const;
    unsigned long long ExecuteInternal() override;
};

}  // namespace algos
