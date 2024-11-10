#pragma once

#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "algorithms/md/hymd/indexes/global_value_identifier.h"
#include "algorithms/md/hymd/preprocessing/column_matches/column_match.h"
#include "config/exceptions.h"
#include "model/index.h"
#include "model/table/column.h"

namespace algos::hymd::preprocessing::column_matches {
using ColumnIdentifier = std::variant<std::string, model::Index>;

template <typename Transformer, typename Calculator>
class ColumnMatchImpl : public ColumnMatch {
    Transformer transformer_;
    Calculator calculator_;
    ColumnIdentifier left_column_identifier_;
    ColumnIdentifier right_column_identifier_;
    model::Index left_column_index_;
    model::Index right_column_index_;

public:
    ColumnMatchImpl(bool is_symmetrical_and_eq_is_max, std::string name,
                    ColumnIdentifier left_column_identifier,
                    ColumnIdentifier right_column_identifier, Transformer transformer,
                    Calculator calculator)
        : ColumnMatch(is_symmetrical_and_eq_is_max, std::move(name)),
          transformer_(std::move(transformer)),
          calculator_(std::move(calculator)),
          left_column_identifier_(std::move(left_column_identifier)),
          right_column_identifier_(std::move(right_column_identifier)) {}

    [[nodiscard]] indexes::ColumnPairMeasurements MakeIndexes(
            util::WorkerThreadPool* pool_ptr,
            indexes::RecordsInfo const& records_info) const final {
        indexes::KeyedPositionListIndex const& right_pli =
                records_info.GetRightCompressor().GetPli(right_column_index_);
        auto transformed = transformer_.Transform(
                records_info.GetValues(),
                records_info.GetLeftCompressor().GetPli(left_column_index_).GetValueIds(),
                right_pli.GetValueIds());
        return calculator_.Calculate(transformed.left_ptr, transformed.right_ptr, right_pli,
                                     pool_ptr);
    }

    void SetColumns(RelationalSchema const& left_schema,
                    RelationalSchema const& right_schema) final {
        auto check_and_set_index = [](RelationalSchema const& schema,
                                      ColumnIdentifier const& identifier,
                                      model::Index& column_index, auto&& name) {
            std::visit(
                    [&](auto&& arg) {
                        using T = std::remove_cvref_t<decltype(arg)>;
                        if constexpr (std::is_same_v<T, std::string>) {
                            if (!schema.IsColumnInSchema(arg))
                                throw config::ConfigurationError("No column named \"" + arg +
                                                                 "\" in the " + name + " table");
                        } else {
                            static_assert(std::is_same_v<T, model::Index>);
                            static_assert(std::is_unsigned_v<model::Index>);
                            std::size_t const num_columns = schema.GetNumColumns();
                            if (arg >= num_columns)
                                throw config::ConfigurationError(
                                        "No column with index " + std::to_string(arg) + " in the " +
                                        name + " table, this table only has " +
                                        std::to_string(num_columns) + " columns.");
                        }
                        column_index = schema.GetColumn(arg)->GetIndex();
                    },
                    identifier);
        };
        check_and_set_index(left_schema, left_column_identifier_, left_column_index_, "left");
        check_and_set_index(right_schema, right_column_identifier_, right_column_index_, "right");
    }

    std::pair<model::Index, model::Index> GetIndices() const noexcept final {
        return {left_column_index_, right_column_index_};
    }
};
}  // namespace algos::hymd::preprocessing::column_matches
