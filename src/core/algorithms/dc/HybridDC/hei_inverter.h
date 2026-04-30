#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

#include <boost/container/small_vector.hpp>

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

// Lex order by set-bit positions ascending. Must match Java's IBitSet.compareTo —
// changing it shifts which DCs get pruned and how large pos_cover grows.
// Don't replace with integer compare.
static bool BsLess(DBitset const& a, DBitset const& b) noexcept {
    size_t ai = a._Find_first(), bi = b._Find_first();
    while (ai < kNpos && bi < kNpos) {
        if (ai != bi) return ai < bi;
        ai = a._Find_next(ai);
        bi = b._Find_next(bi);
    }
    return ai >= kNpos && bi < kNpos;  // a exhausted first, so a is "less"
}

// Fast total order for dedup — only consistency matters, not which specific order.
// libstdc++ bitset<128> stores bits 0..63 in word 0 and 64..127 in word 1, so
// memcpy into __int128 gives a valid integer compare. Two compares vs bit-scan.
static bool BsLessRaw(DBitset const& a, DBitset const& b) noexcept {
    unsigned __int128 av, bv;
    std::memcpy(&av, &a, sizeof(av));
    std::memcpy(&bv, &b, sizeof(bv));
    return av < bv;
}

// Prefix trie over DBitset. Supports subset queries and get-and-remove-subsets.
// Based on Java NTreeSearch from HEI-P.
//
// Nodes stored in a flat arena vector, referenced by index. unique_ptr layout
// caused one cache miss per step; on Adult that was ~34% of CPU. Arena keeps
// nodes contiguous so traversal is cache-friendly.
//
// Children are sorted by bit for binary search and merge iteration against the
// query. Pruned nodes stay in the arena (trie is short-lived, no freelist).
class HEIBitSetTrie {
    static constexpr uint32_t kRoot = 0;

    // Profiling: heap-allocated children caused ~16% of inversion CPU in cache misses.
    // Most nodes have few children, so inline storage lands them on the same cache line.
    static constexpr size_t kInlineChildren = 4;
    using ChildrenT = boost::container::small_vector<std::pair<uint8_t, uint32_t>,
                                                     kInlineChildren>;

    // Node fits in one 64-byte cache line. small_vector<4> is 56 bytes plus the
    // 1-byte flag = 64 bytes. DBitset lives in a parallel vector (cold path only).
    struct Node {
        bool has_stored;
        ChildrenT children;
    };

    std::vector<Node> nodes_;
    std::vector<DBitset> stored_values_;  // parallel to nodes_; valid where has_stored

    uint32_t NewNode() {
        nodes_.emplace_back();
        stored_values_.emplace_back();
        return static_cast<uint32_t>(nodes_.size() - 1);
    }

    // Get or create child for `bit` under `parent_idx`. NewNode() may reallocate
    // `nodes_`, so save insertion position before allocating, then re-acquire.
    uint32_t GetOrCreateChild(uint32_t parent_idx, size_t bit) {
        {
            auto& children = nodes_[parent_idx].children;
            auto it = std::lower_bound(children.begin(), children.end(), uint8_t(bit),
                                       [](auto const& p, uint8_t b) { return p.first < b; });
            if (it != children.end() && it->first == uint8_t(bit)) return it->second;
        }
        auto& children0 = nodes_[parent_idx].children;
        auto it = std::lower_bound(children0.begin(), children0.end(), uint8_t(bit),
                                   [](auto const& p, uint8_t b) { return p.first < b; });
        size_t pos = static_cast<size_t>(it - children0.begin());
        uint32_t new_idx = NewNode();
        auto& children = nodes_[parent_idx].children;
        children.insert(children.begin() + pos, {uint8_t(bit), new_idx});
        return new_idx;
    }

    void DoInsert(uint32_t node_idx, DBitset const& bs, size_t from) {
        size_t bit = (from == 0) ? bs._Find_first() : bs._Find_next(from - 1);
        if (bit >= kNpos) {
            nodes_[node_idx].has_stored = true;
            stored_values_[node_idx] = bs;
            return;
        }
        uint32_t child_idx = GetOrCreateChild(node_idx, bit);
        DoInsert(child_idx, bs, bit + 1);
    }

    bool DoRemove(uint32_t node_idx, DBitset const& bs, size_t from) {
        size_t bit = (from == 0) ? bs._Find_first() : bs._Find_next(from - 1);
        if (bit >= kNpos) {
            nodes_[node_idx].has_stored = false;
            return nodes_[node_idx].children.empty();
        }
        auto& children = nodes_[node_idx].children;
        auto it = std::lower_bound(children.begin(), children.end(), uint8_t(bit),
                                   [](auto const& p, uint8_t b) { return p.first < b; });
        if (it == children.end() || it->first != uint8_t(bit)) return false;
        size_t pos = static_cast<size_t>(it - children.begin());
        uint32_t child_idx = it->second;
        bool collapse = DoRemove(child_idx, bs, bit + 1);
        if (collapse) {
            auto& children2 = nodes_[node_idx].children;
            children2.erase(children2.begin() + pos);
        }
        Node const& n = nodes_[node_idx];
        return !n.has_stored && n.children.empty();
    }

    bool DoSubtreeHasAnyStored(uint32_t node_idx) const {
        Node const& node = nodes_[node_idx];
        if (node.has_stored) return true;
        for (auto const& [bit, child_idx] : node.children) {
            (void)bit;
            if (DoSubtreeHasAnyStored(child_idx)) return true;
        }
        return false;
    }

    // Find a stored bitset that is a superset of q_bits[q_idx..q_count).
    // Children are sorted, so stop when child bit exceeds the next query bit.
    bool DoContainsSuperset(uint32_t node_idx, std::array<uint8_t, kNpos> const& q_bits,
                            size_t q_idx, size_t q_count) const {
        Node const& node = nodes_[node_idx];

        if (q_idx == q_count) {
            // All query bits consumed — any stored node is a superset.
            if (node.stored.has_value()) return true;
            for (auto const& [bit, child_idx] : node.children) {
                (void)bit;
                if (DoSubtreeHasAnyStored(child_idx)) return true;
            }
            return false;
        }

        uint8_t next_q = q_bits[q_idx];
        for (auto const& [bit, child_idx] : node.children) {
            if (bit < next_q) {
                // Extra bit — keep looking for next_q deeper.
                if (DoContainsSuperset(child_idx, q_bits, q_idx, q_count)) return true;
            } else if (bit == next_q) {
                if (DoContainsSuperset(child_idx, q_bits, q_idx + 1, q_count)) return true;
            } else {
                // Children are sorted, so no later child can match next_q.
                break;
            }
        }
        return false;
    }

    bool DoContainsSubset(uint32_t node_idx, DBitset const& query, size_t from) const {
        Node const& node = nodes_[node_idx];
        if (node.has_stored) return true;

        size_t qbit = (from == 0) ? query._Find_first() : query._Find_next(from - 1);
        size_t ci = 0, n = node.children.size();
        auto const* children = node.children.data();

        while (qbit < kNpos && ci < n) {
            size_t cbit = children[ci].first;
            if (qbit == cbit) {
                if (DoContainsSubset(children[ci].second, query, qbit + 1)) return true;
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

    bool DoGetAndRemoveSubsets(uint32_t node_idx, DBitset const& query, size_t from,
                               std::vector<DBitset>& result) {
        if (nodes_[node_idx].has_stored) {
            result.push_back(std::move(stored_values_[node_idx]));
            nodes_[node_idx].has_stored = false;
        }

        size_t qbit = (from == 0) ? query._Find_first() : query._Find_next(from - 1);
        size_t ci = 0;

        while (qbit < kNpos && ci < nodes_[node_idx].children.size()) {
            size_t cbit = nodes_[node_idx].children[ci].first;
            if (qbit == cbit) {
                uint32_t child_idx = nodes_[node_idx].children[ci].second;
                if (DoGetAndRemoveSubsets(child_idx, query, qbit + 1, result)) {
                    auto& children = nodes_[node_idx].children;
                    children.erase(children.begin() + ci);
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
        Node const& n = nodes_[node_idx];
        return !n.has_stored && n.children.empty();
    }

    void DoForEach(uint32_t node_idx, std::function<void(DBitset const&)> const& fn) const {
        Node const& node = nodes_[node_idx];
        if (node.has_stored) fn(stored_values_[node_idx]);
        for (auto const& [bit, child_idx] : node.children) {
            (void)bit;
            DoForEach(child_idx, fn);
        }
    }

public:
    HEIBitSetTrie() {
        nodes_.emplace_back();
        stored_values_.emplace_back();
    }

    void Insert(DBitset const& bs) { DoInsert(kRoot, bs, 0); }

    bool ContainsSubset(DBitset const& query) const {
        return DoContainsSubset(kRoot, query, 0);
    }

    // True iff some stored bitset is a (non-strict) superset of `query`.
    bool ContainsSuperset(DBitset const& query) const {
        std::array<uint8_t, kNpos> q_bits;
        size_t q_count = 0;
        for (size_t b = query._Find_first(); b < kNpos; b = query._Find_next(b)) {
            q_bits[q_count++] = static_cast<uint8_t>(b);
        }
        return DoContainsSuperset(kRoot, q_bits, 0, q_count);
    }

    std::vector<DBitset> GetAndRemoveSubsets(DBitset const& query) {
        std::vector<DBitset> result;
        DoGetAndRemoveSubsets(kRoot, query, 0, result);
        return result;
    }

    void Remove(DBitset const& bs) { DoRemove(kRoot, bs, 0); }

    void ForEach(std::function<void(DBitset const&)> const& fn) const {
        DoForEach(kRoot, fn);
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

    // Keep only maximal evidences; subsets are dominated and redundant.
    // Brute-force O(N^2) was ~15% of CPU on Adult. Trie reduces to O(N * depth).
    static std::vector<DBitset> Minimize(std::vector<DBitset> evs) {
        std::sort(evs.begin(), evs.end(), [](DBitset const& a, DBitset const& b) {
            if (a.count() != b.count()) return a.count() > b.count();
            return BsLessRaw(a, b);  // any total order works here, not the lex one
        });

        HEIBitSetTrie kept_trie;
        std::vector<DBitset> result;
        result.reserve(evs.size());
        for (DBitset& ev : evs) {
            // After sort, all kept items have cardinality >= ev, so a superset dominates.
            if (kept_trie.ContainsSuperset(ev)) continue;
            kept_trie.Insert(ev);
            result.push_back(std::move(ev));
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
            // Compiler can't prove ~preds_to_remove is loop-invariant (std::bitset<128>),
            // so hoist it. Was ~5% of CPU on Adult from repeated _M_do_flip calls.
            DBitset const keep_mask = ~preds_to_remove;

            std::vector<DBitset> modulo_evs;
            modulo_evs.reserve(pred2evi[pid].size());
            for (size_t eid : pred2evi[pid]) {
                DBitset ev = all_evs[eid];
                ev &= keep_mask;
                modulo_evs.push_back(std::move(ev));
            }

            // Dedup needs only a total order. InnerSearch re-sorts before use,
            // so use the fast __int128 compare.
            std::sort(modulo_evs.begin(), modulo_evs.end(), BsLessRaw);
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

        // Dedup, then sort by cardinality ASC. Process one cardinality level at a time:
        // trie holds only smaller DCs, so same-level checks are read-only and parallel
        // (two equal-size DCs can't be subsets of each other).
        std::sort(all_covers_raw.begin(), all_covers_raw.end(), BsLessRaw);
        all_covers_raw.erase(std::unique(all_covers_raw.begin(), all_covers_raw.end()),
                             all_covers_raw.end());
        std::sort(all_covers_raw.begin(), all_covers_raw.end(),
                  [](DBitset const& a, DBitset const& b) {
                      size_t ca = a.count(), cb = b.count();
                      if (ca != cb) return ca < cb;
                      return BsLessRaw(a, b);
                  });

        HEIBitSetTrie nt;
        std::vector<DenialConstraint> result;
        result.reserve(all_covers_raw.size());
        size_t non_minimal_count = 0;

        size_t const total = all_covers_raw.size();
        size_t idx = 0;
        std::vector<uint8_t> is_minimal;  // reused across levels
        while (idx < total) {
            size_t card = all_covers_raw[idx].count();
            size_t end = idx;
            while (end < total && all_covers_raw[end].count() == card) ++end;
            size_t batch = end - idx;
            is_minimal.assign(batch, 0);

            // Threshold: parallelism overhead dominates for very small batches.
            if (threads_ > 1 && batch >= 64 && !log) {
                std::atomic<size_t> next{idx};
                std::vector<std::thread> workers;
                workers.reserve(threads_);
                for (unsigned t = 0; t < threads_; ++t) {
                    workers.emplace_back([&]() {
                        while (true) {
                            size_t i = next.fetch_add(1, std::memory_order_relaxed);
                            if (i >= end) break;
                            is_minimal[i - idx] = !nt.ContainsSubset(all_covers_raw[i]);
                        }
                    });
                }
                for (auto& w : workers) w.join();
            } else {
                for (size_t i = idx; i < end; ++i) {
                    is_minimal[i - idx] = !nt.ContainsSubset(all_covers_raw[i]);
                }
            }

            // Serial: trie writes can't race with the next level's reads.
            for (size_t i = idx; i < end; ++i) {
                if (is_minimal[i - idx]) {
                    nt.Insert(all_covers_raw[i]);
                    result.emplace_back(ToBoostBitset(all_covers_raw[i]),
                                        pred_index_provider_, schema_);
                } else {
                    ++non_minimal_count;
                }
            }
            idx = end;
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
