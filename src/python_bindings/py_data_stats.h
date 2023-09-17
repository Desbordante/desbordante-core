#pragma once

#include "algorithms/statistics/data_stats.h"
#include "get_algorithm.h"
#include "py_algorithm.h"

namespace python_bindings {

class PyDataStats : public PyAlgorithm<algos::DataStats, PyAlgorithmBase> {
    using PyAlgorithmBase::algorithm_;

public:
    [[nodiscard]] std::string GetAllStatisticsAsString() const {
        return GetAlgorithm<algos::DataStats>(algorithm_).ToString();
    }

    [[nodiscard]] size_t GetNumberOfValues(size_t index) const;
    [[nodiscard]] size_t GetNumberOfColumns() const;
    [[nodiscard]] std::vector<size_t> GetNullColumns() const;
    [[nodiscard]] std::vector<size_t> GetColumnsWithNull() const;
    [[nodiscard]] std::vector<size_t> GetColumnsWithUniqueValues() const;
    [[nodiscard]] size_t Distinct(size_t index) const;
    [[nodiscard]] bool IsCategorical(size_t index, size_t quantity);
    [[nodiscard]] std::vector<std::vector<std::string>> ShowSample(size_t start_row, size_t end_row,
                                                                   size_t start_col, size_t end_col,
                                                                   size_t str_len = 10,
                                                                   size_t unsigned_len = 5,
                                                                   size_t double_len = 10) const;
    [[nodiscard]] pybind11::object GetAverage(size_t index) const;
    [[nodiscard]] pybind11::object GetCorrectedSTD(size_t index) const;
    [[nodiscard]] pybind11::object GetSkewness(size_t index) const;
    [[nodiscard]] pybind11::object GetKurtosis(size_t index) const;
    [[nodiscard]] pybind11::object GetCentralMomentOfDist(size_t index, int number) const;
    [[nodiscard]] pybind11::object GetStandardizedCentralMomentOfDist(size_t index,
                                                                      int number) const;
    [[nodiscard]] pybind11::object CalculateCentralMoment(size_t index, int number,
                                                          bool bessel_correction) const;
    [[nodiscard]] pybind11::object GetMin(size_t index) const;
    [[nodiscard]] pybind11::object GetMax(size_t index) const;
    [[nodiscard]] pybind11::object GetSum(size_t index) const;
    [[nodiscard]] pybind11::object GetQuantile(double part, size_t index, bool calc_all = false);
    [[nodiscard]] pybind11::object GetNumberOfZeros(size_t index) const;
    [[nodiscard]] pybind11::object GetNumberOfNegatives(size_t index) const;
    [[nodiscard]] pybind11::object GetSumOfSquares(size_t index) const;
    [[nodiscard]] pybind11::object GetGeometricMean(size_t index) const;
    [[nodiscard]] pybind11::object GetMeanAD(size_t index) const;
    [[nodiscard]] pybind11::object GetMedian(size_t index) const;
    [[nodiscard]] pybind11::object GetMedianAD(size_t index) const;
    [[nodiscard]] size_t GetNumNulls(size_t index) const;
};

}  // namespace python_bindings
