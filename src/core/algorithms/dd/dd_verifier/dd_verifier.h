#pragma once
#include "algorithm.h"
#include "algorithms/dd/dd.h"
#include "table/column_layout_relation_data.h"
#include "table/column_layout_typed_relation_data.h"
#include "tabular_data/input_table_type.h"

namespace algos::dd {
    using DF = model::DFStringConstraint;
    using DD = model::DDString;

    class DDVerifier : public Algorithm {
    private:
        DD dd_;
        config::InputTable input_table_;
        std::shared_ptr<ColumnLayoutRelationData> relation_;
        std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
        unsigned num_rows_;
        model::ColumnIndex num_columns_;
        void RegisterOptions();
        std::vector<model::PLI::Cluster> GetRowsHolds(DF constraint);
        double DDVerifier::CalculateDistance(model::ColumnIndex column_index,
                                         const std::pair<std::size_t, std::size_t> &tuple_pair) const;
    protected:
        void LoadDataInternal() override;
        void MakeExecuteOptsAvailable() override;
        unsigned long long ExecuteInternal() override;
    public:
        DDVerifier(DD dd);
        void VerifyDD();




    };


}
