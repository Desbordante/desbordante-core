#pragma once
#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/fd/fd.h"
#include "core/model/index.h"
#include "core/model/table/table_header.h"

namespace algos {
struct SingleAttrRhsStrippedFd {
    boost::dynamic_bitset<> lhs;

    model::FunctionalDependency ToFd(model::TableHeader const& table_header,
                                     model::Index rhs) const {
        std::vector<model::Attribute> lhs_attrs;
        lhs_attrs.reserve(lhs.count());
        util::ForEachIndex(lhs, [&](model::Index i) {
            lhs_attrs.emplace_back(table_header.column_names[i], i);
        });
        return {table_header.table_name,
                std::move(lhs_attrs),
                {{table_header.column_names[rhs], rhs}}};
    }
};
}  // namespace algos
