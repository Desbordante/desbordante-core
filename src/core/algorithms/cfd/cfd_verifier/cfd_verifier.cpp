#include "core/algorithms/cfd/cfd_verifier/cfd_verifier.h"

#include "core/algorithms/cfd/model/cfd_relation_data.h"
#include "core/algorithms/cfd/util/cfd_output_util.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/util/logger.h"
#include "core/util/timed_invoke.h"

namespace algos::cfd_verifier {

CFDVerifier::CFDVerifier() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void CFDVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto check_item = [this](cfd::RawCFD::RawItem const& item) {
        if (item.GetValue().has_value() && item.GetValue().value() != "_") {
            cfd::Item item_id = relation_->GetItem(item.GetAttribute(), item.GetValue().value());
            if (item_id == -1) {
                throw config::ConfigurationError(
                        "Value '" + item.GetValue().value() + "' for attribute " +
                        std::to_string(item.GetAttribute()) + " not found in dataset.");
            }
        }
    };

    auto check_items = [check_item](cfd::RawCFD::RawItems const& items) {
        for (auto const& item : items) {
            check_item(item);
        }
    };

    auto validate_rule = [check_item, check_items](cfd::RawCFD const& rule) {
        check_items(rule.GetLhs());
        check_item(rule.GetRhs());
    };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option{&raw_cfd_rule_, kCFDRule, kDCFDRule, cfd::RawCFD{}}.SetValueCheck(
            validate_rule));
    RegisterOption(Option{&minconf_, kMinimumConfidence, kDMinimumConfidence, 0.0});
    RegisterOption(Option{&minsup_, kMinimumSupport, kDMinimumSupport, 0});
}

void CFDVerifier::LoadDataInternal() {
    relation_ = cfd::CFDRelationData::CreateFrom(*input_table_);

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: CFD verifying is meaningless.");
    }
}

void CFDVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kCFDRule, kMinimumSupport, kMinimumConfidence});
}

unsigned long long CFDVerifier::ExecuteInternal() {
    auto build_item_id = [this](cfd::RawCFD::RawItem const& item) -> cfd::Item {
        auto const& [attr_id, item_name] = item;
        if (item_name.has_value() && item_name.value() != "_") {
            return relation_->GetItem(attr_id, item_name.value());
        } else {
            // Negative values (-1 - attr_id) are used to represent wildcards,
            // indicating that the rule applies to any value in this column
            return -1 - attr_id;
        }
    };

    auto build_item_ids = [&build_item_id](cfd::RawCFD::RawItems const& rule_part) -> cfd::Itemset {
        cfd::Itemset item_ids;
        item_ids.reserve(rule_part.size());

        for (auto const& item : rule_part) {
            item_ids.push_back(build_item_id(item));
        }

        return item_ids;
    };

    cfd_ = {build_item_ids(raw_cfd_rule_.GetLhs()), build_item_id(raw_cfd_rule_.GetRhs())};

    LOG_DEBUG("Starting CFD verification...");
    LOG_DEBUG("\tRule to verify: {}", cfd::Output::CFDToString(cfd_, relation_));

    auto verification_time = ::util::TimedInvoke(&CFDVerifier::VerifyCFD, this);
    LOG_DEBUG("CFD verification took {} ms", verification_time);

    auto stats_calculation_time = ::util::TimedInvoke(&CFDVerifier::CalculateStatistics, this);
    LOG_DEBUG("Statistics calculation took {} ms ", stats_calculation_time);

    return verification_time + stats_calculation_time;
}

void CFDVerifier::VerifyCFD() {
    stats_calculator_ = CFDStatsCalculator(relation_, std::move(cfd_));
}

}  // namespace algos::cfd_verifier
