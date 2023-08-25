#include <memory>

#include <gtest/gtest.h>

#include "types.h"

namespace tests {
class NumericCast : public ::testing::Test {
protected:
    std::byte* dbl_twenty_one;
    std::byte* dbl_seven;
    std::byte* int_twenty_one;
    std::byte* int_seven;

    model::INumericType const& dbl_type_ref = dbl_type;
    model::INumericType const& int_type_ref = int_type;

    void SetUp() override {
        int_twenty_one_ptr.reset(int_type_ref.MakeValueOfInt(21));
        int_seven_ptr.reset(int_type_ref.MakeValueOfInt(7));

        int_twenty_one = int_twenty_one_ptr.get();
        int_seven = int_seven_ptr.get();

        dbl_twenty_one_ptr.reset(model::DoubleType::MakeFrom(int_twenty_one, int_type_ref));
        dbl_seven_ptr.reset(model::DoubleType::MakeFrom(int_seven, int_type_ref));

        dbl_twenty_one = dbl_twenty_one_ptr.get();
        dbl_seven = dbl_seven_ptr.get();
    }

private:
    std::unique_ptr<std::byte[]> dbl_twenty_one_ptr;
    std::unique_ptr<std::byte[]> dbl_seven_ptr;
    std::unique_ptr<std::byte[]> int_twenty_one_ptr;
    std::unique_ptr<std::byte[]> int_seven_ptr;

    model::IntType int_type;
    model::DoubleType dbl_type;
};

TEST_F(NumericCast, GetValueAsDouble) {
    ASSERT_DOUBLE_EQ(dbl_type_ref.GetValueAs<model::Double>(dbl_twenty_one), 21);
    ASSERT_DOUBLE_EQ(int_type_ref.GetValueAs<model::Double>(int_twenty_one), 21);
}

TEST_F(NumericCast, GetValueAsInt) {
    ASSERT_EQ(dbl_type_ref.GetValueAs<model::Int>(dbl_twenty_one), 21);
    ASSERT_EQ(int_type_ref.GetValueAs<model::Int>(int_twenty_one), 21);
}

TEST_F(NumericCast, DoubleTypeCastTo) {
    dbl_type_ref.CastTo(dbl_twenty_one, model::DoubleType());
    ASSERT_DOUBLE_EQ(dbl_type_ref.GetValueAs<model::Double>(dbl_twenty_one), 21);
    dbl_type_ref.CastTo(dbl_twenty_one, model::IntType());
    ASSERT_DOUBLE_EQ(int_type_ref.GetValueAs<model::Int>(dbl_twenty_one), 21);
}

TEST_F(NumericCast, IntTypeCastTo) {
    int_type_ref.CastTo(int_twenty_one, model::IntType());
    ASSERT_EQ(int_type_ref.GetValueAs<model::Int>(int_twenty_one), 21);
    int_type_ref.CastTo(int_twenty_one, model::DoubleType());
    ASSERT_DOUBLE_EQ(dbl_type_ref.GetValueAs<model::Double>(int_twenty_one), 21);
}

TEST_F(NumericCast, ArifmeticDoubleCastedToInt) {
    std::unique_ptr<std::byte[]> sum_res_ptr(int_type_ref.MakeValueOfInt(0));
    std::unique_ptr<std::byte[]> sub_res_ptr(int_type_ref.MakeValueOfInt(0));
    std::unique_ptr<std::byte[]> div_res_ptr(int_type_ref.MakeValueOfInt(0));
    std::unique_ptr<std::byte[]> mult_res_ptr(int_type_ref.MakeValueOfInt(0));
    std::byte* sum_res = sum_res_ptr.get();
    std::byte* sub_res = sub_res_ptr.get();
    std::byte* div_res = div_res_ptr.get();
    std::byte* mult_res = mult_res_ptr.get();
    std::byte* a = dbl_twenty_one;
    std::byte* b = dbl_seven;
    dbl_type_ref.CastTo(a, int_type_ref);
    dbl_type_ref.CastTo(b, int_type_ref);
    int_type_ref.Add(a, b, sum_res);
    int_type_ref.Sub(a, b, sub_res);
    int_type_ref.Mul(a, b, mult_res);
    int_type_ref.Div(a, b, div_res);
    ASSERT_EQ(int_type_ref.GetValueAs<model::Int>(sum_res), 21 + 7);
    ASSERT_EQ(int_type_ref.GetValueAs<model::Int>(sub_res), 21 - 7);
    ASSERT_EQ(int_type_ref.GetValueAs<model::Int>(div_res), 21 / 7);
    ASSERT_EQ(int_type_ref.GetValueAs<model::Int>(mult_res), 21 * 7);
}

TEST_F(NumericCast, ArifmeticIntCastedToDouble) {
    std::unique_ptr<std::byte[]> sum_res_ptr(
            model::DoubleType::MakeFrom(int_twenty_one, int_type_ref));
    std::unique_ptr<std::byte[]> sub_res_ptr(
            model::DoubleType::MakeFrom(int_twenty_one, int_type_ref));
    std::unique_ptr<std::byte[]> div_res_ptr(
            model::DoubleType::MakeFrom(int_twenty_one, int_type_ref));
    std::unique_ptr<std::byte[]> mult_res_ptr(
            model::DoubleType::MakeFrom(int_twenty_one, int_type_ref));
    std::byte* sum_res = sum_res_ptr.get();
    std::byte* sub_res = sub_res_ptr.get();
    std::byte* div_res = div_res_ptr.get();
    std::byte* mult_res = mult_res_ptr.get();
    std::byte* a = int_twenty_one;
    std::byte* b = int_seven;
    int_type_ref.CastTo(a, dbl_type_ref);
    int_type_ref.CastTo(b, dbl_type_ref);
    dbl_type_ref.Add(a, b, sum_res);
    dbl_type_ref.Sub(a, b, sub_res);
    dbl_type_ref.Mul(a, b, mult_res);
    dbl_type_ref.Div(a, b, div_res);
    ASSERT_DOUBLE_EQ(dbl_type_ref.GetValueAs<model::Double>(sum_res), 21 + 7);
    ASSERT_DOUBLE_EQ(dbl_type_ref.GetValueAs<model::Double>(sub_res), 21 - 7);
    ASSERT_DOUBLE_EQ(dbl_type_ref.GetValueAs<model::Double>(div_res), 21.0 / 7.0);
    ASSERT_DOUBLE_EQ(dbl_type_ref.GetValueAs<model::Double>(mult_res), 21 * 7);
}

TEST_F(NumericCast, CastDoubleToBuiltin) {
    ASSERT_DOUBLE_EQ(dbl_type_ref.GetValueAs<model::Double>(dbl_twenty_one), 21.0);
    ASSERT_FLOAT_EQ(dbl_type_ref.GetValueAs<float>(dbl_twenty_one), 21.0f);
    ASSERT_EQ(dbl_type_ref.GetValueAs<model::Int>(dbl_twenty_one), 21);
}

TEST_F(NumericCast, CastIntToBuiltin) {
    ASSERT_DOUBLE_EQ(int_type_ref.GetValueAs<model::Double>(int_twenty_one), 21.0);
    ASSERT_FLOAT_EQ(int_type_ref.GetValueAs<float>(int_twenty_one), 21.0f);
    ASSERT_EQ(int_type_ref.GetValueAs<model::Int>(int_twenty_one), 21);
}
}  // namespace tests
