#pragma once

#include "core/config/common_option.h"
#include "core/model/table/column_index.h"
#include "core/model/transaction/input_format_type.h"

namespace config {

extern CommonOption<model::InputFormatType> const kInputFormatOpt;
extern CommonOption<model::ColumnIndex> const kTIdColumnOpt;
extern CommonOption<model::ColumnIndex> const kItemColumnOpt;
extern CommonOption<bool> const kFirstColumnTIdOpt;

inline bool IsSingularFormat(model::InputFormatType format) {
    return format == +model::InputFormatType::singular;
}

inline bool IsTabularFormat(model::InputFormatType format) {
    return format == +model::InputFormatType::tabular;
}

}  // namespace config
