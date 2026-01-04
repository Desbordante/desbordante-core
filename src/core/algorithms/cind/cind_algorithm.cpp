#include "cind_algorithm.h"

#include <cstdint>
#include <memory>

#include "condition_miners/cinderella.h"
#include "condition_miners/pli_cind.h"
#include "core/algorithms/cind/types.h"
#include "core/algorithms/create_algorithm.h"
#include "core/config/conditions/algo_type/option.h"
#include "core/config/conditions/completeness/option.h"
#include "core/config/conditions/condition_type/option.h"
#include "core/config/conditions/validity/option.h"
#include "core/config/equal_nulls/option.h"
#include "core/config/error/option.h"
#include "core/config/mem_limit/option.h"
#include "core/config/tabular_data/input_tables/option.h"
#include "core/config/thread_number/option.h"
#include "core/util/timed_invoke.h"

namespace algos::cind {
CindAlgorithm::CindAlgorithm(std::vector<std::string_view> phase_names)
    : Algorithm(std::move(phase_names)) {
    CreateSpiderAlgo();

    RegisterOption(config::kAlgoTypeOpt(&algo_type_));
    MakeOptionsAvailable({config::kAlgoTypeOpt.GetName()});
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
    timings_.load =
            static_cast<std::uint64_t>(util::TimedInvoke(&Algorithm::LoadData, spider_algo_));
    CreateCindMinerAlgo();
}

void CindAlgorithm::CreateCindMinerAlgo() {
    if (algo_type_._value == AlgoType::cinderella) {
        cind_miner_ = std::make_unique<Cinderella>(spider_algo_->input_tables_);
    } else {
        cind_miner_ = std::make_unique<PliCind>(spider_algo_->input_tables_);
    }
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
    auto const spider_exec_time = static_cast<std::uint64_t>(spider_algo_->Execute());
    auto const cind_exec_time =
            static_cast<std::uint64_t>(cind_miner_->Execute(spider_algo_->INDList()));

    timings_.compute = spider_exec_time + cind_exec_time;
    timings_.total = timings_.load + timings_.compute;

    return static_cast<unsigned long long>(timings_.total);
}

void CindAlgorithm::ResetState() {
    timings_.load = 0;
    timings_.compute = 0;
    timings_.total = 0;
}
}  // namespace algos::cind
