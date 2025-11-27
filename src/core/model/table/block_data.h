/** \file
 * \brief Block data
 *
 * Definition of the block data type for block-based datastream.
 */
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "core/model/table/column_index.h"
#include "core/model/table/idataset_stream.h"

namespace model {

namespace details {

/// column data class for block data
class ColumnData {
public:
    using Value = std::string_view;

    /// column data iterator
    class ColumnIterator {
    public:
        using Value = ColumnData::Value;
        using Offset = unsigned int;

    private:
        using OffsetIt = std::vector<Offset>::const_iterator;
        std::vector<char> const& buffer_;
        OffsetIt cur_;
        OffsetIt const end_;
        Value view_;

        void ConstructView();

    public:
        ColumnIterator(std::vector<char> const& data, std::vector<Offset> const& offsets);

        ///
        /// \brief try move to next row
        ///
        /// @return return false means end of block
        ///
        bool TryMoveToNext();

        /// get current iterator value
        Value const& GetValue() const noexcept {
            return view_;
        }
    };

private:
    using Offset = ColumnIterator::Offset;

    std::vector<char> raw_data_;  /* store data as a serial chars buffer */
    std::vector<Offset> offsets_; /* column values offsets in `raw_data_`  */

public:
    ColumnData() = default;

    /// insert column value
    void Insert(std::string const& str);

    /// check if column empty
    bool IsEmpty() const noexcept {
        return raw_data_.empty();
    }

    /// get column data iterator
    ColumnIterator GetIt() const noexcept {
        return {raw_data_, offsets_};
    }
};

}  // namespace details

/// block-based data stream block data
class BlockData {
public:
    using ColumnData = details::ColumnData;
    using Value = ColumnData::Value;

private:
    std::vector<ColumnData> columns_;

public:
    explicit BlockData(ColumnIndex cols_count) : columns_(cols_count) {}

    /// insert row from data stream
    void InsertRow(IDatasetStream::Row const& row);

    /// get column data
    ColumnData const& GetColumn(ColumnIndex id) const noexcept {
        return columns_[id];
    }

    /// check if block empty
    bool IsEmpty() const {
        /* if some column empty, then all block columns are empty */
        return columns_.front().IsEmpty();
    }
};

}  // namespace model
