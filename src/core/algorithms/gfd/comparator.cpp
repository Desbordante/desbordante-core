#include "algorithms/gfd/comparator.h"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <boost/graph/vf2_sub_graph_iso.hpp>

#include "algorithms/gfd/gfd.h"

namespace gfd::comparator {

bool CompareLiterals(Literal const& lhs, Literal const& rhs) {
    return (lhs == rhs) || ((lhs.first == rhs.second) && (lhs.second == rhs.first));
}

bool ContainsLiteral(std::vector<Literal> const& literals, Literal const& l) {
    auto check = [&l](auto const& cur_lit) { return CompareLiterals(cur_lit, l); };
    return std::ranges::any_of(literals, check);
}

bool CompareLiteralSets(std::vector<Literal> const& lhs, std::vector<Literal> const& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    auto check = [&rhs](auto const& cur_lit) { return ContainsLiteral(rhs, cur_lit); };
    return std::ranges::all_of(lhs, check);
}

}  // namespace gfd::comparator
