#include "core/algorithms/fd/fastfds/fastfds.h"

#include <algorithm>
#include <mutex>
#include <thread>

#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/fd/bitset_result_reporter.h"
#include "core/algorithms/fd/make_plain_lhs_mask_adder.h"
#include "core/config/max_lhs/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/config/thread_number/option.h"
#include "core/model/table/compute_agree_sets.h"
#include "core/util/bitset_utils.h"
#include "core/util/logger.h"
#include "core/util/parallel_for.h"

namespace algos::fd {

using std::vector, std::set;

FastFDs::FastFDs() {
    RegisterOptions();
}

void FastFDs::RegisterOptions() {
    RegisterOption(config::kMaxLhsOpt(&max_lhs_));
    RegisterOption(config::kThreadNumberOpt(&threads_num_));
}

void FastFDs::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::kMaxLhsOpt.GetName(), config::kThreadNumberOpt.GetName()});
}

void FastFDs::ResetState() {
    fd_view_ = nullptr;
    diff_sets_.clear();
}

void FastFDs::ExecuteInternal() {
    GenDiffSets();

    LhsMaskFdView::Storage lhs_masks(input_table_column_plis_.size());

    if (diff_sets_.size() == 1 && diff_sets_.back().none()) {
        fd_view_ = std::make_shared<LhsMaskFdView>(table_header_, std::move(lhs_masks));
        return;
    }

    boost::dynamic_bitset<> empty_attribute_mask(input_table_column_plis_.size());

    auto task = [this, &empty_attribute_mask, &lhs_masks](model::Index column) {
        auto report_fd_lhs = MakePlainLhsMaskAdder(lhs_masks[column]);
        if (input_table_column_plis_[column].IsConstant()) {
            report_fd_lhs(empty_attribute_mask);
            return;
        }

        vector<DiffSet> diff_sets_mod = GetDiffSetsMod(column);
        assert(!diff_sets_mod.empty());
        if (!(diff_sets_mod.size() == 1 && diff_sets_mod.back().none())) {
            set<model::Index, OrderingComparator> init_ordering =
                    GetInitOrdering(diff_sets_mod, column);
            FindCovers(diff_sets_mod, diff_sets_mod, empty_attribute_mask, init_ordering,
                       report_fd_lhs);
        }
    };

    if (threads_num_ > 1) {
        boost::asio::thread_pool pool(threads_num_);

        for (model::Index column = 0; column != input_table_column_plis_.size(); ++column) {
            boost::asio::post(pool, [column, task]() { return task(column); });
        }

        pool.join();
    } else {
        for (model::Index column = 0; column != input_table_column_plis_.size(); ++column) {
            task(column);
        }
    }

    fd_view_ = std::make_shared<LhsMaskFdView>(table_header_, std::move(lhs_masks));
}

void FastFDs::FindCovers(vector<DiffSet> const& diff_sets_mod, vector<DiffSet> const& cur_diff_sets,
                         boost::dynamic_bitset<> const& path,
                         set<model::Index, OrderingComparator> const& ordering,
                         BitsetResultReporter const& report_fd_lhs) {
    if (path.count() > max_lhs_) {
        return;
    }

    if (ordering.size() == 0 && !cur_diff_sets.empty()) {
        return;  // no FDs here
    }

    if (cur_diff_sets.empty()) {
        if (CoverMinimal(path, diff_sets_mod)) {
            report_fd_lhs(path);
            return;
        }
        return;  // wasted effort, non-minimal result
    }

    auto path_next = path;
    for (model::Index column : ordering) {
        vector<DiffSet> next_diff_sets;
        for (DiffSet const& diff_set : cur_diff_sets) {
            if (!diff_set.test(column)) {
                next_diff_sets.push_back(diff_set);
            }
        }

        auto next_ordering = GetNextOrdering(next_diff_sets, column, ordering);
        FindCovers(diff_sets_mod, next_diff_sets, boost::dynamic_bitset<>(path).set(column),
                   next_ordering, report_fd_lhs);
    }
}

bool FastFDs::IsCover(boost::dynamic_bitset<> const& candidate,
                      vector<boost::dynamic_bitset<>> const& sets) const {
    bool covers = true;

    for (boost::dynamic_bitset<> const& set : sets) {
        if (!set.intersects(candidate)) {
            covers = false;
            break;
        }
    }

    return covers;
}

bool FastFDs::CoverMinimal(boost::dynamic_bitset<> const& cover,
                           vector<DiffSet> const& diff_sets_mod) const {
    for (auto column = cover.find_first(); column != boost::dynamic_bitset<>::npos;
         column = cover.find_next(column)) {
        boost::dynamic_bitset<> subset = boost::dynamic_bitset<>(cover).reset(column);
        bool subset_covers = IsCover(subset, diff_sets_mod);
        if (subset_covers) {
            return false;  // cover is not minimal
        }
    }
    return true;  // cover is minimal
}

bool FastFDs::OrderingComp(vector<DiffSet> const& diff_sets, model::Index l_col,
                           model::Index r_col) const {
    unsigned cov_l = 0;
    unsigned cov_r = 0;

    for (DiffSet const& diff_set : diff_sets) {
        if (diff_set.test(l_col)) {
            ++cov_l;
        }
        if (diff_set.test(r_col)) {
            ++cov_r;
        }
    }

    if (cov_l != cov_r) {
        return cov_l > cov_r;
    }

    return l_col > r_col;
}

set<model::Index, FastFDs::OrderingComparator> FastFDs::GetInitOrdering(
        vector<DiffSet> const& diff_sets, model::Index attribute) const {
    auto ordering_comp = [&diff_sets, this](model::Index l_col, model::Index r_col) {
        return OrderingComp(diff_sets, l_col, r_col);
    };
    set<model::Index, OrderingComparator> ordering(ordering_comp);

    for (model::Index col = 0; col != input_table_column_plis_.size(); ++col) {
        if (col != attribute) {
            ordering.insert(col);
        }
    }

    return ordering;
}

set<model::Index, FastFDs::OrderingComparator> FastFDs::GetNextOrdering(
        vector<DiffSet> const& diff_sets, model::Index attribute,
        set<model::Index, OrderingComparator> const& cur_ordering) const {
    auto ordering_comp = [&diff_sets, this](model::Index l_col, model::Index r_col) {
        return OrderingComp(diff_sets, l_col, r_col);
    };
    set<model::Index, OrderingComparator> ordering(ordering_comp);
    // columns that are contained in at least one diff set
    std::unordered_set<model::Index> diff_sets_cols;

    for (DiffSet const& diff_set : diff_sets) {
        util::ForEachIndex(diff_set, [&](model::Index col) { diff_sets_cols.insert(col); });
    }

    auto p = cur_ordering.find(attribute);
    assert(p != cur_ordering.end());
    for (++p; p != cur_ordering.end(); ++p) {
        if (diff_sets_cols.find(*p) != diff_sets_cols.end()) {
            ordering.insert(*p);
        }
    }
    return ordering;
}

/* Metanome uses thread pool here. No need for it because main loop over columns in
 * execute() is parallelized, this approach should be much better.
 */
vector<FastFDs::DiffSet> FastFDs::GetDiffSetsMod(model::Index col) const {
    vector<DiffSet> diff_sets_mod;

    /* diff_sets_ is sorted, before adding next diff_set to
     * diff_sets_mod need to check if diff_sets_mod contains
     * a subset of diff_set, that means that diff_set
     * is not minimal.
     */
    for (DiffSet const& diff_set : diff_sets_) {
        if (diff_set.test(col)) {
            bool is_minimal = true;

            for (DiffSet const& min_diff_set : diff_sets_mod) {
                if (min_diff_set.is_subset_of(diff_set)) {
                    is_minimal = false;
                    break;
                }
            }

            if (is_minimal) {
                diff_sets_mod.push_back(std::move(boost::dynamic_bitset<>(diff_set).reset(col)));
            }
        }
    }

    return diff_sets_mod;
}

void FastFDs::GenDiffSets() {
    model::AttributeMaskSet agree_sets = model::ComputeAgreeSets(input_table_column_plis_);

    // Complement agree sets to get difference sets
    diff_sets_.reserve(agree_sets.size());
    if (threads_num_ > 1) {
        std::mutex m;
        auto const task = [&m, this](model::AttributeMask const& as) {
            DiffSet diff_set = ~as;
            std::lock_guard lock(m);
            diff_sets_.push_back(std::move(diff_set));
        };

        util::ParallelForeach(agree_sets.begin(), agree_sets.end(), threads_num_, task);
    } else {
        for (model::AttributeMask const& agree_set : agree_sets) {
            diff_sets_.push_back(~agree_set);
        }
    }

    // sort diff_sets_, it will be used further to find minimal difference sets modulo column
    std::sort(diff_sets_.begin(), diff_sets_.end());
}

}  // namespace algos::fd
