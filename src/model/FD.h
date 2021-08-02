#pragma once

#include <string>

#include "Column.h"
#include "Vertical.h"

#include "json.hpp"

class FD {
private:
    Vertical lhs_;
    Column rhs_;

public:
    FD(Vertical const& lhs, Column const& rhs) : lhs_(lhs), rhs_(rhs) {}

    std::string toJSONString() const {
        return "{\"lhs\": " + lhs_.toIndicesString() + ", \"rhs\": " + rhs_.toIndicesString() + "}";
    }

    nlohmann::json toJSON() const {
        nlohmann::json json;
        json["lhs"] = nlohmann::json::parse(lhs_.toIndicesString());
        json["rhs"] = nlohmann::json::parse(rhs_.toIndicesString());
        return json;
    }

    bool operator<(FD const& rhs) const {
        return toJSONString() < rhs.toJSONString();
    }

    Vertical const& getLhs() const { return lhs_; }
    Column const& getRhs() const { return rhs_; }

    // unsigned int fletcher16() const;
};