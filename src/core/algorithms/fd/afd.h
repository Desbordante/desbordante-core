#pragma once

#include <string>

#include "core/algorithms/fd/fd.h"

class AFD : public FD {
private:
    long double threshold_;

public:
    AFD(Vertical const& lhs, Column const& rhs, long double const& threshold,
        std::shared_ptr<RelationalSchema const> schema)
        : FD(lhs, rhs, schema), threshold_(threshold) {}

    AFD(Vertical const& lhs, Column const& rhs, std::shared_ptr<RelationalSchema const> schema)
        : FD(lhs, rhs, schema), threshold_(0) {}

    long double const& GetThreshold() const {
        return threshold_;
    }
};
