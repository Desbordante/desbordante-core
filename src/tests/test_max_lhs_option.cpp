#include <filesystem>

#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/fd/fastfds/fastfds.h"
#include "algorithms/fd/fd_algorithm.h"
#include "algorithms/fd/tane/tane.h"
#include "config/names.h"
#include "datasets.h"
#include "model/table/relational_schema.h"

using algos::FastFDs, algos::Tane;
using config::InputTable;

InputTable GetInputTable(const std::filesystem::path& path, char separator = ',',
                         bool has_header = true) {
    return std::make_unique<CSVParser>(path, separator, has_header);
}

class MaxLHSOptionTest : public ::testing::Test {
protected:
    template <typename Algo>
    static std::unique_ptr<algos::FDAlgorithm> CreateAlgoInstance(unsigned int max_lhs,
                                                                  bool EqNullsType,
                                                                  const std::filesystem::path& path,
                                                                  char separator = ',',
                                                                  bool hasHeader = true) {
        using namespace config::names;

        algos::StdParamsMap params{
                {kCsvPath, path},        {kSeparator, separator},
                {kHasHeader, hasHeader}, {kTable, GetInputTable(path, separator, hasHeader)},
                {kMaximumLhs, max_lhs},  {kEqualNulls, EqNullsType}};

        return algos::CreateAndLoadAlgorithm<Algo>(params);
    }
};

TEST_F(MaxLHSOptionTest, TaneMaxLHSOptionWork) {
    unsigned int max_lhs = 3;
    auto const path = test_data_dir / "TestFD.csv";
    auto algorithm = CreateAlgoInstance<Tane>(max_lhs, true, path);
    algorithm->Execute();
    std::list<FD> result_fds_list = algorithm->FdList();
    for (auto& fd : result_fds_list) {
        ASSERT_TRUE(fd.GetLhs().GetArity() <= max_lhs);
    }
}

TEST_F(MaxLHSOptionTest, FastFDMaxLHSOptionWork) {
    unsigned int max_lhs = 3;
    auto const path = test_data_dir / "TestFD.csv";
    auto algorithm = CreateAlgoInstance<FastFDs>(max_lhs, true, path);
    algorithm->Execute();
    std::list<FD> result_fds_list = algorithm->FdList();
    for (auto& fd : result_fds_list) {
        ASSERT_TRUE(fd.GetLhs().GetArity() <= max_lhs);
    }
}
