#pragma once

#include <string>
#include <vector>

namespace algos::nd_verifier::util {

template <typename V>
struct ValueCombination {
private:
    std::vector<V> data_;
    size_t size_;

public:
    explicit ValueCombination(std::vector<V> data) : data_(data), size_(data.size()) {}

    explicit ValueCombination(std::initializer_list<V> init_list)
        : data_(init_list), size_(init_list.size()) {}

    ValueCombination() : data_(), size_(0) {}

    ValueCombination(ValueCombination<V> const& rhs) = default;
    ValueCombination(ValueCombination<V>&& rhs) = default;
    ValueCombination<V>& operator=(ValueCombination<V> const& rhs) = default;
    ValueCombination<V>& operator=(ValueCombination<V>&& rhs) = default;
    ~ValueCombination() = default;

    auto Size() const {
        return size_;
    }

    auto begin() const {
        return data_.begin();
    }

    auto end() const {
        return data_.end();
    }

    std::string ToString() const {
        std::stringstream ss;
        if (size() == 1) {
            ss << data_[0];
        } else {
            ss << '(';
            for (auto pt{begin()}; pt != end(); ++pt) {
                if (pt != begin()) ss << ", ";
                ss << *pt;
            }
            ss << ')';
        }
        return ss.str();
    }

    bool operator==(ValueCombination<V> const& rhs) const {
        return !(*this != rhs);
    }

    bool operator!=(ValueCombination<V> const& rhs) const {
        if (size() != rhs.size()) return true;

        for (size_t i{0}; i < size(); ++i) {
            if (data_.at(i) != rhs.data_.at(i)) return true;
        }

        return false;
    }

    bool operator<(ValueCombination<V> const& rhs) const {
        if (size() < rhs.size()) {
            return true;
        } else if (size() > rhs.size()) {
            return false;
        }

        for (size_t i{0}; i < size(); ++i) {
            auto const& lhs_value = data_.at(i);
            auto const& rhs_value = rhs.data_.at(i);

            if (lhs_value < rhs_value) {
                return true;
            } else if (lhs_value > rhs_value) {
                return false;
            }
        }

        return true;
    }
};

template <typename V>
std::ostream& operator<<(std::ostream& os, ValueCombination<V> const& vc) {
    os << vc.ToString();
    return os;
}

}  // namespace algos::nd_verifier::util
