#include <climits>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/algorithms/cmspade/cmspade.h"
#include "core/algorithms/cmspade/parser/cmspade_parser.h"
#include "core/algorithms/cmspade/model/item.h"
#include "core/algorithms/cmspade/model/itemset.h"
#include "core/algorithms/cmspade/model/sequence.h"
#include "core/algorithms/cmspade/model/pattern.h"
#include "core/algorithms/cmspade/model/id_list.h"
#include "core/algorithms/cmspade/model/equivalence_class.h"
#include "core/config/names.h"

namespace tests {
namespace {
class TempSequenceFile {
public:
    TempSequenceFile(const std::string& filename, const std::string& content) {
        path_ = std::filesystem::temp_directory_path() / filename;
        std::ofstream out(path_);
        out << content;
        out.close();
    }
    ~TempSequenceFile() { std::filesystem::remove(path_); }
    std::filesystem::path GetPath() const { return path_; }

private:
    std::filesystem::path path_;
};

class TestableCMSpade : public algos::cmspade::CMSpade {
public:
    using algos::cmspade::CMSpade::BuildCMap;
    using algos::cmspade::CMSpade::Generator;
    using algos::cmspade::CMSpade::BuildFrequentItems;
    using algos::cmspade::CMSpade::RemoveInfrequentItemsFromSequences;
    using algos::cmspade::CMSpade::RemoveEmptySequences;
    using algos::cmspade::CMSpade::LoadDataInternal;
    using algos::cmspade::CMSpade::sequences_;
    using algos::cmspade::CMSpade::cmap_equal_;
    using algos::cmspade::CMSpade::cmap_after_;
    using algos::cmspade::CMSpade::minsup_absolute_;
    using algos::cmspade::CMSpade::database_path_;
    using algos::cmspade::CMSpade::minsup_;
    using algos::cmspade::CMSpade::itemset_counts_;
    
    void SetPath(std::filesystem::path path) { database_path_ = std::move(path); }
    void SetMinSup(double minsup) { minsup_ = minsup; }
};

} // namespace

using namespace algos::cmspade;

TEST(CMSpadeModelsTest, ItemEqualityAndComparison) {
    Item a(1), b(1), c(2);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
    EXPECT_TRUE(a < c);
    EXPECT_FALSE(c < a);
}

TEST(CMSpadeModelsTest, ItemsetAddAndRemove) {
    Itemset is;
    EXPECT_TRUE(is.empty());
    is.AddItem(Item(10));
    is.AddItem(Item(20));
    EXPECT_EQ(is.size(), 2);
    
    Item removed = is.RemoveItem(0);
    EXPECT_EQ(removed.GetId(), 10);
    EXPECT_EQ(is.size(), 1);
}

TEST(CMSpadeModelsTest, SequenceAddItemsAndItemsets) {
    Sequence seq(0);
    seq.CreateNewItemset();
    seq.AddItem(Item(1));
    seq.CreateNewItemset();
    seq.AddItem(Item(2));
    
    EXPECT_EQ(seq.size(), 2);
    EXPECT_EQ(seq.length(), 2);
    
    auto cloned = seq.CloneSequence();
    EXPECT_EQ(cloned->size(), 2);
    EXPECT_EQ(cloned->length(), 2);
}

TEST(CMSpadePatternTest, ItemAbstractionPairEquals) {
    ItemAbstractionPair p1(Item(1), true);
    ItemAbstractionPair p2(Item(1), true);
    ItemAbstractionPair p3(Item(1), false);
    
    EXPECT_EQ(p1, p2);
    EXPECT_NE(p1, p3);
    EXPECT_TRUE(p3 < p1);
}

TEST(CMSpadePatternTest, PatternAddAndRetrieve) {
    Pattern pat;
    pat.Add(ItemAbstractionPair(Item(1), false));
    pat.Add(ItemAbstractionPair(Item(2), true));
    
    EXPECT_EQ(pat.Size(), 2);
    EXPECT_EQ(pat.GetLastElement().GetItem().GetId(), 2);
    EXPECT_EQ(pat.GetPenultimateElement().GetItem().GetId(), 1);
}

TEST(CMSpadePatternTest, PatternIsPrefix) {
    Pattern p_short, p_long;
    p_short.Add(ItemAbstractionPair(Item(1), false));
    p_long.Add(ItemAbstractionPair(Item(1), false));
    p_long.Add(ItemAbstractionPair(Item(2), true));
    
    EXPECT_TRUE(p_short.IsPrefix(p_long));
    EXPECT_FALSE(p_long.IsPrefix(p_short));
}

TEST(CMSpadePatternTest, PatternToStringFormat) {
    Pattern pat;
    pat.Add(ItemAbstractionPair(Item(1), false));
    pat.Add(ItemAbstractionPair(Item(2), true));
    pat.Add(ItemAbstractionPair(Item(3), false));
    
    std::string str = pat.ToString();
    EXPECT_NE(str.find("[1 2]"), std::string::npos);
    EXPECT_NE(str.find("[3]"), std::string::npos);
}

TEST(CMSpadeIdListTest, RegisterBitAndSupport) {
    auto counts = std::make_shared<std::vector<ItemsetId>>(std::vector<ItemsetId>{5, 5});
    IdList id_list(counts);
    
    id_list.RegisterBit(0, 1);
    id_list.RegisterBit(0, 2);
    id_list.RegisterBit(1, 0);
    
    EXPECT_EQ(id_list.GetSupport(), 2);
}

TEST(CMSpadeIdListTest, JoinEqualOperation) {
    boost::dynamic_bitset<> b1(4, 0b0101);
    boost::dynamic_bitset<> b2(4, 0b0110);
    
    auto res = IdList::EqualOperation(b1, b2);
    EXPECT_TRUE(res.test(2));
    EXPECT_FALSE(res.test(0));
}

TEST(CMSpadeIdListTest, JoinAfterOperation) {
    boost::dynamic_bitset<> b1(4, 0b0001);
    boost::dynamic_bitset<> b2(4, 0b0110);
    
    auto res = IdList::AfterOperation(b1, b2);
    EXPECT_TRUE(res.test(1));
    EXPECT_TRUE(res.test(2));
}

TEST(CMSpadeEquivalenceClassTest, AddAndGetMembers) {
    Pattern p1(ItemAbstractionPair(Item(1), false));
    IdList empty_list(std::make_shared<std::vector<ItemsetId>>());
    EquivalenceClass root(&p1, std::move(empty_list));
    
    Pattern p2(ItemAbstractionPair(Item(2), false));
    EquivalenceClass child(&p2, IdList(std::make_shared<std::vector<ItemsetId>>()));
    
    root.AddClassMember(std::move(child));
    EXPECT_EQ(root.GetClassMembers().size(), 1);
    EXPECT_EQ(root.GetMember(0).GetClassIdentifier()->GetLastElement().GetItem().GetId(), 2);
}

TEST(CMSpadeParserTest, ParseSingleSequence) {
    TempSequenceFile file("seq1.txt", "1 2 -1 3 -1 -2\n");
    parser::CMSpadeParser parser(file.GetPath());
    
    auto seqs = parser.ParseAll();
    ASSERT_EQ(seqs.size(), 1);
    EXPECT_EQ(seqs[0]->size(), 2);
    EXPECT_EQ(seqs[0]->GetItemsets()[0]->GetItems()[0].GetId(), 1);
}

TEST(CMSpadeParserTest, ParseMultipleSequences) {
    TempSequenceFile file("seq2.txt", "1 -1 -2\n1 2 -1 -2\n");
    parser::CMSpadeParser parser(file.GetPath());
    
    auto seqs = parser.ParseAll();
    ASSERT_EQ(seqs.size(), 2);
    EXPECT_EQ(seqs[0]->length(), 1);
    EXPECT_EQ(seqs[1]->length(), 2);
}

TEST(CMSpadeParserTest, IgnoreCommentsAndEmptyLines) {
    TempSequenceFile file("seq3.txt", "# \n\n1 -1 -2\n");
    parser::CMSpadeParser parser(file.GetPath());
    
    auto seqs = parser.ParseAll();
    ASSERT_EQ(seqs.size(), 1);
    EXPECT_EQ(seqs[0]->GetId(), 0);
}

class CMSpadeInternalTest : public ::testing::Test {
protected:
    TestableCMSpade algo;
};

TEST_F(CMSpadeInternalTest, BuildCMapEqual) {
    TempSequenceFile file("cmap_eq.txt", "1 2 -1 3 -1 -2\n");
    algo.SetPath(file.GetPath());
    algo.LoadDataInternal();
    algo.BuildCMap();
    
    auto cmap_eq = algo.cmap_equal_;
    ASSERT_TRUE(cmap_eq != nullptr);
    EXPECT_EQ((*cmap_eq)[1][2], 1); 
    EXPECT_EQ(cmap_eq->find(1)->second.find(3), cmap_eq->find(1)->second.end());
}

TEST_F(CMSpadeInternalTest, BuildCMapAfter) {
    TempSequenceFile file("cmap_after.txt", "1 2 -1 3 -1 -2\n");
    algo.SetPath(file.GetPath());
    algo.LoadDataInternal();
    algo.BuildCMap();
    
    auto cmap_after = algo.cmap_after_;
    ASSERT_TRUE(cmap_after != nullptr);
    EXPECT_EQ((*cmap_after)[1][3], 1);
    EXPECT_EQ((*cmap_after)[2][3], 1);
}

TEST_F(CMSpadeInternalTest, GeneratorCreatesValidExtensions) {
    Pattern p1(ItemAbstractionPair(Item(1), false));
    Pattern p2(ItemAbstractionPair(Item(2), false));

    p1.SetAppearing(boost::dynamic_bitset<>(4, 0b1111));
    p2.SetAppearing(boost::dynamic_bitset<>(4, 0b1111));
    
    auto candidates = algo.Generator(&p1, &p2, 1, false, false, false, false);
    EXPECT_EQ(candidates.size(), 3);
}

TEST_F(CMSpadeInternalTest, GeneratorPrunesWithCmapFlags) {
    Pattern p1(ItemAbstractionPair(Item(1), false));
    Pattern p2(ItemAbstractionPair(Item(2), false));
    p1.SetAppearing(boost::dynamic_bitset<>(4, 0b1111));
    p2.SetAppearing(boost::dynamic_bitset<>(4, 0b1111));
    
    auto candidates = algo.Generator(&p1, &p2, 1, true, true, true, true);
    EXPECT_TRUE(candidates.empty());
}

TEST_F(CMSpadeInternalTest, FilterInfrequentItems) {
    TempSequenceFile file("filter.txt", "1 2 -1 -2\n1 3 -1 -2\n");
    algo.SetPath(file.GetPath());
    algo.LoadDataInternal();
    
    algo.itemset_counts_ = std::make_shared<std::vector<ItemsetId>>(std::vector<ItemsetId>{1, 1});
    algo.BuildFrequentItems(2);
    algo.RemoveInfrequentItemsFromSequences();
    
    auto& seqs = algo.sequences_;
    EXPECT_EQ(seqs[0]->length(), 1);
    EXPECT_EQ(seqs[0]->GetItemsets()[0]->GetItems()[0].GetId(), 1);
}

TEST(CMSpadeExecuteTest, RemoveEmptySequences) {
    TestableCMSpade algo;
    TempSequenceFile file("empty_seq.txt", "1 -1 -2\n99 -1 -2\n1 -1 -2\n");
    algo.SetPath(file.GetPath());
    algo.LoadDataInternal();
    algo.itemset_counts_ = std::make_shared<std::vector<ItemsetId>>(std::vector<ItemsetId>{1, 1, 1});
    
    algo.BuildFrequentItems(2);
    algo.RemoveInfrequentItemsFromSequences();
    algo.RemoveEmptySequences();
    
    EXPECT_EQ(algo.sequences_.size(), 2);
}

} // namespace tests