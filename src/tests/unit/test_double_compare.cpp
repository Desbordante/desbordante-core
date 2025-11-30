#include <boost/math/special_functions/next.hpp>
#include <gtest/gtest.h>

#include "core/model/types/types.h"

namespace tests {

class DoubleCompare : public ::testing::Test {
private:
    std::unique_ptr<std::byte[]> double_min_ptr_;
    std::unique_ptr<std::byte[]> double_max_ptr_;
    std::unique_ptr<std::byte[]> result_ptr_;

protected:
    model::DoubleType double_type_;
    model::INumericType& double_type_ref_ = double_type_;

    std::byte* double_min_;
    std::byte* double_max_;
    std::byte* result_;

    static void Add(std::byte* a, std::byte* b, std::byte* res) {
        model::DoubleType double_type;
        double_type.Add(a, b, res);
    }

    static void Sub(std::byte* a, std::byte* b, std::byte* res) {
        model::DoubleType double_type;
        double_type.Sub(a, b, res);
    }

    static void Mul(std::byte* a, std::byte* b, std::byte* res) {
        model::DoubleType double_type;
        double_type.Mul(a, b, res);
    }

    static void Div(std::byte* a, std::byte* b, std::byte* res) {
        model::DoubleType double_type;
        double_type.Div(a, b, res);
    }

    void SetUp() override {
        double_min_ptr_.reset(double_type_.MakeValue(std::numeric_limits<model::Double>().min()));
        double_max_ptr_.reset(double_type_.MakeValue(std::numeric_limits<model::Double>().max()));
        result_ptr_.reset(double_type_.MakeValue(0.0));
        result_ = result_ptr_.get();
        double_min_ = double_min_ptr_.get();
        double_max_ = double_max_ptr_.get();
    }

    template <typename Fnc>
    void TestArithmetic(model::Double left, model::Double right, model::Double expected,
                        Fnc operation) {
        std::unique_ptr<std::byte[]> left_ptr(double_type_.MakeValue(left));
        std::unique_ptr<std::byte[]> right_ptr(double_type_.MakeValue(right));
        std::unique_ptr<std::byte[]> result(double_type_.Allocate());
        std::unique_ptr<std::byte[]> expected_ptr(double_type_.MakeValue(expected));
        operation(left_ptr.get(), right_ptr.get(), result.get());
        ASSERT_EQ(double_type_ref_.Compare(result.get(), expected_ptr.get()),
                  model::CompareResult::kEqual);
    }
};

TEST_F(DoubleCompare, AddMin) {
    std::unique_ptr<std::byte[]> number_128_ptr(double_type_.MakeValue(128.0));  // 2^7
    std::unique_ptr<std::byte[]> number_100_ptr(double_type_.MakeValue(100.0));
    std::unique_ptr<std::byte[]> buff_ptr(double_type_.MakeValue(0));
    std::unique_ptr<std::byte[]> less_ptr(double_type_.MakeValue(0));

    std::byte* buff = buff_ptr.get();
    std::byte* number_128 = number_128_ptr.get();
    std::byte* number_100 = number_100_ptr.get();
    std::byte* less = less_ptr.get();

    double_type_ref_.Mul(number_128, double_min_, result_);
    double_type_ref_.Mul(number_100, double_min_, less);

    for (int i = 0; i < 128; i++) {
        double_type_ref_.Add(buff, double_min_, buff);
    }

    ASSERT_EQ(double_type_ref_.Compare(buff, result_), model::CompareResult::kEqual);
    ASSERT_EQ(double_type_ref_.Compare(less, buff), model::CompareResult::kLess);
}

TEST_F(DoubleCompare, DivMax) {
    std::unique_ptr<std::byte[]> three_ptr(double_type_.MakeValue(3.0));
    std::unique_ptr<std::byte[]> three_power_20_ptr(double_type_.MakeValue(std::pow(3., 20.)));
    std::unique_ptr<std::byte[]> expected_ptr(double_type_.MakeValue(0.0));

    std::byte* three = three_ptr.get();
    std::byte* three_power_20 = three_power_20_ptr.get();
    std::byte* expected = expected_ptr.get();

    double_type_ref_.Div(double_max_, three_power_20, expected);
    for (int i = 0; i < 20; i++) {
        double_type_ref_.Div(double_max_, three, double_max_);
    }

    ASSERT_EQ(double_type_ref_.Compare(double_max_, expected), model::CompareResult::kEqual);
}

TEST_F(DoubleCompare, ZeroEQMinDivMax) {
    std::unique_ptr<std::byte[]> double_zero_ptr(double_type_.MakeValue(0.0));
    std::byte* double_zero = double_zero_ptr.get();

    ASSERT_EQ(double_type_ref_.Compare(double_zero, double_min_), model::CompareResult::kEqual);
    double_type_ref_.Div(double_min_, double_max_, result_);
    ASSERT_EQ(double_type_ref_.Compare(result_, double_zero), model::CompareResult::kEqual);
}

TEST_F(DoubleCompare, ComprasionFromAcAlgorithm) {
    std::unique_ptr<std::byte[]> seven_point_seven(double_type_.MakeValue(7.7));
    std::unique_ptr<std::byte[]> six_point_nine(double_type_.MakeValue(6.9));
    std::unique_ptr<std::byte[]> expect(double_type_.MakeValue(7.7 + 6.9));

    double_type_ref_.Add(seven_point_seven.get(), six_point_nine.get(), result_);
    ASSERT_EQ(double_type_ref_.Compare(result_, expect.get()), model::CompareResult::kEqual);
}

TEST_F(DoubleCompare, AddNumbers) {
    TestArithmetic(7.7, 8.8, 7.7 + 8.8, Add);
    TestArithmetic(777.777, 888.888, 777.777 + 888.888, Add);
}

TEST_F(DoubleCompare, SubNumbers) {
    TestArithmetic(7.7, 8.8, 7.7 - 8.8, Sub);
    TestArithmetic(777.777, 888.888, 777.777 - 888.888, Sub);
}

TEST_F(DoubleCompare, MulNumbers) {
    TestArithmetic(7.7, 8.8, 7.7 * 8.8, Mul);
    TestArithmetic(777.777, 888.888, 777.777 * 888.888, Mul);
}

TEST_F(DoubleCompare, DivNumbers) {
    TestArithmetic(7.7, 8.8, 7.7 / 8.8, Div);
    TestArithmetic(777.777, 888.888, 777.777 / 888.888, Div);
}

TEST_F(DoubleCompare, TwoSmallNumbers) {
    std::unique_ptr<std::byte[]> eighteen_zeroes_one(double_type_.MakeValue(std::pow(10, -18)));
    std::unique_ptr<std::byte[]> seventeen_zeroes_one(double_type_.MakeValue(std::pow(10, -17)));

    ASSERT_NE(double_type_ref_.Compare(eighteen_zeroes_one.get(), seventeen_zeroes_one.get()),
              model::CompareResult::kEqual);
}

TEST_F(DoubleCompare, EpsilonMin) {
    std::unique_ptr<std::byte[]> epsilon(
            double_type_.MakeValue(std::numeric_limits<model::Double>::epsilon() / 2.0));
    std::unique_ptr<std::byte[]> minimum(
            double_type_.MakeValue(std::numeric_limits<model::Double>::min()));

    ASSERT_EQ(double_type_ref_.Compare(epsilon.get(), minimum.get()),
              model::CompareResult::kGreater);
}

TEST_F(DoubleCompare, LowFractionalNumber) {
    std::unique_ptr<std::byte[]> ten_power_minus_2(double_type_.MakeValue(1e-2));
    std::unique_ptr<std::byte[]> ten_power_minus_2_plus_eps_div_ten(
            double_type_.MakeValue(1e-2 + 2.5e-17));

    ASSERT_EQ(double_type_ref_.Compare(ten_power_minus_2.get(),
                                       ten_power_minus_2_plus_eps_div_ten.get()),
              model::CompareResult::kLess);
}

TEST_F(DoubleCompare, BigFractionalNumber) {
    std::unique_ptr<std::byte[]> thirty_thousands(double_type_.MakeValue(30000.0));
    std::unique_ptr<std::byte[]> thirty_thousands_next(
            double_type_.MakeValue(boost::math::float_next(30000.0)));

    ASSERT_EQ(double_type_ref_.Compare(thirty_thousands.get(), thirty_thousands_next.get()),
              model::CompareResult::kEqual);
}

TEST_F(DoubleCompare, Denormalized) {
    std::unique_ptr<std::byte[]> min_denorm(
            double_type_.MakeValue(std::numeric_limits<model::Double>::denorm_min()));
    std::unique_ptr<std::byte[]> denorm(
            double_type_.MakeValue(std::numeric_limits<model::Double>::min() / 2.0));

    ASSERT_EQ(double_type_ref_.Compare(min_denorm.get(), denorm.get()),
              model::CompareResult::kEqual);
}

TEST_F(DoubleCompare, Infinity) {
    std::unique_ptr<std::byte[]> infinity(
            double_type_.MakeValue(std::numeric_limits<model::Double>::infinity()));
    std::unique_ptr<std::byte[]> min_denorm(
            double_type_.MakeValue(std::numeric_limits<model::Double>::denorm_min()));
    std::unique_ptr<std::byte[]> max_norm(
            double_type_.MakeValue(std::numeric_limits<model::Double>::max()));
    std::unique_ptr<std::byte[]> min_norm(
            double_type_.MakeValue(std::numeric_limits<model::Double>::min()));

    ASSERT_EQ(double_type_ref_.Compare(infinity.get(), infinity.get()),
              model::CompareResult::kEqual);
    ASSERT_EQ(double_type_ref_.Compare(infinity.get(), min_denorm.get()),
              model::CompareResult::kGreater);
    ASSERT_EQ(double_type_ref_.Compare(max_norm.get(), infinity.get()),
              model::CompareResult::kLess);
    ASSERT_EQ(double_type_ref_.Compare(min_norm.get(), infinity.get()),
              model::CompareResult::kLess);
}

TEST_F(DoubleCompare, DenormalizedNormalized) {
    std::unique_ptr<std::byte[]> min_denorm(
            double_type_.MakeValue(std::numeric_limits<model::Double>::denorm_min()));
    std::unique_ptr<std::byte[]> min_norm(
            double_type_.MakeValue(std::numeric_limits<model::Double>::min()));

    ASSERT_EQ(double_type_ref_.Compare(min_denorm.get(), min_norm.get()),
              model::CompareResult::kEqual);
}
}  // namespace tests
