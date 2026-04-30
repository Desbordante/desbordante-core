#pragma once

#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

#include <boost/dynamic_bitset.hpp>

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

using DBitset = boost::dynamic_bitset<>;

// Prefix trie for DBitset supporting subset queries and get-and-remove.
// Mirrors the Java NTreeSearch logic used in HEI-P.
class HEIBitSetTrie {
    struct Node {
        std::unordered_map<size_t, std::unique_ptr<Node>> children;
        std::optional<DBitset> stored;
    };

    Node root_;

    // Insert bs into trie following set bits >= 'from'
    static void DoInsert(Node& node, DBitset const& bs, size_t from) {
        size_t bit = (from == 0) ? bs.find_first() : bs.find_next(from - 1);
        if (bit == DBitset::npos) {
            node.stored = bs;
            return;
        }
        auto& child = node.children[bit];
        if (!child) child = std::make_unique<Node>();
        DoInsert(*child, bs, bit + 1);
    }

    // Returns true if this subtree is now empty
    static bool DoRemove(Node& node, DBitset const& bs, size_t from) {
        size_t bit = (from == 0) ? bs.find_first() : bs.find_next(from - 1);
        if (bit == DBitset::npos) {
            node.stored.reset();
            return node.children.empty();
        }
        auto it = node.children.find(bit);
        if (it == node.children.end()) return false;
        if (DoRemove(*it->second, bs, bit + 1)) {
            node.children.erase(it);
        }
        return !node.stored.has_value() && node.children.empty();
    }

    // Find any stored bitset that is a subset of 'query'
    // Follows only children whose key is a set bit of 'query'
    static bool DoContainsSubset(Node const& node, DBitset const& query, size_t from) {
        if (node.stored.has_value()) return true;
        // Iterate set bits of query >= from
        size_t bit = (from == 0) ? query.find_first() : query.find_next(from - 1);
        while (bit != DBitset::npos) {
            auto it = node.children.find(bit);
            if (it != node.children.end()) {
                if (DoContainsSubset(*it->second, query, bit + 1)) return true;
            }
            bit = query.find_next(bit);
        }
        return false;
    }

    // Get and remove all stored bitsets that are subsets of 'query'
    // Returns true if this subtree became empty
    static bool DoGetAndRemoveSubsets(Node& node, DBitset const& query, size_t from,
                                      std::vector<DBitset>& result) {
        if (node.stored.has_value()) {
            result.push_back(std::move(*node.stored));
            node.stored.reset();
        }
        size_t bit = (from == 0) ? query.find_first() : query.find_next(from - 1);
        while (bit != DBitset::npos) {
            auto it = node.children.find(bit);
            if (it != node.children.end()) {
                if (DoGetAndRemoveSubsets(*it->second, query, bit + 1, result)) {
                    node.children.erase(it);
                }
            }
            bit = query.find_next(bit);
        }
        return !node.stored.has_value() && node.children.empty();
    }

    static void DoForEach(Node const& node,
                          std::function<void(DBitset const&)> const& fn) {
        if (node.stored.has_value()) fn(*node.stored);
        for (auto const& [bit, child] : node.children) {
            DoForEach(*child, fn);
        }
    }

public:
    void Insert(DBitset const& bs) {
        DoInsert(root_, bs, 0);
    }

    bool ContainsSubset(DBitset const& query) const {
        return DoContainsSubset(root_, query, 0);
    }

    std::vector<DBitset> GetAndRemoveSubsets(DBitset const& query) {
        std::vector<DBitset> result;
        DoGetAndRemoveSubsets(root_, query, 0, result);
        return result;
    }

    void Remove(DBitset const& bs) {
        DoRemove(root_, bs, 0);
    }

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

    // Convert PredicateBitset (std::bitset<128>) to DBitset
    static DBitset ToDBitset(PredicateBitset const& pb, size_t n) {
        DBitset db(n);
        for (size_t i = pb._Find_first(); i < pb.size() && i < n; i = pb._Find_next(i)) {
            db.set(i);
        }
        return db;
    }

    // Keep only maximal evidences (supersets dominate — subsets are redundant).
    static std::vector<DBitset> Minimize(std::vector<DBitset> evs) {
        std::sort(evs.begin(), evs.end(), [](DBitset const& a, DBitset const& b) {
            if (a.count() != b.count()) return a.count() > b.count();
            return a > b;
        });

        std::vector<DBitset> result;
        result.reserve(evs.size());
        for (DBitset& ev : evs) {
            bool dominated = false;
            for (DBitset const& kept : result) {
                if (ev.is_subset_of(kept)) {
                    dominated = true;
                    break;
                }
            }
            if (!dominated) result.push_back(std::move(ev));
        }
        return result;
    }

    // Handle one invalid evidence: expand the pos_cover using predicate groups.
    // Mirrors TranslatingTreeSearch::handleInvalid from FDCD Java.
    static void HandleInvalid(HEIBitSetTrie& pos_cover, DBitset const& invalid_ev,
                               std::vector<DBitset> const& groups) {
        std::vector<DBitset> removed = pos_cover.GetAndRemoveSubsets(invalid_ev);

        for (DBitset const& removed_dc : removed) {
            for (DBitset const& group : groups) {
                // If removed_dc has no predicate from this group yet, extend it
                if ((removed_dc & group).none()) {
                    // Candidates: group members not already in invalid_ev
                    DBitset candidates = group & ~invalid_ev;
                    for (size_t i = candidates.find_first(); i != DBitset::npos;
                         i = candidates.find_next(i)) {
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

    // Inner Evidence Inversion (EI) search: find all minimal DCs that hit all evs,
    // constrained to pick at most one predicate per group.
    // Implements the EI algorithm from HyDRA (the inner step of HEI-P).
    static std::vector<DBitset> InnerSearch(std::vector<DBitset> const& evs,
                                            std::vector<DBitset> const& groups, size_t n) {
        if (evs.empty()) {
            // Nothing to hit — empty DC covers everything (trivially valid)
            return {DBitset(n)};
        }

        // Sort evidences: largest cardinality first (as in Java)
        std::vector<DBitset> sorted_evs = evs;
        std::sort(sorted_evs.begin(), sorted_evs.end(), [](DBitset const& a, DBitset const& b) {
            if (a.count() != b.count()) return a.count() > b.count();
            return a > b;
        });

        HEIBitSetTrie pos_cover;
        pos_cover.Insert(DBitset(n));

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
        // Step 1: Convert evidence set to dynamic bitsets and minimize
        std::vector<DBitset> all_evs;
        all_evs.reserve(evidence_set.Size());
        for (size_t i = 0; i < evidence_set.Size(); ++i) {
            all_evs.push_back(ToDBitset(evidence_set[i].evidence, n_predicates_));
        }
        all_evs = Minimize(all_evs);
        fprintf(stderr, "[HEI-P] Evidences after minimize: %zu\n", all_evs.size());

        // Step 2: Build pred2evi — for each predicate, IDs of evidences containing it
        size_t n_evs = all_evs.size();
        std::vector<DBitset> pred2evi(n_predicates_, DBitset(n_evs));
        for (size_t eid = 0; eid < n_evs; ++eid) {
            for (size_t pid = all_evs[eid].find_first(); pid != DBitset::npos;
                 pid = all_evs[eid].find_next(pid)) {
                pred2evi[pid].set(eid);
            }
        }

        // Step 3: Deduplicate predicate groups from mutex_map_
        std::vector<DBitset> unique_groups;
        std::vector<bool> seen(n_predicates_, false);
        for (size_t pid = 0; pid < n_predicates_; ++pid) {
            if (seen[pid]) continue;
            DBitset grp = ToDBitset(mutex_map_[pid], n_predicates_);
            for (size_t p = grp.find_first(); p != DBitset::npos; p = grp.find_next(p)) {
                seen[p] = true;
            }
            unique_groups.push_back(std::move(grp));
        }

        // Step 4: Sort predicates by ascending evidence frequency
        std::vector<size_t> sorted_preds(n_predicates_);
        std::iota(sorted_preds.begin(), sorted_preds.end(), 0);
        std::sort(sorted_preds.begin(), sorted_preds.end(), [&](size_t a, size_t b) {
            return pred2evi[a].count() < pred2evi[b].count();
        });

        // Step 5: Parallel outer loop
        std::vector<DBitset> all_covers_raw;
        std::mutex covers_mutex;

        auto process_one = [&](size_t i) {
            size_t pid = sorted_preds[i];

            if (pred2evi[pid].none()) {
                // pid appears in no evidence → {pid} alone is a valid DC
                DBitset dc(n_predicates_);
                dc.set(pid);
                std::lock_guard<std::mutex> lock(covers_mutex);
                all_covers_raw.push_back(std::move(dc));
                return;
            }

            // preds_to_remove = pid's group + all predicates earlier in sorted order
            DBitset pid_group = ToDBitset(mutex_map_[pid], n_predicates_);
            DBitset preds_to_remove = pid_group;
            for (size_t j = 0; j < i; ++j) {
                preds_to_remove.set(sorted_preds[j]);
            }

            // Modulo evidences: for each evidence containing pid, clear preds_to_remove
            std::vector<DBitset> modulo_evs;
            modulo_evs.reserve(pred2evi[pid].count());
            for (size_t eid = pred2evi[pid].find_first(); eid != DBitset::npos;
                 eid = pred2evi[pid].find_next(eid)) {
                DBitset ev = all_evs[eid];
                ev &= ~preds_to_remove;
                modulo_evs.push_back(std::move(ev));
            }

            // Deduplicate
            std::sort(modulo_evs.begin(), modulo_evs.end());
            modulo_evs.erase(std::unique(modulo_evs.begin(), modulo_evs.end()), modulo_evs.end());

            // Inner groups: unique_groups excluding pid's group, with preds_to_remove cleared
            std::vector<DBitset> inner_groups;
            inner_groups.reserve(unique_groups.size());
            for (DBitset const& grp : unique_groups) {
                if ((grp & pid_group).any()) continue;
                DBitset inner_grp = grp & ~preds_to_remove;
                if (inner_grp.any()) inner_groups.push_back(std::move(inner_grp));
            }

            std::vector<DBitset> partial_dcs = InnerSearch(modulo_evs, inner_groups, n_predicates_);

            std::lock_guard<std::mutex> lock(covers_mutex);
            for (DBitset& dc : partial_dcs) {
                dc.resize(n_predicates_);
                dc.set(pid);
                all_covers_raw.push_back(std::move(dc));
            }
        };

        if (threads_ <= 1) {
            for (size_t i = 0; i < n_predicates_; ++i) {
                process_one(i);
            }
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

        // Deduplicate: different pid iterations can produce the same DC
        // (Java uses ConcurrentHashMap.newKeySet() for this purpose)
        std::sort(all_covers_raw.begin(), all_covers_raw.end());
        all_covers_raw.erase(std::unique(all_covers_raw.begin(), all_covers_raw.end()),
                             all_covers_raw.end());

        fprintf(stderr, "[HEI-P] Raw covers after dedup: %zu\n", all_covers_raw.size());

        // Step 6: Minimality check — remove non-minimal covers
        HEIBitSetTrie nt;
        for (DBitset const& dc : all_covers_raw) {
            nt.Insert(dc);
        }

        std::vector<DenialConstraint> result;
        result.reserve(all_covers_raw.size());
        size_t non_minimal_count = 0;
        for (DBitset const& dc : all_covers_raw) {
            nt.Remove(dc);
            if (!nt.ContainsSubset(dc)) {
                result.emplace_back(dc, pred_index_provider_, schema_);
            } else {
                ++non_minimal_count;
            }
            nt.Insert(dc);
        }

        fprintf(stderr, "[HEI-P] Non-minimal removed: %zu, Final: %zu\n",
                non_minimal_count, result.size());

        // Validity check: every DC must not be a subset of any evidence
        size_t invalid_count = 0;
        for (DenialConstraint const& dc_obj : result) {
            DBitset const& dc_bs = dc_obj.GetPredicateSet().GetBitset();
            for (DBitset const& ev : all_evs) {
                // dc is invalid if dc ⊆ ev (DC would be violated by this tuple pair)
                if (dc_bs.is_subset_of(ev)) {
                    ++invalid_count;
                    break;
                }
            }
        }
        fprintf(stderr, "[HEI-P] Invalid DCs (subsets of some evidence): %zu\n", invalid_count);

        // Brute-force check on result: are any result DCs truly non-minimal?
        // Collect result as DBitsets for comparison
        std::vector<DBitset> result_bs;
        result_bs.reserve(result.size());
        for (DenialConstraint const& dc_obj : result) {
            result_bs.push_back(dc_obj.GetPredicateSet().GetBitset());
        }
        size_t brute_non_minimal = 0;
        size_t check_limit = std::min(result_bs.size(), size_t(1000));
        for (size_t x = 0; x < check_limit; ++x) {
            for (size_t y = 0; y < result_bs.size(); ++y) {
                if (x == y) continue;
                if (result_bs[y].is_subset_of(result_bs[x]) && result_bs[y] != result_bs[x]) {
                    ++brute_non_minimal;
                    if (brute_non_minimal <= 3) {
                        fprintf(stderr, "[HEI-P] Non-minimal in result: res[%zu] (bits=%zu) has subset res[%zu] (bits=%zu)\n",
                                x, result_bs[x].count(), y, result_bs[y].count());
                    }
                    break;
                }
            }
            if (brute_non_minimal >= 5) break;
        }
        fprintf(stderr, "[HEI-P] Brute-force non-minimal in result (checked first %zu): %zu\n",
                check_limit, brute_non_minimal);

        return result;
    }
};

}  // namespace algos::dc
