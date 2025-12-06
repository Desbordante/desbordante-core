#pragma once

#include "core/config/names.h"
#include "core/config/tabular_data/crud_operations/delete/option.h"
#include "core/config/tabular_data/crud_operations/insert/option.h"
#include "core/config/tabular_data/crud_operations/update/option.h"

std::vector<std::string_view> const kCrudOptions{config::names::kInsertStatements,
                                                 config::names::kDeleteStatements,
                                                 config::names::kUpdateStatements};
