#pragma once

#include <string>
#include <vector>

namespace model {

class IDatasetStream {
public:
    virtual std::vector<std::string> GetNextRow() = 0;
    [[nodiscard]] virtual bool HasNextRow() const = 0;
    [[nodiscard]] virtual size_t GetNumberOfColumns() const = 0;
    [[nodiscard]] virtual std::string GetColumnName(size_t index) const = 0;
    [[nodiscard]] virtual std::string GetRelationName() const = 0;
    virtual void Reset() = 0;
    virtual ~IDatasetStream() = default;
};

}  // namespace model
