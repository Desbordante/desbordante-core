#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <iterator>
#include <roaring.hh>
#include <stdexcept>
#include <utility>
#include <vector>

#include "core/util/logger.h"

namespace utils {
/* LT-tree node invariant
// ======================

// Each node stores one distinct value and two id sets:

//         +------------------------------+
//         | val = x                      |
//         | {row.id} s.t row.val == x    |
//         | {row.id} s.t row.val <  x    |
//         +------------------------------+
//               /          \
//         values < x     values > x


Example
=======

Table:
index|value
0|100
1|50
2|25
3|200
4|50
5|10

After consecutive insertion of values with according indices the tree looks as follows:

                        +----------------------+
                        |    value: 100        |
                        |    tuple set: {0}    |
                        |    LT-set: {1,2,4,5} |
                        +----------------------+
                             /              \
                            /                \
        +----------------------+     +----------------------+
        |    value: 50         |     |    value: 200        |
        |    tuple set: {1,4}  |     |    tuple set: {3}    |
        |    LT-set: {2,5}     |     |    LT-set: {}        |
        +----------------------+     +----------------------+
                /
               /
+----------------------+
|    value: 25         |
|    tuple set: {2}    |
|    LT-set: {}        |
+----------------------+


For every node:
  LT(node) = union(ids of every node in node.left subtree)


Left rotation
=============

Before rotating left at X:

             X
            / \
           A   Y
              / \
             B   C

After:

                 Y
                / \
               X   C
              / \
             A   B

Code shape:
  Y.LT |= X.LT
  Y.LT |= X.ids


Right rotation
==============

Before rotating right at X:

                 X
                / \
               Y   C
              / \
             A   B

After:

             Y
       LT = Ly
       ids = Iy
            / \
           A   X
         LT = Lx
         ids = Ix
              / \
             B   C

*/

/* @brief Less than tree, represents a balanced binary search
 * tree with fast lookup for indices which values are less than x
 * @tparam T type of stored values in tree nodes
 */
template <std::totally_ordered T>
class LTTree {
    /* @brief LTTree node
     * @property val current value in the node
     * @property height the height of the subtree with a root as current node
     * @property left left child
     * @property right right child
     * @property lt_indices indices which values are less than 'val'
     * @property cur_indices indices which values are equal to 'val'
     */
    class Node {
    public:
        roaring::Roaring lt_indices;
        roaring::Roaring cur_indices;
        T val;

        size_t height = 1;
        Node* left = nullptr;
        Node* right = nullptr;
        Node* parent = nullptr;

        template <typename U>
        Node(U&& _val, size_t _index, Node* _parent = nullptr)
            : val(std::forward<U>(_val)), parent(_parent) {
            cur_indices.add(static_cast<uint32_t>(_index));
        };

        void Unlink() {
            left = nullptr;
            right = nullptr;
            parent = nullptr;
        }

        ~Node() {
            delete left;
            delete right;
        }

        // bool operator<(Node const& rhs) const {
        //     return val < rhs.val;
        // }

        void Swap(Node*& other) {
            std::swap(left, other->left);
            std::swap(right, other->right);
            std::swap(parent, other->parent);
            std::swap(lt_indices, other->lt_indices);
            std::swap(cur_indices, other->cur_indices);
        }
    };

    class NodeView {
    public:
        NodeView(Node const* cur) : cur_(cur) {}

        T const& val() const noexcept {
            return cur_->val;
        }

        roaring::Roaring const& lt_indices() const noexcept {
            return cur_->lt_indices;
        };

        roaring::Roaring const& cur_indices() const noexcept {
            return cur_->cur_indices;
        };

        bool operator<(NodeView const& rhs) const noexcept {
            return cur_->val < rhs.cur_->val;
        }

    private:
        Node const* cur_;
    };

    class Iterator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = NodeView;
        using reference = NodeView;
        using pointer = NodeView*;
        using iterator_concept = std::bidirectional_iterator_tag;
        using iterator_category = std::bidirectional_iterator_tag;

        Iterator(Node* cur, Node* root) : cur_(cur), root_(root) {}

        Iterator& operator++() {
            if (cur_->right) {
                cur_ = cur_->right;
                while (cur_->left) {
                    cur_ = cur_->left;
                }
            } else {
                Node* child = cur_;
                cur_ = cur_->parent;
                while (cur_ != nullptr and cur_->right == child) {
                    child = cur_;
                    cur_ = cur_->parent;
                }
            }
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        Iterator& operator--() {
            if (cur_ == nullptr) {
                cur_ = root_;
                while (cur_->right != nullptr) {
                    cur_ = cur_->right;
                }
            } else if (cur_->left) {
                cur_ = cur_->left;
                while (cur_->right) {
                    cur_ = cur_->right;
                }
            } else {
                Node* child = cur_;
                cur_ = cur_->parent;
                while (cur_ != nullptr and cur_->left == child) {
                    child = cur_;
                    cur_ = cur_->parent;
                }
            }
            return *this;
        }

        Iterator operator--(int) {
            Iterator tmp = *this;
            --(*this);
            return tmp;
        }

        NodeView operator*() const {
            return {cur_};
        }

        NodeView* operator->() const {
            view_ = NodeView(cur_);
            return &view_;
        }

        bool operator==(Iterator const& rhs) const noexcept {
            return cur_ == rhs.cur_;
        };

        bool operator!=(Iterator const& rhs) const noexcept {
            return !(cur_ == rhs.cur_);
        };

        Iterator& operator=(Iterator const& rhs) noexcept {
            this->cur_ = rhs.cur_;
            this->root_ = rhs.root_;
            return *this;
        }

    private:
        Node* cur_ = nullptr;
        Node* root_ = nullptr;
        mutable NodeView view_ = {nullptr};
    };

    // struct Entry {
    //     T val;
    //     size_t index;
    // };

    /* @param val inserted value
     * @param index index of the according value
     * @param cur the current node where the insertion occurs
     */
    template <typename U>
    Node* InsertRecursive(U&& val, size_t index, Node* cur);

    bool IsBalancedRecursive(Node const* cur) const noexcept;
    void FindLessRecursive(T const& val, Node const* cur, roaring::Roaring& res) const;
    void FindLessOrEqualRecursive(T const& val, Node const* cur, roaring::Roaring& res) const;
    Node* RemoveRecursive(T const& val, size_t index, Node* cur);
    bool ContainsRecursive(T const& val, Node const* cur) const noexcept;
    bool ContainsRecursive(T const& val, size_t index, Node const* cur) const noexcept;

    size_t Height(Node const* cur) const noexcept;
    int Balance(Node const* cur) const noexcept;
    void UpdateHeight(Node* cur) noexcept;
    Node* GetLeftMost(Node* cur) const noexcept;

    bool isConsistent(Node const* cur, Node const* parent) {
        if (cur == nullptr) return true;
        return cur->parent == parent and isConsistent(cur->left, cur) and
               isConsistent(cur->right, cur);
    }

    /* @brief Rotate appropriate nodes to the left
     *  (pull the right child up and move 'cur' node down to the left)
     *  @return Right child of 'cur' as a root
     */
    Node* RotateLeft(Node* cur) noexcept;

    /* @brief Rotate appropriate nodes to the right
     *  (pull the left child up and move 'cur' node down to the right)
     *  @return Left child of 'cur' as a root
     */
    Node* RotateRight(Node* cur) noexcept;

    /* @brief Rebalance the 'cur' Node passed so its balance is either -1, 0 or 1
     *  @return
     */
    Node* Rebalance(Node* cur);

    Node* root_ = nullptr;
    Node* min_ = nullptr;
    size_t size_ = 0;

public:
    LTTree() noexcept {};

    ~LTTree() {
        delete root_;
    }

    template <typename U>
    void Insert(U&& val, size_t index);

    bool IsBalanced() const;
    void Remove(T const& val, size_t index);

    roaring::Roaring FindLess(T const& val) const {
        roaring::Roaring res;
        FindLessRecursive(val, root_, res);
        return res;
    }

    roaring::Roaring FindLessOrEqual(T const& val) const {
        roaring::Roaring res;
        FindLessOrEqualRecursive(val, root_, res);
        return res;
    }

    static std::vector<size_t> RoaringToVector(roaring::Roaring const& rb);

    bool Contains(T const& val) const noexcept {
        return ContainsRecursive(val, root_);
    }

    bool Contains(T const& val, size_t index) const noexcept {
        return ContainsRecursive(val, index, root_);
    }

    // @return Number of total unique indices in all nodes
    size_t Size() const noexcept {
        return size_;
    }

    size_t Height() const noexcept {
        return Height(root_);
    }

    Iterator begin() {
        return {min_, root_};
    }

    Iterator end() {
        return {nullptr, root_};
    }
};

template <std::totally_ordered T>
LTTree<T>::Node* LTTree<T>::GetLeftMost(Node* cur) const noexcept {
    if (cur == nullptr) return cur;
    while (cur->left != nullptr) cur = cur->left;
    return cur;
}

template <std::totally_ordered T>
void LTTree<T>::UpdateHeight(Node* cur) noexcept {
    size_t left_height = Height(cur->left);
    size_t right_height = Height(cur->right);
    cur->height = 1 + std::max(left_height, right_height);
}

template <std::totally_ordered T>
size_t LTTree<T>::Height(Node const* cur) const noexcept {
    return cur == nullptr ? 0 : cur->height;
}

template <std::totally_ordered T>
int LTTree<T>::Balance(Node const* cur) const noexcept {
    return static_cast<int>(Height(cur->left)) - static_cast<int>(Height(cur->right));
}

template <typename T>
concept HasToString = requires(T t) {
    { t.ToString() } -> std::same_as<std::string>;
};

// Generic print function
template <typename T>
void print_object(T const& obj) {
    if constexpr (HasToString<T>) {
        // This branch is compiled ONLY if obj has ToString() returning std::string
        std::cout << obj.ToString() << std::endl;
    } else {
        // This branch is compiled ONLY if obj does not have the method
        std::cout << obj << std::endl;
    }
}

template <std::totally_ordered T>
template <typename U>
void LTTree<T>::Insert(U&& val, size_t index) {
    root_ = InsertRecursive(std::forward<U>(val), index, root_);
    assert(root_ != nullptr and isConsistent(root_, nullptr));
    min_ = GetLeftMost(root_);
    ++size_;
}

template <std::totally_ordered T>
template <typename U>
LTTree<T>::Node* LTTree<T>::InsertRecursive(U&& val, size_t index, Node* cur) {
    if (cur == nullptr) {
        return new Node(std::forward<U>(val), index);
    }

    if (static_cast<T>(val) < cur->val) {
        cur->lt_indices.add(static_cast<uint32_t>(index));
        cur->left = InsertRecursive(std::forward<U>(val), index, cur->left);
        cur->left->parent = cur;

    } else if (static_cast<T>(val) == cur->val) {
        cur->cur_indices.add(static_cast<uint32_t>(index));

    } else {
        cur->right = InsertRecursive(std::forward<U>(val), index, cur->right);
        cur->right->parent = cur;
    }

    UpdateHeight(cur);
    return Rebalance(cur);
}

// 0 1 2, для нуля не устанавливается родитель при перебалансировке
template <std::totally_ordered T>
LTTree<T>::Node* LTTree<T>::Rebalance(Node* cur) {
    int balance = Balance(cur);
    // Left side is imbalanced
    if (balance > 1) {
        if (Balance(cur->left) < 0) {
            cur->left = RotateLeft(cur->left);
            if (cur->left) cur->left->parent = cur;
        }
        return RotateRight(cur);
    }
    // Right side is imbalanced
    else if (balance < -1) {
        if (Balance(cur->right) > 0) {
            cur->right = RotateRight(cur->right);
            if (cur->right) cur->right->parent = cur;
        }
        return RotateLeft(cur);
    }

    return cur;
}

template <std::totally_ordered T>
std::vector<size_t> LTTree<T>::RoaringToVector(roaring::Roaring const& rb) {
    std::vector<size_t> result;
    result.reserve(rb.cardinality());
    for (uint32_t val : rb) {
        result.push_back(static_cast<size_t>(val));
    }
    return result;
}

template <std::totally_ordered T>
LTTree<T>::Node* LTTree<T>::RotateLeft(Node* cur) noexcept {
    assert(cur != nullptr);

    // Update node's lt sets
    cur->right->lt_indices |= cur->lt_indices;
    cur->right->lt_indices |= cur->cur_indices;

    // Update node's links
    Node* right = cur->right;
    cur->right = right->left;
    if (right->left) right->left->parent = cur;
    right->left = cur;
    right->parent = cur->parent;
    cur->parent = right;

    UpdateHeight(cur);
    UpdateHeight(right);

    return right;
}

template <std::totally_ordered T>
LTTree<T>::Node* LTTree<T>::RotateRight(Node* cur) noexcept {
    assert(cur != nullptr);

    // Update node's lt sets
    cur->lt_indices -= cur->left->lt_indices;
    cur->lt_indices -= cur->left->cur_indices;

    // Update node's links
    Node* left = cur->left;
    cur->left = left->right;
    if (left->right) left->right->parent = cur;
    left->right = cur;
    left->parent = cur->parent;
    cur->parent = left;

    UpdateHeight(cur);
    UpdateHeight(left);

    return left;
}

template <std::totally_ordered T>
bool LTTree<T>::IsBalanced() const {
    return IsBalancedRecursive(root_);
}

template <std::totally_ordered T>
bool LTTree<T>::IsBalancedRecursive(Node const* cur) const noexcept {
    if (cur == nullptr) return true;
    return std::abs(Balance(cur)) <= 1 and IsBalancedRecursive(cur->left) and
           IsBalancedRecursive(cur->right);
}

template <std::totally_ordered T>
void LTTree<T>::FindLessRecursive(T const& val, Node const* cur, roaring::Roaring& res) const {
    if (cur == nullptr) return;
    if (val < cur->val) {
        return FindLessRecursive(val, cur->left, res);
    } else if (val > cur->val) {
        res |= cur->lt_indices;
        res |= cur->cur_indices;
        return FindLessRecursive(val, cur->right, res);
    } else {
        res |= cur->lt_indices;
        return;
    }
}

template <std::totally_ordered T>
void LTTree<T>::FindLessOrEqualRecursive(T const& val, Node const* cur,
                                         roaring::Roaring& res) const {
    if (cur == nullptr) return;
    if (val < cur->val) {
        return FindLessOrEqualRecursive(val, cur->left, res);
    } else if (val > cur->val) {
        res |= cur->lt_indices;
        res |= cur->cur_indices;
        return FindLessOrEqualRecursive(val, cur->right, res);
    } else {
        res |= cur->lt_indices;
        res |= cur->cur_indices;
        return;
    }
}

template <std::totally_ordered T>
void LTTree<T>::Remove(T const& val, size_t index) {
    if (!Contains(val, index)) {
        throw std::logic_error("[LTTree]: Entry " + std::to_string(index) + " doesn't exist");
    }

    root_ = RemoveRecursive(val, index, root_);
    if (root_) root_->parent = nullptr;
    assert(isConsistent(root_, nullptr));
    min_ = root_ ? GetLeftMost(root_) : nullptr;
    --size_;
}

template <std::totally_ordered T>
LTTree<T>::Node* LTTree<T>::RemoveRecursive(T const& val, size_t index, Node* cur) {
    if (val > cur->val) {
        cur->right = RemoveRecursive(val, index, cur->right);
        if (cur->right) cur->right->parent = cur;
    } else if (val < cur->val) {
        cur->lt_indices.remove(static_cast<uint32_t>(index));
        cur->left = RemoveRecursive(val, index, cur->left);
        if (cur->left) cur->left->parent = cur;
    } else {
        // if cur_indices is not empty no need to delete node
        cur->cur_indices.remove(index);
        if (cur->cur_indices.cardinality() != 0) {
            return cur;
        }

        // if node is a leaf simply delete it
        if (cur->left == nullptr and cur->right == nullptr) {
            delete cur;
            return nullptr;
        }

        // if node has only right subtree delete cur
        if (cur->left == nullptr) {
            Node* right = cur->right;
            cur->Unlink();
            delete cur;
            return right;
        }

        // if node has only left subtree delete cur
        if (cur->right == nullptr) {
            Node* left = cur->left;
            cur->Unlink();
            delete cur;
            return left;
        }

        // Replace current node with in-order successor (leftmost of right subtree).
        Node* succ = GetLeftMost(cur->right);
        assert(succ != nullptr);
        cur->val = succ->val;
        cur->cur_indices = succ->cur_indices;
        for (auto idx : cur->cur_indices) {
            cur->right = RemoveRecursive(succ->val, idx, cur->right);
            if (cur->right) cur->right->parent = cur;
        }
    }

    UpdateHeight(cur);
    cur = Rebalance(cur);
    return cur;
}

template <std::totally_ordered T>
bool LTTree<T>::ContainsRecursive(T const& val, Node const* cur) const noexcept {
    if (cur == nullptr) return false;
    if (val > cur->val) return ContainsRecursive(val, cur->right);
    if (val < cur->val) return ContainsRecursive(val, cur->left);
    return true;
}

template <std::totally_ordered T>
bool LTTree<T>::ContainsRecursive(T const& val, size_t index, Node const* cur) const noexcept {
    if (cur == nullptr) return false;
    if (val > cur->val) return ContainsRecursive(val, cur->right);
    if (val < cur->val) return ContainsRecursive(val, cur->left);
    return cur->cur_indices.contains(static_cast<uint32_t>(index));
}

}  // namespace utils
