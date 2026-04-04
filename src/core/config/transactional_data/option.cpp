#include "core/config/transactional_data/option.h"

#include "core/config/names_and_descriptions.h"

namespace config {

CommonOption<model::InputFormatType> const kInputFormatOpt{names::kInputFormat,
                                                           descriptions::kDInputFormat};

CommonOption<model::ColumnIndex> const kTIdColumnOpt{names::kTIdColumnIndex,
                                                     descriptions::kDTIdColumnIndex, 0u};

CommonOption<model::ColumnIndex> const kItemColumnOpt{names::kItemColumnIndex,
                                                      descriptions::kDItemColumnIndex, 1u};

CommonOption<bool> const kFirstColumnTIdOpt{names::kFirstColumnTId, descriptions::kDFirstColumnTId,
                                            false};

}  // namespace config
