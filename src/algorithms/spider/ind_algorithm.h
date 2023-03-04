#pragma once

#include <filesystem>
#include <list>

#include "algorithms/primitive.h"
#include "type.h"

namespace algos {

template <typename BasePrimitive>
class INDAlgorithm : public BasePrimitive {
    static_assert(std::is_same_v<BasePrimitive, MultiCsvPrimitive> ||
                          std::is_same_v<BasePrimitive, Primitive>,
                  "The INDAlgorithm can either process multiple tables at once using the "
                  "Desbordante standard parser, or use a custom parser.");

    struct ColumnCombination {
        unsigned table_index;
        std::vector<unsigned> column_indices;
    };

    using IND = std::pair<std::shared_ptr<ColumnCombination>, std::shared_ptr<ColumnCombination>>;
    using INDList = std::list<IND>;

public:
    using BasePrimitive::BasePrimitive;

    //    virtual INDList GetResult() const;
};

}  // namespace algos
