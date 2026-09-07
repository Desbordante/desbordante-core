#pragma once

#include <functional>
#include <set>

#include "core/algorithms/fd/bitset_result_reporter.h"
#include "core/algorithms/fd/lhs_mask_fd_view.h"
#include "core/algorithms/fd/probing_tables_load_data.h"
#include "core/config/max_lhs/type.h"
#include "core/config/thread_number/type.h"
#include "core/model/index.h"
#include "core/model/table/table_header.h"

namespace algos::fd {

class FastFDs : public ProbingTablesLoadData {
public:
    FastFDs();

    LhsMaskFdView::OwningPointer GetFds() {
        return fd_view_;
    }

private:
    using OrderingComparator = std::function<bool(model::Index, model::Index)>;
    using DiffSet = boost::dynamic_bitset<>;

    void RegisterOptions();

    void MakeExecuteOptsAvailable() final;

    void ResetState() final;
    void ExecuteInternal() final;

    // Computes all difference sets of `relation_` by complementing agree sets
    void GenDiffSets();

    /* Computes minimal difference sets
     * of `relation_` modulo `col`
     */
    std::vector<DiffSet> GetDiffSetsMod(model::Index col) const;
    /* Returns initial ordering,
     * the total ordering of { schema_->GetColumns() \ `attribute` } according to `diff_sets`
     */
    std::set<model::Index, OrderingComparator> GetInitOrdering(
            std::vector<DiffSet> const& diff_sets, model::Index attribute) const;
    /* Returns next ordering,
     * the total ordering of { B in schema_->GetColumns() | B > `attribute` (in `cur_ordering`) }
     * according to `diff_sets`
     */
    std::set<model::Index, OrderingComparator> GetNextOrdering(
            std::vector<DiffSet> const& diff_sets, model::Index attribute,
            std::set<model::Index, OrderingComparator> const& cur_ordering) const;
    void FindCovers(std::vector<DiffSet> const& diff_sets_mod,
                    std::vector<DiffSet> const& cur_diff_sets, boost::dynamic_bitset<> const& path,
                    std::set<model::Index, OrderingComparator> const& ordering,
                    BitsetResultReporter const& report_fd_lhs);
    /* Returns true if `cover` is the minimal cover of `diff_sets_mod`,
     * false otherwise
     */
    bool CoverMinimal(boost::dynamic_bitset<> const& cover,
                      std::vector<DiffSet> const& diff_sets_mod) const;
    /* Returns true if `candidate` covers `sets`,
     * false otherwise
     */
    bool IsCover(boost::dynamic_bitset<> const& candidate,
                 std::vector<boost::dynamic_bitset<>> const& sets) const;
    /* Returns true if `l_col` > `r_col`,
     * false otherwise.
     * `l_col` > `r_col` iff
     * `l_col` covers more sets in `diff_sets` than `r_col` or
     * `l_col` and `r_col` cover the same number of sets but
     * `l_col` index less than `r_col` index
     */
    bool OrderingComp(std::vector<DiffSet> const& diff_sets, model::Index l_col,
                      model::Index r_col) const;

    config::MaxLhsType max_lhs_;
    config::ThreadNumType threads_num_;
    std::vector<DiffSet> diff_sets_;

    LhsMaskFdView::OwningPointer fd_view_;
};

}  // namespace algos::fd
