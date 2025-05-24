#pragma once

#include <cstddef>
#include <memory>

#include "algorithms/mde/hymde/records/global_value_identifier.h"
#include "algorithms/mde/record.h"
#include "model/table/idataset_stream.h"

namespace algos::hymde::records {
class DictionaryCompressed {
public:
    using Values = std::vector<model::mde::TableValue>;
    using CompressedRecord = std::vector<GlobalValueIdentifier>;
    using Table = std::vector<CompressedRecord>;

private:
    Values const values_;
    std::shared_ptr<Table const> const left_table_;
    std::shared_ptr<Table const> const right_table_;

public:
    [[nodiscard]] Table const& GetLeftTable() const noexcept {
        return *left_table_;
    }

    [[nodiscard]] Table const& GetRightTable() const noexcept {
        return *right_table_;
    }

    [[nodiscard]] bool HoldsOneTable() const noexcept {
        return left_table_ == right_table_;
    }

    [[nodiscard]] Values const& GetValues() const noexcept {
        return values_;
    }

    [[nodiscard]] std::size_t GetTotalPairsCount() const noexcept {
        return GetLeftTable().size() * GetRightTable().size();
    }

    static std::unique_ptr<DictionaryCompressed> Compress(model::IDatasetStream& left);
    static std::unique_ptr<DictionaryCompressed> Compress(model::IDatasetStream& left,
                                                          model::IDatasetStream& right);

    DictionaryCompressed(std::vector<model::mde::TableValue> values,
                         std::shared_ptr<Table const> left_table)
        : values_(std::move(values)),
          left_table_(std::move(left_table)),
          right_table_(left_table_) {}

    DictionaryCompressed(std::vector<model::mde::TableValue> values,
                         std::shared_ptr<Table const> left_table,
                         std::shared_ptr<Table const> right_table)
        : values_(std::move(values)),
          left_table_(std::move(left_table)),
          right_table_(std::move(right_table)) {}
};
}  // namespace algos::hymde::records
