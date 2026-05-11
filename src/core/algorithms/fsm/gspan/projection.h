#pragma once

#include <span>
#include <vector>
#include <boost/unordered/unordered_flat_map.hpp>
#include "graph.h"
#include "extended_edge.h"

namespace gspan {

class FlatIsoms {
    std::vector<vertex_t> data_;
    std::vector<size_t> offsets_;

public:
    void clear() {
        data_.clear();
        offsets_.clear();
    }

    size_t size() const {
        return offsets_.size();
    }

    void swap(FlatIsoms& other) {
        data_.swap(other.data_);
        offsets_.swap(other.offsets_);
    }

    void push(std::span<vertex_t const> iso) {
        offsets_.push_back(data_.size());
        data_.insert(data_.end(), iso.begin(), iso.end());
    }

    void push_with_extra(std::span<vertex_t const> iso, vertex_t extra) {
        offsets_.push_back(data_.size());
        data_.insert(data_.end(), iso.begin(), iso.end());
        data_.push_back(extra);
    }

    std::span<vertex_t const> operator[](size_t i) const {
        size_t start = offsets_[i];
        size_t end = (i + 1 < offsets_.size()) ? offsets_[i + 1] : data_.size();
        return {data_.data() + start, end - start};
    }

    struct Iterator {
        FlatIsoms const* list;
        size_t idx;

        std::span<vertex_t const> operator*() const {
            return (*list)[idx];
        }

        Iterator& operator++() {
            ++idx;
            return *this;
        }

        bool operator!=(Iterator const& other) const {
            return idx != other.idx;
        }
    };

    Iterator begin() const {
        return {this, 0};
    }

    Iterator end() const {
        return {this, offsets_.size()};
    }
};

// Represents a specific entry of the dfscode in the graph
struct ProjectionEntry {
    int graph_id;
    FlatIsoms isoms;
};

// A projection is a collection of entries across multiple graphs
using Projection = std::vector<ProjectionEntry>;

using projection_map_t = boost::unordered_flat_map<ExtendedEdge, Projection, ExtendedEdge::Hash>;

} // namespace gspan
