#pragma once

#include <memory>

#include "algorithms/mde/hymde/record_match_indexes/indexes.h"
#include "algorithms/mde/hymde/record_match_indexes/partition_index.h"
#include "algorithms/mde/hymde/records/dictionary_compressed_records.h"
#include "algorithms/mde/record_match.h"
#include "model/table/relational_schema.h"
#include "util/worker_thread_pool.h"

// (lists representing) Eq partitions (eq quotient maps?) and Eq is max comparison result
// partition lists are equal and comparison of values associated with each set gives max result

namespace algos::hymde::record_match_indexes::calculators {
class Calculator {
    model::mde::RecordMatch record_match_;

public:
    class Creator {
    public:
        virtual std::unique_ptr<Calculator> Create(
                RelationalSchema const& left_schema, RelationalSchema const& right_schema,
                records::DictionaryCompressed const& records) const = 0;

        // Throw ConfigurationError if there is an error.
        virtual void CheckSchemas(RelationalSchema const& left_schema,
                                  RelationalSchema const& right_schema) const = 0;

        virtual ~Creator() = default;
    };

    Calculator(model::mde::RecordMatch record_match) : record_match_(std::move(record_match)) {}

    virtual ComponentHandlingInfo Calculate(util::WorkerThreadPool* pool_ptr,
                                            PartitionIndex::Adder&& left_adder,
                                            PartitionIndex::Adder&& right_adder) const = 0;

    model::mde::RecordMatch const& GetRecordMatch() const noexcept {
        return record_match_;
    }

    virtual ~Calculator() = default;
};
}  // namespace algos::hymde::record_match_indexes::calculators
