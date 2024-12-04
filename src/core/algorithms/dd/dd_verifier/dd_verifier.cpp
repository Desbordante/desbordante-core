#include "dd_verifier.h"

#include <utility>

#include "descriptions.h"
#include "names.h"
#include "option_using.h"
#include "tabular_data/input_table/option.h"

namespace algos::dd {
    DDVerifier::DDVerifier(DD dd) : Algorithm({}), dd_(std::move(dd)) {
        RegisterOptions();
        MakeOptionsAvailable({config::kTableOpt.GetName()});
    }

    void DDVerifier::RegisterOptions() {
        DESBORDANTE_OPTION_USING;
        config::InputTable default_table;
        RegisterOption(config::kTableOpt(&input_table_));
        RegisterOption(Option{&num_rows_, kNumRows, kDNumRows, 0U});
        RegisterOption(Option{&num_columns_, kNumColumns, kDNUmColumns, 0U});
    }
    void DDVerifier::LoadDataInternal() {
        relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_, false);  // nulls are
        // ignored
        input_table_->Reset();
        typed_relation_ = model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_,
                                                                           false);  // nulls are ignored
    }
    void DDVerifier::MakeExecuteOptsAvailable() {
        using namespace config::names;
        MakeOptionsAvailable({kNumRows, kNumColumns});
    }

    /*VerifyDD(){
     *get tuple of pairs, which holds lhs
     *Check this pairs on holding rhs{
     *if not hold{
     *break;
     *NOT HOLDS
     *}
     *}
     *return OK;
     *
     *}
     *
     *
     *
     */

}
