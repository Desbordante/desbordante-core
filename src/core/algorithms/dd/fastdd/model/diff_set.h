#pragma once

#include <cstddef>
#include <vector>

#include <boost/dynamic_bitset.hpp>
#include <boost/unordered/unordered_flat_set.hpp>

#include "core/algorithms/dd/fastdd/model/match_df.h"
#include "core/algorithms/dd/fastdd/util/differential_function_builder.h"
#include "core/algorithms/dd/fastdd/util/isn_info.h"

namespace algos::dd {

class DiffSet {
private:
    std::vector<MatchDF> match_dfs_;

    std::vector<std::vector<boost::dynamic_bitset<>>> offset_to_predicates_;
    std::shared_ptr<ISNInfo> isn_info_;
    std::size_t const bitset_size_;

public:
    DiffSet(DifferentialFunctionBuilder const& df_builder, std::shared_ptr<ISNInfo> isn_info)
        : isn_info_(isn_info), bitset_size_(df_builder.GetDifFuncNum()) {
        model::ColumnIndex const num_columns = df_builder.GetDifFuncsSize();
        offset_to_predicates_.reserve(num_columns);
        for (model::ColumnIndex column_index = 0; column_index != num_columns; ++column_index) {
            offset_to_predicates_.push_back(df_builder.GetSatisfiedDFs(column_index));
        }
    }

    void Build(boost::unordered::unordered_flat_set<std::size_t> const& clue_set) {
        match_dfs_.reserve(clue_set.size());
        for (std::size_t clue : clue_set) {
            match_dfs_.emplace_back(clue, bitset_size_, offset_to_predicates_,
                                    isn_info_->GetBases());
        }
    }

    std::vector<MatchDF> GetMatchDFs() const noexcept {
        return match_dfs_;
    }
};

}  // namespace algos::dd
