#include "core/algorithms/fd/pyrocommon/core/search_space.h"

#include <queue>
#include <ranges>
#include <variant>

#include "core/util/logger.h"

// TODO: extra careful with const& -> shared_ptr conversions via make_shared-smart pointer may
// delete the object - pass empty deleter [](*) {}

void SearchSpace::Discover() {
    while (true) {  // на второй итерации дропается
        std::optional<DependencyCandidate> launch_pad = PollLaunchPad();
        if (!launch_pad.has_value()) break;

        if (local_visitees_ == nullptr) {
            local_visitees_ =
                    std::make_unique<model::VerticalMap<VerticalInfo>>(context_->GetSchema());
        }

        bool is_dependency_found = Ascend(*launch_pad);
        ReturnLaunchPad(*launch_pad, !is_dependency_found);
    }
}

std::optional<DependencyCandidate> SearchSpace::PollLaunchPad() {
    while (true) {
        if (launch_pads_.empty()) {
            if (deferred_launch_pads_.empty()) return std::optional<DependencyCandidate>();

            launch_pads_.insert(deferred_launch_pads_.begin(), deferred_launch_pads_.end());
            deferred_launch_pads_.clear();
        }

        auto launch_pad = launch_pads_.extract(launch_pads_.begin()).value();

        // launchPads_.erase(launchPads_.begin());
        launch_pad_index_->Remove(launch_pad.vertical_);

        if (IsImpliedByMinDep(launch_pad.vertical_, global_visitees_.get()) ||
            (local_visitees_ != nullptr &&
             IsImpliedByMinDep(launch_pad.vertical_, local_visitees_.get()))) {
            launch_pad_index_->Remove(launch_pad.vertical_);
            continue;
        }

        auto superset_entries = global_visitees_->GetSupersetEntries(launch_pad.vertical_);
        if (local_visitees_ != nullptr) {
            auto local_superset_entries = local_visitees_->GetSupersetEntries(launch_pad.vertical_);
            auto end_iterator =
                    std::remove_if(local_superset_entries.begin(), local_superset_entries.end(),
                                   [](auto& entry) { return !entry.second->IsPruningSubsets(); });

            std::for_each(local_superset_entries.begin(), end_iterator,
                          [&superset_entries](auto& entry) { superset_entries.push_back(entry); });
        }
        if (superset_entries.empty()) return launch_pad;
        std::vector<boost::dynamic_bitset<>> superset_verticals;

        for (auto& entry : superset_entries) {
            superset_verticals.push_back(entry.first);
        }

        EscapeLaunchPad(launch_pad.vertical_, std::move(superset_verticals));
    }
}

// this move looks legit IMO
void SearchSpace::EscapeLaunchPad(boost::dynamic_bitset<> const& launch_pad,
                                  std::vector<boost::dynamic_bitset<>> pruning_supersets) {
    std::transform(
            pruning_supersets.begin(), pruning_supersets.end(), pruning_supersets.begin(),
            [this](auto& superset) { return ~superset - strategy_->GetIrrelevantColumns(); });

    auto pruning_function = [this, &launch_pad](auto const& hitting_set_candidate) -> bool {
        if (scope_ != nullptr &&
            scope_->GetAnySupersetEntry(hitting_set_candidate).second == nullptr) {
            return true;
        }

        auto launch_pad_candidate = launch_pad | hitting_set_candidate;

        if ((local_visitees_ == nullptr &&
             IsImpliedByMinDep(launch_pad_candidate, local_visitees_.get())) ||
            IsImpliedByMinDep(launch_pad_candidate, global_visitees_.get())) {
            return true;
        }

        if (launch_pad_index_->GetAnySubsetEntry(launch_pad_candidate).second != nullptr) {
            return true;
        }

        return false;
    };
    auto hitting_set = CalculateHittingSet(std::move(pruning_supersets), pruning_function);
    for (auto& escaping : hitting_set) {
        auto escaped_launch_pad_vertical = launch_pad | escaping;

        // assert, который не имплементнуть из-за трансформа
        LOG_TRACE("CreateDependencyCandidate while escaping launch pad");
        DependencyCandidate escaped_launch_pad =
                strategy_->CreateDependencyCandidate(escaped_launch_pad_vertical);
        LOG_DEBUG("Proposed launch pad arity: {} should be <= max_lhs: {}",
                  escaped_launch_pad.vertical_.count(), context_->GetParameters().max_lhs);
        if (escaped_launch_pad.vertical_.count() <= context_->GetParameters().max_lhs) {
            launch_pads_.insert(escaped_launch_pad);
            launch_pad_index_->Put(escaped_launch_pad.vertical_,
                                   std::make_unique<DependencyCandidate>(escaped_launch_pad));
        }
    }
}

void SearchSpace::AddLaunchPad(DependencyCandidate const& launch_pad) {
    launch_pads_.insert(launch_pad);
    launch_pad_index_->Put(launch_pad.vertical_, std::make_unique<DependencyCandidate>(launch_pad));
}

void SearchSpace::ReturnLaunchPad(DependencyCandidate const& launch_pad, bool is_defer) {
    if (is_defer && context_->GetParameters().is_defer_failed_launch_pads) {
        deferred_launch_pads_.push_back(launch_pad);
    } else {
        launch_pads_.insert(launch_pad);
    }
    launch_pad_index_->Put(launch_pad.vertical_, std::make_unique<DependencyCandidate>(launch_pad));
}

bool SearchSpace::Ascend(DependencyCandidate const& launch_pad) {
    if (strategy_->ShouldResample(launch_pad.vertical_, sample_boost_)) {
        LOG_TRACE("Resampling.");
        context_->CreateFocusedSample(launch_pad.vertical_, sample_boost_);
    }

    DependencyCandidate traversal_candidate = launch_pad;
    boost::optional<double> error;

    while (true) {
        if (context_->GetParameters().is_check_estimates) {
            CheckEstimate(strategy_.get(), traversal_candidate);
        }

        if (traversal_candidate.IsExact()) {
            // TODO: GetError()?
            error = traversal_candidate.error_.Get();

            bool can_be_dependency = *error <= strategy_->max_dependency_error_;
            local_visitees_->Put(traversal_candidate.vertical_,
                                 std::make_unique<VerticalInfo>(can_be_dependency, false, *error));
            if (can_be_dependency) break;
        } else {
            if (traversal_candidate.error_.GetMin() > strategy_->max_dependency_error_) {
                error.reset();
            } else {
                error = context_->GetParameters().is_estimate_only
                                ? traversal_candidate.error_.GetMean()
                                : strategy_->CalculateError(traversal_candidate.vertical_);
                // double errorDiff = *error - traversal_candidate.error_.GetMean();

                local_visitees_->Put(
                        traversal_candidate.vertical_,
                        std::make_unique<VerticalInfo>(error <= strategy_->max_dependency_error_,
                                                       false, *error));

                if (*error <= strategy_->max_dependency_error_) break;

                if (strategy_->ShouldResample(traversal_candidate.vertical_, sample_boost_)) {
                    LOG_TRACE("Resampling.");
                    context_->CreateFocusedSample(traversal_candidate.vertical_, sample_boost_);
                }
            }
        }

        if (traversal_candidate.vertical_.count() >=
                    context_->GetColumnLayoutRelationData()->GetNumColumns() -
                            strategy_->GetNumIrrelevantColumns() ||
            traversal_candidate.vertical_.count() >= context_->GetParameters().max_lhs) {
            break;
        }

        boost::optional<DependencyCandidate> next_candidate;
        int num_seen_elements = is_ascend_randomly_ ? 1 : -1;
        for (model::Index extension_column_index = 0;
             extension_column_index != context_->GetSchema()->GetNumColumns();
             ++extension_column_index) {
            if (traversal_candidate.vertical_[extension_column_index] ||
                strategy_->IsIrrelevantColumn(extension_column_index)) {
                continue;
            }
            auto extended_vertical = boost::dynamic_bitset<>(traversal_candidate.vertical_)
                                             .set(extension_column_index);

            if (scope_ != nullptr && scope_->GetSupersetEntries(extended_vertical).empty()) {
                continue;
            }

            bool is_subset_pruned = IsImpliedByMinDep(extended_vertical, global_visitees_.get());
            if (is_subset_pruned) {
                continue;
            }
            LOG_TRACE("CreateDependencyCandidate while ascending");
            DependencyCandidate extended_candidate =
                    strategy_->CreateDependencyCandidate(extended_vertical);

            if (!next_candidate ||
                (num_seen_elements == -1 &&
                 extended_candidate.error_.GetMean() < next_candidate->error_.GetMean()) ||
                (num_seen_elements != -1 && context_->NextInt(++num_seen_elements) == 0)) {
                next_candidate = extended_candidate;
            }
        }

        if (next_candidate) {
            traversal_candidate = *next_candidate;
        } else {
            break;
        }
    }

    if (!error) {
        error = strategy_->CalculateError(traversal_candidate.vertical_);
        [[maybe_unused]] double error_diff = *error - traversal_candidate.error_.GetMean();
        LOG_TRACE("Checking candidate... actual error: {}", *error);
    }

    if (*error <= strategy_->max_dependency_error_) {
        TrickleDown(traversal_candidate.vertical_, *error);

        if (recursion_depth_ == 0) {
            assert(scope_ == nullptr);
            global_visitees_->Put(
                    traversal_candidate.vertical_,
                    std::make_unique<VerticalInfo>(VerticalInfo::ForMinimalDependency()));
        }

        return true;
    } else {
        if (recursion_depth_ == 0) {
            assert(scope_ == nullptr);
            global_visitees_->Put(
                    traversal_candidate.vertical_,
                    std::make_unique<VerticalInfo>(VerticalInfo::ForMaximalNonDependency()));
        } else {
            local_visitees_->Put(traversal_candidate.vertical_,
                                 std::make_unique<VerticalInfo>(VerticalInfo::ForNonDependency()));
        }
    }
    return false;
}

void SearchSpace::CheckEstimate([[maybe_unused]] DependencyStrategy* strategy,
                                [[maybe_unused]] DependencyCandidate const& traversal_candidate) {
    LOG_DEBUG("Stepped into method 'checkEstimate' - not implemented yet being a debug method\n");
}

void SearchSpace::TrickleDown(boost::dynamic_bitset<> const& main_peak, double main_peak_error) {
    std::unordered_set<boost::dynamic_bitset<>> maximal_non_deps;
    auto alleged_min_deps =
            std::make_unique<model::VerticalMap<VerticalInfo>>(context_->GetSchema());
    auto peaks_comparator = [](auto& candidate1, auto& candidate2) -> bool {
        return DependencyCandidate::ArityComparator(candidate1, candidate2);
    };
    std::vector<DependencyCandidate> peaks;
    std::make_heap(peaks.begin(), peaks.end(), peaks_comparator);
    peaks.emplace_back(main_peak, model::ConfidenceInterval(main_peak_error), true);
    std::push_heap(peaks.begin(), peaks.end(), peaks_comparator);
    std::unordered_set<boost::dynamic_bitset<>> alleged_non_deps;

    while (!peaks.empty()) {
        auto peak = peaks.front();

        auto subset_deps = GetSubsetDeps(peak.vertical_, alleged_min_deps.get());
        if (!subset_deps.empty()) {
            std::pop_heap(peaks.begin(), peaks.end(), peaks_comparator);
            peaks.pop_back();

            auto peak_hitting_set = CalculateHittingSet(std::move(subset_deps));
            std::unordered_set<boost::dynamic_bitset<>> escaped_peak_verticals;

            for (auto& vertical : peak_hitting_set) {
                escaped_peak_verticals.insert(peak.vertical_ - vertical);
            }

            for (auto& escaped_peak_vertical : escaped_peak_verticals) {
                if (escaped_peak_vertical.count() > 0 &&
                    alleged_non_deps.find(escaped_peak_vertical) == alleged_non_deps.end()) {
                    LOG_TRACE("CreateDependencyCandidate as an escaped peak while trickling down");
                    auto escaped_peak = strategy_->CreateDependencyCandidate(escaped_peak_vertical);

                    if (escaped_peak.error_.GetMean() > strategy_->min_non_dependency_error_) {
                        alleged_non_deps.insert(escaped_peak_vertical);
                        continue;
                    }
                    if (IsKnownNonDependency(escaped_peak_vertical, local_visitees_.get()) ||
                        IsKnownNonDependency(escaped_peak_vertical, global_visitees_.get())) {
                        continue;
                    }
                    peaks.push_back(escaped_peak);
                    std::push_heap(peaks.begin(), peaks.end(), peaks_comparator);
                }
            }
            continue;
        }
        auto alleged_min_dep =
                TrickleDownFrom(std::move(peak), strategy_.get(), alleged_min_deps.get(),
                                alleged_non_deps, global_visitees_.get(), sample_boost_);
        if (!alleged_min_dep.has_value()) {
            std::pop_heap(peaks.begin(), peaks.end(), peaks_comparator);
            peaks.pop_back();
        }
    }

    for (auto& [alleged_min_dep, info] : alleged_min_deps->EntrySet()) {
        if (info->is_extremal_ && !global_visitees_->ContainsKey(alleged_min_dep)) {
            // TODO: Костыль -- info в нескольких местах должен храниться. ХЗ, кому он принадлежит,
            // пока копирую
            global_visitees_->Put(alleged_min_dep, std::make_unique<VerticalInfo>(*info));
            strategy_->RegisterDependency(alleged_min_dep, info->error_, *context_);
        }
    }

    auto alleged_min_deps_set = alleged_min_deps->KeySet();
    // TODO: костыль: CalculateHittingSet needs a list, but KeySet returns an unordered_set
    // ещё и морока с transform и unordered_set - мб вообще в лист переделать.
    auto alleged_max_non_deps_hs = CalculateHittingSet(std::vector<boost::dynamic_bitset<>>(
            alleged_min_deps_set.begin(), alleged_min_deps_set.end()));
    std::unordered_set<boost::dynamic_bitset<>> alleged_max_non_deps;

    for (auto& min_leave_out_vertical : alleged_max_non_deps_hs) {
        alleged_max_non_deps.insert(min_leave_out_vertical ^ main_peak);
    }

    LOG_DEBUG("* {} alleged maximum non-dependencies (UNIMPLEMENTED)", alleged_max_non_deps.size());
    // std::transform(alleged_max_non_deps_hs.begin(), alleged_max_non_deps_hs.end(),
    // alleged_max_non_deps.begin(),
    //         [mainPeak](auto minLeaveOutVertical) { return minLeaveOutVertical->Invert(*mainPeak);
    //         });

    // checking the consistency of all data model
    if (auto alleged_min_deps_key_set = alleged_min_deps->KeySet(); !std::all_of(
                alleged_min_deps_key_set.begin(), alleged_min_deps_key_set.end(),
                [main_peak](auto& vertical) -> bool { return vertical.is_subset_of(main_peak); })) {
        throw std::runtime_error("Main peak should contain all alleged min dependencies");
    }
    if (!std::all_of(
                alleged_max_non_deps.begin(), alleged_max_non_deps.end(),
                [main_peak](auto& vertical) -> bool { return vertical.is_subset_of(main_peak); })) {
        throw std::runtime_error("Main peak should contain all alleged max non-dependencies");
    }

    for (auto& alleged_max_non_dep : alleged_max_non_deps) {
        if (alleged_max_non_dep.count() == 0) continue;

        if (maximal_non_deps.find(alleged_max_non_dep) != maximal_non_deps.end() ||
            IsKnownNonDependency(alleged_max_non_dep, local_visitees_.get()) ||
            IsKnownNonDependency(alleged_max_non_dep, global_visitees_.get())) {
            continue;
        }

        double error =
                context_->GetParameters().is_estimate_only
                        ? strategy_->CreateDependencyCandidate(alleged_max_non_dep).error_.GetMean()
                        : strategy_->CalculateError(alleged_max_non_dep);
        bool is_non_dep = error > strategy_->min_non_dependency_error_;
        if (is_non_dep) {
            maximal_non_deps.insert(alleged_max_non_dep);
            local_visitees_->Put(alleged_max_non_dep,
                                 std::make_unique<VerticalInfo>(VerticalInfo::ForNonDependency()));
        } else {
            peaks.emplace_back(alleged_max_non_dep, model::ConfidenceInterval(error), true);
            std::push_heap(peaks.begin(), peaks.end(), peaks_comparator);
        }
    }

    if (peaks.empty()) {
        for (auto& [alleged_min_dep, info] : alleged_min_deps->EntrySet()) {
            if (!info->is_extremal_ && !global_visitees_->ContainsKey(alleged_min_dep)) {
                // TODO: тут надо сделать non-const - костыльный mutable; опять Info в двух местах
                // хранится
                info->is_extremal_ = true;
                global_visitees_->Put(alleged_min_dep, std::make_unique<VerticalInfo>(*info));
                strategy_->RegisterDependency(alleged_min_dep, info->error_, *context_);
            }
        }
    } else {
        LOG_DEBUG("* {} new peaks (UNIMPLEMENTED)", peaks.size());
        auto new_scope = std::make_unique<model::VerticalMap<boost::dynamic_bitset<>>>(
                context_->GetSchema());
        std::sort_heap(peaks.begin(), peaks.end(), peaks_comparator);
        for (auto& peak : peaks) {
            new_scope->Put(peak.vertical_,
                           std::make_unique<boost::dynamic_bitset<>>(peak.vertical_));
        }

        double new_sample_boost = sample_boost_ * sample_boost_;
        LOG_DEBUG("* Increasing sampling boost factor to {}", new_sample_boost);

        auto scope_verticals = new_scope->KeySet();
        // TODO: что делать с strategy, globalVisitees?
        auto nested_search_space = std::make_unique<SearchSpace>(
                -1, strategy_->CreateClone(), std::move(new_scope), std::move(global_visitees_),
                context_->GetSchema(), launch_pads_.key_comp(), recursion_depth_ + 1,
                sample_boost_ * context_->GetParameters().sample_booster);
        nested_search_space->SetContext(context_);

        std::unordered_set<model::Index> scope_columns;
        for (auto& vertical : scope_verticals) {
            util::ForEachIndex(vertical, [&](auto i) { scope_columns.insert(i); });
        }
        for (auto& scope_column : scope_columns) {
            LOG_TRACE("CreateDependencyCandidate while building a nested search space");
            nested_search_space->AddLaunchPad(strategy_->CreateDependencyCandidate(
                    boost::dynamic_bitset<>(context_->GetSchema()->GetNumColumns())
                            .set(scope_column)));
        }
        num_nested_++;
        // std::cout << static_cast<std::string>(*strategy_) << ' ';
        // std::cout << numNested << std::endl;
        nested_search_space->MoveInLocalVisitees(std::move(local_visitees_));
        nested_search_space->Discover();
        global_visitees_ = nested_search_space->MoveOutGlobalVisitees();
        local_visitees_ = nested_search_space->MoveOutLocalVisitees();

        for (auto& [alleged_min_dep, info] : alleged_min_deps->EntrySet()) {
            if (!IsImpliedByMinDep(alleged_min_dep, global_visitees_.get())) {
                // TODO: тут надо сделать non-const - костыльный mutable; опять Info в двух местах
                // хранится
                info->is_extremal_ = true;
                global_visitees_->Put(alleged_min_dep, std::make_unique<VerticalInfo>(*info));
                strategy_->RegisterDependency(alleged_min_dep, info->error_, *context_);
            }
        }
    }
}

std::optional<boost::dynamic_bitset<>> SearchSpace::TrickleDownFrom(
        DependencyCandidate min_dep_candidate, DependencyStrategy* strategy,
        model::VerticalMap<VerticalInfo>* alleged_min_deps,
        std::unordered_set<boost::dynamic_bitset<>>& alleged_non_deps,
        model::VerticalMap<VerticalInfo>* global_visitees, double boost_factor) {
    if (min_dep_candidate.error_.GetMin() > strategy->max_dependency_error_) {
        throw std::runtime_error(
                "Error in trickleDownFrom: minDepCandidate's error should be <= maxError");
    }

    bool are_all_parents_known_non_deps = true;
    if (min_dep_candidate.vertical_.count() > 1) {
        std::priority_queue<DependencyCandidate, std::vector<DependencyCandidate>,
                            std::function<bool(DependencyCandidate&, DependencyCandidate&)>>
                parent_candidates([](auto& candidate1, auto& candidate2) {
                    return DependencyCandidate::MinErrorComparator(candidate1, candidate2);
                });
        auto parent_vertical = min_dep_candidate.vertical_;
        util::ForEachIndex(parent_vertical, [&](model::Index i) {
            parent_vertical.reset(i);
            if (IsKnownNonDependency(parent_vertical, local_visitees_.get()) ||
                IsKnownNonDependency(parent_vertical, global_visitees)) {
                parent_vertical.set(i);
                return;
            }
            if (alleged_non_deps.count(parent_vertical) != 0) {
                are_all_parents_known_non_deps = false;
                parent_vertical.set(i);
                return;
            }
            // TODO: construction methods should return unique_ptr<...>
            LOG_TRACE("CreateDependencyCandidate while trickling down from");
            parent_candidates.push(strategy->CreateDependencyCandidate(parent_vertical));
            parent_vertical.set(i);
        });

        while (!parent_candidates.empty()) {
            auto parent_candidate = parent_candidates.top();
            parent_candidates.pop();

            if (parent_candidate.error_.GetMin() > strategy->min_non_dependency_error_) {
                do {
                    if (parent_candidate.IsExact()) {
                        local_visitees_->Put(
                                parent_candidate.vertical_,
                                std::make_unique<VerticalInfo>(VerticalInfo::ForNonDependency()));
                    } else {
                        alleged_non_deps.insert(parent_candidate.vertical_);
                        are_all_parents_known_non_deps = false;
                    }
                    if (!parent_candidates.empty()) {
                        parent_candidate = parent_candidates.top();
                        parent_candidates.pop();
                    }
                } while (!parent_candidates.empty());
                break;
            }

            auto alleged_min_dep =
                    TrickleDownFrom(std::move(parent_candidate), strategy, alleged_min_deps,
                                    alleged_non_deps, global_visitees, boost_factor);

            if (alleged_min_dep.has_value()) {
                return alleged_min_dep;
            }

            if (!min_dep_candidate.IsExact()) {
                double error = strategy->CalculateError(min_dep_candidate.vertical_);
                // TODO: careful with reference shenanigans - looks like it works this way in the
                // original
                min_dep_candidate = DependencyCandidate(min_dep_candidate.vertical_,
                                                        model::ConfidenceInterval(error), true);
                if (error > strategy->min_non_dependency_error_) break;
            }
        }
    }

    double candidate_error = min_dep_candidate.IsExact()
                                     ? min_dep_candidate.error_.Get()
                                     : strategy->CalculateError(min_dep_candidate.vertical_);
    [[maybe_unused]] double error_diff = candidate_error - min_dep_candidate.error_.GetMean();
    if (candidate_error <= strategy->max_dependency_error_) {
        alleged_min_deps->RemoveSupersetEntries(min_dep_candidate.vertical_);
        alleged_min_deps->Put(min_dep_candidate.vertical_,
                              std::make_unique<VerticalInfo>(true, are_all_parents_known_non_deps,
                                                             candidate_error));
        if (are_all_parents_known_non_deps && context_->GetParameters().is_check_estimates) {
            RequireMinimalDependency(strategy, min_dep_candidate.vertical_);
        }
        return min_dep_candidate.vertical_;
    } else {
        LOG_TRACE("* Guessed incorrect {}-ary minimum dependency candidate.",
                  min_dep_candidate.vertical_.count());
        local_visitees_->Put(min_dep_candidate.vertical_,
                             std::make_unique<VerticalInfo>(VerticalInfo::ForNonDependency()));

        if (strategy->ShouldResample(min_dep_candidate.vertical_, boost_factor)) {
            context_->CreateFocusedSample(min_dep_candidate.vertical_, boost_factor);
        }
        return std::optional<boost::dynamic_bitset<>>();
    }
}

// TODO: critical part - consider optimization
// TODO: list -> vector as list doesn't have RAIterators therefore can't be sorted
std::unordered_set<boost::dynamic_bitset<>> SearchSpace::CalculateHittingSet(
        std::vector<boost::dynamic_bitset<>> verticals, auto pruning_function) const {
    RelationalSchema const* schema = context_->GetSchema();
    auto arity_comparer = [](boost::dynamic_bitset<> const& vertical1,
                             boost::dynamic_bitset<> const& vertical2) {
        return vertical1.count() < vertical2.count();
    };

    std::ranges::sort(verticals, [](boost::dynamic_bitset<> const& vertical1,
                                    boost::dynamic_bitset<> const& vertical2) {
        return vertical1.count() < vertical2.count();
    });
    model::VerticalMap<std::monostate> consolidated_verticals(schema);
    model::VerticalMap<std::monostate> hitting_set(schema);
    // TODO: VerticalMap requires `shared_ptr`s, so using this hack with a dummy pointer here.
    auto dummy_ptr = std::make_shared<std::monostate>();

    hitting_set.Put(boost::dynamic_bitset<>(schema->GetNumColumns()), dummy_ptr);

    for (boost::dynamic_bitset<> const& vertical : verticals) {
        if (consolidated_verticals.GetAnySubsetEntry(vertical).second != nullptr) {
            continue;
        }
        consolidated_verticals.Put(vertical, dummy_ptr);

        std::vector<boost::dynamic_bitset<>> invalid_hitting_set_members =
                hitting_set.GetSubsetKeys(~vertical);
        std::ranges::sort(invalid_hitting_set_members, arity_comparer);

        for (auto& invalid_hitting_set_member : invalid_hitting_set_members) {
            hitting_set.Remove(invalid_hitting_set_member);
        }

        for (boost::dynamic_bitset<> const& invalid_member : invalid_hitting_set_members) {
            for (size_t corrective_column_index = vertical.find_first();
                 corrective_column_index != boost::dynamic_bitset<>::npos;
                 corrective_column_index = vertical.find_next(corrective_column_index)) {
                boost::dynamic_bitset<> corrected_member =
                        boost::dynamic_bitset<>(invalid_member).set(corrective_column_index);

                if (hitting_set.GetAnySubsetEntry(corrected_member).second == nullptr) {
                    bool is_pruned = pruning_function(corrected_member);
                    if (is_pruned) continue;

                    hitting_set.Put(corrected_member, dummy_ptr);
                }
            }
        }
        if (hitting_set.IsEmpty()) break;
    }
    return hitting_set.KeySet();
}

std::unordered_set<boost::dynamic_bitset<>> SearchSpace::CalculateHittingSet(
        std::vector<boost::dynamic_bitset<>> verticals) const {
    return CalculateHittingSet(std::move(verticals), [](auto&&...) { return false; });
}

void SearchSpace::RequireMinimalDependency(DependencyStrategy* strategy,
                                           boost::dynamic_bitset<> const& min_dependency) {
    double error = strategy->CalculateError(min_dependency);
    if (error > strategy->max_dependency_error_) {
        throw std::runtime_error("Wrong minimal dependency estimate");
    }
    if (min_dependency.count() > 1) {
        boost::dynamic_bitset<> parent = min_dependency;
        util::ForEachIndex(min_dependency, [&](model::Index i) {
            parent.reset(i);
            double parent_error = strategy->CalculateError(parent);
            parent.set(i);
            if (parent_error <= strategy->min_non_dependency_error_) {
                throw std::runtime_error("Wrong minimal dependency estimate");
            }
        });
    }
}

std::vector<boost::dynamic_bitset<>> SearchSpace::GetSubsetDeps(
        boost::dynamic_bitset<> const& vertical, model::VerticalMap<VerticalInfo>* vertical_infos) {
    auto subset_entries = vertical_infos->GetSubsetEntries(vertical);
    auto subset_entries_end =
            std::remove_if(subset_entries.begin(), subset_entries.end(),
                           [](auto& entry) { return !entry.second->is_dependency_; });
    std::vector<boost::dynamic_bitset<>> subset_deps;

    std::transform(subset_entries.begin(), subset_entries_end,
                   std::inserter(subset_deps, subset_deps.begin()),
                   [&](auto& entry) { return entry.first; });

    return subset_deps;
}

bool SearchSpace::IsImpliedByMinDep(boost::dynamic_bitset<> const& vertical,
                                    model::VerticalMap<VerticalInfo>* vertical_infos) {
    return vertical_infos
                   ->GetAnySubsetEntry(vertical,
                                       []([[maybe_unused]] auto vertical, auto info) {
                                           return info->is_dependency_ && info->is_extremal_;
                                       })
                   .second != nullptr;
}

bool SearchSpace::IsKnownNonDependency(boost::dynamic_bitset<> const& vertical,
                                       model::VerticalMap<VerticalInfo>* vertical_infos) {
    return vertical_infos
                   ->GetAnySupersetEntry(vertical, []([[maybe_unused]] auto vertical,
                                                      auto info) { return !info->is_dependency_; })
                   .second != nullptr;
}

void SearchSpace::PrintStats() const {
    LOG_INFO("Num nested: {}", num_nested_ / 1000000);
}

void SearchSpace::EnsureInitialized() {
    strategy_->EnsureInitialized(this);
}
