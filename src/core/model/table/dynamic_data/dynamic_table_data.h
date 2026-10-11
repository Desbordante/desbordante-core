#pragma once

#include <cassert>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "core/config/exceptions.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/dynamic_data/row.h"
#include "core/model/table/dynamic_data/update_strategies/update_strategy.h"
#include "core/model/types/type.h"

namespace model {

class IUpdateStrategy;

// TODO (Anosov Pavel):
// table might have strings as values as well, consider using template interface

class IDynamicTableData {
    std::unique_ptr<IUpdateStrategy> update_strategy_;

protected:
    std::set<size_t> inserted_;
    std::map<size_t, Row> updated_;
    std::map<size_t, Row> deleted_;

public:
    // using Row = std::vector<std::byte const*>;
    // using Col = std::vector<std::byte const*>;

    virtual ~IDynamicTableData() = default;

    void SetStrategy(std::unique_ptr<IUpdateStrategy> strategy) {
        update_strategy_ = std::move(strategy);
    }

    // On each new Update call previous cached indexes are cleared and new row indexes are inserted
    void Update() {
        if (update_strategy_) {
            inserted_.clear();
            updated_.clear();
            deleted_.clear();
            update_strategy_->Update(this);
        }
    }

    std::map<size_t, Row> const& GetUpdated() const noexcept {
        return updated_;
    }

    std::map<size_t, Row> const& GetDeleted() const noexcept {
        return deleted_;
    }

    std::set<size_t> const& GetInserted() const noexcept {
        return inserted_;
    }

    virtual void AppendRow(std::vector<std::string> const& row) = 0;
    virtual void UpdateRow(size_t index, std::vector<std::string> const& new_row) = 0;
    virtual void DeleteRow(size_t index) = 0;
    virtual bool RowExists(size_t row) const = 0;

    virtual std::set<size_t> GetAllIds() const = 0;
    virtual std::vector<std::byte const*> GetRow(size_t index) const = 0;
    virtual std::vector<std::byte const*> GetCol(size_t index) const = 0;
    virtual std::byte const* GetValue(size_t row, size_t col) const = 0;
    virtual size_t GetNumRows() const = 0;
    virtual size_t GetNumCols() const = 0;
};

class ITypedDynamicTableData : public IDynamicTableData {
public:
    virtual std::vector<model::Type const*> GetTypes() const = 0;
};

}  // namespace model
