#pragma once

#include <memory>
#include <string>

#include "algorithms/mde/hymde/records/dictionary_compressed_records.h"
#include "model/table/relational_schema.h"

namespace algos::hymde::record_match_indexes::partitioning_functions {
template <typename R>
class PartitioningFunction {
    std::string name_;

public:
    class Creator {
        // A little ugly, but better than setting the schema with a method.
    public:
        virtual std::unique_ptr<PartitioningFunction> Create(
                RelationalSchema const& schema,
                // TODO: do not let PartitioningFunction access all values, pass an object that
                // stores a pointer to values to GetValue instead.
                records::DictionaryCompressed::Values const& values) const = 0;
        virtual void CheckSchema(RelationalSchema const& schema) const = 0;

        virtual ~Creator() = default;
    };

    PartitioningFunction(std::string name) : name_(std::move(name)) {}

    std::string const& ToString() const noexcept {
        return name_;
    }

    virtual R GetValue(records::DictionaryCompressed::CompressedRecord const& record) const = 0;

    virtual ~PartitioningFunction() = default;
};
}  // namespace algos::hymde::record_match_indexes::partitioning_functions
