#pragma once

#include <memory>
#include <vector>

#include "algorithms/algorithm.h"
#include "algorithms/association_rules/ar_algorithm_enums.h"
#include "config/tabular_data/input_table_type.h"
#include "model/transaction/transactional_data.h"
#include "near.h"

namespace algos {

class NeARDiscovery : public Algorithm {
private:
    unsigned int tid_column_index_;
    unsigned int item_column_index_;
    bool first_column_tid_;
    config::InputTable input_table_;
    InputFormat input_format_ = InputFormat::singular;

    void RegisterOptions();

protected:
    std::shared_ptr<model::TransactionalData> transactional_data_;
    // ParamType some_parameter_
    // . . .
    std::vector<model::NeARIDs> near_collection_;

public:
    constexpr static std::string_view kDefaultPhaseName = "NeAR mining";
    explicit NeARDiscovery(std::vector<std::string_view> phase_names);
    explicit NeARDiscovery();
    void LoadDataInternal() final;

    std::vector<model::NeARIDs> GetNeARIDsVector() const {
        return near_collection_;
    }

    std::vector<model::NeARStrings> const& GetNeARStringsVector() const {
        throw std::runtime_error("GetNeARStringsVector is not implemented");
    }
};

}  // namespace algos
