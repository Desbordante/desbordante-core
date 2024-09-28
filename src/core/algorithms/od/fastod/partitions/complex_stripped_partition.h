#pragma once

#include <memory>
#include <string>
#include <vector>

#include "algorithms/od/fastod/storage/data_frame.h"
#include "algorithms/od/fastod/od_ordering.h"
#include "table/tuple_index.h"

namespace algos::fastod {

class ComplexStrippedPartition {
private:
    struct Tuple {
        model::TupleIndex tuple_index;
        int left_value;
        int right_value;

        // For vector::emplace_back() to work on x86 macos gcc
        Tuple(model::TupleIndex tuple_index, int left_value, int right_value)
            : tuple_index(tuple_index), left_value(left_value), right_value(right_value) {}
    };

    std::shared_ptr<std::vector<size_t>> sp_indexes_;
    std::shared_ptr<std::vector<size_t>> sp_begins_;
    std::shared_ptr<std::vector<DataFrame::Range>> rb_indexes_;
    std::shared_ptr<std::vector<size_t>> rb_begins_;
    std::shared_ptr<DataFrame> data_;
    bool is_stripped_partition_;
    bool should_be_converted_to_sp_;

    static constexpr inline double kSmallRangesRatioToConvert = 0.5;
    static constexpr inline size_t kMinMeaningfulRangeSize = static_cast<size_t>(40);

    std::string CommonToString() const;
    void CommonProduct(model::ColumnIndex attribute);
    bool CommonSplit(model::ColumnIndex right) const;

    std::string RangeBasedToString() const;
    void RangeBasedProduct(model::ColumnIndex attribute);
    bool RangeBasedSplit(model::ColumnIndex right) const;

    std::vector<DataFrame::ValueIndices> IntersectWithAttribute(model::ColumnIndex attribute,
                                                                size_t group_start,
                                                                size_t group_end);

    ComplexStrippedPartition(std::shared_ptr<DataFrame> data,
                             std::shared_ptr<std::vector<size_t>> indexes,
                             std::shared_ptr<std::vector<size_t>> begins);

    ComplexStrippedPartition(std::shared_ptr<DataFrame> data,
                             std::shared_ptr<std::vector<DataFrame::Range>> indexes,
                             std::shared_ptr<std::vector<size_t>> begins);
    std::vector<Tuple> GetTuplesForColumns(model::ColumnIndex left, model::ColumnIndex right,
                                           size_t group_ptr) const;

public:
    enum class Type { kStripped, kRangeBased };

    ComplexStrippedPartition();
    ComplexStrippedPartition(ComplexStrippedPartition const& origin) = default;

    ComplexStrippedPartition& operator=(ComplexStrippedPartition const& other);

    std::string ToString() const;
    void Product(model::ColumnIndex attribute);
    bool Split(model::ColumnIndex right) const;

    bool ShouldBeConvertedToStrippedPartition() const;
    void ToStrippedPartition();

    template <od::Ordering Ordering>
    bool Swap(model::ColumnIndex left, model::ColumnIndex right) const;

    template <Type PartitionType>
    static ComplexStrippedPartition Create(std::shared_ptr<DataFrame> data);
};

}  // namespace algos::fastod
