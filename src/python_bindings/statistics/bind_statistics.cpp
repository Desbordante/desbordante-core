#include "bind_statistics.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "algorithms/statistics/data_stats.h"
#include "py_util/bind_primitive.h"

namespace {
namespace py = pybind11;
}  // namespace

namespace PYBIND11_NAMESPACE {
namespace detail {
template <>
struct type_caster<algos::Statistic> {
public:
    /**
     * This macro establishes the name 'float | int | str' in
     * function signatures and declares a local variable
     * 'value' of type algos::Statistic
     */
    PYBIND11_TYPE_CASTER(algos::Statistic, const_name("float | int | str"));

    /**
     * Conversion part 2 (C++ -> Python): convert an algos::Statistic instance into
     * a Python object. The second and third arguments are used to
     * indicate the return value policy and parent object (for
     * ``return_value_policy::reference_internal``) and are generally
     * ignored by implicit casters.
     */
    static handle cast(algos::Statistic const& stat, return_value_policy /* policy */,
                       handle /* parent */) {
        using model::Type, model::TypeId, model::Double, model::Int, model::String;
        std::byte const* data = stat.GetData();
        Type const* type = stat.GetType();

        if (!stat.HasValue()) Py_RETURN_NONE;

        switch (type->GetTypeId()) {
            case TypeId::kDouble:
                return PyFloat_FromDouble(Type::GetValue<Double>(data));
            case TypeId::kInt:
                return PyLong_FromLongLong(Type::GetValue<Int>(data));
            case TypeId::kString: {
                auto const& str = Type::GetValue<String>(data);
                return PyUnicode_FromStringAndSize(str.data(), str.size());
            }
            default:
                assert(false);
                __builtin_unreachable();
        }
    }
};
}  // namespace detail

}  // namespace PYBIND11_NAMESPACE

namespace python_bindings {
void BindStatistics(pybind11::module_& main_module) {
    using namespace algos;

    auto statistics_module = main_module.def_submodule("statistics");

    BindPrimitiveNoBase<DataStats>(statistics_module, "DataStats")
            .def("get_all_statistics_as_string", &DataStats::ToString)
            .def("get_number_of_values", &DataStats::NumberOfValues,
                 "Get number of values in the column.", py::arg("index"))
            .def("get_number_of_columns", &DataStats::GetNumberOfColumns,
                 "Get number of columns in the table.")
            .def("get_null_columns", &DataStats::GetNullColumns,
                 "Get indices of columns with only null values.")
            .def("get_columns_with_null", &DataStats::GetColumnsWithNull,
                 "Get indices of columns which contain null value.")
            .def("get_columns_with_all_unique_values", &DataStats::GetColumnsWithUniqueValues,
                 "Get indices of columns where all values are distinct.")
            .def("get_number_of_distinct", &DataStats::Distinct,
                 "Get number of unique values in the column.", py::arg("index"))
            .def("is_categorical", &DataStats::IsCategorical,
                 "Check if quantity is greater than number of unique values in the "
                 "column.",
                 py::arg("index"), py::arg("quantity"))
            .def("show_sample", &DataStats::ShowSample, py::arg("start_row"), py::arg("end_row"),
                 py::arg("start_col"), py::arg("end_col"), py::arg("str_len") = 10,
                 py::arg("unsigned_len") = 5, py::arg("double_len") = 10,
                 "Returns a table slice containing values from rows in the range "
                 "[start_row, "
                 "end_row] and columns in the range [start_col, end_col]. Data "
                 "values are "
                 "converted to strings.")
            .def("get_average", &DataStats::GetAvg,
                 "Returns average value in the column if it's numeric.", py::arg("index"))
            .def("get_corrected_std", &DataStats::GetCorrectedSTD,
                 "Returns corrected standard deviation of the column if it's "
                 "numeric.",
                 py::arg("index"))
            .def("get_skewness", &DataStats::GetSkewness,
                 "Returns skewness of the column if it's numeric.", py::arg("index"))
            .def("get_kurtosis", &DataStats::GetKurtosis,
                 "Returns kurtosis of the column if it's numeric.", py::arg("index"))
            .def(
                    "get_min",
                    [](DataStats const& data_stats, std::size_t index) {
                        return data_stats.GetMin(index);
                    },
                    "Returns minimum value of the column.", py::arg("index"))
            .def("get_max", &DataStats::GetMax, "Returns maximumin value of the column.",
                 py::arg("index"))
            .def("get_sum", &DataStats::GetSum,
                 "Returns sum of the column's values if it's numeric.", py::arg("index"))
            .def("get_quantile", &DataStats::GetQuantile,
                 "Returns quantile of the column if its type is comparable.", py::arg("part"),
                 py::arg("index"), py::arg("calc_all") = false)
            .def("get_number_of_zeros", &DataStats::GetNumberOfZeros,
                 "Returns number of zeros in the column if it's numeric.", py::arg("index"))
            .def("get_number_of_negatives", &DataStats::GetNumberOfNegatives,
                 "Returns number of negative numbers in the column if it's "
                 "numeric.",
                 py::arg("index"))
            .def("get_sum_of_squares", &DataStats::GetSumOfSquares,
                 "Returns sum of numbers' squares in the column if it's numeric.", py::arg("index"))
            .def("get_geometric_mean", &DataStats::GetGeometricMean,
                 "Returns geometric mean of numbers in the column if it's numeric.",
                 py::arg("index"))
            .def("get_mean_ad", &DataStats::GetMeanAD,
                 "Returns mean absolute deviation of the column if it's numeric.", py::arg("index"))
            .def("get_median", &DataStats::GetMedian,
                 "Returns median of the column if it's numeric.", py::arg("index"))
            .def("get_median_ad", &DataStats::GetMedianAD,
                 "Returns meadian absolute deviation of the column if it's "
                 "numeric.",
                 py::arg("index"))
            .def("get_num_nulls", &DataStats::GetNumNulls, "Returns number of nulls in the column.",
                 py::arg("index"))
            .def("get_vocab", &DataStats::GetVocab,
                 "Returns all the symbols of the columns as a sorted string.", py::arg("index"))
            .def("get_number_of_non_letter_chars", &DataStats::GetNumberOfNonLetterChars,
                 "Returns number of non-letter chars in a string column.", py::arg("index"))
            .def("get_number_of_digit_chars", &DataStats::GetNumberOfDigitChars,
                 "Returns number of digit chars in a string column.", py::arg("index"))
            .def("get_number_of_lowercase_chars", &DataStats::GetNumberOfLowercaseChars,
                 "Returns number of lowercase chars in a string column.", py::arg("index"))
            .def("get_number_of_uppercase_chars", &DataStats::GetNumberOfUppercaseChars,
                 "Returns number of uppercase chars in a string column.", py::arg("index"))
            .def("get_number_of_chars", &DataStats::GetNumberOfChars,
                 "Returns total number of characters in a string column.", py::arg("index"))
            .def("get_avg_number_of_chars", &DataStats::GetAvgNumberOfChars,
                 "Returns average number of chars in a string column.", py::arg("index"));
}
}  // namespace python_bindings
