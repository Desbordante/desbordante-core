#pragma once

#include <memory>
#include <string>
#include <vector>

#include "algorithms/od/fastod/storage/data_frame.h"

namespace algos::fastod {

class ComplexStrippedPartition {
private:
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

public:
    ComplexStrippedPartition();
    ComplexStrippedPartition(ComplexStrippedPartition const& origin) = default;

    ComplexStrippedPartition& operator=(ComplexStrippedPartition const& other);

    std::string ToString() const;
    void Product(model::ColumnIndex attribute);
    bool Split(model::ColumnIndex right) const;

    bool ShouldBeConvertedToStrippedPartition() const;
    void ToStrippedPartition();

    template <bool Ascending>
    bool Swap(model::ColumnIndex left, model::ColumnIndex right) const;
    template <bool RangeBasedMode>
    static ComplexStrippedPartition Create(std::shared_ptr<DataFrame> data);
};

}  // namespace algos::fastod
