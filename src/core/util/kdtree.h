#pragma once

#include <algorithm>
#include <concepts>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace util {

template <typename T>
concept Comparable = std::equality_comparable<T> and std::three_way_comparable<T>;

template <typename T>
concept SubscriptableOrder = requires(T t, size_t i) {
    { t[i] } -> Comparable;
    t.GetDim();
};

template <SubscriptableOrder PointType>
class KDTree;

// @brief k-dimensional Rectangle used for query search
template <SubscriptableOrder PointType>
struct Rect {
    enum bound_type { kOpen = 0, kClosed };

    PointType lower_bound_, upper_bound_;
    std::vector<bound_type> lower_bound_type_, upper_bound_type_;

    Rect() = default;
    Rect(Rect<PointType> const&) = default;
    Rect(Rect<PointType>&&) = default;
    Rect<PointType>& operator=(Rect<PointType> const&) = default;

    Rect(PointType lower_bound, PointType upper_bound)
        : lower_bound_(std::move(lower_bound)), upper_bound_(std::move(upper_bound)) {
        if (lower_bound.GetDim() != upper_bound.GetDim())
            throw std::logic_error("Dimensionalities of given points don't match");

        lower_bound_type_ = std::vector<bound_type>(lower_bound.GetDim(), kOpen);
        upper_bound_type_ = std::vector<bound_type>(upper_bound.GetDim(), kOpen);
    }

    void SetBoundType(std::vector<bound_type> lower_bound_type,
                      std::vector<bound_type> upper_bound_type) {
        if (lower_bound_.GetDim() != upper_bound_.GetDim())
            throw std::logic_error("Dimensionalities of given points don't match");

        lower_bound_type_ = std::move(lower_bound_type);
        upper_bound_type_ = std::move(upper_bound_type);
    }

    bool Fits(PointType const& point) const {
        if (lower_bound_.GetDim() != upper_bound_.GetDim())
            throw std::logic_error("Dimensionalities of a given point and boundaries don't match");

        for (size_t i = 0; i < point.GetDim(); i++) {
            if (lower_bound_type_[i] == kOpen) {
                if (point[i] <= lower_bound_[i]) return false;
            } else if (point[i] < lower_bound_[i]) {
                return false;
            }

            if (upper_bound_type_[i] == kOpen) {
                if (point[i] >= upper_bound_[i]) return false;
            } else if (point[i] > upper_bound_[i]) {
                return false;
            }
        }

        return true;
    }

    std::string ToString() const {
        std::ostringstream ss;
        static char constexpr const kOpen[]{'(', '['};
        static char constexpr const kClosed[]{')', ']'};
        for (size_t i = 0; i < lower_bound_.GetDim(); i++) {
            ss << kOpen[lower_bound_type_[i]] << lower_bound_[i].ToString() << ", ";
            ss << upper_bound_[i].ToString() << kClosed[upper_bound_type_[i]] << '\n';
        }

        return ss.str();
    }
};

template <SubscriptableOrder PointType>
class KDTree {
private:
    class Node {
        friend class KDTree;

        PointType point_;
        std::unique_ptr<Node> left_;
        std::unique_ptr<Node> right_;
        size_t axis_;

    public:
        Node() = default;

        template <typename U>
        Node(U&& point, std::unique_ptr<Node> left, std::unique_ptr<Node> right, size_t axis)
            : point_(std::move(point)),
              left_(std::move(left)),
              right_(std::move(right)),
              axis_(axis) {}

        ~Node() = default;

        Node(Node&&) = delete;
        Node(Node const&) = delete;
        Node& operator=(Node const&) = delete;
        Node& operator=(Node&&) = delete;
    };

    std::unique_ptr<Node> root_;
    size_t size_;

    template <typename T>
    void InsertRecursive(T&& point, std::unique_ptr<Node>& dest, size_t cur_dim);

    void SearchRecursive(Node* start, std::vector<PointType>& res,
                         Rect<PointType> const& box) const;

public:
    KDTree() : root_(nullptr), size_(0) {};

    KDTree(std::vector<PointType> const& points);
    KDTree(std::initializer_list<PointType> const& points);
    KDTree& operator=(KDTree&& tree);
    KDTree(KDTree&& tree);
    ~KDTree() = default;

    KDTree(KDTree const&) = delete;

    size_t Size() const;

    std::vector<PointType> AsVector() const;
    void AddSubtree(Node* start, std::vector<PointType>& res) const;

    template <typename T>
    void Insert(T&& point);

    // Since boolean search support won't be applicable in the next version
    // with highlight support so there is a little sense to implement it
    std::vector<PointType> QuerySearch(Rect<PointType> const& box) const;
};

template <SubscriptableOrder PointType>
KDTree<PointType>& KDTree<PointType>::operator=(KDTree<PointType>&& tree) {
    std::swap(root_, tree.root_);
    std::swap(size_, tree.size_);
    return *this;
}

template <SubscriptableOrder PointType>
KDTree<PointType>::KDTree(KDTree<PointType>&& tree) {
    std::swap(root_, tree.root_);
    std::swap(size_, tree.size_);
}

template <SubscriptableOrder PointType>
std::vector<PointType> KDTree<PointType>::AsVector() const {
    std::vector<PointType> res;
    AddSubtree(root_.get(), res);
    return res;
}

template <SubscriptableOrder PointType>
void KDTree<PointType>::AddSubtree(Node* start, std::vector<PointType>& res) const {
    if (start == nullptr) return;
    AddSubtree(start->left_.get(), res);
    AddSubtree(start->right_.get(), res);
    res.push_back(start->point_);
}

template <SubscriptableOrder PointType>
std::vector<PointType> KDTree<PointType>::QuerySearch(Rect<PointType> const& box) const {
    std::vector<PointType> res;
    SearchRecursive(root_.get(), res, box);
    return res;
}

template <SubscriptableOrder PointType>
void KDTree<PointType>::SearchRecursive(Node* start, std::vector<PointType>& res,
                                        Rect<PointType> const& box) const {
    if (start == nullptr) return;

    size_t cur_axis = start->axis_;
    auto cur_val = start->point_[cur_axis];
    if (box.Fits(start->point_)) res.push_back(start->point_);

    if (box.lower_bound_[cur_axis] <= cur_val) SearchRecursive(start->left_.get(), res, box);

    if (cur_val <= box.upper_bound_[cur_axis]) SearchRecursive(start->right_.get(), res, box);
}

template <SubscriptableOrder PointType>
size_t KDTree<PointType>::Size() const {
    return size_;
}

template <SubscriptableOrder PointType>
KDTree<PointType>::KDTree(std::vector<PointType> const& points) : KDTree<PointType>() {
    std::for_each(points.begin(), points.end(), &Insert);
}

template <SubscriptableOrder PointType>
KDTree<PointType>::KDTree(std::initializer_list<PointType> const& points) : KDTree<PointType>() {
    std::for_each(points.begin(), points.end(), &Insert);
}

template <SubscriptableOrder PointType>
template <typename T>
void KDTree<PointType>::Insert(T&& point) {
    size_t cur_dim = 0;
    InsertRecursive(std::forward<T>(point), root_, cur_dim);
    size_++;
}

template <SubscriptableOrder PointType>
template <typename T>
void KDTree<PointType>::InsertRecursive(T&& point, std::unique_ptr<KDTree<PointType>::Node>& dest,
                                        size_t cur_dim) {
    cur_dim %= point.GetDim();

    if (dest == nullptr)
        dest = std::make_unique<Node>(std::forward<T>(point), nullptr, nullptr, cur_dim);

    else if (point[cur_dim] <= dest->point_[cur_dim]) {
        if (dest->left_ == nullptr) {
            dest->left_ = std::make_unique<Node>(std::forward<T>(point), nullptr, nullptr,
                                                 (cur_dim + 1) % point.GetDim());
        } else {
            InsertRecursive(std::forward<T>(point), dest->left_, cur_dim + 1);
        }
    } else {
        if (dest->right_ == nullptr) {
            dest->right_ = std::make_unique<Node>(std::forward<T>(point), nullptr, nullptr,
                                                  (cur_dim + 1) % point.GetDim());
        } else {
            InsertRecursive(std::forward<T>(point), dest->right_, cur_dim + 1);
        }
    }
}

}  // namespace util
