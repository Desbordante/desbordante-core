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

    template <typename T>
    void TestArithmetic(std::byte* twenty_one, std::byte* seven,
                        model::INumericType const& type_ref_from,
                        model::INumericType const& type_ref_to) {
        std::unique_ptr<std::byte[]> result_ptr(int_type_ref.MakeValueOfInt(0));
        std::byte* result = result_ptr.get();
        type_ref_from.CastTo(twenty_one, type_ref_to);
        type_ref_from.CastTo(seven, type_ref_to);
        type_ref_to.Add(twenty_one, seven, result);
        ASSERT_EQ(type_ref_to.GetValueAs<T>(result), 21 + 7);
        type_ref_to.Sub(twenty_one, seven, result);
        ASSERT_EQ(type_ref_to.GetValueAs<T>(result), 21 - 7);
        type_ref_to.Mul(twenty_one, seven, result);
        ASSERT_EQ(type_ref_to.GetValueAs<T>(result), 21 * 7);
        type_ref_to.Div(twenty_one, seven, result);
        ASSERT_EQ(type_ref_to.GetValueAs<T>(result), 21 / 7);
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
    TestArithmetic<model::Int>(dbl_twenty_one, dbl_seven, dbl_type_ref, int_type_ref);
}

TEST_F(NumericCast, ArifmeticIntCastedToDouble) {
    TestArithmetic<model::Double>(int_twenty_one, int_seven, int_type_ref, dbl_type_ref);
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
