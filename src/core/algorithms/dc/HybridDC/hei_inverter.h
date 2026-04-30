#pragma once

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <thread>
#include <vector>

#include "core/algorithms/dc/FastADC/model/denial_constraint.h"
#include "core/algorithms/dc/FastADC/model/evidence_set.h"
#include "core/algorithms/dc/FastADC/model/predicate.h"
#include "core/algorithms/dc/FastADC/providers/index_provider.h"
#include "core/model/table/relational_schema.h"

namespace algos::dc {

using fastadc::DenialConstraint;
using fastadc::EvidenceSet;
using fastadc::PredicateBitset;
using fastadc::PredicateIndexProvider;

// Indices guaranteed < 128 (BuildMask throws otherwise), so PredicateBitset fits.
using DBitset = PredicateBitset;

// _Find_first()/_Find_next() return 128 (size()) when no more bits exist.
static constexpr size_t kNpos = 128;

// Strict weak ordering on DBitset: lexicographic by set-bit positions ascending.
static bool BsLess(DBitset const& a, DBitset const& b) noexcept {
    size_t ai = a._Find_first(), bi = b._Find_first();
    while (ai < kNpos && bi < kNpos) {
        if (ai != bi) return ai < bi;
        ai = a._Find_next(ai);
        bi = b._Find_next(bi);
    }
    return ai >= kNpos && bi < kNpos;  // a exhausted first, so a is "less"
}

// Prefix trie for DBitset supporting subset queries and get-and-remove.
// Mirrors the Java NTreeSearch logic used in HEI-P.
// Children stored as a sorted vector for cache-friendly lookup (no hash overhead).
class HEIBitSetTrie {
    struct Node {
        std::vector<std::pair<uint8_t, std::unique_ptr<Node>>> children;
        std::optional<DBitset> stored;
    };

    Node root_;

    static Node& GetOrCreateChild(Node& node, size_t bit) {
        auto it = std::lower_bound(node.children.begin(), node.children.end(), uint8_t(bit),
                                   [](auto const& p, uint8_t b) { return p.first < b; });
        if (it != node.children.end() && it->first == uint8_t(bit)) return *it->second;
        return *node.children.emplace(it, uint8_t(bit), std::make_unique<Node>())->second;
    }

    static void DoInsert(Node& node, DBitset const& bs, size_t from) {
        size_t bit = (from == 0) ? bs._Find_first() : bs._Find_next(from - 1);
        if (bit >= kNpos) {
            node.stored = bs;
            return;
        }
        DoInsert(GetOrCreateChild(node, bit), bs, bit + 1);
    }

    static bool DoRemove(Node& node, DBitset const& bs, size_t from) {
        size_t bit = (from == 0) ? bs._Find_first() : bs._Find_next(from - 1);
        if (bit >= kNpos) {
            node.stored.reset();
            return node.children.empty();
        }
        auto it = std::lower_bound(node.children.begin(), node.children.end(), uint8_t(bit),
                                   [](auto const& p, uint8_t b) { return p.first < b; });
        if (it == node.children.end() || it->first != uint8_t(bit)) return false;
        if (DoRemove(*it->second, bs, bit + 1)) node.children.erase(it);
        return !node.stored.has_value() && node.children.empty();
    }

    // Merge-iterate query bits and sorted children: only recurse into children
    // whose bit is actually set in the query (avoids scanning non-matching children).
    static bool DoContainsSubset(Node const& node, DBitset const& query, size_t from) {
        if (node.stored.has_value()) return true;

        size_t qbit = (from == 0) ? query._Find_first() : query._Find_next(from - 1);
        size_t ci = 0, n = node.children.size();

        while (qbit < kNpos && ci < n) {
            size_t cbit = node.children[ci].first;
            if (qbit == cbit) {
                if (DoContainsSubset(*node.children[ci].second, query, qbit + 1)) return true;
                qbit = query._Find_next(qbit);
                ++ci;
            } else if (qbit < cbit) {
                qbit = query._Find_next(cbit - 1);
            } else {
                ++ci;
            }
        }
        return false;
    }

    static bool DoGetAndRemoveSubsets(Node& node, DBitset const& query, size_t from,
                                      std::vector<DBitset>& result) {
        if (node.stored.has_value()) {
            result.push_back(std::move(*node.stored));
            node.stored.reset();
        }

        size_t qbit = (from == 0) ? query._Find_first() : query._Find_next(from - 1);
        size_t ci = 0;

        while (qbit < kNpos && ci < node.children.size()) {
            size_t cbit = node.children[ci].first;
            if (qbit == cbit) {
                if (DoGetAndRemoveSubsets(*node.children[ci].second, query, qbit + 1, result)) {
                    node.children.erase(node.children.begin() + ci);
                } else {
                    ++ci;
                }
                qbit = query._Find_next(qbit);
            } else if (qbit < cbit) {
                qbit = query._Find_next(cbit - 1);
            } else {
                ++ci;
            }
        }
        return !node.stored.has_value() && node.children.empty();
    }

    static void DoForEach(Node const& node,
                          std::function<void(DBitset const&)> const& fn) {
        if (node.stored.has_value()) fn(*node.stored);
        for (auto const& [bit, child] : node.children) DoForEach(*child, fn);
    }

public:
    void Insert(DBitset const& bs) { DoInsert(root_, bs, 0); }

    bool ContainsSubset(DBitset const& query) const {
        return DoContainsSubset(root_, query, 0);
    }

    std::vector<DBitset> GetAndRemoveSubsets(DBitset const& query) {
        std::vector<DBitset> result;
        DoGetAndRemoveSubsets(root_, query, 0, result);
        return result;
    }

    void Remove(DBitset const& bs) { DoRemove(root_, bs, 0); }

    void ForEach(std::function<void(DBitset const&)> const& fn) const {
        DoForEach(root_, fn);
    }
};

// HEI-P: find all minimal DCs from an evidence set exactly.
// Decomposes per-predicate (INCS), then runs inner EI on each sub-problem.
class HEIInverter {
    size_t n_predicates_;
    std::vector<PredicateBitset> const& mutex_map_;
    std::shared_ptr<PredicateIndexProvider> pred_index_provider_;
    std::shared_ptr<RelationalSchema const> schema_;
    unsigned threads_;

    static DBitset ToDBitset(PredicateBitset const& pb, size_t n) {
        DBitset db;
        for (size_t i = pb._Find_first(); i < pb.size() && i < n; i = pb._Find_next(i)) {
            db.set(i);
        }
        return db;
    }

    // Convert to boost::dynamic_bitset for DenialConstraint construction (not in hot path).
    static boost::dynamic_bitset<> ToBoostBitset(DBitset const& bs) {
        boost::dynamic_bitset<> bdb(bs.size());
        for (size_t i = bs._Find_first(); i < kNpos; i = bs._Find_next(i)) bdb.set(i);
        return bdb;
    }

    // Keep only maximal evidences (supersets dominate — subsets are redundant).
    static std::vector<DBitset> Minimize(std::vector<DBitset> evs) {
        std::sort(evs.begin(), evs.end(), [](DBitset const& a, DBitset const& b) {
            if (a.count() != b.count()) return a.count() > b.count();
            return BsLess(b, a);
        });

        std::vector<DBitset> result;
        result.reserve(evs.size());
        for (DBitset& ev : evs) {
            bool dominated = false;
            for (DBitset const& kept : result) {
                if ((ev & ~kept).none()) {
                    dominated = true;
                    break;
                }
            }
            if (!dominated) result.push_back(std::move(ev));
        }
        return result;
    }

    static void HandleInvalid(HEIBitSetTrie& pos_cover, DBitset const& invalid_ev,
                               std::vector<DBitset> const& groups) {
        std::vector<DBitset> removed = pos_cover.GetAndRemoveSubsets(invalid_ev);

        for (DBitset const& removed_dc : removed) {
            for (DBitset const& group : groups) {
                if ((removed_dc & group).none()) {
                    DBitset candidates = group & ~invalid_ev;
                    for (size_t i = candidates._Find_first(); i < kNpos;
                         i = candidates._Find_next(i)) {
                        DBitset candidate_dc = removed_dc;
                        candidate_dc.set(i);
                        if (!pos_cover.ContainsSubset(candidate_dc)) {
                            pos_cover.Insert(candidate_dc);
                        }
                    }
                }
            }
        }
    }

    static std::vector<DBitset> InnerSearch(std::vector<DBitset> const& evs,
                                            std::vector<DBitset> const& groups) {
        if (evs.empty()) return {DBitset{}};

        std::vector<DBitset> sorted_evs = evs;
        std::sort(sorted_evs.begin(), sorted_evs.end(), [](DBitset const& a, DBitset const& b) {
            if (a.count() != b.count()) return a.count() > b.count();
            return BsLess(b, a);
        });

        HEIBitSetTrie pos_cover;
        pos_cover.Insert(DBitset{});

        for (DBitset const& ev : sorted_evs) {
            HandleInvalid(pos_cover, ev, groups);
        }

        std::vector<DBitset> result;
        pos_cover.ForEach([&result](DBitset const& bs) { result.push_back(bs); });
        return result;
    }

public:
    HEIInverter(size_t n_predicates, std::vector<PredicateBitset> const& mutex_map,
                std::shared_ptr<PredicateIndexProvider> pred_index_provider,
                std::shared_ptr<RelationalSchema const> schema, unsigned threads)
        : n_predicates_(n_predicates),
          mutex_map_(mutex_map),
          pred_index_provider_(std::move(pred_index_provider)),
          schema_(std::move(schema)),
          threads_(threads) {}

    std::vector<DenialConstraint> Run(EvidenceSet const& evidence_set) {
        FILE* log = nullptr;
        if (const char* path = std::getenv("ITER_LOG")) {
            log = fopen(path, "w");
        }

        std::vector<DBitset> all_evs;
        all_evs.reserve(evidence_set.Size());
        for (size_t i = 0; i < evidence_set.Size(); ++i) {
            all_evs.push_back(ToDBitset(evidence_set[i].evidence, n_predicates_));
        }
        all_evs = Minimize(all_evs);

        size_t n_evs = all_evs.size();
        // List, not bitset — evidence IDs are not bounded by 128.
        std::vector<std::vector<size_t>> pred2evi(n_predicates_);
        for (size_t eid = 0; eid < n_evs; ++eid) {
            for (size_t pid = all_evs[eid]._Find_first(); pid < kNpos;
                 pid = all_evs[eid]._Find_next(pid)) {
                pred2evi[pid].push_back(eid);
            }
        }

        std::vector<DBitset> unique_groups;
        std::vector<bool> seen(n_predicates_, false);
        for (size_t pid = 0; pid < n_predicates_; ++pid) {
            if (seen[pid]) continue;
            DBitset grp = ToDBitset(mutex_map_[pid], n_predicates_);
            for (size_t p = grp._Find_first(); p < kNpos; p = grp._Find_next(p)) seen[p] = true;
            unique_groups.push_back(std::move(grp));
        }

        auto const& pred_objects = pred_index_provider_->GetObjects();

        std::vector<size_t> sorted_preds(n_predicates_);
        std::iota(sorted_preds.begin(), sorted_preds.end(), 0);
        std::sort(sorted_preds.begin(), sorted_preds.end(), [&](size_t a, size_t b) {
            size_t ea = pred2evi[a].size(), eb = pred2evi[b].size();
            if (ea != eb) return ea < eb;
            // Same evidence count: single-column before cross-column (matches Java fdcd order).
            bool ca = pred_objects[a]->IsCrossColumn();
            bool cb = pred_objects[b]->IsCrossColumn();
            if (ca != cb) return !ca;
            return a < b;
        });

        if (log) {
            fprintf(log, "SORTED_PREDS:");
            for (size_t i = 0; i < n_predicates_; ++i)
                fprintf(log, " %zu", sorted_preds[i]);
            fprintf(log, "\n");
        }

        std::vector<DBitset> all_covers_raw;
        std::mutex covers_mutex;
        auto process_one = [&](size_t i) {
            size_t pid = sorted_preds[i];

            if (pred2evi[pid].empty()) {
                DBitset dc;
                dc.set(pid);
                std::lock_guard<std::mutex> lock(covers_mutex);
                if (log) fprintf(log, "ITER %zu pid=%zu evi=0 modulo=0 inner_grps=0 partial=1\n", i, pid);
                all_covers_raw.push_back(std::move(dc));
                return;
            }

            DBitset pid_group = ToDBitset(mutex_map_[pid], n_predicates_);
            DBitset preds_to_remove = pid_group;
            for (size_t j = 0; j < i; ++j) preds_to_remove.set(sorted_preds[j]);

            std::vector<DBitset> modulo_evs;
            modulo_evs.reserve(pred2evi[pid].size());
            for (size_t eid : pred2evi[pid]) {
                DBitset ev = all_evs[eid];
                ev &= ~preds_to_remove;
                modulo_evs.push_back(std::move(ev));
            }

            std::sort(modulo_evs.begin(), modulo_evs.end(), BsLess);
            modulo_evs.erase(std::unique(modulo_evs.begin(), modulo_evs.end()), modulo_evs.end());

            std::vector<DBitset> inner_groups;
            inner_groups.reserve(unique_groups.size());
            for (DBitset const& grp : unique_groups) {
                if ((grp & pid_group).any()) continue;
                // Inner EI uses only single-column groups (matches Java fdcd behavior).
                size_t first = grp.find_first();
                if (first != DBitset::npos && pred_objects[first]->IsCrossColumn()) continue;
                DBitset inner_grp = grp & ~preds_to_remove;
                if (inner_grp.any()) inner_groups.push_back(std::move(inner_grp));
            }

            std::vector<DBitset> partial_dcs = InnerSearch(modulo_evs, inner_groups);

            std::lock_guard<std::mutex> lock(covers_mutex);
            if (log) {
                fprintf(log, "ITER %zu pid=%zu evi=%zu modulo=%zu inner_grps=%zu partial=%zu\n",
                        i, pid,
                        pred2evi[pid].size(),
                        modulo_evs.size(),
                        inner_groups.size(),
                        partial_dcs.size());
            }
            for (DBitset& dc : partial_dcs) {
                dc.set(pid);
                all_covers_raw.push_back(std::move(dc));
            }
        };

        if (log || threads_ <= 1) {
            for (size_t i = 0; i < n_predicates_; ++i) process_one(i);
        } else {
            std::atomic<size_t> next_idx{0};
            std::vector<std::thread> worker_threads;
            worker_threads.reserve(threads_);
            for (unsigned t = 0; t < threads_; ++t) {
                worker_threads.emplace_back([&]() {
                    while (true) {
                        size_t i = next_idx.fetch_add(1, std::memory_order_relaxed);
                        if (i >= n_predicates_) break;
                        process_one(i);
                    }
                });
            }
            for (auto& th : worker_threads) th.join();
        }

        std::sort(all_covers_raw.begin(), all_covers_raw.end(), BsLess);
        all_covers_raw.erase(std::unique(all_covers_raw.begin(), all_covers_raw.end()),
                             all_covers_raw.end());

        HEIBitSetTrie nt;
        for (DBitset const& dc : all_covers_raw) nt.Insert(dc);

        std::vector<DenialConstraint> result;
        result.reserve(all_covers_raw.size());
        size_t non_minimal_count = 0;
        for (DBitset const& dc : all_covers_raw) {
            nt.Remove(dc);
            if (!nt.ContainsSubset(dc)) {
                result.emplace_back(ToBoostBitset(dc), pred_index_provider_, schema_);
            } else {
                ++non_minimal_count;
            }
            nt.Insert(dc);
        }

        if (log) {
            fprintf(log, "FINAL raw=%zu non_minimal=%zu result=%zu\n",
                    all_covers_raw.size(), non_minimal_count, result.size());
            fclose(log);
        }

        return result;
    }
};

}  // namespace algos::dc
