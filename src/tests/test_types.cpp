#include <functional>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "types.h"

namespace tests {

namespace mo = model;

template <typename T>
class TestNumeric : public ::testing::Test {
public:
    using NumericType = T;
    using UnderlyingType = typename T::UnderlyingType;

private:
    using NumericTypeMethod = std::byte* (NumericType::*)(std::byte const*, std::byte const*,
                                                          std::byte*) const;

    std::unique_ptr<std::byte[]> actual_owner_;
    std::unique_ptr<std::byte[]> literal_owner_;

protected:
    std::unique_ptr<NumericType> type_;
    std::byte* actual_ptr_;
    std::byte* literal_ptr_;

    void SetUp() override {
        type_ = std::make_unique<NumericType>();
        actual_owner_.reset(type_->MakeValue(0));
        literal_owner_.reset(type_->MakeValue(0));
        actual_ptr_ = actual_owner_.get();
        literal_ptr_ = literal_owner_.get();
    }

    void SetActualAndLiteral(UnderlyingType actual, UnderlyingType literal) {
        SetActual(actual);
        SetLiteral(literal);
    }

    void SetActual(UnderlyingType actual) {
        mo::Type::GetValue<UnderlyingType>(actual_ptr_) = actual;
    }

    void SetLiteral(UnderlyingType literal) {
        mo::Type::GetValue<UnderlyingType>(literal_ptr_) = literal;
    }

    template <typename F>
    void TestBinary(UnderlyingType l, UnderlyingType r, F result_f,
                    NumericTypeMethod method_to_test) {
        SetActualAndLiteral(l, r);
        ExpectEq(result_f(l, r),
                 std::invoke(method_to_test, type_.get(), actual_ptr_, literal_ptr_, actual_ptr_),
                 GetErrorStr(l, r));
    }

    template <typename K, typename Y>
    static std::string GetErrorStr(K l, Y r) {
        return "Failed with l=" + std::to_string(l) + " and r=" + std::to_string(r);
    }

    static void ExpectEq(UnderlyingType expected, std::byte const* v, std::string error = "") {
        UnderlyingType actual = mo::Type::GetValue<decltype(expected)>(v);
        if constexpr (std::is_floating_point_v<UnderlyingType>) {
            EXPECT_DOUBLE_EQ(expected, actual) << error;
        } else {
            EXPECT_EQ(expected, actual) << error;
        }
    };
};

using NumericTypes = ::testing::Types<mo::DoubleType, mo::IntType>;
TYPED_TEST_SUITE(TestNumeric, NumericTypes);

TYPED_TEST(TestNumeric, Compare) {
    using Type = typename TypeParam::UnderlyingType;
    auto test = [this](Type l, Type r, mo::CompareResult res) {
        TestFixture::SetActualAndLiteral(l, r);
        EXPECT_EQ(res, this->type_->Compare(this->actual_ptr_, this->literal_ptr_));
    };

    test(3, 3, mo::CompareResult::kEqual);
    test(1, 2, mo::CompareResult::kLess);
    test(100, 12, mo::CompareResult::kGreater);
    test(-3, 3, mo::CompareResult::kLess);
    test(0, -100, mo::CompareResult::kGreater);
}

TYPED_TEST(TestNumeric, Negate) {
    auto test = [this](typename TypeParam::UnderlyingType v) {
        TestFixture::SetActual(v);
        TestFixture::ExpectEq(-v, this->type_->Negate(this->actual_ptr_, this->actual_ptr_));
    };

    test(0);
    test(typename TypeParam::UnderlyingType(-123.5));
    test(typename TypeParam::UnderlyingType(321.4));
}

TYPED_TEST(TestNumeric, Abs) {
    auto test = [this](typename TypeParam::UnderlyingType v) {
        TestFixture::SetActual(v);
        TestFixture::ExpectEq(std::abs(v), this->type_->Abs(this->actual_ptr_, this->actual_ptr_));
    };

    test(0);
    test(typename TypeParam::UnderlyingType(-123.5));
    test(typename TypeParam::UnderlyingType(321.4));
}

TYPED_TEST(TestNumeric, Add) {
    using Type = typename TypeParam::UnderlyingType;
    auto test = [this](Type l, Type r) {
        this->TestBinary(l, r, std::plus<Type>(), &TypeParam::Add);
    };

    test(0, 1);
    test(0, 0);
    test(1, 1);
    test(-4, 123);
    test(-123, -11);
    test(-123, 808);
}

TYPED_TEST(TestNumeric, Div) {
    using Type = typename TypeParam::UnderlyingType;
    auto test = [this](Type l, Type r) {
        this->TestBinary(l, r, std::divides<Type>(), &TypeParam::Div);
    };

    test(0, 100);
    test(22, 1);
    test(123, 321);
    test(Type(11.4), Type(3.14));
    test(-102, 11);
    test(-123, 123);
    test(-21, -7);
}

TYPED_TEST(TestNumeric, Sub) {
    using Type = typename TypeParam::UnderlyingType;
    auto test = [this](Type l, Type r) {
        this->TestBinary(l, r, std::minus<Type>(), &TypeParam::Sub);
    };

    test(0, 100);
    test(22, 12);
    test(123, 321);
    test(Type(2.72), Type(1.3123141));
    test(-102, 11);
    test(-123, 123);
    test(-21, -7);
}

TYPED_TEST(TestNumeric, Mul) {
    using Type = typename TypeParam::UnderlyingType;
    auto test = [this](Type l, Type r) {
        this->TestBinary(l, r, std::multiplies<Type>(), &TypeParam::Mul);
    };

    test(0, 100);
    test(100, 0);
    test(22, 12);
    test(123, 321);
    test(Type(2.72), Type(1.3123141));
    test(-102, 11);
    test(-123, 123);
    test(-21, -7);
}

TYPED_TEST(TestNumeric, Pow) {
    using Type = typename TypeParam::UnderlyingType;
    auto test = [this](Type l, double r) {
        this->SetActual(l);
        this->ExpectEq(std::pow(l, r), this->type_->Power(this->actual_ptr_, r, this->actual_ptr_),
                       TestFixture::GetErrorStr(l, r));
    };

    test(0, 100);
    test(22, 12);
    test(123, 321);
    test(Type(2.72), Type(1.3123141));
    test(-102, 11);
    test(-123, 123);
    test(-21, -7);
}

TYPED_TEST(TestNumeric, Dist) {
    using Type = typename TypeParam::UnderlyingType;
    auto test = [this](Type l, Type r) {
        this->SetActualAndLiteral(l, r);
        EXPECT_DOUBLE_EQ(std::abs(l - r), this->type_->Dist(this->actual_ptr_, this->literal_ptr_))
                << this->GetErrorStr(l, r);
    };

    test(0, 100);
    test(22, 12);
    test(123, 321);
    test(Type(2.72), Type(1.3123141));
    test(-102, 11);
    test(-123, 123);
    test(-21, -7);
}

TYPED_TEST(TestNumeric, ValueToString) {
    auto test = [this](typename TypeParam::UnderlyingType v) {
        this->SetActual(v);
        EXPECT_EQ(std::to_string(v), this->type_->ValueToString(this->actual_ptr_));
    };

    test(0);
    test(123);
    test(typename TypeParam::UnderlyingType(3.14123123182387));
    test(typename TypeParam::UnderlyingType(-1231.123456678987654321));
}

struct TestStringParam {
    std::string const l;
    std::string const r;

    TestStringParam(std::string l, std::string r) noexcept : l(std::move(l)), r(std::move(r)) {}
};

class TestString : public ::testing::TestWithParam<TestStringParam> {};

TEST_P(TestString, Default) {
    TestStringParam const& p = GetParam();
    std::string const& l_val = p.l;
    std::string const& r_val = p.r;
    std::string const concat_result_val = l_val + r_val;
    std::unique_ptr<mo::Type> type(mo::CreateType(mo::TypeId::kString, true));
    mo::StringType const* s = static_cast<mo::StringType const*>(type.get());
    model::CompareResult cr = s->Compare(l_val, r_val);

    using Owner = std::unique_ptr<std::byte[], mo::StringTypeDeleter>;

    Owner l_owner(s->MakeValue(l_val), s->GetDeleter());
    Owner r_owner(s->MakeValue(r_val), s->GetDeleter());
    Owner concat_result_owner(s->MakeValue(concat_result_val), s->GetDeleter());
    std::byte* l = l_owner.get();
    std::byte* r = r_owner.get();
    std::byte* concat_result = concat_result_owner.get();

    EXPECT_EQ(s->Compare(l, r), cr);

    std::unique_ptr<std::byte[]> actual(s->Concat(l, r));

    EXPECT_EQ(s->Compare(actual.get(), concat_result), mo::CompareResult::kEqual);
    EXPECT_EQ(s->ValueToString(actual.get()), concat_result_val);
}

INSTANTIATE_TEST_SUITE_P(TestStringSuite, TestString,
                         ::testing::Values(TestStringParam("123", "123"),
                                           TestStringParam("aa", "bbbb"),
                                           TestStringParam("a", "abcde"), TestStringParam("", ""),
                                           TestStringParam("", "abc"), TestStringParam("abc", ""),
                                           TestStringParam("bb", "aa")));

namespace {
using DatePtr = std::unique_ptr<std::byte[], mo::DateTypeDeleter>;

DatePtr DatePtrFromString(std::string const& s, std::unique_ptr<mo::DateType> const& date_type) {
    return {date_type->MakeValue(boost::gregorian::from_simple_string(s)),
            date_type->GetDeleater()};
}
}  // namespace

TEST(TestDateSuite, Compare) {
    std::unique_ptr<mo::DateType> date_type(
            mo::CreateSpecificType<mo::DateType>(mo::TypeId::kDate, true));
    auto test = [date_type = std::move(date_type)](
                        std::string const& date1, std::string const& date2, mo::CompareResult res) {
        DatePtr ptr1 = DatePtrFromString(date1, date_type);
        DatePtr ptr2 = DatePtrFromString(date2, date_type);
        EXPECT_EQ(date_type->Compare(ptr1.get(), ptr2.get()), res);
    };
    test("2004-07-28", "2004-07-28", mo::CompareResult::kEqual);
    test("2005-01-20", "2004-07-28", mo::CompareResult::kGreater);
    test("2023-09-01", "2024-10-25", mo::CompareResult::kLess);
    test("2023-10-28", "2023-10-28", mo::CompareResult::kEqual);
    test("2203-09-01", "2024-10-25", mo::CompareResult::kGreater);
    test("1991-11-20", "2200-07-28", mo::CompareResult::kLess);
}

TEST(TestDateSuite, Dist) {
    std::unique_ptr<mo::DateType> date_type(
            mo::CreateSpecificType<mo::DateType>(mo::TypeId::kDate, true));
    auto test = [date_type = std::move(date_type)](std::string const& date1,
                                                   std::string const& date2, long res) {
        DatePtr left_val = DatePtrFromString(date1, date_type);
        DatePtr right_val = DatePtrFromString(date2, date_type);
        EXPECT_EQ(static_cast<long>(date_type->Dist(left_val.get(), right_val.get())), res);
    };
    test("2004-07-28", "2004-07-28", 0);
    test("2005-01-20", "2004-07-28", 176);
    test("1991-11-20", "2200-07-28", 76221);
    test("1941-06-22", "1945-05-09", 1417);
    test("2023-10-28", "2297-08-12", 100000);
}

namespace {
struct TestDateArithmeticsParam {
    std::string const date1;
    std::string const date2;
    long const dist;
    TestDateArithmeticsParam(std::string l, std::string r, long dist)
        : date1(std::move(l)), date2(std::move(r)), dist(dist){};
};

using DateTypeBinop = std::byte* (mo::DateType::*)(std::byte const* date,
                                                   mo::DateType::Delta const&) const;

void TestBinop(std::string const& initial, std::string const& expected_date, long const dist,
               DateTypeBinop Binop) {
    std::unique_ptr<mo::DateType> date_type(
            mo::CreateSpecificType<mo::DateType>(mo::TypeId::kDate, true));
    mo::DateType::Delta delta(dist);

    DatePtr initial_date = DatePtrFromString(initial, date_type);
    DatePtr expected = DatePtrFromString(expected_date, date_type);
    auto actual = DatePtr(std::invoke(Binop, date_type.get(), initial_date.get(), delta),
                          date_type->GetDeleater());

    EXPECT_EQ(date_type->Compare(expected.get(), actual.get()), mo::CompareResult::kEqual);
}
}  // namespace

class TestDateArithmetics : public ::testing::TestWithParam<TestDateArithmeticsParam> {};

TEST_P(TestDateArithmetics, SubtractDelta) {
    auto const& [initial_date, expected, delta] = GetParam();
    TestBinop(initial_date, expected, delta, &mo::DateType::SubDelta);
}

TEST_P(TestDateArithmetics, AddDelta) {
    auto const& [expected, initial_date, delta] = GetParam();
    TestBinop(initial_date, expected, delta, &mo::DateType::AddDelta);
}

TEST_P(TestDateArithmetics, SubDate) {
    std::unique_ptr<mo::DateType> date_type(
            mo::CreateSpecificType<mo::DateType>(mo::TypeId::kDate, true));
    auto const& [l_val, r_val, delta] = GetParam();

    DatePtr left_val = DatePtrFromString(l_val, date_type);
    DatePtr right_val = DatePtrFromString(r_val, date_type);

    EXPECT_EQ(date_type->SubDate(left_val.get(), right_val.get()).days(), delta);
}

INSTANTIATE_TEST_SUITE_P(
        TestDateSuite, TestDateArithmetics,
        ::testing::Values(TestDateArithmeticsParam("2004-07-28", "2004-07-28", 0),
                          TestDateArithmeticsParam("2005-01-20", "2004-07-28", 176),
                          TestDateArithmeticsParam("2004-07-28", "2005-01-20", -176),
                          TestDateArithmeticsParam("2023-10-28", "2297-08-12", -100000),
                          TestDateArithmeticsParam("2297-08-12", "2023-10-28", 100000),
                          TestDateArithmeticsParam("2200-07-28", "1991-11-20", 76221),
                          TestDateArithmeticsParam("1991-11-20", "2200-07-28", -76221),
                          TestDateArithmeticsParam("1945-05-09", "1941-06-22", 1417),
                          TestDateArithmeticsParam("1941-06-22", "1945-05-09", -1417)));
}  // namespace tests
