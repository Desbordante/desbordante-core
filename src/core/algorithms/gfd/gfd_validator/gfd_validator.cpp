#include "algorithms/gfd/gfd_validator/gfd_validator.h"

#include <cstddef>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/eccentricity.hpp>
#include <boost/graph/exterior_property.hpp>
#include <boost/graph/floyd_warshall_shortest.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/named_function_params.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/property_maps/constant_property_map.hpp>
#include <boost/graph/property_maps/container_property_map.hpp>
#include <boost/graph/vf2_sub_graph_iso.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/range/irange.hpp>
#include <boost/tuple/tuple.hpp>
#include <easylogging++.h>

#include "algorithms/gfd/gfd_validator/balancer.h"
#include "common_option.h"
#include "config/names_and_descriptions.h"
#include "config/thread_number/option.h"
#include "gfd/gfd.h"
#include "gfd/gfd_validator/gfd_handler.h"
#include "thread_number/type.h"

namespace {

using namespace algos;

std::vector<std::vector<model::vertex_t>> GetPartition(
        std::vector<model::vertex_t> const& candidates, config::ThreadNumType const& threads_num) {
    std::vector<std::vector<model::vertex_t>> result = {};

    if (candidates.empty()) {
        return {};
    }

    int musthave = candidates.size() / threads_num;
    int oversized_num = candidates.size() % threads_num;

    std::vector<model::vertex_t>::const_iterator from, to;
    from = candidates.begin();
    to = std::next(candidates.begin(), musthave + 1);
    for (int i = 0; i < oversized_num; ++i) {
        std::vector<model::vertex_t> temp(from, to);
        result.push_back(temp);
        from = std::next(from, musthave + 1);
        to = std::next(to, musthave + 1);
    }
    to--;
    for (int i = 0; i < threads_num - oversized_num; ++i) {
        std::vector<model::vertex_t> temp(from, to);
        result.push_back(temp);
        from = std::next(from, musthave);
        to = std::next(to, musthave);
    }

    return result;
}

std::vector<model::vertex_t> GetCandidates(model::graph_t const& graph, std::string const& label) {
    std::vector<model::vertex_t> result = {};

    BGL_FORALL_VERTICES_T(v, graph, model::graph_t) {
        if (graph[v].attributes.at("label") == label) {
            result.push_back(v);
        }
    }

    return result;
}

model::vertex_t GetCenter(model::graph_t const& pattern, int& radius) {
    using DistanceProperty = boost::exterior_vertex_property<model::graph_t, int>;
    using DistanceMatrix = typename DistanceProperty::matrix_type;
    using DistanceMatrixMap = typename DistanceProperty::matrix_map_type;

    using EccentricityProperty = boost::exterior_vertex_property<model::graph_t, int>;
    using EccentricityContainer = typename EccentricityProperty::container_type;
    using EccentricityMap = typename EccentricityProperty::map_type;

    DistanceMatrix distances(boost::num_vertices(pattern));
    DistanceMatrixMap dm(distances, pattern);

    using WeightMap = boost::constant_property_map<model::edge_t, int>;

    WeightMap wm(1);
    boost::floyd_warshall_all_pairs_shortest_paths(pattern, dm, weight_map(wm));

    int r, d;
    EccentricityContainer eccs(boost::num_vertices(pattern));
    EccentricityMap em(eccs, pattern);
    boost::tie(r, d) = all_eccentricities(pattern, dm, em);
    radius = r;

    model::vertex_t result = 0;
    typename boost::graph_traits<model::graph_t>::vertex_iterator i, end;
    for (boost::tie(i, end) = vertices(pattern); i != end; ++i) {
        bool is_center = true;
        typename boost::graph_traits<model::graph_t>::vertex_iterator j;
        for (j = vertices(pattern).first; j != end; ++j) {
            if (boost::get(boost::get(dm, *i), *j) > r) {
                is_center = false;
                break;
            }
        }
        if (is_center) {
            result = pattern[*i].node_id;
            break;
        }
    }
    return result;
}

void CalculateMessages(model::graph_t const& graph,
                       std::vector<gfd_validator::Request> const& requests,
                       std::map<int, std::vector<gfd_validator::Message>>& weighted_messages) {
    for (gfd_validator::Request const& request : requests) {
        int gfd_index = std::get<0>(request);
        model::vertex_t center = std::get<1>(request);
        int radius = std::get<2>(request);
        std::vector<model::vertex_t> candidates = std::get<3>(request);
        for (model::vertex_t const& candidate : candidates) {
            std::set<model::vertex_t> vertices = {candidate};
            std::set<model::vertex_t> current = {candidate};
            for (int i = 0; i < radius; ++i) {
                std::set<model::vertex_t> temp = {};
                for (auto& v : current) {
                    typename boost::graph_traits<model::graph_t>::adjacency_iterator adjacency_it,
                            adjacency_end;
                    boost::tie(adjacency_it, adjacency_end) = boost::adjacent_vertices(v, graph);
                    for (; adjacency_it != adjacency_end; ++adjacency_it) {
                        if (vertices.find(*adjacency_it) == vertices.end()) {
                            vertices.insert(*adjacency_it);
                            temp.insert(*adjacency_it);
                        }
                    }
                }
                current = temp;
            }
            int weight = vertices.size();
            for (auto& v : vertices) {
                for (auto& u : vertices) {
                    if (boost::edge(v, u, graph).second) {
                        weight++;
                    }
                }
            }
            gfd_validator::Message message(gfd_index, center, candidate);
            if (weighted_messages.find(weight) != weighted_messages.end()) {
                weighted_messages.at(weight).push_back(message);
            } else {
                std::vector<gfd_validator::Message> current = {message};
                weighted_messages.emplace(weight, current);
            }
        }
    }
}

class CheckCallback {
private:
    model::graph_t const& query_;
    model::graph_t const& graph_;
    std::vector<model::Gfd::Literal> const premises_;
    std::vector<model::Gfd::Literal> const conclusion_;
    bool& res_;

public:
    CheckCallback(model::graph_t const& query_, model::graph_t const& graph_,
                  std::vector<model::Gfd::Literal> const& premises_,
                  std::vector<model::Gfd::Literal> const& conclusion_, bool& res_)
        : query_(query_),
          graph_(graph_),
          premises_(premises_),
          conclusion_(conclusion_),
          res_(res_) {}

    template <typename CorrespondenceMap1To2, typename CorrespondenceMap2To1>
    bool operator()(CorrespondenceMap1To2 f, CorrespondenceMap2To1) const {
        auto satisfied = [this, &f](std::vector<model::Gfd::Literal> const literals) {
            for (model::Gfd::Literal const& l : literals) {
                auto fst_token = l.first;
                auto snd_token = l.second;
                std::string fst;
                std::string snd;
                if (fst_token.first == -1) {
                    fst = fst_token.second;
                } else {
                    model::vertex_t v;
                    model::vertex_t u = boost::vertex(fst_token.first, query_);
                    v = boost::get(f, u);
                    auto attrs = graph_[v].attributes;
                    if (attrs.find(fst_token.second) == attrs.end()) {
                        return false;
                    }
                    fst = attrs.at(fst_token.second);
                }
                if (snd_token.first == -1) {
                    snd = snd_token.second;
                } else {
                    model::vertex_t v;
                    model::vertex_t u = boost::vertex(snd_token.first, query_);
                    v = boost::get(f, u);
                    auto attrs = graph_[v].attributes;
                    if (attrs.find(snd_token.second) == attrs.end()) {
                        return false;
                    }
                    snd = attrs.at(snd_token.second);
                }
                if (fst != snd) {
                    return false;
                }
            }
            return true;
        };

        if (!satisfied(premises_)) {
            return true;
        }
        if (!satisfied(conclusion_)) {
            res_ = false;
            return false;
        }
        return true;
    }
};

struct VCompare {
    model::graph_t const& pattern;
    model::graph_t const& graph;
    model::vertex_t pinted_fr;
    model::vertex_t pinted_to;

    bool operator()(model::vertex_t fr, model::vertex_t to) const {
        if (fr == pinted_fr && to == pinted_to) {
            return true;
        }
        if (fr == pinted_fr || to == pinted_to) {
            return false;
        }
        return pattern[fr].attributes.at("label") == graph[to].attributes.at("label");
    }
};

struct ECompare {
    model::graph_t const& pattern;
    model::graph_t const& graph;

    bool operator()(model::edge_t fr, model::edge_t to) const {
        return pattern[fr].label == graph[to].label;
    }
};

void CalculateUnsatisfied(model::graph_t const& graph,
                          std::vector<gfd_validator::Message> const& messages,
                          std::map<int, model::Gfd> const& indexed_gfds,
                          std::set<int>& unsatisfied) {
    for (auto& message : messages) {
        int gfd_index = std::get<0>(message);
        if (unsatisfied.find(gfd_index) != unsatisfied.end()) {
            continue;
        }

        model::vertex_t u = std::get<1>(message);
        model::vertex_t v = std::get<2>(message);

        model::Gfd gfd = indexed_gfds.at(gfd_index);
        model::graph_t pattern = gfd.GetPattern();

        VCompare vcompare{pattern, graph, u, v};
        ECompare ecompare{pattern, graph};

        bool satisfied = true;
        CheckCallback callback(pattern, graph, gfd.GetPremises(), gfd.GetConclusion(), satisfied);

        boost::vf2_subgraph_iso(pattern, graph, callback, boost::get(boost::vertex_index, pattern),
                                boost::get(boost::vertex_index, graph),
                                vertex_order_by_mult(pattern), ecompare, vcompare);
        if (!satisfied) {
            unsatisfied.insert(gfd_index);
        }
    }
}

}  // namespace

namespace algos {

GfdValidator::GfdValidator() : GfdHandler() {
    RegisterOption(config::kThreadNumberOpt(&threads_num_));
    MakeOptionsAvailable({config::kThreadNumberOpt.GetName()});
};

std::vector<model::Gfd> GfdValidator::GenerateSatisfiedGfds(model::graph_t const& graph,
                                                            std::vector<model::Gfd> const& gfds) {
    std::vector<std::vector<gfd_validator::Request>> requests = {};
    for (int i = 0; i < threads_num_; ++i) {
        std::vector<gfd_validator::Request> empty = {};
        requests.push_back(empty);
    }

    std::map<int, model::Gfd> indexed_gfds;
    int index = 0;
    for (auto& gfd : gfds) {
        int radius = 0;
        model::vertex_t center = GetCenter(gfd.GetPattern(), radius);
        std::vector<model::vertex_t> candidates =
                GetCandidates(graph, gfd.GetPattern()[center].attributes.at("label"));
        auto partition = GetPartition(candidates, threads_num_);
        for (std::size_t i = 0; i < partition.size(); ++i) {
            if (!partition.at(i).empty()) {
                gfd_validator::Request request(index, center, radius, partition.at(i));
                requests[i].push_back(request);
            }
        }
        indexed_gfds.emplace(index, gfd);
        index++;
    }

    std::vector<std::map<int, std::vector<gfd_validator::Message>>> weighted_messages;
    for (int i = 0; i < threads_num_; ++i) {
        std::map<int, std::vector<gfd_validator::Message>> empty;
        weighted_messages.push_back(empty);
    }

    std::vector<std::thread> threads = {};
    for (int i = 0; i < threads_num_; ++i) {
        std::thread thrd(CalculateMessages, std::cref(graph), std::cref(requests.at(i)),
                         std::ref(weighted_messages.at(i)));
        threads.push_back(std::move(thrd));
    }
    for (std::thread& thrd : threads) {
        if (thrd.joinable()) {
            thrd.join();
        }
    }

    std::map<int, std::vector<gfd_validator::Message>> all_weighted_messages;
    for (int i = 0; i < threads_num_; ++i) {
        for (auto& kv : weighted_messages.at(i)) {
            all_weighted_messages.emplace(kv.first, kv.second);
        }
    }

    std::vector<int> weights = {};
    for (auto& kv : all_weighted_messages) {
        weights.push_back(kv.first);
    }

    // balance
    Balancer balancer;
    std::vector<std::vector<int>> balanced_weights = balancer.Balance(weights, threads_num_);
    std::vector<std::vector<gfd_validator::Message>> balanced_messages = {};
    for (std::size_t i = 0; i < balanced_weights.size(); ++i) {
        std::vector<gfd_validator::Message> messages = {};
        for (int const& weight : balanced_weights.at(i)) {
            gfd_validator::Message message = *all_weighted_messages.at(weight).begin();
            all_weighted_messages.at(weight).erase(all_weighted_messages.at(weight).begin());
            messages.push_back(message);
        }
        balanced_messages.push_back(messages);
    }

    std::vector<model::Gfd> result = {};
    std::vector<std::set<int>> unsatisfied = {};
    for (int i = 0; i < threads_num_; ++i) {
        std::set<int> empty = {};
        unsatisfied.push_back(empty);
    }

    LOG(DEBUG) << "Messages constructed. Matching...";
    // calculate unsatisfied forall processor (vf2)
    threads.clear();
    for (int i = 0; i < threads_num_; ++i) {
        std::thread thrd(CalculateUnsatisfied, std::cref(graph), std::cref(balanced_messages.at(i)),
                         std::cref(indexed_gfds), std::ref(unsatisfied.at(i)));
        threads.push_back(std::move(thrd));
    }
    for (std::thread& thrd : threads) {
        if (thrd.joinable()) {
            thrd.join();
        }
    }
    // concatmap unsatisfied
    std::set<int> all_unsatisfied = {};
    for (int i = 0; i < threads_num_; ++i) {
        for (auto& ind : unsatisfied.at(i)) {
            all_unsatisfied.insert(ind);
        }
    }

    for (std::size_t i = 0; i < indexed_gfds.size(); ++i) {
        if (all_unsatisfied.find(i) == all_unsatisfied.end()) {
            result.push_back(indexed_gfds.at(i));
        }
    }
    return result;
}

}  // namespace algos
