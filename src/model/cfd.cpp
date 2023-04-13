#include "model/cfd.h"

// see ../algorithms/cfd/LICENSE
#include <functional>

bool IsValid(const Itemset& lhs, int rhs) {
    // Discard Variable -> Constant and Constant -> Variable CFDs
    if (rhs < 0) {
        // if (lhs.size() && !Has(lhs, [](int si) -> bool { return si < 0; })) return false;
        if (!(lhs.empty() ||
              std::any_of(lhs.begin(), lhs.end(), [](int si) -> bool { return si < 0; })))
            return false;
    } else {
        // if (Has(lhs, [](int si) -> bool { return si < 0; })) return false;
        if (std::any_of(lhs.begin(), lhs.end(), [](int si) -> bool { return si < 0; }))
            return false;
    }
    return true;
}
