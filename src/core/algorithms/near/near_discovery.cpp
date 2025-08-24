#include "near_discovery.h"

#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"

namespace algos {

NeARDiscovery::NeARDiscovery(std::vector<std::string_view> phase_names)
    : Algorithm(std::move(phase_names)) {
    using namespace config::names;
    RegisterOptions();
    MakeOptionsAvailable({kTable, kInputFormat});
}

NeARDiscovery::NeARDiscovery() : NeARDiscovery({kDefaultPhaseName}) {}

void NeARDiscovery::LoadDataInternal() {
    switch (input_format_) {
        case InputFormat::singular:
            transactional_data_ = model::TransactionalData::CreateFromSingular(
                    *input_table_, tid_column_index_, item_column_index_);
            break;
        case InputFormat::tabular:
            transactional_data_ =
                    model::TransactionalData::CreateFromTabular(*input_table_, first_column_tid_);
            break;
        default:
            assert(0);
    }
    if (transactional_data_->GetNumTransactions() == 0) {
        throw std::runtime_error("Got an empty dataset: NeAR mining is meaningless.");
    }
}

void NeARDiscovery::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto sing_eq = [](InputFormat input_format) { return input_format == +InputFormat::singular; };
    auto tab_eq = [](InputFormat input_format) { return input_format == +InputFormat::tabular; };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option{&first_column_tid_, kFirstColumnTId, kDFirstColumnTId, false});
    RegisterOption(Option{&item_column_index_, kItemColumnIndex, kDItemColumnIndex, 1u});
    RegisterOption(Option{&tid_column_index_, kTIdColumnIndex, kDTIdColumnIndex, 0u});
    RegisterOption(Option{&input_format_, kInputFormat, kDInputFormat}.SetConditionalOpts(
            {{sing_eq, {kTIdColumnIndex, kItemColumnIndex}}, {tab_eq, {kFirstColumnTId}}}));
}

}  // namespace algos
