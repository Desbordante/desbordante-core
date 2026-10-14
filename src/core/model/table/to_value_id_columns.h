#pragma once

#include <vector>

#include "core/model/table/idataset_stream.h"
#include "core/model/table/value_id_column.h"

namespace model {
std::vector<ValueIdColumn> ToValueIdColumns(IDatasetStream& data_stream);
}  // namespace model
