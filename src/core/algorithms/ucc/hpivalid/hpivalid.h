#pragma once

#include <memory>

#include "core/algorithms/ucc/hpivalid/config.h"
#include "core/algorithms/ucc/hpivalid/pli_table.h"
#include "core/algorithms/ucc/hpivalid/result_collector.h"
#include "core/algorithms/ucc/ucc_algorithm.h"

// see algorithms/ucc/hpivalid/LICENSE

namespace algos {

class HPIValid : public UCCAlgorithm {
private:
    std::shared_ptr<RelationalSchema const> schema_;
    hpiv::PLITable tab_;

    void LoadDataInternal() override;
    unsigned long long ExecuteInternal() override;
    void RegisterUCCs(hpiv::ResultCollector const& rc);
    void PrintInfo(hpiv::ResultCollector const& rc) const;

    void ResetUCCAlgorithmState() override {}
};

}  // namespace algos
