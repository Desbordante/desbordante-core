#include <functional>
#include <utility>

#include <boost/functional/hash.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "algo_factory.h"
#include "typo_miner.h"
#include "program_option_strings.h"

namespace tests {
namespace posr = program_option_strings;

struct TestingParam {
    algos::StdParamsMap params;

    TestingParam(std::string const& dataset,
                 char const separator, bool const has_header, bool const is_null_equal_null,
                 unsigned const max_lhs, double const error, ushort const threads)
        : params({{posr::Data, dataset},
                  {posr::SeparatorConfig, separator},
                  {posr::HasHeader, has_header},
                  {posr::EqualNulls, is_null_equal_null},
                  {posr::MaximumLhs, max_lhs},
                  {posr::Error, error},
                  {posr::Threads, threads},
                  {posr::Seed, 0}}) {}
};

/* FD represented as vector where all elements except last are lhs indices and
 * last element is rhs.
 */
using FdByIndices = std::vector<unsigned>;

struct FDsParam : TestingParam {
    std::vector<FdByIndices> expected;

    template<typename... Params>
    explicit FDsParam(std::vector<FdByIndices> expected, Params&&... params)
        : TestingParam(std::forward<Params>(params)...), expected(std::move(expected)) {}
};

struct ClustersParam : TestingParam {
    using ClustersVec = std::vector<util::PLI::Cluster>;
    using FdToClustersMap =
        std::unordered_map<FdByIndices, ClustersVec, boost::hash<FdByIndices>>;

    FdToClustersMap expected;

    template <typename... Params>
    explicit ClustersParam(FdToClustersMap expected, Params&&... params)
        : TestingParam(std::forward<Params>(params)...), expected(std::move(expected)) {}

};

struct LinesParam : TestingParam {
    using TyposVec = std::vector<util::PLI::Cluster::value_type>;
    using ClusterAndTyposPair = std::pair<util::PLI::Cluster, TyposVec>;
    using FdToTyposMap =
        std::unordered_map<FdByIndices, std::vector<ClusterAndTyposPair>, boost::hash<FdByIndices>>;

    double ratio;
    double radius;
    FdToTyposMap expected;

    template <typename... Params>
    explicit LinesParam(FdToTyposMap expected, double ratio, double radius, Params&&... params)
        : TestingParam(std::forward<Params>(params)...),
          ratio(ratio),
          radius(radius),
          expected(std::move(expected)) {}
};

static std::unique_ptr<algos::TypoMiner> CreateTypoMiner(algos::Algo const algo,
                                                         algos::StdParamsMap m) {
    auto typo_miner =
        algos::CreateAlgorithmInstance(algos::AlgoMiningType::typos, algo, std::move(m));
    auto casted = static_cast<algos::TypoMiner*>(typo_miner.release());
    return std::unique_ptr<algos::TypoMiner>(casted);
}

static std::string MakeJsonFromFds(std::vector<FdByIndices> const& fds) {
    std::string json_fds = "{\"fds\": [";

    for (FdByIndices const& fd : fds) {
        json_fds += "{lhs: [";
        auto last = std::prev(fd.end());
        for (auto it = fd.begin(); it != last; ++it) {
            json_fds += std::to_string(*it);
            if (std::next(it) != last) {
                json_fds += ", ";
            }
        }
        json_fds += "], rhs: ";
        assert(!fd.empty());
        json_fds += std::to_string(fd.back());
        json_fds += "},";
    }

    if (json_fds.back() == ',') {
        json_fds.erase(json_fds.size() - 1);
    }
    json_fds += "]}";

    return json_fds;
}

template<typename F>
static void TestForEachAlgo(F&& test) {
    for (algos::Algo const algo : algos::Algo::_values()) {
        try {
            /* Temporary fix. Currently, the Algo enum contains all the algorithms, including
             * FD mining algorithms and Apriori AR mining algorithm. But the template
             * TypoMiner class can be instantiated only with the classes that lies in the
             * AlgorithmTypesTuple tuple, which can not contain Apriori algorithm class (due to the
             * current architecture)
             * Same issue with "metric" algorithm
             * */
            if (algo != +algos::Algo::apriori && algo != +algos::Algo::metric) {
                test(algo);
            }
        } catch (std::runtime_error const& e) {
            std::cout << "Exception raised in test: " << e.what() << std::endl;
            FAIL();
        }
    }
}

TEST(SimpleTypoMinerTest, ThrowsOnEmpty) {
    auto const test = [](algos::Algo const algo) {
        TestingParam p("TestEmpty.csv", ',', true, true, -1, 0.1, 0);
        auto typo_miner = CreateTypoMiner(algo, std::move(p.params));
        ASSERT_THROW(typo_miner->Execute(), std::runtime_error);
    };
    TestForEachAlgo(test);
}

class ApproxFdsMiningTest : public ::testing::TestWithParam<FDsParam> {};

TEST_P(ApproxFdsMiningTest, SyntheticTest) {
    auto const test = [&param = GetParam()](algos::Algo const algo) {
        std::string const expected = MakeJsonFromFds(GetParam().expected);
        auto typo_miner = CreateTypoMiner(algo, GetParam().params);
        typo_miner->Execute();
        std::string actual = typo_miner->GetApproxFDsAsJson();
        EXPECT_EQ(std::hash<std::string>()(expected), std::hash<std::string>()(actual))
            << "TyposMiner with " << algo._to_string() << " as precise algorithm\n"
            << "Expected:\n\t" << expected << "\nActual:\n\t" << actual << std::endl;
    };
    TestForEachAlgo(test);
}

INSTANTIATE_TEST_SUITE_P(
    TypoMinerTestSuite, ApproxFdsMiningTest,
    ::testing::Values(
        /* Expected fds should be sorted by lhs */
        FDsParam({{1, 2}}, "SimpleTypos.csv", ',', true, true, -1, 0.05, 0),
        FDsParam({{0, 1}, {0, 2}, {1, 2}, {0}}, "SimpleTypos.csv", ',', true, true, -1, 0.81, 0),
        FDsParam({{0, 1}, {1, 2}}, "SimpleTypos.csv", ',', true, true, -1, 0.1, 0)));


class ClustersWithTyposMiningTest : public ::testing::TestWithParam<ClustersParam> {};

static FdByIndices FDtoFdByIndices(FD const& fd) {
    FdByIndices fd_by_indicies = fd.GetLhs().GetColumnIndicesAsVector();
    fd_by_indicies.push_back(fd.GetRhs().GetIndex());
    return fd_by_indicies;
}

TEST_P(ClustersWithTyposMiningTest, FindClustersWithTypos) {
    auto const test = [&param = GetParam()](algos::Algo const algo) {
        ClustersParam::FdToClustersMap const& expected = GetParam().expected;
        auto typo_miner = CreateTypoMiner(algo, GetParam().params);
        typo_miner->Execute();

        for (FD const& fd : typo_miner->GetApproxFDs()) {
            FdByIndices actual_fd = FDtoFdByIndices(fd);
            ASSERT_TRUE(expected.count(actual_fd));
            std::vector<util::PLI::Cluster> const& expected_cluster = expected.at(actual_fd);
            std::vector<util::PLI::Cluster> const actual_cluster =
                typo_miner->FindClustersWithTypos(fd);
            EXPECT_EQ(expected_cluster, actual_cluster)
                << "TyposMiner with " << algo._to_string() << " as precise algorithm\n"
                << "Clusters of FD: " << fd.ToJSONString();

        }
    };
    TestForEachAlgo(test);
}

INSTANTIATE_TEST_SUITE_P(
    TypoMinerTestSuite, ClustersWithTyposMiningTest,
    ::testing::Values(
        /* Expected clusters should be sorted with respect to TyposMiner::SortCluster sorting. */
        ClustersParam({ {FdByIndices{1, 2}, {util::PLI::Cluster{7, 9}}} },
                      "SimpleTypos.csv", ',', true, true, -1, 0.05, 0),
        ClustersParam({ {FdByIndices{0, 1}, {util::PLI::Cluster{4, 0, 1, 5, 6}}},
                        {FdByIndices{1, 2}, {util::PLI::Cluster{7, 9}}} },
                      "SimpleTypos.csv", ',', true, true, -1, 0.1, 0),
        ClustersParam({ {FdByIndices{0, 1}, {util::PLI::Cluster{4, 0, 1, 5, 6}}},
                        {FdByIndices{1, 2}, {util::PLI::Cluster{7, 9}}},
                        {FdByIndices{0, 2}, {util::PLI::Cluster{4, 0, 1, 5, 6},
                                             util::PLI::Cluster{7, 9}}},
                        {FdByIndices{0}, {util::PLI::Cluster{8, 2, 3, 7, 9, 0, 1, 4, 5, 6}}}},
                      "SimpleTypos.csv", ',', true, true, -1, 0.81, 0)));


class SquashClusterTest : public ::testing::TestWithParam<TestingParam> {};

static void VerifySquashed(ColumnLayoutRelationData const& rel, FD const& fd,
                           util::PLI::Cluster const& cluster,
                           std::vector<algos::TypoMiner::SquashedElement> const& squashed) {
    std::vector<int> const& probing_table =
        rel.GetColumnData(fd.GetRhs().GetIndex()).GetProbingTable();
    unsigned cluster_index = 0;
    for (auto const& squashed_element : squashed) {
        int const value = probing_table[squashed_element.tuple_index];
        ASSERT_FALSE(value == util::PLI::singleton_value_id_ && squashed_element.amount != 1);
        for (unsigned i = 0; i != squashed_element.amount; ++i, ++cluster_index) {
            /* Check that tuples in one squashed element have equal values in rhs */
            int const actual_value = probing_table[cluster[cluster_index]];
            ASSERT_EQ(value, actual_value)
                << "Squashed element tuple index: " << squashed_element.tuple_index
                << "\n\tamount: " << squashed_element.amount
                << "\nFD: " << fd.ToJSONString() << '\n';
        }
        /* Check that the next tuple (after the last in this squashed element) is not equal to
         * previous one and therefore is correctly not presented at this squashed element */
        ASSERT_TRUE(cluster_index == cluster.size() || value == util::PLI::singleton_value_id_ ||
                    value != probing_table[cluster[cluster_index]]);
    }
}

TEST_P(SquashClusterTest, SquashCluster) {
    auto typo_miner = CreateTypoMiner(algos::Algo::pyro, GetParam().params);
    ColumnLayoutRelationData const& rel = typo_miner->GetRelationData();
    typo_miner->Execute();

    for (FD const& fd : typo_miner->GetApproxFDs()) {
        std::vector<util::PLI::Cluster> const actual_clusters =
            typo_miner->FindClustersWithTypos(fd);
        for (auto const& cluster : actual_clusters) {
            std::vector<algos::TypoMiner::SquashedElement> squashed =
                typo_miner->SquashCluster(fd, cluster);
            VerifySquashed(rel, fd, cluster, squashed);
        }
    }
}

INSTANTIATE_TEST_SUITE_P(
    TypoMinerTestSuite, SquashClusterTest,
    ::testing::Values(TestingParam("SimpleTypos.csv", ',', true, true, -1, 0.05, 0),
                      TestingParam("SimpleTypos.csv", ',', true, true, -1, 0.1, 0),
                      TestingParam("SimpleTypos.csv", ',', true, true, -1, 0.81, 0)));


class LinesWithTyposMiningTest : public ::testing::TestWithParam<LinesParam> {};

TEST_P(LinesWithTyposMiningTest, FindLinesWithTypos) {
    auto const& p = GetParam();
    auto typo_miner = CreateTypoMiner(algos::Algo::pyro, GetParam().params);
    RelationalSchema const* schema = typo_miner->GetRelationData().GetSchema();

    for (auto const& [fd_by_indices, clusters_with_typos] : p.expected) {
        assert(fd_by_indices.size() > 1);
        auto bitset = schema->IndicesToBitset(fd_by_indices.cbegin(),
                                              std::prev(fd_by_indices.cend()));
        FD fd(schema->GetVertical(std::move(bitset)), *schema->GetColumn(fd_by_indices.back()));
        for (auto const& [cluster, typos] : clusters_with_typos) {
            std::vector<util::PLI::Cluster::value_type> const actual =
                typo_miner->FindLinesWithTypos(fd, cluster, p.radius, p.ratio);
            EXPECT_EQ(typos, actual);
        }
    }
}

INSTANTIATE_TEST_SUITE_P(
    TypoMinerTestSuite, LinesWithTyposMiningTest,
    ::testing::Values(
        LinesParam({{ FdByIndices{1, 2},
                    {std::pair(util::PLI::Cluster{7, 9}, std::vector{7, 9})} }},
                   1, -1, "SimpleTypos.csv", ',', true, true, -1, 0.05, 0),
        LinesParam({ {FdByIndices{0, 1},
                      {std::pair(util::PLI::Cluster{4, 0, 1, 5, 6}, std::vector{4})}},
                     {FdByIndices{1, 2},
                      {std::pair(util::PLI::Cluster{7, 9}, std::vector{7, 9})}} },
                   1, -1, "SimpleTypos.csv", ',', true, true, -1, 0.1, 0)));

}  // namespace tests
