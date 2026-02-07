#pragma once

#include <sstream>
#include <string>

#include "core/algorithms/cfd/enums.h"
#include "core/algorithms/fd/tane/enums.h"
#include "core/algorithms/md/hymd/enums.h"
#include "core/algorithms/metric/enums.h"
#include "core/algorithms/nar/des/enums.h"
#include "core/algorithms/od/fastod/od_ordering.h"
#include "core/algorithms/pac/model/default_domains/domain_type.h"
#include "core/util/enum_to_available_values.h"

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
std::string const kDODLeftOrdering = "Ordering of the left attribute of OC or OD to use\n" +
                                     util::EnumToAvailableValues<algos::od::Ordering>();
std::string const kDDomainTypeString =
        "Domain type\n" + util::EnumToAvailableValues<pac::model::DomainType>();
}  // namespace details

// Common
constexpr auto kDEqualNulls = "specify whether two NULLs should be considered equal";
constexpr auto kDError = "error threshold value for Approximate FD algorithms";
constexpr auto kDLhsIndices = "LHS column indices";
constexpr auto kDMaximumLhs = "max considered LHS size";
constexpr auto kDRhsIndices = "RHS column indices";
constexpr auto kDSeed = "RNG seed";
constexpr auto kDTable = "table processed by the algorithm";
constexpr auto kDThreads =
        "number of threads to use. If 0, then as many threads are used as the "
        "hardware can handle concurrently.";
// AC
constexpr auto kDACSeed = "seed, needed for choosing a data sample";
constexpr auto kDBinaryOperation = "one of available operations: /, *, +, -";
constexpr auto kDBumpsLimit = "max considered intervals amount. Pass 0 to remove limit";
constexpr auto kDFuzziness = "fraction of exceptional records, lies in (0, 1]";
constexpr auto kDFuzzinessProbability =
        "probability, the fraction of exceptional records that "
        "lie outside the bump intervals is at most Fuzziness, lies in (0, 1]";
constexpr auto kDIterationsLimit = "limit for iterations of sampling";
constexpr auto kDWeight =
        "value lies in (0, 1]. Closer to 0 - many short intervals. "
        "Closer to 1 - small number of long intervals";
// AR, NAR
constexpr auto kDFirstColumnTId = "indicates that the first column contains the transaction IDs";
constexpr auto kDInputFormat = "format of the input dataset for AR mining\n[singular|tabular]";
constexpr auto kDItemColumnIndex = "index of the column where an item name is stored";
constexpr auto kDMinimumConfidence = "minimum confidence value (between 0 and 1)";
constexpr auto kDMinimumSupport = "minimum support value (between 0 and 1)";
constexpr auto kDTIdColumnIndex = "index of the column where a TID is stored";
// CFD
constexpr auto kDCfdColumnsNumber =
        "Number of columns in the part of the dataset if you "
        "want to use algo not on the full dataset, but on its part";
constexpr auto kDCfdMaximumLhs = "cfd max considered LHS size";
constexpr auto kDCfdMinimumConfidence = "cfd minimum confidence value (between 0 and 1)";
constexpr auto kDCfdMinimumSupport =
        "minimum support value (integer number "
        "between 1 and number of tuples in dataset)";
constexpr auto kDCFDRuleLeft = "CFD left rule";
constexpr auto kDCFDRuleRight = "CFD right rule";
auto const kDCfdSubstrategy = details::kDCfdSubstrategyString.c_str();
constexpr auto kDCfdTuplesNumber =
        "Number of tuples in the part of the dataset if you "
        "want to use algo not on the full dataset, but on its part";
// CORDS
constexpr auto kDDelta =
        "additional constant for computation of sample size. Value lies in (0, 1) and must be "
        "greater than minimum_cardinality_.";
constexpr auto kDFixedSample =
        "Indicates that instead of random generated sample CORDS uses sample consisting of n first "
        "rows of the given table. Intended for tests only.";
constexpr auto kDMaxAmountOfCategories =
        "Max amount of categories for the chi-squared test in case the data is not skewed";
constexpr auto kDMaxDiffValsProportion =
        "the maximum proportion that the number"
        "of different values in the concatenation of sample columns from the number of rows in the "
        "sample for the test for SFD to be meaningful. Value lies in (0, 1)";
constexpr auto kDMaxFalsePositiveProbability =
        "(1 - max_false_positive_probability) is the maximum acceptable probability of a "
        "false-positive correlation test "
        "result. Value lies in (0, 0.39).";
constexpr auto kDMinCard =
        " (1 - min_cardinality) * |R| (where |R| is amount of rows in table R) denotes minimum "
        "cardinality of a column to be considered a soft key. Value lies in (0, 1)";
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
constexpr auto kDOnlySFD = "Don't mine correlations";
// DC verifier
constexpr auto kDDenialConstraint = "String representation of a Denial Constraint";
// DD verifier
constexpr auto kDDDString = "Differential dependency that needs to be verified";
// DES
constexpr auto kDCrossoverProbability = "probability of a gene getting mutated in a new individual";
constexpr auto kDDifferentialScale = "the magnitude of mutations";
auto const kDDifferentialStrategy = details::kDDifferentialStrategyString.c_str();
constexpr auto kDMaxFitnessEvaluations =
        "the algorithm will be stopped after calculating the fitness "
        "function this many times";
constexpr auto kDPopulationSize = "the number of individuals in the population at any given time";
// Dynamic FD verifier
constexpr auto kDDeleteStatements = "Rows to be deleted from the table using the delete operation";
constexpr auto kDInsertStatements = "Rows to be inserted into the table using the insert operation";
constexpr auto kDUpdateStatements = "Rows to be replaced in the table using the update operation";
// FAIDA
constexpr auto kDHllAccuracy =
        "HyperLogLog approximation accuracy. Must be positive\n"
        "Closer to 0 - higher accuracy, more memory needed and slower the algorithm.\n";
constexpr auto kDIgnoreConstantCols =
        "Ignore INDs which contain columns filled with only one value. May "
        "increase performance but impacts the result. [true|false]";
constexpr auto kDIgnoreNullCols =
        "Ignore INDs which contain columns filled only with NULLs. May increase "
        "performance but impacts the result. [true|false]";
constexpr auto kDSampleSize =
        "Size of a table sample. Greater value - more correct answers, but higher memory "
        "consumption.\n Applies to all tables";
// FAIDA, MIND
constexpr auto kDMaximumArity = "max considered arity";
// FastADC
constexpr auto kDAllowCrossColumns =
        "Specifies whether to allow the construction of Denial Constraints between different "
        "attributes";
constexpr auto kDComparableThreshold =
        "Threshold for the ratio of smaller to larger average values between two numeric columns";
constexpr auto kDEvidenceThreshold =
        "Denotes the maximum fraction of evidence violations allowed for a Denial Constraint to be "
        "considered approximate.";
constexpr auto kDMinimumSharedValue =
        "Minimum threshold for the shared percentage of values between two columns";
constexpr auto kDShardLength =
        "Number of rows each shard will cover when building PLI shards. Determines the "
        "segmentation of rows for parallel processing in the FastADC algorithm";
// FastOD
constexpr auto kDTimeLimitSeconds = "max running time of the algorithm. Pass 0 to remove limit";
// GFD
constexpr auto kDGfdData = "Path to file with GFD";
constexpr auto kDGraphData = "Path to dot-file with graph";
// GFD miner
constexpr auto kDDoCollectViolations = "Flag which tells whether to collect violations or not";
constexpr auto kDGfdK = "Max size of mined gfds";
constexpr auto kDGfdSigma = "Min frequency of mined gfds";
// HyMD
constexpr auto kDColumnMatches = "column matches to examine";
constexpr auto kDLeftTable = "first table processed by the algorithm";
auto const kDLevelDefinition = details::kDLevelDefinitionString.c_str();
constexpr auto kDMaxCardinality = "maximum number of MD matching classifiers";
constexpr auto kDMinSupport = "minimum support for a dependency's LHS";
constexpr auto kDPruneNonDisjoint =
        "don't search for dependencies where the LHS decision boundary at the same index as the "
        "RHS decision boundary limits the number of records matched";
constexpr auto kDRightTable = "second table processed by the algorithm";
// IND
constexpr auto kDTables = "table collection processed by the algorithm";
// Metric verifier
auto const kDMetric = details::kDMetricString.c_str();
auto const kDMetricAlgorithm = details::kDMetricAlgorithmString.c_str();
constexpr auto kDParameter = "metric FD parameter";
constexpr auto kDQGramLength = "q-gram length for cosine metric";
// Metric verifier, PAC
constexpr auto kDDistFromNullIsInfinity =
        "specify whether distance from NULL value is infinity "
        "(if not, it is 0)";
// ND
constexpr auto kDNDWeight = "Weight of ND to verify (positive integer)";
// PAC
constexpr auto kDCenter = "N-ary ball center.";
constexpr auto kDColumnIndices = "Column indices.";
constexpr auto kDDeltaSteps =
        "Select how many delta values to check while verifying PAC. "
        "0 has special meaning: make (1 - min_delta) * 1000 steps, i. e. 10 steps per cent. "
        "Default is 0.";
constexpr auto kDDiagonalThreshold =
        "Maximal k such that diagonal segment of ECDF with slope coefficient of k is considered "
        "horizontal, i. e. maximal ratio m/N such that an addition of m values on a table "
        "containing N rows is not considered a change (default is 1e-5).";
constexpr auto kDDomain = "Ordered domain for Domain PAC. ";
constexpr auto kDDomainName =
        "Optional name for custom domain. This name is displayed as a part of Domain PAC's string "
        "reprsentation, so short unique names are preferrable.";
auto const kDDomainType = details::kDDomainTypeString.c_str();
constexpr auto kDFirst = "Lower bound of n-ary parallelepiped.";
constexpr auto kDLast = "Upper bound of n-ary parallelepiped.";
constexpr auto kDLevelingCoeffs =
        "Coefficients by which distances between individual coordinates are multiplied (for "
        "domains based on coordinate-wise metrics). Default is [1, 1, ..., 1].";
constexpr auto kDMaxEpsilon =
        "Maximal value of epsilon, which shows how much values deviate from exact dependency "
        "(default is +infinity).";
constexpr auto kDMinEpsilon =
        "Minimal value of epsilon, which shows how much values deviate from exact dependency "
        "(default is 0).";
constexpr auto kDMinDelta =
        "Minimal value of delta, which is a probability at which values satisfy the dependency "
        "(default is 0 if min_eps or max_eps is passed, 0.9 otherwise).";
constexpr auto kDPACRadius = "Radius of n-ary ball.";
constexpr auto kDStringDistFromDomain =
        "Custom metric for n-ary value tuples. Must return distance from domain to its argument.";
// Pyro
constexpr auto kDCustomRandom =
        "seed for the custom random generator. Used for consistency of results across "
        "platforms.";
// Spider
constexpr auto kDMemLimitMB = "memory limit im MBs";
// Split
constexpr auto kDDifferenceTable = "CSV table containing difference limits for each column";
constexpr auto kDNumColumns = "Use only first N columns of the table";
constexpr auto kDNumRows = "Use only first N rows of the table";
// Tane
auto const kDAfdErrorMeasure = details::kDAfdErrorMeasureString.c_str();
// Tane, Pfd verifier
auto const kDPfdErrorMeasure = details::kDPfdErrorMeasureString.c_str();
// Typo miner
constexpr auto kDApproximateAlgorithm = "Algorithm which gets approximate FDs for typo miner";
constexpr auto kDPreciseAlgorithm =
        "Algorithm that gives exact FDs for typo miner to compare "
        "against approximate FDs";
constexpr auto kDRadius =
        "maximum difference between a value and the most common value in a "
        "cluster";
constexpr auto kDRatio =
        "ratio between the number of deviating values in a cluster and the "
        "cluster's size";
// UCC verifier
constexpr auto kDUCCIndices = "column indices for UCC verification";
// MD verifier
constexpr auto kDMDLHS = "Left-hand side of Matching Dependancy";
constexpr auto kDMDRHS = "Right-hand side of Matching Dependancy";
// AOD verifier
constexpr auto kDOcContext = "Context of the OC to verify";
constexpr auto kDOcLeftIndex = "Index of the left attribute of the OC to verify";
constexpr auto kDOcRightIndex = "Index of the right attribute of the OC to verify";
auto const kDODLeftOrdering = details::kDODLeftOrdering.c_str();
constexpr auto kDOFDContext = "Context of the OFD to verify";
constexpr auto kDOFDRightIndex = "Right index of the OFD to verify";
}  // namespace config::descriptions
