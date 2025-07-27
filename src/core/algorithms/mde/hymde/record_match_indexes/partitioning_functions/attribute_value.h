#pragma once

#include <memory>
#include <string>

#include "algorithms/mde/hymde/record_match_indexes/partitioning_functions/partitioning_function.h"
#include "config/exceptions.h"
#include "model/index.h"
#include "model/table/column.h"
#include "model/table/relational_schema.h"
#include "model/types/builtin.h"

namespace algos::hymde::record_match_indexes::partitioning_functions {
template <typename R>
class AttributeValue final : public PartitioningFunction<R> {
    model::Index index_;
    records::DictionaryCompressed::Values const& values_;

public:
    using Type = R;

    class Creator final : public PartitioningFunction<R>::Creator {
    public:
        using ColumnIdentifier = std::variant<std::string, model::Index>;

    private:
        ColumnIdentifier identifier_;

    public:
        std::unique_ptr<PartitioningFunction<R>> Create(
                RelationalSchema const& schema,
                records::DictionaryCompressed::Values const& values) const final {
            auto [name, index] = std::visit(
                    [&](auto&& arg) {
                        Column const* column = schema.GetColumn(arg);
                        return std::pair{column->GetName(), column->GetIndex()};
                    },
                    identifier_);
            return std::make_unique<AttributeValue>(std::move(name), index, values);
        }

        void CheckSchema(RelationalSchema const& schema) const final {
            std::visit(
                    [&](auto&& arg) {
                        using T = std::remove_cvref_t<decltype(arg)>;
                        if constexpr (std::is_same_v<T, std::string>) {
                            if (!schema.IsColumnInSchema(arg))
                                throw config::ConfigurationError("No column named \"" + arg +
                                                                 "\" in the " + schema.GetName() +
                                                                 " table");
                        } else {
                            static_assert(std::is_same_v<T, model::Index>);
                            static_assert(std::is_unsigned_v<model::Index>);
                            std::size_t const num_columns = schema.GetNumColumns();
                            if (arg >= num_columns)
                                throw config::ConfigurationError(
                                        "No column with index " + std::to_string(arg) + " in the " +
                                        schema.GetName() + " table, this table only has " +
                                        std::to_string(num_columns) + " columns.");
                        }
                    },
                    identifier_);
        }

        Creator(ColumnIdentifier identifier) : identifier_(std::move(identifier)) {}
    };

    R GetValue(records::DictionaryCompressed::CompressedRecord const& record) const final {
        assert(record[index_] < values_.size());
        if constexpr (std::is_same_v<R, std::string>) {
            return values_[record[index_]];
        } else {
            return model::TypeConverter<R>::kConvert(values_[record[index_]]);
        }
    }

    AttributeValue(std::string name, model::Index index,
                   records::DictionaryCompressed::Values const& values)
        : PartitioningFunction<R>(std::move(name)), index_(index), values_(values) {}
};
}  // namespace algos::hymde::record_match_indexes::partitioning_functions
