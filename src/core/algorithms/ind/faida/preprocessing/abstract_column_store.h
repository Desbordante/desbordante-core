#pragma once

#include <filesystem>
#include <fstream>

#include "algorithms/ind/faida/hashing/hashing.h"
#include "irow_iterator.h"
#include "model/table/column_combination.h"
#include "model/table/idataset_stream.h"
#include "model/table/relational_schema.h"

namespace algos::faida {

using ColumnIndex = model::ColumnIndex;
using TableIndex = model::TableIndex;

class AbstractColumnStore {
public:
    using HashedTableSample = std::vector<std::vector<std::size_t>>;

private:
    std::string const dir_name_ = "temp";

    std::filesystem::path sample_file_;
    unsigned sample_size_ = 0;

protected:
    enum class ColumnProperty : char { ORDINARY, CONSTANT, NULL_CONSTANT };

    std::unique_ptr<RelationalSchema> schema_;
    int const sample_goal_;
    std::vector<ColumnProperty> column_properties_;

    size_t const null_hash_;

    AbstractColumnStore(int sample_goal, size_t null_hash)
        : sample_goal_(sample_goal), null_hash_(null_hash){};

    void LoadData(std::string const& dataset_name, TableIndex table_idx,
                  model::IDatasetStream& input_data);

    std::filesystem::path PrepareDir(std::string const& dataset_name, TableIndex table_idx);
    void WriteSample(std::vector<std::vector<std::string>> const& rows);

    virtual std::filesystem::path PrepareDirNext(std::filesystem::path dir,
                                                 TableIndex table_idx) = 0;
    virtual void WriteColumnsAndSample(model::IDatasetStream& data_stream) = 0;

    HashedTableSample ReadSample() const;

    size_t hash(std::string const& str) const {
        size_t curr_hash = hashing::CalcMurmurHash(str);

        if (curr_hash == null_hash_ && !str.empty()) {
            // Avoid collision with null_hash_
            curr_hash += 1;
        }

        return curr_hash;
    }

public:
    AbstractColumnStore(AbstractColumnStore&& other) = default;

    bool IsConstantCol(ColumnIndex col_idx) const {
        return column_properties_[col_idx] == AbstractColumnStore::ColumnProperty::CONSTANT;
    }

    bool IsNullCol(ColumnIndex col_idx) const {
        return column_properties_[col_idx] == AbstractColumnStore::ColumnProperty::NULL_CONSTANT;
    }

    RelationalSchema const* GetSchema() const {
        return schema_.get();
    }

    HashedTableSample GetSample() const {
        return ReadSample();
    }

    virtual std::unique_ptr<IRowIterator> GetRows(
            std::unordered_set<ColumnIndex> const& columns) const = 0;

    virtual ~AbstractColumnStore() = default;
};

}  // namespace algos::faida
