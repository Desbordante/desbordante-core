#pragma once

#include "column_layout_typed_relation_data.h"
#include "fd_algorithm.h"
#include "statistic.h"

namespace algos {

class CsvStats : public algos::Primitive {
    FDAlgorithm::Config config_;
    const std::vector<model::TypedColumnData> col_data_;
    std::vector<ColumnStats> all_stats_;
    ushort threads_num_;

    size_t MixedDistinct(size_t index) const;

public:
    explicit CsvStats(const FDAlgorithm::Config& config);

    const std::vector<model::TypedColumnData>& GetData() const noexcept;

    // Returns number of non-NULL and nonempty values in the column.
    size_t NumberOfValues(size_t index) const;
    // Returns number of columns in table.
    size_t GetNumberOfColumns() const;
    // Returns number of unique values in the column.
    size_t Distinct(size_t index);
    // Check if quantity <= count of unique values in the column.
    bool IsCategorical(size_t index, size_t quantity);
    // Returns table slice from start_row to end_row and from start_col to end_col.
    // Data values converted to string type.
    std::vector<std::vector<std::string>> ShowSample(size_t start_row, size_t end_row,
                                                     size_t start_col, size_t end_col,
                                                     size_t str_len = 10, size_t unsigned_len = 5,
                                                     size_t double_len = 10) const;
    // Returns avarage value in the column if it's numeric.
    Statistic GetAvg(size_t index) const;
    // Returns corrected standard deviation of the column if it's numeric.
    Statistic GetCorrectedSTD(size_t index) const;
    // Returns skewness of the column if it's numeric.
    Statistic GetSkewness(size_t index) const;
    // Returns kurtosis of the column if it's numeric.
    Statistic GetKurtosis(size_t index) const;
    // Returns central moment of the column if it's numeric.
    Statistic GetCentralMomentOfDist(size_t index, int number) const;
    // Returns standardized moment of the column if it's numeric.
    Statistic GetStandardizedCentralMomentOfDist(size_t index, int number) const;
    // Returns central moment of the column if it's numeric.
    Statistic CalculateCentralMoment(size_t index, int number, bool bessel_correction) const;
    // Returns minimum (maximumin if order = mo::CompareResult::kGreater) value of the column.
    Statistic GetMin(size_t index, model::CompareResult order = model::CompareResult::kLess) const;
    // Returns maximumin value of the column.
    Statistic GetMax(size_t index) const;
    // Returns sum of the column's values if it's numeric.
    Statistic GetSum(size_t index) const;
    // Returns quantile of the column if it's type is comparable.
    Statistic GetQuantile(double part, size_t index, bool calc_all = false);
    // Deletes null and empty values in the column.
    std::vector<const std::byte*> DeleteNullAndEmpties(size_t index);

    const ColumnStats& GetAllStats(size_t index) const;
    const std::vector<ColumnStats>& GetAllStats() const;
    std::string ToString() const;
    unsigned long long Execute();
};

}  // namespace algos
