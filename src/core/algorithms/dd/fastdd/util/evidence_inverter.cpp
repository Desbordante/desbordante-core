#include "core/algorithms/dd/fastdd/util/evidence_inverter.h"

#include <algorithm>
#include <utility>

#include "core/algorithms/dd/fastdd/trees/translating_tree_search.h"
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

std::vector<boost::dynamic_bitset<>> EvidenceInverter::GetCovers() {
    TranslatingTreeSearch positive_cover(CountDFFrequencies(), column_to_dif_funcs_);
    LOG_TRACE("Built translating tree search");

    positive_cover.Insert(boost::dynamic_bitset<>(df_num_));
    LOG_TRACE("Inserted");
    std::ranges::sort(bitsets_, [&positive_cover](boost::dynamic_bitset<> const& a,
                                                  boost::dynamic_bitset<> const& b) {
        return positive_cover.Compare(a, b);
    });

    for (auto const& invalid_bitset : bitsets_) {
        positive_cover.HandleInvalid(invalid_bitset);
    }
    LOG_TRACE("Handled invalid");

    std::vector<boost::dynamic_bitset<>> covers;
    positive_cover.ForEach(
            [&covers](boost::dynamic_bitset<> const& bitset) { covers.push_back(bitset); });

    return covers;
}

}  // namespace algos::dd
