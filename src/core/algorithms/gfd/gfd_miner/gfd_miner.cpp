#include "algorithms/gfd/gfd_miner/gfd_miner.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iosfwd>
#include <iterator>
#include <list>
#include <map>
#include <numeric>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/graph/adjacency_iterator.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/copy.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/vf2_sub_graph_iso.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/range/irange.hpp>
#include <boost/tuple/tuple.hpp>
#include <easylogging++.h>

#include "algorithm.h"
#include "algorithms/gfd/comparator.h"
#include "config/option_using.h"
#include "descriptions.h"
#include "gfd/gfd.h"
#include "graph_parser/graph_parser.h"
#include "names.h"
#include "option.h"
#include "util/timed_invoke.h"

namespace algos {

namespace {

// A mapping from a query to a subgraph of a larger graph
using Embedding = std::unordered_map<model::vertex_t, model::vertex_t>;
// A set of mappings
using Embeddings = std::vector<Embedding>;

// A construct that is considered to be satisfied
// if all literals on the left-hand side determine
// the satisfiability of the right-hand side
using SingleRule = std::pair<std::vector<model::Gfd::Literal>, model::Gfd::Literal>;
// Single Rule, but the right-hand side consists of few literals
using Rule = std::pair<std::vector<model::Gfd::Literal>, std::vector<model::Gfd::Literal>>;
// A set of rules
using Rules = std::vector<Rule>;

// A data structure designed to store sets of values
// that each field can take
//
// FieldName -> FieldValues
using Info = std::unordered_map<std::string, std::set<std::string>>;

// A data structure that has as its key a pattern vertex
// and as its value a set of graph vertices
// to which the vertex can be mapped
using VertexImage = std::unordered_map<model::vertex_t, std::set<model::vertex_t>>;

// A data structure that returns by literal a set of
// indices of the embeddings on which the literal is satisfied
using SatisfiedEmbeddings = std::map<model::Gfd::Literal, std::set<std::size_t>>;

void NextSubset(std::vector<std::size_t>& indices, std::size_t const border) {
    if (indices.empty() || indices[0] == border - indices.size()) {
        indices.clear();
        return;
    }
    std::size_t i = indices.size();
    while (i > 0) {
        --i;
        if (indices[i] < border - indices.size() + i) {
            ++indices[i];
            for (std::size_t j = i + 1; j < indices.size(); ++j) {
                indices[j] = indices[j - 1] + 1;
            }
            return;
        }
    }
}

template <typename T>
std::vector<std::vector<T>> GetSubsets(std::vector<T> const& elements, std::size_t n) {
    if (elements.size() < n) {
        return {};
    }
    std::vector<std::vector<T>> result;
    std::vector<std::size_t> indices(n);
    std::iota(indices.begin(), indices.end(), 0);
    while (!indices.empty()) {
        std::vector<T> element(indices.size());
        auto get_ith = [&elements](std::size_t i) { return elements.at(i); };
        std::ranges::transform(indices, element.begin(), get_ith);
        result.emplace_back(element);
        NextSubset(indices, elements.size());
    }
    return result;
}

std::size_t GetMinSize(VertexImage const& vertex_map, std::size_t fallback_size) {
    if (vertex_map.empty()) return fallback_size;
    auto get_image_size = [](auto const& map) {
        auto const& [_, image] = map;
        return image.size();
    };
    auto sizes = vertex_map | std::views::transform(get_image_size);
    return std::ranges::min(sizes);
}

template <typename MapType, typename EmbeddingType>
void PopulateVertexMap(MapType& vertex_map, model::graph_t const& pattern,
                       EmbeddingType const& embedding) {
    BGL_FORALL_VERTICES_T(v, pattern, model::graph_t) {
        auto [it, emplaced] = vertex_map.try_emplace(v, std::set<model::vertex_t>{embedding.at(v)});
        if (!emplaced) {
            it->second.insert(embedding.at(v));
        }
    }
}

std::size_t Support(model::graph_t const& graph, Embeddings const& embeddings,
                    model::graph_t const& pattern) {
    VertexImage vertex_map;
    for (Embedding const& embedding : embeddings) {
        PopulateVertexMap(vertex_map, pattern, embedding);
    }

    return GetMinSize(vertex_map, boost::num_vertices(graph));
}

bool IsForbidden(Rules const& forbidden_rules, std::vector<model::Gfd::Literal> const& subset,
                 model::Gfd::Literal const& l) {
    auto is_forbidden = [&subset, &l](auto const& rule) {
        auto const& [premises, conclusion] = rule;
        return gfd::comparator::CompareLiteralSets(premises, subset) &&
               gfd::comparator::ContainsLiteral(conclusion, l);
    };
    return std::ranges::any_of(forbidden_rules, is_forbidden);
}

void ChangeLiterals(std::vector<model::Gfd::Literal>& literals, model::graph_t const& pattern,
                    model::graph_t const& existed, Embedding const& iso) {
    for (auto& [fstToken, sndToken] : literals) {
        if (fstToken.first != -1) {
            auto vertex = boost::vertex(fstToken.first, pattern);
            fstToken.first = existed[iso.at(vertex)].node_id;
        }
        if (sndToken.first != -1) {
            auto vertex = boost::vertex(sndToken.first, pattern);
            sndToken.first = existed[iso.at(vertex)].node_id;
        }
    }
}

void UpdateRules(std::size_t index, model::graph_t const& existed, Embedding const& iso,
                 model::graph_t const& new_pattern, std::vector<Rules>& new_forbidden_rules_set,
                 Rules const& forbidden_rules) {
    Rules new_forbidden_rules(forbidden_rules);
    Rules& rules = new_forbidden_rules_set.at(index);
    for (auto& forbidden_rule : new_forbidden_rules) {
        auto& premises = forbidden_rule.first;
        auto& conclusion = forbidden_rule.second;
        ChangeLiterals(premises, new_pattern, existed, iso);
        ChangeLiterals(conclusion, new_pattern, existed, iso);
        auto rule_eq = [&premises, &conclusion](auto const& existing_rule) {
            auto& [new_premises, new_conclusion] = existing_rule;
            return gfd::comparator::CompareLiteralSets(premises, new_premises) &&
                   gfd::comparator::CompareLiteralSets(conclusion, new_conclusion);
        };
        bool rule_exists = std::ranges::any_of(rules, rule_eq);
        if (!rule_exists) {
            rules.emplace_back(std::move(premises), std::move(conclusion));
        }
    }
}

void UpdateExisted(bool& isoExists, model::graph_t const& new_pattern,
                   std::vector<model::graph_t> const& new_patterns,
                   std::vector<Rules>& new_forbidden_rules_set, Rules const& forbidden_rules) {
    std::size_t index = 0;
    for (auto const& existed : new_patterns) {
        auto vcompare = [&new_pattern, &existed](model::vertex_t const& fr,
                                                 model::vertex_t const& to) {
            return new_pattern[fr].attributes.at("label") == existed[to].attributes.at("label");
        };
        auto ecompare = [&new_pattern, &existed](model::edge_t const& fr, model::edge_t const& to) {
            return new_pattern[fr].label == existed[to].label;
        };
        Embedding iso;
        auto callback = [&new_pattern, &iso, &isoExists](auto f, auto) {
            BGL_FORALL_VERTICES_T(u, new_pattern, model::graph_t) {
                iso.emplace(u, boost::get(f, u));
            }
            isoExists = true;
            return false;
        };
        using property_map_type = boost::property_map<model::graph_t, boost::vertex_index_t>::type;
        property_map_type new_index_map = boost::get(boost::vertex_index, new_pattern);
        property_map_type existed_index_map = boost::get(boost::vertex_index, existed);
        std::vector<model::vertex_t> new_vertex_order = vertex_order_by_mult(new_pattern);
        boost::vf2_subgraph_iso(new_pattern, existed, callback, new_index_map, existed_index_map,
                                new_vertex_order, ecompare, vcompare);
        if (!isoExists) {
            index++;
            continue;
        }
        UpdateRules(index, existed, iso, new_pattern, new_forbidden_rules_set, forbidden_rules);
        break;
    }
}

template <typename ModifyPattern, typename EmbeddingTransform>
void TryAddPattern(model::graph_t const& base_pattern, Embeddings const& embeddings,
                   Rules const& forbidden_rules, std::vector<model::graph_t>& new_patterns,
                   std::vector<Embeddings>& new_embeddings_set,
                   std::vector<Rules>& new_forbidden_rules_set, ModifyPattern&& modify_pattern,
                   EmbeddingTransform&& embedding_transform) {
    model::graph_t new_pattern;
    boost::copy_graph(base_pattern, new_pattern);

    modify_pattern(new_pattern);

    bool iso_exists = false;
    UpdateExisted(iso_exists, new_pattern, new_patterns, new_forbidden_rules_set, forbidden_rules);
    if (iso_exists) return;

    Embeddings new_embeddings;
    for (auto embedding_vector : embeddings | std::views::transform(embedding_transform)) {
        for (auto embedding : embedding_vector) {
            new_embeddings.push_back(embedding);
        }
    }

    if (!new_embeddings.empty()) {
        new_forbidden_rules_set.push_back(forbidden_rules);
        new_patterns.push_back(std::move(new_pattern));
        new_embeddings_set.push_back(std::move(new_embeddings));
    }
}

void AddEdge(std::set<std::string> const& edge_labels, model::graph_t const& graph,
             model::graph_t const& pattern, Embeddings const& embeddings,
             Rules const& forbidden_rules, std::vector<model::graph_t>& new_patterns,
             std::vector<Embeddings>& new_embeddings_set,
             std::vector<Rules>& new_forbidden_rules_set) {
    std::size_t num_vertices = boost::num_vertices(pattern);
    std::size_t max_edges = num_vertices * (num_vertices + 1) / 2 + num_vertices;
    if (boost::num_edges(pattern) == max_edges) {
        return;
    }
    std::vector<std::size_t> indices(num_vertices);
    std::iota(indices.begin(), indices.end(), 0);

    std::vector<std::vector<std::size_t>> pairs = GetSubsets<std::size_t>(indices, 2);
    for (std::size_t j = 0; j < num_vertices; ++j) {
        std::vector<std::size_t> element = {j, j};
        pairs.push_back(std::move(element));
    }
    model::vertex_t origin, finish;
    auto exists_edge = [&pattern](auto const& pair) {
        model::vertex_t u = boost::vertex(pair[0], pattern);
        model::vertex_t v = boost::vertex(pair[1], pattern);
        return !boost::edge(u, v, pattern).second;
    };
    auto it = std::ranges::find_if(pairs, exists_edge);
    if (it != pairs.end()) {
        origin = boost::vertex(it->at(0), pattern);
        finish = boost::vertex(it->at(1), pattern);
    }

    auto embedding_transform = [&origin, &finish, &graph](Embedding const& embedding) {
        bool edge_exists = boost::edge(embedding.at(origin), embedding.at(finish), graph).second;
        return edge_exists ? std::vector<Embedding>{embedding} : std::vector<Embedding>{};
    };

    for (auto& label : edge_labels) {
        auto modify_pattern = [&origin, &finish, &label](model::graph_t& new_pattern) {
            boost::add_edge(origin, finish, {std::move(label)}, new_pattern);
        };
        TryAddPattern(pattern, embeddings, forbidden_rules, new_patterns, new_embeddings_set,
                      new_forbidden_rules_set, modify_pattern, embedding_transform);
    }
}

void AddVertex(std::set<std::string> const& vertex_labels, std::set<std::string> const& edge_labels,
               model::graph_t const& graph, model::graph_t const& pattern,
               Embeddings const& embeddings, Rules const& forbidden_rules,
               std::vector<model::graph_t>& new_patterns,
               std::vector<Embeddings>& new_embeddings_set,
               std::vector<Rules>& new_forbidden_rules_set) {
    std::size_t num_pat_vertices = boost::num_vertices(pattern);
    for (auto const& edge_label : edge_labels) {
        for (auto const& vertex_label : vertex_labels) {
            for (std::size_t j = 0; j < num_pat_vertices; ++j) {
                model::vertex_t u = boost::vertex(j, pattern);
                model::vertex_t curr_vertex;

                auto embedding_transform = [&u, &graph, &pattern, &vertex_label, &edge_label,
                                            &curr_vertex](Embedding const& embedding) {
                    Embeddings res;
                    model::vertex_t v = embedding.at(u);
                    boost::graph_traits<model::graph_t>::adjacency_iterator adjacency_it,
                            adjacency_end;
                    boost::tie(adjacency_it, adjacency_end) = boost::adjacent_vertices(v, graph);
                    for (; adjacency_it != adjacency_end; ++adjacency_it) {
                        boost::graph_traits<model::graph_t>::adjacency_iterator pat_adjacency_it,
                                pat_adjacency_end;
                        boost::tie(pat_adjacency_it, pat_adjacency_end) =
                                boost::adjacent_vertices(u, pattern);
                        auto super_exists = [&embedding, &adjacency_it](auto const& it) {
                            return embedding.at(it) == *adjacency_it;
                        };
                        auto match_it =
                                std::find_if(pat_adjacency_it, pat_adjacency_end, super_exists);
                        bool matched = match_it != pat_adjacency_end;
                        if (matched ||
                            graph[*adjacency_it].attributes.at("label") != vertex_label ||
                            graph[boost::edge(v, *adjacency_it, graph).first].label != edge_label) {
                            continue;
                        }
                        Embedding new_embedding(embedding);
                        new_embedding.emplace(curr_vertex, std::move(*adjacency_it));
                        res.push_back(std::move(new_embedding));
                    }
                    return res;
                };

                auto modify_pattern = [&u, &num_pat_vertices, &vertex_label, &edge_label,
                                       &curr_vertex](model::graph_t& new_pattern) {
                    std::unordered_map<std::string, std::string> attributes = {
                            {"label", vertex_label}};
                    model::Vertex new_v =
                            model::Vertex{static_cast<int>(num_pat_vertices), attributes};
                    curr_vertex = boost::add_vertex(new_v, new_pattern);
                    boost::add_edge(u, curr_vertex, {edge_label}, new_pattern);
                };

                TryAddPattern(pattern, embeddings, forbidden_rules, new_patterns,
                              new_embeddings_set, new_forbidden_rules_set, modify_pattern,
                              embedding_transform);
            }
        }
    }
}

std::vector<model::Gfd::Literal> index_to_literal(std::vector<model::Gfd::Literal> const& literals,
                                                  std::vector<std::size_t> const& indices) {
    std::vector<model::Gfd::Literal> result = {};
    result.reserve(indices.size());
    for (auto const& index : indices) {
        result.push_back(literals[index]);
    }
    return result;
}

bool check_rules(std::vector<model::Gfd::Literal> const& literals,
                 std::unordered_map<std::size_t, std::vector<std::vector<std::size_t>>> const&
                         reversed_rules,
                 std::vector<std::size_t> const& lhs_indices, std::size_t rhs_index) {
    if (reversed_rules.find(rhs_index) == reversed_rules.end()) return true;
    std::vector<model::Gfd::Literal> lhs = index_to_literal(literals, lhs_indices);
    for (auto const& premises_indices : reversed_rules.at(rhs_index)) {
        std::vector<model::Gfd::Literal> premises = index_to_literal(literals, premises_indices);
        if (gfd::comparator::ContainsLiterals(lhs, premises)) {
            return false;
        }
    }
    return true;
}

bool check_deadlocks(std::vector<std::size_t> const& indices,
                     std::set<std::vector<std::size_t>> const& deadlocks) {
    auto check = [&indices](auto const& deadlock) {
        return !std::ranges::includes(indices, deadlock);
    };
    return std::ranges::all_of(deadlocks, check);
}

std::vector<std::size_t> get_rhs(std::size_t n, std::vector<std::size_t> const& lhs) {
    std::vector<std::size_t> result(n);
    std::iota(result.begin(), result.end(), 0);
    auto cond = [&lhs](auto index) {
        return std::find(lhs.begin(), lhs.end(), index) != lhs.end();
    };
    std::erase_if(result, cond);
    return result;
}

}  // namespace

GfdMiner::GfdMiner() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable(
            {config::names::kGraphData, config::names::kGfdK, config::names::kGfdSigma});
}

void GfdMiner::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(config::Option{&graph_path_, kGraphData, kDGraphData});
    RegisterOption(config::Option{&k_, kGfdK, kDGfdK});
    RegisterOption(config::Option{&sigma_, kGfdSigma, kDGfdSigma});
}

void GfdMiner::LoadDataInternal() {
    std::ifstream f(graph_path_);
    graph_ = parser::graph_parser::ReadGraph(f);
}

void GfdMiner::ResetState() {}

unsigned long long GfdMiner::ExecuteInternal() {
    std::size_t elapsed_time = util::TimedInvoke(&GfdMiner::MineGfds, this);
    LOG(DEBUG) << "Mined GFDs: " << gfds_.size();
    return elapsed_time;
}

bool GfdMiner::CheckToken(model::Gfd::Token const& token, std::string& tok,
                          model::graph_t const& pattern, Embedding const& embedding) {
    if (token.first == -1) {
        tok = token.second;
        return true;
    } else {
        model::vertex_t u = boost::vertex(token.first, pattern);
        model::vertex_t v = embedding.at(u);
        std::unordered_map<std::string, std::string> const& attrs = graph_[v].attributes;
        auto it = attrs.find(token.second);
        if (it != attrs.end()) {
            tok = it->second;
            return true;
        }
        return false;
    }
}

void GfdMiner::AddCompacted(std::vector<SingleRule>& rules, model::graph_t& pattern,
                            Rules& forbidden_rules) {
    if (rules.empty()) return;
    std::ranges::sort(rules);
    std::vector<model::Gfd::Literal> prev = rules.front().first;
    std::vector<model::Gfd::Literal> conclusion = {};
    for (auto const& [premises, l] : rules) {
        if (prev != premises) {
            gfds_.emplace_back(pattern, prev, conclusion);
            forbidden_rules.emplace_back(std::move(prev), std::move(conclusion));
            prev = premises;
            conclusion = {l};
        } else {
            conclusion.push_back(l);
        }
    }
    gfds_.emplace_back(pattern, prev, conclusion);
    forbidden_rules.emplace_back(std::move(prev), std::move(conclusion));
}

bool GfdMiner::CheckFrequency(std::set<std::size_t> const& indices, Embeddings const& embeddings,
                              model::graph_t const& pattern) {
    VertexImage vertex_map;
    for (std::size_t index : indices) {
        PopulateVertexMap(vertex_map, pattern, embeddings[index]);
    }
    std::size_t min_size = GetMinSize(vertex_map, boost::num_vertices(graph_));
    return min_size >= sigma_;
}

bool GfdMiner::Validate(std::vector<model::Gfd::Literal> const& lhs, model::Gfd::Literal const& rhs,
                        model::graph_t& pattern, Embeddings const& embeddings,
                        SatisfiedEmbeddings const& satisfied_embeddings) {
    std::set<std::size_t> lhs_embeddings;
    if (lhs.empty()) {
        for (std::size_t i = 0; i < embeddings.size(); ++i) {
            lhs_embeddings.insert(i);
        }
    } else {
        lhs_embeddings = satisfied_embeddings.at(lhs.at(0));
        for (std::size_t i = 1; i < lhs.size(); i++) {
            std::set<std::size_t> intersect;
            std::set<std::size_t> current = satisfied_embeddings.at(lhs.at(i));
            std::ranges::set_intersection(current, lhs_embeddings,
                                          std::inserter(intersect, intersect.begin()));
            lhs_embeddings = intersect;
        }
    }
    std::set<std::size_t> rhs_embeddings = satisfied_embeddings.at(rhs);
    return std::ranges::includes(rhs_embeddings, lhs_embeddings) &&
           CheckFrequency(lhs_embeddings, embeddings, pattern);
}

bool GfdMiner::CheckFrequency(std::vector<model::Gfd::Literal> const& subset,
                              SatisfiedEmbeddings const& satisfied_embeddings) {
    if (subset.empty()) return true;
    std::set<std::size_t> lhs = satisfied_embeddings.at(subset.at(0));
    for (std::size_t i = 1; i < subset.size(); i++) {
        std::set<std::size_t> intersect;
        std::set<std::size_t> current = satisfied_embeddings.at(subset.at(i));
        std::ranges::set_intersection(current, lhs, std::inserter(intersect, intersect.begin()));
        lhs = intersect;
    }
    return lhs.size() >= sigma_;
}

std::vector<SingleRule> GfdMiner::GenerateRules(std::vector<model::Gfd::Literal> const& literals,
                                                model::graph_t& pattern,
                                                Embeddings const& embeddings,
                                                Rules const& forbidden_rules,
                                                SatisfiedEmbeddings const& satisfied_embeddings) {
    std::vector<SingleRule> rules;
    std::unordered_map<std::size_t, std::vector<std::vector<std::size_t>>> reversed_rules;
    std::set<std::vector<std::size_t>> deadlocks;
    std::set<std::vector<std::size_t>> lhs_indices_set = {{}};

    while (!lhs_indices_set.empty()) {
        std::set<std::vector<std::size_t>> new_lhs_indices_set = {};
        for (auto const& lhs_indices : lhs_indices_set) {
            std::vector<model::Gfd::Literal> lhs = index_to_literal(literals, lhs_indices);
            for (auto const& rhs_index : get_rhs(literals.size(), lhs_indices)) {
                model::Gfd::Literal rhs = literals.at(rhs_index);
                if (IsForbidden(forbidden_rules, lhs, rhs) ||
                    !check_rules(literals, reversed_rules, lhs_indices, rhs_index)) {
                    continue;
                }

                if (Validate(lhs, rhs, pattern, embeddings, satisfied_embeddings)) {
                    std::vector<std::size_t> deadlock(lhs_indices.begin(), lhs_indices.end());
                    deadlock.push_back(rhs_index);
                    std::sort(deadlock.begin(), deadlock.end());
                    deadlocks.emplace(std::move(deadlock));
                    std::vector<std::vector<std::size_t>> value = {lhs_indices};
                    auto [reversed_rule, rule_emplaced] =
                            reversed_rules.try_emplace(rhs_index, std::move(value));
                    if (!rule_emplaced) {
                        reversed_rule->second.push_back(lhs_indices);
                    }
                    rules.emplace_back(lhs, rhs);
                }
            }
        }

        for (auto const& lhs_indices : lhs_indices_set) {
            for (std::size_t i = lhs_indices.empty() ? 0 : std::ranges::max(lhs_indices) + 1;
                 i < literals.size(); ++i) {
                std::vector<std::size_t> new_lhs_indices(lhs_indices.begin(), lhs_indices.end());
                new_lhs_indices.emplace_back(i);

                if (!check_deadlocks(new_lhs_indices, deadlocks)) {
                    continue;
                }
                if (CheckFrequency(index_to_literal(literals, new_lhs_indices),
                                   satisfied_embeddings)) {
                    new_lhs_indices_set.emplace(std::move(new_lhs_indices));
                } else {
                    deadlocks.emplace(std::move(new_lhs_indices));
                }
            }
        }
        lhs_indices_set = std::move(new_lhs_indices_set);
    }
    return rules;
}

bool GfdMiner::LiteralSatisfied(model::Gfd::Literal const& l, Embedding const& embedding,
                                model::graph_t const& pattern) {
    auto const& [fst_token, snd_token] = l;
    std::string fst, snd;
    bool first_res = CheckToken(fst_token, fst, pattern, embedding);
    bool second_res = CheckToken(snd_token, snd, pattern, embedding);
    return first_res && second_res && (fst == snd);
}

std::vector<model::Gfd::Literal> GfdMiner::GenerateLiterals(
        model::graph_t const& pattern, std::unordered_map<std::string, Info> const& attrs_info,
        Embeddings const& embeddings, SatisfiedEmbeddings& satisfied_embeddings_set) {
    std::vector<std::size_t> indices(boost::num_vertices(pattern));
    std::iota(indices.begin(), indices.end(), 0);
    std::vector<std::vector<std::size_t>> pairs = GetSubsets<std::size_t>(indices, 2);

    std::vector<model::Gfd::Literal> result;
    result.reserve(pairs.size());

    auto try_add_literal = [&embeddings, &pattern, this, &satisfied_embeddings_set, &result](
                                   model::Gfd::Token const& fst_token,
                                   model::Gfd::Token const& snd_token) {
        model::Gfd::Literal l = {fst_token, snd_token};
        std::set<std::size_t> satisfied_embeddings;
        std::size_t i = 0;
        for (Embedding const& embedding : embeddings) {
            if (LiteralSatisfied(l, embedding, pattern)) {
                satisfied_embeddings.insert(i);
            }
            i++;
        }
        if (satisfied_embeddings.size() >= sigma_) {
            satisfied_embeddings_set.try_emplace(l, std::move(satisfied_embeddings));
            result.emplace_back(fst_token, std::move(snd_token));
        }
    };

    for (auto const& index_pair : pairs) {
        std::size_t fst = index_pair.at(0);
        std::size_t snd = index_pair.at(1);
        std::string const& fst_label = pattern[boost::vertex(fst, pattern)].attributes.at("label");
        std::string const& snd_label = pattern[boost::vertex(snd, pattern)].attributes.at("label");

        if (attrs_info.find(fst_label) == attrs_info.end() ||
            attrs_info.find(snd_label) == attrs_info.end()) {
            continue;
        }
        for (auto const& fst_name : attrs_info.at(fst_label)) {
            model::Gfd::Token fst_token{fst, fst_name.first};
            for (auto const& snd_name : attrs_info.at(snd_label)) {
                std::set<std::string> intersect;
                std::ranges::set_intersection(fst_name.second, snd_name.second,
                                              std::inserter(intersect, intersect.begin()));
                if (intersect.empty()) {
                    continue;
                }

                model::Gfd::Token snd_token{snd, snd_name.first};

                try_add_literal(fst_token, snd_token);
            }
        }
    }

    for (std::size_t i = 0; i < boost::num_vertices(pattern); ++i) {
        std::string const& label = pattern[boost::vertex(i, pattern)].attributes.at("label");
        if (attrs_info.find(label) == attrs_info.end()) {
            continue;
        }
        for (auto& [name, info] : attrs_info.at(label)) {
            model::Gfd::Token name_token{i, name};
            for (auto& value : info) {
                model::Gfd::Token value_token{-1, value};

                try_add_literal(name_token, value_token);
            }
        }
    }
    return result;
}

void GfdMiner::HorizontalSpawn(std::vector<model::graph_t> const& patterns,
                               std::vector<Embeddings> const& embeddings_set,
                               std::vector<Rules>& forbidden_rules_set,
                               std::unordered_map<std::string, Info> const& attrs_info) {
    for (std::size_t i = 0; i < patterns.size(); ++i) {
        auto pattern = patterns[i];
        auto const& embeddings = embeddings_set[i];
        auto& forbidden_rules = forbidden_rules_set[i];

        SatisfiedEmbeddings satisfied_embeddings;
        std::vector<model::Gfd::Literal> literals =
                GenerateLiterals(pattern, attrs_info, embeddings, satisfied_embeddings);

        std::vector<SingleRule> rules =
                GenerateRules(literals, pattern, embeddings, forbidden_rules, satisfied_embeddings);

        AddCompacted(rules, pattern, forbidden_rules);
    }
}

void GfdMiner::FilterSupp(std::vector<model::graph_t>& patterns,
                          std::vector<Embeddings>& embeddings_set) {
    std::vector<std::size_t> del_indices = {};
    for (std::size_t i = 0; i < patterns.size(); ++i) {
        if (Support(graph_, embeddings_set.at(i), patterns.at(i)) < sigma_) {
            del_indices.push_back(i);
        }
    }
    std::reverse(del_indices.begin(), del_indices.end());
    for (std::size_t index : del_indices) {
        patterns[index] = patterns.back();
        patterns.pop_back();

        embeddings_set[index] = embeddings_set.back();
        embeddings_set.pop_back();
    }
}

void GfdMiner::Initialize(std::set<std::string>& vertex_labels, std::set<std::string>& edge_labels,
                          std::vector<model::graph_t>& patterns,
                          std::vector<Embeddings>& embeddings_set,
                          std::vector<Rules>& forbidden_rules_set,
                          std::unordered_map<std::string, Info>& attrs_info) {
    std::unordered_map<std::string, std::size_t> label_to_index;
    BGL_FORALL_VERTICES_T(v, graph_, model::graph_t) {
        std::unordered_map<std::string, std::string> graph_attributes = graph_[v].attributes;
        std::string const& label = graph_attributes.at("label");
        vertex_labels.insert(label);

        auto [all_labels_it, all_labels_emplaced] =
                label_to_index.try_emplace(label, patterns.size());

        if (!all_labels_emplaced) {
            std::size_t index = all_labels_it->second;
            Embedding embedding{{boost::vertex(0, patterns.at(index)), v}};
            embeddings_set.at(index).push_back(std::move(embedding));
        } else {
            model::graph_t pattern = {};
            std::unordered_map<std::string, std::string> attributes = {{"label", label}};

            boost::add_vertex(model::Vertex{0, attributes}, pattern);
            patterns.push_back(pattern);

            Embedding embedding{{boost::vertex(0, pattern), v}};
            embeddings_set.emplace_back(std::vector{embedding});

            forbidden_rules_set.push_back({});
        }
        for (std::pair<std::string const, std::string> const& attr : graph_attributes) {
            if (attr.first == "label") {
                continue;
            }
            std::set<std::string> attrs = {attr.second};
            Info values{{attr.first, attrs}};
            auto [label_info, label_emplaced] = attrs_info.try_emplace(label, std::move(values));
            if (!label_emplaced) {
                std::set<std::string> value = {attr.second};
                auto [it, emplaced] = label_info->second.try_emplace(attr.first, std::move(value));
                if (!emplaced) {
                    it->second.insert(attr.second);
                }
            }
        }
    }

    BGL_FORALL_EDGES_T(e, graph_, model::graph_t) {
        edge_labels.insert(graph_[e].label);
    }
}

// Consists of a large loop that is executed while the set of patterns generated
// in the previous iteration is non-empty
// or the maximum number of iterations k is reached.
void GfdMiner::MineGfds() {
    std::set<std::string> vertex_labels;
    std::set<std::string> edge_labels;
    std::vector<model::graph_t> patterns;
    std::vector<Embeddings> embeddings_set;
    std::vector<Rules> forbidden_rules_set;
    std::unordered_map<std::string, Info> attrs_info;

    Initialize(vertex_labels, edge_labels, patterns, embeddings_set, forbidden_rules_set,
               attrs_info);
    FilterSupp(patterns, embeddings_set);

    while (!patterns.empty()) {
        HorizontalSpawn(patterns, embeddings_set, forbidden_rules_set, attrs_info);

        std::vector<model::graph_t> new_patterns;
        std::vector<Embeddings> new_embeddings_set;
        std::vector<Rules> new_forbidden_rules_set;

        for (std::size_t i = 0; i < patterns.size(); ++i) {
            model::graph_t pattern = patterns.at(i);
            Embeddings embeddings = embeddings_set.at(i);
            Rules forbidden_rules = forbidden_rules_set.at(i);

            AddEdge(edge_labels, graph_, pattern, embeddings, forbidden_rules, new_patterns,
                    new_embeddings_set, new_forbidden_rules_set);

            std::size_t num_pat_vertices = boost::num_vertices(pattern);
            if (num_pat_vertices >= k_ || num_pat_vertices >= boost::num_vertices(graph_)) {
                continue;
            }

            AddVertex(vertex_labels, edge_labels, graph_, pattern, embeddings, forbidden_rules,
                      new_patterns, new_embeddings_set, new_forbidden_rules_set);
        }
        patterns = std::move(new_patterns);
        embeddings_set = std::move(new_embeddings_set);
        forbidden_rules_set = std::move(new_forbidden_rules_set);
        FilterSupp(patterns, embeddings_set);
    }
}

}  // namespace algos
