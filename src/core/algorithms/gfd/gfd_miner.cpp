#include "algorithms/gfd/gfd_miner.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/graph/copy.hpp>
#include <boost/graph/isomorphism.hpp>
#include <boost/graph/vf2_sub_graph_iso.hpp>
#include <easylogging++.h>

#include "algorithms/gfd/comparator.h"
#include "config/option_using.h"
#include "util/timed_invoke.h"

namespace algos {

namespace {

using namespace details;
using namespace comparator;

using Embedding = std::map<vertex_t, vertex_t>;
using Embeddings = std::vector<Embedding>;

using SingleRule = std::pair<std::vector<Literal>, Literal>;
using Rule = std::pair<std::vector<Literal>, std::vector<Literal>>;
using Rules = std::vector<Rule>;

using Info = std::map<std::string, std::set<std::string>>;

void NextSubset(std::vector<std::size_t>& indices, std::size_t const border) {
    if (indices.at(0) == border - indices.size()) {
        indices = {};
        return;
    }
    std::size_t index = 0;
    for (int i = static_cast<int>(indices.size()) - 1; i >= 0; --i) {
        if (indices.at(i) != border - indices.size() + static_cast<std::size_t>(i)) {
            index = static_cast<std::size_t>(i);
            break;
        }
    }
    indices.at(index)++;
    for (std::size_t i = index + 1; i < indices.size(); ++i) {
        indices.at(i) = indices.at(index) + i - index;
    }
}

template <typename T>
std::vector<std::vector<T>> GetSubsets(std::vector<T> const& elements, std::size_t const n) {
    if (elements.size() < n) {
        return {};
    }
    std::vector<std::vector<T>> result = {};
    std::vector<std::size_t> indices = std::vector<std::size_t>(n);
    std::iota(std::begin(indices), std::end(indices), 0);
    while (!indices.empty()) {
        std::vector<T> element = std::vector<T>(indices.size());
        auto get_ith = [&elements](std::size_t i) { return elements.at(i); };
        std::transform(indices.begin(), indices.end(), element.begin(), get_ith);
        result.push_back(std::move(element));
        NextSubset(indices, elements.size());
    }
    return result;
}

bool CheckToken(Token const& token, std::string& tok, graph_t const& graph, graph_t const& pattern,
                Embedding const& embedding) {
    if (token.first == -1) {
        tok = token.second;
    } else {
        vertex_t u = boost::vertex(token.first, pattern);
        vertex_t v = embedding.at(u);
        std::map<std::string, std::string> attrs = graph[v].attributes;
        if (attrs.find(token.second) == attrs.end()) {
            return false;
        }
        tok = attrs.at(token.second);
    }
    return true;
}

bool Satisfied(std::vector<Literal> const& literals, graph_t const& graph, graph_t const& pattern,
               Embedding const& embedding) {
    for (Literal const& l : literals) {
        Token fst_token = l.first;
        Token snd_token = l.second;
        std::string fst, snd;
        bool first_res = CheckToken(fst_token, fst, graph, pattern, embedding);
        bool second_res = CheckToken(snd_token, snd, graph, pattern, embedding);
        if (!first_res or !second_res or fst != snd) return false;
    }
    return true;
}

bool Validate(graph_t const& graph, Gfd const& gfd, Embeddings const& embeddings,
              std::size_t const sigma) {
    bool result = false;
    std::map<vertex_t, std::set<vertex_t>> maps = {};
    for (Embedding const& embedding : embeddings) {
        if (!Satisfied(gfd.GetPremises(), graph, gfd.GetPattern(), embedding)) {
            continue;
        }
        result = true;

        if (!Satisfied(gfd.GetConclusion(), graph, gfd.GetPattern(), embedding)) {
            return false;
        }

        BGL_FORALL_VERTICES_T(v, gfd.GetPattern(), graph_t) {
            std::set<vertex_t> vertices = {embedding.at(v)};
            auto [it, emplaced] = maps.try_emplace(v, std::move(vertices));
            if (!emplaced) {
                it->second.insert(embedding.at(v));
            }
        }
    }

    std::size_t min_size = boost::num_vertices(graph);
    for (auto& [v, image] : maps) {
        min_size = std::min(image.size(), min_size);
    }
    return result && (min_size >= sigma);
}

std::vector<Literal> GenerateLiterals(graph_t const& pattern,
                                      std::map<std::string, Info> const& attrs_info) {
    std::vector<std::size_t> indices(boost::num_vertices(pattern));
    std::iota(std::begin(indices), std::end(indices), 0);
    std::vector<std::vector<std::size_t>> pairs = GetSubsets<std::size_t>(indices, 2);

    std::vector<Literal> result = {};
    for (auto const& index_pair : pairs) {
        std::size_t fst = index_pair.at(0);
        std::size_t snd = index_pair.at(1);
        std::string fst_label = pattern[boost::vertex(fst, pattern)].attributes.at("label");
        std::string snd_label = pattern[boost::vertex(snd, pattern)].attributes.at("label");

        for (auto const& fst_name : attrs_info.at(fst_label)) {
            Token fst_token = {fst, fst_name.first};
            for (auto const& snd_name : attrs_info.at(snd_label)) {
                Token snd_token = {snd, snd_name.first};
                result.emplace_back(fst_token, std::move(snd_token));
            }
        }
    }
    for (std::size_t i = 0; i < boost::num_vertices(pattern); ++i) {
        std::string label = pattern[boost::vertex(i, pattern)].attributes.at("label");
        for (auto& name : attrs_info.at(label)) {
            Token name_token = {i, name.first};
            for (auto& value : name.second) {
                Token value_token = {-1, value};
                Literal l = {name_token, std::move(value_token)};
                result.push_back(std::move(l));
            }
        }
    }
    return result;
}

std::size_t Support(graph_t const& graph, Embeddings const& embeddings, graph_t const& pattern) {
    std::map<vertex_t, std::set<vertex_t>> result = {};
    BGL_FORALL_VERTICES_T(v, pattern, graph_t) {
        for (Embedding const& embedding : embeddings) {
            std::set<vertex_t> vertices = {embedding.at(v)};
            auto [it, emplaced] = result.try_emplace(v, std::move(vertices));
            if (!emplaced) {
                it->second.insert(embedding.at(v));
            }
        }
    }

    std::size_t min_size = boost::num_vertices(graph);
    for (auto const& maps : result) {
        min_size = std::min(maps.second.size(), min_size);
    }
    return min_size;
}

bool IsForbidden(std::vector<Rules>::iterator const& forbidden_rules,
                 std::vector<Literal> const& subset, Literal const& l) {
    for (auto const& [premises, conclusion] : *forbidden_rules) {
        if (CompareLiteralSets(premises, subset) && ContainsLiteral(conclusion, l)) {
            return true;
        }
    }
    return false;
}

void HGenerate(std::vector<Literal>& notsatisfied, Literal const& l, std::vector<SingleRule>& rules,
               graph_t const& graph, graph_t& pattern, Embeddings const& embeddings,
               std::vector<Rules>::iterator const& forbidden_rules, std::size_t const sigma) {
    std::vector<Literal> conclusion = {l};
    for (std::size_t k = 1; k <= 3; ++k) {
        std::vector<std::vector<Literal>> subsets = GetSubsets<Literal>(notsatisfied, k);
        for (auto& subset : subsets) {
            if (IsForbidden(forbidden_rules, subset, l)) {
                continue;
            }

            Gfd gfd = {pattern, subset, conclusion};
            if (Validate(graph, gfd, embeddings, sigma)) {
                rules.emplace_back(subset, l);
                for (auto const& lit : subset) {
                    auto last = std::remove(notsatisfied.begin(), notsatisfied.end(), lit);
                    notsatisfied.erase(last, notsatisfied.end());
                }
            }
        }
    }
}

std::vector<SingleRule> GenerateRules(std::vector<Literal> const& literals, graph_t const& graph,
                                      graph_t& pattern, Embeddings const& embeddings,
                                      std::vector<Rules>::iterator const& forbidden_rules,
                                      std::size_t const sigma) {
    std::vector<SingleRule> rules = {};
    for (std::size_t j = 0; j < literals.size(); ++j) {
        std::vector<Literal> current = literals;
        Literal l = current.at(j);
        current.erase(std::next(current.begin(), j));

        if (IsForbidden(forbidden_rules, {}, l)) {
            continue;
        }

        std::vector<Literal> conclusion = {l};
        std::vector<Literal> premises = {};
        Gfd gfd = {pattern, premises, conclusion};
        if (Validate(graph, gfd, embeddings, sigma)) {
            rules.push_back({{}, l});
            continue;
        }
        std::vector<Literal> notsatisfied = current;
        HGenerate(notsatisfied, l, rules, graph, pattern, embeddings, forbidden_rules, sigma);
    }
    return rules;
}

void AddCompacted(std::vector<SingleRule>& rules, std::vector<Gfd>& result, graph_t& pattern,
                  std::vector<Rules>::iterator& forbidden_rules) {
    if (!rules.empty()) {
        std::sort(rules.begin(), rules.end());
        std::vector<Literal> prev = rules.front().first;
        std::vector<Literal> conclusion = {};
        for (auto const& [premises, l] : rules) {
            if (prev != premises) {
                Gfd gfd = {pattern, prev, conclusion};
                result.push_back(std::move(gfd));
                forbidden_rules->emplace_back(prev, conclusion);
                prev = premises;
                conclusion = {l};
            } else {
                conclusion.push_back(l);
            }
        }
        Gfd gfd = {pattern, prev, conclusion};
        result.push_back(std::move(gfd));
        forbidden_rules->emplace_back(std::move(prev), std::move(conclusion));
    }
}

void ChangeLiterals(std::vector<Literal>& literals, graph_t const& pattern, graph_t const& existed,
                    Embedding const& iso) {
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

class CompareCallback {
private:
    graph_t const& query_;
    Embedding& iso_;
    bool& res_;

public:
    CompareCallback(graph_t const& query, Embedding& iso, bool& res)
        : query_(query), iso_(iso), res_(res) {}

    template <typename CorrespondenceMap1To2, typename CorrespondenceMap2To1>
    bool operator()(CorrespondenceMap1To2 f, CorrespondenceMap2To1) const {
        BGL_FORALL_VERTICES_T(u, query_, graph_t) {
            iso_.emplace(u, get(f, u));
        }
        res_ = true;
        return false;
    }
};

struct VCompare {
    graph_t const& query;
    graph_t const& graph;

    bool operator()(vertex_t const& fr, vertex_t const& to) const {
        return query[fr].attributes.at("label") == graph[to].attributes.at("label");
    }
};

struct ECompare {
    graph_t const& query;
    graph_t const& graph;

    bool operator()(edge_t const& fr, edge_t const& to) const {
        return query[fr].label == graph[to].label;
    }
};

void UpdateExisted(bool& isoExists, graph_t const& new_pattern,
                   std::vector<graph_t> const& new_patterns,
                   std::vector<Rules>& new_forbidden_rules_set, Rules const& forbidden_rules) {
    std::size_t index = 0;
    for (auto const& existed : new_patterns) {
        VCompare vcompare{new_pattern, existed};
        ECompare ecompare{new_pattern, existed};
        Embedding iso;
        CompareCallback callback(new_pattern, iso, isoExists);
        boost::vf2_subgraph_iso(new_pattern, existed, callback,
                                get(boost::vertex_index, new_pattern),
                                get(boost::vertex_index, existed),
                                vertex_order_by_mult(new_pattern), ecompare, vcompare);
        if (!isoExists) {
            index++;
            continue;
        }
        Rules new_forbidden_rules(forbidden_rules);
        for (auto& [premises, conclusion] : new_forbidden_rules) {
            ChangeLiterals(premises, new_pattern, existed, iso);
            ChangeLiterals(conclusion, new_pattern, existed, iso);
            bool rule_exists = false;
            for (auto& [new_premises, new_conclusion] : new_forbidden_rules_set.at(index)) {
                if (CompareLiteralSets(premises, new_premises) &&
                    CompareLiteralSets(conclusion, new_conclusion)) {
                    rule_exists = true;
                    break;
                }
            }
            if (!rule_exists) {
                new_forbidden_rules_set.at(index).emplace_back(premises, conclusion);
            }
        }
        break;
    }
}

void AddEdge(std::set<std::string> const& edge_labels, graph_t const& graph, graph_t const& pattern,
             Embeddings const& embeddings, Rules const& forbidden_rules,
             std::vector<graph_t>& new_patterns, std::vector<Embeddings>& new_embeddings_set,
             std::vector<Rules>& new_forbidden_rules_set) {
    std::size_t num_vertices = boost::num_vertices(pattern);
    std::size_t max_edges = num_vertices * (num_vertices + 1) / 2;
    if (boost::num_edges(pattern) != max_edges) {
        std::vector<std::size_t> indices(num_vertices);
        std::iota(std::begin(indices), std::end(indices), 0);

        std::vector<std::vector<std::size_t>> pairs = GetSubsets<std::size_t>(indices, 2);
        for (std::size_t j = 0; j < num_vertices; ++j) {
            std::vector<std::size_t> element = {j, j};
            pairs.push_back(std::move(element));
        }
        vertex_t origin, finish;
        for (auto& pair : pairs) {
            vertex_t u = boost::vertex(pair.at(0), pattern);
            vertex_t v = boost::vertex(pair.at(1), pattern);

            if (!boost::edge(u, v, pattern).second) {
                origin = u;
                finish = v;
                break;
            }
        }

        for (auto& label : edge_labels) {
            graph_t new_pattern = {};
            boost::copy_graph(pattern, new_pattern);
            boost::add_edge(origin, finish, {std::move(label)}, new_pattern);

            bool iso_exists = false;
            UpdateExisted(iso_exists, new_pattern, new_patterns, new_forbidden_rules_set,
                          forbidden_rules);
            if (iso_exists) {
                continue;
            }

            Embeddings new_embeddings = {};
            for (auto& embedding : embeddings) {
                if (boost::edge(embedding.at(origin), embedding.at(finish), graph).second) {
                    new_embeddings.push_back(embedding);
                }
            }
            if (!new_embeddings.empty()) {
                new_forbidden_rules_set.push_back(std::move(forbidden_rules));
                new_patterns.push_back(std::move(new_pattern));
                new_embeddings_set.push_back(std::move(new_embeddings));
            }
        }
    }
}

void AddVertex(std::set<std::string> const& vertex_labels, std::set<std::string> const& edge_labels,
               graph_t const& graph, graph_t const& pattern, Embeddings const& embeddings,
               Rules const& forbidden_rules, std::vector<graph_t>& new_patterns,
               std::vector<Embeddings>& new_embeddings_set,
               std::vector<Rules>& new_forbidden_rules_set) {
    for (auto const& edge_label : edge_labels) {
        for (auto const& vertex_label : vertex_labels) {
            std::size_t num_pat_vertices = boost::num_vertices(pattern);
            for (std::size_t j = 0; j < num_pat_vertices; ++j) {
                vertex_t u = boost::vertex(j, pattern);
                Embeddings new_embeddings = {};

                graph_t new_pattern = {};
                boost::copy_graph(pattern, new_pattern);
                std::map<std::string, std::string> attributes = {{"label", vertex_label}};
                vertex_t new_vertex = boost::add_vertex(
                        Vertex{static_cast<int>(num_pat_vertices), attributes}, new_pattern);
                boost::add_edge(u, new_vertex, {edge_label}, new_pattern);

                bool iso_exists = false;
                UpdateExisted(iso_exists, new_pattern, new_patterns, new_forbidden_rules_set,
                              forbidden_rules);
                if (iso_exists) {
                    continue;
                }

                for (Embedding const& embedding : embeddings) {
                    vertex_t v = embedding.at(u);
                    boost::graph_traits<graph_t>::adjacency_iterator adjacency_it, adjacency_end;
                    boost::tie(adjacency_it, adjacency_end) = boost::adjacent_vertices(v, graph);
                    for (; adjacency_it != adjacency_end; ++adjacency_it) {
                        boost::graph_traits<graph_t>::adjacency_iterator pat_adjacency_it,
                                pat_adjacency_end;
                        boost::tie(pat_adjacency_it, pat_adjacency_end) =
                                boost::adjacent_vertices(u, pattern);
                        bool matched = false;
                        for (; pat_adjacency_it != pat_adjacency_end; ++pat_adjacency_it) {
                            if (embedding.at(*pat_adjacency_it) == *adjacency_it) {
                                matched = true;
                                break;
                            }
                        }
                        if (matched ||
                            graph[*adjacency_it].attributes.at("label") != vertex_label ||
                            graph[boost::edge(v, *adjacency_it, graph).first].label != edge_label) {
                            continue;
                        }
                        Embedding new_embedding(embedding);
                        new_embedding.emplace(new_vertex, std::move(*adjacency_it));
                        new_embeddings.push_back(std::move(new_embedding));
                    }
                }
                if (!new_embeddings.empty()) {
                    new_forbidden_rules_set.push_back(forbidden_rules);
                    new_patterns.push_back(std::move(new_pattern));
                    new_embeddings_set.push_back(std::move(new_embeddings));
                }
            }
        }
    }
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

void GfdMiner::HorizontalSpawn(std::vector<graph_t> const& patterns,
                               std::vector<Embeddings> const& embeddings_set,
                               std::vector<Rules>& forbidden_rules_set,
                               std::map<std::string, Info> const& attrs_info) {
    for (std::size_t i = 0; i < patterns.size(); ++i) {
        auto pattern = patterns.at(i);
        auto embeddings = embeddings_set.at(i);
        auto forbidden_rules = std::next(forbidden_rules_set.begin(), i);

        std::vector<Literal> literals = GenerateLiterals(pattern, attrs_info);
        std::vector<SingleRule> rules =
                GenerateRules(literals, graph_, pattern, embeddings, forbidden_rules, sigma_);
        AddCompacted(rules, gfds_, pattern, forbidden_rules);
    }
}

void GfdMiner::FilterSupp(std::vector<graph_t>& patterns, std::vector<Embeddings>& embeddings_set) {
    std::vector<std::size_t> del_indices = {};
    for (std::size_t i = 0; i < patterns.size(); ++i) {
        if (Support(graph_, embeddings_set.at(i), patterns.at(i)) < sigma_) {
            del_indices.push_back(i);
        }
    }
    std::reverse(del_indices.begin(), del_indices.end());
    for (auto& index : del_indices) {
        patterns[index] = patterns.back();
        patterns.pop_back();

        embeddings_set[index] = embeddings_set.back();
        embeddings_set.pop_back();
    }
}

void GfdMiner::Initialize(std::set<std::string>& vertex_labels, std::set<std::string>& edge_labels,
                          std::vector<graph_t>& patterns, std::vector<Embeddings>& embeddings_set,
                          std::vector<Rules>& forbidden_rules_set,
                          std::map<std::string, Info>& attrs_info) {
    std::vector<std::string> labels = {};
    BGL_FORALL_VERTICES_T(v, graph_, graph_t) {
        std::string label = graph_[v].attributes.at("label");
        vertex_labels.insert(label);
        std::vector<std::string>::iterator label_it =
                std::find(labels.begin(), labels.end(), label);
        if (label_it != labels.end()) {
            std::size_t index = label_it - labels.begin();
            Embedding embedding{{boost::vertex(0, patterns.at(index)), v}};
            embeddings_set.at(index).push_back(std::move(embedding));
        } else {
            graph_t pattern = {};
            std::map<std::string, std::string> attributes = {{"label", label}};

            boost::add_vertex(Vertex(0, attributes), pattern);
            patterns.push_back(pattern);

            labels.push_back(std::move(label));

            Embedding embedding{{boost::vertex(0, pattern), v}};
            embeddings_set.push_back({embedding});

            forbidden_rules_set.push_back({});
        }
        for (std::pair<std::string const, std::string> const& attr : graph_[v].attributes) {
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

    BGL_FORALL_EDGES_T(e, graph_, graph_t) {
        edge_labels.insert(graph_[e].label);
    }
}

/* Consists of a large loop that is executed while the set of patterns generated
 * in the previous iteration is non-empty
 * or the maximum number of iterations k is reached.
 */
void GfdMiner::MineGfds() {
    std::set<std::string> vertex_labels = {};
    std::set<std::string> edge_labels = {};
    std::vector<graph_t> patterns = {};

    std::vector<Embeddings> embeddings_set = {};
    std::vector<Rules> forbidden_rules_set = {};
    std::map<std::string, Info> attrs_info = {};

    Initialize(vertex_labels, edge_labels, patterns, embeddings_set, forbidden_rules_set,
               attrs_info);
    FilterSupp(patterns, embeddings_set);

    while (!patterns.empty()) {
        HorizontalSpawn(patterns, embeddings_set, forbidden_rules_set, attrs_info);

        std::vector<graph_t> new_patterns = {};
        std::vector<Embeddings> new_embeddings_set = {};
        std::vector<Rules> new_forbidden_rules_set = {};

        for (std::size_t i = 0; i < patterns.size(); ++i) {
            graph_t pattern = patterns.at(i);
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
        patterns = new_patterns;
        embeddings_set = new_embeddings_set;
        forbidden_rules_set = new_forbidden_rules_set;
        FilterSupp(patterns, embeddings_set);
    }
}

}  // namespace algos