#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "algorithms/md/hymd/enums.h"
#include "algorithms/md/hymd/indexes/records_info.h"
#include "algorithms/md/hymd/lattice/md_lattice_node_info.h"
#include "algorithms/md/hymd/preprocessing/column_matches/column_match.h"
#include "algorithms/md/hymd/similarity_data.h"
#include "algorithms/md/md_algorithm.h"
#include "config/tabular_data/input_table_type.h"
#include "config/thread_number/type.h"
#include "model/table/relational_schema.h"

namespace algos::hymd {

// Implementation of the algorithm described in "Efficient Discovery of Matching Dependencies" by
// SCHIRMER, PAPENBROCK, KOUMARELAS, and NAUMANN with additional optimizations.
// The short version is:
// 1) Calculate all similarities between values in the records, as specified by the column matches.
// 2) Assume every dependency holds.
// 3) Check some pairs of records to find dependencies that are violated by them.
// 4) Traverse the search space directly and check which dependencies that are assumed to hold
// actually hold.
// 5) Alternate between 3 and 4 until 4 says all dependencies we have actually hold.

// Notable details in the implementation:
// 1) Instead of decision boundaries directly, the execution part of the algorithm uses their
// indices in a list of all decision boundaries. Throughout the code, those indices are referred to
// as column classifier value identifiers, CCV IDs for short. As stated in the article, column
// classifiers consist of a column match and a decision boundary. CCV refers to the latter.
//  In addition to that, LHS CCV IDs are a subset of RHS CCV IDs and hence are separated,
// occasionally needing (extremely cheap) conversions. This is done to implement the interestingness
// criterion named "Decision boundaries (number)".
//  In some sense, it means that LHSs and RHSs of MDs are "more separated" than in the original
// implementation.
// 2) Sampling is biased towards picking pairs that have a higher LHS CCV ID for some column match.
// This helps us pick better record pairs from the start in the sense that inference from record
// pairs will get the lattice closer to the final state with fewer record pair comparisons.
// 3) Inference from record pairs switches only after processing all pairs from a round of sampling,
// instead of checking statistics before processing each pair.
// 4) Generalization checks during specialization skip nodes that correspond to LHSs that generalize
// the LHS being specialized.
// 5) LHS is represented as an array of (offset, decision boundary) pairs instead of an array of
// decision boundaries.
// 6) Validator only returns invalidated MDs.
class HyMD final : public MdAlgorithm {
public:
    using ColumnMatches = SimilarityData::ColumnMatches;

private:
    config::InputTable left_table_;
    config::InputTable right_table_;

    std::shared_ptr<RelationalSchema> left_schema_;
    std::shared_ptr<RelationalSchema> right_schema_;

    std::unique_ptr<indexes::RecordsInfo> records_info_;

    std::size_t min_support_ = 0;
    bool prune_nondisjoint_ = true;
    std::size_t max_cardinality_ = -1;
    config::ThreadNumType threads_;
    LevelDefinition level_definition_ = LevelDefinition::kCardinality;
    // TODO: different level definitions (cardinality currently used)
    // TODO: comparing only some values during similarity calculation
    // TODO: automatically calculating minimal support
    // TODO: limit LHS bounds searched (currently only size limit is implemented)
    // TODO: memory conservation mode (load only some columns)

    ColumnMatches column_matches_option_;

    void RegisterOptions();

    void LoadDataInternal() final;

    void MakeExecuteOptsAvailable() final;
    void ResetStateMd() final;
    unsigned long long ExecuteInternal() final;

    class RegisterHelper;
    void RegisterResults(SimilarityData const& similarity_data,
                         std::vector<lattice::MdLatticeNodeInfo> lattice_mds);

public:
    HyMD();
};

}  // namespace algos::hymd
