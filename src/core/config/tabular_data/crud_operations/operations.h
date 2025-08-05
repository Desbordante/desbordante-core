#pragma once

#include "config/names.h"
#include "delete/option.h"
#include "insert/option.h"
#include "update/option.h"

std::vector<std::string_view> const kCrudOptions{config::names::kInsertStatements,
                                                 config::names::kDeleteStatements,
                                                 config::names::kUpdateStatements};
