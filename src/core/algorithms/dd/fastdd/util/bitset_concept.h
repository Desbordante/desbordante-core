#pragma once

#include <concepts>
#include <cstddef>

#include <boost/dynamic_bitset.hpp>

namespace algos::dd {

template <typename BitsetType>
concept BoostDynamicBitsetCompatible =
        std::regular<BitsetType> && std::constructible_from<std::size_t> &&
        std::constructible_from<boost::dynamic_bitset<>> &&
        requires(BitsetType bs1, BitsetType const bs2, std::size_t index) {
            { bs2[index] } -> std::same_as<bool>;
            { bs1.set() } -> std::same_as<BitsetType&>;
            { bs1.flip() } -> std::same_as<BitsetType&>;
            { bs1.set(index) } -> std::same_as<BitsetType&>;
            { bs1.set(index, false) } -> std::same_as<BitsetType&>;
            { bs1.reset(index) } -> std::same_as<BitsetType&>;
            { bs1.none() } -> std::same_as<bool>;
            { bs1.size() } -> std::same_as<std::size_t>;
            { bs1.count() } -> std::same_as<std::size_t>;
            { bs1 < bs2 } -> std::same_as<bool>;
            { bs1 &= bs2 } -> std::same_as<BitsetType&>;
            { bs1 |= bs2 } -> std::same_as<BitsetType&>;
            { bs1 -= bs2 } -> std::same_as<BitsetType&>;
            { bs1.is_subset_of(bs2) } -> std::same_as<bool>;
            { bs1 & bs2 } -> std::same_as<BitsetType>;
            { bs1 - bs2 } -> std::same_as<BitsetType>;
            { bs1.find_first() } -> std::same_as<std::size_t>;
            { bs1.find_next(index) } -> std::same_as<std::size_t>;
            { BitsetType::npos } -> std::convertible_to<std::size_t>;
        };

}  // namespace algos::dd
