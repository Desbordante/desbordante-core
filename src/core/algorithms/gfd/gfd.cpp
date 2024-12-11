#include "gfd.h"

#include <sstream>

#include "parser/graph_parser/graph_parser.h"

namespace model {

std::string model::Gfd::ToString() const {
    std::stringstream gfd_stream;
    parser::graph_parser::WriteGfd(gfd_stream, *this);
    return gfd_stream.str();
}

namespace {

bool IsSub(graph_t const& query, graph_t const& graph) {
    bool result = false;
    auto vcmp = [&query, &graph](vertex_t const& fr, vertex_t const& to) {
        return query[fr].attributes.at("label") == graph[to].attributes.at("label");
    };
    auto ecmp = [&query, &graph](edge_t const& fr, edge_t const& to) {
        return query[fr].label == graph[to].label;
    };
    auto callback = [&result](auto, auto) {
        result = true;
        return false;
    };
    using property_map_type = boost::property_map<graph_t, boost::vertex_index_t>::type;
    property_map_type query_index_map = boost::get(boost::vertex_index, query);
    property_map_type graph_index_map = boost::get(boost::vertex_index, graph);
    std::vector<vertex_t> query_vertex_order = vertex_order_by_mult(query);
    boost::vf2_subgraph_iso(query, graph, callback, query_index_map, graph_index_map,
                            query_vertex_order, ecmp, vcmp);
    return result;
}

}  // namespace

bool Gfd::operator==(Gfd const& gfd) const {
    graph_t pat = gfd.GetPattern();
    return IsSub(pattern_, pat) && IsSub(pat, pattern_) &&
           gfd::comparator::CompareLiteralSets(premises_, gfd.GetPremises()) &&
           gfd::comparator::CompareLiteralSets(conclusion_, gfd.GetConclusion());
}

}  // namespace model
