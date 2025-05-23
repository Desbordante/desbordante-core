#include "cfd_verifier.h"

#include <easylogging++.h>

#include "cfd/model/cfd_relation_data.h"
#include "cfd/util/cfd_output_util.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"
#include "util/timed_invoke.h"

namespace algos::cfd_verifier {

CFDVerifier::CFDVerifier() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void CFDVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto check_item_ids = [this](std::vector<CFDAttributeValuePair> const& rule_part) {
        auto get_attr_id = [this](std::string const& attr_name) -> cfd::AttributeIndex {
            cfd::AttributeIndex attr_id = relation_->GetAttr(attr_name);
            if (attr_id == -1) {
                throw config::ConfigurationError("Attribute not found: " + attr_name);
            }
            return attr_id;
        };

        for (auto const& [attr_name, item_name] : rule_part) {
            cfd::Item item_id = relation_->GetItem(get_attr_id(attr_name), item_name);
            if (item_id == -1 && item_name != "_") {
                throw config::ConfigurationError("Item not found in item universe: " + item_name);
            }
        }
    };

    auto validate_rule_part = [check_item_ids](std::vector<CFDAttributeValuePair> const& part) {
        check_item_ids(part);
    };

    auto validate_single_pair = [check_item_ids](CFDAttributeValuePair const& pair) {
        check_item_ids({pair});
    };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option{&string_rule_left_, kCFDRuleLeft, kDCFDRuleLeft,
                          std::vector<CFDAttributeValuePair>{}}
                           .SetValueCheck(validate_rule_part));
    RegisterOption(
            Option{&string_rule_right_, kCFDRuleRight, kDCFDRuleRight, CFDAttributeValuePair{}}
                    .SetValueCheck(validate_single_pair));
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
    MakeOptionsAvailable({kCFDRuleLeft, kCFDRuleRight, kMinimumSupport, kMinimumConfidence});
}

unsigned long long CFDVerifier::ExecuteInternal() {
    auto build_item_ids =
            [this](std::vector<CFDAttributeValuePair> const& rule_part) -> cfd::Itemset {
        cfd::Itemset item_ids;
        item_ids.reserve(rule_part.size());

        for (auto const& [attr_name, item_name] : rule_part) {
            cfd::AttributeIndex attr_id = relation_->GetAttr(attr_name);
            if (item_name != "_") {
                item_ids.push_back(relation_->GetItem(attr_id, item_name));
            } else {
                // Negative values (-1 - attr_id) are used to represent wildcards,
                // indicating that the rule applies to any value in this column
                item_ids.push_back(-1 - attr_id);
            }
        }

        return item_ids;
    };

    cfd_ = {build_item_ids(string_rule_left_), build_item_ids({string_rule_right_}).front()};

    LOG(DEBUG) << "Starting CFD verification...";
    LOG(DEBUG) << "\tRule to verify: " << cfd::Output::CFDToString(cfd_, relation_);

    auto verification_time = ::util::TimedInvoke(&CFDVerifier::VerifyCFD, this);
    LOG(DEBUG) << "CFD verification took " << std::to_string(verification_time) << "ms";

    auto stats_calculation_time = ::util::TimedInvoke(&CFDVerifier::CalculateStatistics, this);
    LOG(DEBUG) << "Statistics calculation took " << std::to_string(stats_calculation_time) << "ms";

    return verification_time + stats_calculation_time;
}

void CFDVerifier::VerifyCFD() {
    stats_calculator_ = CFDStatsCalculator(relation_, std::move(cfd_));
}

}  // namespace algos::cfd_verifier
