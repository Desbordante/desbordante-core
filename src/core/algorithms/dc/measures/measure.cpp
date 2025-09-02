#include "measure.h"

#include <algorithm>
#include <numeric>
#include <ranges>
#include <vector>

#include "algorithms/dc/verifier/dc_verifier.h"

namespace algos::dc {

double Measure::G1() {
    double viol_size = static_cast<double>(verifier_->GetViolations().size());
    return 1 - viol_size / (rel_size_ * rel_size_);
}

double Measure::G1_NORM() {
    std::vector<size_t> freqs = GetFrequencies();
    auto square = [](size_t x) { return x * x; };
    auto squares = freqs | std::views::transform(square);
    size_t sum = std::accumulate(squares.begin(), squares.end(), 0);
    double viol_size = static_cast<double>(verifier_->GetViolations().size());

    return 1 - viol_size / (rel_size_ * rel_size_ - sum);
}

double Measure::G2() {
    auto raw_violations = verifier_->GetRawViolations();
    auto freqs = verifier_->GetFrequencies();

    // Get all tuples that are found in violation pairs
    std::unordered_set<dc::Point<dc::Component>, dc::Point<dc::Component>::Hasher> tuples_set;
    for (auto it = raw_violations.begin(); it != raw_violations.end(); ++it) {
        tuples_set.insert({it->first, it->second});
    }

    size_t total_count = 0;
    for (auto it = tuples_set.begin(); it != tuples_set.end(); ++it) {
        total_count += freqs[*it];
    }

    return 1 - static_cast<double>(total_count) / (rel_size_ * rel_size_);
}

std::vector<size_t> Measure::GetFrequencies() const {
    auto freqs = verifier_->GetFrequencies();
    std::vector<size_t> res;
    auto get_count = [&freqs](auto const& item) { return item.second; };
    std::transform(freqs.begin(), freqs.end(), std::back_inserter(res), get_count);
    return res;
}

Measure::Measure(std::shared_ptr<algos::DCVerifier> verifier) {
    verifier_ = std::move(verifier);
    rel_size_ = verifier_->relation_->GetNumRows();
    type_to_method_map_[MeasureType::G1] = &Measure::G1;
    type_to_method_map_[MeasureType::G1_NORM] = &Measure::G1_NORM;
    type_to_method_map_[MeasureType::G2] = &Measure::G2;
}

double Measure::Get(MeasureType type) {
    auto it = type_to_method_map_.find(type);
    if (it != type_to_method_map_.end()) {
        return (this->*type_to_method_map_[type])();
    }

    throw std::invalid_argument("Current measure is not supported");
}

}  // namespace algos::dc
