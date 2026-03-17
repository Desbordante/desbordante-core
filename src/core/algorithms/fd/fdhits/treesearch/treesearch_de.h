#pragma once

#include "core/algorithms/fd/fdhits/edges/default_edge.h"
#include "core/algorithms/fd/fdhits/treesearch/dynamic_edgemark_store.h"
#include "core/algorithms/fd/fdhits/treesearch/hypergraph.h"
#include "core/algorithms/fd/fdhits/validator/validator.h"

namespace algos::fd::fdhits {

void SearchDE(Hypergraph<edges::DefaultEdge>* graph, Validator<edges::DefaultEdge>* target);

}  // namespace algos::fd::fdhits
