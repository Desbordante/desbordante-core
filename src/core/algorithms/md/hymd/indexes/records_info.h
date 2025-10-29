#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "algorithms/md/hymd/indexes/dictionary_compressor.h"

namespace model {
class IDatasetStream;
}  // namespace model

namespace algos::hymd::indexes {

class RecordsInfo {
    std::vector<std::string> const values_;
    std::shared_ptr<DictionaryCompressor const> const left_compressor_;
    std::shared_ptr<DictionaryCompressor const> const right_compressor_;

public:
    [[nodiscard]] bool OneTableGiven() const noexcept {
        return left_compressor_ == right_compressor_;
    }

    [[nodiscard]] DictionaryCompressor const& GetLeftCompressor() const noexcept {
        return *left_compressor_;
    }

    [[nodiscard]] DictionaryCompressor const& GetRightCompressor() const noexcept {
        return *right_compressor_;
    }

    [[nodiscard]] std::size_t GetTotalPairsNum() const noexcept {
        return GetLeftCompressor().GetNumberOfRecords() * GetRightCompressor().GetNumberOfRecords();
    }

    RecordsInfo(std::vector<std::string> values,
                std::shared_ptr<DictionaryCompressor> compressor) noexcept
        : values_(std::move(values)),
          left_compressor_(std::move(compressor)),
          right_compressor_(left_compressor_) {}

    RecordsInfo(std::vector<std::string> values,
                std::shared_ptr<DictionaryCompressor> left_compressor,
                std::shared_ptr<DictionaryCompressor> right_compressor) noexcept
        : values_(std::move(values)),
          left_compressor_(std::move(left_compressor)),
          right_compressor_(std::move(right_compressor)) {}

    std::vector<std::string> const& GetValues() const noexcept {
        return values_;
    }

    static std::unique_ptr<RecordsInfo> CreateFrom(model::IDatasetStream& left_table);

    static std::unique_ptr<RecordsInfo> CreateFrom(model::IDatasetStream& left_table,
                                                   model::IDatasetStream& right_table);
};

}  // namespace algos::hymd::indexes
