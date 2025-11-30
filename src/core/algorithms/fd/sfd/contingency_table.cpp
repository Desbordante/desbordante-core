#include "core/algorithms/fd/sfd/contingency_table.h"

#include <boost/math/distributions/chi_squared.hpp>

#include "core/algorithms/fd/sfd/frequency_handler.h"
#include "core/algorithms/fd/sfd/sample.h"

namespace algos {
ContingencyTable::ContingencyTable(model::ColumnIndex col_i, model::ColumnIndex col_k,
                                   std::vector<size_t> const &domains_)
    : col_i_(col_i),
      col_k_(col_k),
      n_i_j_(domains_[col_i], std::vector<long double>(domains_[col_k], 0)),
      n_i_(domains_[col_i], 0),
      n_j_(domains_[col_k], 0) {}

[[nodiscard]] size_t ContingencyTable::Category(model::ColumnIndex col_ind, std::string const &val,
                                                size_t domain, bool skew,
                                                FrequencyHandler const &handler) {
    if (skew) {
        return handler.GetValueOrdinalNumberAtColumn(val, col_ind);
    }
    return std::hash<std::string>{}(val) % domain;
}

void ContingencyTable::FillTable(Sample const &smp, std::vector<model::TypedColumnData> const &data,
                                 FrequencyHandler const &handler,
                                 std::vector<bool> const &is_skewed_,
                                 std::vector<size_t> const &domains_) {
    for (model::TupleIndex row_ind : smp.GetRowIndices()) {
        size_t i = Category(col_i_, data[col_i_].GetDataAsString(row_ind), domains_[col_i_],
                            is_skewed_[col_i_], handler);
        size_t j = Category(col_k_, data[col_k_].GetDataAsString(row_ind), domains_[col_k_],
                            is_skewed_[col_k_], handler);
        n_i_j_[i][j]++;
        n_i_[i]++;
        n_j_[j]++;
    }
}

/* Similar to formulae (1) from "CORDS: Automatic Discovery of Correlations and Soft
   Functional Dependencies". Seems like formulae which is given in the paper is
   incorrect. */

long double ContingencyTable::CalculateChiSquared(long double sample_size,
                                                  std::vector<size_t> const &domains) const {
    long double chi_squared = 0;
    for (size_t i = 0; i < domains[col_i_]; i++) {
        for (size_t j = 0; j < domains[col_k_]; j++) {
            if (n_i_[i] * n_j_[j] == 0) return 0;
            long double actual = n_i_j_[i][j];
            long double expected = n_i_[i] * n_j_[j] / sample_size;
            chi_squared += (actual - expected) * (actual - expected) / (expected);
        }
    }
    return chi_squared;
}

bool ContingencyTable::ChiSquaredTest(Sample const &smp, std::vector<size_t> const &domains,
                                      long double max_false_positive_probability) const {
    long double chi_squared = CalculateChiSquared(smp.GetRowIndices().size(), domains);

    long double v = (domains[col_i_] - 1) * (domains[col_k_] - 1);

    boost::math::chi_squared dist(v);

    long double t = quantile(dist, 1 - max_false_positive_probability);
    return chi_squared > t;
}

bool ContingencyTable::TooMuchStructuralZeroes(std::vector<size_t> const &domains,
                                               long double min_structural_zeroes_proportion) const {
    long double zeros_sum = 0;
    for (size_t i = 0; i < domains[col_i_]; i++) {
        zeros_sum += std::count_if(n_i_j_[i].begin(), n_i_j_[i].end(),
                                   [](long double val) { return val == 0; });
    }
    return zeros_sum > min_structural_zeroes_proportion * domains[col_i_] * domains[col_k_];
}

}  // namespace algos
