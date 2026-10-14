#pragma once

#include "core/algorithms/ind/faida/inclusion_testing/iinclusion_tester.h"
#include "core/algorithms/ind/faida/preprocessing/preprocessor.h"
#include "core/algorithms/ind/faida/util/simple_ind.h"
#include "core/algorithms/ind/ind_algorithm.h"
#include "core/config/max_arity/type.h"
#include "core/config/thread_number/type.h"

namespace algos {

class Faida : public INDAlgorithm {
private:
    using ColumnIndex = faida::ColumnIndex;
    using TableIndex = faida::TableIndex;
    using SimpleCC = faida::SimpleCC;
    using SimpleIND = faida::SimpleIND;
    using AbstractColumnStore = faida::AbstractColumnStore;

    int sample_size_;
    double hll_accuracy_;
    config::MaxArityType max_arity_;
    bool ignore_null_cols_;
    bool ignore_const_cols_;
    config::ThreadNumType number_of_threads_;

    std::unique_ptr<faida::IInclusionTester> inclusion_tester_;
    std::unique_ptr<faida::Preprocessor> data_;

    void LoadINDAlgorithmDataInternal() final;
    void MakeExecuteOptsAvailable() final;
    void ExecuteInternal() final;
    void ResetINDAlgorithmState() override;

    // Insert rows into InclusionTester
    void InsertRows(faida::IInclusionTester::ActiveColumns const& active_columns,
                    faida::Preprocessor const& data);
    std::vector<SimpleIND> TestCandidates(std::vector<SimpleIND> const& candidates);
    void RegisterInds(std::vector<SimpleIND> const& inds);

public:
    Faida();

    std::vector<std::shared_ptr<SimpleCC>> CreateUnaryCCs(faida::Preprocessor const& data) const;
    std::vector<SimpleIND> CreateUnaryINDCandidates(
            std::vector<std::shared_ptr<SimpleCC>> const& combinations) const;
    std::vector<std::shared_ptr<SimpleCC>> ExtractCCs(
            std::vector<SimpleIND> const& candidates) const;
};

}  // namespace algos
