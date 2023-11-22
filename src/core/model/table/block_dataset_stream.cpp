/** \file
 * \brief Block data stream
 *
 * BlockDatasetStream class methods definition
 */
#include "block_dataset_stream.h"

#include <numeric>

#include <easylogging++.h>

namespace model {

size_t BlockDatasetStream::GetRowSize(Row const& row) {
    return std::accumulate(row.begin(), row.end(), 0UL,
                           [](size_t acc, std::string const& value) { return acc + value.size(); });
}

bool BlockDatasetStream::TryStoreNextRow() {
    if (cur_row_.empty() && stream_->HasNextRow()) {
        cur_row_ = stream_->GetNextRow();
        size_t const cols_num = stream_->GetNumberOfColumns();
        if (cur_row_.size() != cols_num) {
            LOG(WARNING) << "Received row with size " << cur_row_.size() << ", but expected "
                         << cols_num;
            cur_row_.clear();
            return TryStoreNextRow();
        }
    }
    return HasNextBlock();
}

BlockData BlockDatasetStream::GetNextBlock() {
    BlockData block{static_cast<ColumnIndex>(stream_->GetNumberOfColumns())};
    size_t block_size = 0;
    do {
        if (!block.isEmpty() && block_size + GetRowSize(cur_row_) > capacity_) {
            break;
        }
        block.InsertRow(cur_row_);
        block_size += GetRowSize(cur_row_);
        cur_row_.clear();
    } while (TryStoreNextRow());
    return block;
}

}  // namespace model
