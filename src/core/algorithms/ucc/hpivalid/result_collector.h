#pragma once

#include <chrono>
#include <string>
#include <vector>

#include "core/algorithms/ucc/hpivalid/enums.h"
#include "core/algorithms/ucc/hpivalid/hypergraph.h"
#include "core/algorithms/ucc/raw_ucc.h"
#include "core/algorithms/ucc/ucc_algorithm.h"
#include "core/model/table/column_layout_relation_data.h"

// see algorithms/ucc/hpivalid/LICENSE

namespace algos::hpiv {

namespace timer {

using clock = std::chrono::high_resolution_clock;

extern std::vector<std::string> description;

struct TimerInfo {
    clock::time_point begin;
    clock::time_point end;
    unsigned long long elapsed;
};

}  // namespace timer

class ResultCollector {
    using clock = std::chrono::high_resolution_clock;

private:
    double timeout_;
    unsigned ucc_count_;
    unsigned diff_sets_final_;

    std::vector<timer::TimerInfo> timers_;

    unsigned diff_sets_;
    unsigned diff_sets_initial_;
    unsigned tree_complexity_;
    unsigned tree_nodes_;
    unsigned intersections_;
    unsigned intersection_cluster_size_;

    std::vector<model::RawUCC> ucc_vector_;

public:
    explicit ResultCollector(double timeout);

    //////////////////////////////////////////////////////////////////////////////
    // collecting information

    // Report that a UCC has been found.  The return value is false if
    // the timeout is reached.
    bool UCCFound(Edge const& ucc);

    // Report the final hypergraph of difference sets.
    void FinalHypergraph(Hypergraph const& hg);

    // Start a timer (one of `timer::name`).
    void StartTimer(timer::TimerName timer);

    // Stop a timer (one of `timer::name`).
    void StopTimer(timer::TimerName timer);

    // Counts the `number` of difference sets being sampled.
    void CountDiffSets(unsigned number);

    // Notify that the end of the initial sampling is reached.
    void StopInitialSampling();

    // Counts the tree complexity in terms of `number` of candidate
    // edges that are considered to branch on.
    void CountTreeComplexity(unsigned number);

    // Count the number of tree nodes.
    void CountTreeNode();

    // Count the number of times we intersect clusters.
    void CountIntersections();

    // Count the total cluster size in intersections.
    void CountIntersectionClusterSize(unsigned cluster_size);

    //////////////////////////////////////////////////////////////////////////////
    // getting statistics

    // Number of found UCCs.
    unsigned UCCs() const {
        return ucc_count_;
    }

    // Time in seconds between starting and stopping `timer`.
    unsigned long long Time(timer::TimerName timer) const;

    // Total number of sampled difference sets.
    unsigned DiffSets() const {
        return diff_sets_;
    }

    // Number of difference set in the initial sampling.
    unsigned DiffSetsInitial() const {
        return diff_sets_initial_;
    }

    // Number of difference sets in the final hypergraph.
    unsigned DiffSetsFinal() const {
        return diff_sets_final_;
    }

    // Three complexity in terms of the number of candidate edges
    // considered to branch on.
    unsigned TreeComplexity() const {
        return tree_complexity_;
    }

    // Size of the search tree.
    unsigned TreeNodes() const {
        return tree_nodes_;
    }

    // Number of cluster intersections.
    unsigned Intersections() const {
        return intersections_;
    }

    // Total cluster size in intersections.
    unsigned IntersectionClusterSize() const {
        return intersection_cluster_size_;
    }

    std::vector<model::RawUCC> const& GetUCCs() const {
        return ucc_vector_;
    }
};

}  // namespace algos::hpiv
