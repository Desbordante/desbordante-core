#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "algorithms/algorithm.h"
#include "ind.h"
#include "tabular_data/input_tables_type.h"
#include "util/primitive_collection.h"

namespace algos {

class INDAlgorithm : public Algorithm {
public:
    using IND = model::IND;

private:
    util::PrimitiveCollection<IND> ind_collection_;

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
                             std::shared_ptr<model::ColumnCombination> rhs) {
        ind_collection_.Register(std::move(lhs), std::move(rhs));
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
