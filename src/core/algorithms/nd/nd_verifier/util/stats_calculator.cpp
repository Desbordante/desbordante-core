#include "algorithms/nd/nd_verifier/util/stats_calculator.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

#include <easylogging++.h>

namespace algos::nd_verifier::util {

template <typename It>
std::shared_ptr<std::vector<size_t>> StatsCalculator::CalculateFrequencies(size_t codes_number,
                                                                           It begin, It end) {
    auto result = std::make_shared<std::vector<size_t>>();
    for (size_t code{0}; code < codes_number; ++code) {
        result->push_back(std::count(begin, end, code));
    }

    return result;
}

void StatsCalculator::CalculateStats() {
    auto start_time = std::chrono::system_clock::now();
    lhs_frequencies_ =
            CalculateFrequencies(lhs_values_->size(), encoded_lhs_->begin(), encoded_lhs_->end());
    rhs_frequencies_ =
            CalculateFrequencies(rhs_values_->size(), encoded_rhs_->begin(), encoded_rhs_->end());

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

    auto indices_to_str = [](std::vector<size_t> const& vect) -> std::string {
        return '(' +
               std::accumulate(std::next(vect.begin()), vect.end(), std::to_string(vect[0]),
                               [](std::string&& s, size_t elem) {
                                   return std::move(s) + ", " + std::to_string(elem);
                               }) +
               ')';
    };

    auto values_to_str = [](std::vector<std::string> const& vect) -> std::string {
        return '(' +
               std::accumulate(std::next(vect.begin()), vect.end(), vect[0],
                               [](std::string&& s, std::string const& e) {
                                   return std::move(s) + ", " + e;
                               }) +
               ')';
    };

    auto local_start_time = std::chrono::system_clock::now();
    std::stringstream ss_hl;
    ss_hl << "Highlights:\n";
    for (auto& hl : highlights_) {
        ss_hl << "\tHighlight:\n";
        ss_hl << "\t\tOccurences indices: " << indices_to_str(hl.GetOccurencesIndices()) << '\n';
        ss_hl << "\t\tOccurences number: " << hl.GetOccurencesNumber() << '\n';
        ss_hl << "\t\tLhs value: " << hl.GetLhsValue() << '\n';
        ss_hl << "\t\tRhs values: " << values_to_str(hl.GetRhsValues()) << '\n';
        ss_hl << "\t\tDistinct rhs values number: " << hl.GetDistinctRhsValuesNumber() << '\n';
        ss_hl << "\t\tMost frequent rhs values indices: "
              << indices_to_str(hl.GetMostFrequentRhsValueIndices()) << '\n';
        ss_hl << "\t\tMost frequent rhs values: " << values_to_str(hl.GetMostFrequentRhsValues())
              << '\n';
        ss_hl << "\t\tValues string: " << hl.ToValuesString() << '\n';
        ss_hl << "\t\tIndices string: " << hl.ToIndicesString() << '\n';
    }
    LOG(INFO) << ss_hl.str();
    LOG(WARNING) << "Highlights output took "
                 << std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                                           std::chrono::system_clock::now() - local_start_time)
                                           .count())
                 << "ms";
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);

    LOG(WARNING) << "CalculateStats finished in " << std::to_string(elapsed_milliseconds.count())
                 << "ms";  // We use std::to_string, because compiler on github doesn`t
                           // have implementation for stringstream::operator<<(unsigned)
}

}  // namespace algos::nd_verifier::util
