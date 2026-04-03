#pragma once

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/dd/fastdd/model/differential_dependency.h"
#include "core/algorithms/dd/fastdd/model/match_df.h"
#include "core/algorithms/dd/fastdd/trees/translating_minimize_tree.h"
#include "core/algorithms/dd/fastdd/util/dif_func_info.h"
#include "core/algorithms/dd/fastdd/util/differential_function_builder.h"

namespace algos::dd {

class HybridEvidenceInverter {
private:
    std::vector<MatchDF> match_dfs_;
    std::vector<std::vector<DifferentialFunction>> dif_funcs_;
    std::vector<boost::dynamic_bitset<>> column_to_dif_funcs_;
    std::shared_ptr<DifFuncInfo const> dif_func_info_;

    std::vector<boost::dynamic_bitset<>> dif_func_to_satisfied_bitsets_;
    std::vector<boost::dynamic_bitset<>> dif_func_to_not_satisfied_bitsets_;

    std::unordered_map<std::size_t, std::shared_ptr<TranslatingMinimizeTree>> minimize_tree_map_;

    void BuildClueIndices();
    std::vector<boost::dynamic_bitset<>> MinimizeDifferentialSet(
            std::vector<boost::dynamic_bitset<>> bitsets) const;
    std::vector<DifferentialDependency> Minimize(std::vector<boost::dynamic_bitset<>> covers,
                                                 std::size_t rhs_column, std::size_t rhs_offset);
    std::vector<DifferentialDependency> RemoveTransitive(
            std::vector<DifferentialDependency> dds) const;

public:
    HybridEvidenceInverter(std::vector<MatchDF> match_dfs,
                           DifferentialFunctionBuilder const& df_builder);

    std::vector<DifferentialDependency> BuildDDs();
};

}  // namespace algos::dd
