#pragma once

#include <list>
#include <string_view>
#include <vector>

#include "algorithms/algorithm.h"
#include "algorithms/options/equal_nulls/option.h"
#include "algorithms/options/equal_nulls/type.h"
#include "model/ucc.h"
#include "util/primitive_collection.h"

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
    // Collection of all mined UCCs. Every UCC mining algorithm must register found uccs here.
    util::PrimitiveCollection<model::UCC> ucc_collection_;
    config::EqNullsType is_null_equal_null_{};

    // Pass this value as phase_names to the constructor if your algorithm has only one progress bar
    // phase.
    // If your algorithm has no progress bar implemented, pass an empty vector.
    constexpr static std::string_view kDefaultPhaseName = "UCC mining";

    explicit UCCAlgorithm(std::vector<std::string_view> phase_names)
        : Algorithm(std::move(phase_names)) {
        RegisterOptions();
        MakeOptionsAvailable({config::EqualNullsOpt.GetName()});
    }

public:
    std::list<model::UCC> const& UCCList() const noexcept {
        return ucc_collection_.AsList();
    }

    std::list<model::UCC>& UCCList() noexcept {
        return ucc_collection_.AsList();
    }
};

}  // namespace algos
