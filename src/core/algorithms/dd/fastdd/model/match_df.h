#pragma once

#include <cstddef>

#include <boost/dynamic_bitset.hpp>

namespace algos::dd {

class MatchDF {
private:
    boost::dynamic_bitset<> bitset_;

public:
    MatchDF(std::size_t clue, std::size_t const bitset_size,
            std::vector<std::vector<boost::dynamic_bitset<>>> const& count_to_predicates,
            std::vector<std::size_t> const& bases)
        : bitset_(bitset_size) {
        for (std::size_t i = count_to_predicates.size(); i != 0; --i) {
            std::size_t const offset = clue / bases[i - 1];
            clue %= bases[i - 1];
            boost::dynamic_bitset<> cur_bitset = count_to_predicates[i - 1][offset];
            bitset_.operator|=(cur_bitset);
        }
    }

    boost::dynamic_bitset<> const& GetBitset() const noexcept {
        return bitset_;
    }
};

}  // namespace algos::dd
