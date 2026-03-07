#include <cstddef>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/cfd/cfun/cfun.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"

namespace tests {

struct CFUNParams {
    using FD = std::string;
    using Tableau = std::vector<std::string>;
    using Excepted_CFD = std::pair<FD, Tableau>;

    algos::StdParamsMap params;
    std::set<Excepted_CFD> excepted_cfds;

    CFUNParams(CSVConfig const& csv_config, unsigned int min_supp, std::set<Excepted_CFD> excepted)
        : params({{config::names::kCfdMinimumSupport, min_supp},
                  {config::names::kCsvConfig, csv_config}}),
          excepted_cfds(std::move(excepted)) {}
};

class CFUNTest : public ::testing::TestWithParam<CFUNParams> {};

static void CheckEqualityExceptedCFDs(std::set<CFUNParams::Excepted_CFD> const& expected,
                                      std::list<algos::cfd::cfun::CCFD> const& actual) {
    ASSERT_EQ(actual.size(), expected.size()) << "count of cfds does not match: expected "
                                              << expected.size() << ", got " << actual.size();

    for (auto const& ccfd : actual) {
        auto embedded_fd = ccfd.GetEmbeddedFd().ToLongString();

        std::vector<std::string> patterns;
        for (auto const& pattern : ccfd.GetTableau()) {
            if (pattern.empty()) {
                continue;
            }

            std::ostringstream os;
            for (size_t i = 0; i < pattern.size() - 1; ++i) {
                os << "." << pattern[i] << ".";
            }

            os << "| (" << pattern.back() << ")";
            patterns.push_back(os.str());
        }
        std::pair<std::string, std::vector<std::string>> expected_cfd = {std::move(embedded_fd),
                                                                         std::move(patterns)};
        if (expected.find(expected_cfd) == expected.end()) {
            FAIL() << "generated cfd not found in expected";
        }
    }
    SUCCEED();
}

TEST_P(CFUNTest, Test) {
    auto const& p = GetParam();
    auto mp = algos::StdParamsMap(p.params);
    auto algo = algos::CreateAndLoadAlgorithm<algos::cfd::cfun::CFUN>(mp);
    algo->Execute();

    std::ofstream output_file("/home/oddin60/Work/desbordante-core/cfds.txt", std::ios::trunc);
    for (auto const& cfd : algo->GetCFDList()) {
        output_file << cfd.ToString();
    }

    CheckEqualityExceptedCFDs(p.excepted_cfds, algo->GetCFDList());
}

INSTANTIATE_TEST_SUITE_P(
        CFUNParamTest, CFUNTest,
        ::testing::Values(
                CFUNParams({kTennis,
                            3,
                            {{"[temp] -> humidity", {".cool.| (normal)"}},
                             {"[outlook humidity] -> play", {".sunny..high.| (no)"}},
                             {"[outlook windy] -> play", {".rainy..false.| (yes)"}},
                             {"[outlook play] -> humidity", {".sunny..no.| (high)"}},
                             {"[outlook play] -> windy", {".rainy..yes.| (false)"}},
                             {"[humidity windy] -> play", {".normal..false.| (yes)"}},
                             {"[outlook] -> play", {".overcast.| (yes)"}}}}),
                CFUNParams({kTennis,
                            4,
                            {{"[outlook] -> play", {".overcast.| (yes)"}},
                             {"[temp] -> humidity", {".cool.| (normal)"}},
                             {"[humidity windy] -> play", {".normal..false.| (yes)"}}}}),
                CFUNParams({kMushroom,
                            4500,
                            {{"[] -> veil-type", {"| (p)"}},
                             {"[stalk-shape] -> gill-attachment", {".t.| (f)"}},
                             {"[stalk-shape] -> veil-color", {".t.| (w)"}},
                             {"[stalk-shape] -> ring-number", {".t.| (o)"}},
                             {"[gill-attachment gill-spacing] -> veil-color", {".f..c.| (w)"}},
                             {"[gill-attachment gill-size] -> veil-color", {".f..b.| (w)"}},
                             {"[gill-attachment stalk-surface-above-ring] -> veil-color",
                              {".f..s.| (w)"}},
                             {"[gill-attachment stalk-surface-below-ring] -> veil-color",
                              {".f..s.| (w)"}},
                             {"[stalk-surface-above-ring veil-color] -> gill-attachment",
                              {".s..w.| (f)"}},
                             {"[stalk-surface-below-ring veil-color] -> gill-attachment",
                              {".s..w.| (f)"}},
                             {"[veil-color ring-number] -> gill-attachment", {".w..o.| (f)"}}}}),
                CFUNParams({kMushroom,
                            4000,
                            {{"[] -> veil-type", {"| (p)"}},
                             {"[stalk-shape] -> gill-attachment", {".t.| (f)"}},
                             {"[stalk-shape] -> veil-color", {".t.| (w)"}},
                             {"[stalk-shape] -> ring-number", {".t.| (o)"}},
                             {"[gill-attachment gill-spacing] -> veil-color", {".f..c.| (w)"}},
                             {"[gill-attachment gill-size] -> veil-color", {".f..b.| (w)"}},
                             {"[gill-attachment stalk-surface-above-ring] -> veil-color",
                              {".f..s.| (w)"}},
                             {"[gill-attachment stalk-surface-below-ring] -> veil-color",
                              {".f..s.| (w)"}},
                             {"[stalk-surface-above-ring veil-color] -> gill-attachment",
                              {".s..w.| (f)"}},
                             {"[stalk-surface-below-ring veil-color] -> gill-attachment",
                              {".s..w.| (f)"}},
                             {"[veil-color ring-number] -> gill-attachment", {".w..o.| (f)"}},

                             {"[stalk-color-above-ring] -> gill-attachment", {".w.| (f)"}},
                             {"[stalk-color-above-ring] -> veil-color", {".w.| (w)"}},
                             {"[stalk-color-below-ring] -> gill-attachment", {".w.| (f)"}},
                             {"[stalk-color-below-ring] -> veil-color", {".w.| (w)"}},
                             {"[edible gill-attachment] -> veil-color", {".e..f.| (w)"}},
                             {"[edible veil-color] -> gill-attachment", {".e..w.| (f)"}}}})));
}  // namespace tests
