#pragma once

#include "algorithms/association_rules/ar_algorithm_enums.h"
#include "config/exceptions.h"
#include "config/names_and_descriptions.h"
#include "config/option.h"
#include "config/tabular_data/input_table/option.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/column_index.h"
#include "model/transaction/transactional_data.h"

namespace config {

struct TransactionalDataParams {
    InputTable input_table;
    algos::InputFormat input_format = algos::InputFormat::singular;
    model::ColumnIndex tid_column_index = 0;
    model::ColumnIndex item_column_index = 1;
    bool first_column_tid = false;

    [[nodiscard]] std::unique_ptr<model::TransactionalData> CreateFrom() const {
        switch (input_format) {
            case algos::InputFormat::singular:
                return model::TransactionalData::CreateFromSingular(*input_table, tid_column_index,
                                                                    item_column_index);
            case algos::InputFormat::tabular:
                return model::TransactionalData::CreateFromTabular(*input_table, first_column_tid);
            default:
                throw ConfigurationError("Unsupported input format");
        }
    }
};

struct TransactionalDataOptions {
    Option<InputTable> input_table;
    Option<algos::InputFormat> input_format;
    Option<model::ColumnIndex> tid_column;
    Option<model::ColumnIndex> item_column;
    Option<bool> first_column_tid;

    explicit TransactionalDataOptions(TransactionalDataParams* params)
        : input_table(kTableOpt(&params->input_table)),
          input_format(
                  Option<algos::InputFormat>{&params->input_format, names::kInputFormat,
                                             descriptions::kDInputFormat,
                                             algos::InputFormat::singular}
                          .SetConditionalOpts({{[](algos::InputFormat const fmt) {
                                                    return fmt == +algos::InputFormat::singular;
                                                },
                                                {names::kTIdColumnIndex, names::kItemColumnIndex}},
                                               {[](algos::InputFormat const fmt) {
                                                    return fmt == +algos::InputFormat::tabular;
                                                },
                                                {names::kFirstColumnTId}}})),
          tid_column(Option{&params->tid_column_index, names::kTIdColumnIndex,
                            descriptions::kDTIdColumnIndex, 0u}),
          item_column(Option{&params->item_column_index, names::kItemColumnIndex,
                             descriptions::kDItemColumnIndex, 1u}),
          first_column_tid(Option{&params->first_column_tid, names::kFirstColumnTId,
                                  descriptions::kDFirstColumnTId, false}) {}
};
}  // namespace config