#include "cind_verifier.h"

#include <algorithm>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/algorithms/cind/condition.h"
#include "core/algorithms/cind/utils.h"
#include "core/config/conditions/completeness/option.h"
#include "core/config/conditions/condition_type/option.h"
#include "core/config/conditions/validity/option.h"
#include "core/config/descriptions.h"
#include "core/config/exceptions.h"
#include "core/config/indices/option.h"
#include "core/config/names.h"
#include "core/config/option.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_tables/option.h"
#include "core/model/table/column_combination.h"
#include "core/model/table/encoded_column_data.h"
#include "core/model/table/encoded_tables.h"
#include "core/model/table/tuple_index.h"

namespace algos::cind {
namespace {

using Columns = std::vector<model::EncodedColumnData const*>;
using Key = std::vector<int>;

constexpr std::string_view kAlternativesSeparator = ", ";

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

std::vector<std::string> SplitAlternatives(std::string const& value) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (true) {
        std::size_t const pos = value.find(kAlternativesSeparator, start);
        if (pos == std::string::npos) {
            parts.push_back(value.substr(start));
            return parts;
        }
        parts.push_back(value.substr(start, pos - start));
        start = pos + kAlternativesSeparator.size();
    }
}

Columns BuildOrderedColumns(model::ColumnEncodedRelationData const& enc,
                            config::IndicesType const& indices) {
    std::unordered_map<config::IndexType, model::EncodedColumnData const*> by_index;
    by_index.reserve(enc.GetColumnData().size());

    for (auto const& col : enc.GetColumnData()) {
        by_index.emplace(static_cast<config::IndexType>(col.GetColumn()->GetIndex()), &col);
    }

    Columns out;
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

Columns BuildConditionalColumns(model::ColumnEncodedRelationData const& enc,
                                std::unordered_set<config::IndexType> const& excluded) {
    Columns out;
    for (auto const& column : enc.GetColumnData()) {
        auto const col_index = static_cast<config::IndexType>(column.GetColumn()->GetIndex());
        if (!excluded.contains(col_index)) {
            out.push_back(&column);
        }
    }
    return out;
}

std::optional<int> LookUpCode(model::EncodedColumnData const& column, std::string const& value) {
    if (!column.GetUniqueValues().contains(value)) {
        return std::nullopt;
    }
    return column.GetValueDict().ToInt(value);
}

struct Pattern {
    std::vector<std::optional<int>> lhs_codes;
    std::vector<std::optional<std::unordered_set<int>>> rhs_codes;
    bool lhs_satisfiable{true};

    bool IsUnconditional() const {
        auto const is_set = [](auto const& slot) { return slot.has_value(); };
        return lhs_satisfiable && std::ranges::none_of(lhs_codes, is_set) &&
               std::ranges::none_of(rhs_codes, is_set);
    }

    bool ConstrainsRhs() const {
        return std::ranges::any_of(rhs_codes, [](auto const& slot) { return slot.has_value(); });
    }

    bool MatchesLhs(Columns const& lhs_conditional, std::size_t row) const {
        if (!lhs_satisfiable) return false;
        for (std::size_t i = 0; i < lhs_codes.size(); ++i) {
            if (lhs_codes[i].has_value() && lhs_conditional[i]->GetValue(row) != *lhs_codes[i]) {
                return false;
            }
        }
        return true;
    }

    bool MatchesRhs(Columns const& rhs_conditional, std::size_t row) const {
        for (std::size_t i = 0; i < rhs_codes.size(); ++i) {
            if (rhs_codes[i].has_value() &&
                !rhs_codes[i]->contains(rhs_conditional[i]->GetValue(row))) {
                return false;
            }
        }
        return true;
    }
};

Pattern CompilePattern(std::vector<std::string> values, Columns const& lhs_conditional,
                       Columns const& rhs_conditional) {
    std::size_t const lhs_size = lhs_conditional.size();
    std::size_t const rhs_size = rhs_conditional.size();
    if (values.size() != lhs_size && values.size() != lhs_size + rhs_size) {
        throw std::runtime_error(
                "A CIND condition must have one value per conditional attribute of the LHS "
                "table (" +
                std::to_string(lhs_size) + ") or of both tables (" +
                std::to_string(lhs_size + rhs_size) + "), got " + std::to_string(values.size()));
    }
    for (std::string& value : values) {
        value = StripQuotes(std::move(value));
    }

    Pattern pattern;
    pattern.lhs_codes.resize(lhs_size);
    pattern.rhs_codes.resize(rhs_size);

    for (std::size_t i = 0; i < lhs_size; ++i) {
        if (IsWildcard(values[i])) continue;
        std::optional<int> const code = LookUpCode(*lhs_conditional[i], values[i]);
        if (!code.has_value()) {
            pattern.lhs_satisfiable = false;
            return pattern;
        }
        pattern.lhs_codes[i] = *code;
    }

    for (std::size_t i = lhs_size; i < values.size(); ++i) {
        if (IsWildcard(values[i])) continue;
        model::EncodedColumnData const& column = *rhs_conditional[i - lhs_size];
        std::unordered_set<int> codes;
        for (std::string const& alternative : SplitAlternatives(values[i])) {
            if (std::optional<int> const code = LookUpCode(column, alternative)) {
                codes.insert(*code);
            }
        }
        pattern.rhs_codes[i - lhs_size] = std::move(codes);
    }
    return pattern;
}

class RhsIndex {
private:
    std::unordered_map<Key, std::vector<std::size_t>, utils::VecIntHash> rows_by_key_;

public:
    RhsIndex(Columns const& rhs_inclusion, bool keep_rows) {
        std::size_t const rows = rhs_inclusion.front()->GetNumRows();
        rows_by_key_.reserve(rows);
        for (std::size_t r = 0; r < rows; ++r) {
            std::vector<std::size_t>& bucket = rows_by_key_[utils::MakeKey(r, rhs_inclusion)];
            if (keep_rows) bucket.push_back(r);
        }
    }

    bool Contains(Key const& key) const {
        return rows_by_key_.contains(key);
    }

    std::vector<std::size_t> const& RowsOf(Key const& key) const {
        return rows_by_key_.at(key);
    }
};

}  // namespace

CINDVerifier::CINDVerifier() : condition_type_(CondType::kGroup) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTablesOpt.GetName()});
}

void CINDVerifier::RegisterOptions() {
    RegisterOption(config::kTablesOpt(&input_tables_));

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

    auto check_cind = [this](std::optional<CIND> const& cind) {
        if (cind.has_value()) CheckCind(*cind);
    };
    auto no_cind = [](std::optional<CIND> const& cind) { return !cind.has_value(); };
    RegisterOption(config::Option{&cind_, config::names::kCind, config::descriptions::kDCind,
                                  std::optional<CIND>{}}
                           .SetValueCheck(check_cind)
                           .SetConditionalOpts({{no_cind,
                                                 {config::kLhsIndicesOpt.GetName(),
                                                  config::kRhsIndicesOpt.GetName(),
                                                  config::names::kCindCondValues}}}));

    RegisterOption(config::kValidityOpt(&min_validity_));
    RegisterOption(config::kCompletenessOpt(&min_completeness_));
    RegisterOption(config::kConditionTypeOpt(&condition_type_));
}

void CINDVerifier::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({
            config::names::kCind,
            config::kValidityOpt.GetName(),
            config::kCompletenessOpt.GetName(),
            config::kConditionTypeOpt.GetName(),
    });
}

void CINDVerifier::CheckCind(CIND const& cind) const {
    model::ColumnCombination const& lhs = cind.ind.GetLhs();
    model::ColumnCombination const& rhs = cind.ind.GetRhs();

    std::size_t const tables = input_tables_.size();
    for (model::TableIndex const table : {lhs.GetTableIndex(), rhs.GetTableIndex()}) {
        if (table >= tables) {
            throw config::ConfigurationError{
                    "Invalid input: the CIND refers to table " + std::to_string(table) +
                    ", but only " + std::to_string(tables) +
                    " table(s) were loaded. Load the tables used for mining in the same order."};
        }
    }

    if (lhs.GetColumnIndices().empty() ||
        lhs.GetColumnIndices().size() != rhs.GetColumnIndices().size()) {
        throw config::ConfigurationError{
                "Invalid input: LHS and RHS of the CIND must have the same non-zero number of "
                "columns"};
    }

    for (model::ColumnCombination const* side : {&lhs, &rhs}) {
        auto const columns = input_tables_[side->GetTableIndex()]->GetNumberOfColumns();
        for (model::ColumnIndex const index : side->GetColumnIndices()) {
            if (index >= columns) {
                throw config::ConfigurationError{
                        "Invalid input: column index " + std::to_string(index) +
                        " is out of bounds for table " + std::to_string(side->GetTableIndex())};
            }
        }
    }
}

std::vector<std::vector<std::string>> CINDVerifier::GetConditions() const {
    if (cind_.has_value()) {
        std::vector<std::vector<std::string>> conditions;
        conditions.reserve(cind_->conditions.size());
        for (Condition const& condition : cind_->conditions) {
            conditions.push_back(condition.condition_attrs_values);
        }
        return conditions;
    }
    if (condition_values_.empty()) {
        return {};
    }
    return {condition_values_};
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

void CINDVerifier::ExecuteInternal() {
    model::TableIndex lhs_table_idx = 0;
    model::TableIndex rhs_table_idx = static_cast<model::TableIndex>(input_tables_.size() - 1);
    config::IndicesType lhs_indices = ind_.lhs;
    config::IndicesType rhs_indices = ind_.rhs;
    if (cind_.has_value()) {
        model::ColumnCombination const& lhs = cind_->ind.GetLhs();
        model::ColumnCombination const& rhs = cind_->ind.GetRhs();
        lhs_table_idx = lhs.GetTableIndex();
        rhs_table_idx = rhs.GetTableIndex();
        lhs_indices.assign(lhs.GetColumnIndices().begin(), lhs.GetColumnIndices().end());
        rhs_indices.assign(rhs.GetColumnIndices().begin(), rhs.GetColumnIndices().end());
    }

    auto const& lhs_enc = encoded_tables_->GetTable(lhs_table_idx);
    auto const& rhs_enc = encoded_tables_->GetTable(rhs_table_idx);

    bool const same_table = (lhs_table_idx == rhs_table_idx);

    std::unordered_set<config::IndexType> lhs_set(lhs_indices.begin(), lhs_indices.end());
    std::unordered_set<config::IndexType> rhs_set(rhs_indices.begin(), rhs_indices.end());

    Columns const lhs_inclusion = BuildOrderedColumns(lhs_enc, lhs_indices);
    Columns const rhs_inclusion = BuildOrderedColumns(rhs_enc, rhs_indices);
    if (lhs_inclusion.front()->GetNumRows() == 0) {
        throw std::runtime_error("Got an empty LHS table: CIND verification is meaningless.");
    }

    std::unordered_set<config::IndexType> lhs_excluded = lhs_set;
    std::unordered_set<config::IndexType> rhs_excluded = rhs_set;
    if (same_table) {
        lhs_excluded.insert(rhs_set.begin(), rhs_set.end());
        rhs_excluded.insert(lhs_set.begin(), lhs_set.end());
    }
    Columns const lhs_conditional = BuildConditionalColumns(lhs_enc, lhs_excluded);
    Columns const rhs_conditional = BuildConditionalColumns(rhs_enc, rhs_excluded);

    std::vector<Pattern> patterns;
    for (std::vector<std::string>& values : GetConditions()) {
        patterns.push_back(CompilePattern(std::move(values), lhs_conditional, rhs_conditional));
    }

    bool const is_group = (condition_type_ == CondType::kGroup);
    bool const unconditional =
            patterns.empty() || std::ranges::any_of(patterns, &Pattern::IsUnconditional);
    bool const constrains_rhs = std::ranges::any_of(patterns, &Pattern::ConstrainsRhs);

    RhsIndex const rhs_index(rhs_inclusion, constrains_rhs);

    std::size_t const lhs_rows = lhs_inclusion.front()->GetNumRows();

    if (unconditional) {
        // Simple inclusion check: every LHS row key must appear in RHS keys
        std::unordered_map<Key, Cluster, utils::VecIntHash> violating_map;
        std::size_t distinct_keys = 0;

        {
            std::unordered_set<Key, utils::VecIntHash> seen_keys;
            seen_keys.reserve(lhs_rows);
            for (std::size_t l = 0; l < lhs_rows; ++l) {
                Key key = utils::MakeKey(l, lhs_inclusion);
                seen_keys.insert(key);
                if (!rhs_index.Contains(key)) {
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

    struct Acc {
        bool included{false};
        Cluster basket_rows;
        Cluster matching_rows;
        Cluster violating_rows;
    };

    std::unordered_map<Key, Acc, utils::VecIntHash> acc_by_key;
    acc_by_key.reserve(lhs_rows);

    for (std::size_t l = 0; l < lhs_rows; ++l) {
        Key key = utils::MakeKey(l, lhs_inclusion);

        auto it = acc_by_key.find(key);
        if (it == acc_by_key.end()) {
            bool const included = rhs_index.Contains(key);
            it = acc_by_key
                         .emplace(key, Acc{.included = included,
                                           .basket_rows = {},
                                           .matching_rows = {},
                                           .violating_rows = {}})
                         .first;
        }
        Acc& acc = it->second;
        acc.basket_rows.push_back(static_cast<model::TupleIndex>(l));

        bool matched = false;
        bool included = false;
        for (Pattern const& pattern : patterns) {
            if (!pattern.MatchesLhs(lhs_conditional, l)) continue;
            matched = true;
            if (!acc.included) break;
            if (!pattern.ConstrainsRhs() ||
                std::ranges::any_of(rhs_index.RowsOf(key), [&](std::size_t rhs_row) {
                    return pattern.MatchesRhs(rhs_conditional, rhs_row);
                })) {
                included = true;
                break;
            }
        }
        if (!matched) continue;

        acc.matching_rows.push_back(static_cast<model::TupleIndex>(l));
        if (!included) {
            acc.violating_rows.push_back(static_cast<model::TupleIndex>(l));
        }
    }

    // Count included baskets total
    for (auto const& [_, acc] : acc_by_key) {
        if (acc.included) {
            included_baskets_total_ += is_group ? 1 : acc.basket_rows.size();
        }
    }

    // Compute support and violations
    for (auto& [_, acc] : acc_by_key) {
        if (acc.matching_rows.empty()) continue;

        supporting_baskets_ += is_group ? 1 : acc.matching_rows.size();

        if (acc.violating_rows.empty()) {
            included_support_ += is_group ? 1 : acc.matching_rows.size();
            continue;
        }

        if (!is_group) {
            included_support_ += acc.matching_rows.size() - acc.violating_rows.size();
        }
        violating_rows_ += acc.violating_rows.size();
        violating_clusters_.push_back(ViolatingCluster{
                .basket_rows = std::move(acc.basket_rows),
                .violating_rows = std::move(acc.violating_rows),
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
