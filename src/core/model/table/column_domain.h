/** \file
 * \brief Column domain data
 *
 * implementation of the ColumnDomain class, which provides the column domain values
 */
#pragma once

#include <cassert>
#include <cstddef>
#include <filesystem>
#include <list>
#include <memory>
#include <numeric>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "column_combination.h"
#include "config/mem_limit/type.h"
#include "config/thread_number/type.h"
#include "idataset_stream.h"
#include "table/column_index.h"
#include "table/table_index.h"

namespace model {
class IDatasetStream;

using PartitionIndex = unsigned int;

/// column domain partition storing values in sorted order
class DomainPartition {
public:
    using Value = std::string;
    using Values = std::set<Value>;

    ///
    /// @brief abstract reader class for receiving partition values
    ///
    /// The partition can either be located entirely in memory or entirely on disk.\n
    /// This class is required to provide a single interface for reading values from
    /// the partition.
    ///
    class PartitionReader {
    public:
        using Value = DomainPartition::Value;

    public:
        PartitionReader() = default;
        virtual ~PartitionReader() = default;

        virtual Value const& GetValue() const = 0;
        virtual bool HasNext() const = 0;
        virtual void MoveToNext() = 0;

        bool TryMove() {
            return HasNext() && (MoveToNext(), true);
        }
    };

private:
    struct PartitionInfo {
        TableIndex table_id;
        ColumnIndex column_id;
        PartitionIndex partition_id;
    };

    PartitionInfo info_;
    Values values_;
    std::unique_ptr<std::filesystem::path> swap_file_;

    static constexpr std::string_view kTmpDir = "tmp";

public:
    DomainPartition(TableIndex table_id, ColumnIndex column_id, PartitionIndex partition_id = 0)
        : info_({table_id, column_id, partition_id}) {}

    ~DomainPartition();

    /// how many bytes partition takes to store one char in the partition
    /// this value was obtained experimentally, the worst case is presented
    static constexpr double kMaximumBytesPerChar = 16.0;

    /// insert new value to partition
    void Insert(Value const& value) {
        values_.insert(value);
    }

    /// get table index
    TableIndex GetTableId() const noexcept {
        return info_.table_id;
    }

    /// get column index
    ColumnIndex GetColumnId() const noexcept {
        return info_.column_id;
    }

    /// get partition index
    PartitionIndex GetPartitionId() const noexcept {
        return info_.partition_id;
    }

    /// check if the partition is null
    ///
    /// a partition is not null if and only if it contains
    /// non-null values (null value is empty string)
    bool IsNULL() const noexcept {
        return (values_.empty() || (values_.size() == 1 && values_.begin()->empty())) &&
               !IsSwapped();
    }

    /// get memory usage in bytes
    size_t GetMemoryUsage() const noexcept;
    /// returns true if partition was swapped and false otherwise
    bool TrySwap();

    /// check if partition swapped
    bool IsSwapped() const noexcept {
        return static_cast<bool>(swap_file_);
    }

    /// create partition reader
    std::unique_ptr<PartitionReader> GetReader() const;
};

/// represents a column domain
class ColumnDomain {
public:
    using RawData = std::list<DomainPartition>;

private:
    RawData raw_data_; /* raw domain data */
    size_t mem_usage_; /* memory usage in bytes */

    void RefreshMemoryUsage() {
        mem_usage_ = std::accumulate(raw_data_.begin(), raw_data_.end(), 0UL,
                                     [](size_t acc, DomainPartition const& partition) {
                                         return acc + partition.GetMemoryUsage();
                                     });
    }

public:
    explicit ColumnDomain(RawData&& raw_data) : raw_data_(std::move(raw_data)) {
        assert(!raw_data_.empty());
        RefreshMemoryUsage();
    }

    /// get raw domain data
    RawData const& GetData() const noexcept {
        return raw_data_;
    }

    /// get table index
    TableIndex GetTableId() const noexcept {
        return raw_data_.front().GetTableId();
    }

    /// get column index
    ColumnIndex GetColumnId() const noexcept {
        return raw_data_.front().GetColumnId();
    }

    /// get memory usage in bytes
    size_t GetMemoryUsage() const noexcept {
        return mem_usage_;
    }

    /// swap domain to disk and update memory usage
    void Swap() {
        for (DomainPartition& partition : raw_data_) {
            partition.TrySwap();
        }
        RefreshMemoryUsage();
    }

    /// create domains for the vector of data streams
    static std::vector<ColumnDomain> CreateFrom(
            std::vector<std::shared_ptr<model::IDatasetStream>> const& streams,
            config::MemLimitMBType mem_limit_mb, config::ThreadNumType threads_num);
};

}  // namespace model
