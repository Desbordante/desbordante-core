#include <string>

#include "algorithms/enums.h"
#include "algorithms/metric/enums.h"
#include "util/config/enum_to_available_values.h"

namespace util::config::descriptions {
constexpr extern const char* const kDData = "data for the algorithm to process";
constexpr extern const char* const kDSeparator = "CSV separator";
constexpr extern const char* const kDHasHeader = "CSV header presence flag [true|false]";
constexpr extern const char* const kDEqualNulls =
        "specify whether two NULLs should be considered equal";
constexpr extern const char* const kDThreads =
        "number of threads to use. If 0, then as many threads are used as the hardware can handle "
        "concurrently.";
constexpr extern const char* const kDError = "error threshold value for Approximate FD algorithms";
constexpr extern const char* const kDMaximumLhs = "max considered LHS size";
constexpr extern const char* const kDSeed = "RNG seed";
constexpr extern const char* const kDMinimumSupport = "minimum support value (between 0 and 1)";
constexpr extern const char* const kDMinimumConfidence =
        "minimum confidence value (between 0 and 1)";
constexpr extern const char* const kDInputFormat =
        "format of the input dataset for AR mining\n[singular|tabular]";
constexpr extern const char* const kDTIdColumnIndex = "index of the column where a TID is stored";
constexpr extern const char* const kDItemColumnIndex =
        "index of the column where an item name is stored";
constexpr extern const char* const kDFirstColumnTId =
        "indicates that the first column contains the transaction IDs";
const std::string _kDMetric = "metric to use\n" + EnumToAvailableValues<algos::metric::Metric>();
extern const char* const kDMetric = _kDMetric.c_str();
constexpr extern const char* const kDLhsIndices = "LHS column indices";
constexpr extern const char* const kDRhsIndices = "RHS column indices";
constexpr extern const char* const kDRhsIndex = "RHS column index";
constexpr extern const char* const kDParameter = "metric FD parameter";
constexpr extern const char* const kDDistFromNullIsInfinity =
        "specify whether distance from NULL value is infinity (if not, it is 0)";
constexpr extern const char* const kDQGramLength = "q-gram length for cosine metric";
const std::string _kDMetricAlgorithm =
        "MFD algorithm to use\n" + EnumToAvailableValues<algos::metric::MetricAlgo>();
extern const char* const kDMetricAlgorithm = _kDMetricAlgorithm.c_str();
constexpr extern const char* const kDRadius =
        "maximum difference between a value and the most common value in a cluster";
constexpr extern const char* const kDRatio =
        "ratio between the number of deviating values in a cluster and the cluster's size";
constexpr extern const char* const kDPreciseAlgorithm =
        "Algorithm that gives exact FDs for typo miner to compare against approximate FDs";
constexpr extern const char* const kDApproximateAlgorithm =
        "Algorithm which gets approximate FDs for typo miner";
}  // namespace util::config::descriptions
