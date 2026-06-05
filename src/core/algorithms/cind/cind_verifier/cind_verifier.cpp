#include "cind_verifier.h"

#include <algorithm>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/container_hash/hash.hpp>
#include <boost/functional/hash.hpp>

#include "core/algorithms/cind/condition.h"
#include "core/config/conditions/completeness/option.h"
#include "core/config/conditions/condition_type/option.h"
#include "core/config/conditions/validity/option.h"
#include "core/config/descriptions.h"
#include "core/config/indices/option.h"
#include "core/config/names.h"
#include "core/config/option.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_tables/option.h"
#include "core/model/table/encoded_column_data.h"
#include "core/model/table/encoded_tables.h"
#include "core/model/table/tuple_index.h"
#include "core/util/timed_invoke.h"

namespace algos::cind {
namespace {

struct VecIntHash {
    std::size_t operator()(std::vector<int> const& v) const noexcept {
        return boost::hash_value(v);
    }
};

inline bool IsWildcard(std::string const& s) {
    return s == kAnyValue || s == "_";
}

inline std::string StripQuotes(std::string s) {
    if (s.size() >= 2 &&
        ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

std::vector<int> MakeKey(std::size_t row,
                         std::vector<model::EncodedColumnData const*> const& cols) {
    std::vector<int> key;
    key.reserve(cols.size());
    for (auto const* c : cols) {
        key.push_back(c->GetValue(row));
    }
    return key;
}

std::vector<model::EncodedColumnData const*> BuildOrderedColumns(
        model::ColumnEncodedRelationData const& enc, config::IndicesType const& indices) {
    std::unordered_map<config::IndexType, model::EncodedColumnData const*> by_index;
    by_index.reserve(enc.GetColumnData().size());

    for (auto const& col : enc.GetColumnData()) {
        by_index.emplace(static_cast<config::IndexType>(col.GetColumn()->GetIndex()), &col);
    }

    std::vector<model::EncodedColumnData const*> out;
    out.reserve(indices.size());
    for (auto idx : indices) {
        auto it = by_index.find(idx);
        if (it == by_index.end()) {
            throw std::runtime_error("Internal error: inclusion index not found in encoded table");
        }
        out.push_back(it->second);
    }
    return out;
}

}  // namespace

CINDVerifier::CINDVerifier() : condition_type_(CondType::kGroup) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTablesOpt.GetName()});
}

void CINDVerifier::RegisterOptions() {
    model::TableIndex const table_limit = 2;
    RegisterOption(config::kTablesOpt(&input_tables_, table_limit));

    RegisterOption(config::kLhsIndicesOpt(
            &ind_.lhs, [this]() { return input_tables_.front()->GetNumberOfColumns(); },
            [&rhs = ind_.rhs](config::IndicesType const& indices) {
                if (!rhs.empty() && rhs.size() != indices.size()) {
                    throw config::ConfigurationError{
                            "Invalid input: LHS and RHS indices must have the same size"};
                }
            }));

    RegisterOption(config::kRhsIndicesOpt(
            &ind_.rhs, [this]() { return input_tables_.back()->GetNumberOfColumns(); },
            [&lhs = ind_.lhs](config::IndicesType const& indices) {
                if (!lhs.empty() && lhs.size() != indices.size()) {
                    throw config::ConfigurationError{
                            "Invalid input: LHS and RHS indices must have the same size"};
                }
            }));

    RegisterOption(config::Option{&condition_values_, config::names::kCindCondValues,
                                  config::descriptions::kDCindCondValues,
                                  std::vector<std::string>{}});

    RegisterOption(config::kValidityOpt(&min_validity_));
    RegisterOption(config::kCompletenessOpt(&min_completeness_));
    RegisterOption(config::kConditionTypeOpt(&condition_type_));
}

void CINDVerifier::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({
            config::kLhsIndicesOpt.GetName(),
            config::kRhsIndicesOpt.GetName(),
            config::names::kCindCondValues,
            config::kValidityOpt.GetName(),
            config::kCompletenessOpt.GetName(),
            config::kConditionTypeOpt.GetName(),
    });
}

void CINDVerifier::ResetState() {
    violating_clusters_.clear();
    violating_rows_ = 0;

    supporting_baskets_ = 0;
    included_support_ = 0;
    included_baskets_total_ = 0;

    real_validity_ = -1.0;
    real_completeness_ = 0.0;
}

void CINDVerifier::LoadDataInternal() {
    encoded_tables_ = std::make_unique<model::EncodedTables>(input_tables_);

    model::TableIndex const lhs_table_idx = 0;
    auto const& lhs_enc = encoded_tables_->GetTable(lhs_table_idx);
    if (lhs_enc.GetColumnData().empty() || lhs_enc.GetColumnData().front().GetNumRows() == 0) {
        throw std::runtime_error("Got an empty LHS table: CIND verification is meaningless.");
    }
}

unsigned long long CINDVerifier::ExecuteInternal() {
    return util::TimedInvoke(&CINDVerifier::VerifyCIND, this);
}

void CINDVerifier::VerifyCIND() {
    model::TableIndex const lhs_table_idx = 0;
    model::TableIndex const rhs_table_idx =
            (input_tables_.size() == 1) ? 0
                                        : static_cast<model::TableIndex>(input_tables_.size() - 1);

    auto const& lhs_enc = encoded_tables_->GetTable(lhs_table_idx);
    auto const& rhs_enc = encoded_tables_->GetTable(rhs_table_idx);

    bool const same_table = (lhs_table_idx == rhs_table_idx);

    std::unordered_set<config::IndexType> lhs_set(ind_.lhs.begin(), ind_.lhs.end());
    std::unordered_set<config::IndexType> rhs_set(ind_.rhs.begin(), ind_.rhs.end());

    std::vector<model::EncodedColumnData const*> lhs_inclusion =
            BuildOrderedColumns(lhs_enc, ind_.lhs);
    std::vector<model::EncodedColumnData const*> rhs_inclusion =
            BuildOrderedColumns(rhs_enc, ind_.rhs);

    std::vector<model::EncodedColumnData const*> conditional;
    for (auto const& column : lhs_enc.GetColumnData()) {
        auto const col_index = static_cast<config::IndexType>(column.GetColumn()->GetIndex());
        if (lhs_set.contains(col_index) || (same_table && rhs_set.contains(col_index))) {
            continue;
        }
        conditional.push_back(&column);
    }

    std::vector<std::string> cond_vals = condition_values_;
    if (cond_vals.empty()) {
        cond_vals.assign(conditional.size(), kAnyValue);
    } else if (cond_vals.size() != conditional.size()) {
        throw std::runtime_error(
                "cind_condition_values size must equal number of conditional attributes");
    }
    for (auto& s : cond_vals) {
        s = StripQuotes(s);
    }

    bool const all_wildcards =
            std::ranges::all_of(cond_vals, [](std::string const& s) { return IsWildcard(s); });

    // Build RHS key set
    std::unordered_set<std::vector<int>, VecIntHash> rhs_keys;
    {
        std::size_t rhs_rows = rhs_inclusion.front()->GetNumRows();
        rhs_keys.reserve(rhs_rows);
        for (std::size_t r = 0; r < rhs_rows; ++r) {
            rhs_keys.insert(MakeKey(r, rhs_inclusion));
        }
    }

    if (all_wildcards) {
        // Simple inclusion check: every LHS row key must appear in RHS keys
        std::unordered_map<std::vector<int>, Cluster, VecIntHash> violating_map;
        std::size_t const lhs_rows = lhs_inclusion.front()->GetNumRows();
        std::size_t distinct_keys = 0;

        {
            std::unordered_set<std::vector<int>, VecIntHash> seen_keys;
            seen_keys.reserve(lhs_rows);
            for (std::size_t l = 0; l < lhs_rows; ++l) {
                auto key = MakeKey(l, lhs_inclusion);
                seen_keys.insert(key);
                if (!rhs_keys.contains(key)) {
                    violating_map[std::move(key)].push_back(static_cast<model::TupleIndex>(l));
                    ++violating_rows_;
                }
            }
            distinct_keys = seen_keys.size();
        }

        std::transform(violating_map.begin(), violating_map.end(),
                       std::back_inserter(violating_clusters_), [](auto& kv) {
                           auto& cluster = kv.second;
                           return ViolatingCluster{.basket_rows = cluster,
                                                   .violating_rows = std::move(cluster)};
                       });

        bool const is_group = (condition_type_ == CondType::kGroup);
        supporting_baskets_ = is_group ? distinct_keys : lhs_rows;
        std::size_t const violating = is_group ? violating_clusters_.size() : violating_rows_;
        included_support_ = supporting_baskets_ - violating;
        included_baskets_total_ = included_support_;

        real_validity_ = (supporting_baskets_ == 0)
                                 ? -1.0
                                 : static_cast<double>(included_support_) /
                                           static_cast<double>(supporting_baskets_);

        real_completeness_ = (included_baskets_total_ == 0)
                                     ? 0.0
                                     : static_cast<double>(included_support_) /
                                               static_cast<double>(included_baskets_total_);
        return;
    }

    // Condition path: build expected codes from condition values
    std::vector<std::optional<int>> expected_codes;
    expected_codes.reserve(conditional.size());

    for (std::size_t i = 0; i < conditional.size(); ++i) {
        auto const& want = cond_vals[i];
        if (IsWildcard(want)) {
            expected_codes.push_back(std::nullopt);
            continue;
        }

        auto const* col = conditional[i];
        if (!col->GetUniqueValues().contains(want)) {
            real_validity_ = -1.0;
            real_completeness_ = 0.0;
            return;
        }

        expected_codes.push_back(col->GetValueDict().ToInt(want));
    }

    auto row_matches = [&](std::size_t row) -> bool {
        for (std::size_t i = 0; i < conditional.size(); ++i) {
            if (!expected_codes[i].has_value()) continue;
            if (conditional[i]->GetValue(row) != *expected_codes[i]) return false;
        }
        return true;
    };

    using Key = std::vector<int>;

    struct Acc {
        bool included{false};
        Cluster basket_rows;
        Cluster matching_rows;
    };

    std::unordered_map<Key, Acc, VecIntHash> acc_by_key;
    {
        std::size_t lhs_rows = lhs_inclusion.front()->GetNumRows();
        acc_by_key.reserve(lhs_rows);

        for (std::size_t l = 0; l < lhs_rows; ++l) {
            auto key = MakeKey(l, lhs_inclusion);

            auto it = acc_by_key.find(key);
            if (it == acc_by_key.end()) {
                bool included = rhs_keys.contains(key);
                it = acc_by_key
                             .emplace(std::move(key), Acc{.included = included,
                                                          .basket_rows = {},
                                                          .matching_rows = {}})
                             .first;
            }

            it->second.basket_rows.push_back(static_cast<model::TupleIndex>(l));
            if (row_matches(l)) {
                it->second.matching_rows.push_back(static_cast<model::TupleIndex>(l));
            }
        }
    }

    // Count included baskets total
    for (auto const& [_, acc] : acc_by_key) {
        if (acc.included) {
            included_baskets_total_ +=
                    (condition_type_ == CondType::kGroup) ? 1 : acc.basket_rows.size();
        }
    }

    // Compute support and violations
    bool const is_group = (condition_type_ == CondType::kGroup);
    for (auto& [_, acc] : acc_by_key) {
        if (acc.matching_rows.empty()) continue;

        std::size_t const contribution = is_group ? 1 : acc.matching_rows.size();

        supporting_baskets_ += contribution;

        if (acc.included) {
            included_support_ += contribution;
            continue;
        }

        violating_rows_ += acc.matching_rows.size();
        violating_clusters_.push_back(ViolatingCluster{
                .basket_rows = std::move(acc.basket_rows),
                .violating_rows = std::move(acc.matching_rows),
        });
    }

    real_validity_ = (supporting_baskets_ == 0) ? -1.0
                                                : static_cast<double>(included_support_) /
                                                          static_cast<double>(supporting_baskets_);

    real_completeness_ = (included_baskets_total_ == 0)
                                 ? 0.0
                                 : static_cast<double>(included_support_) /
                                           static_cast<double>(included_baskets_total_);
}

}  // namespace algos::cind
