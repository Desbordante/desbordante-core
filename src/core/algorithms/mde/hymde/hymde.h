#pragma once

#include <memory>

#include "algorithms/algorithm.h"
#include "algorithms/mde/hymde/compact_mde_storage.h"
#include "algorithms/mde/hymde/record_match_indexes/component_calculation_specification.h"
#include "algorithms/mde/hymde/records/dictionary_compressed_records.h"
#include "config/tabular_data/input_table_type.h"
#include "config/thread_number/type.h"
#include "model/table/relational_schema.h"

namespace algos::hymde {
class HyMDE final : public Algorithm {
public:
    using ComponentCalculationSpecification =
            record_match_indexes::ComponentCalculationSpecification;

private:
    config::InputTable left_table_;
    config::InputTable right_table_;

    std::shared_ptr<RelationalSchema> left_schema_;
    std::shared_ptr<RelationalSchema> right_schema_;

    std::unique_ptr<records::DictionaryCompressed> dictionary_compressed_records_;

    // TODO: calculate minimum support as this value + known guaranteed value
    // Example: same table + equality max + same part. funcs means support is at least number of
    // records. Perhaps requiring support to be min guaranteed value of all record matches used in
    // LHS is the right move? But this way complicates the unsupported lattice.
    // I want to avoid the situation where all columns are matched to themselves in some record
    // matches but in others that is not the case. The requirement of support being at least number
    // of records is too strict in that case.
    std::size_t min_support_ = 0;
    bool prune_nondisjoint_ = true;
    std::size_t max_cardinality_ = -1;
    config::ThreadNumType threads_;

    ComponentCalculationSpecification component_calculation_specification_;

    std::vector<SearchSpaceFactorSpecification> search_space_specification_;
    std::vector<SameLhsMDEsSpecification> mde_specifications_;

    void RegisterOptions();

    void LoadDataInternal() final;

    void MakeExecuteOptsAvailable() final;
    void ResetState() final;
    unsigned long long ExecuteInternal() final;

public:
    HyMDE();

    CompactMDEStorage GetMdes() const;
};
}  // namespace algos::hymde
