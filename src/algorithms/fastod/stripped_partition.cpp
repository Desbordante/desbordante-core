#include <optional>
#include <algorithm>
#include <cstdint>
#include <vector>
#include <map>
#include <sstream>

#include "schema_value.h"
#include "value_pair.h"
#include "operator.h"
#include "operator_type.h"
#include "stripped_partition.h"
#include "timer.h"

using namespace algos::fastod;

long StrippedPartition::merge_time_ = 0;
long StrippedPartition::validate_time_ = 0;
long StrippedPartition::clone_time_ = 0;
CacheWithLimit<AttributeSet, StrippedPartition> StrippedPartition::cache_(1e4);

StrippedPartition::StrippedPartition(const DataFrame& data) noexcept : data_(std::move(data)) {
    for (size_t i = 0; i < data.GetTupleCount(); i++) {
        indexes_.push_back(i);
    }

    if (data.GetTupleCount() != 0) {
        begins_.push_back(0);
    }

    begins_.push_back(data.GetTupleCount());
}

StrippedPartition::StrippedPartition(StrippedPartition const &origin) noexcept : data_(origin.data_) {
    indexes_ = std::vector<size_t>(origin.indexes_);
    begins_ = std::vector<size_t>(origin.begins_);
}

StrippedPartition StrippedPartition::Product(size_t attribute) noexcept {
    Timer timer = Timer(true);

    std::vector<size_t> new_indexes;
    std::vector<size_t> new_begins;
    size_t fill_pointer = 0;

    for (size_t begin_pointer = 0; begin_pointer < begins_.size() - 1; begin_pointer++) {
        size_t group_begin = begins_[begin_pointer];
        size_t group_end = begins_[begin_pointer + 1];
        // CHANGE: utilize column types
        std::map<SchemaValue, std::vector<size_t>> subgroups;

        for (size_t i = group_begin; i < group_end; i++) {
            size_t index = indexes_[i];
            auto value = data_.GetValue(index, attribute);

            if (subgroups.find(value) == subgroups.end()) {
                subgroups[value] = {};
            }

            subgroups[value].push_back(index);
        }

        for (auto [_, new_group]: subgroups){
            if (new_group.size() > 1){
                new_begins.push_back(fill_pointer);

                for (size_t i : new_group) {
                    new_indexes.push_back(i);
                    fill_pointer++;
                }
            }
        }
    }

    indexes_ = new_indexes;
    begins_ = new_begins;
    begins_.push_back(indexes_.size());

    merge_time_ += timer.GetElapsedSeconds();

    return *this;
}


bool StrippedPartition::Split(size_t right) noexcept {
    Timer timer = Timer(true);

    for (size_t begin_pointer = 0; begin_pointer <  begins_.size() - 1; begin_pointer++) {
        size_t group_begin = begins_[begin_pointer];
        size_t group_end = begins_[begin_pointer + 1];
        auto group_value = data_.GetValue(indexes_[group_begin], right);

        for (size_t i = group_begin + 1; i < group_end; i++) {
            size_t index = indexes_[i];
            auto value = data_.GetValue(index, right);

            if (value != group_value) {
                validate_time_ += timer.GetElapsedSeconds();

                return true;
            }
        }
    }

    validate_time_ += timer.GetElapsedSeconds();

    return false;
}

bool StrippedPartition::Swap(const SingleAttributePredicate& left, size_t right) noexcept {
    Timer timer = Timer(true);

    for (size_t begin_pointer = 0; begin_pointer <  begins_.size() - 1; begin_pointer++) {
        size_t group_begin = begins_[begin_pointer];
        size_t group_end = begins_[begin_pointer + 1];
        std::vector<ValuePair> values;

        for (size_t i = group_begin; i < group_end; i++) {
            size_t index = indexes_[i];

            values.push_back(ValuePair(
                data_.GetValue(index, left.GetAttribute()),
                data_.GetValue(index, right)
            ));
        }

        // CHANGE: utilize operators
        // SCOPE: from here until the end of this loop
        std::sort(values.begin(), values.end(), [&left](const ValuePair& a, const ValuePair& b) {
            return left.GetOperator().Satisfy(a.GetFirst(), b.GetFirst()) && a.GetFirst() != b.GetFirst();
        });

        size_t prev_group_max_index = 0;
        size_t current_group_max_index = 0;
        bool is_first_group = true;

        for (size_t i = 0; i < values.size(); i++) {
            auto first = values[i].GetFirst();
            auto second = values[i].GetSecond();

            // values are sorted by "first"
            if (i != 0 && values[i - 1].GetFirst() != first) {
                is_first_group = false;
                prev_group_max_index = current_group_max_index;
                current_group_max_index = i;
            }

            if (values[current_group_max_index].GetSecond() <= second) {
                current_group_max_index = i;
            }

            if (!is_first_group && values[prev_group_max_index].GetSecond() > second) {
                validate_time_ += timer.GetElapsedSeconds();
                return true;
            }
        }
    }

    validate_time_ += timer.GetElapsedSeconds();

    return false;
}

std::string StrippedPartition::ToString() const noexcept {
    std::stringstream ss;
    std::string indexes_string;

    for (size_t i = 0; i < indexes_.size(); i++) {
        if (i != 0) {
            indexes_string += ", ";
        }

        indexes_string += std::to_string(indexes_[i]);
    }

    std::string begins_string;

    for (size_t i = 0; i < begins_.size(); i++) {
        if (i != 0) {
            begins_string += ", ";
        }

        begins_string += std::to_string(begins_[i]);
    }

    ss << "StrippedPartition {indexes=" << indexes_string
        << ", begins=" << begins_string
        << "}";

    return ss.str();
}

StrippedPartition StrippedPartition::DeepClone() const noexcept {
    Timer timer = Timer(true);
    StrippedPartition result(data_);

    result.indexes_ = indexes_;
    result.begins_ = begins_;

    clone_time_ += timer.GetElapsedSeconds();
    
    return result;
}

StrippedPartition StrippedPartition::GetStrippedPartition(const AttributeSet& attribute_set, const DataFrame& data) noexcept {
    if (StrippedPartition::cache_.Contains(attribute_set)) {
        return StrippedPartition::cache_.Get(attribute_set);
    }

    std::optional<StrippedPartition> result;

    for (size_t attribute : attribute_set) {
        AttributeSet one_less = attribute_set.DeleteAttribute(attribute);
        
        if (StrippedPartition::cache_.Contains(one_less)) {
            result = StrippedPartition::cache_.Get(one_less).DeepClone().Product(attribute);
        }
    }

    if (!result.has_value()) {
        result = StrippedPartition(data);

        for (size_t attribute : attribute_set) {
            result.value().Product(attribute);
        }
    }

    StrippedPartition::cache_.Set(attribute_set, result.value());

    return result.value();
}

long StrippedPartition::SplitRemoveCount(size_t right) noexcept {
    Timer timer = Timer(true);
    long result = 0;

    for (size_t begin_pointer = 0; begin_pointer < begins_.size() - 1; begin_pointer++) {
        size_t group_begin = begins_[begin_pointer];
        size_t group_end = begins_[begin_pointer + 1];
        size_t group_length = group_end - group_begin;
        // CHANGE: key type according to column types
        std::map<SchemaValue, size_t> group_int_2_count;

        for (size_t i = group_begin; i < group_end; i++) {
            auto right_value = data_.GetValue(indexes_[i], right);
            
            if (group_int_2_count.find(right_value) != group_int_2_count.end()) {
                group_int_2_count[right_value] = group_int_2_count[right_value] + 1;
            } else {
                group_int_2_count[right_value] = 1;
            }
        }

        size_t max = 0;

        for (auto const& [_, count] : group_int_2_count) {
            max = std::max(max, count);
        }

        result += group_length - max;
    }
    
    validate_time_ += timer.GetElapsedSeconds();

    return result;
}

long StrippedPartition::SwapRemoveCount(const SingleAttributePredicate& left, size_t right) noexcept {
    std::size_t length = indexes_.size();
    std::vector<size_t> violations_count(length);
    std::vector<bool> deleted(length);
    size_t result = 0;

next_class:
    for (size_t begin_pointer = 0; begin_pointer < begins_.size() - 1; begin_pointer++) {
        size_t group_begin = begins_[begin_pointer];
        size_t group_end = begins_[begin_pointer + 1];

        for (size_t i = group_begin; i < group_end; i++) {
            // CHANGE: was FiltereDataFrameGet
            auto left_i = data_.GetValue(indexes_[i], left.GetAttribute());
            auto right_i = data_.GetValue(indexes_[i], right);

            for (size_t j = i + 1; j < group_end; j++) {
                // CHANGE: was FiltereDataFrameGet
                auto left_j = data_.GetValue(indexes_[j], left.GetAttribute());
                auto right_j = data_.GetValue(indexes_[j], right);

                // CHANGE: this comparison now uses operators
                // this is needed to get rid of FilteredDataFrameGet
                if (left_i != left_j
                    && right_i != right_j
                    && left.GetOperator().Satisfy(left_i, left_j)
                    && left.GetOperator().Violate(right_i, right_j)
                ) {
                    violations_count[i]++;
                    violations_count[j]++;
                }
            }
        }

        while (true) {
            std::optional<size_t> delete_index;

            for (size_t i = group_begin; i < group_end; i++) {
                if (!deleted[i] && (!delete_index.has_value() || violations_count[i] > violations_count[delete_index.value()])) {
                    delete_index = i;
                }
            }

            if (!delete_index.has_value() || violations_count[delete_index.value()] == 0) {
                goto next_class;
            }

            result++;
            deleted[delete_index.value()] = true;

            // CHANGE: was FiltereDataFrameGet
            auto left_i = data_.GetValue(indexes_[delete_index.value()], left.GetAttribute());
            auto right_i = data_.GetValue(indexes_[delete_index.value()], right);

            for (size_t j = group_begin; j < group_end; j++) {
                // CHANGE: was FiltereDataFrameGet
                auto left_j = data_.GetValue(indexes_[j], left.GetAttribute());
                auto right_j = data_.GetValue(indexes_[j], right);

                // CHANGE: this comparison now uses operators
                // this is needed to get rid of FilteredDataFrameGet
                if (left_i != left_j
                    && right_i != right_j
                    && left.GetOperator().Satisfy(left_i, left_j)
                    && left.GetOperator().Violate(right_i, right_j)
                ) {
                    violations_count[j]--;
                }
            }
        }
    }

    return result;
}

StrippedPartition& StrippedPartition::operator=(const StrippedPartition& other) {
    if (this == &other) {
        return *this;
    }
    
    indexes_ = other.indexes_;
    begins_ = other.begins_;
    merge_time_ = other.merge_time_;
    validate_time_ = other.validate_time_;
    clone_time_ = other.clone_time_;

    return *this;
}

