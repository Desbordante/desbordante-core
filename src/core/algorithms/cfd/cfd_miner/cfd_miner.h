#pragma once

#include "core/algorithms/cfd/cfd_discovery.h"

namespace algos::cfd {

class CFDMiner final : public CFDDiscovery {
private:
    unsigned min_support_;
    unsigned max_lhs_;

    void RegisterOptions();
    void MakeExecuteOptsAvailable() final;
    void ExecuteInternal() final;
    void ResetStateCFD() final;
    void RegisterCfd(Itemset const& lhs, Item rhs);

public:
    CFDMiner();
};

}  // namespace algos::cfd
