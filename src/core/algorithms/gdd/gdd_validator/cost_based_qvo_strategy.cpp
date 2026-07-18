#include "core/util/logger.h"
#include "qvo_strategies.h"

namespace algos {

void CostBasedQvoStrategy::ComputeAvgListSizes() {
    struct Acc {
        std::uint64_t sum = 0;
        std::size_t count = 0;
    };

    std::unordered_map<std::string, Acc> out_acc;
    std::unordered_map<std::string, Acc> in_acc;

    for (auto const gv : boost::make_iterator_range(boost::vertices(graph_))) {
        std::unordered_map<std::string, std::size_t> out_by_label;
        std::unordered_map<std::string, std::size_t> in_by_label;

        for (auto const edge : boost::make_iterator_range(boost::out_edges(gv, graph_))) {
            ++out_by_label[graph_[edge].label];
        }
        for (auto const edge : boost::make_iterator_range(boost::in_edges(gv, graph_))) {
            ++in_by_label[graph_[edge].label];
        }

        for (auto const& [label, n] : out_by_label) {
            auto& [sum, count] = out_acc[label];
            sum += n;
            ++count;
        }
        for (auto const& [label, n] : in_by_label) {
            auto& [sum, count] = in_acc[label];
            sum += n;
            ++count;
        }
    }

    for (auto const& [label, a] : out_acc) {
        avg_out_size_[label] = a.count ? a.sum / static_cast<double>(a.count) : 1.0;
    }
    for (auto const& [label, a] : in_acc) {
        avg_in_size_[label] = a.count ? a.sum / static_cast<double>(a.count) : 1.0;
    }
}

std::size_t CostBasedQvoStrategy::VertexDomainSize(VertexT pattern_vertex) const {
    auto const it = domain_.find(pattern_vertex);
    return it == domain_.end() ? 0 : it->second.size();
}

double CostBasedQvoStrategy::AvgListSize(std::string const& edge_label, Direction direction) const {
    auto const& table = direction == Direction::kIn ? avg_in_size_ : avg_out_size_;
    auto const it = table.find(edge_label);
    return it == table.end() ? 1.0 : it->second;
}

bool CostBasedQvoStrategy::ConnectsToPlaced(
        VertexT vertex, std::unordered_map<VertexT, std::size_t> const& placed) const {
    for (auto const edge : boost::make_iterator_range(boost::out_edges(vertex, pattern_))) {
        if (placed.contains(boost::target(edge, pattern_))) {
            return true;
        }
    }
    for (auto const edge : boost::make_iterator_range(boost::in_edges(vertex, pattern_))) {
        if (placed.contains(boost::source(edge, pattern_))) {
            return true;
        }
    }
    return false;
}

std::vector<CostBasedQvoStrategy::ExtensionListDescriptor> CostBasedQvoStrategy::ExtensionListsFor(
        VertexT to, std::unordered_map<VertexT, std::size_t> const& placed) const {
    std::vector<ExtensionListDescriptor> lists;

    for (auto const edge : boost::make_iterator_range(boost::in_edges(to, pattern_))) {
        if (VertexT const from = boost::source(edge, pattern_); placed.contains(from)) {
            lists.push_back(ExtensionListDescriptor{
                    .from = from,
                    .direction = Direction::kOut,
                    .edge_label = pattern_[edge].label,
            });
        }
    }

    for (auto const edge : boost::make_iterator_range(boost::out_edges(to, pattern_))) {
        if (VertexT const from = boost::target(edge, pattern_); placed.contains(from)) {
            lists.push_back(ExtensionListDescriptor{
                    .from = from,
                    .direction = Direction::kIn,
                    .edge_label = pattern_[edge].label,
            });
        }
    }
    return lists;
}

double CostBasedQvoStrategy::StepCost(VertexT next, std::size_t level,
                                      std::unordered_map<VertexT, std::size_t> const& placed,
                                      std::vector<double> const& card_by_level,
                                      double& match_estimate) const {
    std::vector<ExtensionListDescriptor> const& lists = ExtensionListsFor(next, placed);

    double const domain_size = static_cast<double>(VertexDomainSize(next));

    if (lists.empty()) {
        match_estimate *= std::max(1.0, domain_size);
        return 0.0;
    }

    double min_list = std::numeric_limits<double>::infinity();

    std::size_t earliest_pos = level;  // nothing earlier than level
    for (auto const& [from, direction, edge_label] : lists) {
        earliest_pos = std::min(earliest_pos, placed.at(from));
    }
    bool const cache_usable = lists.size() >= 2 && earliest_pos < level - 1;

    double const cache_card = cache_usable && earliest_pos < card_by_level.size()
                                      ? card_by_level[earliest_pos]
                                      : match_estimate;

    double cost = 0.0;
    for (auto const& [from, direction, edge_label] : lists) {
        double const list_size = AvgListSize(edge_label, direction);
        min_list = std::min(min_list, list_size);

        std::size_t const pos = placed.at(from);
        bool const list_cached = cache_usable && pos <= earliest_pos;
        double const charge_card = list_cached ? cache_card : match_estimate;
        cost += charge_card * list_size;
    }

    double const label_fraction =
            domain_size > 0.0 ? std::min(1.0, domain_size / std::max(1.0, min_list)) : 0.0;
    double const mu_extension = (min_list == std::numeric_limits<double>::infinity())
                                        ? domain_size
                                        : min_list * label_fraction;

    match_estimate *= std::max(std::numeric_limits<double>::epsilon(), mu_extension);

    return cost;
}

std::vector<CostBasedQvoStrategy::VertexT> CostBasedQvoStrategy::OrderExhaustive() const {
    std::vector pattern_vertices(boost::vertices(pattern_).first, boost::vertices(pattern_).second);
    std::ranges::sort(pattern_vertices, pattern_vertices_comparator_);

    std::vector<VertexT> best;
    double best_cost = std::numeric_limits<double>::infinity();

    do {
        std::unordered_map<VertexT, std::size_t> placed;
        std::vector<double> card_by_level;
        card_by_level.reserve(pattern_vertices.size());

        double cost = 0.0;
        double match_estimate = 0.0;
        bool valid = true;

        for (std::size_t level = 0; level < pattern_vertices.size(); ++level) {
            VertexT const pv = pattern_vertices[level];

            if (level == 0) {
                match_estimate = static_cast<double>(VertexDomainSize(pv));
            } else if (!ConnectsToPlaced(pv, placed)) {
                valid = false;  // disconnected prefix -> not a WCO plan
                break;
            } else {
                cost += StepCost(pv, level, placed, card_by_level, match_estimate);
            }

            placed.emplace(pv, level);
            card_by_level.push_back(match_estimate);
        }

        if (valid && cost < best_cost) {
            best_cost = cost;
            best = pattern_vertices;
        }
    } while (std::ranges::next_permutation(pattern_vertices, pattern_vertices_comparator_).found);

    if (best.empty()) {
        LOG_WARN("Disconnected pattern passed to cost-based qvo strategy. Fall back to id order");
        best = pattern_vertices;
    }
    return best;
}

std::vector<CostBasedQvoStrategy::VertexT> CostBasedQvoStrategy::OrderGreedy() const {
    std::vector const pattern_vertices(boost::vertices(pattern_).first,
                                       boost::vertices(pattern_).second);
    if (pattern_vertices.empty()) {
        return {};
    }

    std::vector<VertexT> order;
    order.reserve(pattern_vertices.size());
    std::unordered_map<VertexT, std::size_t> placed;

    // greedy choose first vertex
    VertexT root = pattern_vertices.front();
    for (VertexT const pv : pattern_vertices) {
        std::size_t const pv_size = VertexDomainSize(pv);
        std::size_t const root_size = VertexDomainSize(root);

        if (pv_size < root_size) {
            root = pv;
        } else if (pv_size == root_size) {
            std::size_t const deg_v =
                    boost::out_degree(pv, pattern_) + boost::in_degree(pv, pattern_);
            std::size_t const deg_r =
                    boost::out_degree(root, pattern_) + boost::in_degree(root, pattern_);
            if (deg_v > deg_r) {
                root = pv;
            }
        }
    }

    double match_estimate = static_cast<double>(VertexDomainSize(root));
    order.push_back(root);
    placed.emplace(root, 0);
    std::vector<double> card_by_level;
    card_by_level.reserve(pattern_vertices.size());
    card_by_level.push_back(match_estimate);

    // minimize plan cost greedy as well
    while (order.size() < pattern_vertices.size()) {
        bool found = false;
        VertexT best{};
        double best_cost = std::numeric_limits<double>::infinity();
        double best_estimate_after = match_estimate;

        for (VertexT pv : pattern_vertices) {
            if (placed.contains(pv) || !ConnectsToPlaced(pv, placed)) {
                continue;
            }
            double estimate = match_estimate;
            double const cost = StepCost(pv, order.size(), placed, card_by_level, estimate);
            if (!found || cost < best_cost) {
                found = true;
                best = pv;
                best_cost = cost;
                best_estimate_after = estimate;
            }
        }

        if (!found) {
            LOG_WARN(
                    "Disconnected pattern passed to cost-based qvo strategy. Fall back to id "
                    "order");
            std::vector<VertexT> rest;
            for (VertexT v : pattern_vertices) {
                if (!placed.contains(v)) {
                    rest.push_back(v);
                }
            }
            std::ranges::sort(rest, pattern_vertices_comparator_);
            for (VertexT v : rest) {
                order.push_back(v);
                placed.emplace(v, order.size() - 1);
            }
            break;
        }

        match_estimate = best_estimate_after;
        order.push_back(best);
        placed.emplace(best, order.size() - 1);
        card_by_level.push_back(match_estimate);
    }

    return order;
}

}  // namespace algos