#pragma once

#include "algorithms/fd/fd_algorithm.h"
#include "algorithms/statistics/statistic.h"
#include "config/equal_nulls/type.h"
#include "config/tabular_data/input_table_type.h"
#include "config/thread_number/type.h"
#include "model/table/column_layout_typed_relation_data.h"

namespace algos {

class DataStats : public Algorithm {
    config::EqNullsType is_null_equal_null_;
    config::ThreadNumType threads_num_;

    std::vector<model::TypedColumnData> col_data_;
    std::vector<ColumnStats> all_stats_;

    size_t MixedDistinct(size_t index) const;
    void RegisterOptions();

    void ResetState() final;

    // Returns number of elements satisfying the predicate
    template <class Pred, class Data>
    size_t CountIf(Pred pred, Data const& data) const;

    // Returns vector with indices satisfying the predicate
    template <class Pred, class Data>
    std::vector<size_t> FilterIndices(Pred pred, Data const& data) const;

    // Returns number of chars in a column satisfying the predicate
    template <class Pred>
    Statistic CountIfInColumn(Pred pred, size_t index) const;

    // Base method for number of negatives and number of zeros statistics
    Statistic CountIfInBinaryRelationWithZero(size_t index, model::CompareResult res) const;

    // Returns median value for numeric vector
    static std::byte* MedianOfNumericVector(std::vector<std::byte const*> const& data,
                                            model::INumericType const& type);

protected:
    config::InputTable input_table_;

    void LoadDataInternal() final;
    void MakeExecuteOptsAvailable() final;
    unsigned long long ExecuteInternal() final;

public:
    DataStats();

    std::vector<model::TypedColumnData> const& GetData() const noexcept;

    // Returns number of non-NULL and nonempty values in the column.
    size_t NumberOfValues(size_t index) const;
    // Returns number of columns in table.
    size_t GetNumberOfColumns() const;
    // Returns columns which contain a null value.
    std::vector<size_t> GetColumnsWithNull() const;
    // Returns columns with only null values.
    std::vector<size_t> GetNullColumns() const;
    // Returns columns with only unique values.
    std::vector<size_t> GetColumnsWithUniqueValues();
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
    // Returns quantile of the column if its type is comparable.
    Statistic GetQuantile(double part, size_t index, bool calc_all = false);
    // Deletes null and empty values in the column.
    std::vector<std::byte const*> DeleteNullAndEmpties(size_t index) const;
    // Returns number of zeros in the column if it's numeric.
    Statistic GetNumberOfZeros(size_t index) const;
    // Returns number of negative numbers in the column if it's numeric.
    Statistic GetNumberOfNegatives(size_t index) const;
    // Returns sum of numbers' squares in the column if it's numeric.
    Statistic GetSumOfSquares(size_t index) const;
    // Returns geometric mean of numbers in the column if it's numeric.
    Statistic GetGeometricMean(size_t index) const;
    // Returns mean absolute deviation if it's numeric.
    Statistic GetMeanAD(size_t index) const;
    // Returns median of the column if it's numeric.
    Statistic GetMedian(size_t index) const;
    // Returns meadian absolute deviation in the column if it's numeric.
    Statistic GetMedianAD(size_t index) const;
    // Returns number of nulls in the column.
    Statistic GetNumNulls(size_t index) const;
    // Returns all distinct symbols of the column as a sorted string.
    Statistic GetVocab(size_t index) const;
    // Returns number of non-letter chars in a string column.
    Statistic GetNumberOfNonLetterChars(size_t index) const;
    // Returns number of digit chars in a string column.
    Statistic GetNumberOfDigitChars(size_t index) const;

    ColumnStats const& GetAllStats(size_t index) const;
    std::vector<ColumnStats> const& GetAllStats() const;
    std::string ToString() const;
};

}  // namespace algos
