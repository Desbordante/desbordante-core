#pragma once

#include <type_traits>

#include <boost/container_hash/hash.hpp>

#include "core/util/logger.h"
#include "gdd_validator.h"
#include "qvo_strategies.h"

namespace algos {

class WcojGddValidator : public GddValidator {
private:
    enum class Direction : std::uint8_t { kIn, kOut };
    enum class OperationResult : std::uint8_t { kEmpty, kProduced, kFinished };

    struct AdjacencyDescriptor {
        VertexT pattern_vertex;
        std::size_t qvo_index;  // position of pattern_vertex in qvo_
        Direction direction;
        std::string edge_label;
    };

    struct NeighborKey {
        VertexT graph_vertex;
        Direction direction;
        std::string edge_label;

        bool operator==(NeighborKey const&) const = default;
    };

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

    // wcoj matching is iterative process - match is built vertex-by-vertex.
    // level is a partial mapping of first k pattern vertices to graph vertices.
    // keys may be dropped: it is a prefix of QVO array (prefix size is a
    // number of level). hence, we can store all matches as matrix (flat array
    // of graph vertices, data, with width - mapping level).
    // `MappingT match` can be restored as {(qvo[i], row[i]) for i in 0..width}
    // for each row in level
    struct MatchLevel {
        std::vector<VertexT> data;
        std::size_t width = 0;

        void clear() noexcept {
            data.clear();
            width = 0;
        }

        void reset(std::size_t w) {
            data.clear();
            width = w;
        }

        std::size_t count() const noexcept {
            return width != 0 ? data.size() / width : 0;
        }

        VertexT const* row(std::size_t i) const noexcept {
            return data.data() + i * width;
        }

        void push_row(VertexT const* parent, std::size_t parent_width, VertexT appended) {
            data.insert(data.end(), parent, parent + parent_width);
            data.push_back(appended);
        }
    };

    DomainT domain_;
    MatchLevel cur_level_;
    MatchLevel next_level_;
    std::size_t level_count_ = 0;  // == cur_level_.width
    std::vector<VertexT> qvo_;
    model::Gdd const* gdd_ = nullptr;
    model::gdd::graph_t const* pattern_ = nullptr;
    model::gdd::graph_t const* graph_ = nullptr;

    OperationResult Scan();  // differs from paper - builds 1-match, not 2-match.
    OperationResult ExtendIntersect();

    void Prepare(model::Gdd const& gdd, model::gdd::graph_t const& graph);
    bool IsPatternWeaklyConnected() const;

    template <QvoStrategy Strategy>
    std::vector<VertexT> BuildQueryVertexOrder() const {
        if (pattern_ == nullptr) {
            LOG_WARN("Calling BuildQueryVertexOrder while pattern_ == nullptr");
            return {};
        }
        if (graph_ == nullptr) {
            LOG_WARN("Calling BuildQueryVertexOrder while graph_ == nullptr");
            return {};
        }
        return Strategy(*graph_, *pattern_, domain_).Order();
    }

    std::vector<AdjacencyDescriptor> BuildDescriptorsFor(VertexT new_pv, std::size_t level) const;
    // partial_match points to a FlatLevel row
    std::vector<VertexT> const& ComputeExtensionSet(
            VertexT const* partial_match, VertexT new_pv,
            std::vector<AdjacencyDescriptor> const& descriptors);
    std::vector<VertexT> const& GetNeighbors(VertexT graph_vertex, Direction direction,
                                             std::string const& edge_label);

    // `IntersectSorted` is in hot path, scratch_ is a buffer that supposed not to shrink
    // its capacity and be used for insertions of `std::set_intersection` result
    std::vector<VertexT> scratch_;
    void IntersectSorted(std::vector<std::vector<VertexT> const*>& lists,
                         std::vector<VertexT>& out);

    std::unordered_map<NeighborKey, std::vector<VertexT>, NeighborKeyHash> adjacency_index_;

    struct IntersectionCache {
        // mapped graph vertices in the same order descriptors are
        std::vector<VertexT> last_isect_key;
        std::vector<VertexT> last_isect_set;
        bool last_isect_valid = false;
    } intersection_cache_;

    std::size_t match_count_ = 0;

protected:
    virtual GddHoldsResult Holds(model::Gdd const& gdd, model::gdd::graph_t const& graph) final;

    virtual std::unique_ptr<GddValidator> CreateWorker() const final;

public:
    WcojGddValidator() = default;
};

}  // namespace algos
