/** \file
 * \brief Mind algorithm
 *
 * Class definition to perform dataset stream projection using provided indices.
 */
#pragma once

#include <algorithm>
#include <cassert>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "model/table/column_index.h"
#include "model/table/dataset_stream_wrapper.h"
#include "model/table/idataset_stream.h"

namespace model {

///
/// \brief A dataset stream that performs a projection of the original dataset stream.
///
/// \note It's expected, that provided column indices are unique.
///
template <typename DatasetStream = std::shared_ptr<IDatasetStream>>
class DatasetStreamProjection final : public DatasetStreamWrapper<DatasetStream> {
public:
    using Row = typename DatasetStreamWrapper<DatasetStream>::Row;

private:
    std::vector<ColumnIndex> column_indices_;

public:
    template <typename Stream = DatasetStream>
    explicit DatasetStreamProjection(Stream&& stream,
                                     std::vector<ColumnIndex> const& column_indices)
        : DatasetStreamWrapper<DatasetStream>(std::forward<Stream>(stream)),
          column_indices_(column_indices) {
        /* Check, that provided indices are unique. */
        assert(std::set(column_indices_.begin(), column_indices_.end()).size() ==
               column_indices_.size());
    }

    Row GetNextRow() override {
        Row projected_row;
        projected_row.reserve(GetNumberOfColumns());

        Row row = this->stream_->GetNextRow();
        std::transform(column_indices_.begin(), column_indices_.end(),
                       std::back_inserter(projected_row),
                       [&row](ColumnIndex index) { return std::move(row[index]); });

        return projected_row;
    }

    [[nodiscard]] size_t GetNumberOfColumns() const override {
        return column_indices_.size();
    }

    [[nodiscard]] std::string GetColumnName(size_t index) const override {
        return this->stream_->GetColumnName(column_indices_[index]);
    }
};

}  // namespace model
