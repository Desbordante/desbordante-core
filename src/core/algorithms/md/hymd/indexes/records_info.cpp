#include "algorithms/md/hymd/indexes/records_info.h"

#include <string>
#include <vector>

#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered_map.hpp>
#include <easylogging++.h>

#include "algorithms/md/hymd/indexes/global_value_identifier.h"
#include "index.h"
#include "md/hymd/indexes/dictionary_compressor.h"
#include "table/idataset_stream.h"

namespace {
using namespace algos::hymd::indexes;
using ValueMapType = boost::unordered::unordered_flat_map<std::string, GlobalValueIdentifier>;

std::shared_ptr<DictionaryCompressor> MakeCompressor(model::IDatasetStream& table,
                                                     ValueMapType& value_map,
                                                     std::vector<std::string>& values,
                                                     GlobalValueIdentifier& next_value_id) {
    std::size_t records_processed = 0;
    std::size_t const attributes_num = table.GetNumberOfColumns();
    std::vector<GlobalValueIdentifier> row_values(attributes_num);
    std::shared_ptr<DictionaryCompressor> compressor =
            std::make_shared<DictionaryCompressor>(attributes_num);
    while (table.HasNextRow()) {
        std::vector<std::string> record = table.GetNextRow();
        std::size_t record_size = record.size();
        if (record_size != attributes_num) {
            LOG(WARNING) << "Unexpected number of columns for a record, skipping (expected "
                         << attributes_num << ", got " << record_size
                         << "). Records processed so far: " << records_processed << ".";
            continue;
        }
        for (model::Index i = 0; i != attributes_num; ++i) {
            std::string& value = record[i];
            auto [it, is_value_new] = value_map.try_emplace(value, next_value_id);
            if (is_value_new) {
                values.push_back(std::move(value));
                ++next_value_id;
            }
            row_values[i] = it->second;
        }
        compressor->AddRecord(row_values);
        ++records_processed;
    }
    return compressor;
}
}  // namespace

namespace algos::hymd::indexes {
std::unique_ptr<RecordsInfo> RecordsInfo::CreateFrom(model::IDatasetStream& left_table) {
    ValueMapType value_map;
    std::vector<std::string> values;
    GlobalValueIdentifier next_value_id = 0;
    std::shared_ptr<DictionaryCompressor> compressor =
            MakeCompressor(left_table, value_map, values, next_value_id);
    return std::make_unique<RecordsInfo>(std::move(values), std::move(compressor));
}

std::unique_ptr<RecordsInfo> RecordsInfo::CreateFrom(model::IDatasetStream& left_table,
                                                     model::IDatasetStream& right_table) {
    ValueMapType value_map;
    std::vector<std::string> values;
    GlobalValueIdentifier next_value_id = 0;
    std::shared_ptr<DictionaryCompressor> left_compressor =
            MakeCompressor(left_table, value_map, values, next_value_id);
    std::shared_ptr<DictionaryCompressor> right_compressor =
            MakeCompressor(right_table, value_map, values, next_value_id);
    return std::make_unique<RecordsInfo>(std::move(values), std::move(left_compressor),
                                         std::move(right_compressor));
}
}  // namespace algos::hymd::indexes
