#pragma once

#include <vector>

#include "../../model/types/Types.h"

namespace algos {
class Statistic {
    bool has_value;
    std::unique_ptr<std::byte> data;
    std::unique_ptr<model::Type> type;

public:
    Statistic() noexcept;
    Statistic(const std::byte*, const model::Type*, bool clone_data = true);
    Statistic(const Statistic&);
    Statistic(Statistic&&);
    ~Statistic();

    Statistic& operator=(const Statistic&);
    Statistic& operator=(Statistic&&);

    bool HasValue() const noexcept;
    const std::byte* GetData() const;
    const model::Type* GetType() const;
    std::string ToString() const;
};

class ColumnStats {
public:
    bool isDistinctCorrect = false;
    int count;
    int distinct;
    bool isCategorical;
    Statistic avg, STD, skewness, kurtosis, min, max, sum, quantile25, quantile50, quantile75;
    std::string ToString() const;
    std::vector<std::pair<std::string, std::string>> ToKeyValueStringPairs() const;
};

}  // namespace algos