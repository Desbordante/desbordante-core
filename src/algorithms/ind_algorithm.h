#pragma once

#include <list>
#include <string>
#include <unordered_map>

#include "algorithms/primitive.h"
#include "model/ind.h"

namespace algos {

template <typename BasePrimitive = algos::Primitive>
class INDAlgorithm : public BasePrimitive {
    static_assert(std::is_same_v<BasePrimitive, MultiCsvPrimitive> ||
                          std::is_same_v<BasePrimitive, Primitive>,
                  "The INDAlgorithm can either process multiple tables at once using the "
                  "Desbordante standard parser, or use a custom parser.");

public:
    struct DatasetInfo {
        std::string table_name;
        std::vector<std::string> header;
    };
    using DatasetsOrder = std::vector<DatasetInfo>;
    using IND = model::IND;
    using INDList = std::list<IND>;

public:
    using BasePrimitive::BasePrimitive;

    virtual DatasetsOrder const& GetDatasetsOrder() const = 0;
    virtual INDList IndList() const = 0;
};

using PINDAlgorithm = INDAlgorithm<Primitive>;
using MINDAlgorithm = INDAlgorithm<MultiCsvPrimitive>;

}  // namespace algos
