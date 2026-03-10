#pragma once

#include <algorithm>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/fd/fd_algorithm.h"
#include "core/algorithms/fd/multi_attr_rhs_fd_storage.h"
#include "core/config/error/type.h"
#include "core/config/names.h"
#include "core/util/bitset_utils.h"
#include "tests/common/all_csv_configs.h"
#include "tests/common/csv_config_util.h"

namespace tests {
inline bool NoFDsFound(algos::MultiAttrRhsFdStorage const& storage) {
    return storage.GetStripped().empty();
}

inline bool NoFDsFound(algos::SingleAttrRhsFdStorage const& storage) {
    return std::ranges::all_of(storage.GetStripped(),
                               [](auto& attr_fds) { return attr_fds.empty(); });
}

inline unsigned int Fletcher16(std::string str) {
    unsigned int sum1 = 0, sum2 = 0, modulus = 255;
    for (auto ch : str) {
        sum1 = (sum1 + ch) % modulus;
        sum2 = (sum2 + sum1) % modulus;
    }
    return (sum2 << 8) | sum1;
}

inline static std::string ToIndicesString(boost::dynamic_bitset<> const& lhs) {
    std::string result = "[";

    if (lhs.find_first() == boost::dynamic_bitset<>::npos) {
        return "[]";
    }

    for (size_t index = lhs.find_first(); index != boost::dynamic_bitset<>::npos;
         index = lhs.find_next(index)) {
        result += std::to_string(index);
        if (lhs.find_next(index) != boost::dynamic_bitset<>::npos) {
            result += ',';
        }
    }

    result += ']';

    return result;
}

inline std::vector<std::string> FDToJsonStrings(algos::MultiAttrRhsStrippedFd const& fd) {
    std::vector<std::string> strings;
    strings.reserve(fd.rhs.count());
    auto pre_rhs_portion = "{\"lhs\": " + ToIndicesString(fd.lhs) + ", \"rhs\": ";
    util::ForEachIndex(fd.rhs, [&](model::Index i) {
        strings.push_back(pre_rhs_portion + std::to_string(i) + '}');
    });
    return strings;
}

inline std::string FDToJsonString(algos::SingleAttrRhsStrippedFd const& fd,
                                  model::Index rhs_index) {
    return "{\"lhs\": " + ToIndicesString(fd.lhs) + ", \"rhs\": " + std::to_string(rhs_index) + '}';
}

inline std::string FDsToJson(algos::MultiAttrRhsFdStorage const& fd_storage) {
    std::string result = "{\"fds\": [";
    std::vector<std::string> discovered_fd_strings;
    // FDs used to always have one attribute, this is for consistency with the old hashes.
    for (algos::MultiAttrRhsStrippedFd const& fd : fd_storage.GetStripped()) {
        for (std::string const& single_rhs_fd_string : FDToJsonStrings(fd)) {
            discovered_fd_strings.push_back(single_rhs_fd_string);
        }
    }
    std::sort(discovered_fd_strings.begin(), discovered_fd_strings.end());
    for (std::string const& fd : discovered_fd_strings) {
        result += fd + ",";
    }
    if (result.back() == ',') {
        result.erase(result.size() - 1);
    }
    result += "]}";
    return result;
}

inline std::string FDsToJson(algos::SingleAttrRhsFdStorage const& fd_storage) {
    std::vector<std::string> discovered_fd_strings;
    model::Index rhs_index = 0;
    for (std::deque<algos::SingleAttrRhsStrippedFd> const& attr_fds : fd_storage.GetStripped()) {
        for (algos::SingleAttrRhsStrippedFd const& stripped_fd : attr_fds) {
            discovered_fd_strings.push_back(FDToJsonString(stripped_fd, rhs_index));
        }
        ++rhs_index;
    }
    std::sort(discovered_fd_strings.begin(), discovered_fd_strings.end());
    std::string result = "{\"fds\": [";
    for (std::string const& fd : discovered_fd_strings) {
        result += fd + ",";
    }
    if (result.back() == ',') {
        result.erase(result.size() - 1);
    }
    result += "]}";
    return result;
}

testing::AssertionResult CheckFdCollectionEquality(
        std::set<std::pair<std::vector<unsigned int>, unsigned int>> expected,
        algos::MultiAttrRhsFdStorage const& actual) {
    for (algos::MultiAttrRhsStrippedFd const& fd : actual.GetStripped()) {
        std::vector<unsigned int> lhs_indices = util::BitsetToIndices<unsigned int>(fd.lhs);

        for (auto index = fd.rhs.find_first(); index != boost::dynamic_bitset<>::npos;
             index = fd.rhs.find_next(index)) {
            if (auto it = expected.find(std::make_pair(lhs_indices, index)); it == expected.end()) {
                return testing::AssertionFailure()
                       << "discovered a false FD: " << ToIndicesString(fd.lhs) << "->" << index;
            } else {
                expected.erase(it);
            }
        }
    }
    return expected.empty() ? testing::AssertionSuccess()
                            : testing::AssertionFailure() << "some FDs remain undiscovered";
}

inline testing::AssertionResult CheckFdCollectionEquality(
        std::set<std::pair<std::vector<unsigned int>, unsigned int>> expected,
        algos::SingleAttrRhsFdStorage const& actual) {
    model::Index rhs_index = 0;
    for (std::deque<algos::SingleAttrRhsStrippedFd> const& attr_fds : actual.GetStripped()) {
        for (algos::SingleAttrRhsStrippedFd const& stripped_fd : attr_fds) {
            std::vector<unsigned int> lhs_indices =
                    util::BitsetToIndices<unsigned int>(stripped_fd.lhs);
            if (auto it = expected.find(std::make_pair(lhs_indices, rhs_index));
                it == expected.end()) {
                return testing::AssertionFailure()
                       << "discovered a false FD: " << ToIndicesString(stripped_fd.lhs) << "->"
                       << rhs_index;
            } else {
                expected.erase(it);
            }
        }
        ++rhs_index;
    }
    return expected.empty() ? testing::AssertionSuccess()
                            : testing::AssertionFailure() << "some FDs remain undiscovered";
}

std::set<std::pair<std::vector<unsigned int>, unsigned int>> FDsToSet(
        algos::MultiAttrRhsFdStorage const& storage) {
    std::set<std::pair<std::vector<unsigned int>, unsigned int>> set;
    for (auto const& fd : storage.GetStripped()) {
        util::ForEachIndex(fd.rhs, [&](model::Index i) {
            set.emplace(util::BitsetToIndices<unsigned int>(fd.lhs), i);
        });
    }
    return set;
}

std::set<std::pair<std::vector<unsigned int>, unsigned int>> FDsToSet(
        algos::SingleAttrRhsFdStorage const& storage) {
    std::set<std::pair<std::vector<unsigned int>, unsigned int>> set;
    model::Index rhs_index = 0;
    for (std::deque<algos::SingleAttrRhsStrippedFd> const& attr_fds : storage.GetStripped()) {
        for (algos::SingleAttrRhsStrippedFd const& stripped_fd : attr_fds) {
            set.emplace(util::BitsetToIndices<unsigned int>(stripped_fd.lhs), rhs_index);
        }
        ++rhs_index;
    }
    return set;
}

template <typename T>
class AlgorithmTest : public ::testing::Test {
protected:
    static std::unique_ptr<algos::FDAlgorithm> CreateAndConfToLoad(CSVConfig const& csv_config) {
        using namespace config::names;
        using algos::ConfigureFromMap, algos::StdParamsMap;

        std::unique_ptr<algos::FDAlgorithm> algorithm = std::make_unique<T>();
        ConfigureFromMap(*algorithm, StdParamsMap{{kTable, MakeInputTable(csv_config)}});
        return algorithm;
    }

    static algos::StdParamsMap GetParamMap(
            CSVConfig const& csv_config,
            unsigned int max_lhs_ = std::numeric_limits<unsigned int>::max()) {
        using namespace config::names;
        return {
                {kCsvConfig, csv_config},
                {kError, config::ErrorType{0.0}},
                {kSeed, decltype(algos::pyro::Parameters::seed){0}},
                {kMaximumLhs, max_lhs_},
        };
    }

    static void PerformConsistentHashTestOn(std::vector<CSVConfigHash> const& config_hashes) {
        try {
            for (auto const& [csv_config, hash] : config_hashes) {
                auto algorithm = CreateAlgorithmInstance(csv_config);
                algorithm->Execute();
                EXPECT_EQ(algorithm->Fletcher16(), hash)
                        << "FD collection hash changed for " << csv_config.path.filename();
            }
        } catch (std::runtime_error& e) {
            std::cout << "Exception raised in test: " << e.what() << std::endl;
            FAIL();
        }
        SUCCEED();
    }

public:
    static std::unique_ptr<algos::FDAlgorithm> CreateAlgorithmInstance(
            CSVConfig const& config,
            unsigned int max_lhs = std::numeric_limits<unsigned int>::max()) {
        return algos::CreateAndLoadAlgorithm<T>(GetParamMap(config, max_lhs));
    }

    inline static std::vector<CSVConfigHash> const kLightDatasets = {
            {{tests::kCIPublicHighway10k, 33398},
             {tests::kNeighbors10k, 43368},
             {tests::kWdcAstronomical, 22281},
             {tests::kWdcAge, 19620},
             {tests::kWdcAppearances, 25827},
             {tests::kWdcAstrology, 40815},
             {tests::kWdcGame, 6418},
             {tests::kWdcScience, 19620},
             {tests::kWdcSymbols, 28289},
             {tests::kBreastCancer, 15121},
             {tests::kWdcKepler, 63730}}};

    inline static std::vector<CSVConfigHash> const kHeavyDatasets = {
            {{tests::kAdult, 23075},
             {tests::kCIPublicHighway, 13035},
             {tests::kEpicMeds, 50218},
             {tests::kEpicVitals, 2083},
             {tests::kIowa1kk, 28573},
             {tests::kLegacyPayors, 43612}}};
};

template <typename T>
class FdDiscoveryTest : public ::testing::Test {
protected:
    static std::unique_ptr<algos::Algorithm> CreateAndConfToLoad(CSVConfig const& csv_config) {
        using namespace config::names;
        using algos::ConfigureFromMap, algos::StdParamsMap;

        std::unique_ptr<algos::Algorithm> algorithm = std::make_unique<T>();
        ConfigureFromMap(*algorithm, StdParamsMap{{kTable, MakeInputTable(csv_config)}});
        return algorithm;
    }

    static algos::StdParamsMap GetParamMap(
            CSVConfig const& csv_config,
            unsigned int max_lhs_ = std::numeric_limits<unsigned int>::max()) {
        using namespace config::names;
        return {
                {kCsvConfig, csv_config},
                {kError, config::ErrorType{0.0}},
                {kSeed, decltype(algos::pyro::Parameters::seed){0}},
                {kMaximumLhs, max_lhs_},
        };
    }

    static void PerformConsistentHashTestOn(std::vector<CSVConfigHash> const& config_hashes) {
        using namespace algos;
        try {
            for (auto const& [csv_config, hash] : config_hashes) {
                auto algorithm = CreateAlgorithmInstance(csv_config);
                algorithm->Execute();
                std::string fds_string = FDsToJson(*algorithm->GetFdStorage());
                EXPECT_EQ(Fletcher16(fds_string), hash)
                        << "FD collection hash changed for " << csv_config.path.filename();
            }
        } catch (std::runtime_error& e) {
            std::cout << "Exception raised in test: " << e.what() << std::endl;
            FAIL();
        }
        SUCCEED();
    }

public:
    static auto CreateAlgorithmInstance(
            CSVConfig const& config,
            unsigned int max_lhs = std::numeric_limits<unsigned int>::max()) {
        return algos::CreateAndLoadAlgorithm<T>(GetParamMap(config, max_lhs));
    }

    inline static std::vector<CSVConfigHash> const kLightDatasets = {
            {{tests::kCIPublicHighway10k, 33398},
             {tests::kNeighbors10k, 43368},
             {tests::kWdcAstronomical, 22281},
             {tests::kWdcAge, 19620},
             {tests::kWdcAppearances, 25827},
             {tests::kWdcAstrology, 40815},
             {tests::kWdcGame, 6418},
             {tests::kWdcScience, 19620},
             {tests::kWdcSymbols, 28289},
             {tests::kBreastCancer, 15121},
             {tests::kWdcKepler, 63730}}};

    inline static std::vector<CSVConfigHash> const kHeavyDatasets = {
            {{tests::kAdult, 23075},
             {tests::kCIPublicHighway, 13035},
             {tests::kEpicMeds, 50218},
             {tests::kEpicVitals, 2083},
             {tests::kIowa1kk, 28573},
             {tests::kLegacyPayors, 43612}}};
};

template <typename T>
class ApproximateFDTest : public ::testing::Test {
protected:
    // alias for choosing datasets' hashes specialization
    using AlgorithmType = T;

    static std::unique_ptr<T> CreateAndConfToLoad(CSVConfig const& csv_config) {
        using namespace config::names;
        using algos::ConfigureFromMap, algos::StdParamsMap;

        std::unique_ptr<T> algorithm = std::make_unique<T>();
        ConfigureFromMap(*algorithm, StdParamsMap{{kTable, MakeInputTable(csv_config)}});
        return algorithm;
    }

    static algos::StdParamsMap GetParamMap(CSVConfig const& csv_config) {
        using namespace config::names;
        return {
                {kCsvConfig, csv_config},
                {kCustomRandom, std::optional<int>{47}},
        };
    }

    static void PerformConsistentHashTestOn(std::vector<CSVConfigHash> const& config_hashes) {
        try {
            for (auto const& [csv_config, hash] : config_hashes) {
                auto algorithm = CreateAlgorithmInstance(csv_config);
                algorithm->Execute();
                std::string fds_string = FDsToJson(*algorithm->GetFdStorage());
                EXPECT_EQ(Fletcher16(fds_string), hash)
                        << "FD collection hash changed for " << csv_config.path.filename();
            }
        } catch (std::runtime_error& e) {
            std::cout << "Exception raised in test: " << e.what() << std::endl;
            FAIL();
        }
        SUCCEED();
    }

public:
    static std::unique_ptr<T> CreateAlgorithmInstance(CSVConfig const& config) {
        return algos::CreateAndLoadAlgorithm<T>(GetParamMap(config));
    }
};

// testing data
template <typename Alg>
struct ApproximateDatasets {
    inline static std::vector<CSVConfigHash> const kLightDatasets;
    inline static std::vector<CSVConfigHash> const kHeavyDatasets;
};

// specialization fd for EulerFD
template <>
struct ApproximateDatasets<algos::EulerFD> {
    inline static std::vector<CSVConfigHash> const kLightDatasets = {{
            {tests::kCIPublicHighway10k, 33398},
            {tests::kNeighbors10k, 43368},
            {tests::kWdcAstronomical, 2902},  // answer is 9 / 15
            {tests::kWdcAppearances, 64338},  // answer is 3 / 4
            {tests::kWdcAstrology, 40815},    // answer is 34 / 20
            {tests::kWdcSymbols, 28289},
            {tests::kBreastCancer, 15121},
            {tests::kWdcKepler, 17294},  // empty answer, 0 clusters after stripping
    }};

    inline static std::vector<CSVConfigHash> const kHeavyDatasets = {{
            {tests::kAdult, 23075},
            {tests::kCIPublicHighway, 13035},
            {tests::kEpicMeds, 26201},  // answer is 15 / 16
            {tests::kEpicVitals, 2083},
            {tests::kIowa1kk, 57837},  // answer is 2531 / 1584 (average 2k, it is bad seed) :(
            {tests::kLegacyPayors, 43612},
    }};
};

}  // namespace tests
