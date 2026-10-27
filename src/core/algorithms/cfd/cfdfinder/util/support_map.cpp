#include "core/algorithms/cfd/cfdfinder/util/support_map.h"

#include <memory>
#include <variant>

namespace algos::cfdfinder::utils {

void SupportMap::SetSupport(Candidate const& candidate, double support) {
    std::visit(
            [&](auto& map) {
                if constexpr (std::is_same_v<std::decay_t<decltype(map)>,
                                             std::shared_ptr<ThreadSafeSupportMap>>) {
                    map->SetSupport(candidate, support);
                } else {
                    map[candidate] = support;
                }
            },
            support_map_);
}

double SupportMap::GetSupport(Candidate const& candidate) const {
    return std::visit(
            [&](auto const& map) -> double {
                if constexpr (std::is_same_v<std::decay_t<decltype(map)>,
                                             std::shared_ptr<ThreadSafeSupportMap>>) {
                    return map->GetSupport(candidate);
                } else {
                    auto it = map.find(candidate);
                    return it != map.end() ? it->second : 0.0;
                }
            },
            support_map_);
}

bool SupportMap::Contains(Candidate const& candidate) const {
    return std::visit(
            [&](auto const& map) -> bool {
                if constexpr (std::is_same_v<std::decay_t<decltype(map)>,
                                             std::shared_ptr<ThreadSafeSupportMap>>) {
                    return map->Contains(candidate);
                } else {
                    return map.contains(candidate);
                }
            },
            support_map_);
}
}  // namespace algos::cfdfinder::utils