#include "pyro.h"

#include <chrono>
#include <mutex>
#include <thread>

#include <easylogging++.h>

#include "config/error/option.h"
#include "config/max_lhs/option.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/thread_number/option.h"
#include "core/fd_g1_strategy.h"
#include "core/key_g1_strategy.h"

namespace algos {

std::mutex searchSpacesMutex;

Pyro::Pyro() : PliBasedFDAlgorithm({kDefaultPhaseName}) {
    RegisterOptions();
    ucc_consumer_ = [this](auto const& key) { this->DiscoverUcc(key); };
    fd_consumer_ = [this](auto const& fd) {
        this->DiscoverFd(fd);
        this->FDAlgorithm::RegisterFd(fd.lhs_, fd.rhs_);
    };
}

void Pyro::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(config::ErrorOpt(&parameters_.max_ucc_error));
    RegisterOption(config::MaxLhsOpt(&parameters_.max_lhs));
    RegisterOption(config::ThreadNumberOpt(&parameters_.parallelism));
    RegisterOption(Option{&parameters_.seed, kSeed, kDSeed, 0});
}

void Pyro::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({config::MaxLhsOpt.GetName(), config::ErrorOpt.GetName(),
                          config::ThreadNumberOpt.GetName(), kSeed});
}

void Pyro::ResetStateFd() {
    search_spaces_.clear();
}

unsigned long long Pyro::ExecuteInternal() {
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

    int next_id = 0;
    if (parameters_.is_find_keys) {
        std::unique_ptr<DependencyStrategy> strategy;
        if (parameters_.ucc_error_measure == "g1prime") {
            strategy = std::make_unique<KeyG1Strategy>(parameters_.max_ucc_error,
                                                       parameters_.error_dev);
        } else {
            throw std::runtime_error("Unknown key error measure.");
        }
        search_spaces_.push_back(std::make_unique<SearchSpace>(next_id++, std::move(strategy),
                                                               schema, launch_pad_order));
    }
    if (parameters_.is_find_fds) {
        for (auto& rhs : schema->GetColumns()) {
            std::unique_ptr<DependencyStrategy> strategy;
            if (parameters_.ucc_error_measure == "g1prime") {
                strategy = std::make_unique<FdG1Strategy>(rhs.get(), parameters_.max_ucc_error,
                                                          parameters_.error_dev);
            } else {
                throw std::runtime_error("Unknown key error measure.");
            }
            search_spaces_.push_back(std::make_unique<SearchSpace>(next_id++, std::move(strategy),
                                                                   schema, launch_pad_order));
        }
    }
    unsigned long long init_time_millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                                              std::chrono::system_clock::now() - start_time)
                                              .count();

    start_time = std::chrono::system_clock::now();
    unsigned int total_error_calc_count = 0;
    unsigned long long total_ascension = 0;
    unsigned long long total_trickle = 0;
    double progress_step = 100.0 / search_spaces_.size();

    const auto work_on_search_space = [this, &progress_step](
        std::list<std::unique_ptr<SearchSpace>>& search_spaces,
        ProfilingContext* profiling_context, int id) {
        unsigned long long millis = 0;
        while (true) {
            auto thread_start_time = std::chrono::system_clock::now();
            std::unique_ptr<SearchSpace> polled_space;
            {
                std::scoped_lock<std::mutex> lock(searchSpacesMutex);
                if (search_spaces.empty()) {
                    break;
                }
                polled_space = std::move(search_spaces.front());
                search_spaces.pop_front();
            }
            LOG(TRACE) << "Thread" << id << " got SearchSpace";
            polled_space->SetContext(profiling_context);
            polled_space->EnsureInitialized();
            polled_space->Discover();
            AddProgress(progress_step);

            millis += std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now() - thread_start_time)
                          .count();
        }
        //cout << "Thread" << id << " stopped working, ELAPSED TIME: " << millis << "ms.\n";
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < parameters_.parallelism; i++) {
        //std::thread();
        threads.emplace_back(work_on_search_space, std::ref(search_spaces_),
                             profiling_context.get(), i);
    }

    for (int i = 0; i < parameters_.parallelism; i++) {
        threads[i].join();
    }

    SetProgress(100);
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);

    LOG(INFO) << boost::format{"FdG1 error calculation: %1% ms"} % (FdG1Strategy::nanos_ / 1000000);
    LOG(INFO) << "Init time: " << init_time_millis << "ms";
    LOG(INFO) << "Time: " << elapsed_milliseconds.count() << " milliseconds";
    LOG(INFO) << "Error calculation count: " << total_error_calc_count;
    LOG(INFO) << "Total ascension time: " << total_ascension << "ms";
    LOG(INFO) << "Total trickle time: " << total_trickle << "ms";
    LOG(INFO) << "Total intersection time: " << model::PositionListIndex::micros_ / 1000 << "ms";
    LOG(INFO) << "HASH: " << PliBasedFDAlgorithm::Fletcher16();
    return elapsed_milliseconds.count();
}

}  // namespace algos
