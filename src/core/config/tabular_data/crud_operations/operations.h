#pragma once

#include "delete/option.h"
#include "insert/option.h"
#include "update_new/option.h"
#include "update_old/option.h"
#include "config/names.h"

const std::vector<std::string_view> CRUD_OPTIONS{config::names::kInsertStatements, 
                                                 config::names::kDeleteStatements,
                                                 config::names::kUpdateOldStatements,
                                                 config::names::kUpdateNewStatements};

