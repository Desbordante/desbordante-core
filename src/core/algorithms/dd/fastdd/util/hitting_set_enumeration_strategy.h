#pragma once

namespace algos::dd {

enum class HittingSetEnumerationStrategy {
    EvidenceInverter,  // original strategy, as in java
    MMCS               // faster strategy, used in HPIValid
};

}  // namespace algos::dd
