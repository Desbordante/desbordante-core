#pragma once
#include <cstddef>
#include <string>
#include <vector>

#include "frequency_handler.h"
#include "sample.h"
#include "table/column_index.h"

namespace model {
class TypedColumnData;
}  // namespace model

namespace algos {
class FrequencyHandler;
class Sample;

class ContingencyTable {
private:
    model::ColumnIndex col_i_;
    model::ColumnIndex col_k_;
    std::vector<std::vector<long double>> n_i_j_;
    std::vector<long double> n_i_;
    std::vector<long double> n_j_;

    [[nodiscard]] static size_t Category(model::ColumnIndex col_ind, std::string const &val,
                                         size_t domain, bool skew, FrequencyHandler const &handler);
    [[nodiscard]] long double CalculateChiSquared(long double sample_size,
                                                  std::vector<size_t> const &domains) const;

public:
    bool ChiSquaredTest(Sample const &smp, std::vector<size_t> const &domains,
                        long double max_false_positive_probability) const;
    ContingencyTable(model::ColumnIndex col_i, model::ColumnIndex col_k,
                     std::vector<size_t> const &domains);
    void FillTable(Sample const &smp, std::vector<model::TypedColumnData> const &data,
                   FrequencyHandler const &handler, std::vector<bool> const &is_skewed_,
                   std::vector<size_t> const &domains_);

    [[nodiscard]] bool TooMuchStructuralZeroes(std::vector<size_t> const &domains_,
                                               long double min_structural_zeroes_proportion) const;
};

}  // namespace algos
