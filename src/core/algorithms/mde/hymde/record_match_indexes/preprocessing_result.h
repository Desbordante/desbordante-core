#pragma once

#include <vector>

#include "algorithms/mde/hymde/record_match_indexes/calculators/calculator.h"
#include "algorithms/mde/hymde/record_match_indexes/indexes.h"
#include "algorithms/mde/hymde/record_match_indexes/partition_index.h"
#include "algorithms/mde/hymde/record_match_indexes/rcv_id_lr_map.h"
#include "algorithms/mde/hymde/records/dictionary_compressed_records.h"
#include "algorithms/mde/hymde/record_match_indexes/component_calculation_specification.h"
#include "model/index.h"
#include "model/table/relational_schema.h"
#include "util/worker_thread_pool.h"

namespace algos::hymde::record_match_indexes {
struct PreprocessingResult {
    using CalculatorCreatorPtr = std::shared_ptr<calculators::Calculator::Creator>;

    std::vector<model::mde::RecordMatch> record_matches;
    std::vector<ClassifierValues> classifier_values;

    // Exclude record matches that only have the greatest RCV, order others in some beneficial way.
    std::vector<model::Index> useful_record_matches;

    DataPartitionIndex data_partition_index;
    std::vector<Indexes> indexes;
    std::vector<RcvIdLRMap> rcv_id_lr_maps;
    std::vector<ComponentStructureAssertions> assertions;

    static PreprocessingResult Create(
            RelationalSchema const& left_schema, RelationalSchema const& right_schema,
            records::DictionaryCompressed const& dictionary_compressed_records,
            ComponentCalculationSpecification const& calculator_creators,
            util::WorkerThreadPool* pool_ptr);
};
}  // namespace algos::hymde::record_match_indexes
