#include "pattern_tableau.h"

namespace algos::cfdfinder {

size_t PatternTableau::GetGlobalCover() const {
    return std::accumulate(patterns_.begin(), patterns_.end(), 0u,
                           [](size_t sum, Pattern const& p) { return sum + p.GetNumCover(); });
}

size_t PatternTableau::GetGlobalKeepers() const {
    return std::accumulate(patterns_.begin(), patterns_.end(), 0u,
                           [](size_t sum, Pattern const& p) { return sum + p.GetNumKeepers(); });
}

double PatternTableau::GetSupport() const {
    return static_cast<double>(GetGlobalCover()) / num_tuples_;
}

double PatternTableau::GetConfidence() const {
    size_t cover = GetGlobalCover();
    return cover == 0 ? 0.0 : static_cast<double>(GetGlobalKeepers()) / cover;
}
}  // namespace algos::cfdfinder