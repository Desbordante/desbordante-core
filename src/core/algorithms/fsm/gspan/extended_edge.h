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
    bool SmallerThanOriginal(ExtendedEdge const& other) const;

    bool operator==(ExtendedEdge const& other) const = default;
    bool operator!=(ExtendedEdge const& other) const = default;

    struct Hash {
        size_t operator()(ExtendedEdge const& ee) const {
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

}  // namespace gspan
