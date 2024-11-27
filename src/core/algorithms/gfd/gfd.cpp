#include "gfd.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>

#include <boost/graph/vf2_sub_graph_iso.hpp>

#include "parser/graph_parser/graph_parser.h"

std::string Gfd::ToString() {
    std::stringstream gfd_stream;
    parser::graph_parser::WriteGfd(gfd_stream, *this);
    return gfd_stream.str();
}

namespace {

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

bool CompareLiterals(Literal const& lhs, Literal const& rhs) {
    return (lhs == rhs) || ((lhs.first == rhs.second) && (lhs.second == rhs.first));
}

bool ContainsLiteral(std::vector<Literal> const& literals, Literal const& l) {
    auto check = [&l](auto const& cur_lit) { return CompareLiterals(cur_lit, l); };
    return std::any_of(literals.begin(), literals.end(), check);
}

bool CompareLiteralSets(std::vector<Literal> const& lhs, std::vector<Literal> const& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    auto check = [&rhs](auto const& cur_lit) { return ContainsLiteral(rhs, cur_lit); };
    return std::all_of(lhs.begin(), lhs.end(), check);
}

bool IsSub(graph_t const& query, graph_t const& graph) {
    bool result = false;
    VCmp vcmp{query, graph};
    ECmp ecmp{query, graph};
    CmpCallback callback(result);
    boost::vf2_subgraph_iso(query, graph, callback, get(boost::vertex_index, query),
                            get(boost::vertex_index, graph), vertex_order_by_mult(query), ecmp,
                            vcmp);
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