#pragma once 

#include "types.h"
#include "pattern.h"

#include <cstddef>
#include <vector>
#include <memory>
#include <utility>
#include <optional>

#include <boost/dynamic_bitset.hpp>

namespace algos::cmspade{ 
class IdList{
private:
    std::vector<boost::dynamic_bitset<>> sequence_itemset_entries_;
    boost::dynamic_bitset<> sequences_;
    std::shared_ptr<const std::vector<ItemsetId>> itemset_counts_;
    MinSupport support_ = 0;

public:
    static boost::dynamic_bitset<> EqualOperation(const boost::dynamic_bitset<>& bitset1, 
                                                const boost::dynamic_bitset<>& bitset2);

    static boost::dynamic_bitset<> AfterOperation(const boost::dynamic_bitset<>& bitset1, 
                                                const boost::dynamic_bitset<>& bitset2);

    IdList(std::shared_ptr<const std::vector<ItemsetId>> itemset_counts);

    ~IdList() = default;

    IdList(const IdList&) = default;
    IdList& operator=(const IdList&) = default;

    IdList(IdList&&) = default;
    IdList& operator=(IdList&&) = default;

    void RegisterBit(SequenceId sid, ItemsetId tid);

    MinSupport GetSupport() const {
        return support_;
    }

    const std::vector<boost::dynamic_bitset<>>& GetSequenceItemsetEntries() const{
        return sequence_itemset_entries_;
    }

    const boost::dynamic_bitset<>& GetSequencesBitset() const{
        return sequences_;
    }

    void SetAppearingInPattern(Pattern& pattern){
        pattern.SetAppearing(sequences_);
    }

    void Clear() {
        sequences_.clear();
        sequence_itemset_entries_.clear();
        support_ = 0;
    }

    std::optional<IdList> Join(const IdList& other, bool is_equal, MinSupport min_sup) const;
};
} // namespace algos::cmspade