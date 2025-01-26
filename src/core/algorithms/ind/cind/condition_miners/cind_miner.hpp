#pragma once

#include <memory>
#include <vector>

#include "ind/cind/condition_type.hpp"
#include "ind/ind.h"
#include "table/column_domain.h"

namespace algos::cind {
class CindAlgorithm;

class CindMiner {
protected:
    std::shared_ptr<std::vector<model::ColumnDomain>> domains_;
    double precision_;
    double recall_;
    CondType condition_type_;

public:
    CindMiner(std::shared_ptr<std::vector<model::ColumnDomain>> domains);
    virtual ~CindMiner() = default;

    void Execute(std::list<model::IND> const& aind_list);

protected:
    virtual void ExecuteSingle(model::IND const& aind) = 0;

private:
    friend class CindAlgorithm;
};
}
