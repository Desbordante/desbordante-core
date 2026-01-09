#include "graph_parser.h"

#include <boost/algorithm/string.hpp>

namespace gspan::parser {

std::vector<gspan::graph_t> ReadGraphs(std::istream& stream) {
    std::vector<gspan::graph_t> result;
    std::string line;

    std::getline(stream, line);
    while (stream) {
        if (!line.starts_with("t")) {
            continue;
        }

        gspan::graph_t graph;
        std::vector<std::string> splitted;
        boost::split(splitted, line, boost::is_any_of(" "));
        int graph_id = std::stoi(splitted[2]);
        graph[boost::graph_bundle].id = graph_id;

        std::unordered_map<int, vertex_t> id_to_desc;
        while (std::getline(stream, line) && !line.starts_with("t")) {
            std::vector<std::string> items;
            boost::split(items, line, boost::is_any_of(" "));

            if (line.starts_with("v")) {
                int vertex_id = std::stoi(items[1]);
                auto vertex = (id_to_desc.contains(vertex_id)) ? id_to_desc[vertex_id]
                                                               : boost::add_vertex(graph);
                id_to_desc[vertex_id] = vertex;
                graph[vertex].id = vertex_id;
                graph[vertex].label = std::stoi(items[2]);
            } else if (line.starts_with("e")) {
                int vertex1_id = std::stoi(items[1]);
                int vertex2_id = std::stoi(items[2]);
                int edge_label = std::stoi(items[3]);

                if (!id_to_desc.contains(vertex1_id)) {
                    auto vertex = boost::add_vertex(graph);
                    id_to_desc[vertex1_id] = vertex;
                }

                if (!id_to_desc.contains(vertex2_id)) {
                    auto vertex = boost::add_vertex(graph);
                    id_to_desc[vertex2_id] = vertex;
                }

                auto vertex1 = id_to_desc[vertex1_id];
                auto vertex2 = id_to_desc[vertex2_id];
                auto edge = boost::add_edge(vertex1, vertex2, graph);
                graph[edge.first].label = edge_label;
            }
        }
        result.push_back(std::move(graph));
    }

    return result;
}

std::vector<gspan::graph_t> ReadGraphs(std::filesystem::path const& path) {
    std::ifstream file(path);
    return ReadGraphs(file);
}

void WriteGraphs(std::ostream& stream, std::vector<gspan::FrequentSubgraph> const& result) {
    for (auto& subgraph : result) {
        stream << subgraph.ToString();
        stream << "\n";
    }

    stream << std::endl;
}

void WriteGraphs(std::filesystem::path const& path,
                 std::vector<gspan::FrequentSubgraph> const& result) {
    std::ofstream file(path);
    WriteGraphs(file, result);
}

}  // namespace gspan::parser