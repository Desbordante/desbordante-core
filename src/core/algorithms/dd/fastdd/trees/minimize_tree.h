#pragma once

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <vector>

#include <boost/dynamic_bitset.hpp>

namespace algos::dd {

class MinimizeTree {
private:
    std::unordered_map<std::size_t, std::unique_ptr<MinimizeTree>> children_;
    std::vector<boost::dynamic_bitset<>> bitsets_;

    std::optional<boost::dynamic_bitset<>> FindSuperset(boost::dynamic_bitset<> const& candidate,
                                                        std::vector<std::size_t> const& nodes,
                                                        std::size_t index);
    void AddImpl(boost::dynamic_bitset<> const& candidate, std::vector<std::size_t> const& nodes,
                 std::size_t index);

public:
    std::optional<boost::dynamic_bitset<>> Add(boost::dynamic_bitset<> const& candidate,
                                               std::vector<std::size_t> const& nodes);
};

}  // namespace algos::dd
