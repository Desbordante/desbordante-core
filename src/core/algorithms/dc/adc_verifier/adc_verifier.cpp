#include "core/algorithms/dc/adc_verifier/adc_verifier.h"

#include <ranges>
#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "core/algorithms/algo_factory.h"
#include "core/config/error/option.h"
#include "core/config/error_measure/option.h"
#include "core/config/names.h"
#include "core/config/option.h"
#include "core/config/option_using.h"

static algos::StdParamsMap GetCsvConf(config::InputTable input_table) {
    using namespace config::names;
    auto parser = dynamic_cast<CSVParser const*>(input_table.get());
    if (!parser) throw std::runtime_error("Couldn't get parser");

    // Looks stinky, couldn't set InputTable via options
    auto sep = parser->GetSeparator();
    auto path = parser->GetPath();
    auto has_header = parser->HasHeader();
    CSVConfig csv_config = {path, sep, has_header};

    return {{kCsvConfig, csv_config}};
}

namespace algos {

void ADCVerifier::LoadDataInternal() {
    // Set only csv config for DCVerifier because other options are not available yet here
    algos::StdParamsMap csv_config = GetCsvConf(input_table_);
    verifier_ = CreateAndLoadAlgorithm<DCVerifier>(csv_config);
}

void ADCVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(Option<std::string>(&dc_string_, kDenialConstraint, kDDenialConstraint, ""));
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kADCErrorMeasureOpt(&measure_type_));
    RegisterOption(config::kErrorOpt(&error_treshold_));
}

void ADCVerifier::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::names::kDenialConstraint});
    MakeOptionsAvailable({config::names::kADCErrorMeasure});
    MakeOptionsAvailable({config::names::kError});
}

long long unsigned ADCVerifier::ExecuteInternal() {
    auto start = std::chrono::system_clock::now();
    verifier_->SetOption(config::names::kDoCollectViolations, true);
    verifier_->SetOption(config::names::kDenialConstraint, dc_string_);

    verifier_->ExecuteInternal();

    dc::Measure measure(verifier_);
    error_ = 1 - measure.Get(measure_type_);

    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start);

    return elapsed_time.count();
}

ADCVerifier::ADCVerifier() : Algorithm() {
    using namespace config::names;

    RegisterOptions();
    MakeOptionsAvailable({config::names::kTable});
};

}  // namespace algos