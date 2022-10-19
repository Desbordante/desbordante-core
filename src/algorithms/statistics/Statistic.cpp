#include "Statistic.h"

#include <memory>

namespace algos {

Statistic::Statistic() noexcept : has_value(false) {}

Statistic::Statistic(const std::byte* data, const model::Type* type, bool clone_data)
    : has_value(false) {
    if (type != nullptr && data != nullptr) {
        has_value = true;
        this->type = model::CreateTypeClone(*type);
        if (clone_data)
            this->data = std::unique_ptr<std::byte>(type->Clone(data));
        else
            this->data = std::unique_ptr<std::byte>(const_cast<std::byte*>(data));
    }
}

Statistic::Statistic(const Statistic& other)
    : Statistic::Statistic(other.data.get(), other.type.get()) {}

Statistic::Statistic(Statistic&& other)
    : has_value(other.has_value), data(std::move(other.data)), type(std::move(type)) {
    other.has_value = false;
}
Statistic& Statistic::operator=(const Statistic& other) {
    if (this != &other) {
        has_value = other.has_value;
        if (has_value) {
            type = model::CreateTypeClone(*other.type);
            data = std::unique_ptr<std::byte>(type->Clone(other.data.get()));
        } else {
            type = nullptr;
            data = nullptr;
        }
    }
    return *this;
}

Statistic& Statistic::operator=(Statistic&& other) {
    has_value = other.has_value;
    type = std::move(other.type);
    data = std::move(other.data);
    other.has_value = false;
    return *this;
}

Statistic::~Statistic() {
    if (has_value) {
        type->Free(data.release());
        type.reset(nullptr);
    }
}

bool Statistic::HasValue() const noexcept {
    return has_value;
}

const std::byte* Statistic::GetData() const {
    return data.get();
}

const model::Type* Statistic::GetType() const {
    return type.get();
}

std::string Statistic::ToString() const {
    if (!has_value) return "";
    return type->ValueToString(data.get());
}

static void inline tryAddStat(std::vector<std::pair<std::string, std::string>>& res,
                              const Statistic& stat, const std::string& statName) {
    if (stat.HasValue()) {
        res.push_back(std::make_pair(statName, stat.ToString()));
    }
}

std::vector<std::pair<std::string, std::string>> ColumnStats::ToKeyValueStringPairs() const {
    std::vector<std::pair<std::string, std::string>> res;
    res.reserve(13);
    res.push_back(std::make_pair("count", std::to_string(count)));
    if (isDistinctCorrect) {
        res.push_back(std::make_pair("distinct", std::to_string(distinct)));
        res.push_back(std::make_pair("isCategorical", std::to_string(isCategorical)));
    }
    tryAddStat(res, avg, "avg");
    tryAddStat(res, STD, "STD");
    tryAddStat(res, skewness, "skewness");
    tryAddStat(res, kurtosis, "kurtosis");
    tryAddStat(res, min, "min");
    tryAddStat(res, max, "max");
    tryAddStat(res, sum, "sum");
    tryAddStat(res, quantile25, "quantile25");
    tryAddStat(res, quantile50, "quantile50");
    tryAddStat(res, quantile75, "quantile75");
    return res;
}

std::string ColumnStats::ToString() const {
    std::stringstream res;
    for (const auto& stat : ToKeyValueStringPairs()) {
        res << stat.first << " = " << stat.second << "\n";
    }
    return res.str();
}

}  // namespace algos