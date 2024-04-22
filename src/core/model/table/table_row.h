#pragma once

#include <string>
#include <unordered_set>
#include <utility>
#include <boost/algorithm/string/join.hpp>

#include "column_index.h"
#include "relational_schema.h"

class TableRow {
public:
    using IndexType = model::ColumnIndex;

private:
    std::vector<std::string> data_;
    const size_t hash_;

public:
    TableRow(std::vector<std::string>&& data) : 
        data_(std::move(data)), 
        hash_([&data]() -> size_t {
            size_t hash = 0;
            for (std::string element : data) {
                hash += std::hash<std::string>{}(element);
            }
            return hash;
        }()) {}
    
    TableRow(const TableRow& other) : data_(other.getData()), hash_(other.getHash()) {}

    TableRow() : data_({}), hash_(0) {};

    const std::vector<std::string>& getData() const {
        return data_;
    }

    std::string toString() const {
        return "(" + boost::algorithm::join(data_, ",") + ")";
    }

    explicit operator std::string() const {
        return toString();
    }

    size_t getHash() const {
        return hash_;
    }

    bool operator==(const TableRow &rhs) const {
        return hash_ == rhs.getHash();
    }
};

template<>
struct std::hash<TableRow>
{
    std::size_t operator()(const TableRow& s) const noexcept
    {
        return s.getHash();
    }
};
