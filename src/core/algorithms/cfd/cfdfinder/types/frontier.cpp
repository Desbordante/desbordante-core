#include "core/algorithms/cfd/cfdfinder/types/frontier.h"

#include <utility>

namespace algos::cfdfinder {

void Frontier::Emplace(Pattern&& pattern) {
    container_.insert(std::move(pattern));
}

Pattern Frontier::Poll() {
    auto it = container_.begin();
    return std::move(container_.extract(it).value());
}

bool Frontier::Empty() const {
    return container_.empty();
}

void Frontier::Rebuild(boost::dynamic_bitset<> const& used_rows_mask, Row const& inverted_pli_rhs,
                       PruningStrategy const& pruning_strategy) {
    std::multiset<Pattern, std::greater<Pattern>> old_container;
    container_.swap(old_container);

    while (!old_container.empty()) {
        auto node = old_container.extract(old_container.begin());
        Pattern pattern = std::move(node.value());

        pattern.UpdateCover(used_rows_mask);

        if (pruning_strategy.IsPatternWorthConsidering(pattern.GetSupport())) {
            pattern.UpdateKeepers(inverted_pli_rhs);
            container_.insert(std::move(pattern));
        }
    }
}

}  // namespace algos::cfdfinder
