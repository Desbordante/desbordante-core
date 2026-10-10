#pragma once

#include <span>
#include <type_traits>
#include <utility>

#include <boost/container_hash/hash.hpp>

#include "graph_matcher.h"
#include "qvo_strategies.h"
#include "core/util/logger.h"

namespace algos {

namespace detail {

// independent from callback type, separated from WcojGraphMatcher for compilation time purposes
// (we can place definitions in .cpp file)
class WcojCore : private model::gdd::GraphTypes {
public:
    // wcoj matching is iterative process - match is built vertex-by-vertex.
    // level is a partial mapping of first k pattern vertices to graph vertices.
    // keys may be dropped: it is a prefix of QVO array (prefix size is a
    // number of level). hence, we can store all matches as matrix (flat array
    // of graph vertices, data, with width - mapping level).
    // `Mapping match` can be restored as {(qvo[i], row[i]) for i in 0..width}
    // for each row in level
    struct MatchLevel {
        std::vector<Vertex> data;
        std::size_t width = 0;

        void Clear() noexcept {
            data.clear();
            width = 0;
        }

        void Reset(std::size_t w) noexcept {
            data.clear();
            width = w;
        }

        std::size_t Count() const noexcept {
            return width != 0 ? data.size() / width : 0;
        }

        std::span<Vertex const> Row(std::size_t i) const noexcept {
            return std::span{data.begin() + i * width, width};
        }

        void PushRow(std::span<Vertex const> parent, Vertex appended) {
            data.insert(data.end(), parent.begin(), parent.end());
            data.push_back(appended);
        }
    };

    explicit WcojCore(Graph const& graph) : graph_(graph) {}

    // all matches of pattern, valid until the next call
    // column i of a row is the image of pattern vertex Order()[i]
    MatchLevel const& FindAll(Graph const& pattern, Domain domain, bool injective);

    std::vector<Vertex> const& Order() const noexcept {
        return qvo_;
    }

private:
    enum class Direction : std::uint8_t { kIn, kOut };
    enum class OperationResult : std::uint8_t { kEmpty, kProduced, kFinished };

    struct AdjacencyDescriptor {
        Vertex pattern_vertex;
        std::size_t qvo_index;  // position of pattern_vertex in qvo_
        Direction direction;
        std::string_view edge_label;
    };

    struct NeighborKey {
        Vertex graph_vertex;
        Direction direction;
        std::string_view edge_label;

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

    Graph const& graph_;
    Graph const* pattern_ = nullptr;
    bool injective_ = false;
    Domain domain_;
    MatchLevel cur_level_;
    MatchLevel next_level_;
    std::size_t level_count_ = 0;  // == cur_level_.width
    std::vector<Vertex> qvo_;

    OperationResult Scan();  // differs from paper - builds 1-match, not 2-match.
    OperationResult ExtendIntersect();

    void Prepare(Graph const& pattern, Domain domain);
    bool IsPatternWeaklyConnected() const;

    template <QvoStrategy Strategy>
    std::vector<Vertex> BuildQueryVertexOrder() const {
        if (pattern_ == nullptr) {
            LOG_WARN("Calling BuildQueryVertexOrder while pattern_ == nullptr");
            return {};
        }
        return Strategy(graph_, *pattern_, domain_).Order();
    }

    std::vector<AdjacencyDescriptor> BuildDescriptorsFor(Vertex new_pv, std::size_t level) const;
    std::vector<Vertex> const& ComputeExtensionSet(
            std::span<Vertex const> partial_match, Vertex new_pv,
            std::vector<AdjacencyDescriptor> const& descriptors);
    std::vector<Vertex> const& GetNeighbors(Vertex graph_vertex, Direction direction,
                                            std::string_view edge_label);

    // set intersections is in hot path, the optimal way to deal with them
    // is "Leapfrog join for unary predicates"; see https://arxiv.org/pdf/1210.0481
    // this algo has complexity of O(N_{min} * log(N_{max}/N_{min})) instead of
    // pairwise intersection which is O(N_{max} * lists.size())
    struct ListCursor {
        Vertex const* cursor;
        Vertex const* end;
    };

    // reused by LeapFrogJoin, supposed not to shrink
    std::vector<ListCursor> cursors_;
    void LeapFrogJoin(std::vector<std::vector<Vertex> const*>& lists, std::vector<Vertex>& out);

    std::unordered_map<NeighborKey, std::vector<Vertex>, NeighborKeyHash> adjacency_index_;

    struct IntersectionCache {
        // mapped graph vertices in the same order descriptors are
        std::vector<Vertex> last_isect_key;
        std::vector<Vertex> last_isect_set;
        bool last_isect_valid = false;
    } intersection_cache_;
};

}  // namespace detail

template <MatchCallback Callback>
class WcojGraphMatcher final : public GraphMatcher<Callback>,
                               private virtual model::gdd::GraphTypes {
public:
    using GraphMatcher<Callback>::GraphMatcher;

private:
    detail::WcojCore core_{this->graph_};

    virtual bool DoMatch(Graph const& pattern, Domain domain, bool injective) final {
        auto const& matches = core_.FindAll(pattern, std::move(domain), injective);
        auto const& qvo = core_.Order();

        std::size_t const matches_size = matches.Count();
        std::size_t const width = matches.width;

        Mapping full_match;
        for (std::size_t i = 0; i < matches_size; ++i) {
            auto row = matches.Row(i);
            full_match.clear();
            for (std::size_t j = 0; j < width; ++j) {
                full_match.emplace(qvo[j], row[j]);
            }

            if (!this->OnMatch(full_match)) {
                return false;
            }
        }

        return true;
    }
};

}  // namespace algos
