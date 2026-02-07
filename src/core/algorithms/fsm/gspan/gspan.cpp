
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

// Get all vertex descriptors having a given label
std::vector<vertex_t> FindAllWithLabel(int target_label, graph_t const& graph) {
    return graph[boost::graph_bundle].label_to_vertices.at(target_label);
}

// Find all isomorphisms between graph described by DFSCode and boost::graph, each
// isomorphism is represented by a map
std::vector<std::unordered_map<int, vertex_t>> SubgraphIsomorphisms(DFSCode const& code,
                                                                    graph_t const& graph) {
    LOG_TRACE("Finding subgraph isomorphisms: pattern size={}, target vertices={}, edges={}",
              code.Size(), boost::num_vertices(graph), boost::num_edges(graph));
    std::vector<std::unordered_map<int, vertex_t>> isoms;

    // Initial isomorphisms by finding all vertices with same label as vertex 0 in code
    int start_label = code.GetExtendedEdges()[0].vertex1.label;
    for (vertex_t vertex : FindAllWithLabel(start_label, graph)) {
        std::unordered_map<int, vertex_t> map;
        map[0] = vertex;
        isoms.push_back(std::move(map));
    }
    LOG_TRACE("Initial candidate mappings: {}", isoms.size());

    // Each extended edge will update partial isomorphisms.
    // For forward edge, each isomorphism will be either extended or discarded.
    // For backward edge, each isomorphism will be either unchanged or discarded.
    for (ExtendedEdge const& ee : code.GetExtendedEdges()) {
        int v1 = ee.vertex1.id;
        int v2 = ee.vertex2.id;
        int v2_label = ee.vertex2.label;
        int edge_label = ee.label;

        std::vector<std::unordered_map<int, vertex_t>> update_isoms;
        for (auto& iso : isoms) {
            auto mapped_v1 = iso[v1];

            // If it is a forward edge extension
            if (v1 < v2) {
                auto n = boost::num_vertices(graph);
                std::vector<int> inv(n, -1);
                for (auto& [dfs_id, graph_vertex] : iso) {
                    inv[graph_vertex] = dfs_id;
                }

                // For each neighbor of the vertex corresponding to v1
                for (auto edge : boost::make_iterator_range(boost::out_edges(mapped_v1, graph))) {
                    vertex_t mapped_v2 = (boost::source(edge, graph) == mapped_v1)
                                                 ? boost::target(edge, graph)
                                                 : boost::source(edge, graph);
                    auto& mapped_edge = graph[edge];

                    if (v2_label == graph[mapped_v2].label && (inv[mapped_v2] == -1) &&
                        (edge_label == mapped_edge.label)) {
                        std::unordered_map<int, vertex_t> temp_map(iso);
                        temp_map[v2] = mapped_v2;

                        update_isoms.push_back(std::move(temp_map));
                    }
                }
            } else {
                // If it is a backward edge extension.
                // V2 has been visited, only require mappedV1 and mappedV2 are connected in graph.
                auto mapped_v2 = iso[v2];
                auto [mapped_edge, is_neighbors] = boost::edge(mapped_v1, mapped_v2, graph);
                if (is_neighbors && edge_label == graph[mapped_edge].label) {
                    update_isoms.push_back(iso);
                }
            }
        }
        isoms = update_isoms;
    }

    LOG_TRACE("Found {} valid isomorphisms", isoms.size());
    return isoms;
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
    std::unordered_map<int, vertex_t> id_to_desc;
    for (auto& ee : code.GetExtendedEdges()) {
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

    result[boost::graph_bundle].id = -1;
    PrecalculateLabelsToVertices(result);
    return result;
}

}  // namespace

GSpan::GSpan() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::names::kGraphDatabase, config::names::kGSpanMinimumSupport,
                          config::names::kOutputSingleVertices, config::names::kMaxNumberOfEdges,
                          config::names::kGSpanOutputPath});
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
    graph_database_ = gspan::parser::ReadGraphs(f);
}

void GSpan::ResetState() {
    frequent_subgraphs_.clear();
    frequent_vertex_labels_.clear();
    empty_graphs_removed_ = 0;
}

unsigned long long GSpan::ExecuteInternal() {
    min_sup_ = static_cast<int>(std::ceil(min_frequency_ * graph_database_.size()));

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
    LOG_INFO("Starting GSpan algorithm: {} graphs, min_sup_={}", graph_database_.size(), min_sup_);

    LOG_DEBUG("Searching for frequent vertex labels");
    FindAllOnlyOneVertex();
    LOG_DEBUG("Found {} frequent vertex labels", frequent_vertex_labels_.size());

    LOG_DEBUG("Pruning infrequent vertex pairs and edge labels");
    RemoveInfrequentVertexPairs();
    LOG_DEBUG("Pruning complete");

    // Set with all the graph ids
    LOG_DEBUG("Building active graph set");
    std::unordered_set<int> graph_ids;
    for (size_t i = 0; i < graph_database_.size(); i++) {
        graph_t& graph = graph_database_[i];
        if (boost::num_vertices(graph) != 0) {
            graph_ids.insert(i);
            PrecalculateLabelsToVertices(graph);
        } else {
            empty_graphs_removed_++;
        }
    }
    LOG_DEBUG("Active graphs: {}, empty graphs removed: {}", graph_ids.size(),
              empty_graphs_removed_);

    if (frequent_vertex_labels_.size() != 0) {
        LOG_DEBUG("Starting DFS search with {} graphs", graph_ids.size());
        GSpanDFS(DFSCode(), graph_ids);
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
    std::unordered_set<std::pair<int, int>, boost::hash<std::pair<int, int>>> arleady_seen_pair;
    SparseTriangularMatrix matrix;
    std::unordered_set<int> arleady_seen_edge_label;
    std::unordered_map<int, int> edge_label_to_support;

    // Calculate the support of each entry
    for (graph_t& graph : graph_database_) {
        for (auto v1 : boost::make_iterator_range(boost::vertices(graph))) {
            int v1_label = graph[v1].label;
            for (auto edge : boost::make_iterator_range(boost::out_edges(v1, graph))) {
                vertex_t v2 =
                        (v1 == source(edge, graph)) ? target(edge, graph) : source(edge, graph);
                int v2_label = graph[v2].label;

                // Update vertex pair count
                auto pair = std::minmax(v1_label, v2_label);
                bool seen = arleady_seen_pair.contains(pair);
                if (!seen) {
                    matrix.IncrementCount(v1_label, v2_label);
                    arleady_seen_pair.insert(pair);
                }

                // Update edge label count
                int edge_label = graph[edge].label;
                if (!arleady_seen_edge_label.contains(edge_label)) {
                    arleady_seen_edge_label.insert(edge_label);
                    edge_label_to_support[edge_label]++;
                }
            }
        }
        arleady_seen_pair.clear();

        arleady_seen_edge_label.clear();
    }

    LOG_TRACE("Removing infrequent vertex pairs from support matrix");
    matrix.RemoveInfrequent(min_sup_);

    // Remove infrequent edges
    for (gspan::graph_t& graph : graph_database_) {
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
    }
}

void GSpan::GSpanDFS(gspan::DFSCode const& code, std::unordered_set<int> graph_ids) {
    LOG_TRACE("DFS step: pattern size={}, candidate graphs={}, patterns found={}", code.Size(),
              graph_ids.size(), frequent_subgraphs_.size());

    // If we have reached the maximum size, we do not need to extend this graph
    if (code.Size() == static_cast<size_t>(max_number_of_edges_)) {
        LOG_TRACE("Maximum pattern size reached, backtracking");
        return;
    }

    // Find all the extensions of this graph, with their support values.
    // They are stored in a map where the key is an extended edge, and the value
    // is the list of graph ids where this edge extends the current subgraph.
    std::unordered_map<ExtendedEdge, std::unordered_set<int>, ExtendedEdge::Hash> extensions =
            RightMostPathExtensions(code, graph_ids);
    LOG_TRACE("Found {} candidate extensions", extensions.size());

    for (auto& entry : extensions) {
        auto const& new_graph_ids = entry.second;
        int sup = new_graph_ids.size();

        // If the support is enough
        if (sup >= min_sup_) {
            // Create the new DFS code of this graph
            DFSCode new_code = code;
            ExtendedEdge const& extension = entry.first;
            new_code.Add(extension);

            // If the resulting graph is canonical (it means that the graph is non redundant)
            if (IsCanonical(new_code)) {
                LOG_TRACE("New frequent subgraph: size={}, support={}", new_code.Size(), sup);
                frequent_subgraphs_.emplace_back(frequent_subgraphs_.size(), new_code,
                                                 new_graph_ids, sup);
                GSpanDFS(new_code, new_graph_ids);
            }
        }
    }
}

template <class Emit>
void EnumerateRightMostExtensions(DFSCode const& code, graph_t const& graph, Emit&& emit) {
    LOG_TRACE("Computing sgraph extensions: pattern size={}, vertices={}", code.Size(),
              boost::num_vertices(graph));
    if (code.Empty()) {
        // If we have an empty subgraph that we want to extend,
        // find all distinct label tuples
        LOG_TRACE("Empty pattern, collecting initial edges");
        for (auto vertex : boost::make_iterator_range(boost::vertices(graph))) {
            for (auto edge : boost::make_iterator_range(boost::out_edges(vertex, graph))) {
                int vertex_label = graph[vertex].label;
                auto neighbor = boost::target(edge, graph) == vertex ? boost::source(edge, graph)
                                                                     : boost::target(edge, graph);
                int neighbor_label = graph[neighbor].label;
                ExtendedEdge ee =
                        (vertex_label < neighbor_label)
                                ? ExtendedEdge(Vertex(0, vertex_label), Vertex(1, neighbor_label),
                                               graph[edge].label)
                                : ExtendedEdge(Vertex(0, neighbor_label), Vertex(1, vertex_label),
                                               graph[edge].label);
                emit(ee);
            }
        }
    } else {
        // If we want to extend a subgraph
        auto rightmost = code.GetRightMost();
        std::vector<std::unordered_map<int, vertex_t>> isoms = SubgraphIsomorphisms(code, graph);
        LOG_TRACE("Found {} embeddings for extension", isoms.size());
        for (auto& isom : isoms) {
            // Backward extensions from rightmost child
            auto n = boost::num_vertices(graph);
            std::vector<int> inverted_isom(n, -1);
            for (auto& [dfs_id, graph_vertex] : isom) {
                inverted_isom[graph_vertex] = dfs_id;
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
                    // Rightmost and inverted both have correspondings in graph, so label of
                    // vertices and edge all can be found by correspondings
                    int edge_label = graph[edge].label;
                    ExtendedEdge ee(Vertex(rightmost, mapped_rightmost_label),
                                    Vertex(inverted, graph[neighbor].label), edge_label);
                    emit(ee);
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
                        emit(ee);
                    }
                }
            }
        }
    }
}

std::unordered_map<ExtendedEdge, std::unordered_set<int>, ExtendedEdge::Hash>
GSpan::RightMostPathExtensions(DFSCode const& code, std::unordered_set<int> graph_ids) {
    std::unordered_map<ExtendedEdge, std::unordered_set<int>, ExtendedEdge::Hash> out;

    for (int graph_id : graph_ids) {
        auto const& graph = graph_database_[graph_id];

        EnumerateRightMostExtensions(code, graph,
                                     [&](ExtendedEdge const& ee) { out[ee].insert(graph_id); });
    }

    return out;
}

std::unordered_set<ExtendedEdge, ExtendedEdge::Hash> GSpan::RightMostPathExtensionsFromSingle(
        DFSCode const& code, graph_t const& graph) {
    std::unordered_set<ExtendedEdge, ExtendedEdge::Hash> out;

    EnumerateRightMostExtensions(code, graph, [&](ExtendedEdge const& ee) { out.insert(ee); });

    return out;
}

bool GSpan::IsCanonical(DFSCode const& code) {
    LOG_TRACE("Checking canonicity: pattern size={}", code.Size());
    DFSCode canon;
    graph_t canon_graph = CreateGraphFromDFSCode(code);
    for (size_t i = 0; i < code.Size(); i++) {
        std::unordered_set<ExtendedEdge, ExtendedEdge::Hash> extensions =
                RightMostPathExtensionsFromSingle(canon, canon_graph);

        ExtendedEdge min_ee = *extensions.begin();
        for (auto& ee : extensions) {
            if (ee.SmallerThan(min_ee)) min_ee = ee;
        }

        if (min_ee.SmallerThan(code[i])) {
            LOG_TRACE("Non-canonical at edge {}", i);
            return false;
        }

        canon.Add(min_ee);
    }

    LOG_TRACE("Pattern is canonical");
    return true;
}

// This method finds all frequent vertex labels from a graph database.
void GSpan::FindAllOnlyOneVertex() {
    LOG_DEBUG("Collecting vertex label statistics");

    // Create a map (key = vertex label, value = graph ids)
    // to count the support of each vertex
    std::unordered_map<int, std::unordered_set<int>> label_map;

    for (auto& graph : graph_database_) {
        for (auto vertex : boost::make_iterator_range(boost::vertices(graph))) {
            if (boost::degree(vertex, graph) != 0) {
                int label = graph[vertex].label;
                label_map[label].insert(graph[boost::graph_bundle].id);
            }
        }
    }
    LOG_DEBUG("Found {} distinct vertex labels", label_map.size());

    for (auto entry : label_map) {
        int label = entry.first;

        std::unordered_set<int> temp_sup_g = entry.second;
        int sup = temp_sup_g.size();
        if (sup >= min_sup_) {
            frequent_vertex_labels_.push_back(label);
            LOG_TRACE("Vertex label {} is frequent (support={})", label, sup);

            if (output_single_vertices_) {
                DFSCode temp;
                temp.Add(ExtendedEdge(Vertex(0, label), Vertex(0, label), -1));
                frequent_subgraphs_.emplace_back(frequent_subgraphs_.size(), temp, temp_sup_g, sup);
            }
        } else {
            LOG_TRACE("Removing infrequent vertex label {} (support={})", label, sup);
            for (int graph_id : temp_sup_g) {
                gspan::graph_t& graph = graph_database_[graph_id];
                RemoveInfrequentLabel(graph, label);
            }
        }
    }

    LOG_DEBUG("Vertex label analysis complete: {} frequent labels", frequent_vertex_labels_.size());
}

}  // namespace algos