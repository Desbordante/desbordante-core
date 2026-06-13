#include "subgraph_miner.h"

#include <boost/unordered/unordered_flat_set.hpp>

#include "core/util/logger.h"

namespace gspan {

namespace {

int CountSupport(Projection const& projection) {
    int prev_id = -1;
    int support = 0;

    for (auto const& entry : projection) {
        if (prev_id != entry.graph_id) {
            prev_id = entry.graph_id;
            support++;
        }
    }

    return support;
}

}  // namespace

void SubgraphMiner::MineChild(Projection const& projection, ExtendedEdge const& new_edge,
                              DFSCode code) {
    int support = CountSupport(projection);
    if (static_cast<size_t>(support) < min_sup_) {
        return;
    }

    code.Add(new_edge);

    // If the resulting graph is canonical (it means that the graph is non redundant)
    if (IsCanonical(code)) {
        LOG_TRACE("New frequent subgraph: size={}, support={}", code.Size(), support);
        boost::unordered_flat_set<int> original_graph_ids;
        original_graph_ids.reserve(support);
        for (auto const& proj_entry : projection) {
            original_graph_ids.insert(
                    graph_database_[proj_entry.graph_id][boost::graph_bundle].original_id);
        }

        frequent_subgraphs_.emplace_back(frequent_subgraphs_.size(), code,
                                         std::move(original_graph_ids), support);
        MineSubgraph(projection, code);
    }

    code.Pop();
}

void SubgraphMiner::MineSubgraph(Projection const& projection, DFSCode const& code) {
    if (code.Size() == static_cast<size_t>(max_number_of_edges_)) {
        LOG_TRACE("Maximum pattern size reached, backtracking");
        return;
    }

    ProjectionMapBackward backward_pmap;
    ProjectionMapForward forward_pmap;

    Enumerate(code, projection, backward_pmap, forward_pmap);

    std::atomic<int> pending{0};

    for (auto& [ee, proj] : backward_pmap) {
        if (thread_pool_) {
            auto p_ptr = std::make_shared<Projection>(std::move(proj));
            thread_pool_->Spawn(
                    [this, p_ptr = std::move(p_ptr), ee, code](int t_id) {
                        (*miners_)[t_id]->MineChild(std::move(*p_ptr), ee, code);
                    },
                    pending);
        } else {
            MineChild(std::move(proj), ee, code);
        }
    }
    for (auto it = forward_pmap.rbegin(); it != forward_pmap.rend(); it++) {
        auto& [ee, proj] = *it;
        if (thread_pool_) {
            auto p_ptr = std::make_shared<Projection>(std::move(proj));
            thread_pool_->Spawn(
                    [this, p_ptr = std::move(p_ptr), ee, code](int t_id) {
                        (*miners_)[t_id]->MineChild(std::move(*p_ptr), ee, code);
                    },
                    pending);
        } else {
            MineChild(std::move(proj), ee, code);
        }
    }

    if (thread_pool_) {
        thread_pool_->Wait(pending, thread_id_);
    }
}

void SubgraphMiner::Enumerate(DFSCode const& code, Projection const& projection,
                              ProjectionMapBackward& backward_pmap,
                              ProjectionMapForward& forward_pmap) {
    for (auto const& entry : projection) {
        auto const& graph = graph_database_[entry.graph_id];
        history_.Reconstruct(entry, graph);

        GetBackward(entry, graph, code, backward_pmap);
        GetFirstForward(entry, graph, code, forward_pmap);
        GetOtherForward(entry, graph, code, forward_pmap);
    }
    history_.Clear();
}

void SubgraphMiner::GetBackward(ProjectionEntry const& entry, csr_graph_t const& graph,
                                DFSCode const& code, ProjectionMapBackward& backward_pmap) {
    auto rm_vertex_id = code[rightmost_path_[0]].vertex2.id;
    auto last_edge = history_.GetEdge(rightmost_path_[0]);
    auto last_node = boost::target(last_edge, graph);

    for (auto ln_edge : boost::make_iterator_range(boost::out_edges(last_node, graph))) {
        if (history_.HasEdge(graph[ln_edge].id)) {
            continue;
        }

        auto ln_edge_to = boost::target(ln_edge, graph);
        for (size_t i = rightmost_path_.size() - 1; i > 0; i--) {
            ExtendedEdge const& path_ee = code[rightmost_path_[i]];
            auto edge = history_.GetEdge(rightmost_path_[i]);
            auto edge_source = boost::source(edge, graph);
            auto edge_target = boost::target(edge, graph);

            if (graph[ln_edge_to].id != graph[edge_source].id) {
                continue;
            }

            if (std::tuple{graph[edge].label, graph[edge_target].label} <=
                std::tuple{graph[ln_edge].label, graph[last_node].label}) {
                ExtendedEdge ee(Vertex{rm_vertex_id, graph[last_node].label},
                                Vertex{path_ee.vertex1.id, graph[edge_source].label},
                                graph[ln_edge].label);
                backward_pmap[ee].emplace_back(entry.graph_id, ln_edge, &entry);
            }

            break;
        }
    }
}

void SubgraphMiner::GetFirstForward(ProjectionEntry const& entry, csr_graph_t const& graph,
                                    DFSCode const& code, ProjectionMapForward& forward_pmap) {
    int min_label = code[0].vertex1.label;
    auto rm_vertex_id = code[rightmost_path_[0]].vertex2.id;

    auto last_edge = history_.GetEdge(rightmost_path_[0]);
    auto last_node = boost::target(last_edge, graph);

    for (auto ln_edge : boost::make_iterator_range(boost::out_edges(last_node, graph))) {
        auto ln_edge_to = boost::target(ln_edge, graph);
        // Partial pruning: if this label is less than the minimum label, then there
        // should exist another lexicographical order which renders the same letters, but
        // in the asecending order.
        // Could we perform the same partial pruning as other extending methods?
        // No, we cannot, for this time, the extending id is greater the the last node
        if (history_.HasVertex(graph[ln_edge_to].id) || graph[ln_edge_to].label < min_label) {
            continue;
        }

        ExtendedEdge ee(Vertex{rm_vertex_id, graph[last_node].label},
                        Vertex{rm_vertex_id + 1, graph[ln_edge_to].label}, graph[ln_edge].label);
        forward_pmap[ee].emplace_back(entry.graph_id, ln_edge, &entry);
    }
}

void SubgraphMiner::GetOtherForward(ProjectionEntry const& entry, csr_graph_t const& graph,
                                    DFSCode const& code, ProjectionMapForward& forward_pmap) {
    int min_label = code[0].vertex1.label;
    auto to_id = code[rightmost_path_[0]].vertex2.id;
    for (auto i : rightmost_path_) {
        int from_id = code[i].vertex1.id;

        auto current_edge = history_.GetEdge(i);
        auto current_node = boost::source(current_edge, graph);
        auto node_neighbor = boost::target(current_edge, graph);

        for (auto cn_edge : boost::make_iterator_range(boost::out_edges(current_node, graph))) {
            vertex_t to_node = boost::target(cn_edge, graph);
            if (history_.HasVertex(graph[to_node].id) || graph[to_node].label < min_label) {
                continue;
            }

            if (std::tuple{graph[current_edge].label, graph[node_neighbor].label} <=
                std::tuple{graph[cn_edge].label, graph[to_node].label}) {
                ExtendedEdge ee(Vertex{from_id, graph[current_node].label},
                                Vertex{to_id + 1, graph[to_node].label}, graph[cn_edge].label);
                forward_pmap[ee].emplace_back(entry.graph_id, cn_edge, &entry);
            }
        }
    }
}

bool SubgraphMiner::IsCanonical(DFSCode const& code) {
    LOG_TRACE("Checking canonicity: pattern size={}", code.Size());
    min_graph_.BuildFromDFSCode(code);
    rightmost_path_.clear();
    rightmost_path_.push_back(0);

    if (code.Size() == 1) {
        return true;
    }

    min_projection_.clear();

    // The first edge in the sequence must be the
    // smallest if the sequence itself is minimal
    ExtendedEdge const& min_ee = code[0];

    for (auto const& vertex : min_graph_) {
        for (auto const& edge : vertex.edges) {
            int source_label = vertex.label;
            int target_label = min_graph_[edge.to].label;

            if (source_label <= target_label) {
                ExtendedEdge ee(Vertex(0, source_label), Vertex(1, target_label), edge.label);

                if (ExtendedEdgeProjectCompare{}(ee, min_ee)) {
                    return false;
                }

                if (ee == min_ee) {
                    min_projection_.push_back({&edge, -1});
                }
            }
        }
    }

    return IsProjectionMin(code);
}

bool SubgraphMiner::IsProjectionMin(DFSCode const& code) {
    size_t projection_start_index = 0;

    // index 0 was already validated on previous step
    for (size_t i = 1; i < code.Size(); i++) {
        ExtendedEdge const& ee = code[i];
        size_t projection_end_index = min_projection_.size();

        // If a forward and a backward edge can be both be used to extend
        // the code, then the backward edge must be smaller.
        if (ee.vertex1.id > ee.vertex2.id) {
            // edge is backward, ensure it is minimal.
            if (!IsBackwardMin(code, ee, projection_start_index)) {
                return false;
            }

        } else {
            // edge is forward, so ensure no backward edges exist,
            // then ensure the edge is minimal.
            if (ExistsBackwards(projection_start_index) ||
                !IsForwardMin(code, ee, projection_start_index)) {
                return false;
            }

            // Forward edge was validated, so update the rightmost path.
            UpdateRightmostPath(code, i + 1);
        }

        projection_start_index = projection_end_index;
    }

    LOG_TRACE("Pattern is canonical");
    return true;
}

bool SubgraphMiner::IsBackwardMin(gspan::DFSCode const& code, ExtendedEdge const& ee,
                                  size_t projection_start_index) {
    size_t projection_end_index = min_projection_.size();

    ExtendedEdge const& rightmost_edge = code[rightmost_path_[0]];

    int from_id = rightmost_edge.vertex2.id;
    for (size_t j = projection_start_index; j < projection_end_index; j++) {
        history_.ReconstructEdges(min_projection_, min_graph_, j);

        auto const& last_edge = history_.GetMinEdge(rightmost_path_[0]);
        auto const& last_node = min_graph_[last_edge.to];

        for (auto const& ln_edge : last_node.edges) {
            if (history_.HasEdge(ln_edge.id)) {
                continue;
            }

            for (size_t i = rightmost_path_.size() - 1; i > 0; i--) {
                ExtendedEdge const& path_ee = code[rightmost_path_[i]];
                int to_id = path_ee.vertex1.id;
                auto const& edge = history_.GetMinEdge(rightmost_path_[i]);
                auto const& edge_from = min_graph_[edge.from];
                auto const& edge_to = min_graph_[edge.to];

                if (ln_edge.to == edge.from && std::tuple{edge.label, edge_to.label} <=
                                                       std::tuple{ln_edge.label, last_node.label}) {
                    ExtendedEdge min_ee(Vertex{from_id, last_node.label},
                                        Vertex{to_id, edge_from.label}, ln_edge.label);
                    // A smaller edge was found, so the given edge is not minimal.
                    if (ExtendedEdgeBackwardCompare{}(min_ee, ee)) {
                        return false;
                    }
                    if (min_ee == ee) {
                        min_projection_.push_back({&ln_edge, static_cast<int>(j)});
                    }
                }

                if (to_id == ee.vertex2.id) break;
            }
        }
    }
    return true;
}

bool SubgraphMiner::IsForwardMin(gspan::DFSCode const& code, ExtendedEdge const& ee,
                                 size_t projection_start_index) {
    size_t projection_end_index = min_projection_.size();

    int min_label = code[0].vertex1.label;
    int max_id = code[rightmost_path_[0]].vertex2.id;

    for (size_t i = projection_start_index; i < projection_end_index; i++) {
        history_.ReconstructVertices(min_projection_, min_graph_, i);

        auto const& last_edge = history_.GetMinEdge(rightmost_path_[0]);
        auto const& last_node = min_graph_[last_edge.to];

        for (auto const& ln_edge : last_node.edges) {
            auto const& to_node = min_graph_[ln_edge.to];
            if (history_.HasVertex(to_node.id) || to_node.label < min_label) {
                continue;
            }

            ExtendedEdge min_ee(Vertex{max_id, last_node.label}, Vertex{max_id + 1, to_node.label},
                                ln_edge.label);
            // A smaller edge was found, so the given edge is not minimal
            if (ExtendedEdgeForwardCompare{}(min_ee, ee)) {
                return false;
            }

            if (min_ee == ee) {
                min_projection_.push_back({&ln_edge, static_cast<int>(i)});
            }
        }

        // If ee is an extension from the rightmost vertex, we only
        // need to continue looking at similar extensions (i.e. the block above)
        if (max_id == ee.vertex1.id) {
            continue;
        }

        for (auto j : rightmost_path_) {
            int from_id = code[j].vertex1.id;

            auto const& current_edge = history_.GetMinEdge(j);
            auto const& current_node = min_graph_[current_edge.from];
            auto const& node_neighbor = min_graph_[current_edge.to];

            for (auto const& cn_edge : current_node.edges) {
                auto const& to_node = min_graph_[cn_edge.to];
                if (history_.HasVertex(to_node.id) || to_node.label < min_label) {
                    continue;
                }
                if (std::tuple{current_edge.label, node_neighbor.label} <=
                    std::tuple{cn_edge.label, to_node.label}) {
                    ExtendedEdge min_ee(Vertex{from_id, current_node.label},
                                        Vertex{max_id + 1, to_node.label}, cn_edge.label);
                    // A smaller edge was found, so the given edge is not minimal
                    if (ExtendedEdgeForwardCompare{}(min_ee, ee)) {
                        return false;
                    }
                    if (min_ee == ee) {
                        min_projection_.push_back({&cn_edge, static_cast<int>(i)});
                    }
                }
            }
            // Every member of the RMP after this one will produce larger DFS codes,
            // so they don't need to be checked against the minimum.
            if (from_id == ee.vertex1.id) {
                break;
            }
        }
    }
    return true;
}

bool SubgraphMiner::ExistsBackwards(size_t projection_start_index) {
    size_t projection_end_index = min_projection_.size();

    for (auto j = projection_start_index; j < projection_end_index; j++) {
        history_.ReconstructEdges(min_projection_, min_graph_, j);
        auto const& last_edge = history_.GetMinEdge(rightmost_path_[0]);
        auto const& last_node = min_graph_[last_edge.to];
        for (auto const& ln_edge : last_node.edges) {
            if (history_.HasEdge(ln_edge.id)) {
                continue;
            }

            // i > 0 since a backward edge cannot go to the last vertex.
            for (size_t i = rightmost_path_.size() - 1; i > 0; i--) {
                auto const& edge = history_.GetMinEdge(rightmost_path_[i]);
                auto const& edge_to = min_graph_[edge.to];

                if (ln_edge.to == edge.from && std::tuple{edge.label, edge_to.label} <=
                                                       std::tuple{ln_edge.label, last_node.label}) {
                    return true;
                }
            }
        }
    }

    return false;
}

// Clears right_most_path, then stores into it the rightmost path of the dfs code
// list. The path is stored such that the first item in right_most_path is the
// index of the edge 'discovering' the rightmost vertex, the second is the index
// of the edge discovering the 'from' vertex of the first edge, and so on.
// DFSCode is treated as if it is truncated to the given size.
void SubgraphMiner::UpdateRightmostPath(gspan::DFSCode const& code, size_t size) {
    rightmost_path_.clear();
    int prev_id = -1;

    // Go in reverse, since we need to first look for the edge that discovered
    // the rightmost vertex
    for (auto i = size; i > 0; --i) {
        // Only consider forward edges (as by definition the rightmost path only
        // consists of edges 'discovering' new nodes). The first forward edge (or
        // equivalently, the last forward edge in DFSCode) is the edge discovering
        // the rightmost vertex. After that, each new edge is the edge discovering
        // the 'from' of the previous one.
        if (code[i - 1].vertex1.id < code[i - 1].vertex2.id &&
            (rightmost_path_.empty() || prev_id == code[i - 1].vertex2.id)) {
            prev_id = code[i - 1].vertex1.id;
            rightmost_path_.push_back(i - 1);
        }
    }
}

}  // namespace gspan