#pragma once

#include <stdexcept>

#include "core/algorithms/dc/model/dc.h"
#include "core/algorithms/dc/model/operator.h"
#include "core/algorithms/dc/model/predicate.h"
#include "core/model/table/relational_schema.h"
#include "core/model/table/typed_column_data.h"

namespace algos::dc {

class DCParser {
private:
    std::vector<std::string> str_operators_;
    RelationalSchema const* schema_;
    std::vector<model::Type const*> const& types_;
    static constexpr std::string_view const kSep = " and ";
    std::string dc_string_;
    bool has_next_predicate_;
    std::string next_pred_;
    size_t cur_;

public:
    DCParser(std::string dc_string, RelationalSchema const* schema,
             std::vector<model::Type const*> const& types);

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
