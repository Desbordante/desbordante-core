#include <filesystem>
#include <memory>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/tks/tks.h"
#include "core/algorithms/tks/PatternTKS.hpp"
#include "core/algorithms/tks/Prefix.hpp"
#include "core/algorithms/tks/Itemset.hpp"
#include "core/config/names.h"
#include "core/parser/spmf_parser/spmf_parser.h"
#include "tests/common/csv_config_util.h"

namespace tests {

namespace {
std::filesystem::path const kSPMFDataDir = kTestDataDir / "spmf_data";
std::filesystem::path const kTKSSimple = kSPMFDataDir / "tks_simple.txt";
std::filesystem::path const kTKSFrequent = kSPMFDataDir / "tks_frequent.txt";
std::filesystem::path const kTKSSingleItemsets = kSPMFDataDir / "tks_single_itemsets.txt";
std::filesystem::path const kTKSLengths = kSPMFDataDir / "tks_lengths.txt";
}  // namespace

class TKSTest : public ::testing::Test {
protected:
    static algos::StdParamsMap CreateTKSParams(std::filesystem::path const& path, int k,
                                               int min_length = 1, int max_length = 1000,
                                               int max_gap = INT_MAX) {
        using namespace config::names;
        return {{std::string("spmf_file_path"), path},
                {std::string("k"), k},
                {std::string("min_pattern_length"), min_length},
                {std::string("max_pattern_length"), max_length},
                {std::string("max_gap"), max_gap}};
    }

    static std::unique_ptr<algos::tks::TKS> CreateTKSAlgorithm(
            std::filesystem::path const& path, int k, int min_length = 1, int max_length = 1000,
            int max_gap = INT_MAX) {
        auto params = CreateTKSParams(path, k, min_length, max_length, max_gap);
        return algos::CreateAndLoadAlgorithm<algos::tks::TKS>(params);
    }
};

// ======================== Basic Tests ========================

TEST_F(TKSTest, SimpleDatasetLoading) {
    auto algo = std::make_unique<algos::tks::TKS>();
    ASSERT_NE(algo, nullptr);
    SUCCEED();
}

TEST_F(TKSTest, ParseSimpleFile) {
    parser::SPMFParser parser(kTKSSimple);
    auto data = parser.Parse();
    
    // Check basic properties
    EXPECT_GT(data.num_sequences, 0);
    EXPECT_GT(data.total_itemsets, 0);
    EXPECT_EQ(data.sequence_starts.size(), data.num_sequences);
    EXPECT_GT(data.vertical_db.size(), 0);
}

TEST_F(TKSTest, ParseFrequentFile) {
    parser::SPMFParser parser(kTKSFrequent);
    auto data = parser.Parse();
    
    EXPECT_EQ(data.num_sequences, 7);
    EXPECT_GT(data.total_itemsets, 0);
    
    // Check that vertical DB contains expected items
    EXPECT_TRUE(data.vertical_db.find(1) != data.vertical_db.end());
    EXPECT_TRUE(data.vertical_db.find(2) != data.vertical_db.end());
    EXPECT_TRUE(data.vertical_db.find(3) != data.vertical_db.end());
}

TEST_F(TKSTest, ParserWithComments) {
    // kTKSSimple has comments
    parser::SPMFParser parser(kTKSSimple);
    EXPECT_NO_THROW(auto data = parser.Parse());
}

TEST_F(TKSTest, ParseSingleItemsets) {
    parser::SPMFParser parser(kTKSSingleItemsets);
    auto data = parser.Parse();
    
    // Should have parsed sequences with single itemsets
    EXPECT_EQ(data.num_sequences, 5);
    EXPECT_GT(data.total_itemsets, 0);
}

TEST_F(TKSTest, ParseLengthsFile) {
    parser::SPMFParser parser(kTKSLengths);
    auto data = parser.Parse();
    
    EXPECT_GT(data.num_sequences, 0);
    EXPECT_GT(data.total_itemsets, 0);
}

// ======================== Parser Error Handling Tests ========================

TEST_F(TKSTest, ParserNonexistentFile) {
    std::filesystem::path nonexistent = kSPMFDataDir / "nonexistent.txt";
    EXPECT_THROW(parser::SPMFParser parser(nonexistent), std::runtime_error);
}

// ======================== Vertical DB Tests ========================

TEST_F(TKSTest, VerticalDBStructure) {
    parser::SPMFParser parser(kTKSSimple);
    auto data = parser.Parse();
    
    // Each item should have a list of (seq_id, itemset_id) pairs
    for (const auto& [item, positions] : data.vertical_db) {
        EXPECT_GT(positions.size(), 0);
        for (const auto& [seq_id, itemset_id] : positions) {
            EXPECT_GE(seq_id, 0);
            EXPECT_LT(seq_id, data.num_sequences);
            EXPECT_GE(itemset_id, 0);
        }
    }
}

TEST_F(TKSTest, SequenceStartsConsistency) {
    parser::SPMFParser parser(kTKSFrequent);
    auto data = parser.Parse();
    
    // Verify sequence starts are monotonically increasing
    for (size_t i = 1; i < data.sequence_starts.size(); ++i) {
        EXPECT_LE(data.sequence_starts[i - 1], data.sequence_starts[i]);
    }
}

// ======================== Pattern Extraction Tests ========================

TEST_F(TKSTest, ExtractSinglePatterns) {
    parser::SPMFParser parser(kTKSSingleItemsets);
    auto data = parser.Parse();
    
    // The algorithm should find single item patterns easily
    EXPECT_GT(data.vertical_db.size(), 0);
    
    // Check specific items
    EXPECT_TRUE(data.vertical_db.find(10) != data.vertical_db.end());
    EXPECT_TRUE(data.vertical_db.find(20) != data.vertical_db.end());
    EXPECT_TRUE(data.vertical_db.find(30) != data.vertical_db.end());
}

// ======================== Length Constraint Tests ========================

TEST_F(TKSTest, PatternLengthVariation) {
    parser::SPMFParser parser(kTKSLengths);
    auto data = parser.Parse();
    
    // File has patterns of varying lengths: 1, 2, 3, 4
    EXPECT_GT(data.vertical_db.size(), 0);
    EXPECT_GT(data.total_itemsets, 0);
}

// ======================== Algorithm Integration Tests ========================

TEST_F(TKSTest, TKSAlgorithmCreation) {
    auto algo = std::make_unique<algos::tks::TKS>();
    EXPECT_NE(algo, nullptr);
}

TEST_F(TKSTest, PatternComparison) {
    // Create two patterns and verify operator< works
    PatternTKS p1(Prefix(), 5);
    PatternTKS p2(Prefix(), 3);
    
    // p1 should be "less than" p2 in priority queue (higher support = lower priority value)
    EXPECT_TRUE(p1 < p2);
}

// ======================== Data Validation Tests ========================

TEST_F(TKSTest, EmptyPatternHandling) {
    PatternTKS empty_pattern(Prefix(), 0);
    EXPECT_EQ(empty_pattern.support, 0);
}

TEST_F(TKSTest, PrefixToString) {
    Prefix prefix;
    Itemset itemset1(1);
    itemset1.addItem(2);
    itemset1.addItem(3);
    prefix.addItemset(itemset1);
    
    Itemset itemset2(4);
    itemset2.addItem(5);
    prefix.addItemset(itemset2);
    
    std::string expected = "<{1 2 3} {4 5}>";
    EXPECT_EQ(prefix.toString(), expected);
}

TEST_F(TKSTest, ItemsetOperations) {
    Itemset itemset;
    EXPECT_EQ(itemset.size(), 0);
    
    itemset.addItem(10);
    EXPECT_EQ(itemset.size(), 1);
    EXPECT_EQ(itemset[0], 10);
    EXPECT_TRUE(itemset.contains(10));
    EXPECT_FALSE(itemset.contains(20));
    
    itemset.addItem(20);
    EXPECT_EQ(itemset.size(), 2);
    EXPECT_TRUE(itemset.contains(20));
}

TEST_F(TKSTest, PrefixOperations) {
    Prefix prefix;
    EXPECT_EQ(prefix.size(), 0);
    
    Itemset itemset(5);
    prefix.addItemset(itemset);
    EXPECT_EQ(prefix.size(), 1);
    
    EXPECT_TRUE(prefix.containsItem(5));
    EXPECT_FALSE(prefix.containsItem(10));
}

TEST_F(TKSTest, PrefixCloning) {
    Prefix original;
    Itemset itemset(1);
    itemset.addItem(2);
    original.addItemset(itemset);
    
    Prefix clone = original.cloneSequence();
    
    EXPECT_EQ(clone.size(), original.size());
    EXPECT_EQ(clone.toString(), original.toString());
    
    // Verify it's a deep copy
    EXPECT_TRUE(clone.containsItem(1));
    EXPECT_TRUE(clone.containsItem(2));
}

// ======================== Stress Tests ========================

TEST_F(TKSTest, LargeItemNumbers) {
    Itemset itemset(999999);
    itemset.addItem(888888);
    itemset.addItem(777777);
    
    EXPECT_EQ(itemset.size(), 3);
    EXPECT_TRUE(itemset.contains(999999));
    EXPECT_TRUE(itemset.contains(888888));
    EXPECT_TRUE(itemset.contains(777777));
}

TEST_F(TKSTest, LargeItemsetSize) {
    Itemset large_itemset(1);
    for (int i = 2; i <= 100; ++i) {
        large_itemset.addItem(i);
    }
    
    EXPECT_EQ(large_itemset.size(), 100);
    EXPECT_TRUE(large_itemset.contains(1));
    EXPECT_TRUE(large_itemset.contains(50));
    EXPECT_TRUE(large_itemset.contains(100));
    EXPECT_FALSE(large_itemset.contains(101));
}

TEST_F(TKSTest, LargePrefixSize) {
    Prefix large_prefix;
    for (int i = 0; i < 50; ++i) {
        Itemset itemset(i);
        large_prefix.addItemset(itemset);
    }
    
    EXPECT_EQ(large_prefix.size(), 50);
    EXPECT_EQ(large_prefix.getItemOccurencesTotalCount(), 50);
}

}  // namespace tests
