#pragma once

#include <assert.h>     // for assert
#include <memory>       // for shared_ptr, __s...
#include <optional>     // for optional
#include <string_view>  // for string_view
#include <vector>       // for vector

#include "config/equal_nulls/type.h"                  // for EqNullsType
#include "config/tabular_data/input_table_type.h"     // for InputTable
#include "fd_algorithm.h"                             // for FDAlgorithm
#include "model/table/column_layout_relation_data.h"  // for ColumnLayoutRel...
#include "table/column.h"                             // for Column

namespace algos {

class PliBasedFDAlgorithm : public FDAlgorithm {
public:
    class ColumnLayoutRelationDataManager {
    private:
        config::InputTable* input_table_;
        config::EqNullsType* is_null_equal_null_;
        std::shared_ptr<ColumnLayoutRelationData>* relation_;

    public:
        ColumnLayoutRelationDataManager(
                config::InputTable* input_table, config::EqNullsType* is_null_equal_null,
                std::shared_ptr<ColumnLayoutRelationData>* relation_ptr) noexcept
            : input_table_(input_table),
              is_null_equal_null_(is_null_equal_null),
              relation_(relation_ptr) {}

        std::shared_ptr<ColumnLayoutRelationData> GetRelation() const {
            if (*relation_ == nullptr)
                *relation_ =
                        ColumnLayoutRelationData::CreateFrom(**input_table_, *is_null_equal_null_);
            return *relation_;
        }
    };

private:
    config::InputTable input_table_;
    config::EqNullsType is_null_equal_null_;
    ColumnLayoutRelationDataManager const relation_manager_;

    void RegisterRelationManagerOptions();

protected:
    std::shared_ptr<ColumnLayoutRelationData> relation_;

    void LoadDataInternal() final;

    ColumnLayoutRelationData const& GetRelation() const noexcept {
        // GetRelation should be called after the dataset has been parsed, i.e. after algorithm
        // execution
        assert(relation_ != nullptr);
        return *relation_;
    }

public:
    PliBasedFDAlgorithm(std::vector<std::string_view> phase_names,
                        std::optional<ColumnLayoutRelationDataManager> relation_manager);

    std::vector<Column const*> GetKeys() const override;
};

}  // namespace algos
