#pragma once

#include "Statistic.h"

#include "ColumnLayoutTypedRelationData.h"
#include "FDAlgorithm.h"

namespace statistics {

class CsvStats: public algos::Primitive {
public:

    explicit CsvStats(FDAlgorithm::Config const& config);
    ~CsvStats();

    CsvStats(CsvStats&&) = delete;
    CsvStats(const CsvStats&) = delete;
    CsvStats& operator=(CsvStats&&) = delete;
    CsvStats& operator=(const CsvStats&) = delete;

    const std::vector<model::TypedColumnData> & GetData() const noexcept;

    int Count(unsigned index);
    unsigned GetCountOfColumns();
    int Distinct(unsigned index);
    bool isCategorial(unsigned index, int quantity);
    void ShowSample(unsigned start_row, unsigned end_row, unsigned start_col, unsigned end_col, 
        unsigned str_length = 10, unsigned unsigned_len = 5, unsigned double_len = 10);
    Statistic GetAvg(unsigned index);
    Statistic GetSTD(unsigned index);
    Statistic GetSkewness(unsigned index);
    Statistic GetKurtosis(unsigned index);
    Statistic CountCentralMomentOfDist(unsigned index, int number);
    Statistic CountStandardizedCentralMomentOfDist(unsigned index, int number);
    Statistic GetMin(unsigned index);
    Statistic GetMax(unsigned index);
    Statistic GetSum(unsigned index);
    Statistic GetQuantile(double part, unsigned index);
    std::vector<std::byte const*> DeleteNullAndEmpties(unsigned index);

    const ColumnStats& GetAllStats(unsigned index) const;
    const std::vector<ColumnStats>& GetAllStats() const;

    ColumnStats& GetAllStats(unsigned index);
    std::vector<ColumnStats>& GetAllStats();

    std::string toString();

private:
    unsigned long long Execute();
    FDAlgorithm::Config config_;
    const std::vector<model::TypedColumnData> col_data_;
    std::vector<ColumnStats> allStats;
    
    Statistic STDAndCentrMomnt(unsigned index, int number, bool isForSTD);
};

}
