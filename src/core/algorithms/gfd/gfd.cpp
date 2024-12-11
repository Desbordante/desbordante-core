#include "algorithms/gfd/gfd.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>

#include <boost/graph/vf2_sub_graph_iso.hpp>

#include "algorithms/gfd/comparator.h"
#include "parser/graph_parser/graph_parser.h"

namespace details {

std::string Gfd::ToString() const {
    std::stringstream gfd_stream;
    parser::graph_parser::WriteGfd(gfd_stream, *this);
    return gfd_stream.str();
}

namespace {

using namespace comparator;

class CmpCallback {
private:
    bool& res_;

public:
    CmpCallback(bool& res) : res_(res) {}

    template <typename CorrespondenceMap1To2, typename CorrespondenceMap2To1>
    bool operator()(CorrespondenceMap1To2, CorrespondenceMap2To1) const {
        res_ = true;
        return false;
    }
};

struct VCmp {
    graph_t const& lhs;
    graph_t const& rhs;

    bool operator()(vertex_t const& fr, vertex_t const& to) const {
        return lhs[fr].attributes.at("label") == rhs[to].attributes.at("label");
    }
};

struct ECmp {
    graph_t const& lhs;
    graph_t const& rhs;

    bool operator()(edge_t const& fr, edge_t const& to) const {
        return lhs[fr].label == rhs[to].label;
    }
};

bool IsSub(graph_t const& query, graph_t const& graph) {
    bool result = false;
    VCmp vcmp = {query, graph};
    ECmp ecmp = {query, graph};
    CmpCallback callback(result);
    boost::property_map<graph_t, boost::vertex_index_t>::type query_index_map =
            get(boost::vertex_index, query);
    boost::property_map<graph_t, boost::vertex_index_t>::type graph_index_map =
            get(boost::vertex_index, graph);
    std::vector<vertex_t> query_vertex_order = vertex_order_by_mult(query);
    boost::vf2_subgraph_iso(query, graph, callback, query_index_map, graph_index_map,
                            query_vertex_order, ecmp, vcmp);
    return result;
}

}  // namespace

bool Gfd::operator==(Gfd const& gfd) const {
    graph_t pat = gfd.GetPattern();
    return IsSub(pattern_, pat) && IsSub(pat, pattern_) &&
           CompareLiteralSets(premises_, gfd.GetPremises()) &&
           CompareLiteralSets(conclusion_, gfd.GetConclusion());
}

bool Gfd::operator!=(Gfd const& gfd) const {
    return !(*this == gfd);
}

}  // namespace details
