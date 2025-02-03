#include <boost/parameter/aux_/void.hpp>
#include <gtest/gtest.h>

#include "algo_factory.h"
#include "all_csv_configs.h"
#include "dd/dd_verifier/dd_verifier.h"


TEST(HelloTest, BasicAssertions) {
    model::DDString dd;
    model::DFStringConstraint lhs;
    model::DFStringConstraint rhs;
    lhs.column_name = "Col0";
    lhs.lower_bound = 0;
    lhs.upper_bound = 2;
    rhs.column_name = "Col1";
    rhs.lower_bound = 0;
    rhs.upper_bound = 2;
    dd.left.push_back(lhs);
    dd.right.push_back(rhs);
    CSVConfig const &csv_config = tests::kTestDD1;
    auto mp = algos::StdParamsMap({{config::names::kCsvConfig, csv_config}, {config::names::kDDString, dd}});
    auto verifier = algos::CreateAndLoadAlgorithm<algos::dd::DDVerifier>(mp);
    EXPECT_TRUE(verifier->VerifyDD());
}
