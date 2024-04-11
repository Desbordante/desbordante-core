#pragma once

#include <algorithm>
#include <memory>
#include <numeric>
#include <sstream>
#include <unordered_set>
#include <vector>

namespace algos::nd_verifier::util {

/// @brief Lhs value that violates ND
/// @tparam V -- type of values in table. We assume that V supports operator<<(std::ostream, V)
template <typename V>
class Highlight {
private:
    // Shared data:
    std::shared_ptr<std::vector<V>> lhs_codes_;
    std::shared_ptr<std::vector<V>> rhs_codes_;
    std::shared_ptr<std::vector<size_t>> encoded_lhs_;
    std::shared_ptr<std::vector<size_t>> encoded_rhs_;
    std::shared_ptr<std::vector<size_t>> lhs_frequencies_;
    std::shared_ptr<std::vector<size_t>> rhs_frequencies_;

    // Highlight parameters:
    size_t highlight_lhs_code_;
    std::unordered_set<size_t> highlight_rhs_codes_;

    // Cached data:
    std::shared_ptr<std::vector<size_t>> most_frequent_rhs_value_codes_{nullptr};

    /// @brief Find most frequent rhs values and cache them
    auto const& GetMostFrequentRhsValueCodes() {
        if (most_frequent_rhs_value_codes_ == nullptr) {
            most_frequent_rhs_value_codes_ = std::make_shared<std::vector<size_t>>();
            unsigned num_most_frequent_rhs_values_{0};
            for (auto rhs_code : highlight_rhs_codes_) {
                auto freq = rhs_frequencies_->at(rhs_code);
                if (freq > num_most_frequent_rhs_values_) {
                    num_most_frequent_rhs_values_ = freq;
                    most_frequent_rhs_value_codes_->clear();
                    most_frequent_rhs_value_codes_->push_back(rhs_code);
                } else if (freq == num_most_frequent_rhs_values_) {
                    most_frequent_rhs_value_codes_->push_back(rhs_code);
                }
            }
        }

        return *most_frequent_rhs_value_codes_;
    }

public:
    Highlight(std::shared_ptr<std::vector<V>> lhs_codes, std::shared_ptr<std::vector<V>> rhs_codes,
              std::shared_ptr<std::vector<size_t>> encoded_lhs,
              std::shared_ptr<std::vector<size_t>> encoded_rhs,
              std::shared_ptr<std::vector<size_t>> lhs_frequencies,
              std::shared_ptr<std::vector<size_t>> rhs_frequencies, size_t highlight_lhs_code,
              std::unordered_set<size_t>&& highlight_rhs_codes)
        : lhs_codes_(std::move(lhs_codes)),
          rhs_codes_(std::move(rhs_codes)),
          encoded_lhs_(std::move(encoded_lhs)),
          encoded_rhs_(std::move(encoded_rhs)),
          lhs_frequencies_(std::move(lhs_frequencies)),
          rhs_frequencies_(std::move(rhs_frequencies)),
          highlight_lhs_code_(highlight_lhs_code),
          highlight_rhs_codes_(std::move(highlight_rhs_codes)) {}

    /// @brief Calculate Lhs indices -- heavy operation
    [[nodiscard]] auto GetLhsIndices() const {
        std::vector<size_t> lhs_indices;
        for (size_t i{0}; i < encoded_lhs_->size(); ++i) {
            if (encoded_lhs_->at(i) == highlight_lhs_code_) {
                lhs_indices.push_back(i);
            }
        }

        return lhs_indices;
    }

    /// @brief Calculate Rhs indices -- heavy operation
    [[nodiscard]] auto GetRhsIndices() const {
        std::vector<size_t> rhs_indices;
        for (size_t i{0}; i < encoded_rhs_->size(); ++i) {
            auto const candidate_code = encoded_rhs_->at(i);
            for (auto const code_in_rhs : highlight_rhs_codes_) {
                if (candidate_code == code_in_rhs) {
                    rhs_indices.push_back(i);
                }
            }
        }

        return rhs_indices;
    }

    /// @brief Get Lhs value
    [[nodiscard]] auto GetLhsValue() const {
        return lhs_codes_->at(highlight_lhs_code_);
    }

    /// @brief Get Rhs values -- heavy operation
    [[nodiscard]] auto GetRhsValues() const {
        std::vector<V> rhs_values;
        for (auto rhs_code : highlight_rhs_codes_) {
            rhs_values.push_back(rhs_codes_->at(rhs_code));
        }

        return rhs_values;
    }

    /// @brief Get number of rhs values -- heavy operation
    [[nodiscard]] auto GetRhsValuesNumber() const {
        return std::count_if(encoded_rhs_->begin(), encoded_rhs_->end(), [this](auto code) {
            return highlight_rhs_codes_.find(code) != highlight_rhs_codes_.end();
        });
    }

    /// @brief Get number of distinct values in rhs
    [[nodiscard]] auto GetDistinctRhsValuesNumber() const {
        return highlight_rhs_codes_.size();
    }

    /// @brief Get indices of most frequent rhs values -- heavy operation
    [[nodiscard]] auto GetMostFrequentRhsValueIndices() const {
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

    /// @brief Get most frequent rhs values -- heavy operation
    [[nodiscard]] auto GetMostFrequentRhsValues() const {
        std::vector<V> most_freq_rhs_vals;

        auto const& most_freq_rhs_codes = GetMostFrequentRhsValueCodes();
        for (auto code : most_freq_rhs_codes) {
            most_freq_rhs_vals.push_back(rhs_codes_->at(code));
        }

        return most_freq_rhs_vals;
    }

    /// @brief Get total number of most frequent rhs values -- can be a heavy operation
    [[nodiscard]] auto GetMostFrequentRhsValuesNumber() const {
        return GetMostFrequentRhsValueCodes().size();
    }

    [[nodiscard]] std::string ToIndicesString() const {
        auto const& lhs_idxs = GetLhsIndices();
        auto const& rhs_idxs = GetRhsIndices();

        auto idxs_to_str = [](std::vector<size_t> const& idxs) -> std::string {
            return '[' +
                   std::accumulate(std::next(idxs.begin()), idxs.end(), std::to_string(idxs[0]),
                                   [](std::string&& a, size_t b) {
                                       return std::move(a) + ", " + std::to_string(b);
                                   }) +
                   ']';
        };

        std::stringstream ss;
        ss << idxs_to_str(lhs_idxs) << " -> " << idxs_to_str(rhs_idxs);
        return ss.str();
    }

    [[nodiscard]] std::string ToValuesString() const {
        auto const& lhs = GetLhsValue();
        auto const& rhs = GetRhsValues();

        std::stringstream ss;
        ss << lhs << " -> [";
        for (auto pt{rhs.begin()}; pt != rhs.end(); ++pt) {
            if (pt != rhs.begin()) {
                ss << ", ";
            }
            ss << *pt;
        }
        ss << ']';
        return ss.str();
    }
};

template <typename V>
std::ostream& operator<<(std::ostream& os, Highlight<V> const& hl) {
    os << hl.ToValuesString();
    return os;
}

}  // namespace algos::nd_verifier::util
