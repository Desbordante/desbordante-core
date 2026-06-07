
#include "gspan.h"

#include <algorithm>

#include <boost/functional/hash.hpp>
#include <boost/range/iterator_range.hpp>

#include "core/config/option_using.h"
#include "core/util/logger.h"
#include "core/util/timed_invoke.h"
#include "graph_parser.h"
#include "sparse_triangular_matrix.h"
using namespace gspan;

namespace algos {

namespace {

<<<<<<< HEAD
void BuildIsomFromHistory(HistoryNode const* leaf, size_t num_vertices,
                          std::vector<vertex_t>& out_isom) {
    out_isom.assign(num_vertices, (vertex_t)-1);

    HistoryNode const* curr = leaf;
    int current_dfs_id = num_vertices - 1;
    while (curr != nullptr) {
        if (curr->added_vertex != (vertex_t)-1) {
            out_isom[current_dfs_id] = curr->added_vertex;
            current_dfs_id--;
        }
        curr = curr->prev;
    }
}

// Get all vertex descriptors having a given label (with bounds checking)
std::vector<vertex_t> const& FindAllWithLabel(int target_label, graph_t const& graph) {
    return graph[boost::graph_bundle].label_to_vertices.at(target_label);
}

// Precalculate the list of vertices having each label
void PrecalculateLabelsToVertices(graph_t& graph) {
    auto& label_to_verts = graph[boost::graph_bundle].label_to_vertices;
    for (auto v : boost::make_iterator_range(boost::vertices(graph))) {
        label_to_verts[graph[v].label].push_back(v);
    }
}

graph_t CreateGraphFromDFSCode(DFSCode const& code) {
    graph_t result;
=======
void CreateGraphFromDFSCode(DFSCode const& code, graph_t& result) {
    result.clear();
>>>>>>> 281e3d80 (Implement features from gbolt)
    boost::unordered_flat_map<int, vertex_t> id_to_desc;
    int edge_id = 0;
    for (auto const& ee : code.GetExtendedEdges()) {
        vertex_t vertex1;
        if (id_to_desc.contains(ee.vertex1.id)) {
            vertex1 = id_to_desc[ee.vertex1.id];
        } else {
            vertex1 = boost::add_vertex(result);
            id_to_desc[ee.vertex1.id] = vertex1;
        }

        vertex_t vertex2;
        if (id_to_desc.contains(ee.vertex2.id)) {
            vertex2 = id_to_desc[ee.vertex2.id];
        } else {
            vertex2 = boost::add_vertex(result);
            id_to_desc[ee.vertex2.id] = vertex2;
        }

        result[vertex1].id = ee.vertex1.id;
        result[vertex1].label = ee.vertex1.label;

        result[vertex2].id = ee.vertex2.id;
        result[vertex2].label = ee.vertex2.label;

        auto edge1 = boost::add_edge(vertex1, vertex2, result);
        result[edge1.first].id = edge_id;
        result[edge1.first].label = ee.label;

        auto edge2 = boost::add_edge(vertex2, vertex1, result);
        result[edge2.first].id = edge_id;
        result[edge2.first].label = ee.label;

        edge_id++;
    }

    result[boost::graph_bundle].original_id = -1;
}

boost::unordered_flat_set<int> TranslateToOriginalIds(
        boost::unordered_flat_set<int> const& internal_ids, std::vector<graph_t> const& graph_db) {
    boost::unordered_flat_set<int> original_ids;
    original_ids.reserve(internal_ids.size());
    for (int id : internal_ids) {
        original_ids.insert(graph_db[id][boost::graph_bundle].original_id);
    }
    return original_ids;
}

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

void GetBackward(ProjectionEntry const& entry, History const& history, graph_t const& graph,
                 DFSCode const& code, ProjectionMapBackward& backward_pmap) {
    auto const& rightmost_path = code.GetRightMostPath();
    auto rm_vertex_id = code.GetRightMostEdge().vertex2.id;
    edge_t last_edge = history.GetEdge(rightmost_path[0]);
    vertex_t last_node = boost::target(last_edge, graph);

    for (auto ln_edge : boost::make_iterator_range(boost::out_edges(last_node, graph))) {
        if (history.HasEdge(graph[ln_edge].id)) {
            continue;
        }

        vertex_t ln_edge_to = boost::target(ln_edge, graph);
        for (size_t i = rightmost_path.size() - 1; i > 0; i--) {
            ExtendedEdge const& path_ee = code.GetEdgeFromRightMostPath(i);
            edge_t edge = history.GetEdge(rightmost_path[i]);
            vertex_t edge_source = boost::source(edge, graph);
            vertex_t edge_target = boost::target(edge, graph);

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

void GetFirstForward(ProjectionEntry const& entry, History const& history, graph_t const& graph,
                     DFSCode const& code, ProjectionMapForward& forward_pmap) {
    int min_label = code[0].vertex1.label;
    auto rm_vertex_id = code.GetRightMostEdge().vertex2.id;
    auto const& rightmost_path = code.GetRightMostPath();

    edge_t last_edge = history.GetEdge(rightmost_path[0]);
    vertex_t last_node = boost::target(last_edge, graph);

    for (auto ln_edge : boost::make_iterator_range(boost::out_edges(last_node, graph))) {
        vertex_t ln_edge_to = boost::target(ln_edge, graph);
        // Partial pruning: if this label is less than the minimum label, then there
        // should exist another lexicographical order which renders the same letters, but
        // in the asecending order.
        // Could we perform the same partial pruning as other extending methods?
        // No, we cannot, for this time, the extending id is greater the the last node
        if (history.HasVertex(graph[ln_edge_to].id) || graph[ln_edge_to].label < min_label) {
            continue;
        }

        ExtendedEdge ee(Vertex{rm_vertex_id, graph[last_node].label},
                        Vertex{rm_vertex_id + 1, graph[ln_edge_to].label}, graph[ln_edge].label);
        forward_pmap[ee].emplace_back(entry.graph_id, ln_edge, &entry);
    }
}

void GetOtherForward(ProjectionEntry const& entry, History const& history, graph_t const& graph,
                     DFSCode const& code, ProjectionMapForward& forward_pmap) {
    int min_label = code[0].vertex1.label;
    auto const& rightmost_path = code.GetRightMostPath();
    auto to_id = code.GetRightMostEdge().vertex2.id;
    for (auto i : rightmost_path) {
        int from_id = code[i].vertex1.id;

        edge_t current_edge = history.GetEdge(i);
        vertex_t current_node = boost::source(current_edge, graph);
        vertex_t node_neighbor = boost::target(current_edge, graph);

        for (auto cn_edge : boost::make_iterator_range(boost::out_edges(current_node, graph))) {
            vertex_t to_node = boost::target(cn_edge, graph);
            if (history.HasVertex(graph[to_node].id) || graph[to_node].label < min_label) {
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

}  // namespace

GSpan::GSpan() : Algorithm() {
    RegisterOptions();
    MakeOptionsAvailable({config::names::kGraphDatabase, config::names::kGSpanMinimumSupport,
                          config::names::kOutputSingleVertices, config::names::kMaxNumberOfEdges,
                          config::names::kGSpanOutputPath});
}

void GSpan::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable(
            {kGSpanMinimumSupport, kOutputSingleVertices, kMaxNumberOfEdges, kGSpanOutputPath});
}

void GSpan::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto check_minsup = [](double val) {
        if (val <= 0 || val > 1) {
            throw config::ConfigurationError(
                    "Minimum support must be a value between 0 (exclusive) and 1 (inclusive).");
        }
    };

    auto check_max_edges = [](int val) {
        if (val <= 0) {
            throw config::ConfigurationError("Maximum number of edges must be a positive integer.");
        }
    };

    RegisterOption(config::Option{&graph_database_path_, kGraphDatabase, kDGraphDatabase});
    RegisterOption(config::Option{&min_frequency_, kGSpanMinimumSupport, kDGSpanMinimumSupport}
                           .SetValueCheck(check_minsup));

    RegisterOption(config::Option{&output_single_vertices_, kOutputSingleVertices,
                                  kDOutputSingleVertices, true});
    RegisterOption(
            config::Option{&max_number_of_edges_, kMaxNumberOfEdges, kDMaxNumberOfEdges, INT_MAX}
                    .SetValueCheck(check_max_edges));

    RegisterOption(config::Option{&output_path_, kGSpanOutputPath, kDGSpanOutputPath,
                                  std::filesystem::path{}});
}

void GSpan::LoadDataInternal() {
    std::ifstream f(graph_database_path_);
    raw_dataset_ = gspan::parser::ReadGraphs(f);
    pruned_graphs_ = raw_dataset_;
}

void GSpan::ResetState() {
    pruned_graphs_ = raw_dataset_;
    frequent_subgraphs_.clear();
    frequent_vertex_labels_.clear();
    empty_graphs_removed_ = 0;
}

unsigned long long GSpan::ExecuteInternal() {
    min_sup_ = static_cast<int>(std::ceil(min_frequency_ * raw_dataset_.size()));

    std::size_t elapsed_time = util::TimedInvoke(&GSpan::Launch, this);
    LOG_DEBUG("Mining complete: {} frequent subgraphs found", frequent_subgraphs_.size());

    if (!output_path_.empty()) {
        gspan::parser::WriteGraphs(output_path_, frequent_subgraphs_);
        LOG_INFO("Wrote {} frequent subgraphs to {}", frequent_subgraphs_.size(),
                 output_path_.string());
    }

    return elapsed_time;
}

// TODO: rename to 'PrepareData'
void GSpan::Launch() {
    LOG_INFO("Starting GSpan algorithm: {} graphs, min_sup_={}", raw_dataset_.size(), min_sup_);

    LOG_DEBUG("Searching for frequent vertex labels");
    FindAllOnlyOneVertex();
    LOG_DEBUG("Found {} frequent vertex labels", frequent_vertex_labels_.size());

    LOG_DEBUG("Pruning infrequent vertex pairs and edge labels");
    RemoveInfrequentVertexPairs();
    LOG_DEBUG("Pruning complete");

    int max_edges = 0;
    int max_vertices = 0;
    for (size_t i = 0; i < pruned_graphs_.size(); i++) {
        graph_t& graph = pruned_graphs_[i];
        max_edges = std::max(max_edges, static_cast<int>(boost::num_edges(graph)));
        max_vertices = std::max(max_vertices, static_cast<int>(boost::num_vertices(graph)));
    }

    history_.Reset(max_edges, max_vertices);
    ProjectionMap embeddings = GetInitialEdges();
    for (auto const& [ee, proj] : embeddings) {
        MineChild(proj, ee, DFSCode());
    }

    LOG_INFO("GSpan complete: {} frequent subgraphs found", frequent_subgraphs_.size());
}

ProjectionMap GSpan::GetInitialEdges() {
    ProjectionMap result;
    for (size_t i = 0; i < pruned_graphs_.size(); i++) {
        graph_t& graph = pruned_graphs_[i];
        for (auto v1 : boost::make_iterator_range(boost::vertices(graph))) {
            for (auto edge : boost::make_iterator_range(boost::out_edges(v1, graph))) {
                vertex_t v2 = boost::target(edge, graph);
                int source_label = graph[v1].label;
                int target_label = graph[v2].label;
                // Partial pruning: if the first label is greater than the
                // second label, then there must be another graph whose second
                // label is greater than the first label.
                if (source_label <= target_label) {
                    ExtendedEdge ee = ExtendedEdge(Vertex(0, source_label), Vertex(1, target_label),
                                                   graph[edge].label);
                    result[ee].emplace_back(i, edge, nullptr);
                }
            }
        }
    }
    return result;
}

void GSpan::MineChild(Projection const& projection, ExtendedEdge const& new_edge,
                      DFSCode const& code) {
    int support = CountSupport(projection);
    if (support < min_sup_) {
        return;
    }

    // Create the new DFS code of this graph
    DFSCode new_code = code;
    new_code.Add(new_edge);

    // If the resulting graph is canonical (it means that the graph is non redundant)
    if (IsCanonical(new_code)) {
        LOG_TRACE("New frequent subgraph: size={}, support={}", new_code.Size(), support);
        boost::unordered::unordered_flat_set<int> new_graph_ids;
        for (auto const& proj_entry : projection) new_graph_ids.insert(proj_entry.graph_id);

        frequent_subgraphs_.emplace_back(frequent_subgraphs_.size(), new_code,
                                         TranslateToOriginalIds(new_graph_ids, pruned_graphs_),
                                         support);
        MineSubgraph(projection, new_code);
    }
}

void GSpan::MineSubgraph(Projection const& projection, DFSCode& code) {
    if (code.Size() == static_cast<size_t>(max_number_of_edges_)) {
        LOG_TRACE("Maximum pattern size reached, backtracking");
        return;
    }

    ProjectionMapBackward backward_pmap;
    ProjectionMapForward forward_pmap;

    Enumerate(code, projection, backward_pmap, forward_pmap);
    for (auto const& [ee, proj] : backward_pmap) {
        MineChild(proj, ee, code);
    }
    for (auto it = forward_pmap.rbegin(); it != forward_pmap.rend(); it++) {
        auto [ee, proj] = *it;
        MineChild(proj, ee, code);
    }
}

void GSpan::Enumerate(DFSCode const& code, Projection const& projection,
                      ProjectionMapBackward& backward_pmap, ProjectionMapForward& forward_pmap) {
    for (auto const& entry : projection) {
        graph_t const& graph = pruned_graphs_[entry.graph_id];
        history_.Reconstruct(entry, graph);

        GetBackward(entry, history_, graph, code, backward_pmap);
        GetFirstForward(entry, history_, graph, code, forward_pmap);
        GetOtherForward(entry, history_, graph, code, forward_pmap);
    }
    history_.Clear();
}

bool GSpan::IsCanonical(DFSCode const& code) {
    LOG_TRACE("Checking canonicity: pattern size={}", code.Size());
    CreateGraphFromDFSCode(code, min_graph_);
    code.ResetRightmostPath();

    if (code.Size() == 1) {
        return true;
    }

    min_projection_.clear();

    // The first edge in the sequence must be the
    // smallest if the sequence itself is minimal
    ExtendedEdge const& min_ee = code[0];

    for (auto v1 : boost::make_iterator_range(boost::vertices(min_graph_))) {
        for (auto edge : boost::make_iterator_range(boost::out_edges(v1, min_graph_))) {
            vertex_t v2 = boost::target(edge, min_graph_);
            int source_label = min_graph_[v1].label;
            int target_label = min_graph_[v2].label;

            if (source_label <= target_label) {
                ExtendedEdge ee(Vertex(0, source_label), Vertex(1, target_label),
                                min_graph_[edge].label);

                if (ExtendedEdgeProjectCompare{}(ee, min_ee)) {
                    return false;
                }

                if (ee == min_ee) {
                    min_projection_.emplace_back(edge, -1);
                }
            }
        }
    }

    return IsProjectionMin(code);
}

bool GSpan::IsProjectionMin(DFSCode const& code) {
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
            if (ExistsBackwards(code, projection_start_index) ||
                !IsForwardMin(code, ee, projection_start_index)) {
                return false;
            }

            // Forward edge was validated, so update the rightmost path.
            code.UpdateRightmostPath(i + 1);
        }

        projection_start_index = projection_end_index;
    }

    LOG_TRACE("Pattern is canonical");
    return true;
}

bool GSpan::IsBackwardMin(gspan::DFSCode const& code, ExtendedEdge const& ee,
                          size_t projection_start_index) {
    size_t projection_end_index = min_projection_.size();

    auto const& rightmost_path = code.GetRightMostPath();
    ExtendedEdge const& rightmost_edge = code.GetRightMostEdge();

    int from_id = rightmost_edge.vertex2.id;
    for (size_t j = projection_start_index; j < projection_end_index; j++) {
        history_.ReconstructEdges(min_projection_, min_graph_, j);

        edge_t last_edge = history_.GetEdge(rightmost_path[0]);
        vertex_t last_node = boost::target(last_edge, min_graph_);

        for (auto ln_edge : boost::make_iterator_range(boost::out_edges(last_node, min_graph_))) {
            if (history_.HasEdge(min_graph_[ln_edge].id)) {
                continue;
            }

            vertex_t ln_edge_to = boost::target(ln_edge, min_graph_);
            for (size_t i = rightmost_path.size() - 1; i > 0; i--) {
                ExtendedEdge const& path_ee = code.GetEdgeFromRightMostPath(i);
                int to_id = path_ee.vertex1.id;
                edge_t edge = history_.GetEdge(rightmost_path[i]);
                vertex_t edge_source = boost::source(edge, min_graph_);
                vertex_t edge_target = boost::target(edge, min_graph_);

                if (min_graph_[ln_edge_to].id == min_graph_[edge_source].id &&
                    std::tuple{min_graph_[edge].label, min_graph_[edge_target].label} <=
                            std::tuple{min_graph_[ln_edge].label, min_graph_[last_node].label}) {
                    ExtendedEdge min_ee(Vertex{from_id, min_graph_[last_node].label},
                                        Vertex{to_id, min_graph_[edge_source].label},
                                        min_graph_[ln_edge].label);
                    // A smaller edge was found, so the given edge is not minimal.
                    if (ExtendedEdgeBackwardCompare{}(min_ee, ee)) {
                        return false;
                    }
                    if (min_ee == ee) {
                        min_projection_.emplace_back(ln_edge, j);
                    }
                }

                if (to_id == ee.vertex2.id) break;
            }
        }
    }
    return true;
}

bool GSpan::IsForwardMin(gspan::DFSCode const& code, ExtendedEdge const& ee,
                         size_t projection_start_index) {
    size_t projection_end_index = min_projection_.size();
    auto const& rightmost_path = code.GetRightMostPath();

    int min_label = code[0].vertex1.label;
    int max_id = code.GetRightMostEdge().vertex2.id;

    for (size_t i = projection_start_index; i < projection_end_index; i++) {
        history_.ReconstructVertices(min_projection_, min_graph_, i);

        edge_t last_edge = history_.GetEdge(rightmost_path[0]);
        vertex_t last_node = boost::target(last_edge, min_graph_);

        for (auto ln_edge : boost::make_iterator_range(boost::out_edges(last_node, min_graph_))) {
            vertex_t ln_edge_to = boost::target(ln_edge, min_graph_);
            auto const& to_node = min_graph_[ln_edge_to];
            if (history_.HasVertex(to_node.id) || to_node.label < min_label) {
                continue;
            }

            ExtendedEdge min_ee(Vertex{max_id, min_graph_[last_node].label},
                                Vertex{max_id + 1, to_node.label}, min_graph_[ln_edge].label);
            // A smaller edge was found, so the given edge is not minimal
            if (ExtendedEdgeForwardCompare{}(min_ee, ee)) {
                return false;
            }

            if (min_ee == ee) {
                min_projection_.emplace_back(ln_edge, i);
            }
        }

        // If ee is an extension from the rightmost vertex, we only
        // need to continue looking at similar extensions (i.e. the block above)
        if (max_id == ee.vertex1.id) {
            continue;
        }

        for (auto j : rightmost_path) {
            int from_id = code[j].vertex1.id;

            edge_t current_edge = history_.GetEdge(j);
            vertex_t current_node = boost::source(current_edge, min_graph_);
            vertex_t node_neighbor = boost::target(current_edge, min_graph_);

            for (auto cn_edge :
                 boost::make_iterator_range(boost::out_edges(current_node, min_graph_))) {
                vertex_t to_node = boost::target(cn_edge, min_graph_);
                if (history_.HasVertex(min_graph_[to_node].id) ||
                    min_graph_[to_node].label < min_label) {
                    continue;
                }
                if (std::tuple{min_graph_[current_edge].label, min_graph_[node_neighbor].label} <=
                    std::tuple{min_graph_[cn_edge].label, min_graph_[to_node].label}) {
                    ExtendedEdge min_ee(Vertex{from_id, min_graph_[current_node].label},
                                        Vertex{max_id + 1, min_graph_[to_node].label},
                                        min_graph_[cn_edge].label);
                    // A smaller edge was found, so the given edge is not minimal
                    if (ExtendedEdgeForwardCompare{}(min_ee, ee)) {
                        return false;
                    }
                    if (min_ee == ee) {
                        min_projection_.emplace_back(cn_edge, i);
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

bool GSpan::ExistsBackwards(DFSCode const& code, size_t projection_start_index) {
    size_t projection_end_index = min_projection_.size();
    auto const& rightmost_path = code.GetRightMostPath();

    for (auto j = projection_start_index; j < projection_end_index; j++) {
        history_.ReconstructEdges(min_projection_, min_graph_, j);
        edge_t last_edge = history_.GetEdge(rightmost_path[0]);
        vertex_t last_node = boost::target(last_edge, min_graph_);
        for (auto ln_edge : boost::make_iterator_range(boost::out_edges(last_node, min_graph_))) {
            if (history_.HasEdge(min_graph_[ln_edge].id)) {
                continue;
            }

            vertex_t ln_edge_to = boost::target(ln_edge, min_graph_);
            // i > 0 since a backward edge cannot go to the last vertex.
            for (size_t i = rightmost_path.size() - 1; i > 0; i--) {
                edge_t edge = history_.GetEdge(rightmost_path[i]);
                vertex_t edge_source = boost::source(edge, min_graph_);
                vertex_t edge_target = boost::target(edge, min_graph_);

                if (min_graph_[ln_edge_to].id == min_graph_[edge_source].id &&
                    std::tuple{min_graph_[edge].label, min_graph_[edge_target].label} <=
                            std::tuple{min_graph_[ln_edge].label, min_graph_[last_node].label}) {
                    return true;
                }
            }
        }
    }

    return false;
}

void GSpan::RemoveInfrequentLabel(gspan::graph_t& graph, int label) {
    std::vector<vertex_t> vertices_to_remove;
    for (auto vertex : boost::make_iterator_range(boost::vertices(graph))) {
        if (graph[vertex].label == label) {
            vertices_to_remove.push_back(vertex);
        }
    }
    // Sort in descending order to prevent invalidating descriptors (indices) of subsequent vertices
    std::sort(vertices_to_remove.rbegin(), vertices_to_remove.rend());
    for (auto vertex : vertices_to_remove) {
        boost::clear_vertex(vertex, graph);
        boost::remove_vertex(vertex, graph);
    }
}

void GSpan::RemoveInfrequentVertexPairs() {
    boost::unordered_flat_set<std::pair<int, int>, boost::hash<std::pair<int, int>>>
            already_seen_pair;
    SparseTriangularMatrix matrix;
    boost::unordered_flat_set<int> already_seen_edge_label;
    boost::unordered_flat_map<int, int> edge_label_to_support;

    // Calculate the support of each entry
    for (graph_t& graph : pruned_graphs_) {
        for (auto v1 : boost::make_iterator_range(boost::vertices(graph))) {
            int v1_label = graph[v1].label;
            for (auto edge : boost::make_iterator_range(boost::out_edges(v1, graph))) {
                vertex_t v2 = boost::target(edge, graph);
                int v2_label = graph[v2].label;

                // Update vertex pair count
                auto pair = std::minmax(v1_label, v2_label);
                bool seen = already_seen_pair.contains(pair);
                if (!seen) {
                    matrix.IncrementCount(v1_label, v2_label);
                    already_seen_pair.insert(pair);
                }

                // Update edge label count
                int edge_label = graph[edge].label;
                if (!already_seen_edge_label.contains(edge_label)) {
                    already_seen_edge_label.insert(edge_label);
                    edge_label_to_support[edge_label]++;
                }
            }
        }
        already_seen_pair.clear();

        already_seen_edge_label.clear();
    }

    LOG_TRACE("Removing infrequent vertex pairs from support matrix");
    matrix.RemoveInfrequent(min_sup_);

    // Remove infrequent edges
    for (gspan::graph_t& graph : pruned_graphs_) {
        boost::remove_edge_if(
                [&](gspan::edge_t e) {
                    auto v1 = boost::source(e, graph);
                    auto v2 = boost::target(e, graph);
                    int label1 = graph[v1].label;
                    int label2 = graph[v2].label;
                    int count = matrix.GetSupport(label1, label2);

                    if (count < min_sup_) {
                        return true;
                    }
                    if (edge_label_to_support[graph[e].label] < min_sup_) {
                        return true;
                    }
                    return false;
                },
                graph);

        // Remove isolated vertices
        std::vector<vertex_t> isolated;
        for (auto vertex : boost::make_iterator_range(boost::vertices(graph))) {
            if (boost::out_degree(vertex, graph) == 0) {
                isolated.push_back(vertex);
            }
        }
        std::sort(isolated.rbegin(), isolated.rend());
        for (auto vertex : isolated) {
            boost::clear_vertex(vertex, graph);
            boost::remove_vertex(vertex, graph);
        }
    }
}

// This method finds all frequent vertex labels from a graph database.
void GSpan::FindAllOnlyOneVertex() {
    LOG_DEBUG("Collecting vertex label statistics");

    // Create a map (key = vertex label, value = graph ids)
    // to count the support of each vertex
    boost::unordered_flat_map<int, boost::unordered_flat_set<int>> label_map;

    for (size_t i = 0; i < pruned_graphs_.size(); i++) {
        auto const& graph = pruned_graphs_[i];
        for (auto vertex : boost::make_iterator_range(boost::vertices(graph))) {
            if (boost::out_degree(vertex, graph) != 0) {
                int label = graph[vertex].label;
                label_map[label].insert(i);
            }
        }
    }
    LOG_DEBUG("Found {} distinct vertex labels", label_map.size());

    for (auto [label, temp_sup_g] : label_map) {
        int sup = temp_sup_g.size();
        if (sup >= min_sup_) {
            frequent_vertex_labels_.push_back(label);
            LOG_TRACE("Vertex label {} is frequent (support={})", label, sup);

            if (output_single_vertices_) {
                DFSCode temp;
                temp.Add(ExtendedEdge(Vertex(0, label), Vertex(0, label), -1));
                frequent_subgraphs_.emplace_back(frequent_subgraphs_.size(), temp,
                                                 TranslateToOriginalIds(temp_sup_g, pruned_graphs_),
                                                 sup);
            }
        } else {
            LOG_TRACE("Removing infrequent vertex label {} (support={})", label, sup);
            for (int graph_id : temp_sup_g) {
                gspan::graph_t& graph = pruned_graphs_[graph_id];
                RemoveInfrequentLabel(graph, label);
            }
        }
    }

    LOG_DEBUG("Vertex label analysis complete: {} frequent labels", frequent_vertex_labels_.size());
}

}  // namespace algos
