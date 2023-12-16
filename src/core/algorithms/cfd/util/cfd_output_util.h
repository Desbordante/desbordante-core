#pragma once

#include <memory>
#include <optional>
#include <string>

#include "algorithms/cfd/model/cfd_relation_data.h"
#include "algorithms/cfd/model/cfd_types.h"

// see algorithms/cfd/LICENSE

namespace algos::cfd {

class Output {
public:
    static int ItemToAttrIndex(Item item, CFDRelationData const& db);

    static std::string ItemToAttrName(Item item, CFDRelationData const& db) {
        return db.GetAttrName(ItemToAttrIndex(item, db));
    }

    static std::optional<std::string> ItemToPatternOpt(Item item, CFDRelationData const& db);
    static std::string ItemToPattern(Item item, CFDRelationData const& db);

    static std::string ItemsetToString(Itemset const& items,
                                       std::shared_ptr<CFDRelationData const> const& db);

    static std::string CFDListToString(ItemsetCFDList const& cs,
                                       std::shared_ptr<CFDRelationData const> const& db);

    static std::string CFDToString(ItemsetCFD const& c,
                                   std::shared_ptr<CFDRelationData const> const& db) {
        auto const& [lhs, rhs] = c;
        return CFDToString(lhs, rhs, db);
    }

    static std::string CFDToString(Itemset const& lhs, Item rhs,
                                   std::shared_ptr<CFDRelationData const> const& db);
};
}  // namespace algos::cfd
