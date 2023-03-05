#pragma once

#include <sstream>
#include <string>

#include "algorithms/enums.h"
#include "algorithms/metric/enums.h"
#include "algorithms/spider/enums.h"

namespace algos {

template<typename BetterEnumType>
static std::string EnumToAvailableValues() {
    std::stringstream avail_values;

    avail_values << '[';

    for (auto const& name : BetterEnumType::_names()) {
        avail_values << name << '|';
    }

    avail_values.seekp(-1, std::stringstream::cur);
    avail_values << ']';

    return avail_values.str();
}

}  // namespace algos

namespace algos::config::descriptions {
constexpr auto kDData =
        "path to CSV file (or to directory with CSV files), relative to ./input_data";
constexpr auto kDSeparator = "CSV separator";
constexpr auto kDHasHeader = "CSV header presence flag [true|false]";
constexpr auto kDEqualNulls = "specify whether two NULLs should be considered equal";
constexpr auto kDThreads = "number of threads to use. If 0, then as many threads are used as the "
                           "hardware can handle concurrently.";
constexpr auto kDError = "error threshold value for Approximate FD algorithms";
constexpr auto kDMaximumLhs = "max considered LHS size";
constexpr auto kDSeed = "RNG seed";
constexpr auto kDMinimumSupport = "minimum support value (between 0 and 1)";
constexpr auto kDMinimumConfidence = "minimum confidence value (between 0 and 1)";
constexpr auto kDInputFormat = "format of the input dataset for AR mining\n[singular|tabular]";
constexpr auto kDTIdColumnIndex = "index of the column where a TID is stored";
constexpr auto kDItemColumnIndex = "index of the column where an item name is stored";
constexpr auto kDFirstColumnTId = "indicates that the first column contains the transaction IDs";
const std::string _kDMetric = "metric to use\n" +
                              EnumToAvailableValues<algos::metric::Metric>();
const auto kDMetric = _kDMetric.c_str();
constexpr auto kDLhsIndices = "LHS column indices for metric FD verification";
constexpr auto kDRhsIndices = "RHS column indices for metric FD verification";
constexpr auto kDParameter = "metric FD parameter";
constexpr auto kDDistFromNullIsInfinity = "specify whether distance from NULL value is infinity "
                                          "(if not, it is 0)";
constexpr auto kDQGramLength = "q-gram length for cosine metric";
const std::string _kDMetricAlgorithm = "MFD algorithm to use\n" +
                                       EnumToAvailableValues<algos::metric::MetricAlgo>();
const auto kDMetricAlgorithm = _kDMetricAlgorithm.c_str();
constexpr auto kDRadius = "maximum difference between a value and the most common value in a "
                          "cluster";
constexpr auto kDRatio = "ratio between the number of deviating values in a cluster and the "
                         "cluster's size";
constexpr auto kDPreciseAlgorithm = "Algorithm that gives exact FDs for typo miner to compare "
                                    "against approximate FDs";
constexpr auto kDApproximateAlgorithm = "Algorithm which gets approximate FDs for typo miner";
constexpr auto kDTemp =
        "specify the path to a temporary directory to store intermediate results during the "
        "execution of the program";
constexpr auto kDMemoryLimit =
        "set the maximum amount of memory (in MB) that can be used by the program";
constexpr auto kDMemoryCheckFrequency = "specify the number of rows between memory usage checks";
const std::string _kDColType = "specify the type of container to use for storing column values " +
                               EnumToAvailableValues<algos::ind::ColType>();
const auto kDColType = _kDColType.c_str();
const std::string _kDKeyType = "specify the type of the key used in the column container " +
                               EnumToAvailableValues<algos::ind::KeyType>();
const auto kDKeyType = _kDKeyType.c_str();
}  // namespace algos::config::descriptions
