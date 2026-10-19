#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <vector>

#include "core/algorithms/cfd/ctane/c_lattice_vertex.h"
#include "core/algorithms/cfd/model/cfd_relation_data.h"

class CLatticeLevel {
private:
    using ColumnPartitions = std::map<algos::cfd::Item, algos::cfd::PartitionTIdList>;

    std::size_t arity_;
    std::vector<std::unique_ptr<CLatticeVertex>> vertices_;

    static ColumnPartitions BuildColumnPartitions(algos::cfd::CFDRelationData const& relation,
                                                  std::size_t column_index, unsigned min_support);

public:
    explicit CLatticeLevel(std::size_t arity);

    std::size_t GetArity() const;
    std::vector<std::unique_ptr<CLatticeVertex>>& GetVertices();
    std::vector<std::unique_ptr<CLatticeVertex>> const& GetVertices() const;
    CLatticeVertex* GetLatticeVertex(algos::cfd::TuplePattern const& tuple_pattern) const;
    void Add(std::unique_ptr<CLatticeVertex> vertex);

    static std::unique_ptr<CLatticeVertex> GenerateFirstLevel(
            std::vector<std::unique_ptr<CLatticeLevel>>& levels,
            algos::cfd::CFDRelationData const& relation, unsigned min_support);
    static void GenerateNextLevel(std::vector<std::unique_ptr<CLatticeLevel>>& levels);
};
