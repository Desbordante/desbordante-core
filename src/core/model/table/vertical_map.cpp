#include "core/model/table/vertical_map.h"

#include <exception>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <unordered_set>

#include "core/algorithms/fd/pyrocommon/core/dependency_candidate.h"
#include "core/algorithms/fd/pyrocommon/core/vertical_info.h"
#include "core/algorithms/fd/pyrocommon/model/agree_set_sample.h"
#include "core/model/table/position_list_index.h"

namespace model {

template <class Value>
std::shared_ptr<Value> VerticalMap<Value>::SetTrie::Associate(Bitset const& key, size_t next_bit,
                                                              std::shared_ptr<Value> value) {
    next_bit = (next_bit == 0 ? key.find_first() : key.find_next(next_bit - 1));
    if (next_bit == boost::dynamic_bitset<>::npos) {
        std::swap(value, value_);
        return value;
    }
    return GetOrCreateSubTrie(next_bit)->Associate(key, next_bit + 1, std::move(value));
}

template <class Value>
std::shared_ptr<Value const> VerticalMap<Value>::SetTrie::Get(Bitset const& key,
                                                              size_t next_bit) const {
    next_bit = (next_bit == 0 ? key.find_first() : key.find_next(next_bit - 1));
    if (next_bit == boost::dynamic_bitset<>::npos) {
        return value_;
    }

    auto subtrie = GetSubtrie(next_bit);
    if (subtrie == nullptr) return nullptr;
    return subtrie->Get(key, next_bit + 1);
}

template <class Value>
std::shared_ptr<Value> VerticalMap<Value>::SetTrie::Remove(Bitset const& key, size_t next_bit) {
    next_bit = (next_bit == 0 ? key.find_first() : key.find_next(next_bit - 1));
    if (next_bit == Bitset::npos) {
        auto removed_value = value_;
        value_ = nullptr;
        return removed_value;
    }

    auto subtrie = GetSubtrie(next_bit);
    if (subtrie == nullptr) return nullptr;
    auto removed_value = subtrie->Remove(key, next_bit + 1);
    if (subtrie->IsEmpty()) {
        subtries_[next_bit - offset_] = nullptr;
    }

    return removed_value;
}

template <class Value>
bool VerticalMap<Value>::SetTrie::IsEmpty() const {
    if (value_ == nullptr) return false;
    return std::all_of(subtries_.begin(), subtries_.end(),
                       [](auto& subtrie_ptr) { return subtrie_ptr == nullptr; });
}

template <class Value>
typename VerticalMap<Value>::SetTrie* VerticalMap<Value>::SetTrie::GetOrCreateSubTrie(
        size_t index) {
    if (subtries_.empty()) {
        subtries_ = std::vector<std::unique_ptr<SetTrie>>(dimension_ - offset_);
    }
    SetTrie* subtrie = GetSubtrie(index);
    if (subtrie == nullptr) {
        subtries_[index - offset_] = std::make_unique<SetTrie>(index + 1, dimension_);
    }
    return subtries_[index - offset_].get();
}

template <class Value>
typename VerticalMap<Value>::SetTrie* VerticalMap<Value>::SetTrie::GetSubtrie(size_t index) {
    return const_cast<VerticalMap<Value>::SetTrie*>(
            (const_cast<VerticalMap<Value>::SetTrie const*>(this))->GetSubtrie(index));
}

template <class Value>
typename VerticalMap<Value>::SetTrie const* VerticalMap<Value>::SetTrie::GetSubtrie(
        size_t index) const {
    if (subtries_.empty()) {
        return nullptr;
    }
    if (index < offset_ || index >= dimension_) {
        throw std::runtime_error("Error in GetSubtrie: index must be in [offset_; dimension_)");
    }
    return subtries_[index - offset_].get();
}

template <class Value>
void VerticalMap<Value>::SetTrie::TraverseEntries(
        Bitset& subset_key,
        std::function<void(Bitset const&, std::shared_ptr<Value const>)> collector) const {
    if (value_ != nullptr) {
        collector(Bitset(subset_key), value_);
    }
    for (size_t i = offset_; i < dimension_; i++) {
        auto subtrie = GetSubtrie(i);
        if (subtrie != nullptr) {
            subset_key.set(i);
            subtrie->TraverseEntries(subset_key, collector);
            subset_key.reset(i);
        }
    }
}

template <class Value>
bool VerticalMap<Value>::SetTrie::CollectSubsetKeys(
        Bitset const& key, size_t next_bit, Bitset& subset_key,
        std::function<bool(Bitset const&, std::shared_ptr<Value const>)> const& collector) const {
    if (value_ != nullptr) {
        if (!collector(Bitset(subset_key), value_)) return false;
    }

    for (next_bit = (next_bit == 0 ? key.find_first() : key.find_next(next_bit - 1));
         next_bit != Bitset::npos; next_bit = key.find_next(next_bit)) {
        auto subtrie = GetSubtrie(next_bit);
        if (subtrie != nullptr) {
            subset_key.set(next_bit);
            if (!subtrie->CollectSubsetKeys(key, next_bit + 1, subset_key, collector)) return false;
            subset_key.reset(next_bit);
        }
    }
    return true;
}

template <class Value>
bool VerticalMap<Value>::SetTrie::CollectSupersetKeys(
        Bitset const& key, size_t next_bit, Bitset& superset_key,
        std::function<bool(Bitset const&, std::shared_ptr<Value const>)> const& collector) const {
    if (next_bit != Bitset::npos) {
        next_bit = (next_bit == 0 ? key.find_first() : key.find_next(next_bit - 1));
    }
    if (next_bit == Bitset::npos) {
        if (value_ != nullptr) {
            if (!collector(Bitset(superset_key), value_)) return false;
        }
        for (size_t i = offset_; i < dimension_; i++) {
            auto subtrie = GetSubtrie(i);
            if (subtrie != nullptr) {
                superset_key.set(i);
                if (!subtrie->CollectSupersetKeys(key, next_bit, superset_key, collector))
                    return false;
                superset_key.reset(i);
            }
        }
    } else {
        for (size_t i = offset_; i < next_bit; i++) {
            auto subtrie = GetSubtrie(i);
            if (subtrie != nullptr) {
                superset_key.set(i);
                if (!subtrie->CollectSupersetKeys(key, next_bit, superset_key, collector))
                    return false;
                superset_key.reset(i);
            }
        }

        auto subtrie = GetSubtrie(next_bit);
        if (subtrie != nullptr) {
            superset_key.set(next_bit);
            if (!subtrie->CollectSupersetKeys(key, next_bit + 1, superset_key, collector))
                return false;
            superset_key.reset(next_bit);
        }
    }
    return true;
}

template <class Value>
bool VerticalMap<Value>::SetTrie::CollectRestrictedSupersetKeys(
        Bitset const& key, Bitset const& blacklist, size_t next_bit, Bitset& superset_key,
        std::function<void(Bitset const&, std::shared_ptr<Value const>)> const& collector) const {
    if (next_bit != Bitset::npos) {
        next_bit = (next_bit == 0 ? key.find_first() : key.find_next(next_bit - 1));
    }
    if (next_bit == Bitset::npos) {
        if (value_ != nullptr) {
            collector(Bitset(superset_key), value_);
        }
        for (size_t i = offset_; i < dimension_; i++) {
            if (blacklist.test(i)) continue;
            auto subtrie = GetSubtrie(i);
            if (subtrie != nullptr) {
                superset_key.set(i);
                if (!subtrie->CollectRestrictedSupersetKeys(key, blacklist, next_bit, superset_key,
                                                            collector)) {
                    return false;
                }
                superset_key.reset(i);
            }
        }
    } else {
        for (size_t i = offset_; i < next_bit; i++) {
            if (blacklist.test(i)) continue;
            auto subtrie = GetSubtrie(i);
            if (subtrie != nullptr) {
                superset_key.set(i);
                if (!subtrie->CollectRestrictedSupersetKeys(key, blacklist, next_bit, superset_key,
                                                            collector)) {
                    return false;
                }
                superset_key.reset(i);
            }
        }

        auto subtrie = GetSubtrie(next_bit);
        if (subtrie != nullptr) {
            superset_key.set(next_bit);
            if (!subtrie->CollectRestrictedSupersetKeys(key, blacklist, next_bit + 1, superset_key,
                                                        collector)) {
                return false;
            }
            superset_key.reset(next_bit);
        }
    }
    return true;
}

template <class Value>
auto VerticalMap<Value>::GetSubsetKeys(Bitset const& bitset) const -> std::vector<Bitset> {
    std::vector<Bitset> subset_keys;
    Bitset subset_key(relation_->GetNumColumns());
    set_trie_.CollectSubsetKeys(bitset, 0, subset_key,
                                [&subset_keys](auto& indices, [[maybe_unused]] auto value) {
                                    subset_keys.push_back(indices);
                                    return true;
                                });
    return subset_keys;
}

template <class Value>
std::vector<typename VerticalMap<Value>::Entry> VerticalMap<Value>::GetSubsetEntries(
        Bitset const& bitset) const {
    std::vector<typename VerticalMap<Value>::Entry> entries;
    Bitset subset_key(relation_->GetNumColumns());
    set_trie_.CollectSubsetKeys(bitset, 0, subset_key, [&entries](auto& indices, auto value) {
        entries.emplace_back(indices, value);
        return true;
    });
    return entries;
}

// returns an empty pair if no entry is found
template <class Value>
typename VerticalMap<Value>::Entry VerticalMap<Value>::GetAnySubsetEntry(
        Bitset const& bitset) const {
    typename VerticalMap<Value>::Entry entry;
    Bitset subset_key(relation_->GetNumColumns());
    set_trie_.CollectSubsetKeys(bitset, 0, subset_key, [&entry](auto& indices, auto value) {
        entry = {indices, value};
        return false;
    });
    return entry;
}

template <class Value>
typename VerticalMap<Value>::Entry VerticalMap<Value>::GetAnySubsetEntry(
        Bitset const& bitset,
        std::function<bool(Bitset const*, std::shared_ptr<Value const>)> const& condition) const {
    typename VerticalMap<Value>::Entry entry;
    Bitset subset_key(relation_->GetNumColumns());
    set_trie_.CollectSubsetKeys(bitset, 0, subset_key,
                                [&entry, &condition](auto& indices, auto value) {
                                    if (condition(&indices, value)) {
                                        entry = {indices, value};
                                        return false;
                                    } else {
                                        return true;
                                    }
                                });
    return entry;
}

template <class Value>
std::vector<typename VerticalMap<Value>::Entry> VerticalMap<Value>::GetSupersetEntries(
        Bitset const& bitset) const {
    std::vector<typename VerticalMap<Value>::Entry> entries;
    Bitset superset_key(relation_->GetNumColumns());
    set_trie_.CollectSupersetKeys(bitset, 0, superset_key, [&entries](auto& indices, auto value) {
        entries.emplace_back(indices, value);
        return true;
    });
    return entries;
}

template <class Value>
typename VerticalMap<Value>::Entry VerticalMap<Value>::GetAnySupersetEntry(
        Bitset const& bitset) const {
    typename VerticalMap<Value>::Entry entry;
    Bitset superset_key(relation_->GetNumColumns());
    set_trie_.CollectSupersetKeys(bitset, 0, superset_key, [&entry](auto& indices, auto value) {
        entry = {indices, value};
        return false;
    });
    return entry;
}

template <class Value>
typename VerticalMap<Value>::Entry VerticalMap<Value>::GetAnySupersetEntry(
        Bitset const& bitset,
        std::function<bool(Bitset const*, std::shared_ptr<Value const>)> condition) const {
    typename VerticalMap<Value>::Entry entry;
    Bitset superset_key(relation_->GetNumColumns());
    set_trie_.CollectSupersetKeys(bitset, 0, superset_key,
                                  [&entry, &condition](auto& indices, auto value) {
                                      if (condition(&indices, value)) {
                                          entry = {indices, value};
                                          return false;
                                      } else {
                                          return true;
                                      }
                                  });
    return entry;
}

template <class Value>
std::vector<typename VerticalMap<Value>::Entry> VerticalMap<Value>::GetRestrictedSupersetEntries(
        Bitset const& bitset, Bitset const& exclusion) const {
    if (bitset.intersects(exclusion))
        throw std::runtime_error(
                "Error in GetRestrictedSupersetEntries: a vertical shouldn't intersect with a "
                "restriction");

    std::vector<typename VerticalMap<Value>::Entry> entries;
    Bitset superset_key(relation_->GetNumColumns());
    set_trie_.CollectRestrictedSupersetKeys(bitset, exclusion, 0, superset_key,
                                            [&entries](auto& indices, auto value) {
                                                entries.emplace_back(indices, value);
                                                return true;
                                            });
    return entries;
}

template <class Value>
bool VerticalMap<Value>::RemoveSupersetEntries(Bitset const& key) {
    std::vector<typename VerticalMap<Value>::Entry> superset_entries = GetSupersetEntries(key);
    for (auto superset_entry : superset_entries) {
        Remove(superset_entry.first);
    }
    return !superset_entries.empty();
}

template <class Value>
auto VerticalMap<Value>::KeySet() -> std::unordered_set<Bitset> {
    std::unordered_set<Bitset> key_set;
    Bitset subset_key(relation_->GetNumColumns());
    set_trie_.TraverseEntries(subset_key,
                              [&key_set](auto& k, [[maybe_unused]] auto v) { key_set.insert(k); });
    return key_set;
}

template <class Value>
std::vector<std::shared_ptr<Value const>> VerticalMap<Value>::Values() {
    std::vector<std::shared_ptr<Value const>> values;
    Bitset subset_key(relation_->GetNumColumns());
    set_trie_.TraverseEntries(subset_key, [&values]([[maybe_unused]] auto& k, auto v) -> void {
        values.push_back(v);
    });
    return values;
}

template <class Value>
auto VerticalMap<Value>::EntrySet() -> std::vector<Entry> {
    std::vector<Entry> entry_set;
    Bitset subset_key(relation_->GetNumColumns());
    set_trie_.TraverseEntries(
            subset_key, [&entry_set](auto& k, auto v) -> void { entry_set.emplace_back(k, v); });
    return entry_set;
}

template <class Value>
std::shared_ptr<Value> VerticalMap<Value>::Remove(Bitset const& key) {
    auto removed_value = set_trie_.Remove(key, 0);
    if (removed_value != nullptr) size_--;
    return removed_value;
}

template <class Value>
std::shared_ptr<Value> VerticalMap<Value>::Put(Bitset const& key, std::shared_ptr<Value> value) {
    auto old_value = set_trie_.Associate(key, 0, std::move(value));
    if (old_value == nullptr) size_++;

    return old_value;
}

template <class Value>
std::shared_ptr<Value const> VerticalMap<Value>::Get(Bitset const& key) const {
    return set_trie_.Get(key, 0);
}

// explicitly instantiate to solve template implementation linking issues
template class VerticalMap<PositionListIndex const>;

template class VerticalMap<AgreeSetSample>;

template class VerticalMap<DependencyCandidate>;

template class VerticalMap<VerticalInfo>;

template class VerticalMap<Vertical>;

template class VerticalMap<std::monostate>;

template <class V>
bool BlockingVerticalMap<V>::IsEmpty() const {
    std::shared_lock read_lock(read_write_mutex_);
    return VerticalMap<V>::IsEmpty();
}

template <class V>
std::shared_ptr<V const> BlockingVerticalMap<V>::Get(Bitset const& key) const {
    std::shared_lock read_lock(read_write_mutex_);
    return VerticalMap<V>::Get(key);
}

template <class V>
bool BlockingVerticalMap<V>::ContainsKey(Bitset const& key) const {
    std::shared_lock read_lock(read_write_mutex_);
    return VerticalMap<V>::ContainsKey(key);
}

template <class V>
std::shared_ptr<V> BlockingVerticalMap<V>::Put(Bitset const& key, std::shared_ptr<V> value) {
    std::scoped_lock write_lock(read_write_mutex_);
    return VerticalMap<V>::Put(key, value);
}

template <class V>
std::shared_ptr<V> BlockingVerticalMap<V>::Remove(Bitset const& key) {
    std::scoped_lock write_lock(read_write_mutex_);
    return VerticalMap<V>::Remove(key);
}

template <class V>
auto BlockingVerticalMap<V>::KeySet() -> std::unordered_set<Bitset> {
    std::shared_lock read_lock(read_write_mutex_);
    return VerticalMap<V>::KeySet();
}

template <class V>
std::vector<std::shared_ptr<V const>> BlockingVerticalMap<V>::Values() {
    std::shared_lock read_lock(read_write_mutex_);
    return VerticalMap<V>::Values();
}

template <class V>
auto BlockingVerticalMap<V>::EntrySet() -> std::vector<Entry> {
    std::shared_lock read_lock(read_write_mutex_);
    return VerticalMap<V>::EntrySet();
}

template <class V>
auto BlockingVerticalMap<V>::GetSubsetKeys(Bitset const& bitset) const -> std::vector<Bitset> {
    std::shared_lock read_lock(read_write_mutex_);
    return VerticalMap<V>::GetSubsetKeys(bitset);
}

template <class V>
std::vector<typename BlockingVerticalMap<V>::Entry> BlockingVerticalMap<V>::GetSubsetEntries(
        Bitset const& bitset) const {
    std::shared_lock read_lock(read_write_mutex_);
    return VerticalMap<V>::GetSubsetEntries(bitset);
}

template <class V>
typename BlockingVerticalMap<V>::Entry BlockingVerticalMap<V>::GetAnySubsetEntry(
        Bitset const& bitset) const {
    std::shared_lock read_lock(read_write_mutex_);
    return VerticalMap<V>::GetAnySubsetEntry(bitset);
}

template <class V>
typename BlockingVerticalMap<V>::Entry BlockingVerticalMap<V>::GetAnySubsetEntry(
        Bitset const& bitset,
        std::function<bool(Bitset const*, std::shared_ptr<V const>)> const& condition) const {
    std::shared_lock read_lock(read_write_mutex_);
    return VerticalMap<V>::GetAnySubsetEntry(bitset, condition);
}

template <class V>
std::vector<typename BlockingVerticalMap<V>::Entry> BlockingVerticalMap<V>::GetSupersetEntries(
        Bitset const& bitset) const {
    std::shared_lock read_lock(read_write_mutex_);
    return VerticalMap<V>::GetSupersetEntries(bitset);
}

template <class V>
typename BlockingVerticalMap<V>::Entry BlockingVerticalMap<V>::GetAnySupersetEntry(
        Bitset const& bitset) const {
    std::shared_lock read_lock(read_write_mutex_);
    return VerticalMap<V>::GetAnySupersetEntry(bitset);
}

template <class V>
typename BlockingVerticalMap<V>::Entry BlockingVerticalMap<V>::GetAnySupersetEntry(
        Bitset const& bitset,
        std::function<bool(Bitset const*, std::shared_ptr<V const>)> condition) const {
    std::shared_lock read_lock(read_write_mutex_);
    return VerticalMap<V>::GetAnySupersetEntry(bitset, condition);
}

template <class V>
std::vector<typename BlockingVerticalMap<V>::Entry>
BlockingVerticalMap<V>::GetRestrictedSupersetEntries(Bitset const& bitset,
                                                     Bitset const& exclusion) const {
    std::shared_lock read_lock(read_write_mutex_);
    return VerticalMap<V>::GetRestrictedSupersetEntries(bitset, exclusion);
}

template <class V>
bool BlockingVerticalMap<V>::RemoveSupersetEntries(Bitset const& key) {
    std::scoped_lock write_lock(read_write_mutex_);
    return VerticalMap<V>::RemoveSupersetEntries(key);
}

template class BlockingVerticalMap<PositionListIndex const>;

template class BlockingVerticalMap<AgreeSetSample>;

template class BlockingVerticalMap<DependencyCandidate>;

template class BlockingVerticalMap<VerticalInfo>;

template class BlockingVerticalMap<Vertical>;

}  // namespace model
