#include "cmspade.h"

#include <chrono>
#include <iostream>

namespace algos::cmspade{
CMSpade::CMSpade() : Algorithm({}){
    RegisterOptions();
    MakeOptionsAvailable({config::names::kSequenceDatabase, config::names::kCMSpadeMinimumSupport});
}

void CMSpade::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto check_minsup = [](double val) {
        if (val <= 0 || val > 1) {
            throw config::ConfigurationError(
                    "Minimum support must be a value between 0 (exclusive) and 1 (inclusive).");
        }
    };

    RegisterOption(config::Option{&database_path_, kSequenceDatabase, kDSequenceDatabase});
    RegisterOption(config::Option{&minsup_, kCMSpadeMinimumSupport, kDCMSpadeMinimumSupport}
                           .SetValueCheck(check_minsup));
}

void CMSpade::LoadDataInternal() {
    using namespace algos::cmspade::parser;
    CMSpadeParser parser(database_path_);
    sequences_ = parser.ParseAll();
}

void CMSpade::ResetState() {
    frequent_patterns_.clear();
    frequent_items_size_1_.clear();
}

unsigned long long CMSpade::ExecuteInternal() {
    minsup_absolute_ = static_cast<MinSupport>(std::ceil(minsup_ * sequences_.size()));

    std::size_t elapsed_time = util::TimedInvoke(&CMSpade::SearchFrequentPatterns, this);

    return elapsed_time;
}
 
void CMSpade::SearchFrequentPatterns(){

    itemset_counts_ = std::make_shared<std::vector<ItemsetId>>();
    for (const auto& sequence : sequences_) {
        itemset_counts_->push_back(sequence->size());
    }
    
    BuildFrequentItems(minsup_absolute_);
    RemoveInfrequentItemsFromSequences();
    RemoveEmptySequences();

    auto frequent_items = TakeFrequentItems();
    
    BuildCMap();

    EquivalenceClass root_class(nullptr, IdList(itemset_counts_));

    for (auto& pair : frequent_items){
        const Pattern* saved_ptr = SavePattern(std::move(pair.first));
        
        EquivalenceClass new_eq_class(saved_ptr, std::move(pair.second));
        
        root_class.AddClassMember(std::move(new_eq_class));
    }

    Execute(root_class);
} 

void CMSpade::Execute(EquivalenceClass& eq_class) {
                
    bool any_pattern_created = false;
    auto& eq_members = eq_class.GetClassMembers();

    for (int i = static_cast<int>(eq_members.size()) - 1; i >= 0; --i) {
        EquivalenceClass child_x = std::move(eq_members[i]);
        Int item_x = child_x.GetClassIdentifier()->GetLastElement().GetItem().GetId();

        const std::unordered_map<Int, MinSupport>* cmap_x_after = nullptr;
        const std::unordered_map<Int, MinSupport>* cmap_x_equal = nullptr;
        
        if (cmap_after_) {
            auto it = cmap_after_->find(item_x);
            if (it != cmap_after_->end()){
                cmap_x_after = &it->second;
            }
        }

        if (cmap_equal_) {
            auto it = cmap_equal_->find(item_x);
            if (it != cmap_equal_->end()){
                cmap_x_equal = &it->second;
            }
        }

        for (int j = i; j >= 0; --j){
            EquivalenceClass& child_y = (i == j) ? child_x : eq_members[j];
            Int item_y = child_y.GetClassIdentifier()->GetLastElement().GetItem().GetId();

            bool do_not_explore_XY = false, do_not_explore_YX = false;
            bool do_not_explore_X_Y = false, do_not_explore_Y_X = false;

            if (cmap_equal_) {
                if (cmap_x_equal) {
                    auto it_pair = cmap_x_equal->find(item_y);
                    if (it_pair != cmap_x_equal->end()) {
                        MinSupport count1 = it_pair->second;
                        do_not_explore_XY = (count1 < minsup_absolute_);
                    } else {
                        do_not_explore_XY = true;
                    }
                } else {
                    do_not_explore_XY = true;
                }
                
                auto it_y = cmap_equal_->find(item_y);
                if (it_y != cmap_equal_->end()) {
                    const auto& cmap_y_equal = it_y->second;
                    auto it_pair_reverse = cmap_y_equal.find(item_x);
                    if (it_pair_reverse != cmap_y_equal.end()) {
                        MinSupport count2 = it_pair_reverse->second;
                        do_not_explore_YX = (count2 < minsup_absolute_);
                    } else {
                        do_not_explore_YX = true;
                    }
                } else {
                    do_not_explore_YX = true;
                }
            } else {
                do_not_explore_XY = true;
                do_not_explore_YX = true;
            }

            if (cmap_after_) {
                if (cmap_x_after) {
                    auto it_pair = cmap_x_after->find(item_y);
                    if (it_pair != cmap_x_after->end()) {
                        MinSupport count1 = it_pair->second;
                        do_not_explore_X_Y = (count1 < minsup_absolute_);
                    } else {
                        do_not_explore_X_Y = true;
                    }
                } else {
                    do_not_explore_X_Y = true;
                }
                
                auto it_y = cmap_after_->find(item_y);
                if (it_y != cmap_after_->end()) {
                    const auto& cmap_y_after = it_y->second;
                    auto it_pair_reverse = cmap_y_after.find(item_x);
                    if (it_pair_reverse != cmap_y_after.end()) {
                        MinSupport count2 = it_pair_reverse->second;
                        do_not_explore_Y_X = (count2 < minsup_absolute_);
                    } else {
                        do_not_explore_Y_X = true;
                    }
                } else {
                    do_not_explore_Y_X = true;
                }
            } else {
                do_not_explore_X_Y = true;
                do_not_explore_Y_X = true;
            }

            if (do_not_explore_XY && do_not_explore_YX && do_not_explore_X_Y && do_not_explore_Y_X){
                continue;
            }

            auto extensions = Generator(child_x.GetClassIdentifier(), child_y.GetClassIdentifier(), 
                                                            minsup_absolute_, do_not_explore_XY, do_not_explore_YX, 
                                                            do_not_explore_X_Y, do_not_explore_Y_X);
            
            for (auto& extension: extensions){
                auto new_id_list = Join(extension, child_x, child_y, minsup_absolute_);

                if (new_id_list && (new_id_list->GetSupport() >= minsup_absolute_)){
                    any_pattern_created = true;
                    new_id_list->SetAppearingInPattern(extension);

                    const Pattern* saved_ptr = SavePattern(std::move(extension));

                    EquivalenceClass new_eq_class(saved_ptr, std::move(*new_id_list));

                    bool x_is_prefix = child_x.GetClassIdentifier()->IsPrefix(*saved_ptr);

                    if (x_is_prefix) {
                        child_x.AddClassMember(std::move(new_eq_class));
                    } else {
                        child_y.AddClassMember(std::move(new_eq_class));
                    }
                }
            }
        }

        eq_members.erase(eq_members.begin() + i);
        
        if (any_pattern_created) {
            Execute(child_x);
        }
    }
}

std::vector<Pattern> CMSpade::Generator(
    const Pattern* pattern1, 
    const Pattern* pattern2, 
    MinSupport min_sup, 
    bool do_not_explore_XY, 
    bool do_not_explore_YX,
    bool do_not_explore_X_Y, 
    bool do_not_explore_Y_X) {
    
    std::vector<Pattern> candidates;
    
    const boost::dynamic_bitset<>& appearing1 = pattern1->GetAppearing();
    const boost::dynamic_bitset<>& appearing2 = pattern2->GetAppearing();

    boost::dynamic_bitset<> join_bitset = appearing1 & appearing2;
    if (join_bitset.count() < min_sup) {
        return candidates;
    }

    const auto& last_element_pattern1 = pattern1->GetLastElement();
    const auto& last_element_pattern2 = pattern2->GetLastElement();

    bool is_equal1 = last_element_pattern1.HaveEqualRelation();
    bool is_equal2 = last_element_pattern2.HaveEqualRelation();
    
    const Item& item1 = last_element_pattern1.GetItem();
    const Item& item2 = last_element_pattern2.GetItem();

    int pattern_compare = pattern1->CompareTo(*pattern2);

    if ((!is_equal1) && (!is_equal2)) {
        if (item1 != item2) {
            if (pattern_compare < 0 && !do_not_explore_XY) {
                Pattern candidate = pattern1->Clone();
                candidate.Add(ItemAbstractionPair(item2, true));
                candidates.emplace_back(std::move(candidate));
            }  
            else if (!do_not_explore_YX) {
                Pattern candidate = pattern2->Clone();
                candidate.Add(ItemAbstractionPair(item1, true));
                candidates.emplace_back(std::move(candidate));
            }

            if (!do_not_explore_Y_X) {
                Pattern candidate = pattern2->Clone();
                candidate.Add(last_element_pattern1);
                candidates.emplace_back(std::move(candidate));
            }
        }

	    if (!do_not_explore_X_Y) {
            Pattern candidate = pattern1->Clone();
            candidate.Add(last_element_pattern2);
            candidates.emplace_back(std::move(candidate));
        }
    }
    else if (is_equal1 && is_equal2) {
        if (pattern_compare < 0 && !do_not_explore_XY) {
            Pattern candidate = pattern1->Clone();
            candidate.Add(ItemAbstractionPair(item2, true));
            candidates.emplace_back(std::move(candidate));
        } 
        else if (pattern_compare > 0 && !do_not_explore_YX) {
            Pattern candidate = pattern2->Clone();
            candidate.Add(ItemAbstractionPair(item1, true));
            candidates.emplace_back(std::move(candidate));
        }
    } else {
        if (is_equal1 && !do_not_explore_X_Y) {
            Pattern candidate = pattern1->Clone();
            candidate.Add(last_element_pattern2);
            candidates.emplace_back(std::move(candidate));
        } 
        else if (!do_not_explore_Y_X) {
            Pattern candidate = pattern2->Clone();
            candidate.Add(last_element_pattern1);
            candidates.emplace_back(std::move(candidate));
        }
    } 

    return candidates;
}

std::optional<IdList> CMSpade::Join(
    const Pattern& extension, 
    const EquivalenceClass& class_i, 
    const EquivalenceClass& class_j, 
    MinSupport min_sup) {
    
    const auto& id_list_class_i = class_i.GetIdList();
    const auto& id_list_class_j = class_j.GetIdList();

    const auto& penultimate_element = extension.GetPenultimateElement();
    const auto& last_element = extension.GetLastElement();
    const auto& last_element_from_class_j = class_j.GetClassIdentifier()->GetLastElement();

    if (last_element.HaveEqualRelation()) {
        if (!(penultimate_element.Equals(last_element))) {
            return id_list_class_i.Join(id_list_class_j, true, min_sup);
        } 
    }
    else {
        if (last_element_from_class_j.Equals(last_element)) {
            return id_list_class_i.Join(id_list_class_j, false, min_sup);
        } else {
            return id_list_class_j.Join(id_list_class_i, false, min_sup);
        }
    }

    return std::nullopt;
}
 
void CMSpade::BuildFrequentItems(MinSupport min_support) {
    std::unordered_map<Item, std::pair<Pattern, IdList>> eq_classes;

    std::shared_ptr<const std::vector<ItemsetId>> const_itemset_counts = itemset_counts_;
    
    for (SequenceId sid = 0; sid < sequences_.size(); ++sid) {
        const auto& sequence = sequences_[sid];
        const auto& itemsets = sequence->GetItemsets();
        
        for (ItemsetId tid = 0; tid < itemsets.size(); ++tid) {
            const auto& itemset = itemsets[tid];
            
            for (const auto& item : itemset->GetItems()) {
                auto it = eq_classes.find(item);
                if (it == eq_classes.end()) {
                    IdList id_list(const_itemset_counts);
                    
                    ItemAbstractionPair pair(item, false);
                    Pattern pattern(pair);
                    
                    id_list.RegisterBit(sid, tid);
                    eq_classes.emplace(item, std::make_pair(std::move(pattern), std::move(id_list)));
                } else {
                    it->second.second.RegisterBit(sid, tid);
                }
            }
        }
    }
 
    FilterByMinSupport(eq_classes, min_support);

    for (auto& [item, pair] : eq_classes) {
        pair.second.SetAppearingInPattern(pair.first);
    }
    
    frequent_items_size_1_ = std::move(eq_classes);
}

void CMSpade::BuildCMap(){
    if (cmap_built_){
        return;
    }

    cmap_after_ = std::make_shared<CMAP>();
    cmap_equal_ = std::make_shared<CMAP>();

    for (const auto& sequence : sequences_){
        std::unordered_set<Int> already_seen_after;
        std::unordered_map<Int, std::unordered_set<Int>> already_seen_equal;

        const auto& itemsets = sequence->GetItemsets();
        for (size_t i = 0; i < itemsets.size(); i++){
            const auto& itemset = itemsets[i];

            for (size_t j = 0; j < itemset->size(); j++){
                Int item_a = itemset->GetItem(j).GetId();

                auto& equal_set = already_seen_equal[item_a];
                for (size_t k = j + 1; k < itemset->size(); k++){
                    Int item_b = itemset->GetItem(k).GetId();

                    if (equal_set.find(item_b) == equal_set.end()){
                        (*cmap_equal_)[item_a][item_b]++;
                        equal_set.insert(item_b);
                    }
                }

                if (already_seen_after.find(item_a) == already_seen_after.end()){
                    std::unordered_set<Int> seen_b_after;
                    for(size_t k = i + 1; k < itemsets.size(); k++){
                        const auto& next_itemset = itemsets[k];

                        for (size_t m = 0; m < next_itemset->size(); m++){
                            Int item_b = next_itemset->GetItem(m).GetId();

                            if (seen_b_after.find(item_b) == seen_b_after.end()){
                                (*cmap_after_)[item_a][item_b]++;
                                seen_b_after.insert(item_b);
                            }
                        }
                    }
                    already_seen_after.insert(item_a);
                }
            }
        }
    }

    cmap_built_ = true;
}

std::vector<std::pair<Pattern, IdList>> CMSpade::TakeFrequentItems() {
    std::vector<std::pair<Pattern, IdList>> result;
    result.reserve(frequent_items_size_1_.size());

    for (auto& [item, pair] : frequent_items_size_1_) {
        result.push_back(std::move(pair));
    }

    frequent_items_size_1_.clear();
    std::sort(result.begin(), result.end(), 
        [](const std::pair<Pattern, IdList>& a, 
           const std::pair<Pattern, IdList>& b) {
            return a.first.CompareTo(b.first) < 0;
        });
    return result;
}

void CMSpade::FilterByMinSupport(
    std::unordered_map<Item, std::pair<Pattern, IdList>>& eq_classes,
    MinSupport min_support) {
    
    for (auto it = eq_classes.begin(); it != eq_classes.end(); ) {
        if ((it->second.second.GetSupport()) < min_support) {
            it = eq_classes.erase(it);
        }
        else {
            ++it;
        }
    }
}

void CMSpade::RemoveInfrequentItemsFromSequences(){
    for (auto& sequence : sequences_) {
        for (size_t i = 0; i < sequence->size(); i++) {
            auto& itemset = sequence->GetItemsets()[i];

            for (size_t j = 0; j < itemset->size(); j++) {
                Item item = itemset->GetItem(j);
                if (frequent_items_size_1_.find(item) == frequent_items_size_1_.end()) {
                    sequence->RemoveItem(i, j);
                    j--;
                }
            }
            if (itemset->GetItems().empty()) {
                sequence->RemoveItemset(i);
                i--;
            }
        }
    }
}

void CMSpade::RemoveEmptySequences() {
    sequences_.erase(
        std::remove_if(sequences_.begin(), sequences_.end(),
            [](const std::unique_ptr<Sequence>& seq) { 
                return seq->size() == 0; 
            }),
        sequences_.end()
    );
}
}