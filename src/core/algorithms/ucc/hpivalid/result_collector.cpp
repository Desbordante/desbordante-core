#include "core/algorithms/ucc/hpivalid/result_collector.h"

#include <chrono>
#include <sstream>
#include <string>
#include <vector>

#include "core/util/logger.h"

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
      diff_sets_(0),
      diff_sets_initial_(0),
      tree_complexity_(0),
      tree_nodes_(0),
      intersections_(0),
      intersection_cluster_size_(0) {}

void ResultCollector::AddUCC(Edge const& ucc) {
    ucc_count_++;
    ucc_vector_.push_back(ucc);
}

bool ResultCollector::TimedOut() {
    return std::chrono::duration_cast<std::chrono::duration<double>>(
                   std::chrono::high_resolution_clock::now() - exec_start_)
                   .count() > timeout_;
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

void ResultCollector::SetStartTime() {
    exec_start_ = std::chrono::high_resolution_clock::now();
}

unsigned long long ResultCollector::GetTimeSinceStart() const {
    return std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::high_resolution_clock::now() - exec_start_)
            .count();
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
