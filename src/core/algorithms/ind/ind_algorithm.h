#pragma once

#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "algorithms/algorithm.h"
#include "error/type.h"
#include "ind.h"
#include "model/table/relational_schema.h"
#include "table/column_combination.h"
#include "tabular_data/input_tables_type.h"
#include "util/primitive_collection.h"

class RelationalSchema;

namespace algos {

class INDAlgorithm : public Algorithm {
public:
    using IND = model::IND;

private:
    util::PrimitiveCollection<IND> ind_collection_;
    std::shared_ptr<std::vector<RelationalSchema>> schemas_;

    void LoadDataInternal() final;

    virtual void LoadINDAlgorithmDataInternal() = 0;

    void ResetState() final {
        ind_collection_.Clear();
        ResetINDAlgorithmState();
    }

    virtual void ResetINDAlgorithmState() = 0;

protected:
    constexpr static std::string_view kDefaultPhaseName = "IND mining";

    config::InputTables input_tables_;

    explicit INDAlgorithm(std::vector<std::string_view> phase_names);

    virtual void RegisterIND(std::shared_ptr<model::ColumnCombination> lhs,
                             std::shared_ptr<model::ColumnCombination> rhs,
                             config::ErrorType error = 0.0) {
        ind_collection_.Register(std::move(lhs), std::move(rhs), schemas_, error);
    }

    void RegisterIND(model::ColumnCombination lhs, model::ColumnCombination rhs,
                     config::ErrorType error = 0.0) {
        RegisterIND(std::make_shared<model::ColumnCombination>(std::move(lhs)),
                    std::make_shared<model::ColumnCombination>(std::move(rhs)), error);
    }

    virtual void RegisterIND(IND ind) {
        ind_collection_.Register(std::move(ind));
    }

public:
    std::list<IND> const& INDList() const noexcept {
        return ind_collection_.AsList();
    }
};

}  // namespace algos
