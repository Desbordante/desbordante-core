#pragma once

#include <string>
#include <type_traits>
#include <variant>

#include "algorithms/md/hymd/preprocessing/similarity_measure/similarity_measure.h"
#include "config/exceptions.h"
#include "model/index.h"
#include "model/table/column.h"
#include "model/table/relational_schema.h"

namespace algos::hymd {

using ColumnIdentifier = std::variant<std::string, model::Index>;

class SimilarityMeasureCreator {
private:
    std::string const similarity_measure_name_;
    ColumnIdentifier left_column_identifier_;
    ColumnIdentifier right_column_identifier_;

public:
    SimilarityMeasureCreator(std::string similarity_measure_name,
                             ColumnIdentifier column1_identifier,
                             ColumnIdentifier column2_identifier) noexcept
        : similarity_measure_name_(std::move(similarity_measure_name)),
          left_column_identifier_(std::move(column1_identifier)),
          right_column_identifier_(std::move(column2_identifier)) {}

    std::string const& GetSimilarityMeasureName() const noexcept {
        return similarity_measure_name_;
    }

    std::pair<model::Index, model::Index> GetIndices(RelationalSchema const& left_schema,
                                                     RelationalSchema const& right_schema) const {
        auto get_index = [&](RelationalSchema const& schema, ColumnIdentifier const& identifier) {
            return std::visit([&](auto&& arg) { return schema.GetColumn(arg)->GetIndex(); },
                              identifier);
        };
        return {get_index(left_schema, left_column_identifier_),
                get_index(right_schema, right_column_identifier_)};
    }

    void CheckIndices(RelationalSchema const& left_schema, RelationalSchema const& right_schema) {
        auto check_index = [&](RelationalSchema const& schema, ColumnIdentifier const& identifier,
                               auto&& name) {
            std::visit(
                    [&](auto&& arg) {
                        using T = std::remove_cvref_t<decltype(arg)>;
                        if constexpr (std::is_same_v<T, std::string>) {
                            if (!schema.IsColumnInSchema(arg))
                                throw config::ConfigurationError("No column named \"" + arg +
                                                                 "\" in the " + name + "table");
                        } else {
                            static_assert(std::is_same_v<T, model::Index>);
                            std::size_t const num_columns = schema.GetNumColumns();
                            if (arg >= num_columns)
                                throw config::ConfigurationError(
                                        "No column with index " + std::to_string(arg) + " in the " +
                                        name + " table, this table only has " +
                                        std::to_string(num_columns) + " columns.");
                        }
                    },
                    identifier);
        };
        check_index(left_schema, left_column_identifier_, "left");
        check_index(right_schema, right_column_identifier_, "right");
    }

    virtual std::unique_ptr<preprocessing::similarity_measure::SimilarityMeasure> MakeMeasure(
            util::WorkerThreadPool* thread_pool) const = 0;

    std::unique_ptr<preprocessing::similarity_measure::SimilarityMeasure> MakeMeasure() {
        return MakeMeasure(nullptr);
    }

    virtual ~SimilarityMeasureCreator() = default;
};

}  // namespace algos::hymd
