#pragma once

#include <vector>

#include <boost/graph/vf2_sub_graph_iso.hpp>

#include "algorithms/gfd/gfd.h"

namespace gfd::comparator {

bool CompareLiterals(model::Gfd::Literal const& lhs, model::Gfd::Literal const& rhs);
bool ContainsLiteral(std::vector<model::Gfd::Literal> const& literals,
                     model::Gfd::Literal const& l);
bool ContainsLiterals(std::vector<model::Gfd::Literal> const& superset,
                      std::vector<model::Gfd::Literal> const& subset);
bool CompareLiteralSets(std::vector<model::Gfd::Literal> const& lhs,
                        std::vector<model::Gfd::Literal> const& rhs);

}  // namespace gfd::comparator
