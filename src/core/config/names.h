#pragma once

namespace config::names {
// Common
constexpr auto kCsvConfig = "csv_config";
constexpr auto kEqualNulls = "is_null_equal_null";
constexpr auto kError = "error";
constexpr auto kLhsIndices = "lhs_indices";
constexpr auto kMaximumLhs = "max_lhs";
constexpr auto kRhsIndices = "rhs_indices";
constexpr auto kSeed = "seed";
constexpr auto kTable = "table";
constexpr auto kThreads = "threads";
// AC
constexpr auto kACSeed = "ac_seed";
constexpr auto kBinaryOperation = "bin_operation";
constexpr auto kBumpsLimit = "bumps_limit";
constexpr auto kFuzziness = "fuzziness";
constexpr auto kFuzzinessProbability = "p_fuzz";
constexpr auto kIterationsLimit = "iterations_limit";
// AC, ND
constexpr auto kWeight = "weight";
// AR, NAR
constexpr auto kFirstColumnTId = "has_tid";
constexpr auto kInputFormat = "input_format";
constexpr auto kItemColumnIndex = "item_column_index";
constexpr auto kMinimumConfidence = "minconf";
constexpr auto kMinimumSupport = "minsup";
constexpr auto kTIdColumnIndex = "tid_column_index";
// CFD
constexpr auto kCfdColumnsNumber = "columns_number";
constexpr auto kCfdMaximumLhs = "cfd_max_lhs";
constexpr auto kCfdMinimumConfidence = "cfd_minconf";
constexpr auto kCfdMinimumSupport = "cfd_minsup";
constexpr auto kCFDRuleLeft = "cfd_rule_left";
constexpr auto kCFDRuleRight = "cfd_rule_right";
constexpr auto kCfdSubstrategy = "cfd_substrategy";
constexpr auto kCfdTuplesNumber = "tuples_number";
constexpr auto kMaximumG1 = "max_g1";
constexpr auto kMaxLevelSupportDrop = "max_level_support_drop";
constexpr auto kMinSupportGain = "min_support_gain";
constexpr auto kCfdExpansionStrategy = "expansion_strategy";
constexpr auto kCfdPruningStrategy = "pruning_strategy";
constexpr auto kCfdResultStrategy = "result_strategy";
constexpr auto kPatternTreshold = "pattern_treshold";
constexpr auto kLimitPliCache = "limit_pli_cache";
// CORDS
constexpr auto kDelta = "delta";
constexpr auto kFixedSample = "fixed_sample";
constexpr auto kMaxAmountOfCategories = "max_amount_of_categories";
constexpr auto kMaxDiffValsProportion = "max_different_values_proportion";
constexpr auto kMaxFalsePositiveProbability = "max_false_positive_probability";
constexpr auto kMinCard = "min_cardinality";
constexpr auto kMinSFDStrengthMeasure = "min_sfd_strength";
constexpr auto kMinSkewThreshold = "min_skew_threshold";
constexpr auto kMinStructuralZeroesAmount = "min_structural_zeroes_amount";
constexpr auto kOnlySFD = "only_sfd";
// DC verifier
constexpr auto kDenialConstraint = "denial_constraint";
// DD verifier
constexpr auto kDDString = "dd";
// DES
constexpr auto kCrossoverProbability = "crossover_probability";
constexpr auto kDifferentialScale = "differential_scale";
constexpr auto kDifferentialStrategy = "differential_strategy";
constexpr auto kMaxFitnessEvaluations = "max_fitness_evaluations";
constexpr auto kPopulationSize = "population_size";
// Dynamic FD verifier
constexpr auto kDeleteStatements = "delete";
constexpr auto kInsertStatements = "insert";
constexpr auto kUpdateStatements = "update";
// FAIDA
constexpr auto kHllAccuracy = "hll_accuracy";
constexpr auto kIgnoreConstantCols = "ignore_constant_cols";
constexpr auto kIgnoreNullCols = "ignore_null_cols";
constexpr auto kSampleSize = "sample_size";
// FAIDA, MIND
constexpr auto kMaximumArity = "max_arity";
// FastADC
constexpr auto kAllowCrossColumns = "allow_cross_columns";
constexpr auto kComparableThreshold = "comparable_threshold";
constexpr auto kEvidenceThreshold = "evidence_threshold";
constexpr auto kMinimumSharedValue = "minimum_shared_value";
constexpr auto kShardLength = "shard_length";
// FastOD
constexpr auto kTimeLimitSeconds = "time_limit";
// GFD
constexpr auto kGfdData = "gfd";
constexpr auto kGraphData = "graph";
// GFD miner
constexpr auto kDoCollectViolations = "do_collect_violations";
constexpr auto kGfdK = "gfd_k";
constexpr auto kGfdSigma = "gfd_sigma";
// HyMD
constexpr auto kColumnMatches = "column_matches";
constexpr auto kLeftTable = "left_table";
constexpr auto kLevelDefinition = "level_definition";
constexpr auto kMaxCardinality = "max_cardinality";
constexpr auto kMinSupport = "min_support";
constexpr auto kPruneNonDisjoint = "prune_nondisjoint";
constexpr auto kRightTable = "right_table";
// IND
constexpr auto kCsvConfigs = "csv_configs";
constexpr auto kTables = "tables";
// Metric verifier
constexpr auto kDistFromNullIsInfinity = "dist_from_null_is_infinity";
constexpr auto kMetric = "metric";
constexpr auto kMetricAlgorithm = "metric_algorithm";
constexpr auto kParameter = "parameter";
constexpr auto kQGramLength = "q";
// Pyro
constexpr auto kCustomRandom = "custom_random_seed";
// Spider
constexpr auto kMemLimitMB = "mem_limit";
// Split
constexpr auto kDifferenceTable = "difference_table";
constexpr auto kNumColumns = "num_columns";
constexpr auto kNumRows = "num_rows";
// Tane
constexpr auto kAfdErrorMeasure = "afd_error_measure";
// Tane, Pfd verifier
constexpr auto kPfdErrorMeasure = "pfd_error_measure";
// Typo miner
constexpr auto kApproximateAlgorithm = "approximate_algorithm";
constexpr auto kPreciseAlgorithm = "precise_algorithm";
constexpr auto kRadius = "radius";
constexpr auto kRatio = "ratio";
// UCC verifier
constexpr auto kUCCIndices = "ucc_indices";
// MD verifier
constexpr auto kMDLHS = "lhs";
constexpr auto kMDRHS = "rhs";
// AOD verifier
constexpr auto kOcContext = "oc_context";
constexpr auto kOcLeftIndex = "oc_left_index";
constexpr auto kOcRightIndex = "oc_right_index";
constexpr auto kOcLeftOrdering = "left_ordering";
constexpr auto kOFDContext = "ofd_context";
constexpr auto kOFDRightIndex = "ofd_right_index";
}  // namespace config::names
