#pragma once

#include "config/common_option.h"
#include "model/table/column_index.h"
#include "model/transaction/input_format_type.h"

namespace config {

extern CommonOption<model::InputFormatType> const kInputFormatOpt;
extern CommonOption<model::ColumnIndex> const kTIdColumnOpt;
extern CommonOption<model::ColumnIndex> const kItemColumnOpt;
extern CommonOption<bool> const kFirstColumnTIdOpt;

}  // namespace config
