#include "algorithms/nd/nd_verifier/nd_verifier.h"

#include "config/descriptions.h"
#include "config/equal_nulls/option.h"
#include "config/indices/option.h"
#include "config/names.h"
#include "config/option.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"

namespace algos::nd_verifier {

NDVerifier::NDVerifier() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName(), config::kEqualNullsOpt.GetName()});
}

void NDVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto get_schema_cols = [this]() { return relation_->GetSchema()->GetNumColumns(); };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));
    RegisterOption(config::kLhsIndicesOpt(&lhs_indices_, get_schema_cols));
    RegisterOption(config::kRhsIndicesOpt(&rhs_indices_, get_schema_cols));
    RegisterOption(Option<unsigned>(&weight_, kWeight, kDNDWeight, 1));
}

void NDVerifier::LoadDataInternal() {}

void NDVerifier::MakeExecuteOptsAvailable() {}

unsigned long long NDVerifier::ExecuteInternal() {
    auto start_time = std::chrono::system_clock::now();

    auto const& indices_to_str = [](config::IndicesType const& vect) -> std::string {
        std::stringstream ss;
        for (auto pt{vect.begin()}; pt != vect.end(); ++pt) {
            if (pt != vect.begin()) {
                ss << ", ";
            }
            ss << *pt;
        }
        return ss.str();
    };

    LOG(INFO) << "NDVerifier::ExecuteInternal started";
    LOG(INFO) << "Parameters:";
    LOG(INFO) << "\tInput table: " << input_table_->GetRelationName();
    LOG(INFO) << "\tNull equals null: " << is_null_equal_null_;
    LOG(INFO) << "\tLhs indices: " << indices_to_str(lhs_indices_);
    LOG(INFO) << "\tRhs indices: " << indices_to_str(rhs_indices_);
    LOG(INFO) << "\tWeight: " << weight_;

    NaiveCheckND();

    stats_calculator_.CalculateStats();

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    LOG(INFO) << "NDVerifier::ExecuteInternal finished in " << elapsed_milliseconds;
    return elapsed_milliseconds.count();
}

bool NDVerifier::NDHolds() {
    return stats_calculator_.GetRealWeight() <= weight_;
}

/// @brief Check if ND holds by definition (brute-force)
void NDVerifier::NaiveCheckND() {
    // I want to consider multiple columns as single column:
    auto lhs_rows = CombineColumnValues(typed_relation_, lhs_indices_);
    auto rhs_rows = CombineColumnValues(typed_relation_, rhs_indices_);

    // I want to work with integers, not ValueCombinations:
    auto [lhs_codes, encoded_lhs] = EncodeColumnValues(*lhs_rows);
    auto [rhs_codes, encoded_rhs] = EncodeColumnValues(*rhs_rows);
    // TODO: merge this two operations into one

    auto encoded_rows_to_string =
            [](std::vector<ValueCombination<ValueType>> const& rows,
               std::vector<size_t> const& encoded_rows,
               std::vector<ValueCombination<ValueType>> const& codes) -> std::string {
        std::stringstream ss;
        ss << "Values\tCodes\tDecoded values\n";
        for (size_t i{0}; i < rows.size(); ++i) {
            auto const& initial_value = rows[i];
            auto code = encoded_rows[i];
            auto const& decoded_value = codes[code];

            ss << initial_value.ToString() << '\t' << code << '\t' << decoded_value.ToString()
               << '\n';
        }
        return ss.str();
    };

    LOG(INFO) << "Lhs:\n" << encoded_rows_to_string(*lhs_rows, *encoded_lhs, *lhs_codes) << '\n';
    LOG(INFO) << "Rhs:\n" << encoded_rows_to_string(*rhs_rows, *encoded_rhs, *rhs_codes);

    auto value_deps = std::make_shared<std::unordered_map<size_t, std::unordered_set<size_t>>>();

    for (size_t i{0}; i < encoded_lhs->size(); ++i) {
        auto lhs_code = encoded_lhs->at(i);
        auto rhs_code = encoded_rhs->at(i);

        if (value_deps->find(lhs_code) == value_deps->end()) {
            value_deps->emplace(lhs_code, std::unordered_set<size_t>({rhs_code}));
        } else {
            value_deps->at(lhs_code).insert(rhs_code);
        }
    }

    stats_calculator_ = StatsCalculator(value_deps, lhs_codes, rhs_codes, encoded_lhs, encoded_rhs);
}

void NDVerifier::ResetState() {
    stats_calculator_ = StatsCalculator<ValueType>{};
}

}  // namespace algos::nd_verifier
