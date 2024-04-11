#pragma once

#include <algorithm>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <easylogging++.h>

#include "algorithms/nd/nd.h"
#include "algorithms/nd/nd_verifier/util/highlight.h"

namespace algos::nd_verifier::util {

template <typename V>
class StatsCalculator {
private:
    // Shared data:
    std::shared_ptr<std::unordered_map<size_t, std::unordered_set<size_t>>> value_deps_;
    std::shared_ptr<std::vector<V>> lhs_values_;
    std::shared_ptr<std::vector<V>> rhs_values_;
    std::shared_ptr<std::vector<size_t>> encoded_lhs_;
    std::shared_ptr<std::vector<size_t>> encoded_rhs_;

    // Cached data:
    std::vector<Highlight<V>> highlights_;
    std::shared_ptr<std::vector<size_t>> lhs_frequencies_;
    std::shared_ptr<std::vector<size_t>> rhs_frequencies_;
    model::WeightType global_min_weight_{UINT_MAX};
    model::WeightType real_weight_{0};

    template <typename It>
    auto CalculateFrequencies(size_t codes_number, It begin, It end) {
        auto result = std::make_shared<std::vector<size_t>>();
        for (size_t code{0}; code < codes_number; ++code) {
            result->push_back(std::count(begin, end, code));
        }

        return result;
    }

public:
    StatsCalculator(
            std::shared_ptr<std::unordered_map<size_t, std::unordered_set<size_t>>> value_deps,
            std::shared_ptr<std::vector<V>> lhs_codes, std::shared_ptr<std::vector<V>> rhs_codes,
            std::shared_ptr<std::vector<size_t>> encoded_lhs,
            std::shared_ptr<std::vector<size_t>> encoded_rhs)
        : value_deps_(std::move(value_deps)),
          lhs_values_(std::move(lhs_codes)),
          rhs_values_(std::move(rhs_codes)),
          encoded_lhs_(std::move(encoded_lhs)),
          encoded_rhs_(std::move(encoded_rhs)) {}

    StatsCalculator() = default;

    [[nodiscard]] auto const& GetHighlights() const {
        return highlights_;
    }

    [[nodiscard]] auto GetGlobalMinWeight() const {
        return global_min_weight_;
    }

    [[nodiscard]] auto GetRealWeight() const {
        return real_weight_;
    }

    [[nodiscard]] auto const& GetLhsFrequencies() const {
        return *lhs_frequencies_;
    }

    [[nodiscard]] auto const& GetRhsFrequencies() const {
        return *rhs_frequencies_;
    }

    void CalculateStats() {
        auto start_time = std::chrono::system_clock::now();
        lhs_frequencies_ = CalculateFrequencies(lhs_values_->size(), encoded_lhs_->begin(),
                                                encoded_lhs_->end());
        rhs_frequencies_ = CalculateFrequencies(rhs_values_->size(), encoded_rhs_->begin(),
                                                encoded_rhs_->end());

        std::vector<size_t> highlights_lhs_codes;
        model::WeightType max_weight{0};
        model::WeightType min_weight{UINT_MAX};
        for (size_t code{0}; code < lhs_values_->size(); ++code) {
            auto weight = value_deps_->at(code).size();

            assert(weight > 0);

            if (weight > max_weight) {
                max_weight = weight;
                highlights_lhs_codes.clear();
                highlights_lhs_codes.push_back(code);
            } else if (weight == max_weight) {
                highlights_lhs_codes.push_back(code);
            }

            if (weight < min_weight) {
                min_weight = weight;
            }
        }

        assert(max_weight > 0);

        for (auto highlight_lhs_code : highlights_lhs_codes) {
            auto& rhs_set = value_deps_->at(highlight_lhs_code);

            highlights_.emplace_back(lhs_values_, rhs_values_, encoded_lhs_, encoded_rhs_,
                                     lhs_frequencies_, rhs_frequencies_, highlight_lhs_code,
                                     std::move(rhs_set));
        }

        real_weight_ = max_weight;
        global_min_weight_ = min_weight;

        LOG(INFO) << "Minimal weight: " << global_min_weight_;
        LOG(INFO) << "Maximal weight (real weight): " << real_weight_;

        std::stringstream ss_hl;
        ss_hl << "Highlights:\n";
        for (auto const& hl : highlights_) {
            ss_hl << '\t' << hl << '\n';
        }
        LOG(INFO) << ss_hl.str();

        auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - start_time);
        // TODO(senichenkov): here should be INFO:
        LOG(WARNING) << "CalculateStats finished in "
                     << std::to_string(elapsed_milliseconds.count())
                     << "ms";  // We use std::to_string, because compiler on github doesn`t
                               // have implementation for stringstream::operator<<(unsigned)
    }
};

}  // namespace algos::nd_verifier::util