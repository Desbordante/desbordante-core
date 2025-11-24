#pragma once

#include <memory>

#include "algorithms/algorithm.h"
#include "model/sequence/complex_event_sequence.h"

namespace algos {

class FEMAlgorithm : public Algorithm {
private:
    void LoadDataInternal() override;

protected:
    std::unique_ptr<model::ComplexEventSequence> event_sequence_;

public:
    FEMAlgorithm();
};

}  // namespace algos
