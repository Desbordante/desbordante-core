#include "CsvStats.h"

#include <math.h>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include "AbstractColumnData.h"
#include "BuiltIn.h"
#include "DoubleType.h"
#include "EmptyType.h"
#include "IntType.h"
#include "NullType.h"
#include "NumericType.h"
#include "StringType.h"
#include "Type.h"
#include "TypedColumnData.h"
#include "UndefinedType.h"

static inline std::vector<mo::TypedColumnData> CreateColumnData(std::string_view data, char sep,
                                                                bool has_header) {
    auto const path = fs::current_path() / "inputData" / data;
    CSVParser input_generator(path, sep, has_header);
    std::unique_ptr<model::ColumnLayoutTypedRelationData> relation_data =
        model::ColumnLayoutTypedRelationData::CreateFrom(input_generator, true, -1, -1);
    std::vector<mo::TypedColumnData> col_data = std::move(relation_data->GetColumnData());
    return col_data;
}

CsvStats::CsvStats(std::string_view data, char sep, bool has_header)
    : col_data_(CreateColumnData(data, sep, has_header)) {}

std::optional<std::pair<std::byte const*, mo::Type const*>> CsvStats::GetMin(int index) {
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
    return std::make_pair(min, &col.GetType());
}

std::optional<std::pair<std::byte const*, mo::Type const*>> CsvStats::GetMax(int index) {
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
    return std::make_pair(max, &col.GetType());
}

std::optional<std::pair<std::byte const*, mo::Type const*>> CsvStats::GetSum(int index) {
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
        return std::make_pair(sum, &col.GetType());
    }
    return {};
};

std::optional<std::pair<std::byte const*, mo::Type const*>> CsvStats::GetAvg(int index) {
    mo::TypedColumnData const& col = col_data_[index];
    mo::TypeId type_id = col.GetTypeId();
    if (type_id == +mo::TypeId::kEmpty || type_id == +mo::TypeId::kNull ||
        type_id == +mo::TypeId::kUndefined) {
        return {};
    }
    if (col.IsNumeric()) {
        mo::DoubleType type;
        mo::INumericType const& colType = static_cast<mo::INumericType const&>(col.GetType());
        std::byte const* sum = GetSum(index)->first;
        std::byte const* left;
        if (col.GetType() != type) {
            left = type.MakeValue(colType.GetValue<mo::Int>(sum));
        } else {
            left = sum;
            sum = nullptr;
        }
        std::byte* avg = type.Allocate();
        std::byte const* countOfNums = type.MakeValue(this->Count(index));
        type.Div(left, countOfNums, avg);

        type.Free(sum);
        type.Free(left);
        type.Free(countOfNums);

        return std::make_pair(avg, &type);
    }
    return {};
}

std::optional<std::pair<std::byte const*, mo::Type const*>> CsvStats::GetSTD(int index) {
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
        std::byte const* avg = GetAvg(index)->first;
        std::byte* negAvg = type.Allocate();
        type.Negate(avg, negAvg);
        std::byte* sum = type.Allocate();
        std::byte* dif = type.Allocate();
        std::byte* tmp;
        bool notDoubleType = col.GetType() != type;
        for (unsigned i = 0; i < data.size(); ++i) {
            if (!col.IsNullOrEmpty(i)) {
                if (notDoubleType) {
                    tmp = type.MakeValue(colType.GetValue<mo::Int>(data[i]));
                }
                type.Add(tmp, negAvg, dif);
                type.Power(dif, 2, dif);
                type.Add(sum, dif, sum);
                if (notDoubleType) {
                    type.Free(tmp);
                }
            }
        }
        std::byte* countOfNums = type.MakeValue(this->Count(index) - 1);
        std::byte* result = type.Allocate();
        type.Div(sum, countOfNums, result);
        type.Power(result, 0.5, result);

        type.Free(avg);
        type.Free(negAvg);
        type.Free(sum);
        type.Free(dif);
        type.Free(countOfNums);

        return std::make_pair(result, &type);
    }
    return {};
}

std::optional<std::pair<std::byte const*, mo::Type const*>> CsvStats::CountCentrlMomntOfDist(
    int index, int number) {
    mo::TypedColumnData const& col = col_data_[index];
    if (col.IsNumeric()) {
        auto data = col.GetData();

        mo::DoubleType type;
        mo::INumericType const& colType = static_cast<mo::INumericType const&>(col.GetType());
        std::byte const* avg = GetAvg(index)->first;
        std::byte* negAvg = type.Allocate();
        type.Negate(avg, negAvg);
        std::byte* sum = type.Allocate();
        std::byte* dif = type.Allocate();
        std::byte* tmp;
        bool notDoubleType = col.GetType() != type;
        for (unsigned i = 0; i < data.size(); ++i) {
            if (!col.IsNullOrEmpty(i)) {
                if (notDoubleType) {
                    tmp = type.MakeValue(colType.GetValue<mo::Int>(data[i]));
                }
                type.Add(tmp, negAvg, dif);
                type.Power(dif, number, dif);
                type.Add(sum, dif, sum);
                if (notDoubleType) {
                    type.Free(tmp);
                }
            }
        }
        std::byte* countOfNums = type.MakeValue(this->Count(index));
        std::byte* result = type.Allocate();
        type.Div(sum, countOfNums, result);

        type.Free(avg);
        type.Free(negAvg);
        type.Free(sum);
        type.Free(dif);
        type.Free(countOfNums);

        return std::make_pair(result, &type);
    }
    return {};
}

std::optional<std::pair<std::byte const*, mo::Type const*>> CsvStats::CountStndrtCentrlMomntOfDist(
    int index, int number) {
    mo::DoubleType type;
    std::byte const* left = CountCentrlMomntOfDist(index, number)->first;
    std::byte const* right = GetSTD(index)->first;
    std::byte* result(type.Allocate());
    type.Power(right, number, result);
    type.Div(left, result, result);

    type.Free(left);
    type.Free(right);

    return std::make_pair(result, &type);
}

std::optional<std::pair<std::byte const*, mo::Type const*>> CsvStats::GetSkewness(int index) {
    mo::TypedColumnData const& col = col_data_[index];
    mo::TypeId type_id = col.GetTypeId();
    if (type_id == +mo::TypeId::kEmpty || type_id == +mo::TypeId::kNull ||
        type_id == +mo::TypeId::kUndefined) {
        return {};
    }
    return CountStndrtCentrlMomntOfDist(index, 3);
}

std::optional<std::pair<std::byte const*, mo::Type const*>> CsvStats::GetKurtosis(int index) {
    mo::TypedColumnData const& col = col_data_[index];
    mo::TypeId type_id = col.GetTypeId();
    if (type_id == +mo::TypeId::kEmpty || type_id == +mo::TypeId::kNull ||
        type_id == +mo::TypeId::kUndefined) {
        return {};
    }
    return CountStndrtCentrlMomntOfDist(index, 4);
}

int CsvStats::Count(int index) {
    mo::TypedColumnData const& col = col_data_[index];
    return col.GetNumRows() - col.GetNumNulls() - col.GetNumEmpties();
};

int CsvStats::Distinct(int index) {
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

void CsvStats::ShowSample(int start_row, int end_row, int start_col, int end_col, int str_len,
                          int int_len, int double_len) {
    // int quantity_of_headers = end_col - start_col + 1;
    for (int i = start_row - 1; i <= end_row - 1; ++i) {
        for (int j = start_col - 1; j <= end_col - 1; ++j) {
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

bool CsvStats::isCategorial(int index, int quantity) {
    return this->Distinct(index) <= quantity ? true : false;
}

std::vector<std::byte const*> CsvStats::DeleteNullAndEmpties(int index) {
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

std::optional<std::pair<std::byte const*, mo::Type const*>> CsvStats::GetQuantile(double part,
                                                                                  int index) {
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
    return std::make_pair(data[quantile], &col.GetType());
}
