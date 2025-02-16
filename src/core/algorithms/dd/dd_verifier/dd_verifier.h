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
        std::size_t num_error_pairs_;
        double error_ = 0.;
        std::shared_ptr<ColumnLayoutRelationData> relation_;
        std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
        std::vector<std::pair<std::size_t, std::pair<int, int> > > highlights_;
        //TODO: I need function PrintStats like in fd_verifier
        //That field mean "vector of number of pairs, that not holds the dd"

        void RegisterOptions();

        void VisualizeHighlights();

        void PrintStatistics();

        std::vector<std::pair<int, int> > GetRowsWhereLhsHolds(
            const std::list<model::DFStringConstraint> &constraints) const;

        double CalculateDistance(model::ColumnIndex column_index,
                                 const std::pair<std::size_t, std::size_t> &tuple_pair) const;

        void CheckDFOnRhs(const std::vector<std::pair<int, int> > &lhs);

        void ResetState() final {
            num_columns_ = 0;
            num_rows_ = 0;
        }

    protected:
        void LoadDataInternal() override;

        void MakeExecuteOptsAvailable() override;

        unsigned long long ExecuteInternal() override;

    public:
        DDVerifier();
        double GetError() const;
        std::size_t GetNumErrorPairs() const;
        bool DDHolds() const;
        void VerifyDD();
    };
}
