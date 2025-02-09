#pragma once

#include <memory>

#include "cind/condition_type.hpp"
#include "ind/ind.h"
#include "table/column_encoded_relation_data.h"
#include "tabular_data/input_tables_type.h"

namespace algos::cind {
class CindAlgorithm;
using model::ColumnEncodedRelationData;
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

    virtual void ExecuteSingle(model::IND const& aind) = 0;
    Attributes ClassifyAttributes(model::IND const& aind) const;

protected:
    std::vector<std::unique_ptr<ColumnEncodedRelationData>> tables_;
    double min_validity_;
    double min_completeness_;
    CondType condition_type_;

private:
    friend class CindAlgorithm;
};
}
