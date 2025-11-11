#include "algorithms/ucc/hpivalid/result_collector.h"

#include <chrono>
#include <sstream>
#include <string>
#include <vector>

#include "util/logger.h"

// see algorithms/ucc/hpivalid/LICENSE

namespace algos::hpiv {

namespace timer {
std::vector<std::string> description = {"Total Execution Time", "Cluster-Structures Construction",
                                        "Total Enumeration Algo Time"
                                        "Difference Set Sampling (all)",
                                        "Cluster Intersection (validation)"};
}  // namespace timer

ResultCollector::ResultCollector(double timeout)
    : timeout_(timeout),
      ucc_count_(0),
      diff_sets_final_(0),
      timers_(timer::TimerName::num_of_timers),
      diff_sets_(0),
      diff_sets_initial_(0),
      tree_complexity_(0),
      tree_nodes_(0),
      intersections_(0),
      intersection_cluster_size_(0) {}

bool ResultCollector::UCCFound(Edge const& ucc) {
    ucc_count_++;
    ucc_vector_.push_back(ucc);
    return std::chrono::duration_cast<std::chrono::duration<double>>(
                   clock::now() - timers_[timer::TimerName::total].begin)
                   .count() <= timeout_;
}

void ResultCollector::FinalHypergraph(Hypergraph const& hg) {
    std::stringstream out;
    for (Edge const& e : hg) {
        for (Edge::size_type v = e.find_first(); v != Edge::npos; v = e.find_next(v)) {
            if (v != e.find_first()) {
                out << ",";
            }
            out << v;
        }
        out << "\n";
    }
    LOG_DEBUG("{}", out.str());

    diff_sets_final_ = hg.NumEdges();
}

void ResultCollector::StartTimer(timer::TimerName timer) {
    timers_[timer].begin = clock::now();
}

void ResultCollector::StopTimer(timer::TimerName timer) {
    timers_[timer].end = clock::now();
    timers_[timer].elapsed += std::chrono::duration_cast<std::chrono::milliseconds>(
                                      timers_[timer].end - timers_[timer].begin)
                                      .count();
}

unsigned long long ResultCollector::Time(timer::TimerName timer) const {
    return timers_[timer].elapsed;
}

void ResultCollector::CountDiffSets(unsigned number) {
    diff_sets_ += number;
}

void ResultCollector::StopInitialSampling() {
    diff_sets_initial_ = diff_sets_;
}

void ResultCollector::CountTreeComplexity(unsigned number) {
    tree_complexity_ += number;
}

void ResultCollector::CountTreeNode() {
    tree_nodes_++;
}

void ResultCollector::CountIntersections() {
    intersections_++;
}

void ResultCollector::CountIntersectionClusterSize(unsigned cluster_size) {
    intersection_cluster_size_ += cluster_size;
}

}  // namespace algos::hpiv
