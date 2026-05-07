#pragma once

#include <memory>

namespace model {

class IDynamicTableData;

// Update strategy for dynamic data
// TODO (Anosov Pavel): Add git-like diff update strategy with single input table as updated data
class IUpdateStrategy {
public:
    virtual void Update(IDynamicTableData* table) = 0;
    virtual ~IUpdateStrategy() = default;
};

}  // namespace model
