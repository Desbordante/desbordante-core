#pragma once

#include <memory>

#include "config/thread_number/option.h"
#include "config/thread_number/type.h"
#include "fd/hycommon/types.h"
#include "model/table/column_layout_relation_data.h"
#include "model/table/idataset_stream.h"
#include "ucc/ucc_algorithm.h"

namespace algos {

class HyUCC : public UCCAlgorithm {
private:
    std::unique_ptr<ColumnLayoutRelationData> relation_;
    config::ThreadNumType threads_num_ = 1;

    void LoadDataInternal() override;
    unsigned long long ExecuteInternal() override;

    void ResetUCCAlgorithmState() override {}

    void RegisterUCCs(std::vector<boost::dynamic_bitset<>>&& uccs,
                      std::vector<hy::ClusterId> const& og_mapping);

    void MakeExecuteOptsAvailable() final {
        MakeOptionsAvailable({config::kThreadNumberOpt.GetName()});
    }

public:
    HyUCC() : UCCAlgorithm({}) {
        RegisterOption(config::kThreadNumberOpt(&threads_num_));
    }
};

}  // namespace algos
