#pragma once

#include "core/algorithms/fd/fdhits/edges/multi_fd.h"
#include "core/algorithms/fd/fdhits/treesearch/dynamic_edgemark_store_multi.h"
#include "core/algorithms/fd/fdhits/treesearch/hypergraph.h"
#include "core/algorithms/fd/fdhits/validator/validator.h"

namespace algos::fd::fdhits {

void SearchMulti(Hypergraph<edges::MultiFD>* graph, Validator<edges::MultiFD>* target);

}  // namespace algos::fd::fdhits
