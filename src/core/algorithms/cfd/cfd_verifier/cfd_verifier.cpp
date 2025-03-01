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
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option{&string_rule_left_, kCFDRuleLeft, kDCFDRuleLeft,
                          std::vector<std::pair<std::string, std::string>>{}});
    RegisterOption(Option{&string_rule_right_, kCFDRuleRight, kDCFDRuleRight,
                          std::pair<std::string, std::string>{}});
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
    auto get_attr_id = [this](std::string const& attr_name) -> int {
        int attr_id = relation_->GetAttr(attr_name);
        if (attr_id == -1) {
            throw config::ConfigurationError("Attribute not found: " + attr_name);
        }
        return attr_id;
    };

    auto extract_item_ids =
            [this, &get_attr_id](std::vector<std::pair<std::string, std::string>> const& rule_part)
            -> std::vector<int> {
        std::vector<int> item_ids;
        for (auto const& [attr_name, item_name] : rule_part) {
            int attr_id = get_attr_id(attr_name);

            if (item_name != "_") {
                int item_id = relation_->GetItem(attr_id, item_name);
                if (item_id == -1) {
                    throw config::ConfigurationError("Item not found in item universe: " +
                                                     item_name);
                }
                item_ids.push_back(item_id);
            } else {
                item_ids.push_back(-1 - attr_id);
            }
        }
        return item_ids;
    };

    std::vector<int> cfd_left_id = extract_item_ids(string_rule_left_);
    std::vector<int> cfd_right_id = extract_item_ids({string_rule_right_});

    cfd_ = std::make_pair(std::move(cfd_left_id), cfd_right_id.back());

    LOG(DEBUG) << "Starting CFD verification...";
    LOG(DEBUG) << "\tRule to verify: " << cfd::Output::CFDToString(cfd_, relation_);

    auto verification_time = ::util::TimedInvoke(&CFDVerifier::VerefyCFD, this);
    LOG(DEBUG) << "CFD verification took " << std::to_string(verification_time) << "ms";

    auto stats_calculation_time = ::util::TimedInvoke(&CFDVerifier::CalculateStatistics, this);
    LOG(DEBUG) << "Statistics calculation took " << std::to_string(stats_calculation_time) << "ms";

    return verification_time + stats_calculation_time;
}

void CFDVerifier::VerefyCFD() {
    stats_calculator_ = CFDStatsCalculator(relation_, std::move(cfd_));
}

}  // namespace algos::cfd_verifier
