#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/dynamic_bitset.hpp>

namespace model {

// difficulties with const methods

// Value: PLI, AgreeSetSample, VerticalInfo, DependencyCandidate, Vertical <- all of these are
// shared_ptrs? Use template specialization for shared_ptr<Value>?
template <class Value>
class VerticalMap {
protected:
    using Bitset = boost::dynamic_bitset<>;

    // Each node corresponds to a bit in a bitset. Each node also has a vector of the possible
    // consequent set bits.
    class SetTrie {
    private:
        size_t offset_;
        size_t dimension_;
        std::vector<std::unique_ptr<SetTrie>> subtries_;
        std::shared_ptr<Value> value_;

        bool IsEmpty() const;

    public:
        explicit SetTrie(size_t dimension) : SetTrie(0, dimension) {}

        SetTrie(size_t offset, size_t dimension) : offset_(offset), dimension_(dimension) {}

        // Sets given key to a given value
        // Returns the old value with ownership
        // Not a const method as SetTrie gets changed
        std::shared_ptr<Value> Associate(Bitset const& key, size_t next_bit,
                                         std::shared_ptr<Value> value);

        // Returns a pointer to the value mapped by the given key
        std::shared_ptr<Value const> Get(Bitset const& key, size_t next_bit) const;

        // Erases an entry with the given key
        // Returns the old value with ownership
        std::shared_ptr<Value> Remove(Bitset const& key, size_t next_bit);

        // Gets the subtrie with the given index. If such a subtrie does not exist, creates one
        // Not a const method as a SetTrie may be created
        SetTrie* GetOrCreateSubTrie(size_t index);

        // Gets the subtrie with the given index
        // Non-const version of the method -- needed in getOrCreateSubtrie to manipulate SetTrie*
        SetTrie* GetSubtrie(size_t index);

        // Gets the subtrie with the given index
        // Const version of the method
        SetTrie const* GetSubtrie(size_t index) const;

        // Calls collector on every trie that is a subset of the given subset_key
        bool CollectSubsetKeys(
                Bitset const& key, size_t next_bit, Bitset& subset_key,
                std::function<bool(Bitset const&, std::shared_ptr<Value const>)> const& collector)
                const;

        // Calls collector on every trie that is a superset of the given subsetKey
        bool CollectSupersetKeys(
                Bitset const& key, size_t next_bit, Bitset& superset_key,
                std::function<bool(Bitset const&, std::shared_ptr<Value const>)> const& collector)
                const;

        // Calls collector on every trie that is a superset of the given subsetKey with no bits from
        // the blacklist
        bool CollectRestrictedSupersetKeys(
                Bitset const& key, Bitset const& blacklist, size_t next_bit, Bitset& superset_key,
                std::function<void(Bitset const&, std::shared_ptr<Value const>)> const& collector)
                const;

        // Calls collector on every entry
        void TraverseEntries(
                Bitset& subset_key,
                std::function<void(Bitset const&, std::shared_ptr<Value const>)> collector) const;
    };

    std::size_t num_columns_;
    size_t size_ = 0;
    SetTrie set_trie_;

public:
    using Entry = std::pair<Bitset, std::shared_ptr<Value const>>;

    explicit VerticalMap(std::size_t num_columns)
        : num_columns_(num_columns), set_trie_(num_columns) {}

    virtual bool IsEmpty() const {
        return size_ == 0;
    }

    // basic get-check-insert-remove operations
    virtual std::shared_ptr<Value const> Get(Bitset const& key) const;

    virtual bool ContainsKey(Bitset const& key) const {
        return Get(key) != nullptr;
    }

    virtual std::shared_ptr<Value> Put(Bitset const& key, std::shared_ptr<Value> value);
    virtual std::shared_ptr<Value> Remove(Bitset const& key);

    // get all keys/values/entries for traversing
    virtual std::unordered_set<Bitset> KeySet();
    virtual std::vector<std::shared_ptr<Value const>> Values();
    virtual std::vector<Entry> EntrySet();

    // get specific entries for traversing
    virtual std::vector<Bitset> GetSubsetKeys(Bitset const& vertical) const;
    virtual std::vector<Entry> GetSubsetEntries(Bitset const& vertical) const;
    virtual Entry GetAnySubsetEntry(Bitset const& vertical) const;
    virtual Entry GetAnySubsetEntry(
            Bitset const& vertical,
            std::function<bool(Bitset const*, std::shared_ptr<Value const>)> const& condition)
            const;
    virtual std::vector<Entry> GetSupersetEntries(Bitset const& vertical) const;
    virtual Entry GetAnySupersetEntry(Bitset const& vertical) const;
    virtual Entry GetAnySupersetEntry(
            Bitset const& vertical,
            std::function<bool(Bitset const*, std::shared_ptr<Value const>)> condition) const;
    virtual std::vector<Entry> GetRestrictedSupersetEntries(Bitset const& vertical,
                                                            Bitset const& exclusion) const;
    virtual bool RemoveSupersetEntries(Bitset const& key);

    virtual ~VerticalMap() = default;
};

/*
 * A version of VerticalMap for parallel processing. Uses reader-writer mutex for blocking.
 * */
template <class V>
class BlockingVerticalMap : public VerticalMap<V> {
private:
    // mutable нужен, чтобы const методы могли его лочить (т.к. меняется состояние мьютекса)
    mutable std::shared_mutex read_write_mutex_;
    // shared_mutex, scoped_lock, shared_lock
public:
    using typename VerticalMap<V>::Entry;
    using typename VerticalMap<V>::Bitset;

    explicit BlockingVerticalMap(std::size_t num_columns) : VerticalMap<V>(num_columns) {}

    virtual bool IsEmpty() const override;

    virtual std::shared_ptr<V const> Get(Bitset const& key) const override;
    virtual bool ContainsKey(Bitset const& key) const override;
    virtual std::shared_ptr<V> Put(Bitset const& key, std::shared_ptr<V> value) override;
    virtual std::shared_ptr<V> Remove(Bitset const& key) override;

    virtual std::unordered_set<Bitset> KeySet() override;
    virtual std::vector<std::shared_ptr<V const>> Values() override;
    virtual std::vector<Entry> EntrySet() override;

    virtual std::vector<Bitset> GetSubsetKeys(Bitset const& vertical) const override;
    virtual std::vector<Entry> GetSubsetEntries(Bitset const& vertical) const override;
    virtual Entry GetAnySubsetEntry(Bitset const& vertical) const override;
    virtual Entry GetAnySubsetEntry(
            Bitset const& vertical,
            std::function<bool(Bitset const*, std::shared_ptr<V const>)> const& condition)
            const override;
    virtual std::vector<Entry> GetSupersetEntries(Bitset const& vertical) const override;
    virtual Entry GetAnySupersetEntry(Bitset const& vertical) const override;
    virtual Entry GetAnySupersetEntry(
            Bitset const& vertical,
            std::function<bool(Bitset const*, std::shared_ptr<V const>)> condition) const override;
    virtual std::vector<Entry> GetRestrictedSupersetEntries(Bitset const& vertical,
                                                            Bitset const& exclusion) const override;
    virtual bool RemoveSupersetEntries(Bitset const& key) override;

    virtual ~BlockingVerticalMap() = default;
};

}  // namespace model
