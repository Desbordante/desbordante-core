#include "CsvStats.h"

#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/thread.hpp>
#include <iostream>

namespace algos {

namespace fs = std::filesystem;
namespace mo = model;

static inline std::vector<mo::TypedColumnData> CreateColumnData(const FDAlgorithm::Config& config) {
    CSVParser input_generator(config.data, config.separator, config.has_header);
    std::unique_ptr<model::ColumnLayoutTypedRelationData> relation_data =
        model::ColumnLayoutTypedRelationData::CreateFrom(input_generator, config.is_null_equal_null,
                                                         -1, -1);
    std::vector<mo::TypedColumnData> col_data = std::move(relation_data->GetColumnData());
    return col_data;
}

CsvStats::CsvStats(const FDAlgorithm::Config& config)
    : Primitive(config.data, config.separator, config.has_header, {}),
      config_(config),
      col_data_(CreateColumnData(config)),
      all_stats(col_data_.size()),
      threads_num(config_.parallelism) {}

static bool inline isComparable(const mo::TypeId& type_id) {
    return !(type_id == +mo::TypeId::kEmpty || type_id == +mo::TypeId::kNull ||
             type_id == +mo::TypeId::kUndefined || type_id == +mo::TypeId::kMixed);
}

Statistic CsvStats::GetMin(unsigned index) {
    if (all_stats[index].min.HasValue()) return all_stats[index].min;
    const auto& col = col_data_[index];
    if (!isComparable(col.GetTypeId())) return {};
    const mo::Type& type = col.GetType();
    const auto& data = col.GetData();
    const std::byte* min = nullptr;
    bool isFirstNotEmpty = true;
    for (unsigned i = 0; i < data.size(); ++i) {
        if (!col.IsNullOrEmpty(i)) {
            if (!isFirstNotEmpty) {
                if (type.Compare(data[i], min) == mo::CompareResult::kLess) min = data[i];
            } else {
                min = data[i];  // for first init of min with elem of data
                isFirstNotEmpty = false;
            }
        }
    }
    return Statistic(min, &type);
}

Statistic CsvStats::GetMax(unsigned index) {
    if (all_stats[index].max.HasValue()) return all_stats[index].max;
    const auto& col = col_data_[index];
    if (!isComparable(col.GetTypeId())) return {};
    const mo::Type& type = col.GetType();
    const auto& data = col.GetData();
    const std::byte* max = nullptr;
    bool isFirstNotEmpty = true;
    for (unsigned i = 0; i < data.size(); ++i) {
        if (!col.IsNullOrEmpty(i)) {
            if (!isFirstNotEmpty) {
                if (type.Compare(data[i], max) == mo::CompareResult::kGreater) max = data[i];
            } else {
                max = data[i];  // for first init of max with elem of data
                isFirstNotEmpty = false;
            }
        }
    }
    return Statistic(max, &col.GetType());
}

Statistic CsvStats::GetSum(unsigned index) {
    if (all_stats[index].sum.HasValue()) return all_stats[index].sum;
    const auto& col = col_data_[index];
    if (!col.IsNumeric()) return {};
    const auto& data = col.GetData();
    const auto& type = static_cast<const mo::INumericType&>(col.GetType());
    std::byte* sum(type.Allocate());
    for (unsigned i = 0; i < data.size(); ++i) {
        if (!col.IsNullOrEmpty(i)) type.Add(sum, data[i], sum);
    }
    return Statistic(sum, &type, false);
};

Statistic CsvStats::GetAvg(unsigned index) {
    if (all_stats[index].avg.HasValue()) return all_stats[index].avg;
    const auto& col = col_data_[index];
    if (!col.IsNumeric()) return {};
    mo::DoubleType double_type;
    const auto& col_type = static_cast<const mo::INumericType&>(col.GetType());
    Statistic sum_stat = GetSum(index);
    bool is_double_type = col.GetType() == mo::DoubleType();
    const std::byte* double_sum =
        is_double_type ? sum_stat.GetData()
                       : double_type.MakeValue(col_type.GetValue<mo::Int>(sum_stat.GetData()));
    std::byte* avg = double_type.Allocate();
    const std::byte* count_of_nums = double_type.MakeValue(this->Count(index));
    double_type.Div(double_sum, count_of_nums, avg);
    if (!is_double_type) double_type.Free(double_sum);
    double_type.Free(count_of_nums);
    return Statistic(avg, &double_type, false);
}

Statistic CsvStats::STDAndCentralMoment(unsigned index, int number, bool is_for_STD) {
    const auto& col = col_data_[index];
    if (!col.IsNumeric()) return {};
    const auto& data = col.GetData();
    mo::DoubleType double_type;
    const auto& col_type = static_cast<const mo::INumericType&>(col.GetType());
    Statistic avg = GetAvg(index);
    std::byte* neg_avg = double_type.Allocate();
    double_type.Negate(avg.GetData(), neg_avg);
    std::byte* sum_of_difs = double_type.Allocate();
    std::byte* dif = double_type.Allocate();
    bool is_double_type = col.GetType() == double_type;
    for (unsigned i = 0; i < data.size(); ++i) {
        if (!col.IsNullOrEmpty(i)) {
            const std::byte* double_num =
                is_double_type ? data[i]
                               : double_type.MakeValue(col_type.GetValue<mo::Int>(data[i]));
            double_type.Add(double_num, neg_avg, dif);
            double_type.Power(dif, number, dif);
            double_type.Add(sum_of_difs, dif, sum_of_difs);
            if (!is_double_type) double_type.Free(double_num);
        }
    }
    std::byte* count_of_nums = double_type.MakeValue(this->Count(index) - (is_for_STD ? 1 : 0));
    std::byte* result = double_type.Allocate();
    double_type.Div(sum_of_difs, count_of_nums, result);
    if (is_for_STD) double_type.Power(result, 0.5, result);
    double_type.Free(neg_avg);
    double_type.Free(sum_of_difs);
    double_type.Free(dif);
    double_type.Free(count_of_nums);
    return Statistic(result, &double_type, false);
}

// This is corrected standard deviation!
Statistic CsvStats::GetSTD(unsigned index) {
    return STDAndCentralMoment(index, 2, true);
}

Statistic CsvStats::CountCentralMomentOfDist(unsigned index, int number) {
    return STDAndCentralMoment(index, number, false);
}

Statistic CsvStats::CountStandardizedCentralMomentOfDist(unsigned index, int number) {
    mo::DoubleType double_type;
    Statistic std = GetSTD(index);
    Statistic central_moment = CountCentralMomentOfDist(index, number);
    if (!central_moment.HasValue() || !std.HasValue()) return {};
    std::byte* result(double_type.Allocate());
    double_type.Power(std.GetData(), number, result);
    double_type.Div(central_moment.GetData(), result, result);
    return Statistic(result, &double_type, false);
}

Statistic CsvStats::GetSkewness(unsigned index) {
    if (all_stats[index].skewness.HasValue()) return all_stats[index].skewness;
    const auto& col = col_data_[index];
    if (!col.IsNumeric()) return {};
    return CountStandardizedCentralMomentOfDist(index, 3);
}

Statistic CsvStats::GetKurtosis(unsigned index) {
    if (all_stats[index].kurtosis.HasValue()) return all_stats[index].kurtosis;
    const auto& col = col_data_[index];
    if (!col.IsNumeric()) return {};
    return CountStandardizedCentralMomentOfDist(index, 4);
}

int CsvStats::Count(unsigned index) {
    const auto& col = col_data_[index];
    return col.GetNumRows() - col.GetNumNulls() - col.GetNumEmpties();
};

int CsvStats::Distinct(unsigned index) {
    if (all_stats[index].isDistinctCorrect) return all_stats[index].distinct;
    const auto& col = col_data_[index];
    if (!isComparable(col.GetTypeId())) return {};
    std::unordered_set<std::byte> set;
    const auto& data = col.GetData();
    for (unsigned i = 0; i < data.size(); ++i) {
        if (!col.IsNullOrEmpty(i)) set.insert(*data[i]);
    }
    all_stats[index].distinct = set.size();
    all_stats[index].isDistinctCorrect = true;
    return set.size();
}

void CsvStats::ShowSample(unsigned start_row, unsigned end_row, unsigned start_col,
                          unsigned end_col, unsigned str_len, unsigned int_len,
                          unsigned double_len) {
    // int quantity_of_headers = end_col - start_col + 1;
    for (unsigned i = start_row - 1; i <= end_row - 1; ++i) {
        for (unsigned j = start_col - 1; j <= end_col - 1; ++j) {
            const auto& col = col_data_[j];
            const auto& data = col.GetData();
            const auto& type = col.GetType();
            switch (type.GetTypeId()) {
            case mo::TypeId::kString:
                std::cout << std::setw(str_len);
                if (!col.IsNullOrEmpty(i)) type.Print(data[i], std::cout);
                std::cout << "";
                break;
            case mo::TypeId::kInt:
                std::cout << std::setw(int_len);
                if (!col.IsNullOrEmpty(i)) type.Print(data[i], std::cout);
                std::cout << "";
                break;
            case mo::TypeId::kDouble:
                std::cout << std::setw(double_len);
                if (!col.IsNullOrEmpty(i)) type.Print(data[i], std::cout);
                std::cout << "";
                break;
            case mo::TypeId::kUndefined:
                if (col.IsEmpty(i)) {
                    mo::EmptyType empty;
                    std::cout << std::setw(5);
                    empty.Print(data[i], std::cout);
                }
                if (col.IsNull(i)) {
                    mo::NullType null(true);
                    std::cout << std::setw(5);
                    null.Print(data[i], std::cout);
                }
                break;
            default:
                break;
            }
            std::cout << "\t";
        }
        std::cout << "\n";
    }
}

bool CsvStats::isCategorical(unsigned index, int quantity) {
    return this->Distinct(index) <= quantity;
}

std::vector<const std::byte*> CsvStats::DeleteNullAndEmpties(unsigned index) {
    const auto& col = col_data_[index];
    const auto& data = col.GetData();
    std::vector<const std::byte*> vec = std::move(data);
    for (unsigned i = 0; i < vec.size(); ++i) {
        if (col.IsNullOrEmpty(i)) {
            vec.erase(vec.begin() + i);
        }
    }
    return vec;
}

Statistic CsvStats::GetQuantile(double part, unsigned index) {
    const auto& col = col_data_[index];
    if (!isComparable(col.GetTypeId())) return {};
    const mo::Type& type = col.GetType();
    const auto& raw_data = col.GetData();
    std::vector<const std::byte*> data;
    data.reserve(raw_data.size());
    for (unsigned i = 0; i < raw_data.size(); ++i) {
        if (!col.IsNullOrEmpty(i)) data.push_back(raw_data[i]);
    }
    int quantile = data.size() * part;
    if (!all_stats[index].quantile25.HasValue()) {
        std::sort(data.begin(), data.end(), type.GetComparator());
        all_stats[index].quantile25 = Statistic(data[(int)(data.size() * 0.25)], &col.GetType());
        all_stats[index].quantile50 = Statistic(data[(int)(data.size() * 0.5)], &col.GetType());
        all_stats[index].quantile75 = Statistic(data[(int)(data.size() * 0.75)], &col.GetType());
        all_stats[index].min = Statistic(data[0], &col.GetType());
        all_stats[index].max = Statistic(data[data.size() - 1], &col.GetType());
    } else
        std::nth_element(data.begin(), data.begin() + quantile, data.end(), type.GetComparator());
    return Statistic(data[quantile], &col.GetType());
}

unsigned long long CsvStats::Execute() {
    auto start_time = std::chrono::system_clock::now();
    double percent_per_col = kTotalProgressPercent / all_stats.size();
    auto task = [percent_per_col, this](unsigned index) {
        all_stats[index].sum = GetSum(index);
        // will use all_stats[index].sum
        all_stats[index].avg = GetAvg(index);
        all_stats[index].count = Count(index);
        Distinct(index);
        // after distinct, for faster executing
        all_stats[index].isCategorical = isCategorical(index, all_stats[index].count - 1);
        all_stats[index].kurtosis = GetKurtosis(index);
        all_stats[index].skewness = GetSkewness(index);
        GetQuantile(0.25, index);
        all_stats[index].STD = GetSTD(index);
        AddProgress(percent_per_col);
    };

    if (threads_num > 1) {
        boost::asio::thread_pool pool(threads_num);
        for (unsigned i = 0; i < all_stats.size(); ++i)
            boost::asio::post(pool, [i, task]() { return task(i); });
        pool.join();
    } else {
        for (unsigned i = 0; i < all_stats.size(); ++i) task(i);
    }
    SetProgress(kTotalProgressPercent);
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

unsigned CsvStats::GetCountOfColumns() {
    return col_data_.size();
}

const ColumnStats& CsvStats::GetAllStats(unsigned index) const {
    return all_stats[index];
}

const std::vector<ColumnStats>& CsvStats::GetAllStats() const {
    return all_stats;
}

ColumnStats& CsvStats::GetAllStats(unsigned index) {
    return all_stats[index];
}

std::vector<ColumnStats>& CsvStats::GetAllStats() {
    return all_stats;
}

const std::vector<model::TypedColumnData>& CsvStats::GetData() const noexcept {
    return col_data_;
}

std::string CsvStats::ToString() {
    std::stringstream res;
    for (unsigned i = 0; i < GetCountOfColumns(); ++i) {
        res << "Column num = " << i << std::endl;
        res << all_stats[i].ToString() << std::endl;
    }
    return res.str();
}

}  // namespace algos
