#include "extended_edge.h"

#include <sstream>

namespace gspan {

bool ExtendedEdge::PairSmallerThan(int x1, int x2, int y1, int y2) const {
    bool x_forward = x1 < x2;
    bool y_forward = y1 < y2;
    if (x_forward && y_forward)
        return x2 < y2 || (x2 == y2 && x1 > y1);
    else if ((!x_forward) && (!y_forward))
        return x1 < y1 || (x1 == y1 && x2 < y2);
    else if (x_forward)
        return x2 <= y1;
    else
        return x1 < y2;
}

bool ExtendedEdge::SmallerThan(ExtendedEdge const& other) const {
    if (PairSmallerThan(vertex1.id, vertex2.id, other.vertex1.id, other.vertex2.id)) return true;

    if (vertex1.id != other.vertex1.id || vertex2.id != other.vertex2.id) return false;

    return std::tuple{vertex1.label, vertex2.label, label} <
           std::tuple{other.vertex1.label, other.vertex2.label, other.label};
}

bool ExtendedEdge::SmallerThanOriginal(ExtendedEdge const& other) const {
    if (PairSmallerThan(vertex1.id, vertex2.id, other.vertex1.id, other.vertex2.id)) return true;

    if (vertex1.id != other.vertex1.id || vertex2.id != other.vertex2.id) return false;

    return std::tuple{vertex1.label, label, vertex2.label} <
           std::tuple{other.vertex1.label, other.label, other.vertex2.label};
}

std::string ExtendedEdge::ToString() const {
    std::ostringstream oss;
    oss << '<' << vertex1.id << ',' << vertex2.id << ',' << vertex1.label << ',' << vertex2.label
        << ',' << label << '>';
    return oss.str();
}

}  // namespace gspan