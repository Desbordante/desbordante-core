#include "complex_stripped_partition.h"

#include <sstream>

namespace algos::fastod {

ComplexStrippedPartition::ComplexStrippedPartition()
    : data_(nullptr), is_stripped_partition_(true), should_be_converted_to_sp_(false) {}

ComplexStrippedPartition::ComplexStrippedPartition(std::shared_ptr<DataFrame> data,
                                                   std::shared_ptr<std::vector<size_t>> indexes,
                                                   std::shared_ptr<std::vector<size_t>> begins)
    : sp_indexes_(indexes),
      sp_begins_(begins),
      data_(data),
      is_stripped_partition_(true),
      should_be_converted_to_sp_(false) {}

ComplexStrippedPartition::ComplexStrippedPartition(
        std::shared_ptr<DataFrame> data, std::shared_ptr<std::vector<DataFrame::Range>> indexes,
        std::shared_ptr<std::vector<size_t>> begins)
    : rb_indexes_(indexes),
      rb_begins_(begins),
      data_(data),
      is_stripped_partition_(false),
      should_be_converted_to_sp_(false) {}

ComplexStrippedPartition& ComplexStrippedPartition::operator=(
        ComplexStrippedPartition const& other) {
    if (this == &other) {
        return *this;
    }

    sp_indexes_ = other.sp_indexes_;
    sp_begins_ = other.sp_begins_;
    rb_indexes_ = other.rb_indexes_;
    rb_begins_ = other.rb_begins_;
    data_ = other.data_;

    should_be_converted_to_sp_ = other.should_be_converted_to_sp_;
    is_stripped_partition_ = other.is_stripped_partition_;

    return *this;
}

std::string ComplexStrippedPartition::ToString() const {
    return is_stripped_partition_ ? CommonToString() : RangeBasedToString();
}

void ComplexStrippedPartition::Product(model::ColumnIndex attribute) {
    if (is_stripped_partition_) {
        CommonProduct(attribute);
    } else {
        RangeBasedProduct(attribute);
    }
}

bool ComplexStrippedPartition::Split(model::ColumnIndex right) const {
    return is_stripped_partition_ ? CommonSplit(right) : RangeBasedSplit(right);
}

bool ComplexStrippedPartition::ShouldBeConvertedToStrippedPartition() const {
    return should_be_converted_to_sp_;
}

void ComplexStrippedPartition::ToStrippedPartition() {
    sp_begins_ = std::make_unique<std::vector<size_t>>();
    sp_indexes_ = std::make_unique<std::vector<size_t>>();

    size_t sp_begin = 0;
    sp_begins_->push_back(sp_begin);

    for (size_t begin_pointer = 0; begin_pointer < rb_begins_->size() - 1; begin_pointer++) {
        const size_t group_begin = (*rb_begins_)[begin_pointer];
        const size_t group_end = (*rb_begins_)[begin_pointer + 1];

        for (size_t i = group_begin; i < group_end; ++i) {
            const DataFrame::Range range = (*rb_indexes_)[i];
            sp_begin += RangeSize(range);

            for (size_t sp_index = range.first; sp_index <= range.second; ++sp_index) {
                sp_indexes_->push_back(sp_index);
            }
        }

        sp_begins_->push_back(sp_begin);
    }

    rb_begins_->clear();
    rb_indexes_->clear();

    is_stripped_partition_ = true;
    should_be_converted_to_sp_ = false;
}

template <bool Ascending>
bool ComplexStrippedPartition::Swap(model::ColumnIndex left, model::ColumnIndex right) const {
    size_t const group_count = is_stripped_partition_ ? sp_begins_->size() : rb_begins_->size();

    for (size_t begin_pointer = 0; begin_pointer < group_count - 1; begin_pointer++) {
        size_t const group_begin = is_stripped_partition_ ? (*sp_begins_)[begin_pointer]
                                                          : (*rb_begins_)[begin_pointer];

        size_t const group_end = is_stripped_partition_ ? (*sp_begins_)[begin_pointer + 1]
                                                        : (*rb_begins_)[begin_pointer + 1];

        std::vector<std::pair<int, int>> values;

        if (is_stripped_partition_) {
            values.reserve(group_end - group_begin);

            for (size_t i = group_begin; i < group_end; ++i) {
                size_t const index = (*sp_indexes_)[i];

                values.emplace_back(data_->GetValue(index, left), data_->GetValue(index, right));
            }
        } else {
            for (size_t i = group_begin; i < group_end; ++i) {
                DataFrame::Range const range = (*rb_indexes_)[i];

                for (size_t j = range.first; j <= range.second; ++j) {
                    values.emplace_back(data_->GetValue(j, left), data_->GetValue(j, right));
                }
            }
        }

        if constexpr (Ascending) {
            std::sort(values.begin(), values.end(),
                      [](auto const& p1, auto const& p2) { return p1.first < p2.first; });
        } else {
            std::sort(values.begin(), values.end(),
                      [](auto const& p1, auto const& p2) { return p2.first < p1.first; });
        }

        size_t prev_group_max_index = 0;
        size_t current_group_max_index = 0;
        bool is_first_group = true;

        for (size_t i = 0; i < values.size(); i++) {
            auto const& [first, second] = values[i];

            if (i != 0 && values[i - 1].first != first) {
                is_first_group = false;
                prev_group_max_index = current_group_max_index;
                current_group_max_index = i;
            } else if (values[current_group_max_index].second <= second) {
                current_group_max_index = i;
            }

            if (!is_first_group && values[prev_group_max_index].second > second) {
                return true;
            }
        }
    }

    return false;
}

template bool ComplexStrippedPartition::Swap<true>(model::ColumnIndex left,
                                                   model::ColumnIndex right) const;
template bool ComplexStrippedPartition::Swap<false>(model::ColumnIndex left,
                                                    model::ColumnIndex right) const;

template <bool RangeBasedMode>
ComplexStrippedPartition ComplexStrippedPartition::Create(std::shared_ptr<DataFrame> data) {
    if constexpr (RangeBasedMode) {
        auto rb_indexes = std::make_unique<std::vector<DataFrame::Range>>();
        auto rb_begins = std::make_unique<std::vector<size_t>>();

        size_t const tuple_count = data->GetTupleCount();
        rb_begins->push_back(0);

        if (tuple_count != 0) {
            rb_indexes->push_back({0, tuple_count - 1});
            rb_begins->push_back(1);
        }

        return ComplexStrippedPartition(std::move(data), std::move(rb_indexes),
                                        std::move(rb_begins));
    }

    auto sp_indexes = std::make_unique<std::vector<size_t>>();
    auto sp_begins = std::make_unique<std::vector<size_t>>();

    sp_indexes->reserve(data->GetTupleCount());

    for (size_t i = 0; i < data->GetTupleCount(); i++) {
        sp_indexes->push_back(i);
    }

    if (data->GetTupleCount() != 0) {
        sp_begins->push_back(0);
    }

    sp_begins->push_back(data->GetTupleCount());

    return ComplexStrippedPartition(std::move(data), std::move(sp_indexes), std::move(sp_begins));
}

template ComplexStrippedPartition ComplexStrippedPartition::Create<true>(
        std::shared_ptr<DataFrame> data);
template ComplexStrippedPartition ComplexStrippedPartition::Create<false>(
        std::shared_ptr<DataFrame> data);

std::string ComplexStrippedPartition::CommonToString() const {
    std::stringstream result;
    std::string indexes_string;

    for (size_t i = 0; i < sp_indexes_->size(); i++) {
        if (i != 0) {
            indexes_string += ", ";
        }

        indexes_string += std::to_string((*sp_indexes_)[i]);
    }

    std::string begins_string;

    for (size_t i = 0; i < sp_begins_->size(); i++) {
        if (i != 0) {
            begins_string += ", ";
        }

        begins_string += std::to_string((*sp_begins_)[i]);
    }

    result << "ComplexStrippedPartition[SP mode] { indexes = [ " << indexes_string
           << " ]; begins = [ " << begins_string << " ] }";

    return result.str();
}

void ComplexStrippedPartition::CommonProduct(model::ColumnIndex attribute) {
    auto new_indexes = std::make_unique<std::vector<size_t>>();
    new_indexes->reserve(data_->GetColumnCount());

    auto new_begins = std::make_unique<std::vector<size_t>>();
    size_t fill_pointer = 0;

    for (size_t begin_pointer = 0; begin_pointer < sp_begins_->size() - 1; begin_pointer++) {
        const size_t group_begin = (*sp_begins_)[begin_pointer];
        const size_t group_end = (*sp_begins_)[begin_pointer + 1];

        std::vector<std::pair<int, size_t>> values(group_end - group_begin);

        for (size_t i = group_begin; i < group_end; i++) {
            const size_t index = (*sp_indexes_)[i];
            values[i - group_begin] = {data_->GetValue(index, attribute), index};
        }

        std::sort(values.begin(), values.end(),
                  [](auto const& p1, auto const& p2) { return p1.first < p2.first; });

        size_t group_start = 0;
        size_t i = 1;

        auto add_group = [&]() {
            const size_t group_size = i - group_start;

            if (group_size > 1) {
                new_begins->push_back(fill_pointer);
                fill_pointer += group_size;

                for (size_t j = group_start; j < group_start + group_size; ++j) {
                    new_indexes->push_back(values[j].second);
                }
            }

            group_start = i;
        };

        for (; i < values.size(); ++i) {
            if (values[i - 1].first != values[i].first) add_group();
        }

        add_group();
    }

    new_begins->push_back(new_indexes->size());

    sp_indexes_ = std::move(new_indexes);
    sp_begins_ = std::move(new_begins);
}

bool ComplexStrippedPartition::CommonSplit(model::ColumnIndex right) const {
    for (size_t begin_pointer = 0; begin_pointer < sp_begins_->size() - 1; begin_pointer++) {
        const size_t group_begin = (*sp_begins_)[begin_pointer];
        const size_t group_end = (*sp_begins_)[begin_pointer + 1];

        int const group_value = data_->GetValue((*sp_indexes_)[group_begin], right);

        for (size_t i = group_begin + 1; i < group_end; i++) {
            if (data_->GetValue((*sp_indexes_)[i], right) != group_value) {
                return true;
            }
        }
    }

    return false;
}

std::string ComplexStrippedPartition::RangeBasedToString() const {
    std::stringstream result;
    std::string indexes_string;

    for (size_t i = 0; i < rb_indexes_->size(); i++) {
        if (i != 0) {
            indexes_string += ", ";
        }

        indexes_string += "(" + std::to_string((*rb_indexes_)[i].first) + ";" +
                          std::to_string((*rb_indexes_)[i].second) + ")";
    }

    std::string begins_string;

    for (size_t i = 0; i < rb_begins_->size(); i++) {
        if (i != 0) {
            begins_string += ", ";
        }

        begins_string += std::to_string((*rb_begins_)[i]);
    }

    result << "ComplexStrippedPartition[RB mode] { indexes = [ " << indexes_string
           << " ]; begins = [ " << begins_string << " ] }";

    return result.str();
}

void ComplexStrippedPartition::RangeBasedProduct(model::ColumnIndex attribute) {
    auto new_begins = std::make_unique<std::vector<size_t>>();
    auto new_indexes = std::make_unique<std::vector<DataFrame::Range>>();

    size_t curr_begin = 0;

    for (size_t begin_pointer = 0; begin_pointer < rb_begins_->size() - 1; ++begin_pointer) {
        const size_t group_begin = (*rb_begins_)[begin_pointer];
        const size_t group_end = (*rb_begins_)[begin_pointer + 1];

        std::vector<DataFrame::ValueIndices> intersection =
                IntersectWithAttribute(attribute, group_begin, group_end - 1);

        const size_t intersection_size = intersection.size();
        size_t small_ranges_count = 0;

        auto add_group = [&new_indexes, &new_begins, &intersection, &curr_begin,
                          &small_ranges_count](size_t start_index, size_t end_index) {
            if (start_index == end_index) {
                DataFrame::Range range = intersection[start_index].second;

                if (range.second == range.first) {
                    return;
                }
            }

            for (size_t i = start_index; i <= end_index; ++i) {
                DataFrame::Range const& range = intersection[i].second;

                if (RangeSize(range) < kMinMeaningfulRangeSize) {
                    small_ranges_count++;
                }

                new_indexes->push_back(std::move(range));
            }

            new_begins->push_back(curr_begin);
            curr_begin += end_index - start_index + 1;
        };

        size_t group_start = 0;

        for (size_t i = 1; i < intersection_size; ++i) {
            if (intersection[i].first != intersection[i - 1].first) {
                add_group(group_start, i - 1);
                group_start = i;
            }
        }

        add_group(group_start, intersection_size - 1);

        if (!should_be_converted_to_sp_ && intersection_size > 0 &&
            small_ranges_count / static_cast<double>(intersection_size) >=
                    kSmallRangesRatioToConvert) {
            should_be_converted_to_sp_ = true;
        }
    }

    new_begins->push_back(new_indexes->size());

    rb_indexes_ = std::move(new_indexes);
    rb_begins_ = std::move(new_begins);
}

bool ComplexStrippedPartition::RangeBasedSplit(model::ColumnIndex right) const {
    for (size_t begin_pointer = 0; begin_pointer < rb_begins_->size() - 1; ++begin_pointer) {
        const size_t group_begin = (*rb_begins_)[begin_pointer];
        const size_t group_end = (*rb_begins_)[begin_pointer + 1];

        int const group_value = data_->GetValue((*rb_indexes_)[group_begin].first, right);

        for (size_t i = group_begin; i < group_end; ++i) {
            const DataFrame::Range range = (*rb_indexes_)[i];

            for (size_t j = range.first; j <= range.second; ++j) {
                if (data_->GetValue(j, right) != group_value) {
                    return true;
                }
            }
        }
    }

    return false;
}

std::vector<DataFrame::ValueIndices> ComplexStrippedPartition::IntersectWithAttribute(
        model::ColumnIndex attribute, size_t group_start, size_t group_end) {
    std::vector<DataFrame::ValueIndices> result;

    std::vector<DataFrame::ValueIndices> const& attr_ranges = data_->GetDataRanges()[attribute];

    for (size_t i = group_start; i <= group_end; ++i) {
        DataFrame::Range const& range = (*rb_indexes_)[i];

        const size_t lower_bound_range_index = data_->GetRangeIndexByItem(range.first, attribute);
        const size_t upper_bound_range_index = data_->GetRangeIndexByItem(range.second, attribute);

        for (size_t j = lower_bound_range_index; j <= upper_bound_range_index; ++j) {
            DataFrame::ValueIndices const& attr_value_range = attr_ranges[j];
            DataFrame::Range const& attr_range = attr_value_range.second;

            const size_t start = std::max(range.first, attr_range.first);
            const size_t end = std::min(range.second, attr_range.second);

            if (start <= end) {
                result.push_back({attr_value_range.first, {start, end}});
            }
        }
    }

    std::sort(result.begin(), result.end(),
              [](auto const& x, auto const& y) { return x.first < y.first; });

    return result;
}

}  // namespace algos::fastod
