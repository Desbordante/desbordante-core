#include "core/algorithms/cind/condition_miners/cure_cind.h"

#include <algorithm>
#include <functional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <boost/container_hash/hash.hpp>

#include "core/algorithms/cind/utils.h"

namespace algos::cind {

namespace {
struct PairIntHash {
    std::size_t operator()(std::pair<int, int> const& p) const noexcept {
        std::size_t seed = 0;
        boost::hash_combine(seed, p.first);
        boost::hash_combine(seed, p.second);
        return seed;
    }
};
}  // namespace

CureCind::CureCind(config::InputTables& input_tables) : CindMiner(input_tables) {}

CureCind::CureAttributes CureCind::ClassifyCureAttributes(model::IND const& aind) const {
    CureAttributes result;

    model::ColumnCombination const& lhs = aind.GetLhs();
    model::ColumnCombination const& rhs = aind.GetRhs();

    std::vector<model::ColumnIndex> const& lhs_indices = lhs.GetColumnIndices();
    std::vector<model::ColumnIndex> const& rhs_indices = rhs.GetColumnIndices();

    model::TableIndex const lhs_table = lhs.GetTableIndex();
    model::TableIndex const rhs_table = rhs.GetTableIndex();
    bool const same_table = (lhs_table == rhs_table);

    std::unordered_set<model::ColumnIndex> lhs_set(lhs_indices.begin(), lhs_indices.end());
    std::unordered_set<model::ColumnIndex> rhs_set(rhs_indices.begin(), rhs_indices.end());

    for (model::EncodedColumnData const& column : tables_.GetTable(lhs_table).GetColumnData()) {
        model::ColumnIndex const col_index = column.GetColumn()->GetIndex();
        if (lhs_set.contains(col_index)) {
            result.lhs_inclusion.push_back(&column);
        } else if (same_table && rhs_set.contains(col_index)) {
            continue;
        } else {
            result.lhs_conditional.push_back(&column);
        }
    }

    for (model::EncodedColumnData const& column : tables_.GetTable(rhs_table).GetColumnData()) {
        model::ColumnIndex const col_index = column.GetColumn()->GetIndex();
        if (rhs_set.contains(col_index)) {
            result.rhs_inclusion.push_back(&column);
        } else if (same_table && lhs_set.contains(col_index)) {
            continue;
        } else {
            result.rhs_conditional.push_back(&column);
        }
    }

    return result;
}

std::vector<CureCind::PatternPair> CureCind::DiscoverPatterns(CureAttributes const& attrs) {
    if (attrs.lhs_conditional.empty() || attrs.rhs_conditional.empty()) {
        return {};
    }

    std::unordered_map<std::vector<int>, std::vector<std::size_t>, utils::VecIntHash> rhs_index;
    std::size_t const rhs_rows = attrs.rhs_inclusion.front()->GetNumRows();
    rhs_index.reserve(rhs_rows);
    for (std::size_t r = 0; r < rhs_rows; ++r) {
        rhs_index[utils::MakeKey(r, attrs.rhs_inclusion)].push_back(r);
    }

    std::size_t const lhs_rows = attrs.lhs_inclusion.front()->GetNumRows();

    std::vector<std::vector<std::size_t> const*> matching_rhs_rows(lhs_rows, nullptr);
    for (std::size_t lhs_row = 0; lhs_row < lhs_rows; ++lhs_row) {
        auto it = rhs_index.find(utils::MakeKey(lhs_row, attrs.lhs_inclusion));
        if (it != rhs_index.end()) {
            matching_rhs_rows[lhs_row] = &it->second;
        }
    }

    std::vector<PatternPair> patterns;

    for (std::size_t li = 0; li < attrs.lhs_conditional.size(); ++li) {
        model::EncodedColumnData const* lhs_attr = attrs.lhs_conditional[li];

        for (std::size_t ri = 0; ri < attrs.rhs_conditional.size(); ++ri) {
            model::EncodedColumnData const* rhs_attr = attrs.rhs_conditional[ri];

            std::unordered_map<std::pair<int, int>, std::size_t, PairIntHash> pair_counts;

            for (std::size_t lhs_row = 0; lhs_row < lhs_rows; ++lhs_row) {
                std::vector<std::size_t> const* rhs_rows_for_lhs = matching_rhs_rows[lhs_row];
                if (rhs_rows_for_lhs == nullptr) continue;

                int const lv = lhs_attr->GetValue(lhs_row);
                for (std::size_t rhs_row : *rhs_rows_for_lhs) {
                    int const rv = rhs_attr->GetValue(rhs_row);
                    ++pair_counts[{lv, rv}];
                }
            }

            for (auto const& [pair, count] : pair_counts) {
                if (count >= min_support_) {
                    patterns.push_back({li, pair.first, ri, pair.second, count});
                }
            }
        }
    }

    std::ranges::sort(patterns, [](PatternPair const& a, PatternPair const& b) {
        return std::tie(a.lhs_attr_idx, a.lhs_value, a.rhs_attr_idx, a.rhs_value) <
               std::tie(b.lhs_attr_idx, b.lhs_value, b.rhs_attr_idx, b.rhs_value);
    });

    return patterns;
}

std::vector<Condition> CureCind::MinimalCover(std::vector<PatternPair> const& patterns,
                                              CureAttributes const& attrs) {
    std::size_t const lhs_cond_size = attrs.lhs_conditional.size();
    std::size_t const rhs_cond_size = attrs.rhs_conditional.size();
    std::size_t const total_attrs = lhs_cond_size + rhs_cond_size;

    std::size_t total_joined = 0;
    for (PatternPair const& p : patterns) {
        total_joined += p.support;
    }

    struct CoverEntry {
        std::vector<std::string> values;
        std::vector<std::unordered_set<std::string>> rhs_seen;
        std::size_t support{0};

        CoverEntry(std::size_t total_attrs, std::size_t rhs_cond_size)
            : values(total_attrs, kAnyValue), rhs_seen(rhs_cond_size) {}
    };

    using CoverKey = std::pair<std::size_t, int>;
    std::unordered_map<CoverKey, CoverEntry, PairIntHash> cover;

    for (PatternPair const& p : patterns) {
        auto [it, inserted] = cover.try_emplace(CoverKey{p.lhs_attr_idx, p.lhs_value}, total_attrs,
                                                rhs_cond_size);
        CoverEntry& entry = it->second;
        if (inserted) {
            entry.values[p.lhs_attr_idx] =
                    attrs.lhs_conditional[p.lhs_attr_idx]->DecodeValue(p.lhs_value);
        }

        std::string rhs_decoded = attrs.rhs_conditional[p.rhs_attr_idx]->DecodeValue(p.rhs_value);
        if (entry.rhs_seen[p.rhs_attr_idx].insert(rhs_decoded).second) {
            std::string& slot = entry.values[lhs_cond_size + p.rhs_attr_idx];
            if (slot == kAnyValue) {
                slot = std::move(rhs_decoded);
            } else {
                slot += ", " + rhs_decoded;
            }
        }

        entry.support += p.support;
    }

    std::vector<Condition> conditions;
    conditions.reserve(cover.size());

    for (auto& [_, entry] : cover) {
        double const share =
                (total_joined > 0) ? static_cast<double>(entry.support) / total_joined : 0.0;
        conditions.emplace_back(std::move(entry.values), share, share);
    }

    std::ranges::sort(conditions, std::less{}, &Condition::condition_attrs_values);

    return conditions;
}

CIND CureCind::ExecuteSingle(model::IND const& aind) {
    CureAttributes attrs = ClassifyCureAttributes(aind);
    std::vector<PatternPair> patterns = DiscoverPatterns(attrs);
    std::vector<Condition> conditions = MinimalCover(patterns, attrs);

    AttrsType all_cond;
    all_cond.reserve(attrs.lhs_conditional.size() + attrs.rhs_conditional.size());
    all_cond.insert(all_cond.end(), attrs.lhs_conditional.begin(), attrs.lhs_conditional.end());
    all_cond.insert(all_cond.end(), attrs.rhs_conditional.begin(), attrs.rhs_conditional.end());

    CIND cind{.ind = aind,
              .conditions = std::move(conditions),
              .conditional_attributes = GetConditionalAttributesNames(all_cond)};
    return cind;
}

}  // namespace algos::cind
