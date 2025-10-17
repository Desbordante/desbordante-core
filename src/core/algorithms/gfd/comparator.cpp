#include "algorithms/gfd/comparator.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "algorithms/gfd/gfd.h"

namespace gfd::comparator {

bool CompareLiterals(model::Gfd::Literal const& lhs, model::Gfd::Literal const& rhs) {
    return (lhs == rhs) || ((lhs.first == rhs.second) && (lhs.second == rhs.first));
}

bool ContainsLiteral(std::vector<model::Gfd::Literal> const& literals,
                     model::Gfd::Literal const& l) {
    auto check = [&l](auto const& cur_lit) { return CompareLiterals(cur_lit, l); };
    return std::ranges::any_of(literals, check);
}

bool ContainsLiterals(std::vector<model::Gfd::Literal> const& superset,
                      std::vector<model::Gfd::Literal> const& subset) {
    auto check = [&superset](auto const& cur_lit) { return ContainsLiteral(superset, cur_lit); };
    return std::ranges::all_of(subset, check);
}

bool CompareLiteralSets(std::vector<model::Gfd::Literal> const& lhs,
                        std::vector<model::Gfd::Literal> const& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    auto check = [&rhs](auto const& cur_lit) { return ContainsLiteral(rhs, cur_lit); };
    return std::ranges::all_of(lhs, check);
}

}  // namespace gfd::comparator
