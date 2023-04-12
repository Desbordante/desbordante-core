#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

#include "ac_exception_finder.h"
#include "algorithms/algebraic_constraints/ac.h"
#include "algorithms/algebraic_constraints/ac_pairs_collection.h"
#include "algorithms/algebraic_constraints/ranges_collection.h"
#include "algorithms/algebraic_constraints/typed_column_pair.h"
#include "algorithms/legacy_primitive.h"
#include "algorithms/primitive.h"
#include "model/column_layout_typed_relation_data.h"
#include "types.h"

namespace algos {
/* Discovers Algebraic Constraints (AC). In theory AC consists of: 1) Set of value
 * pairs (a_i, b_k), where a_i from column A and b_k from column B. 2) Pairing
 * rule - bijection beetwen columns A and B. 3) Binary operation. 4) Ranges -
 * set of ranges/intervals that was constructed by grouping results of binary
 * operation between a_i and b_k, boundary elements of a group become range
 * borders. For further reading: https://vldb.org/conf/2003/papers/S20P03.pdf
 * Algorithm chooses pairs of columns with data of same numeric type. Randomly
 * creates sample selection of value pairs and constructs ranges using that sample.
 * Also allows discovering exceptions, where exception is a (a_i, b_k) value pair
 * from columns A and B, that has result of binary operation not belonging to any
 * range discovered for (A, B) column pair */
class ACAlgorithm : public LegacyPrimitive {
private:
    using TypedRelation = model::ColumnLayoutTypedRelationData;

public:
    enum class Binop : char { Plus = '+', Minus = '-', Multiplication = '*', Division = '/' };
    /* Pairing rules: 1) Trivial: a value from column A_1 corresponds to a value in the
     * same row from column A_2 */
    enum class PairingRule { Trivial };

private:
    Binop bin_operation_;
    /* Desired ratio of exceptions. Value lies in (0, 1] */
    double fuzziness_;
    /* Value lies in (0, 1]. Closer to 0 - many short intervals.
     * Closer to 1 - small number of long intervals */
    double weight_;
    /* Limit for ranges amount for a pair of columns.
     * If exceeded, closest ranges combine. Pass 0 to remove limit*/
    size_t bumps_limit_;
    /* Desired probability of not exceeding fuzziness ratio
     * by ratio of exceptional records */
    double p_fuzz_;
    size_t iterations_limit_;
    std::string pairing_rule_;
    std::unique_ptr<TypedRelation> typed_relation_;
    std::unique_ptr<ACExceptionFinder> ac_exception_finder_;
    double seed_;
    std::vector<ACPairsCollection> ac_pairs_;
    std::vector<RangesCollection> ranges_;
    model::INumericType::NumericBinop binop_pointer_ = nullptr;
    std::unique_ptr<model::INumericType> num_type_;

    Binop InitializeBinop(char bin_operation);
    /* Returns vector with ranges boundaries constructed for columns with lhs_i and rhs_i indices.
     * Value pairs (by which ranges constructed) fall into sample selection with chosen probability.
     */
    std::vector<std::byte const*> SamplingIteration(std::vector<model::TypedColumnData> const& data,
                                                    size_t lhs_i, size_t rhs_i, double probability,
                                                    ACPairs& ac_pairs);
    /* Returns vector with ranges boundaries constructed for columns with lhs_i and rhs_i indices.
     * These ranges are part of AC for that column pair (as in AC definition). Uses iterative
     * algorithm that uses SamplingIteration method. In the vast majority of cases there is less
     *  than 4 iterations.  */
    std::vector<std::byte const*> Sampling(std::vector<model::TypedColumnData> const& data,
                                           size_t lhs_i, size_t rhs_i);
    /* Returns vector with ranges boundaries. Ranges constructed by grouping results of binary
     * operation between values in ac_pairs. */
    std::vector<std::byte const*> ConstructDisjunctiveRanges(ACPairs const& ac_pairs,
                                                             double weight) const;
    /* Greedily combines ranges if there is more than bumps_limit_ */
    void RestrictRangesAmount(std::vector<std::byte const*>& ranges) const;

public:
    struct Config {
        std::filesystem::path data{}; /* Path to input file */
        char separator = ',';         /* Separator for csv */
        bool has_header = true;       /* Indicates if input file has header */
        char bin_operation = '+';
        double fuzziness = 0.15;      /* Desired exceptions ratio */
        double p_fuzz = 0.9;          /* Probability of not exceeding fuzziness ratio
                                          by ratio of exceptional records */
        double weight = 0.05;         /* between 0 and 1. Closer to 0 - many short intervals.
                                         To 1 - small number of long intervals */
        size_t bumps_limit = 5;       /* to remove limit: pass value of 0 */
        size_t iterations_limit = 10; /* limit for iterations in Sampling() */
        std::string pairing_rule = "trivial";
        double seed = 0;
    };

    explicit ACAlgorithm(Config const& config)
        : LegacyPrimitive(config.data, config.separator, config.has_header,
                          std::vector<std::string_view>()),
          fuzziness_(config.fuzziness),
          weight_(config.weight),
          bumps_limit_(config.bumps_limit),
          p_fuzz_(config.p_fuzz),
          iterations_limit_(config.iterations_limit),
          pairing_rule_(config.pairing_rule),
          typed_relation_(TypedRelation::CreateFrom(*input_generator_, true)),
          ac_exception_finder_(std::make_unique<ACExceptionFinder>()),
          seed_(config.seed) {
        bin_operation_ = InitializeBinop(config.bin_operation);
    }

    void InvokeBinop(std::byte const* l, std::byte const* r, std::byte* res) const {
        std::invoke(binop_pointer_, num_type_, l, r, res);
    }
    size_t CalculateSampleSize(size_t k_bumps) const;
    /* Returns ranges reconstucted with new weight for pair of columns */
    RangesCollection ReconstructRangesByColumns(size_t lhs_i, size_t rhs_i, double weight) const;
    static bool ValueBelongsToRanges(RangesCollection const& ranges_collection,
                                     std::byte const* val);
    std::vector<RangesCollection> const& GetRangesCollections() const {
        return ranges_;
    }
    std::vector<ACExceptionFinder::ACException> const& GetACExceptions() const {
        return ac_exception_finder_->GetACExceptions();
    }
    RangesCollection const& GetRangesByColumns(size_t lhs_i, size_t rhs_i) const;
    ACPairsCollection const& GetACPairsByColumns(size_t lhs_i, size_t rhs_i) const;
    std::vector<model::TypedColumnData> const& GetTypedData() const {
        return typed_relation_->GetColumnData();
    }
    Binop GetBinOperation() const {
        return bin_operation_;
    }
    void PrintRanges(std::vector<model::TypedColumnData> const& data) const;
    void CollectACExceptions() {
        ac_exception_finder_->CollectExceptions(this);
    }
    unsigned long long ExecuteInternal() override;
};

}  // namespace algos
