#include "cure_cind.h"

#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <boost/container_hash/hash.hpp>

namespace algos::cind {

namespace {
struct VecIntHash {
    std::size_t operator()(std::vector<int> const& vec) const noexcept {
        return boost::hash_value(vec);
    }
};

struct PairIntHash {
    std::size_t operator()(std::pair<int, int> const& p) const noexcept {
        std::size_t seed = 0;
        boost::hash_combine(seed, p.first);
        boost::hash_combine(seed, p.second);
        return seed;
    }
};

std::vector<int> MakeKey(std::size_t row, AttrsType const& cols) {
    std::vector<int> key;
    key.reserve(cols.size());
    for (auto const* c : cols) {
        key.push_back(c->GetValue(row));
    }
    return key;
}
}  // namespace

CureCind::CureCind(config::InputTables& input_tables) : CindMiner(input_tables) {}

CureCind::CureAttributes CureCind::ClassifyCureAttributes(model::IND const& aind) const {
    CureAttributes result;

    auto const& lhs = aind.GetLhs();
    auto const& rhs = aind.GetRhs();

    auto const& lhs_indices = lhs.GetColumnIndices();
    auto const& rhs_indices = rhs.GetColumnIndices();

    auto const lhs_table = lhs.GetTableIndex();
    auto const rhs_table = rhs.GetTableIndex();
    bool const same_table = (lhs_table == rhs_table);

    std::unordered_set<model::ColumnIndex> lhs_set(lhs_indices.begin(), lhs_indices.end());
    std::unordered_set<model::ColumnIndex> rhs_set(rhs_indices.begin(), rhs_indices.end());

    for (auto const& column : tables_.GetTable(lhs_table).GetColumnData()) {
        auto const col_index = column.GetColumn()->GetIndex();
        if (lhs_set.contains(col_index)) {
            result.lhs_inclusion.push_back(&column);
        } else if (same_table && rhs_set.contains(col_index)) {
            continue;
        } else {
            result.lhs_conditional.push_back(&column);
        }
    }

    for (auto const& column : tables_.GetTable(rhs_table).GetColumnData()) {
        auto const col_index = column.GetColumn()->GetIndex();
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

    std::unordered_map<std::vector<int>, std::vector<std::size_t>, VecIntHash> rhs_index;
    std::size_t const rhs_rows = attrs.rhs_inclusion.front()->GetNumRows();
    rhs_index.reserve(rhs_rows);
    for (std::size_t r = 0; r < rhs_rows; ++r) {
        rhs_index[MakeKey(r, attrs.rhs_inclusion)].push_back(r);
    }

    std::size_t const lhs_rows = attrs.lhs_inclusion.front()->GetNumRows();
    std::vector<PatternPair> patterns;

    for (std::size_t li = 0; li < attrs.lhs_conditional.size(); ++li) {
        auto const* lhs_attr = attrs.lhs_conditional[li];

        for (std::size_t ri = 0; ri < attrs.rhs_conditional.size(); ++ri) {
            auto const* rhs_attr = attrs.rhs_conditional[ri];

            std::unordered_map<std::pair<int, int>, std::size_t, PairIntHash> pair_counts;

            for (std::size_t lhs_row = 0; lhs_row < lhs_rows; ++lhs_row) {
                auto key = MakeKey(lhs_row, attrs.lhs_inclusion);
                auto it = rhs_index.find(key);
                if (it == rhs_index.end()) continue;

                int const lv = lhs_attr->GetValue(lhs_row);
                for (std::size_t rhs_row : it->second) {
                    int const rv = rhs_attr->GetValue(rhs_row);
                    pair_counts[{lv, rv}]++;
                }
            }

            for (auto const& [pair, count] : pair_counts) {
                if (count >= min_support_) {
                    patterns.push_back({li, pair.first, ri, pair.second, count});
                }
            }
        }
    }

    return patterns;
}

std::vector<Condition> CureCind::MinimalCover(std::vector<PatternPair> const& patterns,
                                              CureAttributes const& attrs,
                                              std::size_t total_joined) {
    std::size_t const lhs_cond_size = attrs.lhs_conditional.size();
    std::size_t const rhs_cond_size = attrs.rhs_conditional.size();
    std::size_t const total_attrs = lhs_cond_size + rhs_cond_size;

    struct CoverEntry {
        std::vector<std::string> values;
        std::size_t support{0};
    };

    using CoverKey = std::pair<std::size_t, int>;
    std::unordered_map<CoverKey, CoverEntry, PairIntHash> cover;

    for (auto const& p : patterns) {
        CoverKey key{p.lhs_attr_idx, p.lhs_value};
        auto it = cover.find(key);

        if (it == cover.end()) {
            CoverEntry entry;
            entry.values.resize(total_attrs, kAnyValue);
            entry.values[p.lhs_attr_idx] =
                    attrs.lhs_conditional[p.lhs_attr_idx]->DecodeValue(p.lhs_value);
            entry.values[lhs_cond_size + p.rhs_attr_idx] =
                    attrs.rhs_conditional[p.rhs_attr_idx]->DecodeValue(p.rhs_value);
            entry.support = p.support;
            cover.emplace(key, std::move(entry));
        } else {
            auto& entry = it->second;
            std::size_t const rhs_pos = lhs_cond_size + p.rhs_attr_idx;
            std::string const rhs_decoded =
                    attrs.rhs_conditional[p.rhs_attr_idx]->DecodeValue(p.rhs_value);

            if (entry.values[rhs_pos] == kAnyValue) {
                entry.values[rhs_pos] = rhs_decoded;
            } else if (entry.values[rhs_pos].find(rhs_decoded) == std::string::npos) {
                // Disjunction: append with comma
                entry.values[rhs_pos] += "," + rhs_decoded;
            }
            entry.support += p.support;
        }
    }

    std::vector<Condition> conditions;
    conditions.reserve(cover.size());

    for (auto& [key, entry] : cover) {
        double const validity =
                (total_joined > 0) ? static_cast<double>(entry.support) / total_joined : 0.0;
        double const completeness =
                (total_joined > 0) ? static_cast<double>(entry.support) / total_joined : 0.0;
        conditions.emplace_back(std::move(entry.values), validity, completeness);
    }

    return conditions;
}

CIND CureCind::ExecuteSingle(model::IND const& aind) {
    auto attrs = ClassifyCureAttributes(aind);

    auto patterns = DiscoverPatterns(attrs);

    // Total joined tuples used as denominator for validity/completeness.
    std::size_t total_joined = 0;
    for (auto const& p : patterns) {
        total_joined += p.support;
    }

    auto conditions = MinimalCover(patterns, attrs, total_joined);

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
