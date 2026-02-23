#pragma once

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/fd/fd.h"
#include "core/model/index.h"
#include "core/model/table/attribute.h"
#include "core/model/table/table_header.h"

namespace algos {
struct MultiAttrRhsStrippedFd {
    boost::dynamic_bitset<> lhs;
    boost::dynamic_bitset<> rhs;

    model::FunctionalDependency ToFd(model::TableHeader const& table_header_) const {
        std::vector<model::Attribute> lhs_attrs;
        lhs_attrs.reserve(lhs.count());
        util::ForEachIndex(lhs, [&](model::Index i) {
            lhs_attrs.emplace_back(table_header_.column_names[i], i);
        });
        std::vector<model::Attribute> rhs_attrs;
        rhs_attrs.reserve(rhs.count());
        util::ForEachIndex(rhs, [&](model::Index i) {
            rhs_attrs.emplace_back(table_header_.column_names[i], i);
        });
        return {table_header_.table_name, std::move(lhs_attrs), std::move(rhs_attrs)};
    }
};
}  // namespace algos
