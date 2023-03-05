#pragma once

#include <list>
#include <string>
#include <unordered_map>

#include "algorithms/primitive.h"
#include "model/ind.h"

namespace algos {

template <typename BasePrimitive>
class INDAlgorithm : public BasePrimitive {
    static_assert(std::is_same_v<BasePrimitive, MultiCsvPrimitive> ||
                          std::is_same_v<BasePrimitive, Primitive>,
                  "The INDAlgorithm can either process multiple tables at once using the "
                  "Desbordante standard parser, or use a custom parser.");

protected:
    using IND = model::IND;
    using INDList = std::list<IND>;
    struct DatasetInfo {
        std::string table_name;
        std::vector<std::string> header;
    };

    using DatasetsInfo = std::vector<DatasetInfo>;

public:
    using BasePrimitive::BasePrimitive;

    virtual DatasetsInfo const& GetDatasetsInfo() const = 0;
    virtual INDList IndList() const = 0;
};

}  // namespace algos
