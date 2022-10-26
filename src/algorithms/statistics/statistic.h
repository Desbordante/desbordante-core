#pragma once

#include <unordered_map>

#include "types.h"

namespace algos {
class Statistic {
    bool has_value_ = false;
    const std::byte* data_ = nullptr;
    std::unique_ptr<model::Type> type_ = nullptr;

public:
    Statistic() noexcept = default;
    Statistic(const std::byte*, const model::Type*, bool clone_data = true);
    Statistic(const Statistic&);
    Statistic(Statistic&&);
    ~Statistic();

    Statistic& operator=(const Statistic&);
    Statistic& operator=(Statistic&&);

    bool HasValue() const noexcept;
    const std::byte* GetData() const;
    const model::Type* GetType() const;
    const std::byte* ReleaseData();
    std::string ToString() const;
};

struct ColumnStats {
    bool is_distinct_correct = false;
    size_t count;
    size_t distinct;
    bool is_categorical;
    Statistic avg, STD, skewness, kurtosis, min, max, sum, quantile25, quantile50, quantile75;
    std::string ToString() const;
    std::unordered_map<std::string, std::string> ToKeyValueStringPairs() const;
};

}  // namespace algos
