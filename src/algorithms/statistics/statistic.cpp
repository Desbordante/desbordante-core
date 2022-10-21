#include "statistic.h"

#include <memory>

namespace algos {

Statistic::Statistic(const std::byte* data, const model::Type* type, bool clone_data) {
    if (type != nullptr && data != nullptr) {
        has_value_ = true;
        this->type_ = type->CloneType();
        if (clone_data)
            this->data_ = type->Clone(data);
        else
            this->data_ = data;
    }
}

Statistic::Statistic(const Statistic& other)
    : Statistic::Statistic(other.data_, other.type_.get()) {}

Statistic::Statistic(Statistic&& other)
    : has_value_(other.has_value_), data_(std::move(other.data_)), type_(std::move(other.type_)) {
    other.has_value_ = false;
}
Statistic& Statistic::operator=(const Statistic& other) {
    if (this != &other) {
        has_value_ = other.has_value_;
        if (has_value_) {
            type_ = other.type_->CloneType();
            data_ = type_->Clone(other.data_);
        } else {
            type_ = nullptr;
            data_ = nullptr;
        }
    }
    return *this;
}

Statistic& Statistic::operator=(Statistic&& other) {
    has_value_ = other.has_value_;
    type_ = std::move(other.type_);
    data_ = std::move(other.data_);
    other.has_value_ = false;
    return *this;
}

Statistic::~Statistic() {
    if (has_value_) type_->Free(data_);
}

bool Statistic::HasValue() const noexcept {
    return has_value_;
}

const std::byte* Statistic::GetData() const {
    return data_;
}

const std::byte* Statistic::ReleaseData() {
    const std::byte* res = data_;
    data_ = nullptr;
    has_value_ = false;
    return res;
}


const model::Type* Statistic::GetType() const {
    return type_.get();
}

std::string Statistic::ToString() const {
    if (!has_value_) return "";
    return type_->ValueToString(data_);
}

std::unordered_map<std::string, std::string> ColumnStats::ToKeyValueStringPairs() const {
    std::unordered_map<std::string, std::string> res;
    res.insert(std::make_pair("count", std::to_string(count)));
    if (is_distinct_correct) {
        res.insert(std::make_pair("distinct", std::to_string(distinct)));
        res.insert(std::make_pair("isCategorical", std::to_string(is_categorical)));
    }

    auto try_add_stat = [&res](const Statistic& stat, const std::string& statName) {
        if (stat.HasValue()) res.insert(std::make_pair(statName, stat.ToString()));
    };

    try_add_stat(avg, "avg");
    try_add_stat(STD, "STD");
    try_add_stat(skewness, "skewness");
    try_add_stat(kurtosis, "kurtosis");
    try_add_stat(min, "min");
    try_add_stat(max, "max");
    try_add_stat(sum, "sum");
    try_add_stat(quantile25, "quantile25");
    try_add_stat(quantile50, "quantile50");
    try_add_stat(quantile75, "quantile75");
    return res;
}

std::string ColumnStats::ToString() const {
    std::stringstream res;
    for (const auto& stat : ToKeyValueStringPairs()) {
        res << stat.first << " = " << stat.second << '\n';
    }
    return res.str();
}

}  // namespace algos
