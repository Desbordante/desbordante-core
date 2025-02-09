#pragma once

#include <memory>
#include <vector>

#include "column_encoded_relation_data.h"
#include "tabular_data/input_tables_type.h"

namespace model {
class EncodedTables {
public:
    explicit EncodedTables(config::InputTables& input_tables)
        : value_dictionary_(std::make_shared<ValueDictionary>()) {
        for (size_t table_id = 0; table_id < input_tables.size(); ++table_id) {
            input_tables[table_id]->Reset();
            tables_.push_back(ColumnEncodedRelationData::CreateFrom(input_tables[table_id],
                                                                    table_id, value_dictionary_));
        }
    }

    ColumnEncodedRelationData& GetTable(size_t index) const {
        assert(index < tables_.size());
        return *tables_[index];
    }

    const std::vector<std::unique_ptr<ColumnEncodedRelationData>>& GetTables() const noexcept {
        return tables_;
    }

private:
    std::vector<std::unique_ptr<ColumnEncodedRelationData>> tables_;
    ValueDictionaryType value_dictionary_;
};
}  // namespace model
