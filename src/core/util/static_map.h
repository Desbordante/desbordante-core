#pragma once

#include <array>
#include <stdexcept>
#include <utility>

namespace util {
/**
 * @brief Provides a constexpr immutable map-like lookup structure based on std::array.

 * @tparam Key The key type. Must be comparable with `operator==`.
 * @tparam Value The value type.
 * @tparam N The number of key-value pairs in the map.
 */
template <typename Key, typename Value, std::size_t N>
struct StaticMap {
    std::array<std::pair<Key, Value>, N> data;

    [[nodiscard]] constexpr Value const& At(Key const& key) const {
        for (auto const& pair : data) {
            if (pair.first == key) {
                return pair.second;
            }
        }
        throw std::out_of_range("StaticMap::at: key not found");
    }

    [[nodiscard]] constexpr Value const* Find(Key const& key) const {
        for (auto const& pair : data) {
            if (pair.first == key) {
                return &pair.second;
            }
        }
        return nullptr;
    }

    [[nodiscard]] constexpr auto begin() const {
        return data.begin();
    }

    [[nodiscard]] constexpr auto end() const {
        return data.end();
    }

    [[nodiscard]] constexpr auto Size() const {
        return N;
    }

    [[nodiscard]] constexpr bool Empty() const {
        return N == 0;
    }
};
}  // namespace util
