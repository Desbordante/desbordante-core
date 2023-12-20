#include "algorithms/ucc/pyroucc/pyroucc.h"

#include <chrono>
#include <mutex>
#include <thread>

#include <easylogging++.h>

#include "algorithms/fd/pyrocommon/core/key_g1_strategy.h"
#include "config/error/option.h"
#include "config/max_lhs/option.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"

namespace algos {

PyroUCC::PyroUCC() : UCCAlgorithm({kDefaultPhaseName}) {
    RegisterOptions();
    fd_consumer_ = nullptr;
    ucc_consumer_ = [this](auto const& ucc) {
        this->DiscoverUcc(ucc);
        ucc_collection_.Register(ucc.vertical_);
    };
}

void PyroUCC::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(config::ErrorOpt(&parameters_.max_ucc_error));
    RegisterOption(config::MaxLhsOpt(&parameters_.max_lhs));
    RegisterOption(Option{&parameters_.seed, kSeed, kDSeed, 0});
}

void PyroUCC::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({config::MaxLhsOpt.GetName(), config::ErrorOpt.GetName(), kSeed});
}

void PyroUCC::LoadDataInternal() {
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_, is_null_equal_null_);

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: UCC mining is meaningless.");
    }
}

void PyroUCC::ResetUCCAlgorithmState() {
    search_space_.reset(nullptr);
}

unsigned long long PyroUCC::ExecuteInternal() {
    auto start_time = std::chrono::system_clock::now();

    auto schema = relation_->GetSchema();

    auto profiling_context = std::make_unique<ProfilingContext>(
            parameters_, relation_.get(), ucc_consumer_, fd_consumer_, caching_method_,
            eviction_method_, caching_method_value_);

    std::function<bool(DependencyCandidate const&, DependencyCandidate const&)> launch_pad_order;
    if (parameters_.launch_pad_order == "arity") {
        launch_pad_order = DependencyCandidate::FullArityErrorComparator;
    } else if (parameters_.launch_pad_order == "error") {
        launch_pad_order = DependencyCandidate::FullErrorArityComparator;
    } else {
        throw std::runtime_error("Unknown comparator type");
    }

    std::unique_ptr<DependencyStrategy> strategy;
    if (parameters_.ucc_error_measure == "g1prime") {
        strategy =
                std::make_unique<KeyG1Strategy>(parameters_.max_ucc_error, parameters_.error_dev);
    } else {
        throw std::runtime_error("Unknown key error measure.");
    }
    search_space_ = std::make_unique<SearchSpace>(0, std::move(strategy), schema, launch_pad_order);
    unsigned long long init_time_millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                  std::chrono::system_clock::now() - start_time)
                                                  .count();

    start_time = std::chrono::system_clock::now();

    search_space_->SetContext(profiling_context.get());
    search_space_->EnsureInitialized();
    search_space_->Discover();
    SetProgress(100);

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);

    LOG(INFO) << "Init time: " << init_time_millis << "ms";
    LOG(INFO) << "Time: " << elapsed_milliseconds.count() << " milliseconds";
    LOG(INFO) << "Total intersection time: " << model::PositionListIndex::micros_ / 1000 << "ms";
    return elapsed_milliseconds.count();
}

}  // namespace algos
