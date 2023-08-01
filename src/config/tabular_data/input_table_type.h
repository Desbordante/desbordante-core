#pragma once

#include <memory>

#include "model/table/idataset_stream.h"

namespace config {
using InputTable = std::shared_ptr<model::IDatasetStream>;
}  // namespace config
