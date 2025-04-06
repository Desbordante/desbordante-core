#include "core/algorithms/dd/fastdd/trees/translating_minimize_tree.h"

#include <algorithm>
#include <unordered_set>

namespace algos::dd {

std::vector<std::size_t> TranslatingMinimizeTree::TransformToNodes(
        boost::dynamic_bitset<> const& bitset) const {
    std::vector<std::size_t> nodes;
    nodes.reserve(bitset.count());
    for (std::size_t index = bitset.find_first(); index != boost::dynamic_bitset<>::npos;
         index = bitset.find_next(index)) {
        nodes.push_back(dif_func_info_->dif_func_to_node_id_[index]);
    }

    return nodes;
}

boost::dynamic_bitset<> TranslatingMinimizeTree::TransformToBitset(
        boost::dynamic_bitset<> const& bitset) const {
    boost::dynamic_bitset<> transformed(bitset.size());
    transformed.flip();

    for (std::size_t index = bitset.find_first(); index != boost::dynamic_bitset<>::npos;
         index = bitset.find_next(index)) {
        std::size_t const node_id = dif_func_info_->dif_func_to_node_id_[index];
        bool is_greater = node_id >= dif_func_info_->num_columns_;
        std::size_t const column_index = node_id % dif_func_info_->num_columns_;
        std::size_t df_offset = dif_func_info_->dif_func_to_offset_[index];
        if (is_greater) {
            for (std::size_t i = dif_func_info_->dif_func_nums_[column_index];
                 i != dif_func_info_->dif_func_nums_[column_index] + df_offset + 1; ++i) {
                transformed.set(i, false);
            }
        } else {
            df_offset = dif_func_info_->dif_func_sizes_[column_index] - df_offset - 1;
            for (std::size_t i = dif_func_info_->dif_func_nums_[column_index] + df_offset;
                 i != dif_func_info_->dif_func_nums_[column_index] +
                              dif_func_info_->dif_func_sizes_[column_index];
                 ++i) {
                transformed.set(i, false);
            }
        }
    }

    return transformed;
}

std::unordered_set<boost::dynamic_bitset<>> TranslatingMinimizeTree::Minimize(
        std::vector<boost::dynamic_bitset<>> candidates) {
    std::ranges::sort(candidates,
                      [](boost::dynamic_bitset<> const& a, boost::dynamic_bitset<> const& b) {
                          if (a.count() == b.count()) {
                              return a < b;
                          }
                          return a.count() < b.count();
                      });

    std::unordered_set<boost::dynamic_bitset<>> result;
    for (auto const& candidate : candidates) {
        std::optional<boost::dynamic_bitset<>> superset =
                tree_.Add(TransformToBitset(candidate), TransformToNodes(candidate));
        if (!superset) {
            result.insert(candidate);
        }
    }

    return result;
}

}  // namespace algos::dd
