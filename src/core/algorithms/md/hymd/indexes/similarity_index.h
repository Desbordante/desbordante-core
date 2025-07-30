#pragma once

#include <cassert>     // for assert
#include <functional>  // for less, gre...
#include <memory>      // for shared_ptr
#include <mutex>       // for lock_guard
#include <utility>     // for pair
#include <vector>      // for vector

#include <boost/container/flat_map.hpp>            // for flat_map
#include <boost/container/vector.hpp>              // for vec_iterator
#include <boost/container_hash/hash.hpp>           // for hash
#include <boost/unordered/unordered_flat_set.hpp>  // for unordered...

#include "algorithms/md/hymd/column_classifier_value_id.h"  // for ColumnCla...
#include "algorithms/md/hymd/table_identifiers.h"           // for RecordIde...
#include "model/index.h"                                    // for Index

namespace algos::hymd::indexes {
using EndIdMap = boost::container::flat_map<ColumnClassifierValueId, model::Index, std::greater<>>;

struct FlatUpperSetIndex {
    std::vector<RecordIdentifier> sorted_records;
    EndIdMap end_ids;
};

using RecSet = boost::unordered::unordered_flat_set<RecordIdentifier>;
using UpperSetMapping = boost::container::flat_map<ColumnClassifierValueId, RecSet>;

class FastUpperSetMapping {
    FlatUpperSetIndex flat_;
    UpperSetMapping sets_;

public:
    FlatUpperSetIndex const& GetFlat() const noexcept {
        return flat_;
    }

    RecSet const* GetUpperSet(ColumnClassifierValueId lhs_ccv_id) const {
        auto it = sets_.lower_bound(lhs_ccv_id);
        if (it == sets_.end()) return nullptr;
        return &it->second;
    }

    FastUpperSetMapping(FlatUpperSetIndex flat);
    FastUpperSetMapping() = default;
};

class CachingUpperSetMapping {
    using SetCont = std::pair<std::shared_ptr<std::mutex>, RecSet>;
    FlatUpperSetIndex flat_;
    boost::container::flat_map<ColumnClassifierValueId, SetCont> sets_;

public:
    FlatUpperSetIndex const& GetFlat() const noexcept {
        return flat_;
    }

    RecSet const* GetUpperSet(ColumnClassifierValueId lhs_ccv_id) const {
        auto it = sets_.lower_bound(lhs_ccv_id);
        auto const end = sets_.end();
        if (it == end) return nullptr;
        auto& [mutex, set] = const_cast<SetCont&>(it->second);
        {
            auto lock = std::lock_guard(*mutex);
            if (set.empty()) {
                auto start_it = flat_.sorted_records.begin();
                assert(flat_.end_ids.find(it->first) == flat_.end_ids.begin() + (end - it - 1));
                set.insert(start_it, start_it + (flat_.end_ids.begin() + (end - it - 1))->second);
            }
        }
        return &set;
    }

    CachingUpperSetMapping(FlatUpperSetIndex flat);
    CachingUpperSetMapping() = default;
};

using ValueUpperSetMapping = FastUpperSetMapping;
using SimilarityIndex = std::vector<ValueUpperSetMapping>;

}  // namespace algos::hymd::indexes
