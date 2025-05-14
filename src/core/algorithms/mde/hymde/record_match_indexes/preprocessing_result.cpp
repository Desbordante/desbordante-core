#include "algorithms/mde/hymde/record_match_indexes/preprocessing_result.h"

#include <numeric>

#include "algorithms/mde/hymde/record_match_indexes/partition_index.h"
#include "algorithms/mde/hymde/utility/index_range.h"
#include "algorithms/mde/hymde/utility/make_unique_for_overwrite.h"
#include "algorithms/mde/hymde/utility/zip.h"
#include "util/get_preallocated_vector.h"

namespace {
using namespace algos::hymde::record_match_indexes;

class ResultCreator {
    std::vector<ClassifierValues> classifier_values;
};
}  // namespace

namespace algos::hymde::record_match_indexes {
PreprocessingResult PreprocessingResult::Create(
        RelationalSchema const& left_schema, RelationalSchema const& right_schema,
        records::DictionaryCompressed const& dictionary_compressed_records,
        ComponentCalculationSpecification const& calculator_creators,
        util::WorkerThreadPool* pool_ptr) {
    using Table = records::DictionaryCompressed::Table;
    using CalculatorPtr = std::unique_ptr<calculators::Calculator>;

    std::size_t const total_record_matches = calculator_creators.size();
    std::vector<model::mde::RecordMatch> record_matches;
    record_matches.reserve(total_record_matches);

    Table const& left_table = dictionary_compressed_records.GetLeftTable();
    Table const& right_table = dictionary_compressed_records.GetRightTable();

    PartitionIndex partition_index_left{left_table.size(), total_record_matches};
    PartitionIndex partition_index_right{right_table.size(), total_record_matches};

    std::vector<ClassifierValues> classifier_values_all =
            util::GetPreallocatedVector<ClassifierValues>(total_record_matches);

    // Exclude record matches that only have the greatest RCV, order others in some beneficial way.
    std::vector<model::Index> useful_record_matches;

    std::vector<Indexes> indexes;
    std::vector<RcvIdLRMap> rcv_id_lr_maps;
    std::vector<ComponentStructureAssertions> assertions;

    std::vector<CalculatorPtr> calculators =
            util::GetPreallocatedVector<CalculatorPtr>(total_record_matches);
    for (CalculatorCreatorPtr const& creator : calculator_creators) {
        calculators.push_back(
                creator->Create(left_schema, right_schema, dictionary_compressed_records));
    }
    for (model::Index record_match_index : utility::IndexRange(total_record_matches)) {
        calculators::Calculator const& calculator = *calculators[record_match_index];
        record_matches.push_back(calculator.GetRecordMatch());
        ComponentHandlingInfo info =
                calculator.Calculate(pool_ptr, partition_index_left.NewPartitionAdder(),
                                     partition_index_right.NewPartitionAdder());
        auto& [search_space_component, rm_indexes, rm_assertions] = info;
        auto& [classifier_values, rcv_id_lr_map] = search_space_component;
        if (classifier_values_all.emplace_back(std::move(classifier_values)).MaxIsTotal()) continue;
        useful_record_matches.push_back(record_match_index);
        indexes.push_back(std::move(rm_indexes));
        rcv_id_lr_maps.push_back(std::move(rcv_id_lr_map));
        assertions.push_back(rm_assertions);
    }

    std::size_t const non_trivial_number = useful_record_matches.size();
    auto arrangement_ptr = utility::MakeUniqueForOverwrite<model::Index[]>(non_trivial_number);
    auto start = arrangement_ptr.get(), end = start + non_trivial_number;
    std::iota(start, end, 0);
    std::sort(start, end, [&](model::Index i, model::Index j) {
        std::size_t const lhs_ccv_ids1 = rcv_id_lr_maps[i].lhs_to_rhs_map.size();
        std::size_t const lhs_ccv_ids2 = rcv_id_lr_maps[j].lhs_to_rhs_map.size();
        return lhs_ccv_ids1 < lhs_ccv_ids2 || (lhs_ccv_ids1 == lhs_ccv_ids2 && i < j);
    });
    std::vector<Indexes> sorted_indexes = util::GetPreallocatedVector<Indexes>(non_trivial_number);
    std::vector<RcvIdLRMap> sorted_rcv_id_lr_maps =
            util::GetPreallocatedVector<RcvIdLRMap>(non_trivial_number);
    std::vector<model::Index> sorted_useful_record_matches =
            util::GetPreallocatedVector<model::Index>(non_trivial_number);
    std::vector<ComponentStructureAssertions> sorted_assertions =
            util::GetPreallocatedVector<ComponentStructureAssertions>(non_trivial_number);

    for (model::Index non_trivial_index : std::span{arrangement_ptr.get(), non_trivial_number}) {
        sorted_indexes.push_back(std::move(indexes[non_trivial_index]));
        sorted_rcv_id_lr_maps.push_back(std::move(rcv_id_lr_maps[non_trivial_index]));
        sorted_useful_record_matches.push_back(useful_record_matches[non_trivial_index]);
        sorted_assertions.push_back(assertions[non_trivial_index]);
    }

    indexes = std::move(sorted_indexes);
    rcv_id_lr_maps = std::move(sorted_rcv_id_lr_maps);
    useful_record_matches = std::move(sorted_useful_record_matches);
    assertions = std::move(sorted_assertions);

    DataPartitionIndex data_partition_index{
            PartitionIndex::GetPermuted(std::move(partition_index_left), useful_record_matches),
            PartitionIndex::GetPermuted(std::move(partition_index_right), useful_record_matches)};

    return {std::move(record_matches),
            std::move(classifier_values_all),
            std::move(useful_record_matches),
            std::move(data_partition_index),
            std::move(indexes),
            std::move(rcv_id_lr_maps),
            std::move(assertions)};
}
}  // namespace algos::hymde::record_match_indexes
