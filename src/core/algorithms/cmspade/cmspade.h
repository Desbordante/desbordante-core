#pragma once

#include <algorithm>
#include <cmath>
#include <deque>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/util/timed_invoke.h"
#include "model/equivalence_class.h"
#include "model/id_list.h"
#include "model/item_abstraction_pair.h"
#include "model/pattern.h"
#include "parser/cmspade_parser.h"

namespace algos::cmspade {
class CMSpade : public Algorithm {
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

    Pattern const* SavePattern(Pattern&& pattern) {
        frequent_patterns_.push_back(std::move(pattern));
        return &frequent_patterns_.back();
    }

    void Execute(EquivalenceClass& eq_class);

    std::vector<Pattern> Generator(Pattern const* pattern1, Pattern const* pattern2,
                                   MinSupport min_sup, bool do_not_explore_xy,
                                   bool do_not_explore_yx, bool do_not_explore_x_y,
                                   bool do_not_explore_y_x);

    std::optional<IdList> Join(Pattern const& extension, EquivalenceClass const& class_i,
                               EquivalenceClass const& class_j, MinSupport min_sup);

    unsigned long long ExecuteInternal();
    void ResetState();
    void LoadDataInternal();

    void RegisterOptions();

    void FilterByMinSupport(std::unordered_map<Item, std::pair<Pattern, IdList>>& eq_classes);

    void BuildFrequentItems();

    void RemoveInfrequentItemsFromSequences();
    void RemoveEmptySequences();

    void BuildCMap();

    std::shared_ptr<std::vector<ItemsetId> const> GetItemsetCounts() const {
        return itemset_counts_;
    }

    size_t SequenceCount() const {
        return sequences_.size();
    }

    std::vector<std::unique_ptr<Sequence>> const& GetSequences() const {
        return sequences_;
    }

    std::vector<std::pair<Pattern, IdList>> TakeFrequentItems();

public:
    CMSpade();

    void SearchFrequentPatterns();

    std::deque<Pattern> const& GetPatterns() const {
        return frequent_patterns_;
    }
};
}  // namespace algos::cmspade
