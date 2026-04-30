#include "core/algorithms/fd/fd_mine/fd_mine.h"

#include <queue>
#include <vector>

#include <boost/unordered_map.hpp>

#include "core/algorithms/fd/make_plain_table_mask_pair_adder.h"
#include "core/config/max_lhs/option.h"
#include "core/util/logger.h"

namespace algos::fd {

using boost::dynamic_bitset;

FdMine::FdMine() {
    RegisterOption(config::kMaxLhsOpt(&max_lhs_));
}

void FdMine::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::kMaxLhsOpt.GetName()});
}

void FdMine::ResetState() {
    fd_view_ = nullptr;
    candidate_set_.clear();
    eq_set_.clear();
    fd_set_.clear();
    final_fd_set_.clear();
    key_set_.clear();
    closure_.clear();
    plis_.clear();
}

unsigned long long FdMine::ExecuteInternal() {
    // 1
    auto start_time = std::chrono::system_clock::now();

    std::size_t const attr_num = table_header_.column_names.size();
    relation_indices_ = dynamic_bitset<>(table_header_.column_names.size());

    for (size_t column_index = 0; column_index < attr_num; column_index++) {
        dynamic_bitset<> tmp(attr_num);
        tmp[column_index] = 1;
        relation_indices_[column_index] = 1;
        candidate_set_.insert(std::move(tmp));
    }

    for (auto const& candidate : candidate_set_) {
        closure_[candidate] = dynamic_bitset<>(attr_num);
    }

    // 2
    while (!candidate_set_.empty()) {
        for (auto const& candidate : candidate_set_) {
            ComputeNonTrivialClosure(candidate);
            ObtainFDandKey(candidate);
        }
        ObtainEqSet();
        PruneCandidates();
        GenerateNextLevelCandidates();
    }

    // 3
    Reconstruct();
    Display();

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

void FdMine::ComputeNonTrivialClosure(dynamic_bitset<> const& xi) {
    if (!closure_.count(xi)) {
        closure_[xi] = dynamic_bitset<>(xi.size());
    }
    std::size_t const attr_num = table_header_.column_names.size();
    for (std::size_t column_index = 0; column_index < attr_num; column_index++) {
        if ((relation_indices_ - xi - closure_[xi])[column_index]) {
            dynamic_bitset<> candidate_xy = xi;
            dynamic_bitset<> candidate_y(attr_num);
            candidate_xy[column_index] = 1;
            candidate_y[column_index] = 1;

            if (xi.count() == 1) {
                model::PositionListIndex const& candidate_x_pli =
                        stripped_partitions_[xi.find_first()];
                model::PositionListIndex const& candidate_y_pli =
                        stripped_partitions_[column_index];

                plis_[candidate_xy] = candidate_x_pli.Intersect(&candidate_y_pli);

                if (candidate_x_pli.GetNumCluster() == plis_[candidate_xy]->GetNumCluster()) {
                    closure_[xi][column_index] = 1;
                }

                continue;
            }

            if (!plis_.count(candidate_xy)) {
                model::PositionListIndex const& candidate_y_pli =
                        stripped_partitions_[candidate_y.find_first()];
                plis_[candidate_xy] = plis_[xi]->Intersect(&candidate_y_pli);
            }

            if (plis_[xi]->GetNumCluster() == plis_[candidate_xy]->GetNumCluster()) {
                closure_[xi][column_index] = 1;
            }
        }
    }
}

void FdMine::ObtainFDandKey(dynamic_bitset<> const& xi) {
    fd_set_[xi] = closure_[xi];
    if (relation_indices_ == (xi | closure_[xi])) {
        key_set_.insert(xi);
    }
}

void FdMine::ObtainEqSet() {
    for (auto const& candidate : candidate_set_) {
        for (auto& [lhs, closure] : fd_set_) {
            auto common_atrs = candidate & lhs;
            if ((candidate - common_atrs).is_subset_of(closure) &&
                (lhs - common_atrs).is_subset_of(closure_[candidate])) {
                if (lhs != candidate) {
                    eq_set_[lhs].insert(candidate);
                    eq_set_[candidate].insert(lhs);
                }
            }
        }
    }
}

void FdMine::PruneCandidates() {
    auto it = candidate_set_.begin();
    while (it != candidate_set_.end()) {
        bool found = false;
        auto const& xi = *it;

        for (auto const& xj : eq_set_[xi]) {
            if (candidate_set_.find(xj) != candidate_set_.end()) {
                it = candidate_set_.erase(it);
                found = true;
                break;
            }
        }

        if (found) continue;

        if (key_set_.find(xi) != key_set_.end()) {
            it = candidate_set_.erase(it);
            continue;
        }
        it++;
    }
}

void FdMine::GenerateNextLevelCandidates() {
    std::vector<dynamic_bitset<>> candidates(candidate_set_.begin(), candidate_set_.end());

    dynamic_bitset<> candidate_i;
    dynamic_bitset<> candidate_j;
    dynamic_bitset<> candidate_ij;

    for (size_t i = 0; i < candidates.size(); i++) {
        candidate_i = candidates[i];

        for (size_t j = i + 1; j < candidates.size(); j++) {
            candidate_j = candidates[j];

            // apriori-gen
            bool similar = true;
            size_t set_bits = 0;

            assert(candidate_i.count() != 0);
            for (size_t k = 0; set_bits < candidate_i.count() - 1; k++) {
                if (candidate_i[k] == candidate_j[k]) {
                    if (candidate_i[k]) {
                        set_bits++;
                    }
                } else {
                    similar = false;
                    break;
                }
            }
            //

            if (similar) {
                candidate_ij = candidate_i | candidate_j;

                if (!(candidate_j).is_subset_of(fd_set_[candidate_i]) &&
                    !(candidate_i).is_subset_of(fd_set_[candidate_j])) {
                    if (candidate_i.count() == 1) {
                        model::PositionListIndex const& candidate_i_pli =
                                stripped_partitions_[candidate_i.find_first()];
                        model::PositionListIndex const& candidate_j_pli =
                                stripped_partitions_[candidate_j.find_first()];
                        plis_[candidate_ij] = candidate_i_pli.Intersect(&candidate_j_pli);
                    } else {
                        plis_[candidate_ij] =
                                plis_[candidate_i]->Intersect(plis_[candidate_j].get());
                    }

                    auto closure_ij = closure_[candidate_i] | closure_[candidate_j];
                    if (relation_indices_ == (candidate_ij | closure_ij)) {
                        key_set_.insert(candidate_ij);
                    } else {
                        candidate_set_.insert(candidate_ij);
                    }
                }
            }
        }

        candidate_set_.erase(candidate_i);
    }
}

void FdMine::Reconstruct() {
    std::queue<dynamic_bitset<>> queue;
    dynamic_bitset<> generated_lhs(relation_indices_.size());
    dynamic_bitset<> generated_lhs_tmp(relation_indices_.size());

    for (auto const& [lhs, rhs] : fd_set_) {
        std::unordered_set<dynamic_bitset<>> observed;

        observed.insert(lhs);
        auto rhs_copy = rhs;
        queue.push(lhs);

        for (auto const& [eq, eqset] : eq_set_) {
            if (eq.is_subset_of(rhs_copy)) {
                for (auto const& eq_rhs : eqset) {
                    rhs_copy |= eq_rhs;
                }
            }
        }
        bool rhs_may_change = true;

        while (!queue.empty()) {
            dynamic_bitset<> current_lhs = std::move(queue.front());
            queue.pop();
            size_t rhs_count = rhs_copy.count();
            for (auto const& [eq, eqset] : eq_set_) {
                if (rhs_may_change && eq.is_subset_of(rhs_copy)) {
                    for (auto const& eq_rhs : eqset) {
                        rhs_copy |= eq_rhs;
                    }
                }

                if (eq.is_subset_of(current_lhs)) {
                    generated_lhs_tmp = current_lhs - eq;
                    for (auto const& new_eq : eqset) {
                        generated_lhs = generated_lhs_tmp;
                        generated_lhs |= new_eq;

                        auto [it, is_new] = observed.emplace(generated_lhs);

                        if (is_new) {
                            queue.push(generated_lhs);
                        }
                    }
                }
            }
            if (rhs_count == rhs_copy.count()) {
                rhs_may_change = false;
            }
        }

        for (auto const& lhs : observed) {
            // TODO: investigate how this check can be pushed into an earlier stage.
            if (lhs.count() > max_lhs_) continue;
            auto [it, is_new] = final_fd_set_.try_emplace(lhs, rhs_copy);
            if (!is_new) {
                it->second |= rhs_copy;
            }
        }
    }
}

void FdMine::Display() {
    TableMaskPairFdView::Storage storage;
    auto report_fd = MakePlainTableMaskPairAdder(storage);

    for (auto it = final_fd_set_.begin(); it != final_fd_set_.end();) {
        auto node = final_fd_set_.extract(it++);
        boost::dynamic_bitset<> lhs = std::move(node.key());
        boost::dynamic_bitset<> rhs = std::move(node.mapped());
        rhs -= lhs;
        if (rhs.none()) continue;
        report_fd(std::move(lhs), std::move(rhs));
    }

    fd_view_ = std::make_shared<TableMaskPairFdView>(table_header_, std::move(storage));
}

}  // namespace algos::fd
