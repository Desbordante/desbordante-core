#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

namespace algos::dd {

class EvidenceInverter {
private:
    std::vector<boost::dynamic_bitset<>> bitsets_;
    std::size_t df_num_;
    std::vector<boost::dynamic_bitset<>> column_to_dif_funcs_;

    std::vector<std::size_t> CountDFFrequencies() const;

public:
    explicit EvidenceInverter(std::vector<boost::dynamic_bitset<>> bitsets, std::size_t df_num,
                              std::vector<boost::dynamic_bitset<>> const& column_to_dif_funcs,
                              std::size_t cur_column_index)
        : bitsets_(std::move(bitsets)), df_num_(df_num) {
        column_to_dif_funcs_ = std::vector<boost::dynamic_bitset<>>();
        column_to_dif_funcs_.reserve(column_to_dif_funcs.size() - 1);
        for (std::size_t i = 0; i != column_to_dif_funcs.size(); ++i) {
            if (i == cur_column_index) {
                continue;
            }
            column_to_dif_funcs_.push_back(column_to_dif_funcs[i]);
        }
    }

    std::vector<boost::dynamic_bitset<>> GetCovers();
};

}  // namespace algos::dd
