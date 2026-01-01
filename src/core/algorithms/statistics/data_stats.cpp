#include "core/algorithms/statistics/data_stats.h"

#include <set>

#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/thread.hpp>

#include "core/config/equal_nulls/option.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/config/thread_number/option.h"

namespace algos {

namespace fs = std::filesystem;
namespace mo = model;

DataStats::DataStats() : Algorithm({"Calculating statistics"}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName(), config::kEqualNullsOpt.GetName()});
}

void DataStats::RegisterOptions() {
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));
    RegisterOption(config::kThreadNumberOpt(&threads_num_));
}

void DataStats::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::kThreadNumberOpt.GetName()});
}

void DataStats::ResetState() {
    all_stats_.assign(col_data_.size(), ColumnStats{});
}

Statistic DataStats::GetMin(size_t index, mo::CompareResult order) const {
    mo::TypedColumnData const& col = col_data_[index];
    if (!mo::Type::IsOrdered(col.GetTypeId())) return {};

    mo::Type const& type = col.GetType();
    std::vector<std::byte const*> const& data = col.GetData();
    std::byte const* result = nullptr;
    for (size_t i = 0; i < data.size(); ++i) {
        if (col.IsNullOrEmpty(i)) continue;
        if (result != nullptr) {
            if (type.Compare(data[i], result) == order) result = data[i];
        } else {
            result = data[i];
        }
    }
    return Statistic(result, &type, true);
}

Statistic DataStats::GetMax(size_t index) const {
    return GetMin(index, mo::CompareResult::kGreater);
}

Statistic DataStats::GetSum(size_t index) const {
    if (all_stats_[index].sum.HasValue()) return all_stats_[index].sum;
    mo::TypedColumnData const& col = col_data_[index];
    if (!col.IsNumeric()) return {};

    std::vector<std::byte const*> const& data = col.GetData();
    auto const& type = static_cast<mo::INumericType const&>(col.GetType());
    std::byte* sum(type.MakeValueOfInt(0));
    for (size_t i = 0; i < data.size(); ++i) {
        if (!col.IsNullOrEmpty(i)) type.Add(sum, data[i], sum);
    }
    return Statistic(sum, &type, false);
};

Statistic DataStats::GetAvg(size_t index) const {
    if (all_stats_[index].avg.HasValue()) return all_stats_[index].avg;
    mo::TypedColumnData const& col = col_data_[index];
    if (!col.IsNumeric()) return {};
    mo::DoubleType double_type;

    Statistic sum_stat = GetSum(index);
    std::byte* double_sum = mo::DoubleType::MakeFrom(sum_stat.GetData(), col.GetType());
    std::byte* avg = double_type.Allocate();
    std::byte const* count_of_nums = double_type.MakeValue(this->NumberOfValues(index));
    double_type.Div(double_sum, count_of_nums, avg);

    double_type.Free(double_sum);
    double_type.Free(count_of_nums);
    return Statistic(avg, &double_type, false);
}

Statistic DataStats::CalculateCentralMoment(size_t index, int number,
                                            bool bessel_correction) const {
    mo::TypedColumnData const& col = col_data_[index];
    if (!col.IsNumeric()) return {};
    std::vector<std::byte const*> const& data = col.GetData();
    mo::DoubleType double_type;

    Statistic avg = GetAvg(index);
    std::byte* neg_avg = double_type.Allocate();
    double_type.Negate(avg.GetData(), neg_avg);
    std::byte* sum_of_difs = double_type.MakeValueOfInt(0);
    std::byte* dif = double_type.Allocate();
    std::byte* double_num = double_type.Allocate();
    for (size_t i = 0; i < data.size(); ++i) {
        if (col.IsNullOrEmpty(i)) continue;
        mo::DoubleType::MakeFrom(data[i], col.GetType(), double_num);
        double_type.Add(double_num, neg_avg, dif);
        double_type.Power(dif, number, dif);
        double_type.Add(sum_of_difs, dif, sum_of_difs);
    }
    std::byte* count_of_nums =
            double_type.MakeValue(this->NumberOfValues(index) - (bessel_correction ? 1 : 0));
    std::byte* result = double_type.Allocate();
    double_type.Div(sum_of_difs, count_of_nums, result);

    double_type.Free(double_num);
    double_type.Free(neg_avg);
    double_type.Free(sum_of_difs);
    double_type.Free(dif);
    double_type.Free(count_of_nums);

    return Statistic(result, &double_type, false);
}

Statistic DataStats::GetCorrectedSTD(size_t index) const {
    if (!col_data_[index].IsNumeric()) return {};
    mo::DoubleType double_type;
    std::byte* result = double_type.Allocate();
    double_type.Power(CalculateCentralMoment(index, 2, true).GetData(), 0.5, result);
    return Statistic(result, &double_type, false);
}

Statistic DataStats::GetCentralMomentOfDist(size_t index, int number) const {
    return CalculateCentralMoment(index, number, false);
}

Statistic DataStats::GetStandardizedCentralMomentOfDist(size_t index, int number) const {
    mo::DoubleType double_type;
    Statistic std = GetCorrectedSTD(index);
    Statistic central_moment = GetCentralMomentOfDist(index, number);
    if (!central_moment.HasValue() || !std.HasValue()) return {};

    std::byte* result(double_type.Allocate());
    double_type.Power(std.GetData(), number, result);
    double_type.Div(central_moment.GetData(), result, result);
    return Statistic(result, &double_type, false);
}

Statistic DataStats::GetSkewness(size_t index) const {
    if (all_stats_[index].skewness.HasValue()) return all_stats_[index].skewness;
    mo::TypedColumnData const& col = col_data_[index];
    if (!col.IsNumeric()) return {};
    return GetStandardizedCentralMomentOfDist(index, 3);
}

Statistic DataStats::GetKurtosis(size_t index) const {
    if (all_stats_[index].kurtosis.HasValue()) return all_stats_[index].kurtosis;
    mo::TypedColumnData const& col = col_data_[index];
    if (!col.IsNumeric()) return {};
    Statistic result = GetStandardizedCentralMomentOfDist(index, 4);
    mo::DoubleType double_type;
    std::byte const* correction = double_type.MakeValue(-3);
    std::byte* result_with_correction = double_type.Allocate();
    double_type.Add(result.GetData(), correction, result_with_correction);
    double_type.Free(correction);
    return Statistic(result_with_correction, &double_type, false);
}

size_t DataStats::NumberOfValues(size_t index) const {
    mo::TypedColumnData const& col = col_data_[index];
    return col.GetNumRows() - col.GetNumNulls() - col.GetNumEmpties();
};

inline static size_t CountDistinctInSortedData(std::vector<std::byte const*> const& data,
                                               mo::Type const& type) {
    size_t distinct = data.size() == 0 ? 0 : 1;
    for (size_t i = 0; i + 1 < data.size(); ++i) {
        if (type.Compare(data[i], data[i + 1]) != model::CompareResult::kEqual) ++distinct;
    }
    return distinct;
}

size_t DataStats::MixedDistinct(size_t index) const {
    mo::TypedColumnData const& col = col_data_[index];
    std::vector<std::byte const*> const& data = col.GetData();
    mo::MixedType mixed_type(is_null_equal_null_);

    std::vector<std::vector<std::byte const*>> values_by_type_id(mo::TypeId::_size());

    for (size_t i = 0; i < data.size(); ++i) {
        if (col.IsNullOrEmpty(i)) continue;
        values_by_type_id[mixed_type.RetrieveTypeId(data[i])._to_index()].push_back(data[i]);
    }

    size_t result = 0;
    for (std::vector<std::byte const*>& type_values : values_by_type_id) {
        std::sort(type_values.begin(), type_values.end(), mixed_type.GetComparator());
        result += CountDistinctInSortedData(type_values, mixed_type);
    }
    return result;
}

size_t DataStats::Distinct(size_t index) {
    if (all_stats_[index].distinct != 0) return all_stats_[index].distinct;
    mo::TypedColumnData const& col = col_data_[index];
    if (col.GetTypeId() == +mo::TypeId::kMixed) {
        all_stats_[index].distinct = MixedDistinct(index);
        return all_stats_[index].distinct;
    }
    auto const& type = col.GetType();

    std::vector<std::byte const*> data = DeleteNullAndEmpties(index);
    std::sort(data.begin(), data.end(), type.GetComparator());
    return all_stats_[index].distinct = CountDistinctInSortedData(data, type);
}

std::vector<std::vector<std::string>> DataStats::ShowSample(size_t start_row, size_t end_row,
                                                            size_t start_col,
                                                            size_t end_col) const {
    std::vector<std::vector<std::string>> res(end_row - start_row + 1,
                                              std::vector<std::string>(end_col - start_col + 1));

    for (size_t j = start_col - 1; j < end_col; ++j) {
        mo::TypedColumnData const& col = col_data_[j];
        for (size_t i = start_row - 1; i < end_row; ++i) res[i][j] = col.GetDataAsString(i);
    }

    return res;
}

bool DataStats::IsCategorical(size_t index, size_t quantity) {
    return this->Distinct(index) <= quantity;
}

std::vector<std::byte const*> DataStats::DeleteNullAndEmpties(size_t index) const {
    mo::TypedColumnData const& col = col_data_[index];
    mo::TypeId type_id = col.GetTypeId();
    if (type_id == +mo::TypeId::kNull || type_id == +mo::TypeId::kEmpty ||
        type_id == +mo::TypeId::kUndefined)
        return {};
    std::vector<std::byte const*> const& data = col.GetData();
    std::vector<std::byte const*> res;
    res.reserve(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        if (!col.IsNullOrEmpty(i)) res.push_back(data[i]);
    }
    return res;
}

Statistic DataStats::GetQuantile(double part, size_t index, bool calc_all) {
    mo::TypedColumnData const& col = col_data_[index];
    if (!mo::Type::IsOrdered(col.GetTypeId())) return {};
    mo::Type const& type = col.GetType();
    std::vector<std::byte const*> data = DeleteNullAndEmpties(index);
    int quantile = data.size() * part;

    if (calc_all && !all_stats_[index].quantile25.HasValue()) {
        std::sort(data.begin(), data.end(), type.GetComparator());
        all_stats_[index].quantile25 =
                Statistic(data[(size_t)(data.size() * 0.25)], &col.GetType(), true);
        all_stats_[index].quantile50 =
                Statistic(data[(size_t)(data.size() * 0.5)], &col.GetType(), true);
        all_stats_[index].quantile75 =
                Statistic(data[(size_t)(data.size() * 0.75)], &col.GetType(), true);
        all_stats_[index].min = Statistic(data[0], &col.GetType(), true);
        all_stats_[index].max = Statistic(data.back(), &col.GetType(), true);
        all_stats_[index].distinct = CountDistinctInSortedData(data, type);
    } else {
        std::nth_element(data.begin(), data.begin() + quantile, data.end(), type.GetComparator());
    }

    return Statistic(data[quantile], &col.GetType(), true);
}

template <class Pred, class Data>
std::vector<size_t> DataStats::FilterIndices(Pred pred, Data const& data) const {
    std::vector<size_t> res;
    res.reserve(data.size());
    for (size_t i = 0; i < data.size(); i++) {
        if (pred(i)) res.push_back(i);
    }

    res.shrink_to_fit();

    return res;
}

template <class Pred, class Data>
size_t DataStats::CountIf(Pred pred, Data const& data) const {
    size_t count = 0;
    for (size_t i = 0; i < data.size(); i++) {
        if (pred(data[i])) count++;
    }

    return count;
}

Statistic DataStats::CountIfInBinaryRelationWithZero(size_t index, mo::CompareResult res) const {
    mo::TypedColumnData const& col = GetData()[index];
    if (!col.IsNumeric()) return {};

    auto const& type = static_cast<mo::INumericType const&>(col.GetType());
    std::byte* zero = type.MakeValueOfInt(0);
    mo::IntType int_type;
    std::vector<std::byte const*> const& data = col_data_[index].GetData();

    auto pred = [&zero, &type, &res](std::byte const* el) {
        return el && type.Compare(el, zero) == res;
    };

    size_t count = CountIf(pred, data);
    type.Free(zero);

    return Statistic(int_type.MakeValue(count), &int_type, false);
}

Statistic DataStats::GetNumberOfZeros(size_t index) const {
    if (all_stats_[index].num_zeros.HasValue()) return all_stats_[index].num_zeros;
    return CountIfInBinaryRelationWithZero(index, mo::CompareResult::kEqual);
}

Statistic DataStats::GetNumberOfNegatives(size_t index) const {
    if (all_stats_[index].num_negatives.HasValue()) return all_stats_[index].num_negatives;
    return CountIfInBinaryRelationWithZero(index, mo::CompareResult::kLess);
}

Statistic DataStats::GetSumOfSquares(size_t index) const {
    if (all_stats_[index].sum_of_squares.HasValue()) return all_stats_[index].sum_of_squares;
    mo::TypedColumnData const& col = col_data_[index];
    if (!col.IsNumeric()) return {};

    auto const& type = static_cast<mo::INumericType const&>(col.GetType());
    std::vector<std::byte const*> const& data = col.GetData();
    std::byte* res = type.MakeValueOfInt(0);
    std::byte* square = type.Allocate();

    for (size_t i = 0; i < data.size(); i++) {
        if (col.IsNullOrEmpty(i)) continue;
        type.Power(data[i], 2, square);
        type.Add(res, square, res);
    }

    type.Free(square);

    return Statistic(res, &type, false);
}

Statistic DataStats::GetGeometricMean(size_t index) const {
    if (all_stats_[index].geometric_mean.HasValue()) return all_stats_[index].geometric_mean;
    mo::TypedColumnData const& col = col_data_[index];
    if (!col.IsNumeric()) return {};

    auto const& type = static_cast<mo::INumericType const&>(col.GetType());
    std::vector<std::byte const*> const data = col.GetData();
    mo::DoubleType double_type;
    std::byte* res = double_type.MakeValueOfInt(1);
    std::byte* temp = double_type.Allocate();
    std::byte* zero = type.MakeValueOfInt(0);
    long double num_values_reciprocal = 1.0L / static_cast<long double>(NumberOfValues(index));

    for (size_t i = 0; i < data.size(); i++) {
        if (col.IsNullOrEmpty(i)) continue;
        if (type.Compare(data[i], zero) == mo::CompareResult::kLess) {
            double_type.Free(temp);
            double_type.Free(res);
            type.Free(zero);
            return {};
        }
        mo::DoubleType::MakeFrom(data[i], type, temp);
        double_type.Power(temp, num_values_reciprocal, temp);
        double_type.Mul(res, temp, res);
    }

    double_type.Free(temp);
    type.Free(zero);

    return Statistic(res, &double_type, false);
}

Statistic DataStats::GetMeanAD(size_t index) const {
    if (all_stats_[index].mean_ad.HasValue()) return all_stats_[index].mean_ad;
    mo::TypedColumnData const& col = col_data_[index];
    if (!col.IsNumeric()) return {};

    // Convert each summand to DoubleType
    auto const& col_type = static_cast<mo::INumericType const&>(col.GetType());
    std::vector<std::byte const*> const data = col.GetData();
    mo::DoubleType double_type;
    std::byte* difference = double_type.MakeValue(0);  // data[i] - comparable
    std::byte* temp = double_type.Allocate();          // For converting data[i] to double
    std::byte* res = double_type.MakeValue(0);
    Statistic avg_stat = GetAvg(index);
    std::byte const* comparable = mo::DoubleType::MakeFrom(avg_stat.GetData(), *avg_stat.GetType());

    // Calculating the sum of |data[i] - comparable|
    for (size_t i = 0; i < data.size(); i++) {
        if (col.IsNullOrEmpty(i)) continue;

        mo::DoubleType::MakeFrom(data[i], col_type, temp);
        double_type.Sub(temp, comparable, difference);
        double_type.Abs(difference, difference);  // |data[i] - comparable|
        double_type.Add(res, difference, res);
    }

    size_t number_of_values = NumberOfValues(index);
    std::byte* num = double_type.MakeValue(number_of_values);
    double_type.Div(res, num, res);

    double_type.Free(temp);
    double_type.Free(num);
    double_type.Free(comparable);
    double_type.Free(difference);

    return Statistic(res, &double_type, false);
}

std::byte* DataStats::MedianOfNumericVector(std::vector<std::byte const*> const& data,
                                            mo::INumericType const& type) {
    std::vector<std::byte const*> data_copy = data;

    size_t size = data_copy.size();
    if (data_copy.empty()) return nullptr;

    auto mid = data_copy.begin() + size / 2;

    std::nth_element(data_copy.begin(), mid, data_copy.end(), type.GetComparator());

    if (size % 2 != 0) {
        return mo::DoubleType::MakeFrom(*mid, type);
    } else {
        mo::DoubleType double_type;
        std::byte* two = double_type.MakeValue(2);
        std::byte* res = type.Clone(*mid);
        std::byte* temp;
        auto prev_mid = std::prev(mid);

        std::nth_element(data_copy.begin(), prev_mid, data_copy.end(), type.GetComparator());
        type.Add(*prev_mid, res, res);
        temp = res;
        res = mo::DoubleType::MakeFrom(res, type);
        double_type.Div(res, two, res);

        double_type.Free(two);
        type.Free(temp);

        return res;
    }
}

Statistic DataStats::GetMedian(size_t index) const {
    if (all_stats_[index].median.HasValue()) return all_stats_[index].median;
    mo::TypedColumnData const& col = col_data_[index];
    if (!col.IsNumeric()) return {};

    auto const& type = static_cast<mo::INumericType const&>(col.GetType());
    std::vector<std::byte const*> data = DeleteNullAndEmpties(index);
    mo::DoubleType double_type;
    std::byte* median = MedianOfNumericVector(data, type);

    return Statistic(median, &double_type, false);
}

Statistic DataStats::GetMedianAD(size_t index) const {
    if (all_stats_[index].median_ad.HasValue()) {
        return all_stats_[index].median_ad;
    }
    mo::TypedColumnData const& col = col_data_[index];
    if (!col.IsNumeric()) return {};
    auto const& type = static_cast<mo::INumericType const&>(col.GetType());

    std::vector<std::byte const*> data = DeleteNullAndEmpties(index);
    std::byte* median = MedianOfNumericVector(data, type);
    mo::DoubleType double_type;
    std::byte* temp;

    std::vector<std::byte const*> res;
    res.reserve(data.size());

    for (std::byte const* x : data) {
        temp = mo::DoubleType::MakeFrom(x, col.GetType());
        double_type.Sub(temp, median, temp);
        double_type.Abs(temp, temp);
        res.push_back(temp);
    }

    std::byte* median_ad = MedianOfNumericVector(res, double_type);
    double_type.DeallocateContainer(res);
    double_type.Free(median);

    return Statistic(median_ad, &double_type, false);
}

Statistic DataStats::GetVocab(size_t index) const {
    if (all_stats_[index].vocab.HasValue()) return all_stats_[index].vocab;
    mo::TypedColumnData const& col = col_data_[index];
    if (col.GetTypeId() != +mo::TypeId::kString) return {};

    mo::StringType string_type;
    std::string string_data;
    std::set<char> vocab;

    for (size_t i = 0; i < col.GetNumRows(); i++) {
        if (col.IsNullOrEmpty(i)) continue;
        auto const& string_data = mo::Type::GetValue<std::string>(col.GetValue(i));
        vocab.insert(string_data.begin(), string_data.end());
    }
    std::string temp(vocab.begin(), vocab.end());

    std::byte const* res = string_type.MakeValue(temp);

    return Statistic(res, &string_type, false);
}

Statistic DataStats::GetNumberOfNonLetterChars(size_t index) const {
    if (all_stats_[index].num_non_letter_chars.HasValue())
        return all_stats_[index].num_non_letter_chars;

    auto pred = [](unsigned int symbol) { return !std::isalpha(symbol); };

    return CountIfInColumn(pred, index);
}

Statistic DataStats::GetNumberOfDigitChars(size_t index) const {
    if (all_stats_[index].num_digit_chars.HasValue()) return all_stats_[index].num_digit_chars;

    auto pred = [](unsigned char symbol) { return std::isdigit(symbol); };

    return CountIfInColumn(pred, index);
}

Statistic DataStats::GetNumberOfLowercaseChars(size_t index) const {
    if (all_stats_[index].num_lowercase_chars.HasValue())
        return all_stats_[index].num_lowercase_chars;

    auto pred = [](int symbol) { return std::islower(symbol); };

    return CountIfInColumn(pred, index);
}

Statistic DataStats::GetNumberOfUppercaseChars(size_t index) const {
    if (all_stats_[index].num_uppercase_chars.HasValue())
        return all_stats_[index].num_uppercase_chars;

    auto pred = [](int symbol) { return std::isupper(symbol); };

    return CountIfInColumn(pred, index);
}

template <class Pred>
Statistic DataStats::CountIfInColumn(Pred pred, size_t index) const {
    mo::TypedColumnData const& col = col_data_[index];
    if (col.GetTypeId() != +mo::TypeId::kString) return {};

    size_t count = 0;
    mo::IntType int_type;

    for (size_t i = 0; i < col.GetNumRows(); i++) {
        if (col.IsNullOrEmpty(i)) continue;
        auto const& string_data = mo::Type::GetValue<std::string>(col.GetValue(i));
        for (size_t j = 0; j < string_data.size(); j++)
            if (pred(string_data[j])) count++;
    }

    std::byte const* res = int_type.MakeValue(count);

    return Statistic(res, &int_type, false);
}

Statistic DataStats::GetNumberOfChars(size_t index) const {
    mo::TypedColumnData const& col = col_data_[index];
    if (col.GetTypeId() != +mo::TypeId::kString) return {};

    return GetStringSumOf(index, [](std::string const& line) { return line.size(); });
}

Statistic DataStats::GetAvgNumberOfChars(size_t index) const {
    if (all_stats_[index].num_avg_chars.HasValue()) return all_stats_[index].num_avg_chars;

    mo::TypedColumnData const& col = col_data_[index];
    if (col.GetTypeId() != +mo::TypeId::kString) return {};

    mo::DoubleType double_type;
    mo::IntType int_type;
    std::byte* res = double_type.Allocate();
    Statistic num_stat = GetNumberOfChars(index);
    std::byte const* num = num_stat.GetData();
    std::byte const* count = double_type.MakeValueOfInt(col.GetNumRows() - col.GetNumNulls());

    num = double_type.MakeFrom(num, int_type);
    double_type.Div(num, count, res);

    double_type.Free(num);
    double_type.Free(count);

    return Statistic(res, &double_type, false);
}

template <class Pred>
Statistic DataStats::GetStringMinOf(size_t index, Pred pred) const {
    mo::TypedColumnData const& col = col_data_[index];
    mo::IntType int_type;

    size_t result = std::numeric_limits<size_t>::max();
    for (size_t i = 0; i < col.GetNumRows(); i++) {
        if (col.IsNullOrEmpty(i)) continue;

        auto const& string_data = mo::Type::GetValue<std::string>(col.GetValue(i));
        size_t const& size = pred(string_data);

        if (size < result) result = size;
    }

    std::byte const* res = int_type.MakeValue(result);

    return Statistic(res, &int_type, false);
}

template <class Pred>
Statistic DataStats::GetStringMaxOf(size_t index, Pred pred) const {
    mo::TypedColumnData const& col = col_data_[index];
    mo::IntType int_type;

    size_t result = 0;
    for (size_t i = 0; i < col.GetNumRows(); i++) {
        if (col.IsNullOrEmpty(i)) continue;

        auto const& string_data = mo::Type::GetValue<std::string>(col.GetValue(i));
        size_t const& size = pred(string_data);

        if (size > result) result = size;
    }

    std::byte const* res = int_type.MakeValue(result);

    return Statistic(res, &int_type, false);
}

template <class Pred>
Statistic DataStats::GetStringSumOf(size_t index, Pred pred) const {
    mo::TypedColumnData const& col = col_data_[index];
    mo::IntType int_type;

    size_t result = 0;
    for (size_t i = 0; i < col.GetNumRows(); i++) {
        if (col.IsNullOrEmpty(i)) continue;

        auto const& string_data = mo::Type::GetValue<std::string>(col.GetValue(i));

        result += pred(string_data);
    }

    std::byte const* res = int_type.MakeValue(result);

    return Statistic(res, &int_type, false);
}

Statistic DataStats::GetMinNumberOfChars(size_t index) const {
    if (all_stats_[index].min_num_chars.HasValue()) return all_stats_[index].min_num_chars;
    mo::TypedColumnData const& col = col_data_[index];
    if (col.GetTypeId() != +mo::TypeId::kString) return {};

    return GetStringMinOf(index, [](std::string const& line) { return line.size(); });
}

Statistic DataStats::GetMaxNumberOfChars(size_t index) const {
    if (all_stats_[index].max_num_chars.HasValue()) return all_stats_[index].max_num_chars;
    mo::TypedColumnData const& col = col_data_[index];
    if (col.GetTypeId() != +mo::TypeId::kString) return {};

    return GetStringMaxOf(index, [](std::string const& line) { return line.size(); });
}

std::vector<std::string> DataStats::GetWordsInString(std::string line) {
    std::istringstream iss(line);
    std::vector<std::string> words_in_row(std::istream_iterator<std::string>{iss},
                                          std::istream_iterator<std::string>());
    return words_in_row;
}

std::set<std::string> DataStats::GetWords(size_t index) const {
    mo::TypedColumnData const& col = col_data_[index];
    if (col.GetTypeId() != +mo::TypeId::kString) return {};

    mo::StringType string_type;
    std::set<std::string> words;

    for (size_t i = 0; i < col.GetNumRows(); i++) {
        if (col.IsNullOrEmpty(i)) continue;
        std::vector<std::string> words_in_row =
                GetWordsInString(mo::Type::GetValue<std::string>(col.GetValue(i)));
        words.insert(words_in_row.begin(), words_in_row.end());
    }

    return words;
}

size_t DataStats::GetNumberOfWordsInString(std::string line) {
    size_t count = 0;

    for (size_t i = 0; i < line.size(); i++) {
        if (!isspace(line[i]) && (i == 0 || (i > 0 && isspace(line[i - 1])))) count++;
    }

    return count;
}

Statistic DataStats::GetMinNumberOfWords(size_t index) const {
    if (all_stats_[index].min_num_words.HasValue()) return all_stats_[index].min_num_words;

    mo::TypedColumnData const& col = col_data_[index];
    if (col.GetTypeId() != +mo::TypeId::kString) return {};

    return GetStringMinOf(index,
                          [](std::string const& line) { return GetNumberOfWordsInString(line); });
}

Statistic DataStats::GetMaxNumberOfWords(size_t index) const {
    if (all_stats_[index].max_num_words.HasValue()) return all_stats_[index].max_num_words;
    mo::TypedColumnData const& col = col_data_[index];
    if (col.GetTypeId() != +mo::TypeId::kString) return {};

    return GetStringMaxOf(index,
                          [](std::string const& line) { return GetNumberOfWordsInString(line); });
}

Statistic DataStats::GetNumberOfWords(size_t index) const {
    if (all_stats_[index].num_words.HasValue()) return all_stats_[index].num_words;
    mo::TypedColumnData const& col = col_data_[index];
    if (col.GetTypeId() != +mo::TypeId::kString) return {};

    return GetStringSumOf(index,
                          [](std::string const& line) { return GetNumberOfWordsInString(line); });
}

std::vector<char> DataStats::GetTopKChars(size_t index, size_t k) const {
    mo::TypedColumnData const& col = col_data_[index];
    if (col.GetTypeId() != +mo::TypeId::kString) return {};

    mo::StringType string_type;
    std::unordered_map<char, size_t> count_chars;

    for (size_t i = 0; i < col.GetNumRows(); i++) {
        if (col.IsNullOrEmpty(i)) continue;
        auto const& string_data = mo::Type::GetValue<std::string>(col.GetValue(i));
        for (char const symbol : string_data) {
            if (count_chars.find(symbol) != count_chars.end()) {
                count_chars[symbol]++;
            } else {
                count_chars[symbol] = 1;
            }
        }
    }

    std::vector<std::pair<char, size_t>> sorted_symbols(count_chars.begin(), count_chars.end());
    std::sort(sorted_symbols.begin(), sorted_symbols.end(),
              [](std::pair<char, size_t> const& a, std::pair<char, size_t> const& b) {
                  return a.second > b.second;
              });

    sorted_symbols.resize(k, {' ', 0});

    std::vector<char> res;
    res.reserve(k);

    for (auto const& pair : sorted_symbols) res.push_back(pair.first);

    return res;
}

std::vector<std::string> DataStats::GetTopKWords(size_t index, size_t k) const {
    mo::TypedColumnData const& col = col_data_[index];
    if (col.GetTypeId() != +mo::TypeId::kString) return {};

    mo::StringType string_type;
    std::unordered_map<std::string, size_t> count_words;

    for (size_t i = 0; i < col.GetNumRows(); i++) {
        if (col.IsNullOrEmpty(i)) continue;
        std::vector<std::string> words_in_row =
                GetWordsInString(mo::Type::GetValue<std::string>(col.GetValue(i)));
        for (std::string const& word : words_in_row) {
            if (count_words.find(word) != count_words.end()) {
                count_words[word]++;
            } else {
                count_words[word] = 1;
            }
        }
    }

    std::vector<std::pair<std::string, size_t>> sorted_words(count_words.begin(),
                                                             count_words.end());
    std::sort(sorted_words.begin(), sorted_words.end(),
              [](std::pair<std::string, size_t> const& a, std::pair<std::string, size_t> const& b) {
                  return a.second > b.second;
              });

    sorted_words.resize(k, {" ", 0});

    std::vector<std::string> res;
    res.reserve(k);

    for (auto const& pair : sorted_words) res.push_back(pair.first);

    return res;
}

Statistic DataStats::GetNumberOfEntirelyUppercaseWords(size_t index) const {
    if (all_stats_[index].num_entirely_uppercase.HasValue())
        return all_stats_[index].num_entirely_uppercase;

    auto pred = [&](std::string word) { return IsEntirelyUppercase(word); };

    return CountIfInColumnForWords(pred, index);
}

Statistic DataStats::GetNumberOfEntirelyLowercaseWords(size_t index) const {
    if (all_stats_[index].num_entirely_lowercase.HasValue())
        return all_stats_[index].num_entirely_lowercase;

    auto pred = [&](std::string word) { return IsEntirelyLowercase(word); };

    return CountIfInColumnForWords(pred, index);
}

bool DataStats::IsEntirelyUppercase(std::string word) {
    for (size_t i = 0; i < word.size(); i++) {
        if (std::isalpha(word[i])) {
            if (!std::isupper(word[i])) return false;
        }
    }

    return true;
}

bool DataStats::IsEntirelyLowercase(std::string word) {
    for (size_t i = 0; i < word.size(); i++) {
        if (std::isalpha(word[i])) {
            if (!std::islower(word[i])) return false;
        }
    }

    return true;
}

template <class Pred>
Statistic DataStats::CountIfInColumnForWords(Pred pred, size_t index) const {
    mo::TypedColumnData const& col = col_data_[index];
    if (col.GetTypeId() != +mo::TypeId::kString) return {};

    std::size_t count = 0;
    std::string string_data;
    mo::IntType int_type;

    for (size_t i = 0; i < col.GetNumRows(); i++) {
        if (col.IsNullOrEmpty(i)) continue;
        std::vector<std::string> words_in_row =
                GetWordsInString(mo::Type::GetValue<std::string>(col.GetValue(i)));
        for (size_t j = 0; j < words_in_row.size(); j++)
            if (pred(words_in_row[j])) count++;
    }

    std::byte const* res = int_type.MakeValue(count);

    return Statistic(res, &int_type, false);
}

unsigned long long DataStats::ExecuteInternal() {
    if (all_stats_.empty()) {
        // Table has 0 columns, nothing to do
        return 0;
    }

    auto start_time = std::chrono::system_clock::now();
    double percent_per_col = kTotalProgressPercent / all_stats_.size();
    
    auto task = [percent_per_col, this](size_t index) {
        all_stats_[index].count = NumberOfValues(index);
        
        if (this->col_data_[index].GetTypeId() != +mo::TypeId::kMixed) {
            all_stats_[index].min = GetMin(index);
            all_stats_[index].max = GetMax(index);
            all_stats_[index].sum = GetSum(index);
            // will use all_stats_[index].sum
            all_stats_[index].avg = GetAvg(index);

            GetQuantile(0.25, index, true);  // distinct is calculated here
            // after distinct, for faster executing
            all_stats_[index].kurtosis = GetKurtosis(index);
            all_stats_[index].skewness = GetSkewness(index);
            all_stats_[index].STD = GetCorrectedSTD(index);
            all_stats_[index].num_zeros = GetNumberOfZeros(index);
            all_stats_[index].num_negatives = GetNumberOfNegatives(index);
            all_stats_[index].sum_of_squares = GetSumOfSquares(index);
            all_stats_[index].geometric_mean = GetGeometricMean(index);
            all_stats_[index].mean_ad = GetMeanAD(index);
            all_stats_[index].median = GetMedian(index);
            all_stats_[index].median_ad = GetMedianAD(index);

            all_stats_[index].monotonicity = GetMonotonicity(index);
            
            if (this->col_data_[index].GetTypeId() == +mo::TypeId::kString) {
                all_stats_[index].vocab = GetVocab(index);
                all_stats_[index].num_non_letter_chars = GetNumberOfNonLetterChars(index);
                all_stats_[index].num_digit_chars = GetNumberOfDigitChars(index);
                all_stats_[index].num_lowercase_chars = GetNumberOfLowercaseChars(index);
                all_stats_[index].num_uppercase_chars = GetNumberOfUppercaseChars(index);
                all_stats_[index].num_chars = GetNumberOfChars(index);
                all_stats_[index].num_avg_chars = GetAvgNumberOfChars(index);
                all_stats_[index].min_num_chars = GetMinNumberOfChars(index);
                all_stats_[index].max_num_chars = GetMaxNumberOfChars(index);
                all_stats_[index].min_num_words = GetMinNumberOfWords(index);
                all_stats_[index].max_num_words = GetMaxNumberOfWords(index);
                all_stats_[index].num_words = GetNumberOfWords(index);
                all_stats_[index].num_entirely_uppercase = GetNumberOfEntirelyUppercaseWords(index);
                all_stats_[index].num_entirely_lowercase = GetNumberOfEntirelyLowercaseWords(index);
            }
            
            if (this->col_data_[index].IsNumeric()) {
                all_stats_[index].interquartile_range = GetInterquartileRange(index);
                all_stats_[index].coefficient_of_variation = GetCoefficientOfVariation(index);
            }
        }
        
        all_stats_[index].is_categorical = IsCategorical(
            index, std::min(all_stats_[index].count - 1, 10 + all_stats_[index].count / 1000)
        );
        all_stats_[index].type = this->col_data_[index].GetType().ToString().substr(1);
        AddProgress(percent_per_col);
    };

    if (threads_num_ > 1) {
        boost::asio::thread_pool pool(threads_num_);
        for (size_t i = 0; i < all_stats_.size(); ++i)
            boost::asio::post(pool, [i, task]() { return task(i); });
        pool.join();
    } else {
        for (size_t i = 0; i < all_stats_.size(); ++i) task(i);
    }

    SetProgress(kTotalProgressPercent);
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - start_time
    );
    return elapsed_milliseconds.count();
}

size_t DataStats::GetNumNulls(size_t index) const {
    mo::TypedColumnData const& col = col_data_[index];
    return col.GetNumNulls();
}

std::vector<size_t> DataStats::GetNullColumns() const {
    auto pred = [this, num_rows = col_data_[0].GetNumRows()](size_t index) {
        return col_data_[index].GetNumNulls() == num_rows;
    };

    return FilterIndices(pred, col_data_);
}

std::vector<size_t> DataStats::GetColumnsWithNull() const {
    auto pred = [this](size_t index) { return col_data_[index].GetNumNulls() != 0; };

    return FilterIndices(pred, col_data_);
}

std::vector<size_t> DataStats::GetColumnsWithUniqueValues() {
    auto pred = [this, num_rows = col_data_[0].GetNumRows()](size_t index) {
        return Distinct(index) == num_rows;
    };

    return FilterIndices(pred, col_data_);
}

size_t DataStats::GetNumberOfColumns() const {
    return col_data_.size();
}

ColumnStats const& DataStats::GetAllStats(size_t index) const {
    return all_stats_[index];
}

std::vector<ColumnStats> const& DataStats::GetAllStats() const {
    return all_stats_;
}

std::vector<model::TypedColumnData> const& DataStats::GetData() const noexcept {
    return col_data_;
}

std::string DataStats::ToString() const {
    std::stringstream res;
    for (size_t i = 0; i < GetNumberOfColumns(); ++i) {
        res << "Column num = " << i << '\n';
        res << all_stats_[i].ToString() << '\n';
    }
    return res.str();
}

void DataStats::LoadDataInternal() {
    col_data_ = mo::CreateTypedColumnData(*input_table_, is_null_equal_null_);
    all_stats_ = std::vector<ColumnStats>{col_data_.size()};
}


Statistic DataStats::GetInterquartileRange(size_t index) const {
    if (all_stats_[index].interquartile_range.HasValue()) 
        return all_stats_[index].interquartile_range;
    
    mo::TypedColumnData const& col = col_data_[index];
    if (!col.IsNumeric()) return {};
    
    Statistic q1 = all_stats_[index].quantile25;
    Statistic q3 = all_stats_[index].quantile75;
    
    if (!q1.HasValue() || !q3.HasValue()) {
        return {};
    }
    
    mo::DoubleType double_type;
    std::byte* q1_val = mo::DoubleType::MakeFrom(q1.GetData(), *q1.GetType());
    std::byte* q3_val = mo::DoubleType::MakeFrom(q3.GetData(), *q3.GetType());
    std::byte* iqr_val = double_type.Allocate();
    
    double_type.Sub(q3_val, q1_val, iqr_val);
    
    double_type.Free(q1_val);
    double_type.Free(q3_val);
    
    return Statistic(iqr_val, &double_type, false);
}

Statistic DataStats::GetCoefficientOfVariation(size_t index) const {
    if (all_stats_[index].coefficient_of_variation.HasValue()) 
        return all_stats_[index].coefficient_of_variation;
    
    mo::TypedColumnData const& col = col_data_[index];
    if (!col.IsNumeric()) return {};
    
    Statistic std_stat = GetCorrectedSTD(index);
    Statistic mean_stat = GetAvg(index);
    
    if (!std_stat.HasValue() || !mean_stat.HasValue()) return {};
    
    mo::DoubleType double_type;
    std::byte* std_val = mo::DoubleType::MakeFrom(std_stat.GetData(), *std_stat.GetType());
    std::byte* mean_val = mo::DoubleType::MakeFrom(mean_stat.GetData(), *mean_stat.GetType());
    
    std::byte* zero = double_type.MakeValue(0.0);
    if (double_type.Compare(mean_val, zero) == mo::CompareResult::kEqual) {
        double_type.Free(std_val);
        double_type.Free(mean_val);
        double_type.Free(zero);
        return {};
    }
    
    std::byte* cv_val = double_type.Allocate();
    double_type.Div(std_val, mean_val, cv_val);
    
    double_type.Free(std_val);
    double_type.Free(mean_val);
    double_type.Free(zero);
    
    return Statistic(cv_val, &double_type, false);
}

Statistic DataStats::GetMonotonicity(size_t index) const {
    if (all_stats_[index].monotonicity.HasValue()) 
        return all_stats_[index].monotonicity;
    
    mo::TypedColumnData const& col = col_data_[index];
    if (!mo::Type::IsOrdered(col.GetTypeId())) return {};
    
    std::vector<std::byte const*> data = DeleteNullAndEmpties(index);
    if (data.size() < 2) {
        mo::StringType string_type;
        return Statistic(string_type.MakeValue("none"), &string_type, false);
    }
    
    mo::Type const& type = col.GetType();
    bool ascending = true;
    bool descending = true;
    
    for (size_t i = 0; i < data.size() - 1; ++i) {
        mo::CompareResult cmp = type.Compare(data[i], data[i + 1]);
        if (cmp == mo::CompareResult::kGreater) descending = false;
        if (cmp == mo::CompareResult::kLess) ascending = false;
    }
    
    mo::StringType string_type;
    if (ascending && descending) {
        return Statistic(string_type.MakeValue("equal"), &string_type, false);
    } else if (ascending) {
        return Statistic(string_type.MakeValue("ascending"), &string_type, false);
    } else if (descending) {
        return Statistic(string_type.MakeValue("descending"), &string_type, false);
    } else {
        return Statistic(string_type.MakeValue("none"), &string_type, false);
    }
}

}  // namespace algos
