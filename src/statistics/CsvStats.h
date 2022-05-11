#pragma once

#include <iomanip>
#include <iostream>
#include <unordered_set>

#include "BuiltIn.h"
#include "CSVParser.h"
#include "ColumnLayoutTypedRelationData.h"
#include "NumericType.h"

namespace fs = std::filesystem;
namespace mo = model;

class CsvStats {
public:
    CsvStats(std::string_view data, char sep, bool has_header);

    CsvStats(CsvStats&&) = delete;
    CsvStats(const CsvStats&) = delete;
    CsvStats& operator=(CsvStats&&) = delete;
    CsvStats& operator=(const CsvStats&) = delete;

    std::vector<mo::TypedColumnData> const& GetData() const noexcept {
        return col_data_;
    }

    int Count(int index);
    int Distinct(int index);
    bool isCategorial(int index, int quantity);
    void ShowSample(int start_row, int end_row, int start_col, int end_col, int str_length = 10,
                    int int_len = 5, int double_len = 10);
    std::optional<std::pair<std::byte const*, mo::Type const*>> GetAvg(int index);
    std::optional<std::pair<std::byte const*, mo::Type const*>> GetSTD(int index);
    std::optional<std::pair<std::byte const*, mo::Type const*>> GetSkewness(int index);
    std::optional<std::pair<std::byte const*, mo::Type const*>> GetKurtosis(int index);
    std::optional<std::pair<std::byte const*, mo::Type const*>> CountCentrlMomntOfDist(int index,
                                                                                       int number);
    std::optional<std::pair<std::byte const*, mo::Type const*>> CountStndrtCentrlMomntOfDist(
        int index, int number);
    std::optional<std::pair<std::byte const*, mo::Type const*>> GetMin(int index);
    std::optional<std::pair<std::byte const*, mo::Type const*>> GetMax(int index);
    std::optional<std::pair<std::byte const*, mo::Type const*>> GetSum(int index);
    std::optional<std::pair<std::byte const*, mo::Type const*>> GetQuantile(double part, int index);
    std::vector<std::byte const*> DeleteNullAndEmpties(int index);

private:
    const std::vector<mo::TypedColumnData> col_data_;
};
