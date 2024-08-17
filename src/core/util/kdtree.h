#pragma once

#include <algorithm>
#include <vector>

namespace util {

template <typename PointType>
class KDTree;

// @brief k-dimensional Rectangle used for query search
template <typename PointType>
struct Rect {
    enum bound_type { kOpen = 0, kClosed };

    PointType lower_bound_, upper_bound_;
    std::vector<bound_type> lower_bound_type_, upper_bound_type_;

    Rect() = default;
    Rect(Rect<PointType> const&) = default;
    Rect(Rect<PointType>&&) = default;
    Rect<PointType>& operator=(Rect<PointType> const&) = default;

    Rect(PointType const& lower_bound, PointType const& upper_bound)
        : lower_bound_(lower_bound), upper_bound_(upper_bound) {
        lower_bound_type_ = std::vector<bound_type>(lower_bound.GetDim(), kOpen);
        upper_bound_type_ = std::vector<bound_type>(upper_bound.GetDim(), kOpen);
        assert(lower_bound_.GetDim() == upper_bound_.GetDim());
    }

    void SetBoundType(std::vector<bound_type> lower_bound_type,
                      std::vector<bound_type> upper_bound_type) {
        assert(lower_bound_.GetDim() == upper_bound_type_.size());
        lower_bound_type_ = lower_bound_type;
        upper_bound_type_ = upper_bound_type;
    }

    bool Fits(PointType const& point) const {
        assert(point.GetDim() == lower_bound_.GetDim());
        for (size_t i = 0; i < point.GetDim(); i++) {
            if (lower_bound_type_[i] == kOpen) {
                if (point[i] <= lower_bound_[i]) return false;
            } else if (point[i] < lower_bound_[i])
                return false;

            if (upper_bound_type_[i] == kOpen) {
                if (point[i] >= upper_bound_[i]) return false;
            } else if (point[i] > upper_bound_[i])
                return false;
        }

        return true;
    }
};

// @tparam T is a type of a stored point,
//  T should support operator[] and GetDim() method,
// which returns dimensionality of a point.
// T[i] should support <, <=, >, >=, ==
template <typename PointType>
class KDTree {
private:
    class Node {
        friend class KDTree;

        PointType point_;
        Node* left_;
        Node* right_;
        size_t axis_;

        Node(PointType const& point, Node* left, Node* right, size_t axis)
            : point_(point), left_(left), right_(right), axis_(axis) {}

        ~Node() {
            if (left_ != nullptr) delete left_;
            if (right_ != nullptr) delete right_;
        }

        Node(Node&&) = delete;
        Node(Node const&) = delete;
        Node& operator=(Node const&) = delete;
        Node& operator=(Node&&) = delete;
    };

    Node* root_;
    size_t size_;

    template <typename T>
    void InsertRecursive(T&& point, Node*& dest, size_t cur_dim);

    void SearchRecursive(Node* start, std::vector<PointType>& res,
                         Rect<PointType> const& box) const;

public:
    KDTree() : root_(nullptr), size_(0) {};

    KDTree(std::vector<PointType> const& points);
    KDTree(std::initializer_list<PointType> const& points);
    KDTree& operator=(KDTree&& tree);
    KDTree(KDTree&& tree);
    ~KDTree();

    KDTree(KDTree const&) = delete;

    size_t Size() const;
    void Erase(Node* dest);

    std::vector<PointType> AsVector() const;
    void AddSubtree(Node* start, std::vector<PointType>& res) const;

    template <typename T>
    void Insert(T&& point);

    // TODO: Should better implement boolean search (for some reason
    // naive boolean search fails some tests)
    std::vector<PointType> QuerySearch(Rect<PointType> const& box) const;
};

template <typename PointType>
KDTree<PointType>& KDTree<PointType>::operator=(KDTree<PointType>&& tree) {
    std::swap(root_, tree.root_);
    std::swap(size_, tree.size_);
    return *this;
}

template <typename PointType>
KDTree<PointType>::KDTree(KDTree<PointType>&& tree) {
    std::swap(root_, tree.root_);
    std::swap(size_, tree.size_);
}

template <typename PointType>
std::vector<PointType> KDTree<PointType>::AsVector() const {
    std::vector<PointType> res;
    AddSubtree(root_, res);
    return res;
}

template <typename PointType>
void KDTree<PointType>::AddSubtree(Node* start, std::vector<PointType>& res) const {
    if (start == nullptr) return;
    AddSubtree(start->left_, res);
    AddSubtree(start->right_, res);
    res.push_back(start->point_);
}

template <typename PointType>
std::vector<PointType> KDTree<PointType>::QuerySearch(Rect<PointType> const& box) const {
    std::vector<PointType> res;
    SearchRecursive(root_, res, box);
    return res;
}

template <typename PointType>
void KDTree<PointType>::SearchRecursive(Node* start, std::vector<PointType>& res,
                                        Rect<PointType> const& box) const {
    if (start == nullptr) return;

    size_t cur_axis = start->axis_;
    auto cur_val = start->point_[cur_axis];
    if (box.Fits(start->point_)) res.push_back(start->point_);

    if (box.lower_bound_[cur_axis] <= cur_val) SearchRecursive(start->left_, res, box);

    if (cur_val <= box.upper_bound_[cur_axis]) SearchRecursive(start->right_, res, box);
}

template <typename PointType>
size_t KDTree<PointType>::Size() const {
    return size_;
}

template <typename PointType>
KDTree<PointType>::KDTree(std::vector<PointType> const& points) : KDTree<PointType>() {
    for (auto const& p : points) {
        Insert(p);
    }
}

template <typename PointType>
KDTree<PointType>::KDTree(std::initializer_list<PointType> const& points) : KDTree<PointType>() {
    for (auto const& p : points) {
        Insert(p);
    }
}

template <typename PointType>
template <typename T>
void KDTree<PointType>::Insert(T&& point) {
    size_t cur_dim = 0;
    InsertRecursive(std::forward<T>(point), root_, cur_dim);
    size_++;
}

template <typename PointType>
template <typename T>
void KDTree<PointType>::InsertRecursive(T&& point, KDTree<PointType>::Node*& dest, size_t cur_dim) {
    cur_dim %= point.GetDim();

    if (dest == nullptr)
        dest = new Node(std::forward<T>(point), nullptr, nullptr, cur_dim);

    else if (point[cur_dim] <= dest->point_[cur_dim]) {
        if (dest->left_ == nullptr)
            dest->left_ = new Node(std::forward<T>(point), nullptr, nullptr,
                                   (cur_dim + 1) % point.GetDim());
        else
            InsertRecursive(std::forward<T>(point), dest->left_, cur_dim + 1);
    } else {
        if (dest->right_ == nullptr)
            dest->right_ = new Node(std::forward<T>(point), nullptr, nullptr,
                                    (cur_dim + 1) % point.GetDim());
        else
            InsertRecursive(std::forward<T>(point), dest->right_, cur_dim + 1);
    }
}

template <typename PointType>
void KDTree<PointType>::Erase(KDTree<PointType>::Node* dest) {
    delete dest;
}

template <typename PointType>
KDTree<PointType>::~KDTree() {
    Erase(root_);
}

}  // namespace util
