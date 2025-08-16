#pragma once

#include <memory>   // for shared_ptr
#include <string>   // for allocator, char_traits, operator+
#include <tuple>    // for tuple
#include <utility>  // for move
#include <vector>   // for vector

#include "model/table/column.h"    // for Column
#include "model/table/vertical.h"  // for Vertical
#include "raw_fd.h"                // for RawFD
#include "table/column_index.h"    // for ColumnIndex

class RelationalSchema;

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
