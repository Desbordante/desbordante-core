#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/container/allocator_traits.hpp>

#include "algorithms/dc/model/dc.h"
#include "algorithms/dc/model/operator.h"
#include "algorithms/dc/model/predicate.h"
#include "dc/model/column_operand.h"
#include "model/table/column_layout_relation_data.h"
#include "model/table/typed_column_data.h"

class ColumnLayoutRelationData;

namespace model {
class TypedColumnData;
}  // namespace model

namespace algos::dc {

class DCParser {
private:
    std::vector<std::string> str_operators_;
    ColumnLayoutRelationData const* relation_;
    std::vector<model::TypedColumnData> const& data_;
    static constexpr std::string_view const kSep = " and ";
    std::string dc_string_;
    bool has_next_predicate_;
    std::string next_pred_;
    size_t cur_;

public:
    DCParser(std::string dc_string, ColumnLayoutRelationData const* relation,
             std::vector<model::TypedColumnData> const& data);

    DC Parse();
    Predicate GetNextPredicate();
    ColumnOperand ConvertToVariableOperand(std::string const& operand) const;
    bool IsVarOperand(std::string op) const;
    void RemoveEscaping(std::string& str) const;

    Predicate ConvertToPredicate(std::string const& pred);
    std::pair<ColumnOperand, ColumnOperand> GetOperands(std::string str_left_op,
                                                        std::string str_right_op);
};

}  // namespace algos::dc
