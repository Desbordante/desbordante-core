#pragma once

#include <string>

#include "model/table/column.h"
#include "model/table/vertical.h"
#include "raw_fd.h"

class FD {
private:
    Vertical lhs_;
    Column rhs_;

public:
    FD(Vertical const& lhs, Column const& rhs) : lhs_(lhs), rhs_(rhs) {}

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
};
