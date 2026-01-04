#include "cind_verifier.h"

#include <algorithm>
#include <optional>
#include <set>
#include <sstream>
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
#include "core/config/indices/option.h"
#include "core/config/option.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_tables/option.h"
#include "core/model/table/dataset_stream_fixed.h"
#include "core/model/table/dataset_stream_projection.h"
#include "core/model/table/encoded_column_data.h"
#include "core/model/table/encoded_tables.h"
#include "core/model/table/tuple_index.h"
#include "core/util/timed_invoke.h"

namespace algos::cind {
namespace {

static constexpr char kCindCondValues[] = "cind_condition_values";
static constexpr char kDCindCondValues[] =
        "Condition values aligned with conditional attributes order from "
        "CindMiner::ClassifyAttributes. "
        "Use '-' or '_' as wildcard. If empty => all wildcards.";

struct VecIntHash {
    std::size_t operator()(std::vector<int> const& v) const noexcept {
        return boost::hash_value(v);
    }
};

struct VecStrHash {
    std::size_t operator()(std::vector<std::string> const& v) const noexcept {
        return boost::hash_value(v);
    }
};

inline bool IsWildcard(std::string const& s) {
    return s == "-" || s == "_" || s == kAnyValue;
}

inline std::string StripQuotes(std::string s) {
    if (s.size() >= 2 &&
        ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

std::vector<std::string> MakeKeyDecoded(std::size_t row,
                                        std::vector<model::EncodedColumnData const*> const& cols) {
    std::vector<std::string> key;
    key.reserve(cols.size());
    for (auto const* c : cols) {
        int code = c->GetValue(row);
        key.push_back(c->DecodeValue(code));
    }
    return key;
}

template <class EncTable, class IndicesVec>
std::vector<model::EncodedColumnData const*> BuildOrderedColumns(EncTable const& enc,
                                                                 IndicesVec const& indices) {
    using IndexT = std::decay_t<decltype(indices.front())>;
    std::unordered_map<IndexT, model::EncodedColumnData const*> by_index;
    by_index.reserve(enc.GetColumnData().size());

    for (auto const& col : enc.GetColumnData()) {
        by_index.emplace(static_cast<IndexT>(col.GetColumn()->GetIndex()), &col);
    }

    std::vector<model::EncodedColumnData const*> out;
    out.reserve(indices.size());
    for (auto idx : indices) {
        auto it = by_index.find(static_cast<IndexT>(idx));
        if (it == by_index.end()) {
            throw std::runtime_error("Internal error: inclusion index not found in encoded table");
        }
        out.push_back(it->second);
    }
    return out;
}

}  // namespace

CINDVerifier::CINDVerifier() : Algorithm({}), condition_type_(CondType::group) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTablesOpt.GetName()});
}

void CINDVerifier::RegisterOptions() {
    model::TableIndex const table_limit = 2;
    RegisterOption(config::kTablesOpt(&input_tables_, table_limit));

    auto const check_uniqueness = [](config::IndicesType const& indices) {
        std::set<config::IndexType> unique_ids{indices.begin(), indices.end()};
        if (unique_ids.size() != indices.size()) {
            throw config::ConfigurationError{"Invalid input: all indices should be unique"};
        }
    };

    RegisterOption(config::kLhsRawIndicesOpt(
            &ind_.lhs, [this]() { return input_tables_.front()->GetNumberOfColumns(); },
            [&rhs = ind_.rhs, &check_uniqueness](config::IndicesType const& indices) {
                check_uniqueness(indices);
                if (!rhs.empty() && rhs.size() != indices.size()) {
                    throw config::ConfigurationError{
                            "Invalid input: LHS and RHS indices must have the same size"};
                }
            }));

    RegisterOption(config::kRhsRawIndicesOpt(
            &ind_.rhs, [this]() { return input_tables_.back()->GetNumberOfColumns(); },
            [&lhs = ind_.lhs, &check_uniqueness](config::IndicesType const& indices) {
                check_uniqueness(indices);
                if (!lhs.empty() && lhs.size() != indices.size()) {
                    throw config::ConfigurationError{
                            "Invalid input: LHS and RHS indices must have the same size"};
                }
            }));

    RegisterOption(config::Option{&condition_values_, kCindCondValues, kDCindCondValues,
                                  std::vector<std::string>{}});

    RegisterOption(config::kValidityOpt(&min_validity_));
    RegisterOption(config::kCompletenessOpt(&min_completeness_));
    RegisterOption(config::kConditionTypeOpt(&condition_type_));
}

void CINDVerifier::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({
            config::kLhsIndicesOpt.GetName(),
            config::kRhsIndicesOpt.GetName(),
            kCindCondValues,
            config::kValidityOpt.GetName(),
            config::kCompletenessOpt.GetName(),
            config::kConditionTypeOpt.GetName(),
    });
}

void CINDVerifier::ResetState() {
    for (auto const& table : input_tables_) {
        table->Reset();
    }

    violating_clusters_.clear();
    violating_rows_ = 0;

    supporting_baskets_ = 0;
    included_support_ = 0;
    included_baskets_total_ = 0;

    real_validity_ = -1.0;
    real_completeness_ = 0.0;
}

void CINDVerifier::LoadDataInternal() {}

unsigned long long CINDVerifier::ExecuteInternal() {
    return util::TimedInvoke(&CINDVerifier::VerifyCIND, this);
}

void CINDVerifier::VerifyCIND() {
    if (input_tables_.empty()) {
        throw std::runtime_error("No input tables configured");
    }
    if (ind_.lhs.empty() || ind_.rhs.empty()) {
        throw std::runtime_error("CIND verification requires non-empty LHS and RHS indices");
    }

    config::InputTable const& lhs_table = input_tables_.front();
    config::InputTable const& rhs_table = input_tables_.back();
    bool const same_table = (lhs_table == rhs_table);

    std::unordered_set<config::IndexType> lhs_set(ind_.lhs.begin(), ind_.lhs.end());
    std::unordered_set<config::IndexType> rhs_set(ind_.rhs.begin(), ind_.rhs.end());

    std::vector<config::IndexType> conditional_indices;
    conditional_indices.reserve(lhs_table->GetNumberOfColumns());

    for (config::IndexType ci = 0; ci < lhs_table->GetNumberOfColumns(); ++ci) {
        if (lhs_set.contains(ci)) continue;
        if (same_table && rhs_set.contains(ci)) continue;
        conditional_indices.push_back(ci);
    }

    std::vector<std::string> cond_vals = condition_values_;
    if (cond_vals.empty()) {
        cond_vals.assign(conditional_indices.size(), kAnyValue);
    } else {
        if (cond_vals.size() != conditional_indices.size()) {
            throw std::runtime_error(
                    "cind_condition_values size must equal number of conditional attributes");
        }
    }
    for (auto& s : cond_vals) s = StripQuotes(s);

    bool const all_wildcards = std::all_of(cond_vals.begin(), cond_vals.end(),
                                           [](std::string const& s) { return IsWildcard(s); });

    if (all_wildcards) {
        using FixedStream = model::DatasetStreamFixed<model::IDatasetStream*>;
        using StreamProjection = model::DatasetStreamProjection<FixedStream>;
        using Row = StreamProjection::Row;

        auto const create_stream = [](config::InputTable const& table,
                                      config::IndicesType const& indices) {
            StreamProjection stream{table.get(), indices};
            if (!stream.HasNextRow()) {
                std::stringstream ss;
                ss << "Got an empty file \"" << stream.GetRelationName()
                   << "\": AIND verification is meaningless.";
                throw std::runtime_error(ss.str());
            }
            return stream;
        };

        std::unordered_set<Row, boost::hash<Row>> rhs_rows;
        {
            StreamProjection rhs_stream = create_stream(rhs_table, ind_.rhs);
            while (rhs_stream.HasNextRow()) {
                rhs_rows.insert(rhs_stream.GetNextRow());
            }
        }

        if (same_table) {
            lhs_table->Reset();
        }

        std::unordered_map<Row, Cluster, boost::hash<Row>> violating_map;
        std::unordered_set<Row, boost::hash<Row>> lhs_rows;

        model::TupleIndex current_row_id = 0;
        {
            StreamProjection lhs_stream = create_stream(lhs_table, ind_.lhs);
            while (lhs_stream.HasNextRow()) {
                Row row = lhs_stream.GetNextRow();
                lhs_rows.insert(row);

                if (!rhs_rows.contains(row)) {
                    violating_map[row].push_back(current_row_id);
                    ++violating_rows_;
                }

                ++current_row_id;
            }
        }

        for (auto& [row, cluster] : violating_map) {
            ViolatingCluster vc;
            vc.violating_rows = std::move(cluster);
            vc.basket_rows = vc.violating_rows;
            violating_clusters_.push_back(std::move(vc));
        }

        std::size_t lhs_cardinality = lhs_rows.size();
        std::size_t violating_clusters = violating_clusters_.size();

        supporting_baskets_ = lhs_cardinality;
        included_support_ = lhs_cardinality - violating_clusters;
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

    model::EncodedTables tables{input_tables_};

    model::TableIndex const lhs_table_idx = 0;
    model::TableIndex const rhs_table_idx =
            (input_tables_.size() == 1) ? 0
                                        : static_cast<model::TableIndex>(input_tables_.size() - 1);

    auto const& lhs_enc = tables.GetTable(lhs_table_idx);
    auto const& rhs_enc = tables.GetTable(rhs_table_idx);

    if (lhs_enc.GetColumnData().empty() || lhs_enc.GetColumnData().front().GetNumRows() == 0) {
        throw std::runtime_error("Got an empty LHS table: CIND verification is meaningless.");
    }

    bool const same_table_enc = (lhs_table_idx == rhs_table_idx);

    std::vector<model::EncodedColumnData const*> lhs_inclusion =
            BuildOrderedColumns(lhs_enc, ind_.lhs);
    std::vector<model::EncodedColumnData const*> rhs_inclusion =
            BuildOrderedColumns(rhs_enc, ind_.rhs);

    if (lhs_inclusion.empty() || rhs_inclusion.empty()) {
        throw std::runtime_error("CIND verification requires non-empty inclusion attributes");
    }

    std::vector<model::EncodedColumnData const*> conditional;
    for (auto const& column : lhs_enc.GetColumnData()) {
        auto const col_index = static_cast<config::IndexType>(column.GetColumn()->GetIndex());
        if (lhs_set.contains(col_index)) continue;

        if (!same_table_enc) {
            conditional.push_back(&column);
            continue;
        }

        if (!rhs_set.contains(col_index)) {
            conditional.push_back(&column);
        }
    }

    std::vector<std::string> cond_vals2 = condition_values_;
    if (cond_vals2.empty()) {
        cond_vals2.assign(conditional.size(), kAnyValue);
    }
    if (cond_vals2.size() != conditional.size()) {
        throw std::runtime_error(
                "cind_condition_values size must equal number of conditional attributes");
    }
    for (auto& s : cond_vals2) s = StripQuotes(s);

    std::vector<std::optional<int>> expected_codes;
    expected_codes.reserve(conditional.size());

    for (std::size_t i = 0; i < conditional.size(); ++i) {
        auto const& want = cond_vals2[i];
        if (IsWildcard(want)) {
            expected_codes.push_back(std::nullopt);
            continue;
        }

        auto const* col = conditional[i];
        std::unordered_map<std::string, int> decode_to_code;
        decode_to_code.reserve(col->GetNumRows());

        for (std::size_t r = 0; r < col->GetNumRows(); ++r) {
            int code = col->GetValue(r);
            decode_to_code.try_emplace(col->DecodeValue(code), code);
        }

        auto it = decode_to_code.find(want);
        if (it == decode_to_code.end()) {
            real_validity_ = -1.0;
            real_completeness_ = 0.0;
            return;
        }

        expected_codes.push_back(it->second);
    }

    auto row_matches = [&](std::size_t row) -> bool {
        for (std::size_t i = 0; i < conditional.size(); ++i) {
            if (!expected_codes[i].has_value()) continue;
            if (conditional[i]->GetValue(row) != *expected_codes[i]) return false;
        }
        return true;
    };

    using Key = std::vector<std::string>;

    std::unordered_set<Key, VecStrHash> rhs_keys;
    {
        std::size_t rhs_rows = rhs_inclusion.front()->GetNumRows();
        rhs_keys.reserve(rhs_rows);
        for (std::size_t r = 0; r < rhs_rows; ++r) {
            rhs_keys.insert(MakeKeyDecoded(r, rhs_inclusion));
        }
    }

    struct Acc {
        bool included{false};
        Cluster basket_rows;
        Cluster matching_rows;
    };

    std::unordered_map<Key, Acc, VecStrHash> acc_by_key;
    {
        std::size_t lhs_rows = lhs_inclusion.front()->GetNumRows();
        acc_by_key.reserve(lhs_rows);

        for (std::size_t l = 0; l < lhs_rows; ++l) {
            auto key = MakeKeyDecoded(l, lhs_inclusion);

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

    included_baskets_total_ = 0;
    if (condition_type_._value == CondType::group) {
        for (auto const& [_, acc] : acc_by_key) {
            if (acc.included) ++included_baskets_total_;
        }
    } else {
        for (auto const& [_, acc] : acc_by_key) {
            if (acc.included) included_baskets_total_ += acc.basket_rows.size();
        }
    }

    for (auto& [_, acc] : acc_by_key) {
        if (acc.matching_rows.empty()) continue;

        if (condition_type_._value == CondType::group) {
            ++supporting_baskets_;
            if (acc.included) {
                ++included_support_;
            } else {
                violating_rows_ += acc.matching_rows.size();
                violating_clusters_.push_back(ViolatingCluster{
                        .basket_rows = std::move(acc.basket_rows),
                        .violating_rows = std::move(acc.matching_rows),
                });
            }
        } else {
            supporting_baskets_ += acc.matching_rows.size();
            if (acc.included) {
                included_support_ += acc.matching_rows.size();
            } else {
                violating_rows_ += acc.matching_rows.size();
                violating_clusters_.push_back(ViolatingCluster{
                        .basket_rows = std::move(acc.basket_rows),
                        .violating_rows = std::move(acc.matching_rows),
                });
            }
        }
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
