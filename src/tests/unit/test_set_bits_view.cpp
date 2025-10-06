#include <algorithm>
#include <cstddef>
#include <numeric>
#include <vector>

#include <boost/dynamic_bitset.hpp>
#include <gtest/gtest.h>

#include "bitset_utils.h"
#include "model/types/bitset.h"

namespace tests {

template <std::size_t N>
model::Bitset<N> MakeFixedBitset(std::vector<std::size_t> const& indices) {
    model::Bitset<N> bs;
    for (auto i : indices) {
        bs.set(i);
    }
    return bs;
}

template <std::size_t S>
struct FixedParam {
    static constexpr std::size_t kSize = S;
    using Bitset = model::Bitset<S>;
};

template <class Param>
class FixedSetBitsViewTest : public ::testing::Test {};

TYPED_TEST_SUITE_P(FixedSetBitsViewTest);

TYPED_TEST_P(FixedSetBitsViewTest, EmptyHasNoIteration) {
    using P = TypeParam;
    using B = typename P::Bitset;

    B bs;
    auto view = util::SetBits(bs);
    EXPECT_TRUE(view.begin() == view.end());

    std::vector<std::size_t> seen;
    for (auto i : view) {
        seen.push_back(i);
    }
    EXPECT_TRUE(seen.empty());
}

TYPED_TEST_P(FixedSetBitsViewTest, SingleBitYieldsThatIndex) {
    using P = TypeParam;
    constexpr auto n = P::kSize;
    if constexpr (n == 0) {
        GTEST_SKIP() << "Size is zero.";
    }

    std::size_t const p = n / 2;
    auto bs = MakeFixedBitset<n>({p});

    std::vector<std::size_t> seen;
    for (auto i : util::SetBits(bs)) {
        seen.push_back(i);
    }
    ASSERT_EQ(seen.size(), 1u);
    EXPECT_EQ(seen[0], p);
}

TYPED_TEST_P(FixedSetBitsViewTest, MultipleBitsAreAscendingAndUnique) {
    using P = TypeParam;
    constexpr auto n = P::kSize;
    if constexpr (n < 5) {
        GTEST_SKIP() << "Need at least 5 bits.";
    }

    std::vector<std::size_t> idx = {0u, 2u, n - 1, 3u, 2u};
    auto bs = MakeFixedBitset<n>(idx);

    std::vector<std::size_t> seen;
    for (auto i : util::SetBits(bs)) {
        seen.push_back(i);
    }

    auto exp = idx;
    std::ranges::sort(exp);
    exp.erase(std::unique(exp.begin(), exp.end()), exp.end());
    EXPECT_EQ(seen, exp);
}

TYPED_TEST_P(FixedSetBitsViewTest, AllBitsAndDistanceEqualsCount) {
    using P = TypeParam;
    constexpr auto n = P::kSize;

    std::vector<std::size_t> idx(n);
    std::iota(idx.begin(), idx.end(), 0);
    auto bs = MakeFixedBitset<n>(idx);

    std::vector<std::size_t> seen;
    for (auto i : util::SetBits(bs)) {
        seen.push_back(i);
    }
    EXPECT_EQ(seen, idx);

    EXPECT_EQ(std::ranges::distance(util::SetBits(bs)), static_cast<std::ptrdiff_t>(bs.count()));
}

TYPED_TEST_P(FixedSetBitsViewTest, AlternatingEvenBits) {
    using P = TypeParam;
    constexpr auto n = P::kSize;

    std::vector<std::size_t> idx;
    for (std::size_t i = 0; i < n; i += 2) {
        idx.push_back(i);
    }

    auto bs = MakeFixedBitset<n>(idx);

    std::vector<std::size_t> seen;
    for (auto i : util::SetBits(bs)) {
        seen.push_back(i);
    }
    EXPECT_EQ(seen, idx);
}

TYPED_TEST_P(FixedSetBitsViewTest, IteratorPrePostIncrementAndEquality) {
    using P = TypeParam;
    constexpr auto n = P::kSize;
    if constexpr (n < 6) {
        GTEST_SKIP() << "Need at least 6 bits.";
    }

    auto bs = MakeFixedBitset<n>({2, 5});
    auto view = util::SetBits(bs);

    auto it1 = view.begin();
    auto it2 = it1;
    EXPECT_TRUE(it1 == it2);
    EXPECT_EQ(*it1, 2u);

    auto it1_post = it1++;
    EXPECT_EQ(*it1_post, 2u);
    EXPECT_EQ(*it1, 5u);
    EXPECT_NE(it1, it2);

    ++it2;
    EXPECT_EQ(*it2, 5u);
    EXPECT_TRUE(it1 == it2);

    ++it1;
    EXPECT_TRUE(it1 == view.end());
}

TYPED_TEST_P(FixedSetBitsViewTest, WordBoundaries) {
    using P = TypeParam;
    constexpr auto n = P::kSize;
    if constexpr (n < 66) {
        GTEST_SKIP() << "Need at least 66 bits.";
    }

    auto bs = MakeFixedBitset<n>({0, 63, 64, 65, n - 1});
    std::vector<std::size_t> seen;
    for (auto i : util::SetBits(bs)) {
        seen.push_back(i);
    }
    EXPECT_EQ(seen, (std::vector<std::size_t>{0, 63, 64, 65, n - 1}));
}

REGISTER_TYPED_TEST_SUITE_P(FixedSetBitsViewTest, EmptyHasNoIteration, SingleBitYieldsThatIndex,
                            MultipleBitsAreAscendingAndUnique, AllBitsAndDistanceEqualsCount,
                            AlternatingEvenBits, IteratorPrePostIncrementAndEquality,
                            WordBoundaries);

using FixedSizes = ::testing::Types<FixedParam<0>, FixedParam<1>, FixedParam<2>, FixedParam<7>,
                                    FixedParam<8>, FixedParam<63>, FixedParam<64>, FixedParam<65>,
                                    FixedParam<127>, FixedParam<128>, FixedParam<129>>;
INSTANTIATE_TYPED_TEST_SUITE_P(Fixed, FixedSetBitsViewTest, FixedSizes);

template <std::size_t S>
struct BoostParam {
    static constexpr std::size_t kSize = S;
    using Bitset = boost::dynamic_bitset<>;
};

template <class Param>
class BoostSetBitsViewTest : public ::testing::Test {};

TYPED_TEST_SUITE_P(BoostSetBitsViewTest);

TYPED_TEST_P(BoostSetBitsViewTest, EmptyHasNoIteration) {
    using P = TypeParam;
    auto bs = util::IndicesToBitset(std::vector<std::size_t>{}, P::kSize);

    auto view = util::SetBits(bs);
    EXPECT_TRUE(view.begin() == view.end());

    std::vector<std::size_t> seen;
    for (auto i : view) {
        seen.push_back(i);
    }
    EXPECT_TRUE(seen.empty());
}

TYPED_TEST_P(BoostSetBitsViewTest, SingleBitYieldsThatIndex) {
    using P = TypeParam;
    if constexpr (P::kSize == 0) {
        GTEST_SKIP() << "Size is zero.";
    }

    std::size_t const p = P::kSize / 2;
    auto bs = util::IndicesToBitset(std::vector<std::size_t>{p}, P::kSize);

    std::vector<std::size_t> seen;
    for (auto i : util::SetBits(bs)) {
        seen.push_back(i);
    }
    ASSERT_EQ(seen.size(), 1u);
    EXPECT_EQ(seen[0], p);
}

TYPED_TEST_P(BoostSetBitsViewTest, MultipleBitsAreAscendingAndUnique) {
    using P = TypeParam;
    if constexpr (P::kSize < 5) {
        GTEST_SKIP() << "Need at least 5 bits.";
    }

    std::vector<std::size_t> idx = {0u, 2u, P::kSize - 1, 3u, 2u};
    auto bs = util::IndicesToBitset(idx, P::kSize);

    std::vector<std::size_t> seen;
    for (auto i : util::SetBits(bs)) {
        seen.push_back(i);
    }

    auto exp = idx;
    std::ranges::sort(exp);
    exp.erase(std::unique(exp.begin(), exp.end()), exp.end());
    EXPECT_EQ(seen, exp);
}

TYPED_TEST_P(BoostSetBitsViewTest, AllBitsAndDistanceEqualsCount) {
    using P = TypeParam;
    std::vector<std::size_t> idx(P::kSize);
    std::iota(idx.begin(), idx.end(), 0);
    auto bs = util::IndicesToBitset(idx, P::kSize);

    std::vector<std::size_t> seen;
    for (auto i : util::SetBits(bs)) {
        seen.push_back(i);
    }
    EXPECT_EQ(seen, idx);

    EXPECT_EQ(std::ranges::distance(util::SetBits(bs)), static_cast<std::ptrdiff_t>(bs.count()));
}

TYPED_TEST_P(BoostSetBitsViewTest, AlternatingEvenBits) {
    using P = TypeParam;
    std::vector<std::size_t> idx;
    for (std::size_t i = 0; i < P::kSize; i += 2) {
        idx.push_back(i);
    }
    auto bs = util::IndicesToBitset(idx, P::kSize);

    std::vector<std::size_t> seen;
    for (auto i : util::SetBits(bs)) {
        seen.push_back(i);
    }
    EXPECT_EQ(seen, idx);
}

TYPED_TEST_P(BoostSetBitsViewTest, IteratorPrePostIncrementAndEquality) {
    using P = TypeParam;
    if constexpr (P::kSize < 6) {
        GTEST_SKIP() << "Need at least 6 bits.";
    }

    auto bs = util::IndicesToBitset(std::vector<std::size_t>{2, 5}, P::kSize);
    auto view = util::SetBits(bs);

    auto it1 = view.begin();
    auto it2 = it1;
    EXPECT_TRUE(it1 == it2);
    EXPECT_EQ(*it1, 2u);

    auto it1_post = it1++;
    EXPECT_EQ(*it1_post, 2u);
    EXPECT_EQ(*it1, 5u);
    EXPECT_NE(it1, it2);

    ++it2;
    EXPECT_EQ(*it2, 5u);
    EXPECT_TRUE(it1 == it2);

    ++it1;
    EXPECT_TRUE(it1 == view.end());
}

TYPED_TEST_P(BoostSetBitsViewTest, WordBoundaries) {
    using P = TypeParam;
    if constexpr (P::kSize < 66) {
        GTEST_SKIP() << "Need at least 66 bits.";
    }

    auto bs =
            util::IndicesToBitset(std::vector<std::size_t>{0, 63, 64, 65, P::kSize - 1}, P::kSize);

    std::vector<std::size_t> seen;
    for (auto i : util::SetBits(bs)) {
        seen.push_back(i);
    }
    EXPECT_EQ(seen, (std::vector<std::size_t>{0, 63, 64, 65, P::kSize - 1}));
}

REGISTER_TYPED_TEST_SUITE_P(BoostSetBitsViewTest, EmptyHasNoIteration, SingleBitYieldsThatIndex,
                            MultipleBitsAreAscendingAndUnique, AllBitsAndDistanceEqualsCount,
                            AlternatingEvenBits, IteratorPrePostIncrementAndEquality,
                            WordBoundaries);

using BoostSizes =
        ::testing::Types<BoostParam<0>, BoostParam<1>, BoostParam<2>, BoostParam<63>,
                         BoostParam<64>, BoostParam<65>, BoostParam<128>, BoostParam<129>>;
INSTANTIATE_TYPED_TEST_SUITE_P(Boost, BoostSetBitsViewTest, BoostSizes);

}  // namespace tests
