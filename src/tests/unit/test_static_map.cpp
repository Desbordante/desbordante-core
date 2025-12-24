#include <stdexcept>

#include <gtest/gtest.h>

#include "core/util/static_map.h"

namespace tests {
namespace {

constexpr util::StaticMap<int, int, 3> kMap({{{1, 10}, {2, 20}, {3, 30}}});

static_assert(kMap.size() == 3);
static_assert(!kMap.empty());
static_assert(kMap.Find(2) != nullptr);
static_assert(*kMap.Find(2) == 20);
static_assert(kMap.At(3) == 30);

}  // namespace

TEST(StaticMapTest, FindAndAtWork) {
    auto const* value = kMap.Find(2);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 20);

    EXPECT_EQ(kMap.At(3), 30);
}

TEST(StaticMapTest, MissingKey) {
    constexpr util::StaticMap<int, int, 1> map({{{42, 100}}});

    EXPECT_EQ(map.Find(0), nullptr);
    EXPECT_THROW(static_cast<void>(map.At(0)), std::out_of_range);
}

}  // namespace tests
