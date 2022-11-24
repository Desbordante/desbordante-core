#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

#include "ac.h"
#include "column_layout_typed_relation_data.h"
#include "primitive.h"
#include "types.h"

namespace algos {

class ACAlgorithm : public algos::Primitive {
public:
    enum Binop { Plus = '+', Minus = '-', Multiplication = '*', Division = '/' };
    enum PairingRule { Trivial };

    /* Набор промежутков для конкретной пары колонок */
    struct RangesCollection {
        RangesCollection(std::unique_ptr<model::INumericType> num_type,
                         std::vector<std::byte const*>&& ranges, size_t lhs_i, size_t rhs_i)
            : column_indices(lhs_i, rhs_i),
              num_type(std::move(num_type)),
              ranges(std::move(ranges)) {}
        /* Индексы колонок, первый для колонки, значения
         * которой были левым операндом для binop_, второй -- правым */
        std::pair<size_t, size_t> column_indices;
        /* Тип колонок */
        std::unique_ptr<model::INumericType> num_type;
        /* Значения границ промежутков. Четный элемент --
         * левая граница Нечетный -- правая */
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

    static Binop ValidateOperator(char bin_operation) {
        switch (bin_operation) {
        case Plus:
            return Plus;
        case Minus:
            return Minus;
        case Multiplication:
            return Multiplication;
        case Division:
            return Division;
        default:
            throw std::invalid_argument("Invalid operation for algebraic constraints discovery");
            break;
        }
    }
    void InitializeBinop();
    void InvokeBinop(std::byte const* l, std::byte const* r, std::byte* res) {
        std::invoke(binop_pointer_, num_type_, l, r, res);
    }
    std::vector<std::byte const*> SamplingIteration(std::vector<model::TypedColumnData> const& data,
                                                    size_t lhs_i, size_t rhs_i, double probability,
                                                    Constraints& constraints);
    std::vector<std::byte const*> Sampling(std::vector<model::TypedColumnData> const& data,
                                           size_t lhs_i, size_t rhs_i, size_t k_bumps);
    std::vector<model::AC*> FindAllConstraints(std::vector<model::TypedColumnData> const& data_,
                                               size_t lhs_i, size_t rhs_i);
    void ConstructDisjunctiveRanges(Constraints&, std::vector<std::byte const*>&) const;
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
        : Primitive(config.data, config.separator, config.has_header,
                    std::vector<std::string_view>()),
          typed_relation_(TypedRelation::CreateFrom(*input_generator_, true)),
          fuzziness_(config.fuzziness),
          weight_(config.weight),
          bumps_limit_(config.bumps_limit),
          p_fuzz_(config.p_fuzz),
          iterations_limit(config.iterations_limit),
          pairing_rule_(config.pairing_rule),
          test_mode_(test_mode) {
        bin_operation_ = ValidateOperator(config.bin_operation);
    }

    size_t CalculateSampleSize(size_t k_bumps) const;
    std::vector<RangesCollection> const& GetRangesCollections() const {
        return ranges_;
    }
    RangesCollection const& GetRangesByColumns(size_t lhs_i, size_t rhs_i) const {
        auto res =
            std::find_if(ranges_.begin(), ranges_.end(), [lhs_i, rhs_i](RangesCollection const& r) {
                return (r.column_indices.first == lhs_i && r.column_indices.second == rhs_i);
            });
        if (res == ranges_.end()) {
            throw std::invalid_argument("No ranges for selected pair of columns");
        }
        return *res;
    }
    void PrintRanges(std::vector<model::TypedColumnData> const& data) const;
    unsigned long long Execute() override;
};

}  // namespace algos
