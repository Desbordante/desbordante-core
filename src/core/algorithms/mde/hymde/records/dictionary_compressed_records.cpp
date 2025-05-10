#include "algorithms/mde/hymde/records/dictionary_compressed_records.h"

#include <boost/unordered/unordered_flat_map.hpp>
#include <easylogging++.h>

#include "model/index.h"
#include "util/desbordante_assume.h"
#include "util/get_preallocated_vector.h"

namespace {
using namespace algos::hymde::records;
using ValueMapType = boost::unordered::unordered_flat_map<std::string, GlobalValueIdentifier>;

std::shared_ptr<DictionaryCompressed::Table const> MakeCompr(
        model::IDatasetStream& table, ValueMapType& value_map, std::vector<std::string>& values,
        GlobalValueIdentifier& next_value_id) {
    std::size_t const attributes_num = table.GetNumberOfColumns();
    DictionaryCompressed::Table table_records;
    while (table.HasNextRow()) {
        std::vector<std::string> record = table.GetNextRow();
        std::size_t const record_size = record.size();
        if (record_size != attributes_num) {
            LOG(WARNING) << "Unexpected number of columns for a record, skipping (expected "
                         << attributes_num << ", got " << record_size
                         << "). Records processed so far: " << table_records.size() << ".";
            continue;
        }
        DictionaryCompressed::CompressedRecord compressed_record =
                util::GetPreallocatedVector<GlobalValueIdentifier>(record_size);
        for (std::string& value : record) {
            auto [it, is_value_new] = value_map.try_emplace(value, next_value_id);
            if (is_value_new) {
                values.push_back(std::move(value));
                ++next_value_id;
            }
            compressed_record.push_back(it->second);
        }
        table_records.push_back(std::move(compressed_record));
    }
    return std::make_shared<DictionaryCompressed::Table const>(std::move(table_records));
}
}  // namespace

namespace algos::hymde::records {
std::unique_ptr<DictionaryCompressed> DictionaryCompressed::Compress(
        model::IDatasetStream& left) {
    ValueMapType value_map;
    std::vector<model::mde::TableValue> values;
    GlobalValueIdentifier next_value_id = 0;
    std::shared_ptr<Table const> left_comp = MakeCompr(left, value_map, values, next_value_id);
    return std::make_unique<DictionaryCompressed>(std::move(values), std::move(left_comp));
}

std::unique_ptr<DictionaryCompressed> DictionaryCompressed::Compress(
        model::IDatasetStream& left, model::IDatasetStream& right) {
    ValueMapType value_map;
    std::vector<model::mde::TableValue> values;
    GlobalValueIdentifier next_value_id = 0;
    std::shared_ptr<Table const> left_comp = MakeCompr(left, value_map, values, next_value_id);
    std::shared_ptr<Table const> right_comp = MakeCompr(right, value_map, values, next_value_id);
    return std::make_unique<DictionaryCompressed>(std::move(values), std::move(left_comp),
                                                         std::move(right_comp));
}
}  // namespace algos::hymde::records
