#pragma once

#include "NumericType.h"

namespace statistics {
    class Statistic {
    bool hasValue_;
    std::byte* data;
    model::Type* type;

public:

    //add here types if they will have other Free method!!!
    static model::Type* getTypeClone(const model::Type& type, bool 
        is_null_eq_null);

    Statistic() noexcept;
    Statistic(std::byte const*, model::Type const*) noexcept;
    Statistic(const Statistic &) = default;
    Statistic(Statistic &&) = default;

    Statistic& operator=(const Statistic &) = default;
    Statistic& operator=(Statistic &&) = default;

    void Free();
    bool hasValue() const noexcept;
    std::byte const* getData();
    std::byte const* getDataAndFree();
    model::Type const* getType();
    std::string toString();
};

class ColumnStats {
public:
    int count;
    int distinct;
    bool isCategorical;
    Statistic avg, STD, skewness, kurtosis, min, max, sum, quantile25,
        quantile50, quantile75;
    void Free();
    std::string toString();
};

}