#pragma once

#include <cmath>
#include <cstddef>
#include <hash_table8.hpp>
#include <memory>
#include <optional>
#include <unordered_map>

#include "algorithms/ind/faida/preprocessing/preprocessor.h"
#include "hll_data.h"
#include "iinclusion_tester.h"
#include "ind/faida/inclusion_testing/hyperloglog.h"
#include "ind/faida/preprocessing/irow_iterator.h"
#include "ind/faida/util/simple_cc.h"
#include "sampled_inverted_index.h"

namespace algos::faida {

class CombinedInclusionTester : public IInclusionTester {
private:
    size_t const null_hash_;
    SampledInvertedIndex sampled_inverted_index_;
    std::unordered_map<TableIndex, emhash8::HashMap<std::shared_ptr<SimpleCC>, HLLData>>
            hlls_by_table_;

    TableIndex curr_table_idx_;
    int num_certain_checks_;
    int num_uncertain_checks_;
    int max_id_;
    double error_;
    unsigned num_threads_;

    static int CalcNumBits(double error) {
        return int(log((1.106 / error) * (1.106 / error)) / log(2));
    }

    HLLData CreateApproxDataStructure() const {
        HLLData data;
        data.SetHll(hll::HyperLogLog(CalcNumBits(error_)));
        return data;
    }

    void InsertRowIntoHLL(size_t row_hash, HLLData& data) const {
        std::optional<hll::HyperLogLog>& hll = data.GetHll();
        if (!hll.has_value()) {
            data.SetHll(hll::HyperLogLog(CalcNumBits(error_)));
        }
        hll->add_hash(row_hash);
    }

    bool TestWithHLLs(HLLData const& dep_hll, HLLData const& ref_hll) const {
        return dep_hll.IsIncludedIn(ref_hll);
    }

    bool TestWithHLLs(std::shared_ptr<SimpleCC> const& dep, std::shared_ptr<SimpleCC> const& ref) {
        return TestWithHLLs(hlls_by_table_[dep->GetTableIndex()][dep],
                            hlls_by_table_[ref->GetTableIndex()][ref]);
    }

public:
    CombinedInclusionTester(unsigned num_threads, double error, size_t null_hash)
        : null_hash_(null_hash),
          curr_table_idx_(-1),
          num_certain_checks_(0),
          num_uncertain_checks_(0),
          max_id_(-1),
          error_(error),
          num_threads_(num_threads) {}

    ActiveColumns SetCCs(std::vector<std::shared_ptr<SimpleCC>>& combinations) override;
    void Initialize(std::vector<HashedTableSample> const& table_samples) override;

    void StartInsertRow(TableIndex table_idx) override;
    void InsertRows(IRowIterator::Block const& values, size_t block_size) override;

    bool IsIncludedIn(std::shared_ptr<SimpleCC> const& dep,
                      std::shared_ptr<SimpleCC> const& ref) override;

    void FinalizeInsertion() override {
        sampled_inverted_index_.FinalizeInsertion(hlls_by_table_);
    };

    int GetNumCertainChecks() const override {
        return num_certain_checks_;
    }

    int GetNumUncertainChecks() const override {
        return num_uncertain_checks_;
    }

    ~CombinedInclusionTester() override = default;
};

}  // namespace algos::faida
