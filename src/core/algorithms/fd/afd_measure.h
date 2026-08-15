#pragma once

#include "core/util/export.h"

namespace model {
// See "Measuring Approximate Functional Dependencies: a Comparative Study" by Parciak et al.
enum class DESBORDANTE_EXPORT AfdMeasure : char {
    kRho,  // Implicitly used in Soft FDs
    kG2,
    kG3,        // per-tuple in PFD terminology
    kPerValue,  // not part of the classification, but still an AFD measure by definition
    // kG3Prime,
    // kG1S,
    kFi,
    // kRFIPlus,
    // kRFIPrimePlus,
    // kSFI,
    kG1,
    // kG1Prime,
    kPdep,
    kTau,
    kMuPlus,
};
}  // namespace model
