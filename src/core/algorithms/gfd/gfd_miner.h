#pragma once

#include <cstdlib>
#include <filesystem>
#include <vector>

#include "algorithms/algorithm.h"
#include "algorithms/gfd/gfd.h"
#include "config/names_and_descriptions.h"
#include "parser/graph_parser/graph_parser.h"

namespace algos {

using namespace gfd;

// An algorithm for mining GFD dependencies
class GfdMiner : public Algorithm {
protected:
    std::filesystem::path graph_path_;

    // Graph in which dependencies are mined
    graph_t graph_;
    // Maximal number of vertices in the pattern of the mined dependency
    std::size_t k_;
    // Minimal frequency of the mined dependency
    std::size_t sigma_;

    std::vector<Gfd> gfds_;

    unsigned long long ExecuteInternal();

    void ResetState() final;
    void LoadDataInternal() final;

    void RegisterOptions();

    /* Produces literal rules for each pattern, generates graph dependencies
     * and then checks their satisfiability.
     */
    void HorizontalSpawn(
            std::vector<graph_t> const& patterns,
            std::vector<std::vector<std::map<vertex_t, vertex_t>>> const& embeddings_set,
            std::vector<std::vector<std::pair<std::vector<Literal>, std::vector<Literal>>>>&
                    forbidden_rules_set,
            std::map<std::string, std::unordered_map<std::string, std::set<std::string>>> const&
                    attrs_info);

    /* Filters those patterns whose frequency is less than
     * the input parameter sigma.
     */
    void FilterSupp(std::vector<graph_t>& patterns,
                    std::vector<std::vector<std::map<vertex_t, vertex_t>>>& embeddings_set);

    /* Traverses the initial graph, remembers all the vertex and edge labels
     * present in it, all the embeddings of single-vertex patterns,
     * and also generates a structure that simplifies
     * the generation of literals for patterns.
     */
    void Initialize(std::set<std::string>& vertex_labels, std::set<std::string>& edge_labels,
                    std::vector<graph_t>& patterns,
                    std::vector<std::vector<std::map<vertex_t, vertex_t>>>& embeddings_set,
                    std::vector<std::vector<std::pair<std::vector<Literal>, std::vector<Literal>>>>&
                            forbidden_rules_set,
                    std::map<std::string, std::unordered_map<std::string, std::set<std::string>>>&
                            attrs_info);

public:
    void MineGfds();

    GfdMiner();

    GfdMiner(graph_t graph_) : Algorithm({}), graph_(graph_) {
        ExecutePrepare();
    }

    std::vector<Gfd> const& GfdList() const {
        return gfds_;
    }
};

}  // namespace algos
