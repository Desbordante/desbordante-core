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
Cind::Cind(std::vector<std::string_view> phase_names)
    : Algorithm(std::move(phase_names)) {
    RegisterOptions();
    MakeLoadOptsAvailable();
}

void Cind::RegisterOptions() {
    // Spider options
    // RegisterOption(config::kTablesOpt(&input_tables_));
    // RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));
    // RegisterOption(config::kThreadNumberOpt(&spider_threads_num_));
    // RegisterOption(config::kMemLimitMbOpt(&spider_mem_limit_mb_));
}

void Cind::MakeLoadOptsAvailable() {
    // MakeOptionsAvailable({config::kTablesOpt.GetName(), config::kEqualNullsOpt.GetName(),
    //                       config::kThreadNumberOpt.GetName(), config::kMemLimitMbOpt.GetName()});
}

// void Cind::MakeExecuteOptsAvailable() {
//     MakeOptionsAvailable({config::kErrorOpt.GetName()});
// }

void Cind::LoadDataInternal() {
    // auto const create_domains = [&] {
    //     domains_ = std::make_shared<std::vector<model::ColumnDomain>>(
    //             model::ColumnDomain::CreateFrom(input_tables_, spider_mem_limit_mb_, spider_threads_num_));
    // };
    // time_ = util::TimedInvoke(create_domains);
    // spider_algo_ = CreateAlgorithmInstance<Spider>(AlgorithmType::spider, domains_, is_null_equal_null_);
    // spider_algo_->ExecutePrepare();
}

// bool Cind::SetExternalOption(std::string_view option_name, boost::any const& value) {
//     try {
//         spider_algo_->SetOption(option_name, value);
//         return true;
//     } catch (config::ConfigurationError&) {
//     }
//     return false;
// }


unsigned long long Cind::ExecuteInternal() {
    // auto time = spider_algo_->Execute();
    // return time;
    return ++time_;
}

void Cind::ResetState() {
    
}
}  // namespace algos::cind