#pragma once

#include "core/algorithms/algorithm.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/util/timed_invoke.h"

#include "model/equivalence_class.h"
#include "model/pattern.h"
#include "model/item_abstraction_pair.h"
#include "model/id_list.h"
#include "parser/cmspade_parser.h"

#include <filesystem>
#include <memory>
#include <vector>
#include <deque>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cmath>
#include <optional>

namespace algos::cmspade{
class CMSpade: public Algorithm{
protected:
    std::vector<std::unique_ptr<Sequence>> sequences_;
    std::shared_ptr<std::vector<ItemsetId>> itemset_counts_;

    std::unordered_map<Item, std::pair<Pattern, IdList>> frequent_items_size_1_;

    std::deque<Pattern> frequent_patterns_;

    bool cmap_built_ = false;
    
    using CMAP = std::unordered_map<Int, std::unordered_map<Int, MinSupport>>;
    std::shared_ptr<CMAP> cmap_after_;
    std::shared_ptr<CMAP> cmap_equal_;

    std::filesystem::path database_path_;

    double minsup_;

    MinSupport minsup_absolute_;

    const Pattern* SavePattern(Pattern&& pattern) {
        frequent_patterns_.push_back(std::move(pattern));
        return &frequent_patterns_.back();
    }

    void Execute(EquivalenceClass& eq_class);

    std::vector<Pattern> Generator(const Pattern* pattern1,
            const Pattern* pattern2, MinSupport min_sup, bool do_not_explore_XY,
            bool do_not_explore_YX, bool do_not_explore_X_Y, bool do_not_explore_Y_X);

    std::optional<IdList> Join(const Pattern& extension, 
                                const EquivalenceClass& class_i, 
                                const EquivalenceClass& class_j, MinSupport min_sup);

    unsigned long long ExecuteInternal();
    void ResetState();
    void LoadDataInternal();

    void RegisterOptions();

    void FilterByMinSupport(std::unordered_map<Item, std::pair<Pattern, IdList>>& eq_classes,
                            MinSupport min_support);

    void BuildFrequentItems(MinSupport min_support);

    void RemoveInfrequentItemsFromSequences();
    void RemoveEmptySequences();

    void BuildCMap();

    std::shared_ptr<const std::vector<ItemsetId>> GetItemsetCounts() const { 
        return itemset_counts_; 
    }
    
    size_t SequenceCount() const { return sequences_.size(); }
    
    const std::vector<std::unique_ptr<Sequence>>& GetSequences() const { return sequences_; } 
    
    std::vector<std::pair<Pattern, IdList>> TakeFrequentItems();


public:
    CMSpade();

    void SearchFrequentPatterns();
    
    const std::deque<Pattern>& GetPatterns() const { return frequent_patterns_; }
};
} // namespace algos::cmspade