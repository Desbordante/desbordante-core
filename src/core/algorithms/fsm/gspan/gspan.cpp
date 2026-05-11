
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
    boost::unordered_flat_map<int, vertex_t> id_to_desc;
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
        auto edge = boost::add_edge(vertex1, vertex2, result);
        result[edge.first].label = ee.label;
    }

    result[boost::graph_bundle].original_id = -1;
    PrecalculateLabelsToVertices(result);
    return result;
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

    std::size_t elapsed_time = util::TimedInvoke(&GSpan::MineSubgraphs, this);
    LOG_DEBUG("Mining complete: {} frequent subgraphs found", frequent_subgraphs_.size());

    if (!output_path_.empty()) {
        gspan::parser::WriteGraphs(output_path_, frequent_subgraphs_);
        LOG_INFO("Wrote {} frequent subgraphs to {}", frequent_subgraphs_.size(),
                 output_path_.string());
    }

    return elapsed_time;
}

void GSpan::MineSubgraphs() {
    LOG_INFO("Starting GSpan algorithm: {} graphs, min_sup_={}", raw_dataset_.size(), min_sup_);

    LOG_DEBUG("Searching for frequent vertex labels");
    FindAllOnlyOneVertex();
    LOG_DEBUG("Found {} frequent vertex labels", frequent_vertex_labels_.size());

    LOG_DEBUG("Pruning infrequent vertex pairs and edge labels");
    RemoveInfrequentVertexPairs();
    LOG_DEBUG("Pruning complete");

    // Set with all the graph ids
    LOG_DEBUG("Building active graph set");
    Projection embeddings;
    for (size_t i = 0; i < pruned_graphs_.size(); i++) {
        graph_t& graph = pruned_graphs_[i];
        if (boost::num_vertices(graph) != 0) {
            embeddings.emplace_back(i, FlatIsoms{});
            PrecalculateLabelsToVertices(graph);
        } else {
            empty_graphs_removed_++;
        }
    }
    LOG_DEBUG("Active graphs: {}, empty graphs removed: {}", embeddings.size(),
              empty_graphs_removed_);

    if (frequent_vertex_labels_.size() != 0) {
        LOG_DEBUG("Starting DFS search with {} graphs", embeddings.size());
        GSpanDFS(DFSCode(), std::move(embeddings));
    }

    LOG_INFO("GSpan complete: {} frequent subgraphs found", frequent_subgraphs_.size());
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
                vertex_t v2 = (v1 == boost::source(edge, graph)) ? boost::target(edge, graph)
                                                                 : boost::source(edge, graph);
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
            if (boost::degree(vertex, graph) == 0) {
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

void GSpan::GSpanDFS(gspan::DFSCode const& code, Projection embeddings) {
    LOG_TRACE("DFS step: pattern size={}, candidate graphs={}, patterns found={}", code.Size(),
              embeddings.size(), frequent_subgraphs_.size());

    // If we have reached the maximum size, we do not need to extend this graph
    if (code.Size() == static_cast<size_t>(max_number_of_edges_)) {
        LOG_TRACE("Maximum pattern size reached, backtracking");
        return;
    }

    // Find all the extensions of this graph, with their support values.
    // They are stored in a map where the key is an extended edge, and the value
    // is the list of graph ids where this edge extends the current subgraph.
    auto extensions = RightMostPathExtensions(code, std::move(embeddings));
    LOG_TRACE("Found {} candidate extensions", extensions.size());

    for (auto& [extension, projection] : extensions) {
        int sup = projection.size();

        // If the support is enough
        if (sup >= min_sup_) {
            // Create the new DFS code of this graph
            DFSCode new_code = code;
            new_code.Add(extension);

            // If the resulting graph is canonical (it means that the graph is non redundant)
            if (IsCanonical(new_code)) {
                LOG_TRACE("New frequent subgraph: size={}, support={}", new_code.Size(), sup);
                boost::unordered::unordered_flat_set<int> new_graph_ids;
                for (auto const& proj_entry : projection) new_graph_ids.insert(proj_entry.graph_id);

                frequent_subgraphs_.emplace_back(
                        frequent_subgraphs_.size(), new_code,
                        TranslateToOriginalIds(new_graph_ids, pruned_graphs_), sup);
                GSpanDFS(new_code, std::move(projection));
            }
        }
    }
}

template <class Emit>
void EnumerateRightMostExtensions(DFSCode const& code, graph_t const& graph,
                                  FlatIsoms const& current_isoms, Emit&& emit) {
    LOG_TRACE("Computing sgraph extensions: pattern size={}, vertices={}", code.Size(),
              boost::num_vertices(graph));
    if (code.Empty()) {
        LOG_TRACE("Empty pattern, collecting initial edges");
        for (auto edge : boost::make_iterator_range(boost::edges(graph))) {
            auto v1 = boost::source(edge, graph);
            auto v2 = boost::target(edge, graph);
            int l1 = graph[v1].label;
            int l2 = graph[v2].label;

            if (l1 <= l2) {
                ExtendedEdge ee(Vertex(0, l1), Vertex(1, l2), graph[edge].label);
                vertex_t arr[2] = {v1, v2};
                emit(ee, std::span<vertex_t const>{arr, 2}, nullptr);
            }
            if (l1 >= l2) {
                ExtendedEdge ee(Vertex(0, l2), Vertex(1, l1), graph[edge].label);
                vertex_t arr[2] = {v2, v1};
                emit(ee, std::span<vertex_t const>{arr, 2}, nullptr);
            }
        }
    } else {
        auto rightmost = code.GetRightMost();
        size_t n = boost::num_vertices(graph);
        thread_local std::vector<int> inverted_isom;

        if (inverted_isom.size() < n) {
            inverted_isom.resize(n, -1);
        }

        LOG_TRACE("Found {} embeddings for extension", current_isoms.size());
        for (auto isom : current_isoms) {
            // Backward extensions from rightmost child
            for (size_t dfs_id = 0; dfs_id < isom.size(); ++dfs_id) {
                inverted_isom[isom[dfs_id]] = dfs_id;
            }

            auto mapped_rightmost = isom[rightmost];
            int mapped_rightmost_label = graph[mapped_rightmost].label;
            for (auto edge :
                 boost::make_iterator_range(boost::out_edges(mapped_rightmost, graph))) {
                vertex_t neighbor = (boost::source(edge, graph) == mapped_rightmost)
                                            ? boost::target(edge, graph)
                                            : boost::source(edge, graph);
                if (inverted_isom[neighbor] == -1) {
                    continue;
                }

                auto inverted = inverted_isom[neighbor];
                if (code.OnRightMostPath(inverted) && code.NotPreOfRM(inverted) &&
                    !code.ContainEdge(rightmost, inverted)) {
                    int edge_label = graph[edge].label;
                    ExtendedEdge ee(Vertex(rightmost, mapped_rightmost_label),
                                    Vertex(inverted, graph[neighbor].label), edge_label);
                    emit(ee, isom, nullptr);
                }
            }

            // Forward extensions from nodes on rightmost path
            for (auto vertex : code.GetRightMostPath()) {
                auto mapped_vertex = isom[vertex];
                int mapped_vertex_label = graph[mapped_vertex].label;
                for (auto edge :
                     boost::make_iterator_range(boost::out_edges(mapped_vertex, graph))) {
                    vertex_t neighbor = (boost::source(edge, graph) == mapped_vertex)
                                                ? boost::target(edge, graph)
                                                : boost::source(edge, graph);
                    if (inverted_isom[neighbor] == -1) {
                        int edge_label = graph[edge].label;
                        ExtendedEdge ee(Vertex(vertex, mapped_vertex_label),
                                        Vertex(rightmost + 1, graph[neighbor].label), edge_label);
                        emit(ee, isom, &neighbor);
                    }
                }
            }
            for (size_t dfs_id = 0; dfs_id < isom.size(); ++dfs_id) {
                inverted_isom[isom[dfs_id]] = -1;
            }
        }
    }
}

projection_map_t GSpan::RightMostPathExtensions(DFSCode const& code, Projection const& projection) {
    projection_map_t out;

    for (auto const& proj : projection) {
        int graph_id = proj.graph_id;
        auto const& graph = pruned_graphs_[graph_id];

        EnumerateRightMostExtensions(
                code, graph, proj.isoms,
                [&](ExtendedEdge const& ee, std::span<vertex_t const> old_iso,
                    vertex_t const* extra) {
                    auto& entries = out[ee];
                    if (entries.empty() || entries.back().graph_id != graph_id) {
                        entries.push_back({graph_id, FlatIsoms{}});
                    }
                    if (extra) {
                        entries.back().isoms.push_with_extra(old_iso, *extra);
                    } else {
                        entries.back().isoms.push(old_iso);
                    }
                });
    }

    return out;
}

boost::unordered_flat_map<ExtendedEdge, FlatIsoms, ExtendedEdge::Hash>
GSpan::RightMostPathExtensionsFromSingle(DFSCode const& code, graph_t const& graph,
                                         FlatIsoms const& current_isoms) {
    boost::unordered_flat_map<ExtendedEdge, FlatIsoms, ExtendedEdge::Hash> out;

    EnumerateRightMostExtensions(
            code, graph, current_isoms,
            [&](ExtendedEdge const& ee, std::span<vertex_t const> old_iso, vertex_t const* extra) {
                if (extra) {
                    out[ee].push_with_extra(old_iso, *extra);
                } else {
                    out[ee].push(old_iso);
                }
            });

    return out;
}

bool GSpan::IsCanonical(DFSCode const& code) {
    LOG_TRACE("Checking canonicity: pattern size={}", code.Size());
    DFSCode canon;
    graph_t canon_graph = CreateGraphFromDFSCode(code);
    FlatIsoms isoms;

    for (size_t i = 0; i < code.Size(); i++) {
        auto extensions = RightMostPathExtensionsFromSingle(canon, canon_graph, isoms);

        if (extensions.empty()) {
            return false;
        }

        ExtendedEdge min_ee = extensions.begin()->first;
        for (auto const& [ee, new_isoms] : extensions) {
            if (ee.SmallerThan(min_ee)) min_ee = ee;
        }

        if (min_ee.SmallerThan(code[i])) {
            LOG_TRACE("Non-canonical at edge {}", i);
            return false;
        }

        canon.Add(min_ee);
        isoms = std::move(extensions[min_ee]);
    }

    LOG_TRACE("Pattern is canonical");
    return true;
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
            if (boost::degree(vertex, graph) != 0) {
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
