#include "core/algorithms/cfd/ctane/ctane.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "core/config/equal_nulls/option.h"
#include "core/config/exceptions.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/util/logger.h"

namespace algos::cfd {

CTaneAlgorithm::CTaneAlgorithm() : CFDDiscovery() {
    RegisterOptions();
}

void CTaneAlgorithm::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(Option{&min_supp_, kCfdMinimumSupport, kDCfdMinimumSupport, 0u});
    RegisterOption(Option{&min_conf_, kCfdMinimumConfidence, kDCfdMinimumConfidence, 0.0});
    RegisterOption(Option{&max_lhs_, kCfdMaximumLhs, kDCfdMaximumLhs, 0u});
}

void CTaneAlgorithm::ResetStateCFD() {}

bool CTaneAlgorithm::IsExactCfd(CLatticeVertex const& x_vertex, CLatticeVertex const& xa_vertex) {
    auto const& x_pli = *x_vertex.GetPositionListIndex();
    auto const& xa_pli = *xa_vertex.GetPositionListIndex();
    return x_pli.sets_number == xa_pli.sets_number && x_pli.Support() == xa_pli.Support();
}

double CTaneAlgorithm::CalculateConstConfidence(CLatticeVertex const& x_vertex,
                                                CLatticeVertex const& xa_vertex) {
    auto const x_support = x_vertex.GetPositionListIndex()->Support();
    auto const xa_support = xa_vertex.GetPositionListIndex()->Support();
    return static_cast<double>(xa_support) / x_support;
}

double CTaneAlgorithm::CalculateConfidence(CLatticeVertex const& x_vertex,
                                           CLatticeVertex const& xa_vertex) {
    auto const& x_pli = *x_vertex.GetPositionListIndex();
    auto const& xa_pli = *xa_vertex.GetPositionListIndex();
    auto const error = x_pli.PartitionError(xa_pli);
    return 1 - static_cast<double>(error) / x_pli.Support();
}

void CTaneAlgorithm::RegisterCfd(TuplePattern const& lhs_pattern, Item rhs_pattern,
                                 unsigned support, double confidence) {
    auto const to_raw_item = [this](Item item) -> RawCFD::RawItem {
        auto const attribute = relation_->GetAttrIndex(item);
        return item < 0 ? RawCFD::RawItem{attribute, std::nullopt}
                        : RawCFD::RawItem{attribute, relation_->GetValue(item)};
    };
    RawCFD::RawItems lhs;
    lhs.reserve(lhs_pattern.Size());
    for (auto const& [column, item] : lhs_pattern.GetPatternValues()) {
        static_cast<void>(column);
        lhs.push_back(to_raw_item(item));
    }
    cfd_list_.emplace_back(std::move(lhs), to_raw_item(rhs_pattern));
    LOG_INFO("Discovered CFD: {} with support = {} and confidence = {}.",
             cfd_list_.back().ToString(), support, confidence);
}

void CTaneAlgorithm::Prune(CLatticeLevel* level) const {
    auto& level_vertices = level->GetVertices();
    auto pred = [&](std::unique_ptr<CLatticeVertex>& vertex) {
        return vertex->GetRhsCandidates().empty() ||
               (static_cast<unsigned>(vertex->GetPositionListIndex()->Support()) < min_supp_);
    };
    level_vertices.erase(std::remove_if(level_vertices.begin(), level_vertices.end(), pred),
                         level_vertices.end());
}

void CTaneAlgorithm::PruneCandidates(CLatticeLevel* level, CLatticeVertex const* x_vertex,
                                     CLatticeVertex const* xa_vertex,
                                     Item rhs_column_pattern) const {
    auto other_indices = ~boost::dynamic_bitset<>(relation_->GetSchema()->GetNumColumns()) -
                         x_vertex->GetColumnIndices();

    auto const rhs_col_index = relation_->GetAttrIndex(rhs_column_pattern);
    auto xa_vertex_without_col = xa_vertex->GetTuplePattern().GetWithoutColumn(rhs_col_index);

    for (auto const& vertex : level->GetVertices()) {
        auto const& tuple_pattern = vertex->GetTuplePattern();
        if (tuple_pattern.HasColumnPattern(rhs_col_index, rhs_column_pattern) &&
            tuple_pattern.GetWithoutColumn(rhs_col_index) <= xa_vertex_without_col) {
            auto& candidates = vertex->GetRhsCandidates();
            auto pred = [this, &other_indices, &rhs_column_pattern](int col_pattern) {
                return col_pattern == rhs_column_pattern ||
                       other_indices[relation_->GetAttrIndex(col_pattern)];
            };
            candidates.erase(std::remove_if(candidates.begin(), candidates.end(), pred),
                             vertex->GetRhsCandidates().end());
        }
    }
}

void CTaneAlgorithm::ExecuteInternal() {
    CheckForIncorrectInput();

    auto max_lvl = std::min(max_lhs_, (uint)relation_->GetAttrsNumber() - 1) + 1;
    std::unique_ptr<CLatticeVertex> empty_vertex;
    std::vector<std::unique_ptr<CLatticeLevel>> lattice;

    for (unsigned int arity = 0; arity < max_lvl; arity++) {
        if (arity == 0) {
            empty_vertex = CLatticeLevel::GenerateFirstLevel(lattice, *relation_, min_supp_);
        } else {
            CLatticeLevel::GenerateNextLevel(lattice);
        }

        auto* level = lattice[arity].get();
        if (level->GetVertices().empty()) {
            break;
        }

        for (auto& xa : level->GetVertices()) {
            if (!xa->GetPositionListIndex()) {
                auto pli_1 = xa->GetParents()[0]->GetPositionListIndex();
                auto pli_2 = xa->GetParents()[1]->GetPositionListIndex();
                xa->SetPositionListIndex(pli_1->Intersection(*pli_2));
                if (static_cast<unsigned>(xa->GetPositionListIndex()->Support()) < min_supp_) {
                    continue;
                }
            }
            auto const& xa_pattern_values = xa->GetTuplePattern().GetPatternValues();
            for (auto const* x : xa->GetParents()) {
                auto const& x_pattern_values = x->GetTuplePattern().GetPatternValues();
                auto const a_it = std::ranges::find_if(xa_pattern_values, [&](auto const& entry) {
                    return !x_pattern_values.contains(entry.first);
                });
                if (a_it == xa_pattern_values.end()) {
                    continue;
                }

                auto const a = a_it->second;
                if (std::ranges::find(xa->GetRhsCandidates(), a) != xa->GetRhsCandidates().end()) {
                    double const conf = min_conf_ == 1 ? IsExactCfd(*x, *xa)
                                        : a > 0        ? CalculateConstConfidence(*x, *xa)
                                                       : CalculateConfidence(*x, *xa);
                    if (conf >= min_conf_) {
                        RegisterCfd(x->GetTuplePattern(), a,
                                    static_cast<unsigned>(xa->GetPositionListIndex()->Support()),
                                    conf);
                    }
                    if (conf == 1) {
                        PruneCandidates(level, x, xa.get(), a);
                    }
                }
            }
        }
        Prune(level);
    }

    LOG_INFO("> CFD COUNT: {}", cfd_list_.size());
}

void CTaneAlgorithm::CheckForIncorrectInput() const {
    if (min_supp_ < 1) {
        throw config::ConfigurationError("[ERROR] Illegal Support value: \"" +
                                         std::to_string(min_supp_) + "\"" + " is less than 1");
    }

    if (min_conf_ < 0 || min_conf_ > 1) {
        throw config::ConfigurationError("[ERROR] Illegal Confidence value: \"" +
                                         std::to_string(min_conf_) + "\"" + " not in [0,1]");
    }

    if (max_lhs_ < 1) {
        throw config::ConfigurationError("[ERROR] Illegal Max LHS value: \"" +
                                         std::to_string(max_lhs_) + "\"" + " is less than 1");
    }
}

void CTaneAlgorithm::MakeExecuteOptsAvailable() {
    using namespace config::names;

    MakeOptionsAvailable({kCfdMinimumSupport, kCfdMinimumConfidence, kCfdMaximumLhs});
}

}  // namespace algos::cfd
