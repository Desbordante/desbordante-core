#pragma once

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/dd/fastdd/trees/translating_tree_search.h"
#include "core/algorithms/dd/fastdd/util/bitset_concept.h"
#include "core/util/logger.h"

namespace algos::dd {

template <BoostDynamicBitsetCompatible Bitset>
class EvidenceInverter {
private:
    std::vector<Bitset> bitsets_;
    std::size_t df_num_;
    std::vector<Bitset> column_to_dif_funcs_;

    std::vector<std::size_t> CountDFFrequencies() const {
        std::vector<std::size_t> freqs(df_num_);
        for (auto& bitset : bitsets_) {
            for (std::size_t index = bitset.find_first(); index != Bitset::npos;
                 index = bitset.find_next(index)) {
                ++freqs[index];
            }
        }

        return freqs;
    }

public:
    explicit EvidenceInverter(std::vector<Bitset> bitsets, std::size_t df_num,
                              std::vector<Bitset> const& column_to_dif_funcs,
                              std::size_t cur_column_index)
        : bitsets_(std::move(bitsets)), df_num_(df_num), column_to_dif_funcs_() {
        column_to_dif_funcs_.reserve(column_to_dif_funcs.size() - 1);
        for (std::size_t i = 0; i != column_to_dif_funcs.size(); ++i) {
            if (i == cur_column_index) {
                continue;
            }
            column_to_dif_funcs_.push_back(column_to_dif_funcs[i]);
        }
    }

    std::vector<Bitset> GetCovers() {
        TranslatingTreeSearch<Bitset> positive_cover(CountDFFrequencies(), column_to_dif_funcs_);
        LOG_TRACE("Built translating tree search");

        positive_cover.Insert(Bitset(df_num_));
        LOG_TRACE("Inserted");
        std::ranges::sort(bitsets_, [&positive_cover](Bitset const& a, Bitset const& b) {
            return positive_cover.Compare(a, b);
        });

        for (auto const& invalid_bitset : bitsets_) {
            positive_cover.HandleInvalid(invalid_bitset);
        }
        LOG_TRACE("Handled invalid");

        std::vector<Bitset> covers;
        positive_cover.ForEach([&covers](Bitset const& bitset) { covers.push_back(bitset); });

        return covers;
    }
};

}  // namespace algos::dd
