#pragma once

#include "cind/cind.h"
#include "cind/condition_type.h"
#include "ind/ind.h"
#include "primitive_collection.h"
#include "table/encoded_tables.h"
#include "tabular_data/input_tables_type.h"

namespace algos::cind {
class CindAlgorithm;
using model::EncodedColumnData;
using model::EncodedTables;
using AttrsType = std::vector<EncodedColumnData const*>;

class CindMiner {
public:
    CindMiner(config::InputTables& input_tables);
    virtual ~CindMiner() = default;

    void Execute(std::list<model::IND> const& aind_list);

    std::list<CIND> const& CINDList() const noexcept {
        return cind_collection_.AsList();
    }

protected:
    struct Attributes {
        AttrsType lhs_inclusion, rhs_inclusion, conditional;
    };

    virtual CIND ExecuteSingle(model::IND const& aind) = 0;
    Attributes ClassifyAttributes(model::IND const& aind) const;
    static std::vector<std::string> GetConditionalAttributesNames(AttrsType const& condition_attrs);

protected:
    model::EncodedTables tables_;
    double min_validity_;
    double min_completeness_;
    CondType condition_type_;

    util::PrimitiveCollection<CIND> cind_collection_;

private:
    friend class CindAlgorithm;
};
}  // namespace algos::cind
