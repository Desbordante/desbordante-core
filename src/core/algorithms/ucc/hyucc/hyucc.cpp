#include "hyucc.h"

#include <bits/chrono.h>  // for duration_cast, operator-
#include <list>           // for _List_iterator, list
#include <stdexcept>      // for runtime_error
#include <utility>        // for move

#include <boost/move/utility_core.hpp>  // for move
#include <easylogging++.h>              // for Writer, CDEBUG, LOG

#include "fd/hycommon/preprocessor.h"           // for Preprocess, RestoreAg...
#include "fd/hycommon/types.h"                  // for ClusterId, IdPairs, PLIs
#include "inductor.h"                           // for Inductor
#include "primitive_collection.h"               // for PrimitiveCollection
#include "sampler.h"                            // for Sampler
#include "table/column_layout_relation_data.h"  // for ColumnLayoutRelationData
#include "table/relational_schema.h"            // for RelationalSchema
#include "ucc/hyucc/model/non_ucc_list.h"       // for NonUCCList
#include "ucc/hyucc/model/ucc_tree.h"           // for UCCTree
#include "ucc/ucc.h"                            // for UCC
#include "validator.h"                          // for Validator

namespace algos {

void HyUCC::LoadDataInternal() {
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_, is_null_equal_null_);

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: UCC mining is meaningless.");
    }
}

unsigned long long HyUCC::ExecuteInternal() {
    using namespace hy;
    using namespace hyucc;
    auto const start_time = std::chrono::system_clock::now();

    auto [plis, pli_records, og_mapping] = Preprocess(relation_.get());
    auto const plis_shared = std::make_shared<PLIs>(std::move(plis));
    auto const pli_records_shared = std::make_shared<Rows>(std::move(pli_records));

    hyucc::Sampler sampler(plis_shared, pli_records_shared, threads_num_);

    auto ucc_tree = std::make_unique<UCCTree>(relation_->GetNumColumns());
    Inductor inductor(ucc_tree.get());
    Validator validator(ucc_tree.get(), plis_shared, pli_records_shared, threads_num_);

    IdPairs comparison_suggestions;

    while (true) {
        LOG(DEBUG) << "Sampling...";
        NonUCCList non_uccs = sampler.GetNonUCCs(comparison_suggestions);

        LOG(DEBUG) << "Inducing...";
        inductor.UpdateUCCTree(std::move(non_uccs));

        LOG(DEBUG) << "Validating...";
        comparison_suggestions = validator.ValidateAndExtendCandidates();

        if (comparison_suggestions.empty()) {
            break;
        }
    }

    auto uccs = ucc_tree->FillUCCs();
    RegisterUCCs(std::move(uccs), og_mapping);

    LOG(DEBUG) << "Mined UCCs:";
    for (model::UCC const& ucc : UCCList()) {
        LOG(DEBUG) << ucc.ToString();
    }

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

void HyUCC::RegisterUCCs(std::vector<boost::dynamic_bitset<>>&& uccs,
                         std::vector<hy::ClusterId> const& og_mapping) {
    std::shared_ptr<RelationalSchema const> const& schema = relation_->GetSharedPtrSchema();
    for (auto&& ucc : uccs) {
        boost::dynamic_bitset<> mapped_ucc =
                hy::RestoreAgreeSet(ucc, og_mapping, schema->GetNumColumns());
        ucc_collection_.Register(schema, std::move(mapped_ucc));
    }
}

}  // namespace algos
