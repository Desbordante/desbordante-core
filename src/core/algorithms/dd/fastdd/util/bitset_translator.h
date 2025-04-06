#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

namespace algos::dd {

class BitsetTranslator {
private:
    std::vector<std::size_t> indices_;

public:
    BitsetTranslator() = default;

    BitsetTranslator(std::vector<std::size_t> indices) : indices_(std::move(indices)) {}

    boost::dynamic_bitset<> Transform(boost::dynamic_bitset<> const& bitset) const {
        std::size_t const bitset_size = bitset.size();
        boost::dynamic_bitset<> transformed_bitset(bitset_size);
        for (std::size_t i = 0; i != indices_.size(); ++i) {
            if (indices_[i] < bitset_size && bitset[indices_[i]]) {
                transformed_bitset.set(i);
            }
        }

        return transformed_bitset;
    }

    boost::dynamic_bitset<> Retransform(boost::dynamic_bitset<> const& bitset) const {
        boost::dynamic_bitset<> retransformed_bitset(bitset.size());
        for (std::size_t index = bitset.find_first(); index != boost::dynamic_bitset<>::npos;
             index = bitset.find_next(index)) {
            retransformed_bitset.set(indices_[index]);
        }

        return retransformed_bitset;
    }
};

}  // namespace algos::dd
