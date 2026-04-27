#include "Candidate.hpp"

Candidate::Candidate(Prefix prefix, Bitmap bitmap, std::vector<int> sn,
                    std::vector<int> in, int hasToBeGreaterThanForIStep, 
                    int candidateLength, int support)
    : prefix(std::move(prefix)), bitmap(std::move(bitmap)),
      sn(std::move(sn)), in(std::move(in)),
      hasToBeGreaterThanForIStep(hasToBeGreaterThanForIStep),
      candidateLength(candidateLength),
      cachedSupport(support) {}

bool Candidate::operator<(const Candidate& other) const {
    if (&other == this) return false;
    int compare = other.cachedSupport - cachedSupport;
    if (compare != 0) return compare < 0;
    size_t hashThis = reinterpret_cast<size_t>(this);
    size_t hashOther = reinterpret_cast<size_t>(&other);
    compare = static_cast<int>(hashThis - hashOther);
    if (compare != 0) return compare < 0;
    compare = static_cast<int>(prefix.getItemsets().size() -
                              other.prefix.getItemsets().size());
    if (compare != 0) return compare < 0;
    return hasToBeGreaterThanForIStep < other.hasToBeGreaterThanForIStep;
}