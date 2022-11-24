#pragma once

#include "algorithms/ind/faida/inclusion_testing/iinclusion_tester.h"
#include "algorithms/ind/faida/preprocessing/preprocessor.h"
#include "algorithms/ind/faida/util/simple_ind.h"
#include "algorithms/ind/ind_algorithm.h"
#include "config/thread_number/type.h"

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
    bool detect_nary_;
    bool ignore_null_cols_;
    bool ignore_const_cols_;
    config::ThreadNumType number_of_threads_;

    size_t prepr_time_ = 0;
    size_t insert_time_ = 0;
    size_t check_time_ = 0;

    std::unique_ptr<faida::IInclusionTester> inclusion_tester_;
    std::unique_ptr<faida::Preprocessor> data_;

    void LoadDataInternal() final;
    void MakeExecuteOptsAvailable() final;
    unsigned long long ExecuteInternal() final;
    void ResetINDAlgorithmState() override;

    // Insert rows into InclusionTester
    void InsertRows(faida::IInclusionTester::ActiveColumns const& active_columns,
                    faida::Preprocessor const& data);
    std::vector<SimpleIND> TestCandidates(std::vector<SimpleIND> const& candidates);
    void RegisterInds(std::vector<SimpleIND> const& inds);

public:
    struct TimeStats {
        size_t preprocessing_time;
        size_t inserting_time;
        size_t checking_time;
    };

    Faida();

    TimeStats GetTimeStats() const {
        return {prepr_time_, insert_time_, check_time_};
    }

    std::string GetTimeStatsString() const {
        std::stringstream ss;
        ss << "Preprocessing time:\t" << prepr_time_ << "ms\n"
           << "Inserting rows time:\t" << insert_time_ << "ms\n"
           << "Candidates check time:\t" << check_time_ << "ms\n";
        return ss.str();
    }

    std::vector<std::shared_ptr<SimpleCC>> CreateUnaryCCs(faida::Preprocessor const& data) const;
    std::vector<SimpleIND> CreateUnaryINDCandidates(
            std::vector<std::shared_ptr<SimpleCC>> const& combinations) const;
    std::vector<std::shared_ptr<SimpleCC>> ExtractCCs(
            std::vector<SimpleIND> const& candidates) const;
};

}  // namespace algos
