#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "core/algorithms/dd/dd.h"
#include "core/algorithms/dd/fastdd/model/differential_function.h"
#include "core/algorithms/dd/fastdd/util/bitset_concept.h"
#include "core/model/table/column_index.h"
#include "core/model/table/column_layout_typed_relation_data.h"
#include "core/model/types/type.h"

namespace algos::dd {

struct ThresholdInfo {
    double threshold;
    bool is_upper_bound;
    std::size_t dif_func_index;

    auto operator<=>(ThresholdInfo const& other) const {
        if (!model::IsEqual(threshold, other.threshold)) {
            return model::Less(threshold, other.threshold) ? std::strong_ordering::less
                                                           : std::strong_ordering::greater;
        }
        if (is_upper_bound != other.is_upper_bound) {
            return is_upper_bound < other.is_upper_bound ? std::strong_ordering::less
                                                         : std::strong_ordering::greater;
        }
        if (dif_func_index != other.dif_func_index) {
            if (!is_upper_bound) {
                return dif_func_index < other.dif_func_index ? std::strong_ordering::less
                                                             : std::strong_ordering::greater;
            } else {
                return dif_func_index > other.dif_func_index ? std::strong_ordering::less
                                                             : std::strong_ordering::greater;
            }
        }
        return std::strong_ordering::equal;
    }

    bool operator==(ThresholdInfo const& other) const {
        return model::IsEqual(threshold, other.threshold) &&
               is_upper_bound == other.is_upper_bound && dif_func_index == other.dif_func_index;
    }
};

class DifferentialFunctionBuilder {
private:
    std::vector<std::vector<DifferentialFunction>> differential_functions_;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    model::ColumnIndex num_columns_;

    std::vector<std::vector<ThresholdInfo>> thresholds_;
    std::vector<std::vector<std::size_t>> threshold_zones_;
    std::vector<std::vector<boost::dynamic_bitset<>>> zone_to_bitset_;
    std::vector<std::size_t> dif_func_nums_;
    std::vector<std::size_t> old_index_to_new_index_;

    void AddThresholds(std::vector<model::DFConstraint> const& thresholds,
                       model::ColumnIndex const column_index, std::size_t const offset,
                       model::DFConstraint const& min_max_dif);

    void CalculateThresholdZones();

public:
    DifferentialFunctionBuilder(
            std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation,
            model::ColumnIndex num_columns)
        : typed_relation_(typed_relation), num_columns_(num_columns) {}

    void BuildDFList(std::vector<std::vector<model::DFConstraint>> const& thresholds,
                     std::vector<model::DFConstraint> const& min_max_dif = {});
    void UpdateDFList(std::vector<model::DFConstraint> const& min_max_dif);

    template <BoostDynamicBitsetCompatible SourceBitset, BoostDynamicBitsetCompatible TargetBitset>
    TargetBitset TranslateBitset(SourceBitset const& bitset) const {
        TargetBitset result(GetDifFuncNum());

        for (std::size_t index = bitset.find_first(); index != SourceBitset::npos;
             index = bitset.find_next(index)) {
            if (old_index_to_new_index_[index] < bitset.size()) {
                result.set(old_index_to_new_index_[index]);
            }
        }

        return result;
    }

    std::vector<ThresholdInfo> const& GetThresholds(model::ColumnIndex const column_index) const {
        return thresholds_[column_index];
    }

    std::vector<std::size_t> const& GetThresholdZones(model::ColumnIndex const column_index) const {
        return threshold_zones_[column_index];
    }

    std::vector<std::vector<DifferentialFunction>> const& GetDifFuncs() const noexcept {
        return differential_functions_;
    }

    std::size_t GetDifFuncsSize() const noexcept {
        return differential_functions_.size();
    }

    std::size_t GetDifFuncNum() const noexcept {
        return dif_func_nums_[dif_func_nums_.size() - 1];
    }

    bool IsDistanceOrdered(model::ColumnIndex const column_index) const {
        return model::Type::IsDistanceOrdered(
                typed_relation_->GetColumnData(column_index).GetTypeId());
    }

    template <BoostDynamicBitsetCompatible Bitset>
    std::vector<Bitset> GetSatisfiedDFs(model::ColumnIndex const column_index) const {
        return std::vector<Bitset>(zone_to_bitset_[column_index].begin(),
                                   zone_to_bitset_[column_index].end());
    }

    std::size_t GetZoneNum(model::ColumnIndex const column_index) const {
        return zone_to_bitset_[column_index].size();
    }
};

}  // namespace algos::dd
