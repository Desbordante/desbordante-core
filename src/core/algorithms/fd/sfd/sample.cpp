#include "core/algorithms/fd/sfd/sample.h"

#include <chrono>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

#include "core/algorithms/fd/sfd/frequency_handler.h"
#include "core/model/table/tuple_index.h"

namespace algos {
Sample::Sample(bool fixed_sample, unsigned long long sample_size, model::TupleIndex rows,
               model::ColumnIndex lhs, model::ColumnIndex rhs,
               std::vector<model::TypedColumnData> const &data, RelationalSchema const *rel_schema_)
    : lhs_col_(rel_schema_, rel_schema_->GetColumn(lhs)->GetName(), lhs),
      rhs_col_(rel_schema_, rel_schema_->GetColumn(rhs)->GetName(), rhs) {
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937 gen(seed);
    std::uniform_int_distribution<model::TupleIndex> distribution(0, rows - 1);

    std::unordered_set<std::string> map_lhs;
    std::unordered_set<std::string> map_rhs;
    std::unordered_set<std::string> map_cardinality;

    for (model::ColumnIndex i = 0; i < sample_size; i++) {
        model::TupleIndex row = (fixed_sample) ? i % rows : distribution(gen);

        row_indices_.push_back(row);
        map_lhs.insert(data[lhs].GetDataAsString(row));
        map_rhs.insert(data[rhs].GetDataAsString(row));
        map_cardinality.insert(data[lhs].GetDataAsString(row) + data[rhs].GetDataAsString(row));
    }
    lhs_cardinality_ = map_lhs.size();
    rhs_cardinality_ = map_rhs.size();
    concat_cardinality_ = map_cardinality.size();
}

unsigned long long Sample::CalculateSampleSize(size_t lhs_cardinality, size_t rhs_cardinality,
                                               long double max_false_positive_probability,
                                               long double delta) {
    long double v = (lhs_cardinality - 1) * (rhs_cardinality - 1);
    long double d = std::min(lhs_cardinality, rhs_cardinality);
    long double log = std::log(max_false_positive_probability * std::sqrt(2 * std::numbers::pi));
    long double numerator = std::pow(-16 * v * log, 0.5) - 8 * log;
    long double denominator = delta * (d - 1);
    long double v2 = std::pow(v, 0.071);
    return static_cast<long long>((numerator / denominator) * (v2 / 1.69));
}

void Sample::Filter(FrequencyHandler const &handler,
                    std::vector<model::TypedColumnData> const &data, model::ColumnIndex col_ind) {
    std::erase_if(row_indices_, [&handler, &data, col_ind](model::TupleIndex row_id) {
        return !handler.ContainsValAtColumn(data[col_ind].GetDataAsString(row_id), col_ind);
    });
}
}  // namespace algos
