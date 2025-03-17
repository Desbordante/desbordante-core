#pragma once

#include <memory>  // for shared_ptr
#include <vector>  // for vector

#include "algorithms/ucc/hpivalid/pli_table.h"  // for PLITable
#include "algorithms/ucc/ucc_algorithm.h"       // for UCCAlgorithm

class ColumnLayoutRelationData;

namespace algos {
namespace hpiv {
class ResultCollector;
}
}  // namespace algos

// see algorithms/ucc/hpivalid/LICENSE

namespace algos {

class HPIValid : public UCCAlgorithm {
private:
    std::shared_ptr<ColumnLayoutRelationData> relation_;

    void LoadDataInternal() override;
    unsigned long long ExecuteInternal() override;
    hpiv::PLITable Preprocess(hpiv::ResultCollector& rc);
    void RegisterUCCs(hpiv::ResultCollector const& rc);
    void PrintInfo(hpiv::ResultCollector const& rc) const;

    void ResetUCCAlgorithmState() override {}

public:
    HPIValid() : UCCAlgorithm({}) {}
};

}  // namespace algos
