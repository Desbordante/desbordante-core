#pragma once

#include <unordered_map>
#include <utility>

#include <boost/dynamic_bitset.hpp>

class SearchTree {
public:
    using Bitset = boost::dynamic_bitset<>;
    using BitsetConsumer = std::function<void(const Bitset&)>;

private:
    struct Node {
        size_t bit_{};

        Bitset set_;
        Bitset union_;
        Bitset inter_;

        std::shared_ptr<Node> left_{};
        std::shared_ptr<Node> right_{};
        std::weak_ptr<Node> parent_{};

        [[nodiscard]] bool IsLeaf() const;

        [[nodiscard]] const Bitset& GetUnion() const;
        [[nodiscard]] const Bitset& GetInter() const;

        // for inner nodes
        Node(size_t bit, Bitset sets_union, Bitset sets_inter, const std::shared_ptr<Node>& parent,
             std::shared_ptr<Node> left = nullptr, std::shared_ptr<Node> right = nullptr);

        // for leaves
        Node(size_t bit, Bitset set, const std::shared_ptr<Node>& parent);

        // for both types of nodes
        Node(size_t bit, Bitset set, Bitset sets_union, Bitset sets_inter,
             const std::shared_ptr<Node>& parent, std::shared_ptr<Node> left = nullptr,
             std::shared_ptr<Node> right = nullptr);
    };

    size_t cardinality_{};
    size_t number_of_attributes_{};
    std::shared_ptr<Node> root_{};

    void CreateSingleElementSets(const Bitset& set);

    void CollectSubsets(const Bitset& set, const std::shared_ptr<Node>& current_node,
                        const BitsetConsumer& collect, bool& go_further) const;

    void ForEach(const std::shared_ptr<Node>& current_node, const BitsetConsumer& collect) const;

    std::shared_ptr<Node> FindNode(const Bitset& set);
    void CutLeaf(const std::shared_ptr<Node>& node_to_remove);
    void InsertLeafIntoEnd(const std::shared_ptr<Node>& current_node, const Bitset& set,
                 size_t node_bit, size_t set_bit);
    void InsertLeafIntoMiddle(const std::shared_ptr<Node>& current_node, const Bitset& set, size_t set_bit);

    static void UpdateInterAndUnion(const std::shared_ptr<Node>& node);

    static std::pair<size_t, size_t> FindNodeAndSetBits(const Bitset& node_set, const Bitset& set);

public:
    explicit SearchTree(size_t number_of_attributes);
    explicit SearchTree(const Bitset& set);

    [[nodiscard]] size_t GetCardinality() const {
        return cardinality_;
    }

    bool Add(const Bitset& set);
    bool Remove(const Bitset& set);

    void ForEach(const BitsetConsumer& collect) const;
    void ForEachSubset(const Bitset& set, const BitsetConsumer& collect) const;
    [[nodiscard]] bool ContainsAnySubsetOf(const Bitset& set) const;
};
