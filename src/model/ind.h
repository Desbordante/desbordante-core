#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace model {

class IND {
public:
    struct ColumnCombination {
        unsigned table_index;
        std::vector<unsigned> column_indices;
    };

private:
    std::shared_ptr<ColumnCombination> lhs, rhs;

    std::string CCToString(ColumnCombination const& cc) const {
        std::stringstream ss;
        for (unsigned i : cc.column_indices) {
            ss << cc.table_index << "." << i << " ";
        }
        return ss.str();
    }

public:
    IND(std::shared_ptr<ColumnCombination> lhs, std::shared_ptr<ColumnCombination> rhs)
        : lhs(std::move(lhs)), rhs(std::move(rhs)) {}

    ColumnCombination const& GetLhs() const {
        return *lhs;
    }
    ColumnCombination const& GetRhs() const {
        return *rhs;
    }
    std::string ToString() const {
        std::stringstream ss;
        ss << CCToString(GetLhs()) << "-> " << CCToString(GetRhs());
        return ss.str();
    }
};

}  // namespace model
