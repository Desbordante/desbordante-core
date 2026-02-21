#pragma once

#include <cstddef>

#include <boost/dynamic_bitset.hpp>

namespace algos::dd {

class MatchDF {
private:
    std::size_t count_;
    boost::dynamic_bitset<> bitset_;

public:
    MatchDF(std::size_t const clue, std::size_t const count, std::size_t const bitset_size,
            std::vector<std::vector<boost::dynamic_bitset<>>> const& count_to_predicates,
            std::vector<std::size_t> const& bases)
        : count_(count) {
        bitset_ = boost::dynamic_bitset<>(bitset_size);
        std::size_t cur_clue = clue;
        for (std::size_t i = count_to_predicates.size(); i != 0; --i) {
            std::size_t const offset = cur_clue / bases[i - 1];
            cur_clue %= bases[i - 1];
            boost::dynamic_bitset<> cur_bitset = count_to_predicates[i - 1][offset];
            bitset_.operator|=(cur_bitset);
        }
    }

    boost::dynamic_bitset<> const& GetBitset() const noexcept {
        return bitset_;
    }
};

}  // namespace algos::dd
