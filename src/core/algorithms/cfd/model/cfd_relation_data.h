#pragma once

#include <cstddef>
#include <utility>

#include "core/algorithms/cfd/model/cfd_column_data.h"
#include "core/algorithms/cfd/model/cfd_types.h"
#include "core/model/table/idataset_stream.h"
#include "core/model/table/relation_data.h"

// see algorithms/cfd/LICENSE

namespace algos::cfd {

// Data presentation class that CFDDiscovery uses.
class CFDRelationData : public AbstractRelationData<CFDColumnData> {
private:
    // ItemInfo contains info about one elem in the table.
    struct ItemInfo {
        ItemInfo() = default;

        ItemInfo(std::string val, AttributeIndex attr)
            : value(std::move(val)), attribute(attr), frequency(0) {}

        std::string value;
        AttributeIndex attribute;
        unsigned frequency;
    };

    // array of data represented as rows of integers
    std::vector<Transaction> data_rows_;
    std::vector<ItemInfo> items_;

public:
    unsigned GetAttrsNumber() const;
    size_t GetNumRows() const override;
    Transaction const& GetRow(unsigned) const;
    AttributeIndex GetAttrIndex(int item_index) const;
    unsigned Frequency(int i) const;
    std::string const& GetValue(int i) const;
    std::vector<int> const& GetDomainOfItem(int) const;
    std::vector<int> const& GetDomain(unsigned attr) const;
    std::vector<int> GetAttrVector(Itemset const&) const;
    std::vector<int> GetAttrVectorItems(Itemset const&) const;
    std::string GetAttrName(int index) const;
    int GetAttr(std::string const&) const;
    int GetItem(int, std::string const&) const;

    static std::unique_ptr<CFDRelationData> CreateFrom(model::IDatasetStream& data_stream);

    CFDRelationData(std::unique_ptr<RelationalSchema> schema,
                    std::vector<CFDColumnData> column_data, std::vector<Transaction> data,
                    std::vector<ItemInfo> items)
        : AbstractRelationData(std::move(schema), std::move(column_data)),
          data_rows_(std::move(data)),
          items_(std::move(items)) {}
};
}  // namespace algos::cfd
