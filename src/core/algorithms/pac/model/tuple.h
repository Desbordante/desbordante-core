#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/model/types/type.h"

namespace pac::model {
using Tuple = std::vector<std::byte const*>;
using Tuples = std::vector<Tuple>;

std::string TupleToString(Tuple const& tp, std::vector<::model::Type const*> const& types);
}  // namespace pac::model
