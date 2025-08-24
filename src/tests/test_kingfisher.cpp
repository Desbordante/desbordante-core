#include <regex>
#include <sstream>
#include <string>
#include <vector>

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
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << vec[i].ToString();  // Convert FeatureIndex to string
        if (i < vec.size() - 1) {
            oss << "\n";
        }
    }
    return oss.str();
}

std::vector<std::string> ReorderTreeHistory(std::vector<FeatureIndex> const& feature_order,
                                            std::vector<std::string> const& tree_history) {
    std::vector<std::string> result;

    // Regular expression to match [p:xxxx n:xxxx]
    std::regex node_pattern(R"(\[p:([01]{4}) n:([01]{4})\])");

    for (std::string const& line : tree_history) {
        std::string updated_line = line;
        std::sregex_iterator begin(updated_line.begin(), updated_line.end(), node_pattern);
        std::sregex_iterator end;

        // Offset to keep track of insertion point as string length may change
        size_t offset = 0;

        for (std::sregex_iterator i = begin; i != end; ++i) {
            std::smatch match = *i;
            std::string p = match[1];
            std::string n = match[2];

            std::string new_p, new_n;
            for (int idx : feature_order) {
                new_p += p[idx];
                new_n += n[idx];
            }

            std::string new_node = "[p:" + new_p + " n:" + new_n + "]";
            updated_line.replace(match.position(0) + offset, match.length(0), new_node);
            offset += new_node.size() - match.length(0);
        }

        result.push_back(updated_line);
    }

    return result;
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
    static std::unique_ptr<algos::Kingfisher> CreateAlgorithmInstance(Args&&... args) {
        return algos::CreateAndLoadAlgorithm<algos::Kingfisher>(
                GetParamMap(std::forward<Args>(args)...));
    }

    void CompareResults(CSVConfig csv_config, double max_p, unsigned max_rules) {
        auto algorithm = CreateAlgorithmInstance(csv_config, max_p, max_rules, false);
        algorithm->Execute();
        auto rules = algorithm->GetNeARIDsVector();
        std::string rules_string = VectorToString(rules);
        // These are the results from the paper the algorithm is based on
        std::string expected_rules_string =
                "3.947e-18  {3, 0} -> not 1\n"
                "5.777e-14  {3} -> not 2\n"
                "5.777e-14  {1, 0} -> 2\n"
                "5.777e-14  {1, 0} -> not 3\n"
                "1.735e-10  {1} -> 2\n"
                "1.735e-10  {3} -> not 1\n"
                "1.735e-10  {1} -> not 3\n"
                "1.735e-10  {2} -> 1";
        ASSERT_EQ(rules_string, expected_rules_string);
    }

    void CompareTreeHistory(CSVConfig csv_config, double max_p, unsigned max_rules) {
        auto algorithm = CreateAlgorithmInstance(csv_config, max_p, max_rules, false);
        std::vector<std::string> tree_history = algorithm->GetTreeHistory();
        // Expected tree history from the paper the algorithm is based on
        auto expected_tree_history = std::vector<std::string>();
        // step 1
        expected_tree_history.push_back(
                "Node [p: n:]\n"
                "├── [0] Node [p:0110 n:0011]\n"
                "├── [1] Node [p:0110 n:0101]\n"
                "├── [2] Node [p:0111 n:0111]\n"
                "└── [3] Node [p:0111 n:0111]\n");
        // step 3
        expected_tree_history.push_back(
                "Node [p: n:]\n"
                "├── [0] Node [p:0010 n:0011]\n"
                "│   └── [1] Node [p:0010 n:0000]\n"
                "├── [1] Node [p:0110 n:0101]\n"
                "├── [2] Node [p:0111 n:0111]\n"
                "└── [3] Node [p:0111 n:0111]\n");
        // step 4
        expected_tree_history.push_back(
                "Node [p: n:]\n"
                "├── [0] Node [p:0010 n:0010]\n"
                "│   └── [1] Node [p:0010 n:0000]\n"
                "├── [1] Node [p:0110 n:0101]\n"
                "├── [2] Node [p:0101 n:0101]\n"
                "└── [3] Node [p:0111 n:0111]\n");
        // step 5
        expected_tree_history.push_back(
                "Node [p: n:]\n"
                "├── [0] Node [p:0010 n:0010]\n"
                "│   ├── [1] Node [p:0010 n:0000]\n"
                "│   └── [3] Node [p:0010 n:0010]\n"
                "├── [1] Node [p:0110 n:0101]\n"
                "├── [2] Node [p:0101 n:0101]\n"
                "└── [3] Node [p:0111 n:0111]\n");
        // step 6
        expected_tree_history.push_back(
                "Node [p: n:]\n"
                "├── [0] Node [p:0010 n:0010]\n"
                "│   ├── [1] Node [p:0010 n:0000]\n"
                "│   ├── [2] Node [p:0000 n:0000]\n"
                "│   └── [3] Node [p:0010 n:0010]\n"
                "├── [1] Node [p:0110 n:0101]\n"
                "|   └── [2] Node [p:0000 n:0101]\n"
                "├── [2] Node [p:0001 n:0101]\n"
                "└── [3] Node [p:0111 n:0111]\n");

        // step between 6 and 7
        expected_tree_history.push_back(
                "Node [p: n:]\n"
                "├── [0] Node [p:0010 n:0010]\n"
                "│   ├── [1] Node [p:0010 n:0000]\n"
                "│   ├── [2] Node [p:0000 n:0000]\n"
                "│   └── [3] Node [p:0010 n:0010]\n"
                "├── [1] Node [p:0110 n:0101]\n"
                "|   ├── [2] Node [p:0000 n:0101]\n"
                "|   └── [3] Node [p:0110 n:0101]\n"
                "├── [2] Node [p:0001 n:0101]\n"
                "└── [3] Node [p:0111 n:0111]\n");
        // step 7
        expected_tree_history.push_back(
                "Node [p: n:]\n"
                "├── [0] Node [p:0010 n:0010]\n"
                "│   ├── [1] Node [p:0010 n:0000]\n"
                "│   ├── [2] Node [p:0000 n:0000]\n"
                "│   └── [3] Node [p:0010 n:0010]\n"
                "├── [1] Node [p:0110 n:0101]\n"
                "|   ├── [2] Node [p:0000 n:0101]\n"
                "|   └── [3] Node [p:0110 n:0101]\n"
                "├── [2] Node [p:0001 n:0101]\n"
                "|   └── [3] Node [p:0001 n:0101]\n"
                "└── [3] Node [p:0111 n:0111]\n");
        // step 8
        expected_tree_history.push_back(
                "Node [p: n:]"
                "├── [0] Node [p: n:]\n"
                "│   ├── [1] Node [p:0010 n:0000]\n"
                "│   ├── [2] Node [p:0000 n:0000]\n"
                "│   └── [3] Node [p:0010 n:0010]\n"
                "├── [1] Node [p: n:]\n"
                "|   ├── [2] Node [p:0000 n:0101]\n"
                "|   └── [3] Node [p:0100 n:0101]\n"
                "├── [2] Node [p: n:]\n"
                "|   └── [3] Node [p:0001 n:0101]\n"
                "└── [3] Node [p: n:]\n");
        // step 9
        expected_tree_history.push_back(
                "Node [p: n:]\n"
                "├── [0] Node [p: n:]\n"
                "│   ├── [1] Node [p:0010 n:0000]\n"
                "│   ├── [2] Node [p:0000 n:0000]\n"
                "│   └── [3] Node [p:0010 n:0010]\n"
                "├── [1] Node [p: n:]\n"
                "|   ├── [2] Node [p:0000 n:0101]\n"
                "|   └── [3] Node [p:0100 n:0100]\n"
                "├── [2] Node [p: n:]\n"
                "|   └── [3] Node [p:0001 n:0001]\n"
                "└── [3] Node [p: n:]\n");
        std::vector<FeatureIndex> feature_frequency_order_{2, 1, 3, 0};
        std::vector<std::string> expected_ordered_tree_history =
                ReorderTreeHistory(feature_frequency_order_, expected_tree_history);
        ASSERT_EQ(tree_history, expected_ordered_tree_history);
    }
};

TEST_F(NeARAlgorithmTest, PaperExampleResultComparison) {
    CompareResults(kTestNeAR1, 1.2e-8, 1000);
}

TEST_F(NeARAlgorithmTest, PaperExampleTreeComparison) {
    CompareTreeHistory(kTestNeAR1, 1.2e-8, 1000);
}

}  // namespace tests
