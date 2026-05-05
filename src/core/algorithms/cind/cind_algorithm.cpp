#include "cind_algorithm.h"

#include <cstdint>
#include <memory>

#include "condition_miners/cinderella.h"
#include "condition_miners/cure_cind.h"
#include "condition_miners/pli_cind.h"
#include "core/algorithms/cind/types.h"
#include "core/config/conditions/algo_type/option.h"
#include "core/config/conditions/completeness/option.h"
#include "core/config/conditions/condition_type/option.h"
#include "core/config/conditions/validity/option.h"
#include "core/config/equal_nulls/option.h"
#include "core/config/error/option.h"
#include "core/config/exceptions.h"
#include "core/config/mem_limit/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option.h"
#include "core/config/tabular_data/input_tables/option.h"
#include "core/config/thread_number/option.h"

namespace algos::cind {
CindAlgorithm::CindAlgorithm() {
    CreateSpiderAlgo();

    RegisterOption(config::kAlgoTypeOpt(&algo_type_));
    MakeOptionsAvailable({config::kAlgoTypeOpt.GetName()});
    RegisterSpiderOptions();
}

void CindAlgorithm::CreateSpiderAlgo() {
    spider_algo_ = std::make_unique<Spider>();
}

void CindAlgorithm::RegisterSpiderOptions() {
    RegisterOption(config::kTablesOpt(&spider_algo_->input_tables_));
    RegisterOption(config::kEqualNullsOpt(&spider_algo_->is_null_equal_null_));
    RegisterOption(config::kThreadNumberOpt(&spider_algo_->threads_num_));
    RegisterOption(config::kMemLimitMbOpt(&spider_algo_->mem_limit_mb_));
    RegisterOption(config::kErrorOpt(&spider_algo_->max_ind_error_));
}

void CindAlgorithm::LoadDataInternal() {
    spider_algo_->LoadData();
    CreateCindMinerAlgo();
}

void CindAlgorithm::CreateCindMinerAlgo() {
    if (algo_type_ == AlgoType::kCinderella) {
        cind_miner_ = std::make_unique<Cinderella>(spider_algo_->input_tables_);
    } else if (algo_type_ == AlgoType::kCureCind) {
        cind_miner_ = std::make_unique<CureCind>(spider_algo_->input_tables_);
    } else {
        cind_miner_ = std::make_unique<PliCind>(spider_algo_->input_tables_);
    }
    RegisterCindMinerOptions();
}

void CindAlgorithm::RegisterCindMinerOptions() {
    RegisterOption(config::kValidityOpt(&cind_miner_->min_validity_));
    RegisterOption(config::kCompletenessOpt(&cind_miner_->min_completeness_));
    RegisterOption(config::kConditionTypeOpt(&cind_miner_->condition_type_));

    if (algo_type_._value == AlgoType::cure_cind) {
        auto* cure = static_cast<CureCind*>(cind_miner_.get());
        config::Option<unsigned int> support_opt{&cure->min_support_,
                                                 config::names::kCindMinSupport,
                                                 config::descriptions::kDCindMinSupport, 2u};
        support_opt.SetValueCheck([](unsigned int support) {
            if (support < 1) {
                throw config::ConfigurationError("ERROR: support must be >= 1.");
            }
        });
        RegisterOption(std::move(support_opt));
    }
}

void CindAlgorithm::MakeExecuteOptsAvailable() {
    std::vector<std::string_view> opts{config::kValidityOpt.GetName(),
                                       config::kCompletenessOpt.GetName(),
                                       config::kConditionTypeOpt.GetName()};
    if (algo_type_._value == AlgoType::cure_cind) {
        opts.push_back(config::names::kCindMinSupport);
    }
    MakeOptionsAvailable(opts);
}

bool CindAlgorithm::SetExternalOption(std::string_view option_name, boost::any const& value) {
    try {
        spider_algo_->SetOption(option_name, value);
        return true;
    } catch (config::ConfigurationError&) {
    }
    return false;
}

void CindAlgorithm::AddSpecificNeededOptions(
        std::unordered_set<std::string_view>& previous_options) const {
    auto const& spider_options = spider_algo_->GetNeededOptions();
    previous_options.insert(spider_options.begin(), spider_options.end());
}

void CindAlgorithm::ExecuteInternal() {
    spider_algo_->Execute();
    cind_miner_->Execute(spider_algo_->INDList());
}

void CindAlgorithm::ResetState() {}
}  // namespace algos::cind
