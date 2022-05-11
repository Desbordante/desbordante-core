#include "CsvStats.h"

#include "BuiltIn.h"
#include "NumericType.h"
#include "Type.h"

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

/*std::optional<std::variant<mo::Int, mo::Double>> CsvStats::GetSum(int index) {
    mo::TypedColumnData const& col = col_data_[index];
    if (col.IsNumeric()) {
        mo::INumericType const& type = static_cast<mo::INumericType const&>(col.GetType());
        std::byte* sum(type.Allocate());
        type.GetValue<std::variant<mo::Int, mo::Double>>(sum) = mo::Int(0);
        for (std::byte const* value : col.GetData()) {
            type.Add(sum, value, sum);
        }
        return type.GetValue<std::variant<mo::Int, mo::Double>>(sum);
    }
    return {};
};*/

std::byte* CsvStats::GetSum(int index) {
    mo::TypedColumnData const& col = col_data_[index];
    if (col.IsNumeric()) {
        mo::INumericType const& type = static_cast<mo::INumericType const&>(col.GetType());
        //mo::DoubleType type;

        std::byte* sum(type.Allocate());
        //type.GetValue<std::variant<mo::Int, mo::Double>>(sum) = mo::Int(0);
        for (std::byte const* value : col.GetData()) {
            type.Add(sum, value, sum);
        }

        return sum;
    }
    return {};
};

std::byte* CsvStats::GetAvg(int index) {
    mo::TypedColumnData const& col = col_data_[index];
    if (col.IsNumeric()) {
        mo::DoubleType type;
        mo::INumericType const& ctype = static_cast<mo::INumericType const&>(col.GetType());
        std::byte* sum = GetSum(index);
        std::byte* avg = type.Allocate();
        std::byte* left = type.MakeValue(ctype.GetValue<mo::Int>(sum));
        std::byte* right = type.MakeValue(col.GetNumRows() - col.GetNumNulls());
        type.Div(left, right, avg);
        return avg;
    }
    return {};
}

std::byte* CsvStats::GetSTD(int index)
{
    mo::TypedColumnData const& col = col_data_[index];
    if (col.IsNumeric()) {
        mo::DoubleType type;
        mo::INumericType const& cType = static_cast<mo::INumericType const&>(col.GetType());
        std::byte* avg = GetAvg(index);
        std::byte* nAvg = type.Allocate();
        type.Negate(avg, nAvg);
        std::byte* sum = type.Allocate();
        std::byte* dif = type.Allocate();
        for (std::byte const* value : col.GetData()) {
            if(col.GetTypeId() != static_cast<char>(mo::TypeId::kDouble)) {
                value = type.MakeValue(cType.GetValue<mo::Int>(value));
            }
            type.Add(value, nAvg, dif);
            type.Power(dif, 2, dif);
            type.Add(sum, dif, sum);
        }
        std::byte* divisor = type.MakeValue(col.GetNumRows() - col.GetNumNulls() - 1);
        std::byte* dispersion = type.Allocate();
        type.Div(sum, divisor, dispersion);
        return type.Power(dispersion, 0.5, dispersion);
    }
    return {};
}

std::byte* CsvStats::CountCentrlMomntOfDist(int index, int number) {
    mo::TypedColumnData const& col = col_data_[index];
    if (col.IsNumeric()) {
        mo::DoubleType type;
        mo::INumericType const& cType = static_cast<mo::INumericType const&>(col.GetType());
        std::byte* avg = GetAvg(index);
        std::byte* nAvg = type.Allocate();
        type.Negate(avg, nAvg);
        std::byte* sum = type.Allocate();
        std::byte* dif = type.Allocate();
        for (std::byte const* value : col.GetData()) {
            if(col.GetTypeId() != static_cast<char>(mo::TypeId::kDouble)) {
                value = type.MakeValue(cType.GetValue<mo::Int>(value));
            }
            type.Add(value, nAvg, dif);
            type.Power(dif, number, dif);
            type.Add(sum, dif, sum);
        }
        std::byte* divisor = type.MakeValue(col.GetNumRows() - col.GetNumNulls());
        return type.Div(sum, divisor, sum);
    }
    return {};
}

std::byte* CsvStats::CountStndrtCentrlMomntOfDist(int index, int number) {
    mo::DoubleType type;
    std::byte* left = type.Allocate();
    left = CountCentrlMomntOfDist(index, number);
    std::byte* right = type.Allocate();
    right = GetSTD(index);
    type.Power(right, number, right);
    return type.Div(left, right, right);
}

std::byte* CsvStats::GetSkewness(int index)
{
    return CountStndrtCentrlMomntOfDist(index, 3);
}

std::byte* CsvStats::GetKurtosis(int index)
{
    return CountStndrtCentrlMomntOfDist(index, 4);
}

int CsvStats::Count(int index) {
    mo::TypedColumnData const& col = col_data_[index];
    return col.GetNumRows() - col.GetNumNulls() - col.GetNumEmpties();
};