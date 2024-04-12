#include "algorithms/nd/nd_verifier/util/highlight.h"

#include <algorithm>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

namespace algos::nd_verifier::util {

std::vector<size_t> const& Highlight::GetMostFrequentRhsValueCodes() {
    if (most_frequent_rhs_value_codes_ == nullptr) {
        most_frequent_rhs_value_codes_ = std::make_unique<std::vector<size_t>>();
        unsigned num_most_frequent_rhs_values{0};
        for (auto rhs_code : highlight_rhs_) {
            auto freq = rhs_frequencies_->at(rhs_code);
            if (freq > num_most_frequent_rhs_values) {
                num_most_frequent_rhs_values = freq;
                most_frequent_rhs_value_codes_->clear();
                most_frequent_rhs_value_codes_->push_back(rhs_code);
            } else if (freq == num_most_frequent_rhs_values) {
                most_frequent_rhs_value_codes_->push_back(rhs_code);
            }
        }
    }

    return *most_frequent_rhs_value_codes_;
}

[[nodiscard]] std::vector<size_t> const& Highlight::GetOccurencesIndices() {
    if (occurences_indices_ == nullptr) {
        occurences_indices_ = std::make_unique<std::vector<size_t>>();
        for (size_t i{0}; i < encoded_lhs_->size(); ++i) {
            if (encoded_lhs_->at(i) == highlight_lhs_) {
                occurences_indices_->push_back(i);
            }
        }
    }

    return *occurences_indices_;
}

[[nodiscard]] size_t Highlight::GetOccurencesNumber() const {
    // This operation doesn't calculate occurences if they're not cached, so it's pretty fast
    if (occurences_indices_ == nullptr) {
        return std::count(encoded_lhs_->begin(), encoded_lhs_->end(), highlight_lhs_);
    } else {
        return occurences_indices_->size();
    }
}

[[nodiscard]] std::vector<std::string> const& Highlight::GetRhsValues() {
    if (highlight_rhs_values_ == nullptr) {
        highlight_rhs_values_ = std::make_unique<std::vector<std::string>>();
        for (auto rhs_code : highlight_rhs_) {
            highlight_rhs_values_->push_back(rhs_values_->at(rhs_code));
        }
    }

    return *highlight_rhs_values_;
}

[[nodiscard]] std::vector<size_t> Highlight::GetMostFrequentRhsValueIndices() {
    std::vector<size_t> most_freq_rhs_inds;

    auto const& most_freq_rhs_codes = GetMostFrequentRhsValueCodes();
    for (size_t idx{0}; idx < encoded_rhs_->size(); ++idx) {
        auto candidate_code = encoded_rhs_->at(idx);
        for (auto code : most_freq_rhs_codes) {
            if (candidate_code == code) {
                most_freq_rhs_inds.push_back(idx);
            }
        }
    }

    return most_freq_rhs_inds;
}

[[nodiscard]] std::vector<std::string> Highlight::GetMostFrequentRhsValues() {
    std::vector<std::string> most_freq_rhs_vals;

    auto const& most_freq_rhs_codes = GetMostFrequentRhsValueCodes();
    for (auto code : most_freq_rhs_codes) {
        most_freq_rhs_vals.push_back(rhs_values_->at(code));
    }

    return most_freq_rhs_vals;
}

[[nodiscard]] std::string Highlight::ToIndicesString() {
    auto const& occ_idxs = GetOccurencesIndices();

    return '[' +
           std::accumulate(std::next(occ_idxs.begin()), occ_idxs.end(), std::to_string(occ_idxs[0]),
                           [](std::string&& s, size_t v) {
                               return std::move(s) + ", " + std::to_string(v);
                           }) +
           ']';
}

[[nodiscard]] std::string Highlight::ToValuesString() {
    auto const& lhs = GetLhsValue();
    auto const& rhs = GetRhsValues();

    return lhs + " -> [" +
           std::accumulate(
                   std::next(rhs.begin()), rhs.end(), rhs[0],
                   [](std::string&& s, std::string const& e) { return std::move(s) + ", " + e; }) +
           ']';
}

std::ostream& operator<<(std::ostream& os, Highlight& hl) {
    os << hl.ToValuesString();
    return os;
}

}  // namespace algos::nd_verifier::util
