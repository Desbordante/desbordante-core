#pragma once

#include <string>
#include <vector>

#include "core/model/table/idataset_stream.h"

namespace model {
struct TableHeader {
    std::string table_name;
    std::vector<std::string> column_names;

    static TableHeader FromDatasetStream(IDatasetStream& stream) {
        std::vector<std::string> column_names;
        std::size_t const attr_num = stream.GetNumberOfColumns();
        column_names.reserve(attr_num);
        for (size_t i = 0; i != attr_num; ++i) {
            column_names.push_back(stream.GetColumnName(i));
        }
        return {stream.GetRelationName(), std::move(column_names)};
    }
};
}  // namespace model
