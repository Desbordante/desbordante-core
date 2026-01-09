#include <memory>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/config/names.h"
#include "core/parser/sequence_parser/file_sequence_parser.h"
#include "tests/common/csv_config_util.h"

namespace tests {

class MaxFEMTest : public ::testing::Test {
protected:
    static algos::StdParamsMap GetParamMap(std::string const& filename, size_t minsup,
                                           size_t window_size) {
        using namespace config::names;

        auto path = kTestDataDir / filename;
        std::shared_ptr<model::ISequenceStream> sequence =
                std::make_shared<parser::FileSequenceParser>(path);

        algos::StdParamsMap param_map = {
                {kSequence, sequence},
                {kMinimumSupport, minsup},
                {kWindowSize, window_size},
        };
        return param_map;
    }
};

TEST_F(MaxFEMTest, MaxFEMSimpleTest) {
    auto param_map = GetParamMap("sequence_data/timed_event_sequence.txt", 2, 3);
    auto algo = algos::CreateAndLoadAlgorithm<algos::maxfem::MaxFEM>(param_map);
    algo->Execute();
}

}  // namespace tests
