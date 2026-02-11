#include "ar_verifier.h"

#include "core/config/ar_minimum_conf/option.h"
#include "core/config/ar_minimum_support/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/config/transactional_data/option.h"
#include "core/model/transaction/input_format_type.h"
#include "core/util/logger.h"
#include "core/util/timed_invoke.h"

namespace algos::ar_verifier {
ARVerifier::ARVerifier() : Algorithm({}) {
    RegisterOptions();
    using namespace config::names;
    MakeOptionsAvailable({kTable, kInputFormat});
}

void ARVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto check_rule_part = [this](std::vector<std::string> const& string_rule_part) {
        std::vector<std::string> const& item_names_map = transactional_data_->GetItemUniverse();
        for (auto const& item_name : string_rule_part) {
            auto it = std::ranges::find(item_names_map, item_name);
            if (it == item_names_map.end()) {
                throw config::ConfigurationError("Item in rule part not found in item universe: " +
                                                 item_name);
            }
        }
    };

    auto check_rule_not_empty = [](std::vector<std::string> const& string_rule_part) {
        if (string_rule_part.empty())
            throw config::ConfigurationError("Got an empty rule: AR verifying is meaningless.");
    };

    auto combined_rule_check = [check_rule_not_empty,
                                check_rule_part](std::vector<std::string> const& string_rule_part) {
        check_rule_not_empty(string_rule_part);
        check_rule_part(string_rule_part);
    };

    RegisterOption(config::kTableOpt(&transactional_data_params_.input_table));
    RegisterOption(config::kInputFormatOpt(&transactional_data_params_.input_format_type)
                           .SetConditionalOpts(
                                   {{config::IsSingularFormat, {kTIdColumnIndex, kItemColumnIndex}},
                                    {config::IsTabularFormat, {kFirstColumnTId}}}));

    RegisterOption(config::kFirstColumnTIdOpt(&transactional_data_params_.first_column_tid));
    RegisterOption(config::kItemColumnOpt(&transactional_data_params_.item_column_index));
    RegisterOption(config::kTIdColumnOpt(&transactional_data_params_.tid_column_index));

    RegisterOption(config::kArMinimumSupportOpt(&minsup_));
    RegisterOption(config::kArMinimumConfidenceOpt(&minconf_));

    RegisterOption(Option{&string_rule_left_, kArLhsRule, kDArLhsRule, std::vector<std::string>{}}
                           .SetValueCheck(std::move(combined_rule_check)));
    RegisterOption(Option{&string_rule_right_, kArRhsRule, kDArRhsRule, std::vector<std::string>{}}
                           .SetValueCheck(std::move(check_rule_part)));
}

void ARVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kArLhsRule, kArRhsRule, kArMinimumSupport, kArMinimumConfidence});
}

void ARVerifier::LoadDataInternal() {
    transactional_data_ = ::model::TransactionalData::CreateFrom(transactional_data_params_);
    if (transactional_data_->GetNumTransactions() == 0) {
        throw std::runtime_error("Got an empty dataset: AR verifying is meaningless.");
    }
}

unsigned long long ARVerifier::ExecuteInternal() {
    ar_ids_ = ::model::ArIDs(string_rule_left_, string_rule_right_, transactional_data_.get(),
                             minconf_, minsup_);
    LOG_DEBUG("Parameters of ARVerifier:");
    LOG_DEBUG("\tInput table: {}", transactional_data_params_.input_table->GetRelationName());
    LOG_DEBUG("\tInput format: {}", transactional_data_params_.input_format_type._to_string());
    LOG_DEBUG("\tARule to verify: {}",
              ::model::ARStrings(ar_ids_, transactional_data_.get()).ToString());

    auto const verification_time = ::util::TimedInvoke(&ARVerifier::VerifyAR, this);

    LOG_DEBUG("AR verification took {} ms", std::to_string(verification_time));

    auto const stats_calculation_time = ::util::TimedInvoke(&ARVerifier::CalculateStatistics, this);

    LOG_DEBUG("Statistics calculation took {} ms", std::to_string(stats_calculation_time));

    return verification_time + stats_calculation_time;
}

void ARVerifier::VerifyAR() {
    stats_calculator_ = ARStatsCalculator(transactional_data_, ar_ids_);
}
}  // namespace algos::ar_verifier
