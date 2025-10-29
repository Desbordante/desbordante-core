//
// Created by Ilya Vologin
// https://github.com/cupertank
//

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "bitset_utils.h"

class Column;
class Vertical;

namespace boost {
template <class T>
class optional;
}  // namespace boost

class RelationalSchema {
private:
    std::vector<std::unique_ptr<Column>> columns_;
    std::string name_;

public:
    RelationalSchema(std::string name);

    RelationalSchema(RelationalSchema const& other) = delete;
    RelationalSchema& operator=(RelationalSchema const& rhs) = delete;
    RelationalSchema(RelationalSchema&& other) noexcept = delete;
    RelationalSchema& operator=(RelationalSchema&& rhs) noexcept = delete;

    std::string GetName() const {
        return name_;
    }

    std::vector<std::unique_ptr<Column>> const& GetColumns() const {
        return columns_;
    };

    bool IsColumnInSchema(std::string const& col_name) const;

    Column const* GetColumn(std::string const& col_name) const;
    Column const* GetColumn(size_t index) const;
    size_t GetNumColumns() const;
    Vertical GetVertical(boost::dynamic_bitset<> indices) const;

    Vertical CreateEmptyVertical() const;

    void AppendColumn(std::string const& col_name);
    void AppendColumn(Column column);

    ~RelationalSchema();

    friend inline bool operator==(RelationalSchema const& l, RelationalSchema const& r);
};

inline bool operator==(RelationalSchema const& l, RelationalSchema const& r) {
    return (l.name_ == r.name_ && l.columns_.size() == r.columns_.size());
}

inline bool operator!=(RelationalSchema const& l, RelationalSchema const& r) {
    return !(l == r);
}
