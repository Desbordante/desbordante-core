#include "sd_miner.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <list>
#include <numeric>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "core/algorithms/sd/util/edit_distance_calculator.h"
#include "core/algorithms/sd/util/numeric_utils.h"
#include "core/config/indices/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"

namespace {

std::vector<algos::sd_miner::SDCandidateInterval> RemoveContainedIntervals(
        std::vector<algos::sd_miner::SDCandidateInterval> intervals) {
    size_t kept = 0;
    for (auto const& interval : intervals) {
        if (kept == 0 || interval.right > intervals[kept - 1].right) {
            intervals[kept++] = interval;
        }
    }
    intervals.resize(kept);
    return intervals;
}

size_t ComputeUnionSupport(std::span<algos::sd_miner::SDCandidateInterval const> intervals) {
    if (intervals.empty()) {
        return 0;
    }
    size_t support = 0;
    size_t current_left = intervals.front().left;
    size_t current_right = intervals.front().right;
    for (size_t i = 1; i < intervals.size(); ++i) {
        if (intervals[i].left <= current_right + 1) {
            current_right = std::max(current_right, intervals[i].right);
        } else {
            support += current_right - current_left + 1;
            current_left = intervals[i].left;
            current_right = intervals[i].right;
        }
    }
    support += current_right - current_left + 1;
    return support;
}

// Finds a minimum-size subset of the selected intervals that preserves their union.
std::vector<algos::sd_miner::SDCandidateInterval> MinimizeSelectedIntervalCover(
        std::vector<algos::sd_miner::SDCandidateInterval> intervals) {
    std::sort(intervals.begin(), intervals.end(), [](auto const& lhs, auto const& rhs) {
        if (lhs.left != rhs.left) {
            return lhs.left < rhs.left;
        }
        return lhs.right > rhs.right;
    });

    std::vector<algos::sd_miner::SDCandidateInterval> result;
    result.reserve(intervals.size());
    size_t next_interval = 0;
    while (next_interval < intervals.size()) {
        size_t next_uncovered = intervals[next_interval].left;
        while (next_interval < intervals.size() &&
               intervals[next_interval].left <= next_uncovered) {
            size_t best_interval = next_interval;
            size_t best_right = intervals[next_interval].right;
            while (next_interval < intervals.size() &&
                   intervals[next_interval].left <= next_uncovered) {
                if (intervals[next_interval].right > best_right) {
                    best_interval = next_interval;
                    best_right = intervals[next_interval].right;
                }
                ++next_interval;
            }
            result.push_back(intervals[best_interval]);
            next_uncovered = best_right + 1;
        }
    }
    return result;
}

}  // namespace

namespace algos::sd_miner {

SDMiner::SDMiner() : Algorithm() {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void SDMiner::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({config::kLhsIndicesOpt.GetName(), config::kRhsIndicesOpt.GetName(), kSdG1,
                          kSdMinimumConfidence, kSdMinimumSupport, kSdDelta, kSdIntervalStrategy,
                          kSdAssemblyStrategy});
}

void SDMiner::RegisterOptions() {
    DESBORDANTE_OPTION_USING;
    RegisterOption(config::kTableOpt(&input_table_));

    auto get_cols_num = [this]() {
        return input_table_ ? static_cast<config::IndexType>(input_table_->GetNumberOfColumns())
                            : 0;
    };
    auto check_single_col = [](config::IndicesType const& val) {
        if (val.size() != 1) {
            throw std::runtime_error(
                    "SDMiner currently supports exactly one lhs and one rhs column.");
        }
    };
    auto make_threshold_check = [](std::string_view name) {
        return [name](double val) {
            if (!std::isfinite(val) || val < 0.0 || val > 1.0) {
                throw std::runtime_error(std::string{name} + " must be in [0, 1].");
            }
        };
    };

    RegisterOption(config::kLhsIndicesOpt(&lhs_indices_, get_cols_num, check_single_col));
    RegisterOption(config::kRhsIndicesOpt(&rhs_indices_, get_cols_num, check_single_col));
    RegisterOption(Option<double>{&g1_, kSdG1, kDSdG1}
                           .SetValueCheck([](double val) {
                               if (!std::isfinite(val) || val < 0.0) {
                                   throw std::runtime_error("g1 must be finite and non-negative.");
                               }
                           })
                           .SetConditionalOpts({{{}, {kSdG2}}}));
    RegisterOption(Option<double>{&g2_, kSdG2, kDSdG2}.SetValueCheck([this](double val) {
        if (!std::isfinite(val)) {
            throw std::runtime_error("g2 must be finite.");
        }
        if (val >= 0.0 && val < g1_) {
            throw std::runtime_error("g2 must be >= g1 or negative.");
        }
        if (val == 0.0 && g1_ == 0.0) {
            throw std::runtime_error("g1 and g2 cannot both be zero.");
        }
    }));
    RegisterOption(Option<double>{
            &min_confidence_, kSdMinimumConfidence, kDSdMinimumConfidence, []() {
                return 1.0;
            }}.SetValueCheck(make_threshold_check(kSdMinimumConfidence)));
    RegisterOption(Option<double>{
            &min_support_, kSdMinimumSupport, kDSdMinimumSupport, []() {
                return 1.0;
            }}.SetValueCheck(make_threshold_check(kSdMinimumSupport)));
    RegisterOption(Option<double>{
            &approximation_delta_, kSdDelta, kDSdDelta, []() {
                return 0.05;
            }}.SetValueCheck([](double val) {
        if (!std::isfinite(val) || val <= 0.0 || val >= 1.0 || 1.0 + val == 1.0) {
            throw std::runtime_error("delta must be in (0, 1).");
        }
    }));
    RegisterOption(Option<IntervalStrategy>{&interval_strategy_, kSdIntervalStrategy,
                                            kDSdIntervalStrategy,
                                            []() { return IntervalStrategy::kExact; }});
    RegisterOption(Option<AssemblyStrategy>{&assembly_strategy_, kSdAssemblyStrategy,
                                            kDSdAssemblyStrategy,
                                            []() { return AssemblyStrategy::kExact; }});
}

void SDMiner::LoadDataInternal() {
    raw_data_.clear();
    if (!input_table_) {
        throw std::runtime_error("Input table is not initialized.");
    }

    input_table_->Reset();
    while (input_table_->HasNextRow()) {
        raw_data_.push_back(input_table_->GetNextRow());
    }
}

void SDMiner::ExtractSortedData() {
    sorted_x_values_.clear();
    sorted_y_values_.clear();
    sorted_row_indices_.clear();

    size_t const lhs_idx = lhs_indices_[0];
    size_t const rhs_idx = rhs_indices_[0];
    std::vector<double> x_values;
    std::vector<double> y_values;
    std::vector<size_t> row_indices;

    for (size_t i = 0; i < raw_data_.size(); ++i) {
        auto const& row = raw_data_[i];
        if (row.size() <= std::max(lhs_idx, rhs_idx)) {
            continue;
        }
        x_values.push_back(sd::util::ParseNumeric(row[lhs_idx]));
        y_values.push_back(sd::util::ParseNumeric(row[rhs_idx]));
        row_indices.push_back(i);
    }

    std::vector<size_t> permutation(x_values.size());
    std::iota(permutation.begin(), permutation.end(), 0);
    std::sort(permutation.begin(), permutation.end(), [&](size_t lhs, size_t rhs) {
        if (x_values[lhs] != x_values[rhs]) {
            return x_values[lhs] < x_values[rhs];
        }
        if (y_values[lhs] != y_values[rhs]) {
            return y_values[lhs] < y_values[rhs];
        }
        return lhs < rhs;
    });

    sorted_x_values_.reserve(permutation.size());
    sorted_y_values_.reserve(permutation.size());
    sorted_row_indices_.reserve(permutation.size());
    for (size_t pos : permutation) {
        sorted_x_values_.push_back(x_values[pos]);
        sorted_y_values_.push_back(y_values[pos]);
        sorted_row_indices_.push_back(row_indices[pos]);
    }
}

size_t SDMiner::GetTargetSupport() const {
    size_t const n = sorted_y_values_.size();
    if (n == 0) {
        return 0;
    }
    size_t target = static_cast<size_t>(std::ceil(min_support_ * n));
    while (target > 0 && static_cast<double>(target - 1) / n >= min_support_) {
        --target;
    }
    while (target < n && static_cast<double>(target) / n < min_support_) {
        ++target;
    }
    return target;
}

std::vector<SDCandidateInterval> SDMiner::GenerateExactIntervals() const {
    algos::sd::util::EditDistanceCalculator calculator(g1_, g2_);
    size_t const n = sorted_y_values_.size();
    std::vector<SDCandidateInterval> intervals;

    for (size_t left = 0; left < n; ++left) {
        auto ops = calculator.CalculatePrefixOps(
                std::span<double const>(sorted_y_values_.data() + left, n - left));
        for (size_t len = ops.size(); len > 0; --len) {
            double const confidence = static_cast<double>(len - ops[len - 1]) / len;
            if (confidence >= min_confidence_) {
                intervals.push_back({left, left + len - 1, confidence});
                break;
            }
        }
        if (intervals.back().right == n - 1) {
            break;
        }
    }

    return RemoveContainedIntervals(std::move(intervals));
}

std::vector<SDCandidateInterval> SDMiner::GenerateApproximateIntervals() const {
    size_t const n = sorted_y_values_.size();
    if (n == 0) {
        return {};
    }

    long double const delta = approximation_delta_;
    long double const relaxed_confidence = min_confidence_ * (1.0L - delta) / (1.0L + delta);
    long double const data_len = static_cast<long double>(n);

    std::vector<long double> real_lengths;
    for (long double len = 1.0L;;) {
        real_lengths.push_back(std::min(len, data_len));
        if (len >= data_len) {
            break;
        }
        long double const next_len = len * (1.0L + delta);
        if (!std::isfinite(next_len) || next_len <= len) {
            throw std::runtime_error("delta is too small for approximate interval generation.");
        }
        len = next_len;
    }

    std::vector<std::optional<SDCandidateInterval>> best_by_left(n);
    algos::sd::util::EditDistanceCalculator calculator(g1_, g2_);

    size_t length_idx = 0;
    for (size_t group_base = 1; group_base <= n;) {
        long double const group_low = static_cast<long double>(group_base);
        long double const group_high = group_low * 2.0L;
        std::vector<long double> group_lengths;
        while (length_idx < real_lengths.size() && real_lengths[length_idx] < group_high) {
            if (real_lengths[length_idx] >= group_low) {
                group_lengths.push_back(real_lengths[length_idx]);
            }
            ++length_idx;
        }

        if (!group_lengths.empty()) {
            std::vector<std::vector<size_t>> requested_lengths(n);
            long double const start_step = delta * group_low;
            long double previous_left = -1.0L;
            for (size_t start_idx = 0;; ++start_idx) {
                long double const real_left = static_cast<long double>(start_idx) * start_step;
                if (!std::isfinite(real_left)) {
                    throw std::runtime_error("Approximate interval endpoint overflowed.");
                }
                if (real_left >= data_len) {
                    break;
                }
                if (start_idx != 0 && real_left <= previous_left) {
                    throw std::runtime_error(
                            "delta is too small for approximate interval generation.");
                }
                previous_left = real_left;

                long double const adjusted_left =
                        std::nextafter(real_left, -std::numeric_limits<long double>::infinity());
                size_t const left = static_cast<size_t>(std::ceil(std::max(0.0L, adjusted_left)));
                if (left >= n) {
                    if (start_idx == std::numeric_limits<size_t>::max()) {
                        throw std::runtime_error("Too many approximate interval endpoints.");
                    }
                    continue;
                }
                for (long double real_len : group_lengths) {
                    long double const real_right = std::min(data_len, real_left + real_len);
                    long double const adjusted_right = std::nextafter(
                            real_right, std::numeric_limits<long double>::infinity());
                    size_t const right_exclusive = static_cast<size_t>(
                            std::min(data_len, std::floor(std::max(0.0L, adjusted_right))));
                    if (right_exclusive > left) {
                        requested_lengths[left].push_back(right_exclusive - left);
                    }
                }

                if (start_idx == std::numeric_limits<size_t>::max()) {
                    throw std::runtime_error("Too many approximate interval endpoints.");
                }
            }

            for (size_t left = 0; left < n; ++left) {
                auto& lengths = requested_lengths[left];
                if (lengths.empty()) {
                    continue;
                }

                std::sort(lengths.begin(), lengths.end());
                lengths.erase(std::unique(lengths.begin(), lengths.end()), lengths.end());

                size_t const max_len = lengths.back();
                auto ops = calculator.CalculatePrefixOps(
                        std::span<double const>(sorted_y_values_.data() + left, max_len));
                for (auto it = lengths.rbegin(); it != lengths.rend(); ++it) {
                    size_t const len = *it;
                    double const confidence = static_cast<double>(len - ops[len - 1]) / len;
                    if (confidence >= relaxed_confidence) {
                        size_t const right = left + len - 1;
                        if (!best_by_left[left].has_value() || right > best_by_left[left]->right) {
                            best_by_left[left] = SDCandidateInterval{left, right, confidence};
                        }
                        break;
                    }
                }
            }
        }

        if (group_base > n / 2) {
            break;
        }
        group_base *= 2;
    }

    std::vector<SDCandidateInterval> intervals;
    for (auto const& best : best_by_left) {
        if (best.has_value()) {
            intervals.push_back(*best);
        }
    }
    return RemoveContainedIntervals(std::move(intervals));
}

std::vector<SDCandidateInterval> SDMiner::AssembleExact(
        std::vector<SDCandidateInterval> const& candidates) const {
    size_t const n = sorted_y_values_.size();
    size_t const target = GetTargetSupport();
    if (target == 0 || candidates.empty()) {
        return {};
    }

    for (auto const& interval : candidates) {
        if (interval.right - interval.left + 1 >= target) {
            return {interval};
        }
    }

    std::vector<std::optional<size_t>> best_interval_at_pos(n);
    size_t idx = 0;
    for (size_t pos = 0; pos < n; ++pos) {
        while (idx < candidates.size() && candidates[idx].right < pos) {
            ++idx;
        }
        if (idx < candidates.size() && candidates[idx].left <= pos) {
            best_interval_at_pos[pos] = idx;
        }
    }

    // dp[pos][covered] = fewest intervals covering at least covered of the first pos rows.
    // take[pos][covered] records whether the leftmost interval covering pos - 1 was used.
    long const inf = std::numeric_limits<long>::max() / 4;
    std::vector<std::vector<long>> dp(n + 1, std::vector<long>(target + 1, inf));
    std::vector<std::vector<bool>> take(n + 1, std::vector<bool>(target + 1, false));
    dp[0][0] = 0;

    for (size_t pos = 1; pos <= n; ++pos) {
        for (size_t covered = 0; covered <= target; ++covered) {
            dp[pos][covered] = dp[pos - 1][covered];
        }

        if (!best_interval_at_pos[pos - 1].has_value()) {
            continue;
        }

        auto const& interval = candidates[*best_interval_at_pos[pos - 1]];
        size_t const left = interval.left;
        size_t const interval_support = pos - left;
        for (size_t covered = 0; covered <= target; ++covered) {
            size_t const previous_covered =
                    (covered > interval_support) ? covered - interval_support : 0;
            if (dp[left][previous_covered] == inf) {
                continue;
            }
            long const with_interval = dp[left][previous_covered] + 1;
            if (with_interval < dp[pos][covered]) {
                dp[pos][covered] = with_interval;
                take[pos][covered] = true;
            }
        }
    }

    if (dp[n][target] == inf) {
        return {};
    }

    std::vector<SDCandidateInterval> result;
    size_t pos = n;
    size_t covered = target;
    while (pos > 0 && covered > 0) {
        if (!take[pos][covered]) {
            --pos;
            continue;
        }

        size_t const idx = *best_interval_at_pos[pos - 1];
        auto const& interval = candidates[idx];
        result.push_back(interval);
        size_t const interval_support = pos - interval.left;
        covered = (covered > interval_support) ? covered - interval_support : 0;
        pos = interval.left;
    }

    std::reverse(result.begin(), result.end());
    return result;
}

std::vector<SDCandidateInterval> SDMiner::AssembleGreedy(
        std::vector<SDCandidateInterval> const& candidates) const {
    size_t const n = sorted_y_values_.size();
    size_t const target = GetTargetSupport();
    if (target == 0 || candidates.empty()) {
        return {};
    }

    struct ActiveInterval {
        size_t left = 0;
        size_t right = 0;
        size_t prev = 0;
        size_t next = 0;
        bool active = false;
        size_t bucket = 0;
        std::list<size_t>::iterator bucket_it;
    };

    size_t constexpr invalid = std::numeric_limits<size_t>::max();

    // Active intervals hold only uncovered positions. Their indices identify original candidates.
    std::vector<ActiveInterval> active(candidates.size());
    std::vector<std::list<size_t>> buckets(n + 1);
    size_t head = active.empty() ? invalid : 0;
    size_t max_bucket = 0;

    auto length = [&](size_t idx) { return active[idx].right - active[idx].left + 1; };

    auto remove_from_bucket = [&](size_t idx) {
        if (active[idx].bucket != 0) {
            buckets[active[idx].bucket].erase(active[idx].bucket_it);
            active[idx].bucket = 0;
        }
    };

    auto add_to_bucket = [&](size_t idx) {
        size_t const len = length(idx);
        active[idx].bucket = len;
        buckets[len].push_back(idx);
        active[idx].bucket_it = std::prev(buckets[len].end());
        max_bucket = std::max(max_bucket, len);
    };

    auto unlink = [&](size_t idx) {
        size_t const prev = active[idx].prev;
        size_t const next = active[idx].next;
        if (prev != invalid) {
            active[prev].next = next;
        } else {
            head = next;
        }
        if (next != invalid) {
            active[next].prev = prev;
        }
        active[idx].prev = invalid;
        active[idx].next = invalid;
    };

    auto deactivate = [&](size_t idx) {
        if (!active[idx].active) {
            return;
        }
        remove_from_bucket(idx);
        unlink(idx);
        active[idx].active = false;
    };

    auto update_interval = [&](size_t idx, size_t new_left, size_t new_right) {
        remove_from_bucket(idx);
        if (new_left > new_right) {
            unlink(idx);
            active[idx].active = false;
            return;
        }
        active[idx].left = new_left;
        active[idx].right = new_right;
        add_to_bucket(idx);
    };

    auto delete_if_contained_by_neighbor = [&](size_t idx) {
        if (!active[idx].active) {
            return;
        }
        size_t const prev = active[idx].prev;
        if (prev != invalid && active[prev].left <= active[idx].left &&
            active[idx].right <= active[prev].right) {
            deactivate(idx);
            return;
        }
        size_t const next = active[idx].next;
        if (next != invalid && active[next].left <= active[idx].left &&
            active[idx].right <= active[next].right) {
            deactivate(idx);
        }
    };

    for (size_t pos = 0; pos < candidates.size(); ++pos) {
        auto const& candidate = candidates[pos];
        active[pos].left = candidate.left;
        active[pos].right = candidate.right;
        active[pos].prev = (pos == 0) ? invalid : pos - 1;
        active[pos].next = (pos + 1 == candidates.size()) ? invalid : pos + 1;
        active[pos].active = true;
        add_to_bucket(pos);
    }

    std::vector<SDCandidateInterval> result;
    std::vector<size_t> overlap;
    size_t covered_count = 0;

    while (covered_count < target && head != invalid) {
        while (max_bucket > 0 && buckets[max_bucket].empty()) {
            --max_bucket;
        }
        if (max_bucket == 0) {
            break;
        }

        size_t const chosen = buckets[max_bucket].front();
        size_t const left = active[chosen].left;
        size_t const right = active[chosen].right;
        size_t const prev = active[chosen].prev;
        size_t const next = active[chosen].next;
        size_t const gain = length(chosen);

        result.push_back(candidates[chosen]);
        covered_count += gain;
        deactivate(chosen);

        overlap.clear();
        for (size_t idx = prev; idx != invalid && active[idx].right >= left;) {
            size_t const previous = active[idx].prev;
            overlap.push_back(idx);
            idx = previous;
        }
        if (!overlap.empty()) {
            size_t const keep = overlap.back();
            for (size_t idx : overlap) {
                if (idx != keep) {
                    deactivate(idx);
                }
            }
            if (left == 0) {
                deactivate(keep);
            } else {
                update_interval(keep, active[keep].left, left - 1);
            }
            delete_if_contained_by_neighbor(keep);
        }

        overlap.clear();
        for (size_t idx = next; idx != invalid && active[idx].left <= right;) {
            size_t const following = active[idx].next;
            overlap.push_back(idx);
            idx = following;
        }
        if (!overlap.empty()) {
            size_t const keep = overlap.back();
            for (size_t idx : overlap) {
                if (idx != keep) {
                    deactivate(idx);
                }
            }
            if (right + 1 >= n) {
                deactivate(keep);
            } else {
                update_interval(keep, right + 1, active[keep].right);
            }
            delete_if_contained_by_neighbor(keep);
        }
    }

    return MinimizeSelectedIntervalCover(std::move(result));
}

void SDMiner::BuildTableau(std::vector<SDCandidateInterval> const& intervals, size_t support) {
    tableau_.clear();
    tableau_.reserve(intervals.size());
    for (auto const& interval : intervals) {
        tableau_.push_back(
                SDTableauPattern{interval.left, interval.right, sorted_x_values_[interval.left],
                                 sorted_x_values_[interval.right],
                                 interval.right - interval.left + 1, interval.confidence});
    }
    global_support_ = support;
}

void SDMiner::ExecuteInternal() {
    ExtractSortedData();

    if (sorted_y_values_.empty()) {
        candidates_.clear();
        tableau_.clear();
        global_support_ = 0;
        return;
    }

    candidates_ = (interval_strategy_ == IntervalStrategy::kExact) ? GenerateExactIntervals()
                                                                   : GenerateApproximateIntervals();
    std::vector<SDCandidateInterval> selected = (assembly_strategy_ == AssemblyStrategy::kExact)
                                                        ? AssembleExact(candidates_)
                                                        : AssembleGreedy(candidates_);
    size_t support = ComputeUnionSupport(selected);
    if (support < GetTargetSupport()) {
        selected.clear();
        support = 0;
    }
    BuildTableau(selected, support);
}

}  // namespace algos::sd_miner
