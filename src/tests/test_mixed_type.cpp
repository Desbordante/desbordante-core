#include <memory>

#include <gtest/gtest.h>

#include "mixed_type.h"

#include <typeinfo>


namespace tests{
class ClassMixedType : public ::testing::Test {
    
    protected:

    model::MixedType *mt;

    void SetUp() { mt = new model::MixedType(false); }
    void TearDown() { delete mt; } 

};

TEST_F(ClassMixedType, TestCloneIntType) {

    model::IntType type_int_;
    std::byte * value_;

    value_=mt->MakeValue(12,&type_int_);
    std::byte * new_value_=mt->Clone(value_);

    EXPECT_EQ(mt->ValueToString(value_),mt->ValueToString(new_value_));
};

TEST_F(ClassMixedType, TestCloneDoubleType) {

    model::DoubleType type_double_;
    std::byte * value_;

    value_=mt->MakeValue(2.7,&type_double_);
    std::byte * new_value_=mt->Clone(value_);

    EXPECT_EQ(mt->ValueToString(value_),mt->ValueToString(new_value_));
};

TEST_F(ClassMixedType, TestCloneStringType) {

    model::StringType type_string_(model::TypeId::kString);
    std::byte * value_;
    std::string str_="Hello World!";

    value_=mt->MakeValue(str_,&type_string_);
    std::byte * new_value_=mt->Clone(value_);

    EXPECT_EQ(mt->ValueToString(value_),mt->ValueToString(new_value_));
};

//Don't work 
/*TEST_F(ClassMixedType, TestCloneDate) {
 
    model::DateType type_date_;
    std::byte * value_;
    std::string ds("2001-10-9");
    model::Date date(boost::gregorian::from_simple_string(ds));
    std::cout<<boost::gregorian::to_simple_string(date);

    value_=mt->MakeValue(date,&type_date_);
    std::byte * new_value_=mt->Clone(value_);

    EXPECT_EQ(mt->ValueToString(value_),mt->ValueToString(new_value_));
};*/

TEST_F(ClassMixedType, TestCloneNullType) {
 
    model::NullType type_null_(true);
    std::byte * value_;

    value_=mt->MakeValue(0,&type_null_);
    std::byte * new_value_=mt->Clone(value_);

    EXPECT_EQ(mt->ValueToString(value_),mt->ValueToString(new_value_));
};

TEST_F(ClassMixedType, TestCloneEmptyType) {
 
    model::EmptyType type_empty_;
    std::byte * value_;

    value_=mt->MakeValue(0,&type_empty_);
    std::byte * new_value_=mt->Clone(value_);

    EXPECT_EQ(mt->ValueToString(value_),mt->ValueToString(new_value_));
};
}