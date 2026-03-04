#pragma once

#include <list>
#include <string_view>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/ucc/ucc.h"
#include "core/config/equal_nulls/type.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/util/primitive_collection.h"

namespace algos {

// Base class for all algorithms that mine UCCs
class UCCAlgorithm : public Algorithm {
private:
    void ResetState() final {
        ucc_collection_.Clear();
        ResetUCCAlgorithmState();
    }

    virtual void ResetUCCAlgorithmState() = 0;

    void RegisterOptions();

protected:
    config::InputTable input_table_;

    // Collection of all mined UCCs. Every UCC mining algorithm must register found uccs here.
    util::PrimitiveCollection<model::UCC> ucc_collection_;
    config::EqNullsType is_null_equal_null_{};

public:
    UCCAlgorithm();

    std::list<model::UCC> const& UCCList() const noexcept {
        return ucc_collection_.AsList();
    }

    std::list<model::UCC>& UCCList() noexcept {
        return ucc_collection_.AsList();
    }
};

}  // namespace algos
