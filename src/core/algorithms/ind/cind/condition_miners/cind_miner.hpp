#pragma once

#include <memory>

#include "ind/cind/condition_type.hpp"
#include "ind/ind.h"
#include "table/column_encoded_relation_data.h"
#include "tabular_data/input_tables_type.h"

namespace algos::cind {
class CindAlgorithm;
using model::ColumnEncodedRelationData;

class CindMiner {
protected:
    std::vector<std::unique_ptr<ColumnEncodedRelationData>> tables_;
    double precision_;
    double recall_;
    CondType condition_type_;

public:
    CindMiner(config::InputTables &input_tables);
    virtual ~CindMiner() = default;

    void Execute(std::list<model::IND> const& aind_list);

protected:
    virtual void ExecuteSingle(model::IND const& aind) = 0;

private:
    friend class CindAlgorithm;
};
}
