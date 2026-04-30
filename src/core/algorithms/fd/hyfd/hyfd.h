#pragma once

#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/fd/hycommon/types.h"
#include "core/algorithms/fd/raw_fd.h"
#include "core/algorithms/fd/table_mask_pair_fd_view.h"
#include "core/config/max_lhs/type.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/config/thread_number/type.h"
#include "core/model/table/table_header.h"

// (DELETE WHEN PUTTING UP FOR REVIEW)
// In PR message: Moved FD algorithms to FD namespace because the precise names of FD stuff (like
// the reporting function types) got too long, the ::fd context allows to use shorter names.
namespace algos::fd::hyfd {

/**
 * HyFD is a hybrid functional dependency discovery algorithm, employing both row-efficient
 * sample-based agree set generation and column-efficient lattice traversal.
 *
 * The algorithm has three major components: Sampler, Inductor and Validator --- the former two
 * representing row-efficient step. Sampler acquires a collection of agree sets using a special
 * deterministic sampling approach, then Inductor uses this data to enrich an FD prefix tree. This
 * concludes the first step and the execution moves to Validator, which performs breadth-first
 * bottom-up traverse of the FDTree, validating the FDs and specializing incorrect candidates.
 *
 * It is difficult to come up with a criteria or an algorithm for choosing the optimal point to move
 * from the first step to the second, therefore HyFD uses a notion of efficiency to track how
 * efficient current step is performing and switches to the other step in case it falls beneath a
 * configured threshold. Each step's results are useful for the other step: Sampler-Inductor
 * specialize the prefix tree, which reduces the number or costly PLI checks Validator has to
 * perform, while Validator yields a collection of row pairs, which invalidated a candidate and can
 * be used by Sampler to obtain a new agree set.
 *
 * Thorsten Papenbrock and Felix Naumann. 2016. A Hybrid Approach to Functional Dependency
 * Discovery. In Proceedings of the 2016 International Conference on Management of Data (SIGMOD
 * '16). Association for Computing Machinery, New York, NY, USA, 821–833.
 * https://doi.org/10.1145/2882903.2915203
 */
class HyFD : public Algorithm {
private:
    config::InputTable input_table_;
    config::MaxLhsType max_lhs_;

    model::TableHeader table_header_;
    TableMaskPairFdView::OwningPointer fd_view_;

    std::shared_ptr<hy::PLIs> plis_;
    std::shared_ptr<hy::Rows> pli_records_;
    std::vector<hy::ClusterId> og_mapping_;

    void LoadDataInternal() final;

    void ResetState() final;

    unsigned long long ExecuteInternal() final;

    void MakeExecuteOptsAvailable() final;

    config::ThreadNumType threads_num_ = 1;

public:
    HyFD();

    TableMaskPairFdView::OwningPointer GetFds() {
        return fd_view_;
    }
};

}  // namespace algos::fd::hyfd
