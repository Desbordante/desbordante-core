#include "algorithms/gfd/gfd_validator/naivegfd_validator.h"

#include <string>
#include <unordered_map>
#include <utility>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/vf2_sub_graph_iso.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/tuple/tuple.hpp>
#include <easylogging++.h>

#include "algorithms/gfd/gfd.h"

namespace {

class CheckCallback {
private:
    model::graph_t const& query_;
    model::graph_t const& graph_;
    std::vector<model::Gfd::Literal> const premises_;
    std::vector<model::Gfd::Literal> const conclusion_;
    bool& res_;
    int& amount_;

public:
    CheckCallback(model::graph_t const& query_, model::graph_t const& graph_,
                  std::vector<model::Gfd::Literal> const& premises_,
                  std::vector<model::Gfd::Literal> const& conclusion_, bool& res_, int& amount_)
        : query_(query_),
          graph_(graph_),
          premises_(premises_),
          conclusion_(conclusion_),
          res_(res_),
          amount_(amount_) {}

    template <typename CorrespondenceMap1To2, typename CorrespondenceMap2To1>
    bool operator()(CorrespondenceMap1To2 f, CorrespondenceMap2To1) const {
        amount_++;
        auto satisfied = [this, &f](std::vector<model::Gfd::Literal> const& literals) {
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
                    model::vertex_t u = boost::vertex(fst_token.first, query_);
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

bool Validate(model::graph_t const& graph, model::Gfd const& gfd) {
    model::graph_t pattern = gfd.GetPattern();

    struct VCompare {
        model::graph_t const& pattern;
        model::graph_t const& graph;

        bool operator()(model::vertex_t fr, model::vertex_t to) const {
            return pattern[fr].attributes.at("label") == graph[to].attributes.at("label");
        }
    } vcompare{pattern, graph};

    struct ECompare {
        model::graph_t const& pattern;
        model::graph_t const& graph;

        bool operator()(model::edge_t fr, model::edge_t to) const {
            return pattern[fr].label == graph[to].label;
        }
    } ecompare{pattern, graph};

    bool res = true;
    int amount = 0;
    CheckCallback callback(pattern, graph, gfd.GetPremises(), gfd.GetConclusion(), res, amount);

    bool found = boost::vf2_subgraph_iso(pattern, graph, callback,
                                         boost::get(boost::vertex_index, pattern),
                                         boost::get(boost::vertex_index, graph),
                                         vertex_order_by_mult(pattern), ecompare, vcompare);
    LOG(DEBUG) << "Checked embeddings: " << amount;
    if (!found) {
        return true;
    }
    return res;
}

}  // namespace

namespace algos {

std::vector<model::Gfd> NaiveGfdValidator::GenerateSatisfiedGfds(
        model::graph_t const& graph, std::vector<model::Gfd> const& gfds) {
    for (auto& gfd : gfds) {
        if (Validate(graph, gfd)) {
            result_.push_back(gfd);
        }
    }
    return result_;
}

}  // namespace algos
