#pragma once

#include <memory>

#include "algorithms/fem/fem_algorithm.h"
#include "model/sequence/complex_event_sequence.h"

namespace algos::maxfem {

class MaxFEM : public FEMAlgorithm {
private:
    std::unique_ptr<model::ComplexEventSequence> event_sequence_;

    void ResetState() override;

    unsigned long long ExecuteInternal() override;

public:
    MaxFEM() {}
};

}
