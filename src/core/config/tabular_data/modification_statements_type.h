#pragma once

#include <memory>

#include "model/table/table_row.h"
#include "util/dynamic_collection.h"

namespace config {
using ModificationStatements = std::shared_ptr<util::DynamicCollection<TableRow>>;
}  // namespace config
