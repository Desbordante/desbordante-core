#include "algorithms/fd/fd.h"

#include "util/bitset_utils.h"

std::vector<model::ColumnIndex> FD::GetLhsIndices() const {
    return util::BitsetToIndices<model::ColumnIndex>(lhs_.GetColumnIndices());
}

std::tuple<std::vector<std::string>, std::string> FD::ToNameTuple() const {
    std::tuple<std::vector<std::string>, std::string> name_tuple;
    auto& [lhs_names, rhs_name] = name_tuple;
    std::vector<Column const*> const& lhs_columns = lhs_.GetColumns();
    lhs_names.reserve(lhs_columns.size());
    for (Column const* column : lhs_columns) {
        lhs_names.push_back(column->GetName());
    }
    rhs_name = rhs_.GetName();
    return name_tuple;
}

std::string FD::ToShortString() const {
    std::stringstream stream;
    stream << "[ ";
    for (model::ColumnIndex index : GetLhsIndices()) {
        stream << index << " ";
    }
    stream << "] -> ";
    stream << GetRhsIndex();
    return stream.str();
}

std::string FD::ToLongString() const {
    std::stringstream ss;
    ss << lhs_.ToString();
    ss << " -> ";
    ss << rhs_.GetName();
    return ss.str();
}
