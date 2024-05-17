#pragma once

#include "config/names.h"
#include "delete/option.h"
#include "insert/option.h"
#include "update_new/option.h"
#include "update_old/option.h"

const std::vector<std::string_view> kCrudOptions{
    config::names::kInsertStatements, config::names::kDeleteStatements,
    config::names::kUpdateOldStatements, config::names::kUpdateNewStatements};

