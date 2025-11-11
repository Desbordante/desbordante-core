#include <vector>

#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/od/fastod/model/removal_set.h"
#include "algorithms/od/set_based_verifier/verifier.h"
#include "all_csv_configs.h"

namespace tests {

namespace {

using namespace algos::od;

class SetBasedAodVerifierParams {
private:
    algos::StdParamsMap params_map_;
    RemovalSetAsVec expected_removal_set_;

public:
    SetBasedAodVerifierParams(CSVConfig const& csv_config, config::IndicesType const& oc_context,
                              config::IndexType oc_left, config::IndexType oc_right,
                              Ordering ordering, config::IndicesType const& ofd_context,
                              config::IndexType ofd_right, RemovalSetAsVec removal_set)
        : params_map_({{config::names::kCsvConfig, csv_config},
                       {config::names::kOcContext, oc_context},
                       {config::names::kOcLeftIndex, oc_left},
                       {config::names::kOcRightIndex, oc_right},
                       {config::names::kOcLeftOrdering, ordering},
                       {config::names::kOFDContext, ofd_context},
                       {config::names::kOFDRightIndex, ofd_right}}),
          expected_removal_set_(removal_set) {}

    algos::StdParamsMap const& GetParamsMap() const {
        return params_map_;
    }

    RemovalSetAsVec const& GetExpectedRemovalSet() const {
        return expected_removal_set_;
    }
};

class TestSetBasedAodVerifier : public ::testing::TestWithParam<SetBasedAodVerifierParams> {};

TEST_P(TestSetBasedAodVerifier, CorrectnessTest) {
    SetBasedAodVerifierParams const& params(GetParam());
    algos::StdParamsMap const& params_map = params.GetParamsMap();
    auto verifier = algos::CreateAndLoadAlgorithm<SetBasedAodVerifier>(params_map);
    verifier->Execute();
    EXPECT_EQ(verifier->Holds(), params.GetExpectedRemovalSet().empty());

    RemovalSet actual_removal_set = verifier->GetRemovalSet();
    RemovalSetAsVec actual_removal_set_vec(actual_removal_set.begin(), actual_removal_set.end());
    std::sort(actual_removal_set_vec.begin(), actual_removal_set_vec.end());
    RemovalSetAsVec expected_removal_set_vec = params.GetExpectedRemovalSet();
    std::sort(expected_removal_set_vec.begin(), expected_removal_set_vec.end());
    EXPECT_EQ(actual_removal_set_vec, expected_removal_set_vec);
    SetBasedAodVerifier::Error expected_error =
            static_cast<SetBasedAodVerifier::Error>(expected_removal_set_vec.size()) /
            verifier->GetTupleCount();
    EXPECT_EQ(verifier->GetError(), expected_error);
}

INSTANTIATE_TEST_SUITE_P(
        SetBasedAodVerifierTestSuite, TestSetBasedAodVerifier,
        ::testing::Values(/* {}: Col5 -> Col1 */
                          SetBasedAodVerifierParams(kTestFD, /*oc_context=*/{},
                                                    /*oc_left=*/5, /*oc_right=*/1,
                                                    /*ordering=*/Ordering::kAscending,
                                                    /*ofd_context=*/{5}, /*ofd_right=*/1,
                                                    /*removal_set=*/{}),
                          /* {}: Col5 -> Col2 */
                          SetBasedAodVerifierParams(kTestFD, /*oc_context=*/{},
                                                    /*oc_left=*/5, /*oc_right=*/2,
                                                    /*ordering=*/Ordering::kAscending,
                                                    /*ofd_context=*/{5}, /*ofd_right=*/2,
                                                    /*removal_set=*/{2, 5, 8}),
                          /* {Col3}: Col4 -> Col1 */
                          SetBasedAodVerifierParams(kTestFD, /*oc_context=*/{3},
                                                    /*oc_left=*/4, /*oc_right=*/1,
                                                    /*ordering=*/Ordering::kAscending,
                                                    /*ofd_context=*/{3, 4}, /*ofd_right=*/1,
                                                    /*removal_set=*/{}),
                          /* {Col3}: Col2<= ~ Col4<=, {Col2,Col3}: [] -> Col4 */
                          SetBasedAodVerifierParams(kTestFD, /*oc_context=*/{3},
                                                    /*oc_left=*/2, /*oc_right=*/4,
                                                    /*ordering=*/Ordering::kAscending,
                                                    /*ofd_context=*/{2, 3}, /*ofd_right=*/4,
                                                    /*removal_set=*/{}),
                          /* {Col1}: Col4>= ~ Col3<=, {}: [] -> Col0 */
                          SetBasedAodVerifierParams(kTestFD, /*oc_context=*/{1},
                                                    /*oc_left=*/4, /*oc_right=*/3,
                                                    /*ordering=*/Ordering::kDescending,
                                                    /*ofd_context=*/{}, /*ofd_right=*/0,
                                                    /*removal_set=*/{}),
                          /* {Col1}: Col4>= ~ Col3<=, {Col1}: [] -> Col5 */
                          SetBasedAodVerifierParams(kTestFD, /*oc_context=*/{1},
                                                    /*oc_left=*/4, /*oc_right=*/3,
                                                    /*ordering=*/Ordering::kDescending,
                                                    /*ofd_context=*/{1}, /*ofd_right=*/5,
                                                    /*removal_set=*/{0, 1, 5, 8, 11}),
                          /* {}: F>= ~ E<=, {B,C,D,E}: [] -> A */
                          SetBasedAodVerifierParams(kBernoulliRelation, /*oc_context=*/{},
                                                    /*oc_left=*/5, /*oc_right=*/4,
                                                    /*ordering=*/Ordering::kDescending,
                                                    /*ofd_context=*/{2, 3, 4, 5}, /*ofd_right=*/1,
                                                    /*removal_set=*/{}),
                          /* {A}: D<= ~ E<=, {A}: [] -> F */
                          SetBasedAodVerifierParams(kBernoulliRelation, /*oc_context=*/{0},
                                                    /*oc_left=*/3, /*oc_right=*/4,
                                                    /*ordering=*/Ordering::kAscending,
                                                    /*ofd_context=*/{0}, /*ofd_right=*/5,
                                                    /*removal_set=*/{3}),
                          /* {A}: D>= ~ E<=, {A}: [] -> E */
                          SetBasedAodVerifierParams(kBernoulliRelation, /*oc_context=*/{0},
                                                    /*oc_left=*/3, /*oc_right=*/4,
                                                    /*ordering=*/Ordering::kDescending,
                                                    /*ofd_context=*/{0}, /*ofd_right=*/4,
                                                    /*removal_set=*/{4}),
                          /* {}: C>= ~ B<=, {D}: [] -> C */
                          SetBasedAodVerifierParams(kBernoulliRelation, /*oc_context=*/{},
                                                    /*oc_left=*/2, /*oc_right=*/1,
                                                    /*ordering=*/Ordering::kDescending,
                                                    /*ofd_context=*/{3}, /*ofd_right=*/2,
                                                    /*removal_set=*/{0, 4, 5})));

class TestSetBasedAodVerifierWithFastod : public ::testing::TestWithParam<CSVConfig> {};

template <typename algos::od::Ordering Ordering>
void VerifyOCs(algos::od::SetBasedAodVerifier& verifier,
               std::vector<algos::fastod::CanonicalOD<Ordering>> const& ocs) {
    for (algos::fastod::CanonicalOD<Ordering> const& oc : ocs) {
        std::vector<model::ColumnIndex> oc_context = oc.GetContext().AsVector();
        model::ColumnIndex oc_left = oc.GetLeftColumn();
        model::ColumnIndex oc_right = oc.GetRightColumn();
        algos::StdParamsMap execute_params{{config::names::kOcContext, oc_context},
                                           {config::names::kOcLeftIndex, oc_left},
                                           {config::names::kOcRightIndex, oc_right},
                                           {config::names::kOcLeftOrdering, Ordering}};

        algos::ConfigureFromMap(verifier, execute_params);
        verifier.Execute();
        EXPECT_TRUE(verifier.Holds());

        // Check for false positives
        if (!oc_context.empty()) {
            oc_context.pop_back();
            execute_params[config::names::kOcContext] = oc_context;
            algos::ConfigureFromMap(verifier, execute_params);
            verifier.Execute();
            EXPECT_FALSE(verifier.Holds());
        }
    }
}

void VerifyOFDs(algos::od::SetBasedAodVerifier& verifier,
                std::vector<algos::fastod::SimpleCanonicalOD> const& ofds) {
    for (algos::fastod::SimpleCanonicalOD const& ofd : ofds) {
        std::vector<model::ColumnIndex> ofd_context = ofd.GetContext().AsVector();
        model::ColumnIndex ofd_right = ofd.GetRight();
        algos::StdParamsMap execute_params{{config::names::kOFDContext, ofd_context},
                                           {config::names::kOFDRightIndex, ofd_right}};

        algos::ConfigureFromMap(verifier, execute_params);
        verifier.Execute();
        EXPECT_TRUE(verifier.Holds());

        // Check for false positives
        if (!ofd_context.empty()) {
            ofd_context.pop_back();
            execute_params[config::names::kOFDContext] = ofd_context;
            algos::ConfigureFromMap(verifier, execute_params);
            verifier.Execute();
            EXPECT_FALSE(verifier.Holds());
        }
    }
}

TEST_P(TestSetBasedAodVerifierWithFastod, CorrectnessTest) {
    algos::StdParamsMap params{{config::names::kCsvConfig, GetParam()}};
    auto fastod = algos::CreateAndLoadAlgorithm<algos::Fastod>(params);
    fastod->Execute();
    std::vector<algos::fastod::AscCanonicalOD> ocs_asc = fastod->GetAscendingDependencies();
    std::vector<algos::fastod::DescCanonicalOD> ocs_desc = fastod->GetDescendingDependencies();
    std::vector<algos::fastod::SimpleCanonicalOD> ofds = fastod->GetSimpleDependencies();

    auto verifier = std::make_unique<algos::od::SetBasedAodVerifier>();
    algos::LoadAlgorithmData(*verifier, params);

    VerifyOCs(*verifier, ocs_asc);
    VerifyOCs(*verifier, ocs_desc);
    VerifyOFDs(*verifier, ofds);
}

INSTANTIATE_TEST_SUITE_P(TestSetBasedOdVerifierWithFastodSuite, TestSetBasedAodVerifierWithFastod,
                         ::testing::Values(kTestFD, kAbalone, kBernoulliRelation, kOdTestNormOd,
                                           kOdTestNormSmall2x3, kOdTestNormSmall3x3,
                                           kOdTestNormAbalone, kWdcAstrology, kWdcGame, kWdcPlanetz,
                                           kWdcSymbols, kIris, kBreastCancer));

}  // namespace

}  // namespace tests
