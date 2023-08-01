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
    Statistic(const std::byte*, const model::Type*, bool clone_data);
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
    std::string type;
    size_t count;
    size_t distinct;
    bool is_categorical;
    Statistic avg, STD, skewness, kurtosis, min, max, sum, quantile25, quantile50, quantile75,
            num_zeros, num_negatives, sum_of_squares, geometric_mean, mean_ad, median, median_ad;
    std::string ToString() const;
    std::unordered_map<std::string, std::string> ToKeyValueMap() const;
};

}  // namespace algos
