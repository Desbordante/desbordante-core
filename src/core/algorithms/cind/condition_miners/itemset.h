#pragma once

#include <cassert>
#include <functional>
#include <set>
#include <string>
#include <vector>

#include <boost/container_hash/hash.hpp>

#include "table/column_index.h"

namespace algos::cind {
struct Item {
    model::ColumnIndex column_id;
    int value;

    bool operator==(Item const& that) const {
        return this->column_id == that.column_id && this->value == that.value;
    }

    bool operator<(Item const& that) const {
        if (this->column_id != that.column_id) {
            return this->column_id < that.column_id;
        }
        return this->value < that.value;
    }

    std::string ToString() const noexcept {
        return "{" + std::to_string(column_id) + ", " + std::to_string(value) + "}";
    }
};

class Itemset {
public:
    Itemset(std::vector<Item> data, bool is_included)
        : data_(std::move(data)), is_included_(is_included) {}

    Itemset(std::vector<Item> const& data, size_t excluded_id, bool is_included)
        : is_included_(is_included) {
        assert(excluded_id < data.size());
        data_.reserve(data.size() - 1);
        for (size_t index = 0; index < data.size(); ++index) {
            if (index != excluded_id) {
                data_.emplace_back(data[index]);
            }
        }
    }

    Itemset() = default;

    bool IsIncluded() const noexcept {
        return is_included_;
    }

    size_t GetSize() const noexcept {
        return data_.size();
    }

    Item const& GetItem(size_t index) const {
        assert(index < GetSize());
        return data_.at(index);
    }

    Itemset Intersect(Itemset const& that) const {
        if (IsIncluded() == that.IsIncluded() && GetSize() == that.GetSize() &&
            GetItem(GetSize() - 1).column_id < that.GetItem(GetSize() - 1).column_id) {
            for (size_t index = 0; index < GetSize() - 1; ++index) {
                if (GetItem(index) != that.GetItem(index)) {
                    return {};
                }
            }
            std::vector<Item> data = data_;
            data.emplace_back(that.GetItem(GetSize() - 1));
            return Itemset(std::move(data), IsIncluded());
        }
        return {};
    }

    std::set<Itemset> GetSubsets() const {
        std::set<Itemset> result;
        for (size_t index = 0; index < GetSize(); ++index) {
            result.emplace(data_, index, is_included_);
        }
        return result;
    }

    bool operator<(Itemset const& that) const {
        return this->data_ < that.data_;
    }

    std::string ToString() const noexcept {
        std::string result = "[";
        result.append(is_included_ ? "Included, " : "Not included, ");
        for (auto const& item : data_) {
            result.append(item.ToString());
            result.append(", ");
        }
        result.pop_back();
        result.pop_back();
        result.append("]");
        return result;
    }

    bool operator==(Itemset const& other) const {
        return is_included_ == other.is_included_ && data_ == other.data_;
    }

private:
    std::vector<Item> data_;
    bool is_included_ = false;

    friend std::hash<algos::cind::Itemset>;
};
}  // namespace algos::cind

template <>
struct std::hash<algos::cind::Item> {
    size_t operator()(algos::cind::Item const& item) const {
        size_t hash = 0;
        boost::hash_combine(hash, boost::hash_value(item.column_id));
        boost::hash_combine(hash, boost::hash_value(item.value));
        return hash;
    }
};

template <>
struct std::hash<algos::cind::Itemset> {
    size_t operator()(algos::cind::Itemset const& itemset) const {
        std::hash<algos::cind::Item> item_hasher;
        size_t hash = 0;
        for (auto const& item : itemset.data_) {
            boost::hash_combine(hash, item_hasher(item));
        }
        return hash;
    }
};