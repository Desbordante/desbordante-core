#include "core/algorithms/cfd/ctane/c_lattice_level.h"

#include <algorithm>
#include <utility>

#include "core/algorithms/cfd/ctane/tuple_pattern.h"

using boost::dynamic_bitset;

CLatticeLevel::CLatticeLevel(std::size_t arity) : arity_(arity) {}

std::size_t CLatticeLevel::GetArity() const {
    return arity_;
}

std::vector<std::unique_ptr<CLatticeVertex>>& CLatticeLevel::GetVertices() {
    return vertices_;
}

std::vector<std::unique_ptr<CLatticeVertex>> const& CLatticeLevel::GetVertices() const {
    return vertices_;
}

CLatticeVertex* CLatticeLevel::GetLatticeVertex(
        algos::cfd::TuplePattern const& tuple_pattern) const {
    auto const vertex_it = std::ranges::find_if(vertices_, [&](auto const& vertex) {
        return vertex->GetTuplePattern() == tuple_pattern;
    });
    return vertex_it == vertices_.end() ? nullptr : vertex_it->get();
}

void CLatticeLevel::Add(std::unique_ptr<CLatticeVertex> vertex) {
    vertices_.emplace_back(std::move(vertex));
}

CLatticeLevel::ColumnPartitions CLatticeLevel::BuildColumnPartitions(
        algos::cfd::CFDRelationData const& relation, std::size_t column_index,
        unsigned min_support) {
    std::map<algos::cfd::Item, algos::cfd::SimpleTIdList> tids_by_item;
    for (unsigned row_index = 0; row_index < relation.GetNumRows(); ++row_index) {
        auto const item = relation.GetRow(row_index).at(column_index);
        tids_by_item[item].push_back(static_cast<algos::cfd::Item>(row_index));
    }

    ColumnPartitions partitions;
    if (relation.GetNumRows() < min_support) return partitions;

    algos::cfd::PartitionTIdList variable_partition;
    for (auto const& [item, tids] : tids_by_item) {
        static_cast<void>(item);
        if (!variable_partition.tids.empty()) {
            variable_partition.tids.push_back(algos::cfd::PartitionTIdList::kSep);
        }
        variable_partition.tids.insert(variable_partition.tids.end(), tids.begin(), tids.end());
        ++variable_partition.sets_number;
    }

    auto const variable_item = -1 - static_cast<algos::cfd::Item>(column_index);
    partitions.emplace(variable_item, std::move(variable_partition));

    for (auto& [item, tids] : tids_by_item) {
        if (tids.size() < min_support) continue;
        partitions.emplace(item, algos::cfd::PartitionTIdList{std::move(tids), 1});
    }

    return partitions;
}

std::unique_ptr<CLatticeVertex> CLatticeLevel::GenerateFirstLevel(
        std::vector<std::unique_ptr<CLatticeLevel>>& levels,
        algos::cfd::CFDRelationData const& relation, unsigned min_support) {
    auto const cols_number = relation.GetNumColumns();
    auto level = std::make_unique<CLatticeLevel>(1);
    auto empty_vertex =
            std::make_unique<CLatticeVertex>(&relation, algos::cfd::TuplePattern{cols_number});
    empty_vertex->SetPositionListIndex(algos::cfd::PartitionTIdList(relation.GetNumRows()));
    auto const* empty_vertex_ptr = empty_vertex.get();

    std::vector<ColumnPartitions> partitions_by_column;
    partitions_by_column.reserve(cols_number);
    algos::cfd::Itemset all_rhs_candidates;

    for (std::size_t col_index = 0; col_index < cols_number; ++col_index) {
        auto partitions = BuildColumnPartitions(relation, col_index, min_support);
        for (auto const& partition : partitions) {
            all_rhs_candidates.push_back(partition.first);
        }
        partitions_by_column.push_back(std::move(partitions));
    }
    std::sort(all_rhs_candidates.begin(), all_rhs_candidates.end());

    for (std::size_t col_index = 0; col_index < cols_number; ++col_index) {
        auto& column_partitions = partitions_by_column[col_index];
        for (auto& [item, pli] : column_partitions) {
            algos::cfd::TuplePattern::PatternValues pattern_values;
            pattern_values.emplace(static_cast<algos::cfd::AttributeIndex>(col_index), item);
            auto vertex = std::make_unique<CLatticeVertex>(
                    &relation, algos::cfd::TuplePattern{cols_number, std::move(pattern_values)});
            vertex->SetPositionListIndex(std::move(pli));
            vertex->GetParents().push_back(empty_vertex_ptr);

            auto column_rhs_candidates = all_rhs_candidates;
            std::erase_if(column_rhs_candidates, [&](algos::cfd::Item candidate) {
                auto const candidate_column =
                        candidate < 0 ? -1 - candidate : relation.GetAttrIndex(candidate);
                return candidate_column == static_cast<algos::cfd::AttributeIndex>(col_index) &&
                       candidate != item;
            });
            vertex->SetRhsCandidates(std::move(column_rhs_candidates));
            level->Add(std::move(vertex));
        }
    }

    levels.push_back(std::move(level));
    return empty_vertex;
}

void CLatticeLevel::GenerateNextLevel(std::vector<std::unique_ptr<CLatticeLevel>>& levels) {
    unsigned int current_level_index = levels.size() - 1;

    auto* current_level = levels[current_level_index].get();

    std::vector<CLatticeVertex*> current_level_vertices;
    current_level_vertices.reserve(current_level->GetVertices().size());
    for (auto const& vertex : current_level->GetVertices()) {
        current_level_vertices.push_back(vertex.get());
    }

    std::sort(current_level_vertices.begin(), current_level_vertices.end(),
              CLatticeVertex::Comparator);
    auto next_level = std::make_unique<CLatticeLevel>(current_level_index + 1);

    for (unsigned int vertex_index_1 = 0; vertex_index_1 < current_level_vertices.size();
         vertex_index_1++) {
        auto* vertex_1 = current_level_vertices[vertex_index_1];
        if (vertex_1->GetRhsCandidates().empty()) {
            continue;
        }

        for (unsigned int vertex_index_2 = vertex_index_1 + 1;
             vertex_index_2 < current_level_vertices.size(); vertex_index_2++) {
            auto* vertex_2 = current_level_vertices[vertex_index_2];
            if (!vertex_1->ComesBeforeAndSharePrefixWith(*vertex_2)) {
                continue;
            }
            auto intersection = CLatticeVertex::IntersectRhsCandidates(
                    vertex_1->GetRhsCandidates(), vertex_2->GetRhsCandidates());
            if (intersection.empty()) {
                continue;
            }
            std::unique_ptr<CLatticeVertex> child_vertex = std::make_unique<CLatticeVertex>(
                    vertex_1->GetRelation(),
                    algos::cfd::TuplePattern::UnionTuplePatterns(vertex_1->GetTuplePattern(),
                                                                 vertex_2->GetTuplePattern()));
            auto& child_rhs_candidates = child_vertex->GetRhsCandidates();

            dynamic_bitset<> parent_indices = vertex_1->GetTuplePattern().GetColumnIndices() |
                                              vertex_2->GetTuplePattern().GetColumnIndices();
            child_rhs_candidates = std::move(intersection);
            for (unsigned int i = 0, skip_index = parent_indices.find_first();
                 i < current_level_index; i++, skip_index = parent_indices.find_next(skip_index)) {
                auto* parent_vertex = current_level->GetLatticeVertex(
                        child_vertex->GetTuplePattern().GetWithoutColumn(skip_index));

                if (parent_vertex == nullptr) {
                    goto continueMidOuter;
                }
                child_rhs_candidates = CLatticeVertex::IntersectRhsCandidates(
                        child_rhs_candidates, parent_vertex->GetRhsCandidates());
                if (child_rhs_candidates.empty()) {
                    goto continueMidOuter;
                }
                child_vertex->GetParents().push_back(parent_vertex);
            }

            child_vertex->GetParents().push_back(vertex_1);
            child_vertex->GetParents().push_back(vertex_2);

            if (!child_rhs_candidates.empty()) {
                next_level->Add(std::move(child_vertex));
            }

        continueMidOuter:
            continue;
        }
    }

    levels.push_back(std::move(next_level));
}
