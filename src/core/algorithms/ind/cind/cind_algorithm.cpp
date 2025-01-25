#include "cind_algorithm.h"

#include <memory>

#include "algorithms/create_algorithm.h"
#include "config/equal_nulls/option.h"
#include "config/error/option.h"
#include "config/mem_limit/option.h"
#include "config/tabular_data/input_tables/option.h"
#include "config/thread_number/option.h"
#include "util/timed_invoke.h"

namespace algos::cind {
Cind::Cind(std::vector<std::string_view> phase_names) : Algorithm(std::move(phase_names)) {
    CreateSpiderAlgo();

    RegisterOptions();
    MakeLoadOptsAvailable();
}

void Cind::CreateSpiderAlgo() {
    spider_algo_ = CreateAlgorithmInstance<Spider>(AlgorithmType::spider);

    domains_ = spider_algo_->domains_;
}

void Cind::RegisterOptions() {
    // Spider options
    RegisterOption(config::kTablesOpt(&spider_algo_->input_tables_));
    RegisterOption(config::kEqualNullsOpt(&spider_algo_->is_null_equal_null_));
    RegisterOption(config::kThreadNumberOpt(&spider_algo_->threads_num_));
    RegisterOption(config::kMemLimitMbOpt(&spider_algo_->mem_limit_mb_));
    RegisterOption(config::kErrorOpt(&spider_algo_->max_ind_error_));
}

void Cind::MakeLoadOptsAvailable() {
    // MakeOptionsAvailable({config::kTablesOpt.GetName(), config::kEqualNullsOpt.GetName(),
    //                       config::kThreadNumberOpt.GetName(), config::kMemLimitMbOpt.GetName()});
}

// void Cind::MakeExecuteOptsAvailable() {
//     MakeOptionsAvailable({config::kErrorOpt.GetName()});
// }

void Cind::LoadDataInternal() {
    time_ = util::TimedInvoke(&Algorithm::LoadData, spider_algo_);
}

bool Cind::SetExternalOption(std::string_view option_name, boost::any const& value) {
    try {
        spider_algo_->SetOption(option_name, value);
        return true;
    } catch (config::ConfigurationError&) {
    }
    return false;
}

void Cind::AddSpecificNeededOptions(std::unordered_set<std::string_view>& previous_options) const {
    auto const& spider_options = spider_algo_->GetNeededOptions();
    previous_options.insert(spider_options.begin(), spider_options.end());
}

unsigned long long Cind::ExecuteInternal() {
    auto time = spider_algo_->Execute();
    ++time_;
    return time;
}

void Cind::ResetState() {}
}  // namespace algos::cind