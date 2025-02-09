#include "cind_algorithm.h"

#include <memory>

#include "algorithms/create_algorithm.h"
#include "conditions/completeness/option.h"
#include "conditions/condition_type/option.h"
#include "conditions/validity/option.h"
#include "config/equal_nulls/option.h"
#include "config/error/option.h"
#include "config/mem_limit/option.h"
#include "config/tabular_data/input_tables/option.h"
#include "config/thread_number/option.h"
#include "cind/condition_miners/cind_miner.hpp"
#include "cind/condition_miners/cinderella.h"
#include "cind/condition_miners/pli_cind.h"
#include "util/timed_invoke.h"

namespace algos::cind {
CindAlgorithm::CindAlgorithm(std::vector<std::string_view> phase_names)
    : Algorithm(std::move(phase_names)) {
    CreateSpiderAlgo();

    RegisterSpiderOptions();
}

void CindAlgorithm::CreateSpiderAlgo() {
    spider_algo_ = CreateAlgorithmInstance<Spider>(AlgorithmType::spider);
}

void CindAlgorithm::RegisterSpiderOptions() {
    RegisterOption(config::kTablesOpt(&spider_algo_->input_tables_));
    RegisterOption(config::kEqualNullsOpt(&spider_algo_->is_null_equal_null_));
    RegisterOption(config::kThreadNumberOpt(&spider_algo_->threads_num_));
    RegisterOption(config::kMemLimitMbOpt(&spider_algo_->mem_limit_mb_));
    RegisterOption(config::kErrorOpt(&spider_algo_->max_ind_error_));
}

void CindAlgorithm::LoadDataInternal() {
    time_ = util::TimedInvoke(&Algorithm::LoadData, spider_algo_);
    CreateCindMinerAlgo();
}

void CindAlgorithm::CreateCindMinerAlgo() {
    cind_miner_ = std::make_unique<Cinderella>(spider_algo_->input_tables_);
    RegisterCindMinerOptions();
}

void CindAlgorithm::RegisterCindMinerOptions() {
    RegisterOption(config::kValidityOpt(&cind_miner_->min_validity_));
    RegisterOption(config::kCompletenessOpt(&cind_miner_->min_completeness_));
    RegisterOption(config::kConditionTypeOpt(&cind_miner_->condition_type_));
}

void CindAlgorithm::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::kValidityOpt.GetName(), config::kCompletenessOpt.GetName(),
                          config::kConditionTypeOpt.GetName()});
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

unsigned long long CindAlgorithm::ExecuteInternal() {
    auto time = spider_algo_->Execute();
    cind_miner_->Execute(spider_algo_->INDList());
    ++time_;
    return time;
}

void CindAlgorithm::ResetState() {}
}  // namespace algos::cind