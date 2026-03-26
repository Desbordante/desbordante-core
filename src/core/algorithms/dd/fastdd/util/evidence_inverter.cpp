#include "core/algorithms/dd/fastdd/util/evidence_inverter.h"

#include <algorithm>
#include <utility>

#include "core/algorithms/dd/fastdd/trees/translating_tree_search.h"
#include "core/algorithms/dd/fastdd/trees/tree_search.h"
#include "core/util/logger.h"

namespace algos::dd {

std::vector<std::size_t> EvidenceInverter::CountDFFrequencies() const {
    std::vector<std::size_t> freqs(df_num_);
    for (auto& bitset : bitsets_) {
        for (std::size_t index = bitset.find_first(); index != boost::dynamic_bitset<>::npos;
             index = bitset.find_next(index)) {
            ++freqs[index];
        }
    }

    return freqs;
}

std::vector<boost::dynamic_bitset<>> EvidenceInverter::GetCovers() const {
    TranslatingTreeSearch positive_cover(CountDFFrequencies(), column_to_dif_funcs_);
    LOG_TRACE("Built translating tree search");

    std::vector<boost::dynamic_bitset<>> negative_cover(bitsets_.begin(), bitsets_.end());
    LOG_TRACE("Created negative cover: {}", bitsets_.size());
    negative_cover = MinimizeDifferentialSet(std::move(negative_cover));
    LOG_TRACE("Minimized differential set: {}", negative_cover.size());
    positive_cover.Insert(boost::dynamic_bitset<>(df_num_));
    LOG_TRACE("Inserted");
    std::ranges::sort(negative_cover, [&positive_cover](boost::dynamic_bitset<> const& a,
                                                        boost::dynamic_bitset<> const& b) {
        return positive_cover.Compare(a, b);
    });

    for (auto const& invalid_bitset : negative_cover) {
        positive_cover.HandleInvalid(invalid_bitset);
    }
    LOG_TRACE("Handled invalid");

    std::vector<boost::dynamic_bitset<>> covers(positive_cover.begin(), positive_cover.end());

    return covers;
}

std::vector<boost::dynamic_bitset<>> EvidenceInverter::MinimizeDifferentialSet(
        std::vector<boost::dynamic_bitset<>> bitsets) const {
    LOG_TRACE("Start minimize");
    std::ranges::sort(bitsets,
                      [](boost::dynamic_bitset<> const& a, boost::dynamic_bitset<> const& b) {
                          int diff = b.count() - a.count();
                          return diff != 0 ? diff < 0 : b < a;
                      });
    LOG_TRACE("Sorted");

    TreeSearch negative_search;
    LOG_TRACE("Built negative search: {}", bitsets.size());
    std::ranges::for_each(bitsets, [&negative_search](boost::dynamic_bitset<> const& bitset) {
        if (!negative_search.FindSuperSet(bitset)) {
            negative_search.Add(bitset);
        }
    });
    LOG_TRACE("Iterate");
    std::vector<boost::dynamic_bitset<>> remaining_bitsets(negative_search.begin(),
                                                           negative_search.end());
    return remaining_bitsets;
}

}  // namespace algos::dd
