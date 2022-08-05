//
// Created by Shlyonskikh Alexey on 2022-07-30.
// https://github.com/BUYT-1
//

#pragma once

namespace program_option_strings {
constexpr auto Help = "help";
constexpr auto Task = "task";
constexpr auto Algorithm = "algorithm";
constexpr auto Data = "data";
#define SEPARATOR "separator"
constexpr auto SeparatorConfig = SEPARATOR;
constexpr auto SeparatorLibArg = SEPARATOR ",s";
#undef SEPARATOR
constexpr auto HasHeader = "has_header";
constexpr auto EqualNulls = "is_null_equal_null";
constexpr auto Threads = "threads";
constexpr auto Error = "error";
constexpr auto MaximumLhs = "max_lhs";
constexpr auto Seed = "seed";
constexpr auto MinimumSupport = "minsup";
constexpr auto MinimumConfidence = "minconf";
constexpr auto InputFormat = "input_format";
constexpr auto TIdColumnIndex = "tid_column_index";
constexpr auto ItemColumnIndex = "item_column_index";
constexpr auto FirstColumnTId = "has_tid";
constexpr auto Metric = "metric";
constexpr auto LhsIndices = "lhs_indices";
constexpr auto RhsIndex = "rhs_index";
constexpr auto Parameter = "parameter";
constexpr auto DistToNullIsInfinity = "dist_to_null_infinity";
constexpr auto QGramLength = "q";
}  // namespace program_option_strings
