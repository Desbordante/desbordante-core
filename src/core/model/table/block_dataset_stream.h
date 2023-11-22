/** \file
 * \brief Block data stream
 *
 * implementation of the BlockDatasetStream class, which represents
 * a dataset stream with block-base access
 */
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "block_data.h"
#include "idataset_stream.h"

namespace model {

/// block-based data stream
class BlockDatasetStream {
public:
    using Row = std::vector<std::string>;
    using RowVec = std::vector<Row>;

private:
    std::shared_ptr<IDatasetStream> stream_; /* data stream */
    size_t capacity_;                        /* block size limit */
    Row cur_row_;                            /* next row to be added to the output block */

    /* get the count of chars in a row */
    static size_t GetRowSize(Row const& row);

    ///
    /// \brief try to store next row
    ///
    /// \note skip row from data stream when row
    ///       has incorrect number of values.
    ///
    /// @return return false means end of data stream
    ///
    bool TryStoreNextRow();

public:
    ///
    /// \brief block-based stream constructor
    ///
    /// \param stream    shared pointer to the dataset stream.
    /// \param capacity  block size limit in bytes
    ///
    BlockDatasetStream(std::shared_ptr<IDatasetStream> const& stream, size_t capacity)
        : stream_(stream), capacity_(capacity) {
        TryStoreNextRow();
    }

    /// check if there is a next block available in the dataset stream
    bool HasNextBlock() const {
        return !cur_row_.empty();
    }

    /// get the next block of data from the dataset stream
    BlockData GetNextBlock();
};

}  // namespace model
