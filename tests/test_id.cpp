//#include <filesystem>
//
//#include <gtest/gtest.h>
//#include <vector>
//
//#include "algorithms/brute_force.h"
//#include "algorithms/spider.h"
//
//static std::unique_ptr<algos::BruteForce> CreateBruteForceInstance(
//        std::string const& filename,
//        char separator = ',',
//        bool has_header = true,
//        std::size_t threads = 16) {
//    std::filesystem::path path = std::filesystem::current_path() / "input_data" / filename;
//    algos::IDAlgorithm::Config conf{
//            .path = path,
//            .separator = separator,
//            .has_header = has_header,
//            .threads = threads
//    };
//    auto ptr = std::make_unique<algos::BruteForce>(conf);
//    ptr->Fit(ptr->getStream());
//    return ptr;
//}
//
//static std::unique_ptr<algos::Spider> CreateSpiderInstance(
//        std::string const& filename,
//        char separator = ',',
//        bool has_header = true,
//        std::size_t threads = 16) {
//    std::filesystem::path path = std::filesystem::current_path() / "input_data" / filename;
//    algos::IDAlgorithm::Config conf{
//            .path = path,
//            .separator = separator,
//            .has_header = has_header,
//            .threads = threads
//    };
//    auto ptr = std::make_unique<algos::Spider>(conf);
//    ptr->Fit(ptr->getStream());
//    return ptr;
//}
//
//class SpiderTest : public ::testing::Test {
//};
//
//TEST(SpiderTest, BruteForceTPCHAll) {
//    auto algorithm = CreateBruteForceInstance("tpch", '|', false, 16);
//    algorithm->Execute();
//    EXPECT_EQ(algorithm->getUIDs().size(), 96);
//}
////  37.703 | 18.577 | 96 (Sync)
////  23.154 | 18.243 (skip only if i = j)
////  23.334 |  2.576 (add dc.GetValue()  > max_values[ref])
////  23.755 |  2.330 (change dc.GetValue() to max_values[dep] > max_values[ref])
////  23.755 |  2.010 (refactor checkUID) (** <- **)
////  31.479 |  2.129 (Convert to set )
////  24.119 |  2.098 (refactored checkUID)
////  23.729 |  2.040 (back checkUID)
////  24.051 |  2.498 (make dc.GetValue() < rc.GetValue() first)
//TEST(SpiderTest, SpiderLineitem) {
//    auto algorithm = CreateSpiderInstance("tpch/lineitem.tbl", '|', false, 16);
//    algorithm->Execute();
//    EXPECT_EQ(algorithm->getUIDs().size(), 96);
//}
//TEST(SpiderTest, BLineitem) {
//    auto algorithm = CreateBruteForceInstance("tpch/lineitem.tbl", '|', false, 16);
//    algorithm->Execute();
//    EXPECT_EQ(algorithm->getUIDs().size(), 96);
//}
//TEST(SpiderTest, SpiderTPCHAll) {
//    auto algorithm = CreateSpiderInstance("tpch", '|', false, 16);
//    algorithm->Execute();
//    EXPECT_EQ(algorithm->getUIDs().size(), 96);
//}
//TEST(SpiderTest, SpiderGame) {
//    auto algorithm = CreateSpiderInstance("WDC_game.csv", ',', true, 16);
//    algorithm->Execute();
//    EXPECT_EQ(algorithm->getUIDs().size(), 2);
//}
////TEST(SpiderTest, SimpleTest8) {
////    auto algorithm = CreateSpiderInstance("tpch", '|', false, 8);
////    algorithm->Execute();
////}
////
////TEST(SpiderTest, SimpleTest4) {
////    auto algorithm = CreateSpiderInstance("tpch", '|', false, 4);
////    algorithm->Execute();
////}
////TEST(SpiderTest, SimpleTest1) {
////    auto algorithm = CreateSpiderInstance("tpch", '|', false, 1);
////    algorithm->Execute();
////}
//
//
////TEST(SpiderTest, TPCHTest1) {
////
////    //    auto paths = std::vector{std::filesystem::current_path() / "input_data" / "abalone.csv",
////    //                             std::filesystem::current_path() / "input_data" / "Workshop.csv"};
////    //    auto algorithm = CreateSpiderInstance({"ID2.csv", "ID2.csv"},
////    //                                          ',', true);
////    auto algorithm = CreateSpiderInstance("tpch/lineitem.tbl", '|', false);
//////    Processing: 365655 6 min
//////    Checking: 4269     4 s
//////    265
//////    18 sec
//////
////
////    algorithm->Execute();
////}
