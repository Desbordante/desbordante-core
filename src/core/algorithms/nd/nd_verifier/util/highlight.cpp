#include "core/algorithms/nd/nd_verifier/util/highlight.h"

#include <algorithm>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "core/algorithms/nd/nd_verifier/util/value_combination.h"
#include "core/util/range_to_string.h"

namespace algos::nd_verifier::util {

std::unordered_set<size_t> const& Highlight::GetMostFrequentRhsValueCodes() {
    if (most_frequent_rhs_value_codes_ == nullptr) {
        most_frequent_rhs_value_codes_ =
                std::make_unique<std::unordered_set<size_t>>(CalculateMostFrequentRhsValueCodes());
    }

    return *most_frequent_rhs_value_codes_;
}

std::unordered_set<size_t> Highlight::CalculateMostFrequentRhsValueCodes() const {
    size_t max_freq = *(std::max_element(rhs_frequencies_->begin(), rhs_frequencies_->end()));

    std::unordered_set<size_t> most_frequent_rhs_value_codes;
    for (size_t rhs_code : highlight_rhs_) {
        size_t freq = (*rhs_frequencies_)[rhs_code];
        if (freq == max_freq) {
            most_frequent_rhs_value_codes.insert(rhs_code);
        }
    }
    return most_frequent_rhs_value_codes;
}

std::vector<size_t> const& Highlight::GetOccurrencesIndices() {
    if (occurrences_indices_ == nullptr) {
        occurrences_indices_ = std::make_unique<std::vector<size_t>>(CalculateOccurrencesIndices());
    }

    return *occurrences_indices_;
}

std::vector<size_t> Highlight::CalculateOccurrencesIndices() const {
    std::vector<size_t> occurrences_indices;
    for (size_t i{0}; i < encoded_lhs_->size(); ++i) {
        if ((*encoded_lhs_)[i] == highlight_lhs_) {
            occurrences_indices.push_back(i);
        }
    }

    return occurrences_indices;
}

[[nodiscard]] size_t Highlight::GetOccurrencesNumber() {
    return GetOccurrencesIndices().size();
}

std::vector<std::string> const& Highlight::GetRhsValues() {
    if (highlight_rhs_values_ == nullptr) {
        highlight_rhs_values_ = std::make_unique<std::vector<std::string>>(CalculateRhsValues());
    }

    return *highlight_rhs_values_;
}

[[nodiscard]] std::vector<std::string> Highlight::CalculateRhsValues() const {
    std::vector<std::string> highlight_rhs_values;
    std::transform(highlight_rhs_.begin(), highlight_rhs_.end(),
                   std::back_inserter(highlight_rhs_values),
                   [this](size_t code) { return (*rhs_values_)[code].ToString(); });

    return highlight_rhs_values;
}

[[nodiscard]] std::vector<size_t> Highlight::GetMostFrequentRhsValueIndices() {
    std::vector<size_t> most_freq_rhs_inds;

    std::unordered_set<size_t> const& most_freq_rhs_codes = GetMostFrequentRhsValueCodes();
    for (size_t idx{0}; idx < encoded_rhs_->size(); ++idx) {
        size_t candidate_code = (*encoded_rhs_)[idx];
        if (most_freq_rhs_codes.find(candidate_code) != most_freq_rhs_codes.end()) {
            most_freq_rhs_inds.push_back(idx);
        }
    }

    return most_freq_rhs_inds;
}

[[nodiscard]] std::vector<std::string> Highlight::GetMostFrequentRhsValues() {
    std::vector<std::string> most_freq_rhs_vals;
    std::unordered_set<size_t> const& most_freq_rhs_codes = GetMostFrequentRhsValueCodes();

    std::transform(most_freq_rhs_codes.begin(), most_freq_rhs_codes.end(),
                   std::back_inserter(most_freq_rhs_vals),
                   [this](size_t code) { return (*rhs_values_)[code].ToString(); });

    return most_freq_rhs_vals;
}

[[nodiscard]] std::string Highlight::ToIndicesString() const {
    return ::util::RangeToString(CalculateOccurrencesIndices());
}

[[nodiscard]] std::string Highlight::ToValuesString() const {
    std::string const& lhs = GetLhsValue();
    std::vector<std::string> const& rhs = CalculateRhsValues();

    return lhs + " -> " + ::util::RangeToString(rhs);
}

std::ostream& operator<<(std::ostream& os, Highlight const& hl) {
    os << hl.ToValuesString();
    return os;
}

}  // namespace algos::nd_verifier::util
