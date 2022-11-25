//
// Created by Shlyonskikh Alexey on 2022-07-30.
// https://github.com/BUYT-1
//

#pragma once

namespace program_option_strings {
constexpr auto kHelp = "help";
constexpr auto kTask = "task";
constexpr auto kAlgorithm = "algorithm";
constexpr auto kData = "data";
#define SEPARATOR "separator"
constexpr auto kSeparatorConfig = SEPARATOR;
constexpr auto kSeparatorLibArg = SEPARATOR ",s";
#undef SEPARATOR
constexpr auto kHasHeader = "has_header";
constexpr auto kEqualNulls = "is_null_equal_null";
constexpr auto kThreads = "threads";
constexpr auto kError = "error";
constexpr auto kMaximumLhs = "max_lhs";
constexpr auto kSeed = "seed";
constexpr auto kMinimumSupport = "minsup";
constexpr auto kMinimumConfidence = "minconf";
constexpr auto kInputFormat = "input_format";
constexpr auto kTIdColumnIndex = "tid_column_index";
constexpr auto kItemColumnIndex = "item_column_index";
constexpr auto kFirstColumnTId = "has_tid";
constexpr auto kMetric = "metric";
constexpr auto kLhsIndices = "lhs_indices";
constexpr auto kRhsIndices = "rhs_indices";
constexpr auto kParameter = "parameter";
constexpr auto kDistToNullIsInfinity = "dist_to_null_infinity";
constexpr auto kQGramLength = "q";
constexpr auto kMetricAlgorithm = "metric_algorithm";
constexpr auto kRadius = "radius";
constexpr auto kRatio = "ratio";
constexpr auto kBinaryOperation = "bin_operation";
constexpr auto kFuzziness = "fuzziness";
constexpr auto kFuzzinessProbability = "p_fuzz";
constexpr auto kWeight = "weight";
constexpr auto kBumpsLimit = "bumps_limit";
constexpr auto kIterationsLimit = "iterations_limit";
constexpr auto kPairingRule = "pairing_rule";
}  // namespace program_option_strings
