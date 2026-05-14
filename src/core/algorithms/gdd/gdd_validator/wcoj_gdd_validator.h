#pragma once

#include <type_traits>

#include <boost/container_hash/hash.hpp>

#include "gdd_validator.h"

namespace algos {

class WcojGddValidator : public GddValidator {
private:
    enum class Direction : std::uint8_t { kIn, kOut };
    enum class OperationResult : std::uint8_t { kEmpty, kProduced, kFinished };

    struct AdjacencyDescriptor {
        VertexT pattern_vertex;
        Direction direction;
        std::string edge_label;
    };

    struct NeighborKey {
        VertexT graph_vertex;
        Direction direction;
        std::string edge_label;

        bool operator==(NeighborKey const&) const = default;
    };

    using MatchLevelT = std::vector<MappingT>;

    DomainT domain_;
    std::vector<MatchLevelT> match_levels_;
    std::vector<VertexT> qvo_;
    model::Gdd const* gdd_ = nullptr;
    model::gdd::graph_t const* pattern_ = nullptr;
    model::gdd::graph_t const* graph_ = nullptr;

    // differs from paper - builds 1-match, not 2-match.
    OperationResult Scan();
    OperationResult ExtendIntersect();

    void Prepare(model::Gdd const& gdd, model::gdd::graph_t const& graph);
    std::vector<VertexT> BuildQueryVertexOrder() const;
    std::vector<AdjacencyDescriptor> BuildDescriptorsFor(VertexT new_pv, std::size_t level) const;
    std::vector<VertexT> ComputeExtensionSet(MappingT const& partial_match, VertexT new_pv,
                                             std::vector<AdjacencyDescriptor> const& descriptors);
    std::vector<VertexT> const& GetNeighbors(VertexT graph_vertex, Direction direction,
                                             std::string const& edge_label);

    static std::vector<VertexT> IntersectSorted(std::vector<VertexT> const& lhs,
                                                std::vector<VertexT> const& rhs);

    struct NeighborKeyHash {
        std::size_t operator()(NeighborKey const& key) const {
            using boost::hash_combine;
            using boost::hash_value;

            std::size_t seed = 0;

            hash_combine(seed, hash_value(key.graph_vertex));
            hash_combine(seed,
                         hash_value(static_cast<std::underlying_type_t<Direction>>(key.direction)));
            hash_combine(seed, hash_value(key.edge_label));

            return seed;
        }
    };

    std::unordered_map<NeighborKey, std::vector<VertexT>, NeighborKeyHash> adjacency_index_;

protected:
    virtual std::optional<GddCounterexample> Holds(model::Gdd const& gdd,
                                                   model::gdd::graph_t const& graph) final;

public:
    WcojGddValidator() = default;
};

}  // namespace algos
