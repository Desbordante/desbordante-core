#pragma once

#include "core/algorithms/cind/cind.h"
#include "core/algorithms/cind/types.h"
#include "core/algorithms/ind/ind.h"
#include "core/config/tabular_data/input_tables_type.h"
#include "core/model/table/encoded_tables.h"
#include "core/util/primitive_collection.h"

namespace algos::cind {
using model::EncodedColumnData;
using model::EncodedTables;
using AttrsType = std::vector<EncodedColumnData const*>;

class CindMiner {
private:
    friend class CindAlgorithm;

protected:
    struct Attributes {
        AttrsType lhs_inclusion, rhs_inclusion, conditional;
    };

    model::EncodedTables tables_;
    double min_validity_;
    double min_completeness_;
    CondType condition_type_;
    unsigned int min_support_{2};
    util::PrimitiveCollection<CIND> cind_collection_;

    virtual CIND ExecuteSingle(model::IND const& aind) = 0;
    Attributes ClassifyAttributes(model::IND const& aind) const;
    static std::vector<std::string> GetConditionalAttributesNames(AttrsType const& condition_attrs);

public:
    CindMiner(config::InputTables& input_tables);
    virtual ~CindMiner() = default;

    unsigned long long Execute(std::list<model::IND> const& aind_list);

    std::list<CIND> const& CINDList() const noexcept {
        return cind_collection_.AsList();
    }
};
}  // namespace algos::cind
