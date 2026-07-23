#pragma once

#include <memory>

#include "core/algorithms/fd/hycommon/types.h"
#include "core/algorithms/ucc/ucc_algorithm.h"
#include "core/config/thread_number/option.h"
#include "core/config/thread_number/type.h"
#include "core/model/table/idataset_stream.h"
#include "core/model/table/relational_schema.h"

namespace algos {

class HyUCC : public UCCAlgorithm {
private:
    std::shared_ptr<RelationalSchema const> schema_;
    config::ThreadNumType threads_num_ = 1;

    hy::PLIsPtr plis_;
    hy::RowsPtr pli_records_;
    std::vector<hy::ClusterId> og_mapping_;

    void LoadDataInternal() override;
    void ExecuteInternal() override;

    void ResetUCCAlgorithmState() override {}

    void RegisterUCCs(std::vector<boost::dynamic_bitset<>>&& uccs,
                      std::vector<hy::ClusterId> const& og_mapping);

    void MakeExecuteOptsAvailable() final {
        MakeOptionsAvailable({config::kThreadNumberOpt.GetName()});
    }

public:
    HyUCC() : UCCAlgorithm() {
        RegisterOption(config::kThreadNumberOpt(&threads_num_));
    }
};

}  // namespace algos
