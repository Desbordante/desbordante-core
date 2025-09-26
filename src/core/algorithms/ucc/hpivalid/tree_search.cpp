#include "algorithms/ucc/hpivalid/tree_search.h"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <limits>
#include <random>
#include <stack>
#include <tuple>
#include <utility>
#include <vector>

#include <easylogging++.h>

#include "algorithms/ucc/hpivalid/config.h"
#include "algorithms/ucc/hpivalid/pli_table.h"
#include "algorithms/ucc/hpivalid/result_collector.h"
#include "util/set_bits_view.h"

// see algorithms/ucc/hpivalid/LICENSE

namespace algos::hpiv {

TreeSearch::TreeSearch(PLITable const& tab, Config const& cfg, ResultCollector& rc)
    : tab_(tab),
      cfg_(cfg),
      rc_(rc),
      partial_hg_(tab.nr_cols),
      clusterid_to_recordindices_(tab.nr_rows),
      gen_(cfg.seed) {
    // add single edge containing all vertices to partial hypergraph
    partial_hg_.AddEdge(~Edge(partial_hg_.NumVertices()));

    if (cfg_.tiebreaker_heuristic) {
        ComputeNiceness();
    }
}

void TreeSearch::Run() {
    rc_.StartTimer(timer::TimerName::sample_diff_sets);
    for (auto const& pli : tab_.plis) {
        Hypergraph gen = Sample(pli);
        for (Edge const& e : gen) {
            partial_hg_.AddEdgeAndMinimizeInclusion(e);
        }
    }
    rc_.StopInitialSampling();
    rc_.StopTimer(timer::TimerName::sample_diff_sets);

    // S, CAND
    Edge s(partial_hg_.NumVertices());
    Edge cand(partial_hg_.NumVertices());
    cand.set();

    // crit, uncov
    std::vector<Edgemark> crit;
    Edgemark uncov(partial_hg_.NumEdges());
    uncov.set();

    // vertexhittings
    std::vector<Edgemark> vertexhittings(partial_hg_.NumVertices(),
                                         Edgemark(partial_hg_.NumEdges()));
    for (std::vector<Edge>::size_type i_e = 0; i_e < partial_hg_.NumEdges(); ++i_e) {
        for (Edge::size_type i_v : util::SetBits(partial_hg_[i_e])) {
            vertexhittings[i_v].set(i_e);
        }
    }

    std::vector<std::vector<Edgemark>> removed_criticals_stack;

    // intersections
    std::stack<std::deque<model::PLI::Cluster>> intersection_stack;
    std::deque<Edge::size_type> tointersect_queue;

    // Searching
    // find edge from uncov with smallest intersecton C with CAND
    Edge c = partial_hg_[uncov.find_first()] & cand;
    for (Edge::size_type i_e = uncov.find_next(uncov.find_first()); i_e != Edge::npos;
         i_e = uncov.find_next(i_e)) {
        if ((partial_hg_[i_e] & cand).count() < c.count()) {
            c = partial_hg_[i_e] & cand;
        }
    }

    cand -= c;

    try {
        for (Edge::size_type v : util::SetBits(c)) {
            // update crit and uncov
            UpdateCritAndUncov(removed_criticals_stack, crit, uncov, vertexhittings[v]);

            // branch
            s.set(v);
            intersection_stack.push(tab_.plis[v]);
            ExtendOrConfirmS(s, cand, crit, uncov, vertexhittings, removed_criticals_stack,
                             intersection_stack, tointersect_queue);
            intersection_stack.pop();
            s.reset(v);

            // reset update of crit and uncov
            RestoreCritAndUncov(removed_criticals_stack, crit, uncov);

            // update CAND
            cand.set(v);
        }
        // report final hypergraph
        LOG(DEBUG) << "Final hypergraph:";
        rc_.FinalHypergraph(partial_hg_);
    } catch (unsigned timeout) {
        // report current partial hypergraph
        LOG(DEBUG) << "Current partial hypergraph:";
        rc_.FinalHypergraph(partial_hg_);
    }
}

void TreeSearch::ComputeNiceness() {
    std::vector<std::pair<unsigned long, unsigned long>> sq_sizes_col_id_pairs(tab_.nr_cols);

    for (model::ColumnIndex col = 0; col < tab_.nr_cols; ++col) {
        unsigned long sq_size = 0;
        for (auto const& cluster : tab_.plis[col]) {
            sq_size += cluster.size() * cluster.size();
        }
        sq_sizes_col_id_pairs[col] = std::make_pair(sq_size, col);
    }

    std::sort(sq_sizes_col_id_pairs.begin(), sq_sizes_col_id_pairs.end());
    niceness_.clear();
    niceness_.resize(tab_.nr_cols);
    for (std::size_t pos = 0; pos < tab_.nr_cols; ++pos) {
        unsigned long const col = sq_sizes_col_id_pairs[pos].second;
        niceness_[col] = pos;
    }
}

unsigned long TreeSearch::Niceness(Edge const& e) const {
    unsigned long niceness = 0;
    if (cfg_.tiebreaker_heuristic) {
        for (std::size_t col : util::SetBits(e)) {
            if (niceness < niceness_[col]) {
                niceness = niceness_[col];
            }
        }
    }
    return niceness;
}

Hypergraph TreeSearch::Sample(std::deque<model::PLI::Cluster> const& pli) {
    Hypergraph difference_graph(tab_.nr_cols);
    Edge temp_edge(tab_.nr_cols);

    if (pli.size() == 0) {
        return difference_graph;
    }

    std::vector<unsigned long> weights;
    unsigned long total_pairs = 0;
    for (auto const& cluster : pli) {
        unsigned long const pairs = (cluster.size() * (cluster.size() - 1)) / 2;
        total_pairs += pairs;
        weights.push_back(pairs);
    }

    // calculate how many to view
    std::size_t to_view = static_cast<std::size_t>(
            round(std::pow(static_cast<double>(total_pairs), cfg_.sample_exponent)));
    if (to_view < 1) {
        to_view = 1;
    }

    rc_.CountDiffSets(to_view);

    std::discrete_distribution<int> rand_cluster(weights.begin(), weights.end());
    std::uniform_int_distribution<> rand_int(0, std::numeric_limits<int>::max());
    std::vector<std::tuple<int, int, int>> samples(to_view);
    for (std::size_t i = 0; i < to_view; ++i) {
        std::get<0>(samples[i]) = rand_cluster(gen_);
        unsigned size = pli[std::get<0>(samples[i])].size();
        int i_i_r1 = rand_int(gen_) % size;
        int i_i_r2 = (i_i_r1 + 1 + rand_int(gen_) % (size - 1)) % size;
        std::get<1>(samples[i]) = i_i_r1;
        std::get<2>(samples[i]) = i_i_r2;
    }
    std::sort(samples.begin(), samples.end());
    for (std::size_t i = 0; i < to_view; ++i) {
        auto const& cluster = pli[std::get<0>(samples[i])];
        int i_i_r1 = std::get<1>(samples[i]);
        int i_i_r2 = std::get<2>(samples[i]);

        temp_edge.reset();

        // calculate difference edge
        for (model::ColumnIndex i_c = 0; i_c < tab_.nr_cols; ++i_c) {
            // set bit if records have different cluster id's or if they
            // have maxint as id (indicating that they are unique)
            if (tab_.inverse_mapping[i_c][cluster[i_i_r1]] !=
                        tab_.inverse_mapping[i_c][cluster[i_i_r2]] ||
                tab_.inverse_mapping[i_c][cluster[i_i_r1]] == kSizeOneCluster) {
                temp_edge.set(i_c);
            }
        }
        // add difference edge to difference graph
        difference_graph.AddEdgeAndMinimizeInclusion(temp_edge);
    }

    return difference_graph;
}

inline void TreeSearch::UpdateCritAndUncov(
        std::vector<std::vector<Edgemark>>& removed_criticals_stack, std::vector<Edgemark>& crit,
        Edgemark& uncov, Edgemark const& v_hittings) const {
    // update crit[] for vertices in S and put changes on stack

    removed_criticals_stack.emplace_back(crit.size());

    for (std::vector<Edgemark>::size_type i = 0; i < crit.size(); ++i) {
        removed_criticals_stack.back()[i] = crit[i] & v_hittings;
        crit[i] -= v_hittings;
    }

    // set critical edges for v and remove them from uncov

    crit.emplace_back(v_hittings & uncov);
    uncov -= v_hittings;
}

inline void TreeSearch::RestoreCritAndUncov(
        std::vector<std::vector<Edgemark>>& removed_criticals_stack, std::vector<Edgemark>& crit,
        Edgemark& uncov) const {
    uncov |= crit.back();
    crit.pop_back();

    for (std::vector<Edgemark>::size_type i = 0; i < crit.size(); ++i) {
        crit[i] |= removed_criticals_stack.back()[i];
    }

    removed_criticals_stack.pop_back();
}

inline bool TreeSearch::ExtendOrConfirmS(
        Edge& s, Edge& cand, std::vector<Edgemark>& crit, Edgemark& uncov,
        std::vector<Edgemark>& vertexhittings,
        std::vector<std::vector<Edgemark>>& removed_criticals_stack,
        std::stack<std::deque<model::PLI::Cluster>>& intersection_stack,
        std::deque<Edge::size_type>& tointersect_queue) {
    rc_.CountTreeNode();
    if (uncov.none()) {
        PullUpIntersections(intersection_stack, tointersect_queue);

        if (intersection_stack.top().empty()) {
            if (!rc_.UCCFound(s)) {
                // timeout
                throw timeout_;
            }
            return false;
        }

        // gain new edges and minimize
        UpdateEdges(crit, uncov, vertexhittings, removed_criticals_stack, intersection_stack.top());

        // check if minimality still holds
        if (!SFulfillsMinimalityCondition(crit)) {
            return true;
        }
    }

    // find edge from uncov with smallest intersecton C with CAND
    rc_.CountTreeComplexity(uncov.count());
    Edge c = partial_hg_[uncov.find_first()] & cand;
    for (Edge::size_type i_e = uncov.find_next(uncov.find_first()); i_e != Edge::npos;
         i_e = uncov.find_next(i_e)) {
        Edge c_new = (partial_hg_[i_e] & cand);
        if (c_new.count() < c.count() ||
            (c_new.count() == c.count() && Niceness(c_new) < Niceness(c))) {
            c = std::move(c_new);
        }
    }

    cand -= c;

    for (Edge::size_type v : util::SetBits(c)) {
        // don't branch if v is violater for S
        if (IsViolater(crit, vertexhittings[v])) {
            continue;
        }

        // branch
        UpdateCritAndUncov(removed_criticals_stack, crit, uncov, vertexhittings[v]);

        s.set(v);
        tointersect_queue.push_back(v);
        bool check = ExtendOrConfirmS(s, cand, crit, uncov, vertexhittings, removed_criticals_stack,
                                      intersection_stack, tointersect_queue);
        if (tointersect_queue.empty()) {
            intersection_stack.pop();
        } else {
            tointersect_queue.pop_back();
        }
        s.reset(v);
        RestoreCritAndUncov(removed_criticals_stack, crit, uncov);

        // prove if deeper update of edges destroyed minimality condition
        if (check && !SFulfillsMinimalityCondition(crit)) {
            // add C now to CAND; this also adds the violaters to CAND again and it is
            // equal to the CAND passed in
            cand |= c;

            return true;
        }

        // update CAND
        cand.set(v);
    }

    // add C now to CAND; this also adds the violaters to CAND again and it is
    // equal to the CAND passed in
    cand |= c;

    return false;
}

inline void TreeSearch::PullUpIntersections(
        std::stack<std::deque<model::PLI::Cluster>>& intersection_stack,
        std::deque<Edge::size_type>& tointersect_queue) {
    rc_.StartTimer(timer::TimerName::cluster_intersect);
    while (!tointersect_queue.empty()) {
        intersection_stack.push(IntersectClusterListAndClusterMapping(
                intersection_stack.top(), tab_.inverse_mapping[tointersect_queue.front()]));

        tointersect_queue.pop_front();
    }
    rc_.StopTimer(timer::TimerName::cluster_intersect);
}

std::deque<model::PLI::Cluster> TreeSearch::IntersectClusterListAndClusterMapping(
        std::deque<model::PLI::Cluster> const& pli, std::vector<unsigned> const& inverse_mapping) {
    rc_.CountIntersections();
    std::deque<model::PLI::Cluster> intersection;

    std::vector<unsigned long> clusterids;
    for (auto const& cluster : pli) {
        rc_.CountIntersectionClusterSize(cluster.size());
        clusterids.clear();
        for (std::vector<unsigned>::size_type i_r : cluster) {
            if (inverse_mapping[i_r] != kSizeOneCluster) {
                auto& map_entry = clusterid_to_recordindices_[inverse_mapping[i_r]];
                if (map_entry.size() == 0) {
                    clusterids.push_back(inverse_mapping[i_r]);
                }
                map_entry.push_back(i_r);
            }
        }
        for (auto clusterid : clusterids) {
            auto& map_entry = clusterid_to_recordindices_[clusterid];
            if (map_entry.size() != 1) {
                intersection.emplace_back(std::move(map_entry));
            }
            clusterid_to_recordindices_[clusterid] = {};
        }
    }

    return intersection;
}

inline void TreeSearch::UpdateEdges(std::vector<Edgemark>& crit, Edgemark& uncov,
                                    std::vector<Edgemark>& vertexhittings,
                                    std::vector<std::vector<Edgemark>>& removed_criticals_stack,
                                    std::deque<model::PLI::Cluster> const& pli) {
    // sample new edges
    rc_.StartTimer(timer::TimerName::sample_diff_sets);
    Hypergraph new_edges = Sample(pli);
    rc_.StopTimer(timer::TimerName::sample_diff_sets);

    // find out which edges are supersets and therefore can be removed and save
    // indices in descending order
    std::vector<std::vector<Edge>::size_type> supsets_indices;
    for (std::vector<Edge>::size_type i_e = partial_hg_.NumEdges(); i_e > 0;
         /* gets decreased below */) {
        --i_e;

        for (Edge const& new_edge : new_edges) {
            if (new_edge.is_subset_of(partial_hg_[i_e])) {
                supsets_indices.push_back(i_e);
                break;
            }
        }
    }

    // remove these edges from difference_graph, vertexhittings, uncov, crit,
    // removed_criticals

    for (std::vector<Edge>::size_type i_e : supsets_indices) {
        // difference_graph
        partial_hg_[i_e] = partial_hg_[partial_hg_.NumEdges() - 1];
        partial_hg_.RemoveLastEdge();

        // vertexhittings
        for (Edgemark& hittings : vertexhittings) {
            hittings[i_e] = hittings[hittings.size() - 1];
            hittings.pop_back();
        }

        // uncov
        uncov[i_e] = uncov[uncov.size() - 1];
        uncov.pop_back();

        // crit
        for (Edgemark& em_crit : crit) {
            em_crit[i_e] = em_crit[em_crit.size() - 1];
            em_crit.pop_back();
        }

        // removed_criticals
        for (auto& removed_criticals : removed_criticals_stack) {
            for (auto& removed : removed_criticals) {
                removed[i_e] = removed[removed.size() - 1];
                removed.pop_back();
            }
        }
    }

    // insert the new edges in difference_graph, vertexhittings, uncov, crit,
    // removed_criticals

    // difference graph
    for (Edge const& e : new_edges) {
        partial_hg_.AddEdge(e);
    }

    // vertexhittings
    for (Edge::size_type i_v = 0; i_v < partial_hg_.NumVertices(); ++i_v) {
        vertexhittings[i_v].resize(partial_hg_.NumEdges());
    }
    for (std::size_t i_e = partial_hg_.NumEdges() - new_edges.NumEdges();
         i_e < partial_hg_.NumEdges(); ++i_e) {
        for (Edge::size_type i_v : util::SetBits(partial_hg_[i_e])) {
            vertexhittings[i_v].set(i_e);
        }
    }

    // uncov
    uncov.resize(partial_hg_.NumEdges(), true);

    // crit
    for (Edgemark& em : crit) {
        em.resize(partial_hg_.NumEdges());
    }

    // removed_criticals
    for (auto& removed_criticals : removed_criticals_stack) {
        for (auto& removed : removed_criticals) {
            removed.resize(partial_hg_.NumEdges());
        }
    }
}

inline bool TreeSearch::SFulfillsMinimalityCondition(std::vector<Edgemark> const& crit) const {
    bool fulfill = true;
    for (Edgemark const& em : crit) {
        if (em.none()) {
            fulfill = false;
            break;
        }
    }

    return fulfill;
}

inline bool TreeSearch::IsViolater(std::vector<Edgemark> const& crit,
                                   Edgemark const& v_hittings) const {
    bool is_violater = false;
    for (Edgemark const& em : crit) {
        if (em.is_subset_of(v_hittings)) {
            is_violater = true;
            break;
        }
    }

    return is_violater;
}

}  // namespace algos::hpiv
