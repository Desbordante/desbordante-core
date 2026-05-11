#include "cmspade.h"

namespace algos::cmspade {
CMSpade::CMSpade() : Algorithm() {
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
    RegisterOption(
            config::Option{&minsup_, kCMSpadeMinimumSupport, kDCMSpadeMinimumSupport}.SetValueCheck(
                    check_minsup));
}

void CMSpade::LoadDataInternal() {
    using namespace algos::cmspade::parser;
    CMSpadeParser parser(database_path_);
    sequences_ = parser.ParseAll();
}

void CMSpade::ResetState() {
    frequent_patterns_.clear();
    frequent_items_size_1_.clear();
    itemset_counts_.reset();

    cmap_built_ = false;
    cmap_after_.reset();
    cmap_equal_.reset();

    minsup_absolute_ = 0;
}

unsigned long long CMSpade::ExecuteInternal() {
    sequences_.clear();
    LoadDataInternal();

    minsup_absolute_ = static_cast<MinSupport>(std::ceil(minsup_ * sequences_.size()));

    std::size_t elapsed_time = util::TimedInvoke(&CMSpade::SearchFrequentPatterns, this);

    return elapsed_time;
}

void CMSpade::SearchFrequentPatterns() {
    itemset_counts_ = std::make_shared<std::vector<ItemsetId>>();
    for (auto const& sequence : sequences_) {
        itemset_counts_->push_back(sequence->Size());
    }

    BuildFrequentItems();
    RemoveInfrequentItemsFromSequences();
    RemoveEmptySequences();

    auto frequent_items = TakeFrequentItems();

    BuildCMap();

    EquivalenceClass root_class(nullptr, IdList(itemset_counts_));

    for (auto& pair : frequent_items) {
        Pattern const* saved_ptr = SavePattern(std::move(pair.first));

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

        std::unordered_map<Int, MinSupport> const* cmap_x_after = nullptr;
        std::unordered_map<Int, MinSupport> const* cmap_x_equal = nullptr;

        if (cmap_after_) {
            auto it = cmap_after_->find(item_x);
            if (it != cmap_after_->end()) {
                cmap_x_after = &it->second;
            }
        }

        if (cmap_equal_) {
            auto it = cmap_equal_->find(item_x);
            if (it != cmap_equal_->end()) {
                cmap_x_equal = &it->second;
            }
        }

        for (int j = i; j >= 0; --j) {
            EquivalenceClass& child_y = (i == j) ? child_x : eq_members[j];
            Int item_y = child_y.GetClassIdentifier()->GetLastElement().GetItem().GetId();

            bool do_not_explore_xy = false, do_not_explore_yx = false;
            bool do_not_explore_x_y = false, do_not_explore_y_x = false;

            if (cmap_equal_) {
                if (cmap_x_equal) {
                    auto it_pair = cmap_x_equal->find(item_y);
                    if (it_pair != cmap_x_equal->end()) {
                        MinSupport count1 = it_pair->second;
                        do_not_explore_xy = (count1 < minsup_absolute_);
                    } else {
                        do_not_explore_xy = true;
                    }
                } else {
                    do_not_explore_xy = true;
                }

                auto it_y = cmap_equal_->find(item_y);
                if (it_y != cmap_equal_->end()) {
                    auto const& cmap_y_equal = it_y->second;
                    auto it_pair_reverse = cmap_y_equal.find(item_x);
                    if (it_pair_reverse != cmap_y_equal.end()) {
                        MinSupport count2 = it_pair_reverse->second;
                        do_not_explore_yx = (count2 < minsup_absolute_);
                    } else {
                        do_not_explore_yx = true;
                    }
                } else {
                    do_not_explore_yx = true;
                }
            } else {
                do_not_explore_xy = true;
                do_not_explore_yx = true;
            }

            if (cmap_after_) {
                if (cmap_x_after) {
                    auto it_pair = cmap_x_after->find(item_y);
                    if (it_pair != cmap_x_after->end()) {
                        MinSupport count1 = it_pair->second;
                        do_not_explore_x_y = (count1 < minsup_absolute_);
                    } else {
                        do_not_explore_x_y = true;
                    }
                } else {
                    do_not_explore_x_y = true;
                }

                auto it_y = cmap_after_->find(item_y);
                if (it_y != cmap_after_->end()) {
                    auto const& cmap_y_after = it_y->second;
                    auto it_pair_reverse = cmap_y_after.find(item_x);
                    if (it_pair_reverse != cmap_y_after.end()) {
                        MinSupport count2 = it_pair_reverse->second;
                        do_not_explore_y_x = (count2 < minsup_absolute_);
                    } else {
                        do_not_explore_y_x = true;
                    }
                } else {
                    do_not_explore_y_x = true;
                }
            } else {
                do_not_explore_x_y = true;
                do_not_explore_y_x = true;
            }

            if (do_not_explore_xy && do_not_explore_yx && do_not_explore_x_y &&
                do_not_explore_y_x) {
                continue;
            }

            auto extensions = Generator(child_x.GetClassIdentifier(), child_y.GetClassIdentifier(),
                                        minsup_absolute_, do_not_explore_xy, do_not_explore_yx,
                                        do_not_explore_x_y, do_not_explore_y_x);

            for (auto& extension : extensions) {
                auto new_id_list = Join(extension, child_x, child_y, minsup_absolute_);

                if (new_id_list && (new_id_list->GetSupport() >= minsup_absolute_)) {
                    any_pattern_created = true;
                    new_id_list->SetAppearingInPattern(extension);

                    Pattern const* saved_ptr = SavePattern(std::move(extension));

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

std::vector<Pattern> CMSpade::Generator(Pattern const* pattern1, Pattern const* pattern2,
                                        MinSupport min_sup, bool do_not_explore_xy,
                                        bool do_not_explore_yx, bool do_not_explore_x_y,
                                        bool do_not_explore_y_x) {
    std::vector<Pattern> candidates;

    boost::dynamic_bitset<> const& appearing1 = pattern1->GetAppearing();
    boost::dynamic_bitset<> const& appearing2 = pattern2->GetAppearing();

    boost::dynamic_bitset<> join_bitset = appearing1 & appearing2;
    if (join_bitset.count() < min_sup) {
        return candidates;
    }

    auto const& last_element_pattern1 = pattern1->GetLastElement();
    auto const& last_element_pattern2 = pattern2->GetLastElement();

    bool is_equal1 = last_element_pattern1.HaveEqualRelation();
    bool is_equal2 = last_element_pattern2.HaveEqualRelation();

    Item const& item1 = last_element_pattern1.GetItem();
    Item const& item2 = last_element_pattern2.GetItem();

    int pattern_compare = pattern1->CompareTo(*pattern2);

    if ((!is_equal1) && (!is_equal2)) {
        if (item1 != item2) {
            if (pattern_compare < 0 && !do_not_explore_xy) {
                Pattern candidate = pattern1->Clone();
                candidate.Add(ItemAbstractionPair(item2, true));
                candidates.emplace_back(std::move(candidate));
            } else if (!do_not_explore_yx) {
                Pattern candidate = pattern2->Clone();
                candidate.Add(ItemAbstractionPair(item1, true));
                candidates.emplace_back(std::move(candidate));
            }

            if (!do_not_explore_y_x) {
                Pattern candidate = pattern2->Clone();
                candidate.Add(last_element_pattern1);
                candidates.emplace_back(std::move(candidate));
            }
        }

        if (!do_not_explore_x_y) {
            Pattern candidate = pattern1->Clone();
            candidate.Add(last_element_pattern2);
            candidates.emplace_back(std::move(candidate));
        }
    } else if (is_equal1 && is_equal2) {
        if (pattern_compare < 0 && !do_not_explore_xy) {
            Pattern candidate = pattern1->Clone();
            candidate.Add(ItemAbstractionPair(item2, true));
            candidates.emplace_back(std::move(candidate));
        } else if (pattern_compare > 0 && !do_not_explore_yx) {
            Pattern candidate = pattern2->Clone();
            candidate.Add(ItemAbstractionPair(item1, true));
            candidates.emplace_back(std::move(candidate));
        }
    } else {
        if (is_equal1 && !do_not_explore_x_y) {
            Pattern candidate = pattern1->Clone();
            candidate.Add(last_element_pattern2);
            candidates.emplace_back(std::move(candidate));
        } else if (!do_not_explore_y_x) {
            Pattern candidate = pattern2->Clone();
            candidate.Add(last_element_pattern1);
            candidates.emplace_back(std::move(candidate));
        }
    }

    return candidates;
}

std::optional<IdList> CMSpade::Join(Pattern const& extension, EquivalenceClass const& class_i,
                                    EquivalenceClass const& class_j, MinSupport min_sup) {
    auto const& id_list_class_i = class_i.GetIdList();
    auto const& id_list_class_j = class_j.GetIdList();

    auto const& penultimate_element = extension.GetPenultimateElement();
    auto const& last_element = extension.GetLastElement();
    auto const& last_element_from_class_j = class_j.GetClassIdentifier()->GetLastElement();

    if (last_element.HaveEqualRelation()) {
        if (!(penultimate_element.Equals(last_element))) {
            return id_list_class_i.Join(id_list_class_j, true, min_sup);
        }
    } else {
        if (last_element_from_class_j.Equals(last_element)) {
            return id_list_class_i.Join(id_list_class_j, false, min_sup);
        } else {
            return id_list_class_j.Join(id_list_class_i, false, min_sup);
        }
    }

    return std::nullopt;
}

void CMSpade::BuildFrequentItems() {
    std::unordered_map<Item, std::pair<Pattern, IdList>> eq_classes;

    std::shared_ptr<std::vector<ItemsetId> const> const_itemset_counts = itemset_counts_;

    for (SequenceId sid = 0; sid < sequences_.size(); ++sid) {
        auto const& sequence = sequences_[sid];
        auto const& itemsets = sequence->GetItemsets();

        for (ItemsetId tid = 0; tid < itemsets.size(); ++tid) {
            auto const& itemset = itemsets[tid];

            for (auto const& item : itemset->GetItems()) {
                auto it = eq_classes.find(item);
                if (it == eq_classes.end()) {
                    IdList id_list(const_itemset_counts);

                    ItemAbstractionPair pair(item, false);
                    Pattern pattern(pair);

                    id_list.RegisterBit(sid, tid);
                    eq_classes.emplace(item,
                                       std::make_pair(std::move(pattern), std::move(id_list)));
                } else {
                    it->second.second.RegisterBit(sid, tid);
                }
            }
        }
    }

    FilterByMinSupport(eq_classes);

    for (auto& [item, pair] : eq_classes) {
        pair.second.SetAppearingInPattern(pair.first);
    }

    frequent_items_size_1_ = std::move(eq_classes);
}

void CMSpade::BuildCMap() {
    if (cmap_built_) {
        return;
    }

    cmap_after_ = std::make_shared<CMAP>();
    cmap_equal_ = std::make_shared<CMAP>();

    for (auto const& sequence : sequences_) {
        std::unordered_set<Int> already_seen_after;
        std::unordered_map<Int, std::unordered_set<Int>> already_seen_equal;

        auto const& itemsets = sequence->GetItemsets();
        for (size_t i = 0; i < itemsets.size(); i++) {
            auto const& itemset = itemsets[i];

            for (size_t j = 0; j < itemset->Size(); j++) {
                Int item_a = itemset->GetItem(j).GetId();

                auto& equal_set = already_seen_equal[item_a];
                for (size_t k = j + 1; k < itemset->Size(); k++) {
                    Int item_b = itemset->GetItem(k).GetId();

                    if (equal_set.find(item_b) == equal_set.end()) {
                        (*cmap_equal_)[item_a][item_b]++;
                        equal_set.insert(item_b);
                    }
                }

                if (already_seen_after.find(item_a) == already_seen_after.end()) {
                    std::unordered_set<Int> seen_b_after;
                    for (size_t k = i + 1; k < itemsets.size(); k++) {
                        auto const& next_itemset = itemsets[k];

                        for (size_t m = 0; m < next_itemset->Size(); m++) {
                            Int item_b = next_itemset->GetItem(m).GetId();

                            if (seen_b_after.find(item_b) == seen_b_after.end()) {
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
              [](std::pair<Pattern, IdList> const& a, std::pair<Pattern, IdList> const& b) {
                  return a.first.CompareTo(b.first) < 0;
              });
    return result;
}

void CMSpade::FilterByMinSupport(std::unordered_map<Item, std::pair<Pattern, IdList>>& eq_classes) {
    for (auto it = eq_classes.begin(); it != eq_classes.end();) {
        if ((it->second.second.GetSupport()) < minsup_absolute_) {
            it = eq_classes.erase(it);
        } else {
            ++it;
        }
    }
}

void CMSpade::RemoveInfrequentItemsFromSequences() {
    for (auto& sequence : sequences_) {
        for (size_t i = 0; i < sequence->Size(); i++) {
            auto& itemset = sequence->GetItemsets()[i];

            for (size_t j = 0; j < itemset->Size(); j++) {
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
                           [](std::unique_ptr<Sequence> const& seq) { return seq->Size() == 0; }),
            sequences_.end());
}
}  // namespace algos::cmspade