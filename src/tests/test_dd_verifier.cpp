#include <boost/parameter/aux_/void.hpp>
#include <gtest/gtest.h>

#include "algo_factory.h"
#include "all_csv_configs.h"
#include "dd/dd_verifier/dd_verifier.h"

namespace tests {
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
        CSVConfig const &csv_config = tests::kTestDD;
        auto mp = algos::StdParamsMap({{config::names::kCsvConfig, csv_config}, {config::names::kDDString, dd}});
        auto verifier = algos::CreateAndLoadAlgorithm<algos::dd::DDVerifier>(mp);
        verifier->VerifyDD();
    }
}

// namespace tests {
//     struct DDVerifyingParams {
//     algos::StdParamsMap params;
//     long double const error = 0.;
//     size_t const num_error_pairs = 0;
//     size_t const num_error_rows = 0;
//
//     FDVerifyingParams(config::IndicesType lhs_indices, config::IndicesType rhs_indices,
//                       size_t const num_error_clusters = 0, size_t const num_error_rows = 0,
//                       long double const error = 0., CSVConfig const& csv_config = kTestFD)
//         : params({{onam::kCsvConfig, csv_config},
//                   {onam::kLhsIndices, std::move(lhs_indices)},
//                   {onam::kRhsIndices, std::move(rhs_indices)},
//                   {onam::kEqualNulls, true}}),
//           error(error),
//           num_error_clusters(num_error_clusters),
//           num_error_rows(num_error_rows) {}
// };

// class TestFDVerifying : public ::testing::TestWithParam<FDVerifyingParams> {};
//
// TEST_P(TestFDVerifying, DefaultTest) {
//     auto const& p = GetParam();
//     auto mp = algos::StdParamsMap(p.params);
//     auto verifier = algos::CreateAndLoadAlgorithm<algos::fd_verifier::FDVerifier>(mp);
//     verifier->Execute();
//     EXPECT_EQ(verifier->FDHolds(), p.num_error_clusters == 0);
//     EXPECT_DOUBLE_EQ(verifier->GetError(), p.error);
//     EXPECT_EQ(verifier->GetNumErrorRows(), p.num_error_rows);
//     EXPECT_EQ(verifier->GetNumErrorClusters(), p.num_error_clusters);
//
//     TestSorting(std::move(verifier));
// }

// clang-format off
// INSTANTIATE_TEST_SUITE_P(
//         FDVerifierTestSuite, TestFDVerifying,
//         ::testing::Values(
//             FDVerifyingParams({1}, {0}),
//             FDVerifyingParams({2}, {0}),
//             FDVerifyingParams({2}, {1}),
//             FDVerifyingParams({0, 1, 2, 3, 4}, {5}),
//             FDVerifyingParams({2, 3}, {5}),
//             FDVerifyingParams({5}, {0}),
//             FDVerifyingParams({5}, {1}),
//             FDVerifyingParams({5}, {2}),
//             FDVerifyingParams({5}, {3}),
//             FDVerifyingParams({5}, {4}),
//             FDVerifyingParams({1, 3}, {4}),
//             FDVerifyingParams({5}, {0, 1, 2, 3, 4}),
//             FDVerifyingParams({2}, {0, 1}),
//             FDVerifyingParams({2, 3}, {0, 1, 4, 5}),
//             FDVerifyingParams({2, 4}, {0, 1, 3, 5}),
//             FDVerifyingParams({3, 4}, {0, 1}),
//             FDVerifyingParams({1, 4}, {0, 3}),
//             FDVerifyingParams({1, 3}, {0, 3}),
//             FDVerifyingParams({4}, {3}, 1, 2, 2.L/132),
//             FDVerifyingParams({3}, {4}, 2, 10, 26.L/132),
//             FDVerifyingParams({0}, {1}, 1, 12, 108.L/132),
//             FDVerifyingParams({1}, {2}, 4, 12, 16.L/132),
//             FDVerifyingParams({1}, {3}, 2, 6, 8.L/132),
//             FDVerifyingParams({1}, {2, 3}, 4, 12, 18.L/132),
//             FDVerifyingParams({2}, {5}, 1, 2, 2.L/132),
//             FDVerifyingParams({1, 3}, {5}, 3, 8, 10.L/132),
//             FDVerifyingParams({1, 2}, {0, 3}, 1, 2, 2.L/132),
//             FDVerifyingParams({3, 4}, {1, 2}, 3, 8, 10.L/132),
//             FDVerifyingParams({2}, {3, 4}, 1, 2, 2.L/132),
//             FDVerifyingParams({4}, {1, 2}, 4, 10, 12.L/132),
//             FDVerifyingParams({0}, {2, 3}, 1, 12, 126.L/132),
//             FDVerifyingParams({1, 4}, {2, 3, 5}, 3, 8, 10.L/132),
//             FDVerifyingParams({0, 1}, {1, 4}, 2, 6, 8.L/132)
//             ));
// clang-format on
//
// }  // namespace tests