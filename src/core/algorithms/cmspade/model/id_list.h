#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "pattern.h"
#include "types.h"

namespace algos::cmspade {
class IdList {
private:
    std::vector<boost::dynamic_bitset<>> sequence_itemset_entries_;
    boost::dynamic_bitset<> sequences_;
    std::shared_ptr<std::vector<ItemsetId> const> itemset_counts_;
    MinSupport support_ = 0;

public:
    static boost::dynamic_bitset<> EqualOperation(boost::dynamic_bitset<> const& bitset1,
                                                  boost::dynamic_bitset<> const& bitset2);

    static boost::dynamic_bitset<> AfterOperation(boost::dynamic_bitset<> const& bitset1,
                                                  boost::dynamic_bitset<> const& bitset2);

    IdList(std::shared_ptr<std::vector<ItemsetId> const> itemset_counts);

    ~IdList() = default;

    IdList(IdList const&) = default;
    IdList& operator=(IdList const&) = default;

    IdList(IdList&&) = default;
    IdList& operator=(IdList&&) = default;

    void RegisterBit(SequenceId sid, ItemsetId tid);

    MinSupport GetSupport() const {
        return support_;
    }

    std::vector<boost::dynamic_bitset<>> const& GetSequenceItemsetEntries() const {
        return sequence_itemset_entries_;
    }

    boost::dynamic_bitset<> const& GetSequencesBitset() const {
        return sequences_;
    }

    void SetAppearingInPattern(Pattern& pattern) {
        pattern.SetAppearing(sequences_);
    }

    void Clear() {
        sequences_.clear();
        sequence_itemset_entries_.clear();
        support_ = 0;
    }

    std::optional<IdList> Join(IdList const& other, bool is_equal, MinSupport min_sup) const;
};
}  // namespace algos::cmspade