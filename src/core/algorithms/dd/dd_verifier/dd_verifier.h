#pragma once
#include "algorithm.h"
#include "algorithms/dd/dd.h"
#include "table/column_layout_relation_data.h"
#include "table/column_layout_typed_relation_data.h"
#include "tabular_data/input_table_type.h"

namespace algos::dd {
    using DFs = model::DFStringConstraint;
    using DDs = model::DDString;

    class DDVerifier : public Algorithm {
    private:
        DDs dd_;
        config::InputTable input_table_;
        std::size_t num_rows_;
        std::size_t num_columns_;
        std::shared_ptr<ColumnLayoutRelationData> relation_;
        std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
        void RegisterOptions();
        std::vector<std::pair<int, int>> GetRowsHolds(const std::list<model::DFStringConstraint> &constraints) const;
        double CalculateDistance(model::ColumnIndex column_index,
                                         const std::pair<std::size_t, std::size_t> &tuple_pair) const;
        std::vector<std::pair<int, int>> CheckDFOnRhs(const std::vector<std::pair<int, int>>& lhs) const;
        void ResetState() final {
            dd_.left.clear();
            dd_.right.clear();
        }
        void SetLimits();
    protected:
        void LoadDataInternal() override;
        void MakeExecuteOptsAvailable() override;
        unsigned long long ExecuteInternal() override;
    public:
        DDVerifier();
        bool VerifyDD() const;




    };


}
