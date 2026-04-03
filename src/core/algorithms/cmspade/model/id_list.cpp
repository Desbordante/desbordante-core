#include "id_list.h"
#include <iostream>
namespace algos::cmspade{
IdList::IdList(std::shared_ptr<const std::vector<ItemsetId>> itemset_counts)
    : itemset_counts_(itemset_counts) {
        
    sequences_.resize(itemset_counts_->size(), false);
    sequence_itemset_entries_.resize(itemset_counts_->size());
}

boost::dynamic_bitset<> IdList::EqualOperation(const boost::dynamic_bitset<>& bitset1, 
                                            const boost::dynamic_bitset<>& bitset2) {
    return bitset1 & bitset2;
} 
 
boost::dynamic_bitset<> IdList::AfterOperation(const boost::dynamic_bitset<>& bitset1, 
                                            const boost::dynamic_bitset<>& bitset2){
    if (bitset1.none() || bitset2.none()){
        return boost::dynamic_bitset<>();
    }

    size_t first_bit_in_bitset1 = bitset1.find_first();
    if (first_bit_in_bitset1 == boost::dynamic_bitset<>::npos) {
        return boost::dynamic_bitset<>();
    } 
    
    boost::dynamic_bitset<> result = bitset2;
    
    for (size_t i = 0; i <= first_bit_in_bitset1 && i < result.size(); ++i) {
        result.reset(i);
    }
    
    if (result.any()) {
        return result;
    }
    
    return boost::dynamic_bitset<>();
}

void IdList::RegisterBit(SequenceId sid, ItemsetId tid){
    auto& bitset = sequence_itemset_entries_[sid];
    
    if (bitset.size() == 0) {
        bitset.resize((*itemset_counts_)[sid], false);
    } 
    
    bitset.set(tid);
    
    if (!sequences_.test(sid)) {
        sequences_.set(sid);
        support_++;
    }
}

std::optional<IdList> IdList::Join(const IdList& other, bool is_equal, MinSupport min_sup) const {
    boost::dynamic_bitset<> common_sequences = sequences_ & other.sequences_;
    MinSupport potential_support = static_cast<MinSupport>(common_sequences.count());
    
    if (potential_support < min_sup) {
        return std::nullopt;
    }

    IdList result(itemset_counts_);
    
    for (std::size_t sid = common_sequences.find_first(); sid != boost::dynamic_bitset<>::npos; sid = common_sequences.find_next(sid)) {
        const auto& this_bitset = sequence_itemset_entries_[sid];
        const auto& other_bitset = other.sequence_itemset_entries_[sid];
        
        boost::dynamic_bitset<> result_bitset;

        if (is_equal){
            result_bitset = EqualOperation(this_bitset, other_bitset);
        }
        else{
            result_bitset = AfterOperation(this_bitset, other_bitset);
        }

        if (result_bitset.any()) {
            result.sequence_itemset_entries_[sid] = std::move(result_bitset);
            result.sequences_.set(sid);
            result.support_++;
        }
    }
    
    if (result.support_ >= min_sup) {
        return result;
    }
    
    return std::nullopt;
}
} // namespace algos::cmspade