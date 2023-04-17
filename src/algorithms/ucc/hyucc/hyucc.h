#pragma once

#include <memory>

#include "model/column_layout_relation_data.h"
#include "model/idataset_stream.h"
#include "ucc/ucc_algorithm.h"

namespace algos {

class HyUCC : public UCCAlgorithm {
private:
    std::unique_ptr<ColumnLayoutRelationData> relation_;

    void LoadDataInternal(model::IDatasetStream& data_stream) override;
    unsigned long long ExecuteInternal() override;
    void ResetUCCAlgorithmState() override {}
    void RegisterUCCs(std::vector<boost::dynamic_bitset<>>&& uccs,
                      const std::vector<size_t>& og_mapping);

public:
    HyUCC() : UCCAlgorithm({}) {}
};

}  // namespace algos
