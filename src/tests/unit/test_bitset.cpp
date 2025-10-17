#include <bitset>
#include <cstddef>
#include <iosfwd>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <version>

#include <easylogging++.h>
#include <gtest/gtest.h>

#include "model/types/bitset.h"

namespace tests {

template <size_t S>
struct BitsetSizeParam {
    using Custom = model::bitset_impl::BitsetImpl<S>;
    using STL = std::bitset<S>;

    static constexpr size_t kSize = S;
};

template <typename T>
    requires requires { T::kSize; }
class BitsetTest : public ::testing::Test {
public:
    void IsEq(T::Custom const& a, T::STL const& b) {
        EXPECT_EQ(a.to_string(), b.to_string());
    }

    std::string GenerateString(char zero = '0', char one = '1') {
        std::ostringstream ss;
        for (size_t i{0}; i < T::kSize; ++i) {
            if (i % 2 == 0) {
                ss << one;
            } else {
                ss << zero;
            }
        }
        return ss.str();
    }
};

TYPED_TEST_SUITE_P(BitsetTest);

// Some initial values for bitsets
static constexpr auto kVal1 = std::numeric_limits<unsigned long long>::max() - 2;
static constexpr auto kVal2 = std::numeric_limits<unsigned long long>::max() - 1;

TYPED_TEST_P(BitsetTest, DefaultConstructor) {
    typename TypeParam::Custom c;
    typename TypeParam::STL s;

    this->IsEq(c, s);
}

TYPED_TEST_P(BitsetTest, ULongConstructor) {
    typename TypeParam::Custom c{100UL};
    typename TypeParam::STL s{100UL};

    this->IsEq(c, s);
}

TYPED_TEST_P(BitsetTest, ULLongConstructor) {
    typename TypeParam::Custom c{kVal1};
    typename TypeParam::STL s{kVal1};

    // Check that bitsets are created properly:
    LOG(INFO) << "Bitsets with value " << kVal1 << ':';
    LOG(INFO) << "\tCustom bitset: " << c;
    LOG(INFO) << "\tSTL bitset: " << s;

    this->IsEq(c, s);
}

TYPED_TEST_P(BitsetTest, StringConstructor) {
    auto str = this->GenerateString();
    typename TypeParam::Custom c{str};
    typename TypeParam::STL s{str};

    this->IsEq(c, s);
}

TYPED_TEST_P(BitsetTest, CharArrayConstructor) {
    auto str = this->GenerateString('_', '*');
    auto const* arr = str.c_str();
    typename TypeParam::Custom c{arr, 8, '_', '*'};
    typename TypeParam::STL s{arr, 8, '_', '*'};

    this->IsEq(c, s);
}

TYPED_TEST_P(BitsetTest, OperatorsEqNeq) {
    typename TypeParam::Custom c1{kVal1};
    typename TypeParam::Custom c2{kVal1};
    typename TypeParam::Custom c3{kVal2};

    EXPECT_TRUE(c1 == c2);
    EXPECT_TRUE(c1 != c3);
}

TYPED_TEST_P(BitsetTest, SubscriptOperatorRead) {
    auto str = this->GenerateString();
    typename TypeParam::Custom c{str};
    typename TypeParam::STL s{str};

    EXPECT_EQ(c[2], s[2]);
}

TYPED_TEST_P(BitsetTest, SubscriptOperatorWrite) {
    auto str = this->GenerateString();
    typename TypeParam::Custom c{str};
    typename TypeParam::STL s{str};

    c[2] = 1;
    s[2] = 1;

    this->IsEq(c, s);
}

TYPED_TEST_P(BitsetTest, Test) {
    typename TypeParam::Custom c{kVal1};
    typename TypeParam::STL s{kVal1};

    EXPECT_EQ(c.test(2), s.test(2));
}

TYPED_TEST_P(BitsetTest, TestInvalidIndex) {
    typename TypeParam::Custom c{kVal1};

    EXPECT_THROW(c.test(TypeParam::kSize), std::out_of_range);
}

TYPED_TEST_P(BitsetTest, All) {
    std::string str1(TypeParam::kSize, '1');
    std::string str2{str1};
    str2.back() = '0';

    typename TypeParam::Custom c1{str1};
    typename TypeParam::Custom c2{str2};

    EXPECT_TRUE(c1.all());
    EXPECT_FALSE(c2.all());
}

TYPED_TEST_P(BitsetTest, Any) {
    std::string str1(TypeParam::kSize, '0');
    std::string str2{str1};
    str2.back() = '1';

    typename TypeParam::Custom c1{str1};
    typename TypeParam::Custom c2{str2};

    EXPECT_FALSE(c1.any());
    EXPECT_TRUE(c2.any());
}

TYPED_TEST_P(BitsetTest, None) {
    std::string str1(TypeParam::kSize, '0');
    std::string str2{str1};
    str2.back() = '1';

    typename TypeParam::Custom c1{str1};
    typename TypeParam::Custom c2{str2};

    EXPECT_TRUE(c1.none());
    EXPECT_FALSE(c2.none());
}

TYPED_TEST_P(BitsetTest, Count) {
    typename TypeParam::Custom c{kVal1};
    typename TypeParam::STL s{kVal1};

    EXPECT_EQ(c.count(), s.count());
}

TYPED_TEST_P(BitsetTest, kSize) {
    typename TypeParam::Custom c;
    typename TypeParam::STL s;

    EXPECT_EQ(c.size(), s.size());
}

TYPED_TEST_P(BitsetTest, AndAssign) {
    typename TypeParam::Custom c1{kVal1};
    typename TypeParam::Custom c2{kVal2};
    typename TypeParam::STL s1{kVal1};
    typename TypeParam::STL s2{kVal2};

    c1 &= c2;
    s1 &= s2;

    this->IsEq(c1, s1);
}

TYPED_TEST_P(BitsetTest, OrAssign) {
    typename TypeParam::Custom c1{kVal1};
    typename TypeParam::Custom c2{kVal2};
    typename TypeParam::STL s1{kVal1};
    typename TypeParam::STL s2{kVal2};

    c1 |= c2;
    s1 |= s2;

    this->IsEq(c1, s1);
}

TYPED_TEST_P(BitsetTest, XorAssign) {
    typename TypeParam::Custom c1{kVal1};
    typename TypeParam::Custom c2{kVal2};
    typename TypeParam::STL s1{kVal1};
    typename TypeParam::STL s2{kVal2};

    c1 ^= c2;
    s1 ^= s2;

    this->IsEq(c1, s1);
}

TYPED_TEST_P(BitsetTest, NotOperator) {
    typename TypeParam::Custom c1{kVal1};
    typename TypeParam::STL s1{kVal1};

    auto c2 = ~c1;
    auto s2 = ~s1;

    this->IsEq(c2, s2);
}

TYPED_TEST_P(BitsetTest, LeftShift) {
    typename TypeParam::Custom c1{kVal1};
    typename TypeParam::STL s1{kVal1};

    auto c2 = c1 << 2;
    auto s2 = s1 << 2;

    this->IsEq(c2, s2);
}

TYPED_TEST_P(BitsetTest, LeftShiftAssign) {
    typename TypeParam::Custom c{kVal1};
    typename TypeParam::STL s{kVal1};

    c <<= 2;
    s <<= 2;

    this->IsEq(c, s);
}

TYPED_TEST_P(BitsetTest, RightShift) {
    typename TypeParam::Custom c1{kVal1};
    typename TypeParam::STL s1{kVal1};

    auto c2 = c1 >> 2;
    auto s2 = s1 >> 2;

    this->IsEq(c2, s2);
}

TYPED_TEST_P(BitsetTest, RightShiftAssign) {
    typename TypeParam::Custom c{kVal1};
    typename TypeParam::STL s{kVal1};

    c >>= 2;
    s >>= 2;

    this->IsEq(c, s);
}

TYPED_TEST_P(BitsetTest, SetAll) {
    typename TypeParam::Custom c;
    typename TypeParam::STL s;

    c.set();
    s.set();

    this->IsEq(c, s);
}

TYPED_TEST_P(BitsetTest, Set) {
    auto str = this->GenerateString();
    typename TypeParam::Custom c{str};
    typename TypeParam::STL s{str};

    c.set(2);
    s.set(2);

    this->IsEq(c, s);
}

TYPED_TEST_P(BitsetTest, SetInvalidIndex) {
    typename TypeParam::Custom c;

    EXPECT_THROW(c.set(TypeParam::kSize), std::out_of_range);
}

TYPED_TEST_P(BitsetTest, ResetAll) {
    typename TypeParam::Custom c{kVal1};
    typename TypeParam::STL s{kVal1};

    c.reset();
    s.reset();

    this->IsEq(c, s);
}

TYPED_TEST_P(BitsetTest, Reset) {
    auto str = this->GenerateString();
    typename TypeParam::Custom c{str};
    typename TypeParam::STL s{str};

    c.reset(1);
    s.reset(1);

    this->IsEq(c, s);
}

TYPED_TEST_P(BitsetTest, ResetInvalidIndex) {
    typename TypeParam::Custom c;

    EXPECT_THROW(c.reset(TypeParam::kSize), std::out_of_range);
}

TYPED_TEST_P(BitsetTest, FlipAll) {
    typename TypeParam::Custom c{kVal1};
    typename TypeParam::STL s{kVal1};

    c.flip();
    s.flip();

    this->IsEq(c, s);
}

TYPED_TEST_P(BitsetTest, Flip) {
    auto str = this->GenerateString();
    typename TypeParam::Custom c{str};
    typename TypeParam::STL s{str};

    c.flip(1);
    s.flip(1);

    this->IsEq(c, s);
}

TYPED_TEST_P(BitsetTest, FlipInvalidIndex) {
    typename TypeParam::Custom c;

    EXPECT_THROW(c.flip(TypeParam::kSize), std::out_of_range);
}

TYPED_TEST_P(BitsetTest, ToString) {
    auto str = this->GenerateString();
    typename TypeParam::Custom c{str};

    EXPECT_EQ(c.to_string(), str);
}

TYPED_TEST_P(BitsetTest, ToStringCustomChars) {
    typename TypeParam::Custom c{kVal1};
    typename TypeParam::STL s{kVal1};

    EXPECT_EQ(c.to_string('.', 'N'), s.to_string('.', 'N'));
}

TYPED_TEST_P(BitsetTest, ToULong) {
    typename TypeParam::Custom c{kVal1};
    typename TypeParam::STL s{kVal1};

    EXPECT_EQ(c.to_ulong(), s.to_ulong());
}

TYPED_TEST_P(BitsetTest, ToULLong) {
    typename TypeParam::Custom c{kVal1};
    typename TypeParam::STL s{kVal1};

    EXPECT_EQ(c.to_ullong(), s.to_ullong());
}

TYPED_TEST_P(BitsetTest, And) {
    typename TypeParam::Custom c1{kVal1};
    typename TypeParam::Custom c2{kVal2};
    typename TypeParam::STL s1{kVal1};
    typename TypeParam::STL s2{kVal2};

    this->IsEq(c1 & c2, s1 & s2);
}

TYPED_TEST_P(BitsetTest, Or) {
    typename TypeParam::Custom c1{kVal1};
    typename TypeParam::Custom c2{kVal2};
    typename TypeParam::STL s1{kVal1};
    typename TypeParam::STL s2{kVal2};

    this->IsEq(c1 | c2, s1 | s2);
}

TYPED_TEST_P(BitsetTest, Xor) {
    typename TypeParam::Custom c1{kVal1};
    typename TypeParam::Custom c2{kVal2};
    typename TypeParam::STL s1{kVal1};
    typename TypeParam::STL s2{kVal2};

    this->IsEq(c1 ^ c2, s1 ^ s2);
}

TYPED_TEST_P(BitsetTest, ToStream) {
    auto str = this->GenerateString();
    typename TypeParam::Custom c{str};

    std::ostringstream ss;
    ss << c;

    EXPECT_EQ(ss.str(), str);
}

TYPED_TEST_P(BitsetTest, FromStream) {
    auto str = this->GenerateString();
    std::stringstream ss;
    ss << str;
    typename TypeParam::Custom c;

    ss >> c;

    EXPECT_EQ(c.to_string(), str);
}

// SGI Extensions:
// We don't test _Unchecked_XXX methods, as they're used inside of standard ones
TYPED_TEST_P(BitsetTest, FindFirst_libstdcxxExtensions) {
#if defined(__GLIBCXX__)
    typename TypeParam::Custom c{kVal1};
    typename TypeParam::STL s{kVal1};

    EXPECT_EQ(c._Find_first(), s._Find_first());
#else
    GTEST_SKIP() << "SGI extensions unavailible";
#endif
}

TYPED_TEST_P(BitsetTest, FindNext_libstdcxxExtensions) {
#if defined(__GLIBCXX__)
    typename TypeParam::Custom c{kVal1};
    typename TypeParam::STL s{kVal1};

    EXPECT_EQ(c._Find_next(2), s._Find_next(2));
#else
    GTEST_SKIP() << "SGI extensions unavailible";
#endif
}

REGISTER_TYPED_TEST_SUITE_P(BitsetTest, DefaultConstructor, ULongConstructor, ULLongConstructor,
                            StringConstructor, CharArrayConstructor, OperatorsEqNeq,
                            SubscriptOperatorRead, SubscriptOperatorWrite, Test, TestInvalidIndex,
                            All, Any, None, Count, kSize, AndAssign, OrAssign, XorAssign,
                            NotOperator, LeftShift, LeftShiftAssign, RightShift, RightShiftAssign,
                            SetAll, Set, SetInvalidIndex, ResetAll, Reset, ResetInvalidIndex,
                            FlipAll, Flip, FlipInvalidIndex, ToString, ToStringCustomChars, ToULong,
                            ToULLong, And, Or, Xor, ToStream, FromStream,
                            FindFirst_libstdcxxExtensions, FindNext_libstdcxxExtensions);

using BitsetSizes = ::testing::Types<BitsetSizeParam<8>, BitsetSizeParam<128>>;
INSTANTIATE_TYPED_TEST_SUITE_P(DefaultTests, BitsetTest, BitsetSizes);

}  // namespace tests
