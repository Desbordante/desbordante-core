#pragma once

#include <sstream>
#include <string>

#include "algorithms/cfd/enums.h"
#include "algorithms/fd/tane/enums.h"
#include "algorithms/md/hymd/enums.h"
#include "algorithms/metric/enums.h"
#include "algorithms/nar/des/enums.h"
#include "util/enum_to_available_values.h"

namespace config::descriptions {

namespace details {
std::string const kDMetricString =
        "metric to use\n" + util::EnumToAvailableValues<algos::metric::Metric>();
std::string const kDMetricAlgorithmString =
        "MFD algorithm to use\n" + util::EnumToAvailableValues<algos::metric::MetricAlgo>();
std::string const kDCfdSubstrategyString = "CFD lattice traversal strategy to use\n" +
                                           util::EnumToAvailableValues<algos::cfd::Substrategy>();
std::string const kDPfdErrorMeasureString =
        "PFD error measure to use\n" + util::EnumToAvailableValues<algos::PfdErrorMeasure>();
std::string const kDAfdErrorMeasureString =
        "AFD error measure to use\n" + util::EnumToAvailableValues<algos::AfdErrorMeasure>();
std::string const kDLevelDefinitionString =
        "MD lattice level definition to use\n" +
        util::EnumToAvailableValues<algos::hymd::LevelDefinition>();
std::string const kDDifferentialStrategyString =
        "DES mutation strategy to use\n" +
        util::EnumToAvailableValues<algos::des::DifferentialStrategy>();
}  // namespace details

constexpr auto kDTable = "table processed by the algorithm";
constexpr auto kDTables = "table collection processed by the algorithm";
constexpr auto kDCsvPath = "path to the CSV table";
constexpr auto kDCsvPaths = "path to the collection of CSV tables";
constexpr auto kDSeparator = "CSV separator";
constexpr auto kDHasHeader = "CSV header presence flag [true|false]";
constexpr auto kDEqualNulls = "specify whether two NULLs should be considered equal";
constexpr auto kDThreads =
        "number of threads to use. If 0, then as many threads are used as the "
        "hardware can handle concurrently.";
constexpr auto kDCustomRandom =
        "seed for the custom random generator. Used for consistency of results across platforms.";
constexpr auto kDError = "error threshold value for Approximate FD algorithms";
auto const kDPfdErrorMeasure = details::kDPfdErrorMeasureString.c_str();
auto const kDAfdErrorMeasure = details::kDAfdErrorMeasureString.c_str();
constexpr auto kDMaximumLhs = "max considered LHS size";
constexpr auto kDMaximumArity = "max considered arity";
constexpr auto kDSeed = "RNG seed";
constexpr auto kDMinimumSupport = "minimum support value (between 0 and 1)";
constexpr auto kDMinimumConfidence = "minimum confidence value (between 0 and 1)";
constexpr auto kDInputFormat = "format of the input dataset for AR mining\n[singular|tabular]";
constexpr auto kDTIdColumnIndex = "index of the column where a TID is stored";
constexpr auto kDItemColumnIndex = "index of the column where an item name is stored";
constexpr auto kDFirstColumnTId = "indicates that the first column contains the transaction IDs";
constexpr auto kDPopulationSize = "the number of individuals in the population at any given time";
constexpr auto kDMaxFitnessEvaluations =
        "the algorithm will be stopped after calculating the fitness "
        "function this many times";
constexpr auto kDDifferentialScale = "the magnitude of mutations";
constexpr auto kDCrossoverProbability = "probability of a gene getting mutated in a new individual";
auto const kDDifferentialStrategy = details::kDDifferentialStrategyString.c_str();
auto const kDMetric = details::kDMetricString.c_str();
constexpr auto kDLhsIndices = "LHS column indices";
constexpr auto kDRhsIndices = "RHS column indices";
constexpr auto kDRhsIndex = "RHS column index";
constexpr auto kDUCCIndices = "column indices for UCC verification";
constexpr auto kDParameter = "metric FD parameter";
constexpr auto kDDistFromNullIsInfinity =
        "specify whether distance from NULL value is infinity "
        "(if not, it is 0)";
constexpr auto kDQGramLength = "q-gram length for cosine metric";
auto const kDMetricAlgorithm = details::kDMetricAlgorithmString.c_str();
constexpr auto kDRadius =
        "maximum difference between a value and the most common value in a "
        "cluster";
constexpr auto kDRatio =
        "ratio between the number of deviating values in a cluster and the "
        "cluster's size";
constexpr auto kDPreciseAlgorithm =
        "Algorithm that gives exact FDs for typo miner to compare "
        "against approximate FDs";
constexpr auto kDApproximateAlgorithm = "Algorithm which gets approximate FDs for typo miner";
constexpr auto kDCfdColumnsNumber =
        "Number of columns in the part of the dataset if you "
        "want to use algo not on the full dataset, but on its part";
constexpr auto kDCfdTuplesNumber =
        "Number of tuples in the part of the dataset if you "
        "want to use algo not on the full dataset, but on its part";
constexpr auto kDCfdMinimumSupport =
        "minimum support value (integer number "
        "between 1 and number of tuples in dataset)";
constexpr auto kDCfdMinimumConfidence = "cfd minimum confidence value (between 0 and 1)";
constexpr auto kDCfdMaximumLhs = "cfd max considered LHS size";
auto const kDCfdSubstrategy = details::kDCfdSubstrategyString.c_str();
constexpr auto kDBinaryOperation = "one of available operations: /, *, +, -";
constexpr auto kDFuzziness = "fraction of exceptional records, lies in (0, 1]";
constexpr auto kDFuzzinessProbability =
        "probability, the fraction of exceptional records that "
        "lie outside the bump intervals is at most Fuzziness, lies in (0, 1]";
constexpr auto kDWeight =
        "value lies in (0, 1]. Closer to 0 - many short intervals. "
        "Closer to 1 - small number of long intervals";
constexpr auto kDBumpsLimit = "max considered intervals amount. Pass 0 to remove limit";
constexpr auto kDTimeLimitSeconds = "max running time of the algorithm. Pass 0 to remove limit";
constexpr auto kDIterationsLimit = "limit for iterations of sampling";
constexpr auto kDACSeed = "seed, needed for choosing a data sample";
constexpr auto kDHllAccuracy =
        "HyperLogLog approximation accuracy. Must be positive\n"
        "Closer to 0 - higher accuracy, more memory needed and slower the algorithm.\n";
constexpr auto kDSampleSize =
        "Size of a table sample. Greater value - more correct answers, but higher memory "
        "consumption.\n Applies to all tables";
constexpr auto kDIgnoreNullCols =
        "Ignore INDs which contain columns filled only with NULLs. May increase "
        "performance but impacts the result. [true|false]";
constexpr auto kDIgnoreConstantCols =
        "Ignore INDs which contain columns filled with only one value. May "
        "increase performance but impacts the result. [true|false]";
constexpr auto kDGraphData = "Path to dot-file with graph";
constexpr auto kDGfdData = "Path to file with GFD";
constexpr auto kDMemLimitMB = "memory limit im MBs";
constexpr auto kDDifferenceTable = "CSV table containing difference limits for each column";
constexpr auto kDNumRows = "Use only first N rows of the table";
constexpr auto kDNUmColumns = "Use only first N columns of the table";
constexpr auto kDInsertStatements = "Rows to be inserted into the table using the insert operation";
constexpr auto kDDeleteStatements = "Rows to be deleted from the table using the delete operation";
constexpr auto kDUpdateStatements = "Rows to be replaced in the table using the update operation";
constexpr auto kDNDWeight = "Weight of ND to verify (positive integer)";
constexpr auto kDMinCard =
        " (1 - min_cardinality) * |R| (where |R| is amount of rows in table R) denotes minimum "
        "cardinality of a column to be considered a soft key. Value lies in (0, 1)";
constexpr auto kDOnlySFD = "Don't mine correlations";
constexpr auto kDMaxDiffValsProportion =
        "the maximum proportion that the number"
        "of different values in the concatenation of sample columns from the number of rows in the "
        "sample for the test for SFD to be meaningful. Value lies in (0, 1)";
constexpr auto kDMinSFDStrengthMeasure =
        "(1 - min_sfd_strength) denotes minimum threshold of strength measure of a SFD. Value lies "
        "in (0, 1)";
constexpr auto kDMinSkewThreshold =
        "(1 - min_skew_threshold) * |R| (where |R| is amount of rows in table R) is the minimum "
        "sum of the "
        "frequencies of occurrence of the most frequent"
        "values of the column in the table R, indicating that the distribution of values in the "
        "column is skewed. Value lies in (0, 1).";
constexpr auto kDMinStructuralZeroesAmount =
        "min_structural_zeroes_amount*d1*d2 is the minimum number of structural zeros in the "
        "contingency table indicating "
        "that the columns are correlated. d1, d2 - the number of different values in columns C1, "
        "C2, respectively. Value lies in (0, 1).";
constexpr auto kDMaxFalsePositiveProbability =
        "(1 - max_false_positive_probability) is the maximum acceptable probability of a "
        "false-positive correlation test "
        "result. Value lies in (0, 0.39).";
constexpr auto kDDelta =
        "additional constant for computation of sample size. Value lies in (0, 1) and must be "
        "greater than minimum_cardinality_.";
constexpr auto kDMaxAmountOfCategories =
        "Max amount of categories for the chi-squared test in case the data is not skewed";
constexpr auto kDFixedSample =
        "Indicates that instead of random generated sample CORDS uses sample consisting of n first "
        "rows of the given table. Intended for tests only.";
constexpr auto kDLeftTable = "first table processed by the algorithm";
constexpr auto kDRightTable = "second table processed by the algorithm";
constexpr auto kDPruneNonDisjoint =
        "don't search for dependencies where the LHS decision boundary at the same index as the "
        "RHS decision boundary limits the number of records matched";
constexpr auto kDMinSupport = "minimum support for a dependency's LHS";
constexpr auto kDColumnMatches = "column matches to examine";
constexpr auto kDMaxCardinality = "maximum number of MD matching classifiers";
auto const kDLevelDefinition = details::kDLevelDefinitionString.c_str();
constexpr auto kDDenialConstraint = "String representation of a Denial Constraint";
constexpr auto kDShardLength =
        "Number of rows each shard will cover when building PLI shards. Determines the "
        "segmentation of rows for parallel processing in the FastADC algorithm";
constexpr auto kDAllowCrossColumns =
        "Specifies whether to allow the construction of Denial Constraints between different "
        "attributes";
constexpr auto kDMinimumSharedValue =
        "Minimum threshold for the shared percentage of values between two columns";
constexpr auto kDComparableThreshold =
        "Threshold for the ratio of smaller to larger average values between two numeric columns";
constexpr auto kDEvidenceThreshold =
        "Denotes the maximum fraction of evidence violations allowed for a Denial Constraint to be "
        "considered approximate.";
constexpr auto kDDoCollectViolations = "Flag which tells whether to collect violations or not";
constexpr auto kDGfdK = "Max size of mined gfds";
constexpr auto kDGfdSigma = "Min frequency of mined gfds";
}  // namespace config::descriptions
