#include "tks.h"

#include <climits>

#include "core/config/names.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/parser/spmf_parser/spmf_parser.h"

namespace algos::tks {

TKS::TKS() {
    RegisterOptions();
    MakeOptionsAvailable({"spmf_file_path"});
}

void TKS::RegisterOptions() {
    DESBORDANTE_OPTION_USING;
    
    RegisterOption(config::Option{
        &input_file_path_, 
        "spmf_file_path", 
        "Path to SPMF sequence file", 
        std::filesystem::path{}
    });
    RegisterOption(config::Option{&k_, "k", "Number of top patterns to find", 10});
    RegisterOption(config::Option{&min_pattern_length_, "min_pattern_length", 
                                   "Minimum pattern length", 1});
    RegisterOption(config::Option{&max_pattern_length_, "max_pattern_length", 
                                   "Maximum pattern length", 1000});
    RegisterOption(config::Option{&max_gap_, "max_gap", 
                                   "Maximum gap between itemsets", INT_MAX});
    RegisterOption(config::Option{&show_sequence_identifiers_, "show_sequence_ids",
                                   "Show sequence identifiers in output", false});
}

void TKS::ResetState() {
    patterns_ = std::priority_queue<PatternTKS>{};
    engine_.reset();
}

void TKS::LoadDataInternal() {
}

void TKS::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({"k", "min_pattern_length", "max_pattern_length", 
                         "max_gap", "show_sequence_ids"});
}

unsigned long long TKS::ExecuteInternal() {
    if (input_file_path_.empty()) {
        throw std::runtime_error("Input file path must be specified");
    }
    if (k_ <= 0) {
        throw std::runtime_error("Parameter k must be greater than 0");
    }
    if (min_pattern_length_ > max_pattern_length_) {
        throw std::runtime_error("min_pattern_length cannot exceed max_pattern_length");
    }
    
    parser::SPMFParser spmf_parser(input_file_path_);
    auto spmf_data = spmf_parser.Parse();
    
    engine_ = std::make_unique<TKSEngine>();
    engine_->setMaximumPatternLength(max_pattern_length_);
    engine_->setMinimumPatternLength(min_pattern_length_);
    engine_->setMaxGap(max_gap_);
    engine_->showSequenceIdentifiersInOutput(show_sequence_identifiers_);
    
    patterns_ = engine_->runAlgorithm(input_file_path_.string(), "", k_);
    
    return patterns_.size();
}

}  // namespace algos::tks
