#pragma once

#include <string>

#include "graph.h"

namespace gspan {

struct ExtendedEdge {
private:
    bool PairSmallerThan(int x1, int x2, int y1, int y2) const;

public:
    Vertex vertex1;
    Vertex vertex2;

    int label;

    ExtendedEdge() = default;

    ExtendedEdge(Vertex const& v1, Vertex const& v2, int label)
        : vertex1(v1), vertex2(v2), label(label) {}

    bool SmallerThan(ExtendedEdge const& other) const;

    bool operator==(ExtendedEdge const& other) const = default;
    bool operator!=(ExtendedEdge const& other) const = default;

    struct Hash {
        size_t operator()(ExtendedEdge const& ee) const noexcept {
            size_t seed = 0;
            boost::hash_combine(seed, ee.vertex1.id);
            boost::hash_combine(seed, ee.vertex2.id);
            boost::hash_combine(seed, ee.vertex1.label);
            boost::hash_combine(seed, ee.vertex2.label);
            boost::hash_combine(seed, ee.label);
            return seed;
        }
    };

    std::string ToString() const;
};

struct ExtendedEdgeProjectCompare {
    bool operator()(ExtendedEdge const& first, ExtendedEdge const& second) const {
        return std::tuple{first.vertex1.label, first.label, first.vertex2.label} <
               std::tuple{second.vertex1.label, second.label, second.vertex2.label};
    }
};

struct ExtendedEdgeBackwardCompare {
    bool operator()(ExtendedEdge const& first, ExtendedEdge const& second) const {
        if (first.vertex2.id != second.vertex2.id) {
            return first.vertex2.id < second.vertex2.id;
        }
        return first.label < second.label;
    }
};

struct ExtendedEdgeForwardCompare {
    bool operator()(ExtendedEdge const& first, ExtendedEdge const& second) const {
        if (first.vertex1.id != second.vertex1.id) {
            return first.vertex1.id > second.vertex1.id;
        }
        if (first.label != second.label) {
            return first.label < second.label;
        }

        return first.vertex2.label < second.vertex2.label;
    }
};

}  // namespace gspan
