#include "cfd_output_util.h"

#include <iosfwd>
#include <ostream>
#include <sstream>
#include <vector>

#include <boost/algorithm/string/join.hpp>

#include "algorithms/cfd/model/cfd_relation_data.h"
#include "algorithms/cfd/model/cfd_types.h"

// see algorithms/cfd/LICENSE

namespace algos::cfd {

int Output::ItemToAttrIndex(Item item, CFDRelationData const& db) {
    if (item < 0) {
        return -1 - item;
    } else {
        return db.GetAttrIndex(item);
    }
}

std::optional<std::string> Output::ItemToPatternOpt(Item item, CFDRelationData const& db) {
    if (item < 0) {
        return std::nullopt;
    } else if (item == 0) {
        return "N/A";
    } else {
        return db.GetValue(item);
    }
}

std::string Output::ItemToPattern(Item item, CFDRelationData const& db) {
    std::optional<std::string> pattern_opt = ItemToPatternOpt(item, db);
    return pattern_opt.has_value() ? "=" + pattern_opt.value() : "";
}

std::string Output::ItemsetToString(Itemset const& items,
                                    std::shared_ptr<CFDRelationData const> const& db) {
    std::string answer;
    answer += '(';
    std::vector<std::string> parts;
    for (unsigned int i = 0; i < items.size(); i++) {
        Item item = items[i];
        std::string attr_name =
                (item == 0) ? db->GetAttrName(static_cast<int>(i)) : ItemToAttrName(item, *db);
        std::string pattern = ItemToPattern(item, *db);
        parts.emplace_back(attr_name + pattern);
    }
    answer += boost::join(parts, ", ");
    answer += ")";
    return answer;
}

std::string Output::CFDListToString(ItemsetCFDList const& cs,
                                    std::shared_ptr<CFDRelationData const> const& db) {
    std::string answer;
    for (auto const& [lhs, rhs] : cs) {
        answer += CFDToString(lhs, rhs, db);
    }
    return answer;
}

std::string Output::CFDToString(Itemset const& lhs, Item rhs,
                                std::shared_ptr<CFDRelationData const> const& db) {
    std::stringstream ss;
    ss << ItemsetToString(lhs, db) << " => " << ItemToAttrName(rhs, *db) << ItemToPattern(rhs, *db);
    return ss.str();
}

}  // namespace algos::cfd
