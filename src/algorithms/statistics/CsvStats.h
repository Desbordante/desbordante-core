#pragma once

#include "../../model/ColumnLayoutTypedRelationData.h"
#include "../FDAlgorithm.h"
#include "Statistic.h"

namespace algos {

class CsvStats : public algos::Primitive {
public:
    explicit CsvStats(const FDAlgorithm::Config& config);

    CsvStats(CsvStats&&) = delete;
    CsvStats(const CsvStats&) = delete;
    CsvStats& operator=(CsvStats&&) = delete;
    CsvStats& operator=(const CsvStats&) = delete;

    const std::vector<model::TypedColumnData>& GetData() const noexcept;

    // Returns count of defined and nonempty values in the column.
    int Count(unsigned index);
    // Returns count of columns in table.
    unsigned GetCountOfColumns();
    // Returns count of unique values in the column.
    int Distinct(unsigned index);
    // Check if quantity <= count of unique values in the column.
    bool isCategorical(unsigned index, int quantity);
    // Show part of the table.
    void ShowSample(unsigned start_row, unsigned end_row, unsigned start_col, unsigned end_col,
                    unsigned str_length = 10, unsigned unsigned_len = 5, unsigned double_len = 10);
    // Returns avarage value in the column if it's numeric.
    Statistic GetAvg(unsigned index);
    // Returns standard deviation of the column if it's numeric.
    Statistic GetSTD(unsigned index);
    // Returns skewness of the column if it's numeric.
    Statistic GetSkewness(unsigned index);
    // Returns kurtosis of the column if it's numeric.
    Statistic GetKurtosis(unsigned index);
    // Returns central moment of the column if it's numeric.
    Statistic CountCentralMomentOfDist(unsigned index, int number);
    // Returns standardized moment of the column if it's numeric.
    Statistic CountStandardizedCentralMomentOfDist(unsigned index, int number);
    // Returns minimum value of the column.
    Statistic GetMin(unsigned index);
    // Returns maximum value of the column.
    Statistic GetMax(unsigned index);
    // Returns sum of the column's values if it's numeric.
    Statistic GetSum(unsigned index);
    // Returns quantile of the column if it's type is comparable.
    Statistic GetQuantile(double part, unsigned index);
    // Delete null and empty values in the column.
    std::vector<const std::byte*> DeleteNullAndEmpties(unsigned index);

    const ColumnStats& GetAllStats(unsigned index) const;
    const std::vector<ColumnStats>& GetAllStats() const;
    ColumnStats& GetAllStats(unsigned index);
    std::vector<ColumnStats>& GetAllStats();
    std::string ToString();
    unsigned long long Execute();

private:
    FDAlgorithm::Config config_;
    const std::vector<model::TypedColumnData> col_data_;
    std::vector<ColumnStats> all_stats;
    ushort threads_num;

    Statistic STDAndCentralMoment(unsigned index, int number, bool is_for_STD);
};

}  // namespace algos
