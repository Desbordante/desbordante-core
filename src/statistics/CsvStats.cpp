#include "CsvStats.h"

#include <iostream>

namespace statistics {

namespace fs = std::filesystem;
namespace mo = model;

static inline std::vector<mo::TypedColumnData> CreateColumnData(const FDAlgorithm::Config& config) {
    CSVParser input_generator(config.data, config.separator, config.has_header);
    std::unique_ptr<model::ColumnLayoutTypedRelationData> relation_data =
        model::ColumnLayoutTypedRelationData::CreateFrom(input_generator, config.is_null_equal_null, -1, -1);
    std::vector<mo::TypedColumnData> col_data = std::move(relation_data->GetColumnData());
    return col_data;
}

CsvStats::CsvStats(FDAlgorithm::Config const& config)
        : Primitive(config.data, config.separator, config.has_header, {}),
          config_(config), col_data_(CreateColumnData(config)), 
          allStats(col_data_.size()) { Execute(); }

Statistic CsvStats::GetMin(unsigned index) {
    mo::TypedColumnData const& col = col_data_[index];
    mo::TypeId type_id = col.GetTypeId();
    if (type_id == +mo::TypeId::kEmpty || type_id == +mo::TypeId::kNull ||
        type_id == +mo::TypeId::kUndefined) {
        return {};
    }
    mo::Type const& type = static_cast<mo::Type const&>(col.GetType());
    auto data = col.GetData();
    std::byte const* min;

    int counter = 0;
    for (unsigned i = 0; i < data.size(); ++i) {
        if (!col.IsNullOrEmpty(i)) {
            if (counter == 0) {
                min = data[i];  // for first init of min with elem of data
                ++counter;
            }
            if (type.Compare(data[i], min) == mo::CompareResult::kLess) {
                min = data[i];
            }
        }
    }
    return Statistic(type.Clone(min), Statistic::getTypeClone(type, 
        config_.is_null_equal_null));
}

Statistic CsvStats::GetMax(unsigned index) {
    mo::TypedColumnData const& col = col_data_[index];
    mo::TypeId type_id = col.GetTypeId();
    if (type_id == +mo::TypeId::kEmpty || type_id == +mo::TypeId::kNull ||
        type_id == +mo::TypeId::kUndefined) {
        return {};
    }
    mo::Type const& type = static_cast<mo::Type const&>(col.GetType());
    auto data = col.GetData();
    std::byte const* max;

    int counter = 0;
    for (unsigned i = 0; i < data.size(); ++i) {
        if (!col.IsNullOrEmpty(i)) {
            if (counter == 0) {
                max = data[i];  // for first init of max with elem of data
                ++counter;
            }
            if (type.Compare(data[i], max) == mo::CompareResult::kGreater) {
                max = data[i];
            }
        }
    }
    return Statistic(type.Clone(max), Statistic::getTypeClone(col.GetType(), 
        config_.is_null_equal_null));
}

Statistic CsvStats::GetSum(unsigned index) {
    mo::TypedColumnData const& col = col_data_[index];
    mo::TypeId type_id = col.GetTypeId();
    if (type_id == +mo::TypeId::kEmpty || type_id == +mo::TypeId::kNull ||
        type_id == +mo::TypeId::kUndefined) {
        return {};
    }
    if (col.IsNumeric()) {
        auto data = col.GetData();
        mo::INumericType const& type = static_cast<mo::INumericType const&>(col.GetType());
        std::byte* sum(type.Allocate());
        for (unsigned i = 0; i < data.size(); ++i) {
            if (!col.IsNullOrEmpty(i)) {
                type.Add(sum, data[i], sum);
            }
        }
        return Statistic(sum, Statistic::getTypeClone(type, 
            config_.is_null_equal_null));
    }
    return {};
};

Statistic CsvStats::GetAvg(unsigned index) {
    mo::TypedColumnData const& col = col_data_[index];
    mo::TypeId type_id = col.GetTypeId();
    if (type_id == +mo::TypeId::kEmpty || type_id == +mo::TypeId::kNull ||
        type_id == +mo::TypeId::kUndefined) {
        return {};
    }
    if (col.IsNumeric()) {
        mo::DoubleType type;
        mo::INumericType const& colType = static_cast<mo::INumericType const&>(col.GetType());
        auto sumStat = GetSum(index);
        bool isDoubleType = col.GetType() == mo::DoubleType();
        std::byte const* left = isDoubleType ? sumStat.getData(): 
            type.MakeValue(colType.GetValue<mo::Int>(sumStat.getData()));
        std::byte* avg = type.Allocate();
        std::byte const* countOfNums = type.MakeValue(this->Count(index));
        type.Div(left, countOfNums, avg);

        sumStat.Free();
        if(!isDoubleType)
            type.Free(left);
        type.Free(countOfNums);

        return Statistic(avg, new mo::DoubleType());
    }
    return {};
}

Statistic CsvStats::STDAndCentrMomnt(unsigned index, int number, bool isForSTD) {
    mo::TypedColumnData const& col = col_data_[index];
    mo::TypeId type_id = col.GetTypeId();
    if (type_id == +mo::TypeId::kEmpty || type_id == +mo::TypeId::kNull ||
        type_id == +mo::TypeId::kUndefined) {
        return {};
    }
    if (col.IsNumeric()) {
        auto data = col.GetData();
        mo::DoubleType type;
        mo::INumericType const& colType = static_cast<mo::INumericType const&>(col.GetType());
        std::byte const* avg = GetAvg(index).getDataAndFree();
        std::byte* negAvg = type.Allocate();
        type.Negate(avg, negAvg);
        std::byte* sum = type.Allocate();
        std::byte* dif = type.Allocate();
        bool notDoubleType = col.GetType() != type;
        for (unsigned i = 0; i < data.size(); ++i) {
            if (!col.IsNullOrEmpty(i)) {
                const std::byte *tmp = notDoubleType ? 
                    type.MakeValue(colType.GetValue<mo::Int>(data[i])) : data[i];
                type.Add(tmp, negAvg, dif);
                type.Power(dif, number, dif);
                type.Add(sum, dif, sum);
                if (notDoubleType) {
                    type.Free(tmp);
                }
            }
        }
        std::byte* countOfNums = type.MakeValue(this->Count(index) - (isForSTD ? 1 : 0));
        std::byte* result = type.Allocate();
        type.Div(sum, countOfNums, result);
        if (isForSTD)
            type.Power(result, 0.5, result);

        type.Free(avg);
        type.Free(negAvg);
        type.Free(sum);
        type.Free(dif);
        type.Free(countOfNums);

        return Statistic(result, new mo::DoubleType());
    }
    return {};
}

// This is corrected standard deviation!
Statistic CsvStats::GetSTD(unsigned index) {
    return STDAndCentrMomnt(index, 2, true);
}

Statistic CsvStats::CountCentralMomentOfDist(
    unsigned index, int number) {
    return  STDAndCentrMomnt(index, number, false);
}

Statistic CsvStats::CountStandardizedCentralMomentOfDist(
    unsigned index, int number) {
    mo::DoubleType type;
    auto std = GetSTD(index);
    auto centralMoment = CountCentralMomentOfDist(index, number);
    if(!centralMoment.hasValue() || ! std.hasValue()) {
        std.Free();
        centralMoment.Free();
        return {};
    }
    std::byte const* left = centralMoment.getDataAndFree();
    std::byte const* right = std.getDataAndFree();
    std::byte* result(type.Allocate());
    type.Power(right, number, result);
    type.Div(left, result, result);

    type.Free(left);
    type.Free(right);

    return Statistic(result, new mo::DoubleType());
}

Statistic CsvStats::GetSkewness(unsigned index) {
    mo::TypedColumnData const& col = col_data_[index];
    mo::TypeId type_id = col.GetTypeId();
    if (type_id == +mo::TypeId::kEmpty || type_id == +mo::TypeId::kNull ||
        type_id == +mo::TypeId::kUndefined) {
        return {};
    }
    return CountStandardizedCentralMomentOfDist(index, 3);
}

Statistic CsvStats::GetKurtosis(unsigned index) {
    mo::TypedColumnData const& col = col_data_[index];
    mo::TypeId type_id = col.GetTypeId();
    if (type_id == +mo::TypeId::kEmpty || type_id == +mo::TypeId::kNull ||
        type_id == +mo::TypeId::kUndefined) {
        return {};
    }
    return CountStandardizedCentralMomentOfDist(index, 4);
}

int CsvStats::Count(unsigned index) {
    mo::TypedColumnData const& col = col_data_[index];
    return col.GetNumRows() - col.GetNumNulls() - col.GetNumEmpties();
};

int CsvStats::Distinct(unsigned index) {
    mo::TypedColumnData const& col = col_data_[index];
    mo::TypeId type_id = col.GetTypeId();
    if (type_id == +mo::TypeId::kEmpty || type_id == +mo::TypeId::kNull ||
        type_id == +mo::TypeId::kUndefined) {
        return {};
    }

    std::unordered_set<std::byte> set;
    auto data = col.GetData();

    for (unsigned i = 0; i < data.size(); ++i) {
        if (!col.IsNullOrEmpty(i)) {
            set.insert(*data[i]);
        }
    }
    return set.size();
}

void CsvStats::ShowSample(unsigned start_row, unsigned end_row, unsigned start_col, unsigned end_col, unsigned str_len,
                          unsigned int_len, unsigned double_len) {
    // int quantity_of_headers = end_col - start_col + 1;
    for (unsigned i = start_row - 1; i <= end_row - 1; ++i) {
        for (unsigned j = start_col - 1; j <= end_col - 1; ++j) {
            mo::TypedColumnData const& col = col_data_[j];

            mo::Type const& type = static_cast<mo::Type const&>(col.GetType());

            // mo::AbstractColumnData const& column = static_cast<mo::AbstractColumnData
            // const&>(col); if (quantity_of_headers != 0) {
            //     std::cout << std::setw(str_len) << column.GetColumn()->GetName();
            //     --quantity_of_headers;
            //     continue;
            // }

            switch (col.GetTypeId()) {
            case static_cast<mo::TypeId>(mo::TypeId::kString):
                std::cout << std::setw(str_len);
                if (!col.IsNullOrEmpty(i)) {
                    type.Print(col.GetData()[i], std::cout);
                }
                std::cout << "";
                break;
            case static_cast<mo::TypeId>(mo::TypeId::kInt):
                std::cout << std::setw(int_len);
                if (!col.IsNullOrEmpty(i)) {
                    type.Print(col.GetData()[i], std::cout);
                }
                std::cout << "";
                break;
            case static_cast<mo::TypeId>(mo::TypeId::kDouble):
                std::cout << std::setw(double_len);
                if (!col.IsNullOrEmpty(i)) {
                    type.Print(col.GetData()[i], std::cout);
                }
                std::cout << "";
                break;
            case static_cast<mo::TypeId>(mo::TypeId::kUndefined):
                if (col.IsEmpty(i)) {
                    mo::EmptyType empty;
                    std::cout << std::setw(5);
                    empty.Print(col.GetData()[i], std::cout);
                }
                if (col.IsNull(i)) {
                    mo::NullType null(true);
                    std::cout << std::setw(5);
                    null.Print(col.GetData()[i], std::cout);
                }
                break;

            default:
                break;
            }
            std::cout << "\t";
        }
        std::cout << std::endl;
    }
}

bool CsvStats::isCategorial(unsigned index, int quantity) {
    return this->Distinct(index) <= quantity;
}

std::vector<std::byte const*> CsvStats::DeleteNullAndEmpties(unsigned index) {
    mo::TypedColumnData const& col = col_data_[index];
    auto data = col.GetData();
    std::vector<std::byte const*> vec = std::move(data);
    for (unsigned i = 0; i < vec.size(); ++i) {
        if (col.IsNullOrEmpty(i)) {
            vec.erase(vec.begin() + i);
        }
    }
    return vec;
}

Statistic CsvStats::GetQuantile(double part, unsigned index) {
    mo::TypedColumnData const& col = col_data_[index];
    mo::TypeId type_id = col.GetTypeId();
    if (type_id == +mo::TypeId::kEmpty || type_id == +mo::TypeId::kNull ||
        type_id == +mo::TypeId::kUndefined) {
        return {};
    }

    mo::Type const& type = static_cast<mo::Type const&>(col.GetType());
    auto vec = col.GetData();
    std::vector<std::byte const*> data;
    for (unsigned i = 0; i < vec.size(); ++i) {
        if (!col.IsNullOrEmpty(i)) {
            data.insert(data.begin(), vec[i]);
        }
    }
    int quantile = data.size() * part;
    std::nth_element(data.begin(), data.begin() + quantile, data.end(), type.GetComparator());
    return Statistic(type.Clone(data[quantile]), Statistic::getTypeClone(col.GetType(), 
        config_.is_null_equal_null));
}

unsigned long long CsvStats::Execute() {
    for(unsigned i = 0; i < allStats.size(); ++i) {
        allStats[i].avg = GetAvg(i);
        allStats[i].min = GetMin(i);
        allStats[i].max = GetMax(i);
        allStats[i].sum = GetSum(i);
        allStats[i].count = Count(i);
        allStats[i].distinct = Distinct(i);
        allStats[i].kurtosis = GetKurtosis(i);
        allStats[i].isCategorical = isCategorial(i, allStats[i].count);
        allStats[i].skewness = GetSkewness(i);
        allStats[i].quantile25 = GetQuantile(0.25, i);
        allStats[i].quantile50 = GetQuantile(0.5, i);
        allStats[i].quantile75 = GetQuantile(0.75, i);
    }
    return 0;
}

CsvStats::~CsvStats() {
    for(auto &colStat : allStats) {
        colStat.Free();
    }
}

unsigned CsvStats::GetCountOfColumns() {
    return col_data_.size();
}

const ColumnStats& CsvStats::GetAllStats(unsigned index) const {
    return allStats[index];
}

const std::vector<ColumnStats>& CsvStats::GetAllStats() const {
    return allStats;
}

ColumnStats& CsvStats::GetAllStats(unsigned index) {
    return allStats[index];
}

std::vector<ColumnStats>& CsvStats::GetAllStats() {
    return allStats;
}

const std::vector<model::TypedColumnData> & CsvStats::GetData() const noexcept {
    return col_data_;
}

std::string CsvStats::toString() {
    std::stringstream res;
    for(unsigned i = 0; i < GetCountOfColumns(); ++i) {
        res << "Column num = " << i << std::endl;
        res << allStats[i].toString() << std::endl;
    }
    return res.str();
}

}
