#include <iomanip>
#include <iostream>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/fd/tane/afd_measures.h"
#include "core/algorithms/fd/tane/enums.h"
#include "core/algorithms/fd/tane/tane.h"
#include "core/config/names.h"
#include "core/model/table/column_layout_relation_data.h"
#include "core/parser/csv_parser/csv_parser.h"
#include "tests/common/all_csv_configs.h"

namespace tests {
namespace onam = config::names;

struct AFD {
    std::size_t lhs_id;
    std::size_t rhs_id;
    config::ErrorType expected_error;
};

struct TaneMiningParams {
    algos::StdParamsMap params;
    unsigned int result_hash;

    TaneMiningParams(unsigned int result_hash, config::ErrorType error,
                     algos::AfdErrorMeasure error_measure, CSVConfig const& csv_config)
        : params({{onam::kCsvConfig, csv_config},
                  {onam::kError, error},
                  {onam::kAfdErrorMeasure, error_measure}}),
          result_hash(result_hash) {}
};

struct TaneValidationParams {
    algos::AfdErrorMeasure error_measure;
    std::vector<AFD> afds;
    CSVConfig csv_config;
};

struct ColumnErr {
    std::size_t column_id;
    config::ErrorType expected_error;
};

struct PdepSelfValidationParams {
    std::vector<ColumnErr> errors;
    CSVConfig csv_config;
};

class TestTanePdepSelfValidation : public ::testing::TestWithParam<PdepSelfValidationParams> {};

class TestTaneAfdMeasuresValidation : public ::testing::TestWithParam<TaneValidationParams> {};

class TestTaneAfdMeasuresMining : public ::testing::TestWithParam<TaneMiningParams> {};

TEST_P(TestTanePdepSelfValidation, SelfCalculationTest) {
    auto const& p = GetParam();
    double eps = 0.001;
    auto table = std::make_shared<CSVParser>(p.csv_config);
    auto relation = ColumnLayoutRelationData::CreateFrom(*table, true);
    for (auto const& [column_id, expected_error] : p.errors) {
        auto const& column_pli = relation->GetColumnData(column_id).GetPositionListIndex();
        config::ErrorType error = algos::PdepSelf(column_pli);
        EXPECT_NEAR(error, expected_error, eps)
                << "column_id = " << column_id << "\n"
                << "error = " << error << "\n"
                << "expected_error = " << expected_error << std::endl;
    }
}

TEST_P(TestTaneAfdMeasuresValidation, ErrorCalculationTest) {
    auto const& p = GetParam();
    double eps = 0.00001;
    auto table = std::make_shared<CSVParser>(p.csv_config);
    auto relation = ColumnLayoutRelationData::CreateFrom(*table, true);
    for (auto const& [lhs_id, rhs_id, expected_error] : p.afds) {
        auto const& lhs = relation->GetColumnData(lhs_id).GetPositionListIndex();
        auto const& rhs = relation->GetColumnData(rhs_id).GetPositionListIndex();
        config::ErrorType error;
        switch (p.error_measure) {
            case algos::AfdErrorMeasure::kPdep:
                error = algos::CalculatePdepMeasure(lhs, lhs->Intersect(rhs).get());
                break;
            case algos::AfdErrorMeasure::kTau:
                error = algos::CalculateTauMeasure(lhs, rhs, lhs->Intersect(rhs).get());
                break;
            case algos::AfdErrorMeasure::kMuPlus:
                error = algos::CalculateMuPlusMeasure(lhs, rhs, lhs->Intersect(rhs).get());
                break;
            case algos::AfdErrorMeasure::kRho:
                error = algos::CalculateRhoMeasure(lhs, lhs->Intersect(rhs).get());
                break;
            default:
                FAIL();
        }
        EXPECT_NEAR(error, expected_error, eps)
                << "lhs_id = " << lhs_id << "\n"
                << "rhs_id = " << rhs_id << "\n"
                << "error = " << error << "\n"
                << "expected_error = " << expected_error << std::endl;
    }
}

TEST_P(TestTaneAfdMeasuresMining, DefaultTest) {
    auto const& p = GetParam();
    auto algos = algos::CreateAndLoadAlgorithm<algos::Tane>(p.params);
    algos->Execute();
    EXPECT_EQ(p.result_hash, algos->Fletcher16());
}

INSTANTIATE_TEST_SUITE_P(PdepSelfTaneValidationSuite, TestTanePdepSelfValidation,
                         ::testing::Values(PdepSelfValidationParams({{0, 1.0},
                                                                     {1, 0.25},
                                                                     {2, 0.138},
                                                                     {3, 0.375},
                                                                     {4, 0.208},
                                                                     {5, 0.125}},
                                                                    kTestFD),
                                           PdepSelfValidationParams({{0, 0.0396},
                                                                     {1, 0.00578},
                                                                     {2, 0.037},
                                                                     {3, 0.006},
                                                                     {4, 0.069},
                                                                     {5, 0.0466},
                                                                     {6, 0.0757},
                                                                     {7, 0.309}},
                                                                    kWdcSatellites)));

INSTANTIATE_TEST_SUITE_P(
        AfdMeasuresTaneValidationSuite, TestTaneAfdMeasuresValidation,
        ::testing::Values(
                TaneValidationParams(algos::AfdErrorMeasure::kPdep,
                                     {{0, 1, 0.25},
                                      {0, 2, 0.13888888888888887},
                                      {0, 3, 0.3749999999999999},
                                      {0, 4, 0.20833333333333331},
                                      {0, 5, 0.12499999999999997},
                                      {1, 0, 1.0},
                                      {1, 2, 0.5555555555555555},
                                      {1, 3, 0.7777777777777778},
                                      {1, 4, 0.7777777777777778},
                                      {1, 5, 0.4999999999999999},
                                      {2, 0, 1.0},
                                      {2, 1, 1.0},
                                      {2, 3, 0.9166666666666666},
                                      {2, 4, 0.9166666666666666},
                                      {2, 5, 0.9166666666666666},
                                      {3, 0, 1.0},
                                      {3, 1, 0.5833333333333333},
                                      {3, 2, 0.4305555555555556},
                                      {3, 4, 0.5833333333333333},
                                      {3, 5, 0.4305555555555556},
                                      {4, 0, 1.0},
                                      {4, 1, 0.9166666666666666},
                                      {4, 2, 0.6111111111111109},
                                      {4, 3, 0.9166666666666666},
                                      {4, 5, 0.6111111111111109},
                                      {5, 0, 1.0},
                                      {5, 1, 1.0},
                                      {5, 2, 1.0},
                                      {5, 3, 1.0},
                                      {5, 4, 1.0}},
                                     kTestFD),
                TaneValidationParams(algos::AfdErrorMeasure::kPdep,
                                     {{0, 1, 0.32647619047619053}, {0, 2, 0.29765079365079367},
                                      {0, 3, 0.35943386243386244}, {0, 4, 0.6805925925925925},
                                      {1, 0, 0.21734432234432235}, {1, 2, 0.20246682946682948},
                                      {1, 3, 0.2695738705738706},  {1, 4, 0.5296988196988197},
                                      {2, 0, 0.35180952380952385}, {2, 1, 0.3610317460317461},
                                      {2, 3, 0.4651269841269841},  {2, 4, 0.9373333333333334},
                                      {3, 0, 0.207989010989011},   {3, 1, 0.24796214896214902},
                                      {3, 2, 0.24009035409035417}, {3, 4, 0.9372222222222224},
                                      {4, 0, 0.07680000000000001}, {4, 1, 0.10613333333333334},
                                      {4, 2, 0.10933333333333332}, {4, 3, 0.21973333333333336}},
                                     kIris),
                TaneValidationParams(algos::AfdErrorMeasure::kTau,
                                     {{0, 1, 0.0},
                                      {0, 2, -3.2232281360085186e-17},
                                      {0, 3, -8.881784197001253e-17},
                                      {0, 4, -7.01193489236941e-17},
                                      {0, 5, -3.172065784643304e-17},
                                      {1, 0, 1},
                                      {1, 2, 0.48387096774193533},
                                      {1, 3, 0.6444444444444446},
                                      {1, 4, 0.7192982456140351},
                                      {1, 5, 0.42857142857142844},
                                      {2, 0, 1},
                                      {2, 1, 1.0},
                                      {2, 3, 0.8666666666666668},
                                      {2, 4, 0.894736842105263},
                                      {2, 5, 0.9047619047619048},
                                      {3, 0, 1},
                                      {3, 1, 0.44444444444444436},
                                      {3, 2, 0.3387096774193548},
                                      {3, 4, 0.47368421052631565},
                                      {3, 5, 0.34920634920634924},
                                      {4, 0, 1},
                                      {4, 1, 0.8888888888888888},
                                      {4, 2, 0.5483870967741933},
                                      {4, 3, 0.8666666666666668},
                                      {4, 5, 0.5555555555555554},
                                      {5, 0, 1},
                                      {5, 1, 1.0},
                                      {5, 2, 1.0},
                                      {5, 3, 1.0},
                                      {5, 4, 1.0}},
                                     kTestFD),
                TaneValidationParams(algos::AfdErrorMeasure::kTau,
                                     {{0, 1, 0.26903889087952376},  {0, 2, 0.2692658308121177},
                                      {0, 3, 0.3060122257685817},   {0, 4, 0.5208888888888887},
                                      {1, 0, 0.18473366910866912},  {1, 2, 0.1702350718118775},
                                      {1, 3, 0.20865813212211523},  {1, 4, 0.29454822954822957},
                                      {2, 0, 0.32480158730158737},  {2, 1, 0.30654130261018164},
                                      {2, 3, 0.4205198932423509},   {2, 4, 0.906},
                                      {3, 0, 0.1749885531135531},   {3, 1, 0.18382926643104153},
                                      {3, 2, 0.2093791254523707},   {3, 4, 0.9058333333333336},
                                      {4, 0, 0.038333333333333344}, {4, 1, 0.029905460158209544},
                                      {4, 2, 0.07333764912605197},  {4, 3, 0.15466101694915257}},
                                     kIris),
                TaneValidationParams(algos::AfdErrorMeasure::kMuPlus,
                                     {{0, 1, 0.0},
                                      {0, 2, 0.0},
                                      {0, 3, 0.0},
                                      {0, 4, 0.0},
                                      {0, 5, 0.0},
                                      {1, 0, 1},
                                      {1, 2, 0.29032258064516114},
                                      {1, 3, 0.5111111111111112},
                                      {1, 4, 0.6140350877192983},
                                      {1, 5, 0.2142857142857142},
                                      {2, 0, 1},
                                      {2, 1, 1.0},
                                      {2, 3, 0.6333333333333332},
                                      {2, 4, 0.7105263157894736},
                                      {2, 5, 0.7380952380952379},
                                      {3, 0, 1},
                                      {3, 1, 0.23611111111111094},
                                      {3, 2, 0.09072580645161299},
                                      {3, 4, 0.27631578947368407},
                                      {3, 5, 0.10515873015873012},
                                      {4, 0, 1},
                                      {4, 1, 0.8253968253968254},
                                      {4, 2, 0.29032258064516103},
                                      {4, 3, 0.7904761904761903},
                                      {4, 5, 0.3015873015873013},
                                      {5, 0, 1},
                                      {5, 1, 1.0},
                                      {5, 2, 1.0},
                                      {5, 3, 1.0},
                                      {5, 4, 1.0}},
                                     kTestFD),
                TaneValidationParams(algos::AfdErrorMeasure::kMuPlus,
                                     {{0, 1, 0.052928649922165616}, {0, 2, 0.053222685139178605},
                                      {0, 3, 0.10083323164798841},  {0, 4, 0.37923864734299506},
                                      {1, 0, 0.0435064306865488},   {1, 2, 0.026496265354092552},
                                      {1, 3, 0.0715752888676785},   {1, 4, 0.1723439858479231},
                                      {2, 0, 0.05977043465361209},  {2, 1, 0.03434256157866411},
                                      {2, 3, 0.19306041208514257},  {2, 4, 0.8691028037383178},
                                      {3, 0, 0.039635112608745415}, {3, 1, 0.04992625545488438},
                                      {3, 2, 0.0796678882219003},   {3, 4, 0.8903841145833337},
                                      {4, 0, 0.025249433106575903}, {4, 1, 0.016706894990293986},
                                      {4, 2, 0.060729998093753235}, {4, 3, 0.14315980629539937}},
                                     kIris),
                TaneValidationParams(algos::AfdErrorMeasure::kRho,
                                     {{0, 1, 0.25},
                                      {0, 2, 0.125},
                                      {0, 3, 0.25},
                                      {0, 4, 0.2},
                                      {0, 5, 0.1111111111111111},
                                      {1, 0, 1.0},
                                      {1, 2, 0.5},
                                      {1, 3, 0.6666666666666666},
                                      {1, 4, 0.6666666666666666},
                                      {1, 5, 0.4444444444444444},
                                      {2, 0, 1.0},
                                      {2, 1, 1.0},
                                      {2, 3, 0.8888888888888888},
                                      {2, 4, 0.8888888888888888},
                                      {2, 5, 0.8888888888888888},
                                      {3, 0, 1.0},
                                      {3, 1, 0.6666666666666666},
                                      {3, 2, 0.4444444444444444},
                                      {3, 4, 0.6666666666666666},
                                      {3, 5, 0.4444444444444444},
                                      {4, 0, 1.0},
                                      {4, 1, 0.8333333333333334},
                                      {4, 2, 0.5555555555555556},
                                      {4, 3, 0.8333333333333334},
                                      {4, 5, 0.5555555555555556},
                                      {5, 0, 1.0},
                                      {5, 1, 1.0},
                                      {5, 2, 1.0},
                                      {5, 3, 1.0},
                                      {5, 4, 1.0}},
                                     kTestFD),
                TaneValidationParams(algos::AfdErrorMeasure::kRho,
                                     {{0, 1, 0.3017241379310345},
                                      {0, 2, 0.2845528455284553},
                                      {0, 3, 0.3181818181818182},
                                      {0, 4, 0.6140350877192983},
                                      {1, 0, 0.19827586206896552},
                                      {1, 2, 0.1885245901639344},
                                      {1, 3, 0.23711340206185566},
                                      {1, 4, 0.5348837209302325},
                                      {2, 0, 0.34959349593495936},
                                      {2, 1, 0.3524590163934426},
                                      {2, 3, 0.4215686274509804},
                                      {2, 4, 0.8958333333333334},
                                      {3, 0, 0.2},
                                      {3, 1, 0.2268041237113402},
                                      {3, 2, 0.21568627450980393},
                                      {3, 4, 0.8148148148148148},
                                      {4, 0, 0.05263157894736842},
                                      {4, 1, 0.06976744186046512},
                                      {4, 2, 0.0625},
                                      {4, 3, 0.1111111111111111}},
                                     kIris)));

INSTANTIATE_TEST_SUITE_P(
        AfdMeasuresTaneMiningSuite, TestTaneAfdMeasuresMining,
        ::testing::Values(
                TaneMiningParams(3325, 0.3, algos::AfdErrorMeasure::kPdep, kTestFD),
                TaneMiningParams(19266, 0.174, algos::AfdErrorMeasure::kPdep, kIris),
                TaneMiningParams(18528, 0.1, algos::AfdErrorMeasure::kPdep, kIris),
                TaneMiningParams(31178, 0.15, algos::AfdErrorMeasure::kPdep, kNeighbors10k),
                TaneMiningParams(1182, 0.21, algos::AfdErrorMeasure::kPdep, kNeighbors10k),
                TaneMiningParams(33180, 0.01, algos::AfdErrorMeasure::kTau, kTestFD),
                TaneMiningParams(11680, 0.1, algos::AfdErrorMeasure::kTau, kIris),
                TaneMiningParams(60896, 0.01, algos::AfdErrorMeasure::kTau, kIris),
                TaneMiningParams(52638, 0.1, algos::AfdErrorMeasure::kTau, kNeighbors10k),
                TaneMiningParams(44991, 0.01, algos::AfdErrorMeasure::kTau, kNeighbors10k),
                TaneMiningParams(33180, 0.01, algos::AfdErrorMeasure::kMuPlus, kTestFD),
                TaneMiningParams(60841, 0.1, algos::AfdErrorMeasure::kMuPlus, kIris),
                TaneMiningParams(60896, 0.01, algos::AfdErrorMeasure::kMuPlus, kIris),
                TaneMiningParams(12185, 0.1, algos::AfdErrorMeasure::kMuPlus, kNeighbors10k),
                TaneMiningParams(12185, 0.01, algos::AfdErrorMeasure::kMuPlus, kNeighbors10k),
                TaneMiningParams(33180, 0.01, algos::AfdErrorMeasure::kRho, kTestFD),
                TaneMiningParams(11873, 0.1, algos::AfdErrorMeasure::kRho, kIris),
                TaneMiningParams(47878, 0.01, algos::AfdErrorMeasure::kRho, kIris),
                TaneMiningParams(52638, 0.1, algos::AfdErrorMeasure::kRho, kNeighbors10k),
                TaneMiningParams(52638, 0.01, algos::AfdErrorMeasure::kRho, kNeighbors10k)));

}  // namespace tests
