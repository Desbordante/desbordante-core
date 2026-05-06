#pragma once

#include <bitset>
#include <cstdint>
#include <functional>
#include <variant>

#include <boost/dynamic_bitset.hpp>

namespace algos::dd {

using SmallBits = uint64_t;
using MediumBits = std::bitset<256>;
using DynamicBits = boost::dynamic_bitset<>;

constexpr int NPOS = -1;

enum class BitsetType { Small, Medium, Dynamic };

struct Bitset {
    BitsetType type;
    std::variant<SmallBits, MediumBits, DynamicBits> data;
    std::size_t size;
};

template <class F>
inline auto visit_bits(Bitset& b, F&& f) {
    return std::visit(std::forward<F>(f), b.data);
}

template <class F>
inline auto visit_bits(Bitset const& b, F&& f) {
    return std::visit(std::forward<F>(f), b.data);
}

inline Bitset make_bitset(std::size_t size) {
    if (size <= 64) {
        return Bitset{BitsetType::Small, SmallBits(0), size};
    }

    if (size <= 256) {
        return Bitset{BitsetType::Medium, MediumBits(), size};
    }

    return Bitset{BitsetType::Dynamic, DynamicBits(size), size};
}

inline void set(Bitset& b, std::size_t i) {
    visit_bits(b, [&](auto& x) {
        using T = std::decay_t<decltype(x)>;

        if constexpr (std::is_same_v<T, SmallBits>) {
            x |= (1ULL << i);
        } else {
            x.set(i);
        }
    });
}

inline void reset(Bitset& b, std::size_t i) {
    visit_bits(b, [&](auto& x) {
        using T = std::decay_t<decltype(x)>;

        if constexpr (std::is_same_v<T, SmallBits>) {
            x &= ~(1ULL << i);
        } else {
            x.reset(i);
        }
    });
}

inline Bitset bit_and(Bitset const& a, Bitset const& b) {
    Bitset res = a;

    visit_bits(res, [&](auto& x) {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, SmallBits>) {
            x &= std::get<SmallBits>(b.data);
        } else {
            x &= std::get<T>(b.data);
        }
    });

    return res;
}

inline Bitset bit_or(Bitset const& a, Bitset const& b) {
    Bitset res = a;

    visit_bits(res, [&](auto& x) {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, SmallBits>) {
            x |= std::get<SmallBits>(b.data);
        } else {
            x |= std::get<T>(b.data);
        }
    });

    return res;
}

inline Bitset bit_xor(Bitset const& a, Bitset const& b) {
    Bitset res = a;

    visit_bits(res, [&](auto& x) {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, SmallBits>) {
            x ^= std::get<SmallBits>(b.data);
        } else {
            x ^= std::get<T>(b.data);
        }
    });

    return res;
}

inline Bitset bit_not(Bitset const& b) {
    Bitset res = b;

    visit_bits(res, [&](auto& x) {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, SmallBits>) {
            x = ~x;
            if (b.size < 64) {
                x &= ((1ULL << b.size) - 1);
            }
        } else if constexpr (std::is_same_v<T, MediumBits>) {
            x = ~x;
            for (std::size_t i = b.size; i < 256; ++i) {
                x.reset(i);
            }
        } else {
            x.flip();
        }
    });

    return res;
}

inline std::size_t bit_count(Bitset const& b) {
    return visit_bits(b, [&](auto const& x) -> std::size_t {
        using T = std::decay_t<decltype(x)>;

        if constexpr (std::is_same_v<T, SmallBits>) {
            return __builtin_popcountll(x);
        } else {
            return x.count();
        }
    });
}

inline bool bit_none(Bitset const& b) {
    return visit_bits(b, [&](auto const& x) -> bool {
        using T = std::decay_t<decltype(x)>;

        if constexpr (std::is_same_v<T, SmallBits>) {
            return x == 0;
        } else {
            return x.none();
        }
    });
}

inline int find_first(Bitset const& b) {
    return visit_bits(b, [&](auto const& x) -> int {
        using T = std::decay_t<decltype(x)>;

        if constexpr (std::is_same_v<T, SmallBits>) {
            return x ? __builtin_ctzll(x) : NPOS;
        } else {
            for (int i = 0; i < (int)b.size; ++i)
                if (x.test(i)) return i;
            return NPOS;
        }
    });
}

inline int find_next(Bitset const& b, int pos) {
    return visit_bits(b, [&](auto const& x) -> int {
        using T = std::decay_t<decltype(x)>;

        if constexpr (std::is_same_v<T, SmallBits>) {
            SmallBits tmp = x & (~0ULL << (pos + 1));
            return tmp ? __builtin_ctzll(tmp) : NPOS;
        } else {
            for (int i = pos + 1; i < (int)b.size; ++i)
                if (x.test(i)) return i;
            return NPOS;
        }
    });
}

inline void resize(Bitset& b, std::size_t new_size) {
    b.size = new_size;

    visit_bits(b, [&](auto& x) {
        using T = std::decay_t<decltype(x)>;

        if constexpr (std::is_same_v<T, DynamicBits>) {
            x.resize(new_size);
        }
    });
}

inline bool operator==(Bitset const& a, Bitset const& b) {
    if (a.type != b.type || a.size != b.size) return false;

    return visit_bits(a, [&](auto const& x) {
        using T = std::decay_t<decltype(x)>;
        return x == std::get<T>(b.data);
    });
}

struct BitsetHash {
    std::size_t operator()(Bitset const& b) const {
        return visit_bits(b, [&](auto const& x) -> std::size_t {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, SmallBits>) {
                return std::hash<uint64_t>()(x);
            } else if constexpr (std::is_same_v<T, MediumBits>) {
                std::size_t h = 0;
                constexpr size_t chunks = 256 / 64;
                for (size_t c = 0; c < chunks; ++c) {
                    uint64_t block = 0;
                    for (size_t i = 0; i < 64; ++i) {
                        if (x.test(c * 64 + i)) {
                            block |= (1ULL << i);
                        }
                    }
                    h ^= std::hash<uint64_t>()(block + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
                }
                return h;
            }

            else {
                std::size_t h = 0;
                size_t const n = x.size();
                for (size_t i = 0; i < n; ++i) {
                    if (x.test(i)) {
                        h ^= std::hash<size_t>()(i + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
                    }
                }
                return h;
            }
        });
    }
};

}  // namespace algos::dd