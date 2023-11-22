/** \file
 * \brief Column domain data
 *
 * implementation of partition reader classes and domain creation manager
 */
#include "column_domain.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <string>

#include <easylogging++.h>

#include "config/thread_number/type.h"
#include "model/table/block_dataset_stream.h"
#include "util/parallel_for.h"

namespace model {

using PartitionReader = DomainPartition::PartitionReader;

/// reader for reading data from main memory
class MemoryBackedReader final : public PartitionReader {
public:
    using Values = DomainPartition::Values;

private:
    using It = Values::iterator;

    It cur_, end_;

public:
    explicit MemoryBackedReader(Values const& values) : cur_(values.begin()), end_(values.end()) {
        assert(cur_ != end_);
    }

    Value const& GetValue() const noexcept final {
        return *cur_;
    }

    bool HasNext() const noexcept final {
        return std::next(cur_) != end_;
    }

    void MoveToNext() final {
        ++cur_;
    }
};

/// reader for reading data from swap file
class FileBackedReader final : public PartitionReader {
private:
    std::ifstream file_;
    Value cur_;

public:
    explicit FileBackedReader(std::filesystem::path const& path) : file_(path) {
        if (!file_.is_open()) {
            throw std::runtime_error("Error opening file");
        }
        MoveToNext();
    }

    Value const& GetValue() const final {
        return cur_;
    }

    bool HasNext() const final {
        return !file_.eof();
    }

    void MoveToNext() final {
        std::getline(file_, cur_);
    }
};

DomainPartition::~DomainPartition() {
    if (IsSwapped()) {
        std::filesystem::remove(*swap_file_);
    }
}

std::unique_ptr<PartitionReader> DomainPartition::GetReader() const {
    if (IsSwapped()) {
        return std::make_unique<FileBackedReader>(*swap_file_);
    } else {
        return std::make_unique<MemoryBackedReader>(values_);
    }
}

size_t DomainPartition::GetMemoryUsage() const noexcept {
    /* in reality, node in std::set uses 2.2 more memory than the presented estimate */
    static constexpr double kAverageOverhead = 2.2;

    if (IsSwapped()) return 0;

    size_t const data_total_size = std::accumulate(
            values_.begin(), values_.end(), 0UL, [](size_t acc, std::string const& str) {
                /*
                 * if the string is small enough, then the SSO works
                 * and the memory is not allocated on the heap
                 */
                return acc + (str.size() != str.capacity() ? 0 : str.capacity());
            });

    size_t const node_size =
            values_.size() * (sizeof(std::string) + sizeof(Values::node_type)) * kAverageOverhead;

    return node_size + data_total_size;
}

bool DomainPartition::TrySwap() {
    namespace fs = std::filesystem;
    if (IsNULL() || IsSwapped()) {
        return false;
    }
    fs::create_directory(kTmpDir);
    fs::path const file_path = fs::path{kTmpDir} /
                               (std::to_string(GetTableId()) + "." + std::to_string(GetColumnId()) +
                                "." + std::to_string(GetPartitionId()));
    std::ofstream file{file_path};
    if (!file.is_open()) {
        LOG(ERROR) << "unable to open file for swapping";
        throw std::runtime_error("Cannot open file for swapping");
    }

    for (auto it = values_.begin(); it != values_.end(); ++it) {
        file << *it;
        if (std::next(it) != values_.end()) {
            file << '\n';
        }
    }
    file.close();
    values_.clear();
    swap_file_ = std::make_unique<fs::path>(file_path);
    return true;
}

/// class that contains the logic for creating domains
class DomainManager {
public:
    using Domain = ColumnDomain;

private:
    using Partition = DomainPartition;
    using DomainRawData = Domain::RawData;

    size_t mem_limit_;      /* memory limit in bytes */
    size_t block_capacity_; /* optimal block capacity for block datastream */
    size_t mem_usage_;      /* current memory usage in bytes */
    config::ThreadNumType threads_num_;

    std::vector<ColumnDomain> domains_;      /* processed domains */
    std::vector<DomainRawData> raw_domains_; /* current table domains */
    size_t processed_block_count_{};         /* count of processed blocks in current table */
    size_t swap_candidate_{};                /* next domain candidate to swap  */

    /* recalculate memory usage */
    void RefreshMemUsage() {
        mem_usage_ = std::accumulate(raw_domains_.begin(), raw_domains_.end(), 0UL,
                                     [](size_t acc, DomainRawData const& raw_domain) {
                                         /* only last partition isn't swapped */
                                         return acc + raw_domain.back().GetMemoryUsage();
                                     });
        mem_usage_ += std::accumulate(
                domains_.begin(), domains_.end(), 0UL,
                [](size_t acc, Domain const& domain) { return acc + domain.GetMemoryUsage(); });
    }

    size_t GetApproximateBlockCount() {
        /* if no block has been processed yet, then we use an estimate of the number of blocks */
        if (processed_block_count_ == 0) {
            auto const approx_block_count =
                    static_cast<size_t>(Partition::kMaximumBytesPerChar * block_capacity_);
            return std::max(1UL, mem_limit_ / approx_block_count);
        }
        /* otherwise, use the average amount of memory spent per processed block */
        size_t const per_block_mem_usage = mem_usage_ / processed_block_count_;
        return (mem_limit_ - mem_usage_) / per_block_mem_usage;
    }

    /* get the number of blocks for subsequent processing
     * swap to disk if necessary
     */
    size_t GetNumberOfBlocks() {
        RefreshMemUsage();
        size_t block_count;
        while ((block_count = GetApproximateBlockCount()) == 0) {
            SwapNext();
        }
        return block_count;
    }

    /* swap next candidate and refresh memory usage */
    void SwapNext() {
        /* first, try to swap the domain, if there are any */
        if (swap_candidate_ != domains_.size()) {
            Domain& domain = domains_[swap_candidate_];
            size_t const domain_mem_usage = domain.GetMemoryUsage();
            domain.Swap();
            mem_usage_ -= domain_mem_usage;
            ++swap_candidate_;
            return;
        }
        /* swap current table domains */
        for (DomainRawData& raw_domain : raw_domains_) {
            Partition& partition = raw_domain.back();
            /* if the partition is empty, then it will not be swapped */
            if (partition.TrySwap()) {
                raw_domain.emplace_back(partition.GetTableId(), partition.GetColumnId(),
                                        partition.GetPartitionId() + 1);
            }
        }
        RefreshMemUsage();
        processed_block_count_ = 0;
    }

    /* process next `block_count` blocks */
    bool ProcessNext(BlockDatasetStream& block_stream, size_t block_count) {
        while (block_count != 0 && block_stream.HasNextBlock()) {
            BlockData const block = block_stream.GetNextBlock();
            auto store_column = [&block](DomainRawData& raw_domain) {
                Partition& partition = raw_domain.back();
                auto it = block.GetColumn(partition.GetColumnId()).GetIt();
                do {
                    partition.Insert(std::string{it.GetValue()});
                } while (it.TryMoveToNext());
            };
            util::parallel_foreach(raw_domains_.begin(), raw_domains_.end(), threads_num_,
                                   store_column);
            block_count--;
        }
        return block_stream.HasNextBlock();
    }

    void RawDomainsInit(TableIndex table_id, ColumnIndex col_count) {
        raw_domains_ = std::vector<DomainRawData>(col_count);
        for (ColumnIndex col_id = 0; col_id != col_count; ++col_id) {
            raw_domains_[col_id].emplace_back(table_id, col_id);
        }
    }

    static size_t GetOptimalBlockCapacity(size_t mem_limit_bytes) {
        static constexpr size_t kMaxCapacity = 2 << 20UL;
        /*
         * if the memory limit is 64 MB or more, select a block size of 2 MB
         * otherwise, choose to divide by 32 and find the nearest power of two
         *
         * for example, for 16MB we choose block capacity 512 KB
         */
        size_t const capacity = std::pow(2, std::log2(mem_limit_bytes / 32));
        return std::min(capacity, kMaxCapacity);
    }

public:
    DomainManager(size_t mem_limit_mb, config::ThreadNumType threads_num)
        : mem_limit_(mem_limit_mb << 20UL),
          block_capacity_(GetOptimalBlockCapacity(mem_limit_)),
          threads_num_(threads_num) {
        RefreshMemUsage();
    }

    /// create next domains from data stream
    void ProcessDatasetStream(TableIndex table_id,
                              std::shared_ptr<model::IDatasetStream> const& stream) {
        auto const col_count = static_cast<ColumnIndex>(stream->GetNumberOfColumns());
        RawDomainsInit(table_id, col_count);
        processed_block_count_ = 0;
        size_t block_count = 0;
        BlockDatasetStream block_stream{stream, block_capacity_};
        do {
            processed_block_count_ += block_count;
            block_count = GetNumberOfBlocks();
        } while (ProcessNext(block_stream, block_count));

        for (DomainRawData& raw_domain : raw_domains_) {
            /*
             * we do not work with columns that consist entirely of nulls.
             * It is also guaranteed that if the first partition in the `DomainRawData` is null,
             * then there are no more partitions in the `DomainRawData` (raw_domain.size() == 1)
             */
            if (!raw_domain.front().IsNULL()) {
                domains_.emplace_back(std::move(raw_domain));
            }
        }
        raw_domains_.clear();
    }

    /// get processed domains
    std::vector<ColumnDomain>&& GetDomains() && {
        return std::move(domains_);
    }
};

std::vector<ColumnDomain> ColumnDomain::CreateFrom(
        std::vector<std::shared_ptr<model::IDatasetStream>> const& streams,
        config::MemLimitMBType mem_limit_mb, config::ThreadNumType threads_num) {
    TableIndex const table_count = static_cast<TableIndex>(streams.size());
    DomainManager manager{mem_limit_mb, threads_num};
    for (TableIndex table_id = 0; table_id != table_count; ++table_id) {
        manager.ProcessDatasetStream(table_id, streams[table_id]);
    }
    return std::move(manager).GetDomains();
}

};  // namespace model
