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
    size_t id_;

    static int createId() {
        static int id = 1;
        return id++;
    }

public:
    TableRow(std::vector<std::string>&& data) : 
        data_(std::move(data)), 
        id_(createId()) {}
    TableRow(size_t id) : data_({}), id_(id) {}
    TableRow() : data_({}), id_(0) {};

    TableRow(const TableRow& other) : data_(other.data_), id_(other.id_) {}
    TableRow(TableRow&& other) : data_(std::move(other.data_)), id_(other.id_) {}
    TableRow& operator=(const TableRow& other) {
        if (this != &other) {
            data_ = other.data_;
            id_ = other.id_;
        }
        return *this;
    }
    TableRow& operator=(TableRow&& other) {
        if (this != &other) {
            data_ = std::move(other.data_);
            id_ = other.id_;
        }
        return *this;
    }

    const std::vector<std::string>& getData() const {
        return data_;
    }

    std::string toString() const {
        return "(id: " + std::to_string(id_) + ", data: " + boost::algorithm::join(data_, ",") + ")";
    }

    explicit operator std::string() const {
        return toString();
    }

    size_t getId() const {
        return id_;
    }

    bool operator==(const TableRow &rhs) const {
        return id_ == rhs.getId();
    }
};

template<>
struct std::hash<TableRow>
{
    std::size_t operator()(const TableRow& s) const noexcept
    {
        return s.getId();
    }
};
