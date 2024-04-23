#pragma once

#include <memory>

#include <easylogging++.h>

#include "dataset_stream_wrapper.h"
#include "idataset_stream.h"

namespace model {

/// A dataset stream that ensures that all rows have the same fixed size (GetNumberOfColumns()).
template <typename DatasetStream = std::shared_ptr<IDatasetStream>>
class DatasetStreamFixed final : public DatasetStreamWrapper<DatasetStream> {
public:
    using Row = typename DatasetStreamWrapper<DatasetStream>::Row;

private:
    Row next_row_{};

    ///
    /// \brief try to store next row
    ///
    /// \note skip row from data stream when row
    ///       has incorrect number of values
    ///
    /// @return return false means end of the data stream
    ///
    bool TryStoreNextRow() {
        if (this->stream_->HasNextRow()) {
            next_row_ = this->stream_->GetNextRow();
            size_t const cols_num = this->stream_->GetNumberOfColumns();
            if (next_row_.size() != cols_num) {
                LOG(WARNING) << "Received row with size " << next_row_.size() << ", but expected "
                             << cols_num;
                return TryStoreNextRow();
            }
        } else {
            next_row_.clear();
        }
        return HasNextRow();
    }

public:
    template <typename Stream = DatasetStream>
    explicit DatasetStreamFixed(Stream&& stream)
        : DatasetStreamWrapper<DatasetStream>(std::forward<Stream>(stream)) {
        TryStoreNextRow();
    }

    Row GetNextRow() override {
        Row row{std::move(next_row_)};
        TryStoreNextRow();
        return row;
    }

    [[nodiscard]] bool HasNextRow() const override {
        return !next_row_.empty();
    }

    void Reset() override {
        this->stream_->Reset();
        TryStoreNextRow();
    }
};

}  // namespace model
