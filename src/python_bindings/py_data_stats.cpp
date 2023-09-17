#include "py_data_stats.h"

#include "statistics/data_stats.h"

namespace python_bindings {

namespace py = pybind11;
namespace mo = model;

namespace {
py::object GetValue(const algos::Statistic& stat) {
    const std::byte* data = stat.GetData();
    const mo::Type* type = stat.GetType();

    if (data == nullptr) return py::none();

    switch (type->GetTypeId()) {
        case mo::TypeId::kDouble:
            return py::float_(mo::Type::GetValue<mo::Double>(data));
        case mo::TypeId::kInt:
            return py::int_(mo::Type::GetValue<mo::Int>(data));
        case mo::TypeId::kString:
            return py::str(mo::Type::GetValue<mo::String>(data));
        default:
            assert(false);
            __builtin_unreachable();
    }
}
}  // namespace

size_t PyDataStats::GetNumberOfValues(size_t index) const {
    return GetAlgorithm<algos::DataStats>(algorithm_).NumberOfValues(index);
}

size_t PyDataStats::GetNumberOfColumns() const {
    return GetAlgorithm<algos::DataStats>(algorithm_).GetNumberOfColumns();
}

std::vector<size_t> PyDataStats::GetNullColumns() const {
    return GetAlgorithm<algos::DataStats>(algorithm_).GetNullColumns();
}

std::vector<size_t> PyDataStats::GetColumnsWithNull() const {
    return GetAlgorithm<algos::DataStats>(algorithm_).GetColumnsWithNull();
}

std::vector<size_t> PyDataStats::GetColumnsWithUniqueValues() const {
    return GetAlgorithm<algos::DataStats>(algorithm_).GetColumnsWithUniqueValues();
}

size_t PyDataStats::Distinct(size_t index) const {
    return GetAlgorithm<algos::DataStats>(algorithm_).Distinct(index);
}

bool PyDataStats::IsCategorical(size_t index, size_t quantity) {
    return GetAlgorithm<algos::DataStats>(algorithm_).IsCategorical(index, quantity);
}

std::vector<std::vector<std::string>> PyDataStats::ShowSample(size_t start_row, size_t end_row,
                                                              size_t start_col, size_t end_col,
                                                              size_t str_len, size_t unsigned_len,
                                                              size_t double_len) const {
    return GetAlgorithm<algos::DataStats>(algorithm_)
            .ShowSample(start_row, end_row, start_col, end_col, str_len, unsigned_len, double_len);
}

py::object PyDataStats::GetAverage(size_t index) const {
    algos::Statistic stat = GetAlgorithm<algos::DataStats>(algorithm_).GetAvg(index);
    return GetValue(stat);
}

py::object PyDataStats::GetCorrectedSTD(size_t index) const {
    algos::Statistic stat = GetAlgorithm<algos::DataStats>(algorithm_).GetCorrectedSTD(index);
    return GetValue(stat);
}

py::object PyDataStats::GetSkewness(size_t index) const {
    algos::Statistic stat = GetAlgorithm<algos::DataStats>(algorithm_).GetSkewness(index);
    return GetValue(stat);
}

py::object PyDataStats::GetKurtosis(size_t index) const {
    algos::Statistic stat = GetAlgorithm<algos::DataStats>(algorithm_).GetKurtosis(index);
    return GetValue(stat);
}

py::object PyDataStats::GetCentralMomentOfDist(size_t index, int number) const {
    algos::Statistic stat =
            GetAlgorithm<algos::DataStats>(algorithm_).GetCentralMomentOfDist(index, number);
    return GetValue(stat);
}

py::object PyDataStats::GetStandardizedCentralMomentOfDist(size_t index, int number) const {
    algos::Statistic stat = GetAlgorithm<algos::DataStats>(algorithm_)
                                    .GetStandardizedCentralMomentOfDist(index, number);
    return GetValue(stat);
}

py::object PyDataStats::CalculateCentralMoment(size_t index, int number,
                                               bool bessel_correction) const {
    algos::Statistic stat = GetAlgorithm<algos::DataStats>(algorithm_)
                                    .CalculateCentralMoment(index, number, bessel_correction);
    return GetValue(stat);
}

py::object PyDataStats::GetMin(size_t index) const {
    algos::Statistic stat = GetAlgorithm<algos::DataStats>(algorithm_).GetMin(index);
    return GetValue(stat);
}

py::object PyDataStats::GetMax(size_t index) const {
    algos::Statistic stat = GetAlgorithm<algos::DataStats>(algorithm_).GetMax(index);
    return GetValue(stat);
}

py::object PyDataStats::GetSum(size_t index) const {
    algos::Statistic stat = GetAlgorithm<algos::DataStats>(algorithm_).GetSum(index);
    return GetValue(stat);
}

py::object PyDataStats::GetQuantile(double part, size_t index, bool calc_all) {
    algos::Statistic stat =
            GetAlgorithm<algos::DataStats>(algorithm_).GetQuantile(part, index, calc_all);
    return GetValue(stat);
}

py::object PyDataStats::GetNumberOfZeros(size_t index) const {
    algos::Statistic stat = GetAlgorithm<algos::DataStats>(algorithm_).GetNumberOfZeros(index);
    return GetValue(stat);
}

py::object PyDataStats::GetNumberOfNegatives(size_t index) const {
    algos::Statistic stat = GetAlgorithm<algos::DataStats>(algorithm_).GetNumberOfNegatives(index);
    return GetValue(stat);
}

py::object PyDataStats::GetSumOfSquares(size_t index) const {
    algos::Statistic stat = GetAlgorithm<algos::DataStats>(algorithm_).GetSumOfSquares(index);
    return GetValue(stat);
}

py::object PyDataStats::GetGeometricMean(size_t index) const {
    algos::Statistic stat = GetAlgorithm<algos::DataStats>(algorithm_).GetGeometricMean(index);
    return GetValue(stat);
}

py::object PyDataStats::GetMeanAD(size_t index) const {
    algos::Statistic stat = GetAlgorithm<algos::DataStats>(algorithm_).GetMeanAD(index);
    return GetValue(stat);
}

py::object PyDataStats::GetMedian(size_t index) const {
    algos::Statistic stat = GetAlgorithm<algos::DataStats>(algorithm_).GetMedian(index);
    return GetValue(stat);
}

py::object PyDataStats::GetMedianAD(size_t index) const {
    algos::Statistic stat = GetAlgorithm<algos::DataStats>(algorithm_).GetMedianAD(index);
    return GetValue(stat);
}

size_t PyDataStats::GetNumNulls(size_t index) const {
    algos::Statistic stat = GetAlgorithm<algos::DataStats>(algorithm_).GetNumNulls(index);
    return static_cast<size_t>(mo::Type::GetValue<mo::Int>(stat.GetData()));
}

}  // namespace python_bindings
