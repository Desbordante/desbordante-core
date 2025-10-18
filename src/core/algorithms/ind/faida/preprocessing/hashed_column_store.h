#pragma once

#include <cstddef>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "abstract_column_store.h"
#include "ind/faida/preprocessing/irow_iterator.h"

namespace model {
class IDatasetStream;
}  // namespace model

namespace algos::faida {

class HashedColumnStore : public AbstractColumnStore {
private:
    class RowIterator : public IRowIterator {
    private:
        int const default_block_size_ = 65536;
        std::vector<std::optional<std::ifstream>> hashed_col_streams_;
        Block curr_block_;
        size_t curr_block_size_;
        bool has_next_;

    public:
        explicit RowIterator(std::vector<std::optional<std::ifstream>>&& hashed_columns)
            : hashed_col_streams_(std::move(hashed_columns)),
              curr_block_size_(0),
              has_next_(true) {}

        RowIterator(RowIterator const& other) = delete;
        RowIterator& operator=(RowIterator const& other) = delete;
        ~RowIterator() override;

        bool HasNextBlock() override;

        size_t GetBlockSize() const override {
            return curr_block_size_;
        }

        Block const& GetNextBlock() override;
    };

    int const read_buff_size_ = 65536;
    std::vector<std::filesystem::path> column_files_;

    HashedColumnStore(unsigned num_of_columns, int sample_goal, size_t null_hash)
        : AbstractColumnStore(sample_goal, null_hash), column_files_(num_of_columns) {}

    std::filesystem::path PrepareDirNext(std::filesystem::path dir, TableIndex table_idx) override;

protected:
    void WriteColumnsAndSample(model::IDatasetStream& data_stream) override;

public:
    HashedColumnStore(HashedColumnStore&& other) = default;

    std::unique_ptr<IRowIterator> GetRows(
            std::unordered_set<ColumnIndex> const& columns) const override;
    static std::unique_ptr<AbstractColumnStore> CreateFrom(std::string const& dataset_name,
                                                           TableIndex table_idx,
                                                           model::IDatasetStream& data_stream,
                                                           int sample_goal, size_t null_hash);
};

}  // namespace algos::faida
