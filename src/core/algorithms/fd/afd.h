#pragma once

#include <string>

#include "core/algorithms/fd/fd.h"

class AFD : public FD {
private:
long double threeshold_;

public:
    AFD(Vertical const& lhs, Column const& rhs, long double const& threeshold, std::shared_ptr<RelationalSchema const> schema)
        : FD(lhs, rhs, schema), threeshold_(threeshold) {}

    
    AFD(Vertical const& lhs, Column const& rhs, std::shared_ptr<RelationalSchema const> schema)
        : FD(lhs, rhs, schema), threeshold_(0) {}


    long double const& GetThreeshold() const {
        return threeshold_;
    }
};
