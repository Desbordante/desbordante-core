#include <memory>

#include <gtest/gtest.h>

#include "types.h"

namespace tests {
class NumericCast : public ::testing::Test {
private:
    std::unique_ptr<std::byte[]> dbl_twenty_one__ptr_;
    std::unique_ptr<std::byte[]> dbl_seven_ptr_;
    std::unique_ptr<std::byte[]> int_twenty_one_ptr_;
    std::unique_ptr<std::byte[]> int_seven_ptr_;

    model::IntType int_type_;
    model::DoubleType dbl_type_;

protected:
    std::byte* dbl_twenty_one_;
    std::byte* dbl_seven_;
    std::byte* int_twenty_one_;
    std::byte* int_seven_;

    model::INumericType const& dbl_type_ref_ = dbl_type_;
    model::INumericType const& int_type_ref_ = int_type_;

    void SetUp() override {
        int_twenty_one_ptr_.reset(int_type_ref_.MakeValueOfInt(21));
        int_seven_ptr_.reset(int_type_ref_.MakeValueOfInt(7));

        int_twenty_one_ = int_twenty_one_ptr_.get();
        int_seven_ = int_seven_ptr_.get();

        dbl_twenty_one__ptr_.reset(dbl_type_ref_.MakeValueOfInt(21));
        dbl_seven_ptr_.reset(dbl_type_ref_.MakeValueOfInt(7));

        dbl_twenty_one_ = dbl_twenty_one__ptr_.get();
        dbl_seven_ = dbl_seven_ptr_.get();
    }

    template <typename T>
    void TestArithmetic(std::byte* twenty_one, std::byte* seven,
                        model::INumericType const& type_ref_from,
                        model::INumericType const& type_ref_to) {
        std::unique_ptr<std::byte[]> result_ptr(int_type_ref_.MakeValueOfInt(0));
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
};

TEST_F(NumericCast, GetValueAsDouble) {
    ASSERT_DOUBLE_EQ(dbl_type_ref_.GetValueAs<model::Double>(dbl_twenty_one_), 21);
    ASSERT_DOUBLE_EQ(int_type_ref_.GetValueAs<model::Double>(int_twenty_one_), 21);
}

TEST_F(NumericCast, GetValueAsInt) {
    ASSERT_EQ(dbl_type_ref_.GetValueAs<model::Int>(dbl_twenty_one_), 21);
    ASSERT_EQ(int_type_ref_.GetValueAs<model::Int>(int_twenty_one_), 21);
}

TEST_F(NumericCast, DoubleTypeCastTo) {
    dbl_type_ref_.CastTo(dbl_twenty_one_, model::DoubleType());
    ASSERT_DOUBLE_EQ(dbl_type_ref_.GetValueAs<model::Double>(dbl_twenty_one_), 21);
    dbl_type_ref_.CastTo(dbl_twenty_one_, model::IntType());
    ASSERT_DOUBLE_EQ(int_type_ref_.GetValueAs<model::Int>(dbl_twenty_one_), 21);
}

TEST_F(NumericCast, IntTypeCastTo) {
    int_type_ref_.CastTo(int_twenty_one_, model::IntType());
    ASSERT_EQ(int_type_ref_.GetValueAs<model::Int>(int_twenty_one_), 21);
    int_type_ref_.CastTo(int_twenty_one_, model::DoubleType());
    ASSERT_DOUBLE_EQ(dbl_type_ref_.GetValueAs<model::Double>(int_twenty_one_), 21);
}

TEST_F(NumericCast, ArifmeticDoubleCastedToInt) {
    TestArithmetic<model::Int>(dbl_twenty_one_, dbl_seven_, dbl_type_ref_, int_type_ref_);
}

TEST_F(NumericCast, ArifmeticIntCastedToDouble) {
    TestArithmetic<model::Double>(int_twenty_one_, int_seven_, int_type_ref_, dbl_type_ref_);
}

TEST_F(NumericCast, CastDoubleToBuiltin) {
    ASSERT_DOUBLE_EQ(dbl_type_ref_.GetValueAs<model::Double>(dbl_twenty_one_), 21.0);
    ASSERT_FLOAT_EQ(dbl_type_ref_.GetValueAs<float>(dbl_twenty_one_), 21.0f);
    ASSERT_EQ(dbl_type_ref_.GetValueAs<model::Int>(dbl_twenty_one_), 21);
}

TEST_F(NumericCast, CastIntToBuiltin) {
    ASSERT_DOUBLE_EQ(int_type_ref_.GetValueAs<model::Double>(int_twenty_one_), 21.0);
    ASSERT_FLOAT_EQ(int_type_ref_.GetValueAs<float>(int_twenty_one_), 21.0f);
    ASSERT_EQ(int_type_ref_.GetValueAs<model::Int>(int_twenty_one_), 21);
}
}  // namespace tests
