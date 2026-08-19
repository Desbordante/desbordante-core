#pragma once

#include <map>
#include <vector>

#include "core/algorithms/fd/tane/model/lattice_vertex.h"

namespace model {

class LatticeLevel {
private:
    std::map<boost::dynamic_bitset<>, std::unique_ptr<LatticeVertex>> vertices_;

public:
    std::map<boost::dynamic_bitset<>, std::unique_ptr<LatticeVertex>>& GetVertices() {
        return vertices_;
    }

    LatticeVertex const* GetLatticeVertex(boost::dynamic_bitset<> const& column_indices) const;
    void Add(std::unique_ptr<LatticeVertex> vertex);

    // using vectors instead of lists because of .get()
    static void GenerateNextLevel(std::vector<std::unique_ptr<LatticeLevel>>& levels);
    static void ClearLevelsBelow(std::vector<std::unique_ptr<LatticeLevel>>& levels,
                                 unsigned int arity);
};

}  // namespace model
