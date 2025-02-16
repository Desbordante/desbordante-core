#include "ar_verifier.h"

#include <chrono>
#include <stdexcept>

#include <easylogging++.h>

#include "config/equal_nulls/option.h"
#include "config/indices/option.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"
#include "util/timed_invoke.h"

namespace algos::ar_verifier {
ARVerifier::ARVerifier() : Algorithm({}) {
    RegisterOptions();
    using namespace config::names;
    MakeOptionsAvailable({kTable, kInputFormat});
}

void ARVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto sing_eq = [](InputFormat input_format) { return input_format == +InputFormat::singular; };
    auto tab_eq = [](InputFormat input_format) { return input_format == +InputFormat::tabular; };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option{&first_column_tid_, kFirstColumnTId, kDFirstColumnTId, false});
    RegisterOption(Option{&item_column_index_, kItemColumnIndex, kDItemColumnIndex, 1u});
    RegisterOption(Option{&string_rule_left_, kARuleLeft, kDARuleLeft, std::list<std::string>{}});
    RegisterOption(
            Option{&string_rule_right_, kARuleRight, kDARuleRight, std::list<std::string>{}});
    RegisterOption(Option{&minconf_, kMinimumConfidence, kDMinimumConfidence, 0.0});
    RegisterOption(Option{&minsup_, kMinimumSupport, kDMinimumSupport, 0.0});
    RegisterOption(Option{&tid_column_index_, kTIdColumnIndex, kDTIdColumnIndex, 0u});
    RegisterOption(Option{&input_format_, kInputFormat, kDInputFormat}.SetConditionalOpts(
            {{sing_eq, {kTIdColumnIndex, kItemColumnIndex}}, {tab_eq, {kFirstColumnTId}}}));
}

void ARVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kARuleLeft, kARuleRight, kMinimumSupport, kMinimumConfidence});
}

void ARVerifier::LoadDataInternal() {
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
        throw std::runtime_error("Got an empty dataset: AR verifying is meaningless.");
    }
}

unsigned long long ARVerifier::ExecuteInternal() {
    std::vector<std::string> const& item_names_map = transactional_data_->GetItemUniverse();

    std::vector<unsigned> ar_left_id;
    for (auto const& item_name : string_rule_left_) {
        auto it = std::ranges::find(item_names_map, item_name);
        if (it == item_names_map.end()) {
            throw std::runtime_error("Item in left rule part not found in item universe: " +
                                     item_name);
        }
        ar_left_id.push_back(std::distance(item_names_map.begin(), it));
    }

    std::vector<unsigned> ar_right_id;
    for (auto const& item_name : string_rule_right_) {
        auto it = std::ranges::find(item_names_map, item_name);
        if (it == item_names_map.end()) {
            throw std::runtime_error("Item in right rule part not found in item universe: " +
                                     item_name);
        }
        ar_right_id.push_back(std::distance(item_names_map.begin(), it));
    }

    ar_ids_ = model::ArIDs(std::move(ar_left_id), std::move(ar_right_id), minconf_, minsup_);
    if (string_rule_left_.empty()) {
        throw std::runtime_error("Got an empty rule: AR verifying is meaningless.");
    }

    LOG(INFO) << "Parameters of ARVerifier:";
    LOG(INFO) << "\tInput table: " << input_table_->GetRelationName();
    LOG(INFO) << "\tInput format: " << input_format_;
    LOG(INFO) << "\tARule to verify: "
              << model::ARStrings(ar_ids_, transactional_data_.get()).ToString();

    auto verification_time = ::util::TimedInvoke(&ARVerifier::VerifyAR, this);

    LOG(DEBUG) << "AR verification took " << std::to_string(verification_time) << "ms";

    auto stats_calculation_time = ::util::TimedInvoke(&ARVerifier::CalculateStatistics, this);

    LOG(DEBUG) << "Statistics calculation took " << std::to_string(stats_calculation_time) << "ms";

    return verification_time + stats_calculation_time;
}

void ARVerifier::VerifyAR() {
    stats_calculator_ = ARStatsCalculator(std::move(transactional_data_), std::move(ar_ids_));
}
}  // namespace algos::ar_verifier
