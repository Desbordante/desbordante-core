#pragma once

#include "algorithms/fd_algorithm.h"
#include "model/column_layout_relation_data.h"

namespace algos {

class PliBasedFDAlgorithm : public FDAlgorithm {
protected:
    std::shared_ptr<ColumnLayoutRelationData> relation_;

    void LoadDataInternal() final;

    ColumnLayoutRelationData const& GetRelation() const noexcept {
        // GetRelation should be called after the dataset has been parsed, i.e. after algorithm
        // execution
        assert(relation_ != nullptr);
        return *relation_;
    }

public:
    explicit PliBasedFDAlgorithm(std::vector<std::string_view> phase_names);

    std::vector<Column const*> GetKeys() const override;

    using Algorithm::LoadData;
    void LoadData(std::shared_ptr<ColumnLayoutRelationData> data);
};

}  // namespace algos
