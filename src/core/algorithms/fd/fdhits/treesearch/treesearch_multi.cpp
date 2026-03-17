#include "core/algorithms/fd/fdhits/treesearch/treesearch_multi.h"

#include <climits>
#include <optional>

#include "core/algorithms/fd/fdhits/treesearch/dynamic_edgemark_store_multi.h"
#include "core/algorithms/fd/fdhits/treesearch/node_type.h"

namespace algos::fd::fdhits {

static void SearchRecursiveFD(edges::MultiFD* s, DynamicEdgemarkStoreMulti* store,
                              Validator<edges::MultiFD>* target) {
    if (store->GetCurrentCand().GetRhs().IsEmpty()) return;

    target->Push(store->GetNodes());

    if (store->IsFullyCovered()) {
        s->SetRhs(store->GetCurrentCand().GetRhs());

        auto [cont, opt_edges] = target->Check(*s, store->GetNodes(), store->GetCand());
        if (opt_edges.has_value()) {
            store->Extend(*s, std::move(opt_edges.value()));
        }
        if (!cont) return;
    }

    edges::MultiFD const* best_uncov = nullptr;
    size_t best_overlap = SIZE_MAX;
    for (auto it = store->UncoveredEdgesBegin(); it != store->UncoveredEdgesEnd(); ++it) {
        size_t ov = it->Overlap(store->GetCurrentCand());
        if (!best_uncov || ov < best_overlap) {
            best_overlap = ov;
            best_uncov = &(*it);
        }
    }

    if (!best_uncov) return;

    edges::MultiFD c = best_uncov->Intersect(store->GetCurrentCand());

    if (!c.GetRhs().IsEmpty()) {
        if (store->AddRhs(c.GetRhs())) {
            SearchRecursiveFD(s, store, target);
            store->Pop();
        }
    }

    store->GetCurrentCandMut().RemoveAll(c);

    for (size_t lhs_node : c.GetLhs().GetNodes()) {
        if (store->AddLhs(lhs_node)) {
            s->AddLhs(lhs_node);

            SearchRecursiveFD(s, store, target);

            s->RemoveLhs(lhs_node);
            store->Pop();
        }
    }
}

void SearchMulti(Hypergraph<edges::MultiFD>* graph, Validator<edges::MultiFD>* target) {
    DynamicEdgemarkStoreMulti store(graph);

    edges::MultiFD s = edges::MultiFD::Empty(graph->GetVertexCount());

    SearchRecursiveFD(&s, &store, target);
}

}  // namespace algos::fd::fdhits
