#pragma once

#include "cind/cind.hpp"
#include "cind/condition_type.hpp"
#include "ind/ind.h"
#include "table/encoded_tables.hpp"
#include "tabular_data/input_tables_type.h"

namespace algos::cind {
class CindAlgorithm;
using model::EncodedTables;
using model::EncodedColumnData;
using AttrsType = std::vector<EncodedColumnData const*>;

class CindMiner {
public:
    CindMiner(config::InputTables &input_tables);
    virtual ~CindMiner() = default;

    void Execute(std::list<model::IND> const& aind_list);

protected:
    struct Attributes {
        AttrsType lhs_inclusion, rhs_inclusion, conditional;
    };

    virtual Cind ExecuteSingle(model::IND const& aind) = 0;
    Attributes ClassifyAttributes(model::IND const& aind) const;

protected:
    model::EncodedTables tables_;
    double min_validity_;
    double min_completeness_;
    CondType condition_type_;

private:
    friend class CindAlgorithm;
};
}
