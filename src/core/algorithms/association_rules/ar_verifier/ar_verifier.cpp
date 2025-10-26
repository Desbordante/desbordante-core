#include "ar_verifier.h"

#include <chrono>

#include <easylogging++.h>

#include "config/ar_data/threshold/option.h"
#include "config/ar_data/transactional_data_config.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "util/timed_invoke.h"

namespace algos::ar_verifier {
ARVerifier::ARVerifier() : Algorithm({}) {
    RegisterOptions();
    using namespace config::names;
    MakeOptionsAvailable({kTable, kInputFormat});
}

void ARVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto check_rule_part = [this](std::list<std::string> const& string_rule_part) {
        std::vector<std::string> const& item_names_map = transactional_data_->GetItemUniverse();
        for (auto const& item_name : string_rule_part) {
            auto it = std::ranges::find(item_names_map, item_name);
            if (it == item_names_map.end()) {
                throw config::ConfigurationError(
                        "Item in left rule part not found in item universe: " + item_name);
            }
        }
    };

    auto check_rule_not_empty = [](std::list<std::string> const& string_rule_part) {
        if (string_rule_part.empty())
            throw config::ConfigurationError("Got an empty rule: AR verifying is meaningless.");
    };

    auto combined_rule_check = [check_rule_not_empty,
                                check_rule_part](std::list<std::string> const& string_rule_part) {
        check_rule_not_empty(string_rule_part);
        check_rule_part(string_rule_part);
    };

    auto opts = config::TransactionalDataOptions(&transactional_data_params_);
    RegisterOption(std::move(opts.input_table));
    RegisterOption(std::move(opts.input_format));
    RegisterOption(std::move(opts.first_column_tid));
    RegisterOption(std::move(opts.item_column));
    RegisterOption(std::move(opts.tid_column));

    RegisterOption(config::kMinimumConfidenceOpt(&minconf_));
    RegisterOption(config::kMinimumSupportOpt(&minconf_));

    RegisterOption(Option{&string_rule_left_, kARuleLeft, kDARuleLeft, std::list<std::string>{}}
                           .SetValueCheck(std::move(combined_rule_check)));
    RegisterOption(Option{&string_rule_right_, kARuleRight, kDARuleRight, std::list<std::string>{}}
                           .SetValueCheck(std::move(check_rule_part)));
}

void ARVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kARuleLeft, kARuleRight, kMinimumSupport, kMinimumConfidence});
}

void ARVerifier::LoadDataInternal() {
    transactional_data_ = transactional_data_params_.CreateFrom();
    if (transactional_data_->GetNumTransactions() == 0) {
        throw std::runtime_error("Got an empty dataset: AR verifying is meaningless.");
    }
}

unsigned long long ARVerifier::ExecuteInternal() {
    ar_ids_ = model::ArIDs(string_rule_left_, string_rule_right_, transactional_data_.get(),
                           minconf_, minsup_);
    LOG(DEBUG) << "Parameters of ARVerifier:";
    LOG(DEBUG) << "\tInput table: " << transactional_data_params_.input_table->GetRelationName();
    LOG(DEBUG) << "\tInput format: " << transactional_data_params_.input_format;
    LOG(DEBUG) << "\tARule to verify: "
               << model::ARStrings(ar_ids_, transactional_data_.get()).ToString();

    auto const verification_time = ::util::TimedInvoke(&ARVerifier::VerifyAR, this);

    LOG(DEBUG) << "AR verification took " << std::to_string(verification_time) << "ms";

    auto const stats_calculation_time = ::util::TimedInvoke(&ARVerifier::CalculateStatistics, this);

    LOG(DEBUG) << "Statistics calculation took " << std::to_string(stats_calculation_time) << "ms";

    return verification_time + stats_calculation_time;
}

void ARVerifier::VerifyAR() {
    stats_calculator_ = ARStatsCalculator(transactional_data_, ar_ids_);
}
}  // namespace algos::ar_verifier
