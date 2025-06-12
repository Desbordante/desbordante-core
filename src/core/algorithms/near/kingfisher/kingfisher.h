#pragma once

#include <vector>

#include "algorithms/near/near_discovery.h"
#include "node_adress.h"

namespace algos {

class Kingfisher : public NeARDiscovery {
private:
    double max_p_;
    unsigned max_rules_;

    void RegisterOptions();
    void ResetState() override;
    void MakeExecuteOptsAvailable();
    unsigned long long ExecuteInternal() override;

public:
    Kingfisher();
    // For debug and testing, returned possible features are in default order
    std::vector<std::string> GetTreeHistory();
};

}  // namespace algos
