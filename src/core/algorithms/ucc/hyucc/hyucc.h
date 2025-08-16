#pragma once

#include <memory>  // for unique_ptr
#include <vector>  // for vector

#include <boost/dynamic_bitset/dynamic_bitset.hpp>  // for dynamic_bitset
#include <boost/type_index/type_index_facade.hpp>   // for operator==

#include "common_option.h"                            // for CommonOption
#include "config/thread_number/option.h"              // for kThreadNumberOpt
#include "config/thread_number/type.h"                // for ThreadNumType
#include "fd/hycommon/types.h"                        // for ClusterId
#include "model/table/column_layout_relation_data.h"  // for ColumnLayoutRel...
#include "ucc/ucc_algorithm.h"                        // for UCCAlgorithm

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
