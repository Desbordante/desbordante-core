#pragma once

#include <list>
#include <set>

#include "core/algorithms/fd/lhs_mask_fd_view.h"
#include "core/algorithms/fd/probing_tables_load_data.h"
#include "core/config/max_lhs/type.h"
#include "core/model/index.h"
#include "core/model/table/table_header.h"

namespace algos::fd {

class FUN : public ProbingTablesLoadData {
    // Entities from the algorithm itself
private:
    boost::dynamic_bitset<> r_;
    boost::dynamic_bitset<> r_prime_;

    class FunQuadruple;
    using Level = std::list<FunQuadruple>;

    void ResetState() final;
    void ExecuteInternal() final;

    Level GenerateCandidate(Level const& l_k) const;

    void ComputeClosure(Level& l_k_minus_1, Level const& l_k) const;

    unsigned long Count(boost::dynamic_bitset<> const& l) const;

    unsigned long FastCount(Level const& l_k_minus_1, Level const& l_k,
                            FunQuadruple const& l) const;

    void ComputeQuasiClosure(Level const& l_k_minus_1, Level& l_k) const;

    void PurePrune(Level const& l_k_minus_1, Level& l_k) const;

    void DisplayFD(Level const& l_k_minus_1);

    // Supporting entities
private:
    LhsMaskFdView::OwningPointer fd_view_;

    config::MaxLhsType max_lhs_;
    std::vector<std::deque<boost::dynamic_bitset<>>> fds_;

    void RegisterOptions();
    void MakeExecuteOptsAvailable() final;
    bool IsKey(FunQuadruple const& l) const;

public:
    FUN();

    LhsMaskFdView::OwningPointer GetFds() {
        return fd_view_;
    }
};

}  // namespace algos::fd
