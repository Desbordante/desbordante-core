#pragma once

#include <deque>
#include <random>
#include <stack>
#include <vector>

#include "core/algorithms/ucc/hpivalid/hypergraph.h"
#include "core/model/table/position_list_index.h"

// see algorithms/ucc/hpivalid/LICENSE

namespace algos::hpiv {

struct Config;
struct PLITable;
class ResultCollector;

class TreeSearch {
private:
    PLITable const& tab_;
    Config const& cfg_;
    ResultCollector& rc_;

    // the partial hypergraph of difference sets
    Hypergraph partial_hg_;

    // exception to throw, when timeout happens
    unsigned const timeout_ = 10;

    // a mapping from clusterid to record indices that is used for the
    // intersection of PLIs with single-column PLIs
    std::vector<model::PLI::Cluster> clusterid_to_recordindices_;

    // mapping from column to niceness (in [0, nr_cols)) with smaller
    // values being nicer columns
    std::vector<unsigned long> niceness_;
    void ComputeNiceness();
    unsigned long Niceness(Edge const& e) const;

    std::default_random_engine gen_;
    Hypergraph Sample(std::deque<model::PLI::Cluster> const& pli);

    inline void UpdateCritAndUncov(std::vector<std::vector<Edgemark>>& removed_criticals_stack,
                                   std::vector<Edgemark>& crit, Edgemark& uncov,
                                   Edgemark const& v_hittings) const;
    inline void RestoreCritAndUncov(std::vector<std::vector<Edgemark>>& removed_criticals_stack,
                                    std::vector<Edgemark>& crit, Edgemark& uncov) const;

    inline bool ExtendOrConfirmS(Edge& s, Edge& cand, std::vector<Edgemark>& crit, Edgemark& uncov,
                                 std::vector<Edgemark>& vertexhittings,
                                 std::vector<std::vector<Edgemark>>& removed_criticals_stack,
                                 std::stack<std::deque<model::PLI::Cluster>>& intersection_stack,
                                 std::deque<Edge::size_type>& tointersect_queue);

    inline void PullUpIntersections(std::stack<std::deque<model::PLI::Cluster>>& intersection_stack,
                                    std::deque<Edge::size_type>& tointersect_queue);

    std::deque<model::PLI::Cluster> IntersectClusterListAndClusterMapping(
            std::deque<model::PLI::Cluster> const& pli,
            std::vector<unsigned> const& inverse_mapping);

    inline void UpdateEdges(std::vector<Edgemark>& crit, Edgemark& uncov,
                            std::vector<Edgemark>& vertexhittings,
                            std::vector<std::vector<Edgemark>>& removed_criticals_stack,
                            std::deque<model::PLI::Cluster> const& pli);

    inline bool SFulfillsMinimalityCondition(std::vector<Edgemark> const& crit) const;

    inline bool IsViolater(std::vector<Edgemark> const& crit, Edgemark const& v_hittings) const;

public:
    TreeSearch(PLITable const& tab, Config const& cfg, ResultCollector& rc);

    void Run();
};

}  // namespace algos::hpiv
