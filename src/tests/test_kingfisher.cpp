#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/near/kingfisher/kingfisher.h"
#include "algorithms/near/near.h"
#include "algorithms/near/near_discovery.h"
#include "all_csv_configs.h"
#include "csv_config_util.h"

namespace tests {
template <typename T>
std::string VectorToString(std::vector<T> const& vec) {
    std::ostringstream oss;
    oss << "\n";
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << vec[i].ToString();  // Convert FeatureIndex to string
        if (i < vec.size() - 1) {
            oss << "\n";
        }
    }
    return oss.str();
}

class NeARAlgorithmTest : public ::testing::Test {
protected:
    static algos::StdParamsMap GetParamMap(CSVConfig const& csv_config, double maxP,
                                           unsigned int maxRules, unsigned int tidColumnIndex,
                                           unsigned int itemColumnIndex) {
        using namespace config::names;
        return {{kCsvConfig, csv_config},
                {kInputFormat, +algos::InputFormat::singular},
                {kMaxPValue, maxP},
                {kMaxRules, maxRules},
                {kTIdColumnIndex, tidColumnIndex},
                {kItemColumnIndex, itemColumnIndex}};
    }

    static algos::StdParamsMap GetParamMap(CSVConfig const& csv_config, double maxP,
                                           unsigned int maxRules, bool firstColumnTid) {
        using namespace config::names;
        return {{kCsvConfig, csv_config},
                {kInputFormat, +algos::InputFormat::tabular},
                {kMaxPValue, maxP},
                {kMaxRules, maxRules},
                {kFirstColumnTId, firstColumnTid}};
    }

    template <typename... Args>
    static std::unique_ptr<algos::NeARDiscovery> CreateAlgorithmInstance(Args&&... args) {
        return algos::CreateAndLoadAlgorithm<algos::Kingfisher>(
                GetParamMap(std::forward<Args>(args)...));
    }

    void TryKingfisherWithDataset(CSVConfig csv_config, double max_p, unsigned max_rules) {
        auto start = std::chrono::high_resolution_clock::now();
        
        auto algorithm = CreateAlgorithmInstance(csv_config, max_p, max_rules, false);
        algorithm->Execute();
        auto const rules = algorithm->GetNeARIDsVector();
        SUCCEED();
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        
        // Create unique filename based on test name
        const testing::TestInfo* const test_info = testing::UnitTest::GetInstance()->current_test_info();
        std::string output_filename = std::string("results/") + test_info->name() + "_output.txt";
        std::string time_filename = "results/runtimes.csv";
        
        // Save rules to file
        std::ofstream outfile(output_filename);
        outfile << "Rules:\n" << VectorToString(rules);
        outfile << "\n\nRuntime: " << elapsed.count() << " seconds";
        outfile.close();
        
        // Append runtime to CSV file
        std::ofstream timefile(time_filename, std::ios_base::app);
        timefile << test_info->name() << "," << std::setprecision(6) << elapsed.count() << "\n";
        timefile.close();
        
        std::cout << "Test " << test_info->name() << " completed in " << elapsed.count() << " seconds\n";
    }

    std::string GetTimestamp() {
        std::time_t now = std::time(nullptr);
        char buf[20];
        std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", std::localtime(&now));
        return std::string(buf);
    }
};

//TEST_F(NeARAlgorithmTest, PaperExampleDataset) {
//    TryKingfisherWithDataset(kTestNeAR1, 1.2e-8, 1000);
//}
//TEST_F(NeARAlgorithmTest, kTestNeARChess) {
//    TryKingfisherWithDataset(kTestNeARChess, 1.0, 100);
//}
//TEST_F(NeARAlgorithmTest, kTestNeARkosarak) {
//    TryKingfisherWithDataset(kTestNeARkosarak, 1.0, 100);
//}
//TEST_F(NeARAlgorithmTest, kTestNeARretail) {
//    TryKingfisherWithDataset(kTestNeARretail, 1.0, 100);
//}
//TEST_F(NeARAlgorithmTest, kTestNeARaccidents) {
//    TryKingfisherWithDataset(kTestNeARaccidents, 1.0, 100);
//}
//TEST_F(NeARAlgorithmTest, kTestNeARMushroom) {
//    TryKingfisherWithDataset(kTestNeARMushroom, 1.0, 100);
//}
//TEST_F(NeARAlgorithmTest, kTestNeART10I4D100K) {
//    TryKingfisherWithDataset(kTestNeART10I4D100K, 1.0, 100);
//}
//TEST_F(NeARAlgorithmTest, kTestNeARpumsb) {
//    TryKingfisherWithDataset(kTestNeARpumsb, 1.0, 100);
//}
//TEST_F(NeARAlgorithmTest, kTestNeART40I10D100K) {
//    TryKingfisherWithDataset(kTestNeART40I10D100K, 1.0, 100);
//}
//TEST_F(NeARAlgorithmTest, kTestNeARconnect) {
//    TryKingfisherWithDataset(kTestNeARconnect, 1.0, 100);
//}
//TEST_F(NeARAlgorithmTest, kTestNeARpumsb_star) {
//    TryKingfisherWithDataset(kTestNeARpumsb_star, 1.0, 100);
//}
//TEST_F(NeARAlgorithmTest, KTestNeARKaggleRows) {
//    TryKingfisherWithDataset(kRulesKaggleRows, 1.0, 100);
//}
//TEST_F(NeARAlgorithmTest, kTestNeARRealMarketData1) {
//    TryKingfisherWithDataset(kTestNeARMarketItems, 1.0, 100);
//}
TEST_F(NeARAlgorithmTest, kMushroom50) {
    TryKingfisherWithDataset(kMushroom50, 1.0, 100);
}
TEST_F(NeARAlgorithmTest, kMushroom40) {
    TryKingfisherWithDataset(kMushroom40, 1.0, 100);
}
TEST_F(NeARAlgorithmTest, kMushroom30) {
    TryKingfisherWithDataset(kMushroom30, 1.0, 100);
}
TEST_F(NeARAlgorithmTest, kMushroom20) {
    TryKingfisherWithDataset(kMushroom20, 1.0, 100);
}
TEST_F(NeARAlgorithmTest, kMushroom10) {
    TryKingfisherWithDataset(kMushroom10, 1.0, 100);
}
}  // namespace tests
