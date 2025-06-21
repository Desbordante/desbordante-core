#pragma once

#include <cstddef>
#include <vector>

#include "algorithms/md/hymd/indexes/compressed_records.h"
#include "algorithms/md/hymd/indexes/keyed_position_list_index.h"
#include "model/index.h"
#include "model/table/idataset_stream.h"

namespace algos::hymd::indexes {

class DictionaryCompressor {
private:
    std::vector<KeyedPositionListIndex> plis_;
    CompressedRecords records_;
    std::size_t records_processed_ = 0;

public:
    explicit DictionaryCompressor(std::size_t attribute_num);
    void AddRecord(std::vector<GlobalValueIdentifier> const& record);

    [[nodiscard]] std::size_t GetPliNumber() const {
        return plis_.size();
    }

    [[nodiscard]] KeyedPositionListIndex const& GetPli(model::Index column_index) const {
        return plis_[column_index];
    }

    [[nodiscard]] CompressedRecords const& GetRecords() const noexcept {
        return records_;
    }

    [[nodiscard]] std::size_t GetNumberOfRecords() const noexcept {
        return records_processed_;
    }
};

}  // namespace algos::hymd::indexes
