#include "algorithms/md/md_verifier/highlights/highlights.h"

namespace algos::md {

void MDHighlights::AddHighlight(HighlightRecord record) {
    highlights_.push_back(record);
}

void MDHighlights::Reset() {
    highlights_.clear();
}
}  // namespace algos::md