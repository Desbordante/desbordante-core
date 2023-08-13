#pragma once

#include <optional>
#include <string>
#include <vector>

namespace algos::cfd {

using AttributeIndex = int;

class RawCFD {
public:
    struct RawItem {
        AttributeIndex attribute;         /* attribute column index */
        std::optional<std::string> value; /* pattern value is optional */
    };
    using RawItems = std::vector<RawItem>;

private:
    RawItems lhs_;
    RawItem rhs_;

public:
    explicit RawCFD(RawItems lhs, RawItem rhs) : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    std::string ToJSON() const;

    RawItems const& GetLhs() const {
        return lhs_;
    }
    RawItem const& GetRhs() const {
        return rhs_;
    }
};

}  // namespace algos::cfd
