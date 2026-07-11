#pragma once

#include <vector>

#include "core/model/table/idataset_stream.h"

namespace model {
std::vector<std::vector<int>> ToValueIdMappedColumns(model::IDatasetStream& data_stream);
}  // namespace model
