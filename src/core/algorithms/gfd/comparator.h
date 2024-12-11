#pragma once

#include <vector>

#include <boost/graph/vf2_sub_graph_iso.hpp>

#include "algorithms/gfd/gfd.h"

namespace comparator {

using namespace details;

bool CompareLiterals(Literal const& lhs, Literal const& rhs);
bool ContainsLiteral(std::vector<Literal> const& literals, Literal const& l);
bool CompareLiteralSets(std::vector<Literal> const& lhs, std::vector<Literal> const& rhs);

}  // namespace comparator
