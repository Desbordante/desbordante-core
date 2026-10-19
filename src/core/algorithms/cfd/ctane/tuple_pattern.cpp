#include "core/algorithms/cfd/ctane/tuple_pattern.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace algos::cfd {

TuplePattern::TuplePattern(std::size_t columns_number, PatternValues pattern_values)
    : pattern_values_(std::move(pattern_values)), column_indices_(columns_number) {
    for (auto const& pattern_value : pattern_values_) {
        auto const column_index = pattern_value.first;
        if (column_index < 0 || static_cast<std::size_t>(column_index) >= columns_number) {
            throw std::out_of_range("Tuple pattern column index is out of range");
        }
        column_indices_.set(static_cast<std::size_t>(column_index));
    }
}

std::size_t TuplePattern::Size() const {
    return column_indices_.count();
}

TuplePattern::PatternValues const& TuplePattern::GetPatternValues() const {
    return pattern_values_;
}

Item TuplePattern::GetPatternValue(AttributeIndex column_index) const {
    return pattern_values_.at(column_index);
}

TuplePattern::ColumnIndices const& TuplePattern::GetColumnIndices() const {
    return column_indices_;
}

bool TuplePattern::HasColumnPattern(AttributeIndex column_index, Item item) const {
    auto const it = pattern_values_.find(column_index);
    return it != pattern_values_.end() && it->second == item;
}

bool TuplePattern::IsConst() const {
    return std::ranges::all_of(pattern_values_, [](auto const& entry) { return entry.second > 0; });
}

bool TuplePattern::IsVar() const {
    return std::ranges::all_of(pattern_values_, [](auto const& entry) { return entry.second < 0; });
}

TuplePattern TuplePattern::GetWithoutColumn(AttributeIndex column_index) const {
    PatternValues values = pattern_values_;
    values.erase(column_index);
    return TuplePattern(column_indices_.size(), std::move(values));
}

Itemset TuplePattern::ToItemset() const {
    Itemset items;
    items.reserve(pattern_values_.size());
    for (auto const& [column_index, item] : pattern_values_) {
        static_cast<void>(column_index);
        items.push_back(item);
    }
    return items;
}

bool TuplePattern::operator==(TuplePattern const& rhs) const {
    return column_indices_ == rhs.column_indices_ && pattern_values_ == rhs.pattern_values_;
}

bool TuplePattern::operator<=(TuplePattern const& rhs) const {
    for (auto const& [column_index, item] : pattern_values_) {
        auto const rhs_it = rhs.pattern_values_.find(column_index);
        if (rhs_it == rhs.pattern_values_.end()) return false;

        Item const rhs_item = rhs_it->second;
        Item const wildcard = -1 - column_index;
        if (item != rhs_item && rhs_item != wildcard) return false;
    }
    return true;
}

bool TuplePattern::IsMoreGeneralThan(TuplePattern const& rhs) const {
    return rhs <= *this && !(*this <= rhs);
}

TuplePattern TuplePattern::UnionTuplePatterns(TuplePattern const& lhs, TuplePattern const& rhs) {
    if (lhs.column_indices_.size() != rhs.column_indices_.size()) {
        throw std::invalid_argument("Cannot unite tuple patterns of different relations");
    }

    PatternValues values = lhs.pattern_values_;
    for (auto const& [column_index, item] : rhs.pattern_values_) {
        auto const [it, inserted] = values.emplace(column_index, item);
        if (!inserted && it->second != item) {
            throw std::invalid_argument("Cannot unite conflicting tuple patterns");
        }
    }
    return TuplePattern(lhs.column_indices_.size(), std::move(values));
}

}  // namespace algos::cfd
