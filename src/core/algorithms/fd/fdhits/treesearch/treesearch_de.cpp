#include "core/algorithms/fd/fdhits/treesearch/treesearch_de.h"

#include <climits>
#include <optional>

#include "core/algorithms/fd/fdhits/treesearch/dynamic_edgemark_store.h"
#include "core/algorithms/fd/fdhits/treesearch/node_type.h"

namespace algos::fd::fdhits {

static void SearchRecursive(edges::DefaultEdge* s, DynamicEdgemarkStore* store,
                            Validator<edges::DefaultEdge>* target) {
    target->Push(store->GetNodes());

    if (store->IsFullyCovered()) {
        auto [cont, opt_edges] = target->Check(*s, store->GetNodes(), store->GetCand());
        if (opt_edges.has_value()) {
            store->Extend(*s, std::move(opt_edges.value()));
        }
        if (!cont) return;
    }

    edges::DefaultEdge const* best_uncov = nullptr;
    size_t best_overlap = SIZE_MAX;
    for (auto it = store->UncoveredEdgesBegin(); it != store->UncoveredEdgesEnd(); ++it) {
        size_t ov = it->Overlap(store->GetCurrentCand());
        if (!best_uncov || ov < best_overlap) {
            best_overlap = ov;
            best_uncov = &(*it);
        }
    }

    if (!best_uncov) return;

    edges::DefaultEdge c = best_uncov->Intersect(store->GetCurrentCand());

    store->GetCurrentCandMut().RemoveAll(c);

    for (size_t node : c.GetNodes()) {
        if (store->Add(node)) {
            s->Add(node);

            SearchRecursive(s, store, target);

            s->Remove(node);
            store->Pop();
        }
    }

    store->GetCurrentCandMut().AddAll(c);
}

void SearchDE(Hypergraph<edges::DefaultEdge>* graph, Validator<edges::DefaultEdge>* target) {
    DynamicEdgemarkStore store(graph);

    edges::DefaultEdge s = edges::DefaultEdge::Empty(graph->GetVertexCount());

    SearchRecursive(&s, &store, target);
}

}  // namespace algos::fd::fdhits
