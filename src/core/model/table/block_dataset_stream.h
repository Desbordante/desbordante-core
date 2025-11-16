/** \file
 * \brief Block data stream
 *
 * implementation of the BlockDatasetStream class, which
 * represents a dataset stream with block-base access
 */
#pragma once

#include <cassert>
#include <numeric>
#include <string>

#include "block_data.h"
#include "dataset_stream_wrapper.h"
#include "util/logger.h"

namespace model {

/// block-based data stream
template <class DatasetStream>
class BlockDatasetStream {
public:
    using Row = DatasetStream::Row;

private:
    DatasetStreamWrapper<DatasetStream> stream_;
    size_t capacity_; /* block size limit */

    /* get count of chars in a row */
    static size_t GetRowSize(Row const& row) {
        return std::accumulate(
                row.begin(), row.end(), 0UL,
                [](size_t acc, std::string const& value) { return acc + value.size(); });
    }

public:
    ///
    /// \brief block-based stream constructor
    ///
    /// \param stream    shared pointer to the dataset stream.
    /// \param capacity  block size limit in bytes
    ///
    template <typename Stream>
    BlockDatasetStream(Stream&& stream, size_t capacity)
        : stream_(std::forward<Stream>(stream)), capacity_(capacity) {}

    /// check if there is a next block available in the dataset stream
    [[nodiscard]] bool HasNextBlock() const {
        return stream_.HasNextRow();
    }

    /// get the next block of data from the dataset stream
    BlockData GetNextBlock() {
        assert(HasNextBlock());
        BlockData block{static_cast<ColumnIndex>(stream_.GetNumberOfColumns())};
        size_t block_size = 0;
        do {
            Row row = stream_.GetNextRow();
            block_size += GetRowSize(row);
            block.InsertRow(std::move(row));
        } while ((block_size < capacity_) && stream_.HasNextRow());
        return block;
    }
};

}  // namespace model
