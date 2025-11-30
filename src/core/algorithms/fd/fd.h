#pragma once

#include <string>

#include "core/algorithms/fd/raw_fd.h"
#include "core/model/table/column.h"
#include "core/model/table/vertical.h"

class FD {
private:
    Vertical lhs_;
    Column rhs_;
    std::shared_ptr<RelationalSchema const> schema_;

public:
    FD(Vertical const& lhs, Column const& rhs, std::shared_ptr<RelationalSchema const> schema)
        : lhs_(lhs), rhs_(rhs), schema_(std::move(schema)) {}

    std::string ToJSONString() const {
        return "{\"lhs\": " + lhs_.ToIndicesString() + ", \"rhs\": " + rhs_.ToIndicesString() + "}";
    }

    std::shared_ptr<RelationalSchema const> GetSchema() const {
        return schema_;
    }

    Vertical const& GetLhs() const {
        return lhs_;
    }

    Column const& GetRhs() const {
        return rhs_;
    }

    [[nodiscard]] RawFD ToRawFD() const {
        return {lhs_.GetColumnIndices(), rhs_.GetIndex()};
    }

    std::vector<model::ColumnIndex> GetLhsIndices() const;

    model::ColumnIndex GetRhsIndex() const noexcept {
        return rhs_.GetIndex();
    }

    std::tuple<std::vector<std::string>, std::string> ToNameTuple() const;

    std::string ToShortString() const;

    std::string ToLongString() const;
};
