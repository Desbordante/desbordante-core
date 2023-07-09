#pragma once

#include <iostream>
#include <ostream>

#include <boost/algorithm/string.hpp>

#include "functional/cfd/structures/cfd_relation_data.h"
#include "functional/cfd/structures/cfd_types.h"

// see algorithms/cfd/LICENSE

namespace algos::cfd {

class Output {
public:
    static std::string ItemsetToString(const Itemset& items,
                                       std::shared_ptr<CFDRelationData const> const& db) {
        std::string answer;
        answer += '(';
        std::vector<std::string> parts;
        for (uint ix = 0; ix < items.size(); ix++) {
            int item = items[ix];
            if (item < 0) {
                parts.push_back(db->GetAttrName(-1 - item));
            } else if (item == 0) {
                parts.push_back(db->GetAttrName((int)ix) + "=N/A");
            } else {
                parts.push_back(db->GetAttrName(db->GetAttrIndex(item)) + "=" + db->GetValue(item));
            }
        }
        answer += boost::join(parts, ", ");
        answer += ")";
        return answer;
    }

    static std::string CFDListToString(const CFDList& cs,
                                       std::shared_ptr<CFDRelationData const> const& db) {
        std::string answer;
        for (const auto& c : cs) {
            answer += CFDToString(c.first, c.second, db);
        }
        return answer;
    }

    static std::string CFDToString(const CFD& c, std::shared_ptr<CFDRelationData const> const& db) {
        return CFDToString(c.first, c.second, db);
    }

    static std::string CFDToString(const Itemset& lhs, const int rhs,
                                   std::shared_ptr<CFDRelationData const> const& db) {
        std::string answer;
        answer = ItemsetToString(lhs, db);
        answer += " => ";
        if (rhs < 0) {
            answer += db->GetAttrName(-1 - rhs);
        } else {
            answer += (db->GetAttrName(db->GetAttrIndex(rhs)) + "=" + db->GetValue(rhs));
        }
        return answer;
    }
};
}  // namespace algos::cfd
