#include "order_utility.h"

#include <easylogging++.h>

#include "list_lattice.h"
#include "model/table/column_index.h"
#include "model/table/typed_column_data.h"

namespace algos::order {

void PrintOD(AttributeList const& lhs, AttributeList const& rhs) {
    for (model::ColumnIndex attr : lhs) {
        LOG(DEBUG) << attr << " ";
    }
    LOG(DEBUG) << "-> ";
    for (model::ColumnIndex attr : rhs) {
        LOG(DEBUG) << attr << " ";
    }
}

std::vector<AttributeList> GetPrefixes(Node const& node) {
    std::vector<AttributeList> res;
    res.reserve(node.size() - 1);
    for (size_t i = 1; i < node.size(); ++i) {
        res.emplace_back(node.begin(), node.begin() + i);
    }
    return res;
}

AttributeList MaxPrefix(AttributeList const& attribute_list) {
    return AttributeList(attribute_list.begin(), attribute_list.end() - 1);
}

bool InUnorderedMap(OrderDependencies const& map, AttributeList const& lhs,
                    AttributeList const& rhs) {
    auto it = map.find(lhs);
    return it != map.end() && it->second.find(rhs) != it->second.end();
}

bool AreDisjoint(AttributeList const& a, AttributeList const& b) {
    for (model::ColumnIndex a_atr : a) {
        for (model::ColumnIndex b_atr : b) {
            if (a_atr == b_atr) {
                return false;
            }
        }
    }
    return true;
}

bool StartsWith(AttributeList const& rhs_candidate, AttributeList const& rhs) {
    for (model::ColumnIndex i = 0; i < rhs.size(); ++i) {
        if (rhs[i] != rhs_candidate[i]) {
            return false;
        }
    }
    return true;
}

std::unordered_set<model::TupleIndex> GetNullIndices(
        std::vector<model::TypedColumnData> const& data) {
    std::unordered_set<model::TupleIndex> null_rows;
    for (unsigned int i = 0; i < data.size(); ++i) {
        if (!model::Type::IsOrdered(data[i].GetTypeId())) {
            continue;
        }
        for (size_t k = 0; k < data[i].GetNumRows(); ++k) {
            if (data[i].IsNullOrEmpty(k)) {
                null_rows.insert(k);
            }
        }
    }
    return null_rows;
}

std::vector<IndexedByteData> GetIndexedByteData(
        model::TypedColumnData const& data,
        std::unordered_set<model::TupleIndex> const& null_rows) {
    std::vector<IndexedByteData> indexed_byte_data;
    indexed_byte_data.reserve(data.GetNumRows());
    std::vector<std::byte const*> const& byte_data = data.GetData();
    for (size_t k = 0; k < byte_data.size(); ++k) {
        if (null_rows.find(k) != null_rows.end()) {
            continue;
        }
        indexed_byte_data.emplace_back(k, byte_data[k]);
    }
    return indexed_byte_data;
}

}  // namespace algos::order
