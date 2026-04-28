#pragma once

#include <array>
#include <concepts>
#include <stdexcept>
#include <utility>

namespace util {
/**
 * @brief Provides a constexpr immutable map-like lookup structure based on std::array.

 * @tparam Key The key type. Must be comparable with `operator==`.
 * @tparam Value The value type.
 * @tparam N The number of key-value pairs in the map.
 */
template <std::equality_comparable Key, typename Value, std::size_t N>
class StaticMap {
public:
    using Pair = std::pair<Key, Value>;
    using Storage = std::array<Pair, N>;

    consteval explicit StaticMap(Storage data) : data_(data) {
        if (HasDuplicateKeys(data_)) {
            throw "StaticMap contains duplicate keys";
        }
    }

    [[nodiscard]] constexpr Value const& At(Key const& key) const {
        if (auto const* value = Find(key)) {
            return *value;
        }
        throw std::out_of_range("StaticMap::At: key not found");
    }

    [[nodiscard]] constexpr Value const* Find(Key const& key) const {
        for (auto const& [candidate_key, candidate_value] : data_) {
            if (candidate_key == key) {
                return &candidate_value;
            }
        }
        return nullptr;
    }

    [[nodiscard]] constexpr auto begin() const {
        return data_.begin();
    }

    [[nodiscard]] constexpr auto end() const {
        return data_.end();
    }

    [[nodiscard]] constexpr auto size() const {
        return N;
    }

    [[nodiscard]] constexpr bool empty() const {
        return N == 0;
    }

private:
    static consteval bool HasDuplicateKeys(Storage const& data) {
        for (std::size_t i = 0; i < N; ++i) {
            for (std::size_t j = i + 1; j < N; ++j) {
                if (data[i].first == data[j].first) {
                    return true;
                }
            }
        }
        return false;
    }

    Storage data_;
};
}  // namespace util
