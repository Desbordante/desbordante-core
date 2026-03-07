#include "core/algorithms/cfd/cfun/cfun.h"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <ranges>

#include "core/algorithms/cfd/cfun/attribute_utils.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"

namespace algos::cfd::cfun {

CFUN::CFUN() : Algorithm({}) {
    using namespace config::names;
    RegisterOptions();
    MakeOptionsAvailable({kTable});
}

void CFUN::RegisterOptions() {
    DESBORDANTE_OPTION_USING;
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option{&min_support_, kCfdMinimumSupport, kDCfdMinimumSupport, 0u});
}

void CFUN::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kCfdMinimumSupport});
}

void CFUN::LoadDataInternal() {
    cfd_relation_ = CFDRelationData::CreateFrom(*input_table_);
    if (cfd_relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: CFD mining is meaningless.");
    }
}

unsigned long long CFUN::ExecuteInternal() {
    auto start_time = std::chrono::system_clock::now();
    double progress_step = kTotalProgressPercent / (cfd_relation_->GetNumColumns() + 1);

    auto last = BuildZeroLevel();
    auto current = BuildFirstLevel();
    auto mono_attr_ec = GetMonoProbingTables(current);
    AddProgress(progress_step);

    while (!current.empty()) {
        ComputeClosures(current, last, mono_attr_ec);
        ComputeQuasiClosures(current, last);
        GenerateNextLevel(last, current);
        AddProgress(progress_step);
    }
    ComputeClosures(current, last, mono_attr_ec);
    SetProgress(kTotalProgressPercent);

    auto cfun_millis = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);

    return cfun_millis.count();
}

CFUN::Level CFUN::BuildZeroLevel() const {
    Level null_level;
    null_level.reserve(1);

    Quadruple::Partition null_cluster(1, std::vector<unsigned int>(cfd_relation_->GetNumRows()));
    std::iota(null_cluster[0].begin(), null_cluster[0].end(), 0);

    null_level.emplace_back(std::move(null_cluster), AttributeSet(cfd_relation_->GetNumColumns()));
    return null_level;
}

CFUN::Level CFUN::BuildFirstLevel() const {
    Level first_level;
    first_level.reserve(cfd_relation_->GetNumColumns());
    auto partitions = CalculateMonoPartitions();

    for (size_t col = 0; col < cfd_relation_->GetNumColumns(); ++col) {
        auto mono_attribute = AttributeSet(cfd_relation_->GetNumColumns());
        mono_attribute.set(col);

        first_level.emplace_back(std::move(partitions[col]), std::move(mono_attribute));
    }
    return first_level;
}

std::vector<Quadruple::Partition> CFUN::CalculateMonoPartitions() const {
    std::vector<Quadruple::Partition> partitions(cfd_relation_->GetNumColumns());
    std::unordered_map<int, std::pair<int, int>> attr_indices;

    for (size_t a = 0; a < cfd_relation_->GetNumColumns(); ++a) {
        auto const& dom = cfd_relation_->GetDomain(a);
        partitions[a] = std::vector<std::vector<unsigned int>>(dom.size());
        for (unsigned int i = 0; i < dom.size(); i++) {
            partitions[a][i].reserve(cfd_relation_->Frequency(dom[i]));
            attr_indices[dom[i]] = std::make_pair(a, i);
        }
    }

    for (size_t row = 0; row < cfd_relation_->Size(); ++row) {
        auto const& tuple = cfd_relation_->GetRow(row);
        for (int item : tuple) {
            auto const& attr_node_ix = attr_indices.at(item);
            partitions[attr_node_ix.first][attr_node_ix.second].push_back(row);
        }
    }

    for (auto& partition : partitions) {
        std::erase_if(partition,
                      [&](auto const& cluster) { return cluster.size() < min_support_; });
    }

    return partitions;
}

std::vector<std::vector<unsigned int>> CFUN::GetMonoProbingTables(Level const& first_level) const {
    std::vector<std::vector<unsigned int>> mono_probing_tables;
    mono_probing_tables.reserve(cfd_relation_->GetNumColumns());

    auto build_mono_probing_table =
            [num_rows = cfd_relation_->GetNumRows()](Quadruple const& candidate) {
                return candidate.CalculateProbingTable(num_rows);
            };

    std::ranges::transform(first_level, std::back_inserter(mono_probing_tables),
                           build_mono_probing_table);

    return mono_probing_tables;
}

void CFUN::ComputeClosures(Level& current, Level& last,
                           std::vector<std::vector<unsigned int>> const& mono_probing_tables) {
    for (auto& candidate : last) {
        auto unset_attributes = ~candidate.GetAttributes();

        auto compute_cfds_and_update_closure = [&](auto num_col) {
            auto valid_cluster_ids = candidate.CheckCFD(num_col, mono_probing_tables[num_col]);
            if (valid_cluster_ids.empty()) {
                return;
            }

            candidate.UpdateClosure(valid_cluster_ids, num_col);

            auto parent_attribute = candidate.GetAttributes();
            parent_attribute.set(num_col);

            if (auto parent_it = std::ranges::find_if(
                        current,
                        [&parent_attribute](Quadruple const& candidate) {
                            return parent_attribute == candidate.GetAttributes();
                        });
                parent_it != current.end()) {
                parent_it->PruneRedundantSets(candidate, valid_cluster_ids,
                                              cfd_relation_->GetNumRows());
            }

            cfd_list_.push_back(DisplayCFD(candidate, num_col, valid_cluster_ids));
        };

        util::ForEachIndex(unset_attributes, compute_cfds_and_update_closure);
    }

    std::erase_if(current, [](Quadruple const& candidate) { return candidate.IsEmptySets(); });
}

CCFD CFUN::DisplayCFD(Quadruple const& X, unsigned int num_col,
                      std::vector<size_t> const& valid_cluster_id) {
    auto lhs = X.GetAttributes();
    auto const& clusters = X.GetClusters();

    CCFD::Tableau tableau;
    tableau.reserve(valid_cluster_id.size());

    for (auto clusted_id : valid_cluster_id) {
        CCFD::Condition condition;
        condition.reserve(lhs.size() + 1);

        unsigned int tuple_id = clusters[clusted_id].indices[0];
        auto const& row = cfd_relation_->GetRow(tuple_id);

        util::ForEachIndex(
                lhs, [&](auto attr) { condition.push_back(cfd_relation_->GetValue(row[attr])); });
        condition.push_back(cfd_relation_->GetValue(row[num_col]));

        tableau.push_back(std::move(condition));
    }

    auto schema = cfd_relation_->GetSharedPtrSchema();
    Vertical lhs_v(schema.get(), std::move(lhs));
    Column rhs_c(schema.get(), schema->GetColumn(num_col)->GetName(), num_col);

    return CCFD(std::move(lhs_v), std::move(rhs_c), std::move(tableau), std::move(schema));
}

void CFUN::ComputeQuasiClosures(Level& current, Level const& last) const {
    auto attr_to_candidate = AttributeIndex(last);

    for (auto& candidate : current) {
        auto subsets = attr_to_candidate.GetAllSubsets(candidate);
        candidate.UpdateQuasiClosure(subsets, cfd_relation_->GetNumRows());
    }
}

void CFUN::GenerateNextLevel(Level& last, Level& current) const {
    std::swap(last, current);
    current.clear();
    if (last.empty()) {
        return;
    }
    auto attr_to_candidate = AttributeIndex(last);

    for (size_t i = 0; i < last.size() - 1; i++) {
        for (size_t j = i + 1; j < last.size(); j++) {
            auto const& candidate_i = last[i];
            auto const& candidate_j = last[j];

            if (!candidate_i.HasSameBegin(candidate_j)) {
                break;
            }

            if (!attr_to_candidate.AllSubsetsContains(candidate_i.GetAttributes() |
                                                      candidate_j.GetAttributes())) {
                continue;
            }

            auto candidate_ij =
                    candidate_i.Intersect(candidate_j, cfd_relation_->GetNumRows(), min_support_);

            if (candidate_ij.IsEmptySets()) {
                continue;
            }

            current.push_back(std::move(candidate_ij));
        }
    }
}

}  // namespace algos::cfd::cfun
