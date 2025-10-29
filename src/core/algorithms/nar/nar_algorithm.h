#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "algorithms/algorithm.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/column_layout_typed_relation_data.h"
#include "nar.h"

namespace algos {

using model::NAR;
using TypedRelation = model::ColumnLayoutTypedRelationData;

class NARAlgorithm : public Algorithm {
private:
    config::InputTable input_table_;
    void RegisterOptions();

protected:
    std::vector<NAR> nar_collection_;
    std::unique_ptr<TypedRelation> typed_relation_;
    double minsup_;
    double minconf_;

    void LoadDataInternal() final;
    void MakeExecuteOptsAvailable() override;

public:
    std::vector<NAR> const& GetNARVector() const noexcept {
        return nar_collection_;
    };

    void ResetState() override;
    explicit NARAlgorithm(std::vector<std::string_view> phase_names);
};

}  // namespace algos
