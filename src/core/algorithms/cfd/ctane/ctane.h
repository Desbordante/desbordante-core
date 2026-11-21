#pragma once

#include "core/algorithms/cfd/cfd_discovery.h"
#include "core/algorithms/cfd/ctane/c_lattice_level.h"
#include "core/algorithms/cfd/ctane/c_lattice_vertex.h"
#include "core/algorithms/cfd/model/partition_tidlist.h"

namespace algos::cfd {

class CTaneAlgorithm : public algos::cfd::CFDDiscovery {
private:
    unsigned min_supp_;
    unsigned max_lhs_;
    double min_conf_;

    void ResetStateCFD() final;
    void CheckForIncorrectInput() const;
    static bool IsExactCfd(CLatticeVertex const& x_vertex, CLatticeVertex const& xa_vertex);
    static double CalculateConstConfidence(CLatticeVertex const& x_vertex,
                                           CLatticeVertex const& xa_vertex);
    static double CalculateConfidence(CLatticeVertex const& x_vertex,
                                      CLatticeVertex const& xa_vertex);
    void RegisterCfd(TuplePattern const& lhs_pattern, Item rhs_pattern, unsigned support,
                     double confidence);
    void PruneCandidates(CLatticeLevel* level, CLatticeVertex const* x_vertex,
                         CLatticeVertex const* xa_vertex, Item rhs_column_pattern) const;
    void Prune(CLatticeLevel* level) const;

protected:
    void RegisterOptions();
    void MakeExecuteOptsAvailable() override;
    void ExecuteInternal() final;

public:
    explicit CTaneAlgorithm();
};
}  // namespace algos::cfd
