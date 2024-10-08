#include "removal_set.h"

namespace algos::od {

RemovalSet UnionRemovalSets(RemovalSetAsVec const& vec1, RemovalSetAsVec const& vec2) {
    RemovalSet removal_set{vec1.begin(), vec1.end()};
    removal_set.insert(vec2.begin(), vec2.end());
    return removal_set;
}

}  // namespace algos::od
