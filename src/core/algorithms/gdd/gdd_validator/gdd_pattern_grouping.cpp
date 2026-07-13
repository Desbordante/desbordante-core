#include "gdd_pattern_grouping.h"

namespace algos {

namespace {

bool SamePattern(model::gdd::graph_t const& a, model::gdd::graph_t const& b) {
    if (boost::num_vertices(a) != boost::num_vertices(b) ||
        boost::num_edges(a) != boost::num_edges(b)) {
        return false;
    }

    for (std::size_t v = 0; v < boost::num_vertices(a); ++v) {
        if (a[v].id != b[v].id || a[v].label != b[v].label) {
            return false;
        }
    }

    auto edges = [](model::gdd::graph_t const& g) {
        std::vector<std::tuple<std::size_t, std::size_t, std::string>> result;
        result.reserve(boost::num_edges(g));
        for (auto const e : boost::make_iterator_range(boost::edges(g))) {
            result.emplace_back(boost::source(e, g), boost::target(e, g), g[e].label);
        }
        std::ranges::sort(result);
        return result;
    };

    return edges(a) == edges(b);
}

}  // namespace

GddValidator::PatternGrouping GddValidator::PatternGrouping::GroupByPattern(
        std::vector<model::Gdd> const& gdds) {
    std::vector<std::vector<std::size_t>> buckets;

    for (std::size_t i = 0; i < gdds.size(); ++i) {
        auto const bucket = std::ranges::find_if(buckets, [&gdds, i](auto const& indices) {
            return SamePattern(gdds[indices.front()].GetPattern(), gdds[i].GetPattern());
        });

        if (bucket == buckets.end()) {
            buckets.push_back({i});
        } else {
            bucket->push_back(i);
        }
    }

    PatternGrouping grouping;
    grouping.gdds.reserve(gdds.size());
    grouping.origin.reserve(gdds.size());
    grouping.starts.reserve(buckets.size() + 1);
    grouping.starts.push_back(0);

    for (auto const& indices : buckets) {
        for (std::size_t const index : indices) {
            grouping.gdds.push_back(&gdds[index]);
            grouping.origin.push_back(index);
        }
        grouping.starts.push_back(grouping.gdds.size());
    }

    return grouping;
}

}  // namespace algos
