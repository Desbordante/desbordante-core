#pragma once

#include <memory>

#include <enum.h>

#include "core/algorithms/algorithm.h"
#include "core/model/sequence/complex_event_sequence.h"
#include "core/model/sequence/isequence_stream.h"

namespace algos {

class FEMAlgorithm : public Algorithm {
private:
    std::shared_ptr<model::ISequenceStream> sequence_stream_;

    void LoadDataInternal() override;

protected:
    std::unique_ptr<model::ComplexEventSequence> event_sequence_;

public:
    FEMAlgorithm();
};

}  // namespace algos
