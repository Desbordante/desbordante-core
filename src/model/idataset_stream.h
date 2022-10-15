#pragma once

#include <string>
#include <vector>

namespace model {

class IDatasetStream {
public:
    virtual std::vector<std::string> GetNextLine() = 0;
    [[nodiscard]] virtual bool HasLines() const = 0;
    [[nodiscard]] virtual int GetNumberOfColumns() const = 0;
    [[nodiscard]] virtual std::string GetColumnName(int index) const = 0;
    [[nodiscard]] virtual std::string GetRelationName() const = 0;
    virtual void Reset() = 0;
    virtual ~IDatasetStream() = default;
};

}  // namespace model
