#pragma once

#include <cstdlib>
#include <filesystem>
#include <vector>

#include "algorithms/algorithm.h"
#include "algorithms/gfd/gfd.h"
#include "config/names_and_descriptions.h"
#include "parser/graph_parser/graph_parser.h"

namespace algos {

// An algorithm for mining GFD dependencies
class GfdMiner : public Algorithm {
protected:
    std::filesystem::path graph_path_;

    // Graph in which dependencies are mined
    model::graph_t graph_;
    // Maximal number of vertices in the pattern of the mined dependency
    std::size_t k_;
    // Minimal frequency of the mined dependency
    std::size_t sigma_;

    std::vector<model::Gfd> gfds_;

    unsigned long long ExecuteInternal();

    void ResetState() final;
    void LoadDataInternal() final;

    void RegisterOptions();

    /* Unwraps the token and writes the resulting value to the variable tok.
     * Returns false if there is no value, true otherwise.
     */
    bool CheckToken(model::Gfd::Token const& token, std::string& tok, model::graph_t const& pattern,
                    std::unordered_map<model::vertex_t, model::vertex_t> const& embedding);

    /* Composes rules with the same left-hand side and adds the obtained
     * graph dependencies to the result.
     */
    void AddCompacted(
            std::vector<std::pair<std::vector<model::Gfd::Literal>, model::Gfd::Literal>>& rules,
            model::graph_t& pattern,
            std::vector<std::pair<std::vector<model::Gfd::Literal>,
                                  std::vector<model::Gfd::Literal>>>& forbidden_rules);

    /* Checks whether the given dependency is satisfied on the graph
     * and whether it is frequent.
     */
    bool Validate(
            std::vector<model::Gfd::Literal> const& lhs, model::Gfd::Literal const& rhs,
            model::graph_t& pattern,
            std::vector<std::unordered_map<model::vertex_t, model::vertex_t>> const& embeddings,
            std::map<model::Gfd::Literal, std::set<std::size_t>> const& satisfied_embeddings);

    /* Checks whether the number of embeddings represented by the set of literals
     * passed by indices is at least sigma.
     */
    bool CheckFrequency(
            std::set<std::size_t> const& indices,
            std::vector<std::unordered_map<model::vertex_t, model::vertex_t>> const& embeddings,
            model::graph_t const& pattern);

    /* Checks whether the number of embeddings represented by
     * the set of literals is at least sigma.
     */
    bool CheckFrequency(
            std::vector<model::Gfd::Literal> const& subset,
            std::map<model::Gfd::Literal, std::set<std::size_t>> const& satisfied_embeddings);

    /* Generates all the rules of the satisfied dependencies for a given pattern.
     */
    std::vector<std::pair<std::vector<model::Gfd::Literal>, model::Gfd::Literal>> GenerateRules(
            std::vector<model::Gfd::Literal> const& literals, model::graph_t& pattern,
            std::vector<std::unordered_map<model::vertex_t, model::vertex_t>> const& embeddings,
            std::vector<std::pair<std::vector<model::Gfd::Literal>,
                                  std::vector<model::Gfd::Literal>>> const& forbidden_rules,
            std::map<model::Gfd::Literal, std::set<std::size_t>> const& satisfied_embeddings);

    /* Checks if a literal is satisfied on the current embedding.
     */
    bool LiteralSatisfied(model::Gfd::Literal const& l,
                          std::unordered_map<model::vertex_t, model::vertex_t> const& embedding,
                          model::graph_t const& pattern);

    /* Generates all possible literal candidates that can be satisfied on the graph.
     */
    std::vector<model::Gfd::Literal> GenerateLiterals(
            model::graph_t const& pattern,
            std::unordered_map<std::string,
                               std::unordered_map<std::string, std::set<std::string>>> const&
                    attrs_info,
            std::vector<std::unordered_map<model::vertex_t, model::vertex_t>> const& embeddings,
            std::map<model::Gfd::Literal, std::set<std::size_t>>& satisfied_embeddings_set);

    /* Produces literal rules for each pattern, generates graph dependencies
     * and then checks their satisfiability.
     */
    void HorizontalSpawn(
            std::vector<model::graph_t> const& patterns,
            std::vector<std::vector<std::unordered_map<model::vertex_t, model::vertex_t>>> const&
                    embeddings_set,
            std::vector<std::vector<
                    std::pair<std::vector<model::Gfd::Literal>, std::vector<model::Gfd::Literal>>>>&
                    forbidden_rules_set,
            std::unordered_map<std::string,
                               std::unordered_map<std::string, std::set<std::string>>> const&
                    attrs_info);

    /* Filters those patterns whose frequency is less than
     * the input parameter sigma.
     */
    void FilterSupp(std::vector<model::graph_t>& patterns,
                    std::vector<std::vector<std::unordered_map<model::vertex_t, model::vertex_t>>>&
                            embeddings_set);

    /* Traverses the initial graph, remembers all the vertex and edge labels
     * present in it, all the embeddings of single-vertex patterns,
     * and also generates a structure that simplifies
     * the generation of literals for patterns.
     */
    void Initialize(
            std::set<std::string>& vertex_labels, std::set<std::string>& edge_labels,
            std::vector<model::graph_t>& patterns,
            std::vector<std::vector<std::unordered_map<model::vertex_t, model::vertex_t>>>&
                    embeddings_set,
            std::vector<std::vector<
                    std::pair<std::vector<model::Gfd::Literal>, std::vector<model::Gfd::Literal>>>>&
                    forbidden_rules_set,
            std::unordered_map<std::string, std::unordered_map<std::string, std::set<std::string>>>&
                    attrs_info);

public:
    void MineGfds();

    GfdMiner();

    GfdMiner(model::graph_t graph_) : Algorithm({}), graph_(graph_) {
        ExecutePrepare();
    }

    std::vector<model::Gfd> const& GfdList() const noexcept {
        return gfds_;
    }
};

}  // namespace algos
