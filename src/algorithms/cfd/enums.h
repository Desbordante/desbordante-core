#pragma once

#include <enum.h>

namespace algos {

BETTER_ENUM(Substrategy, char,
            dfs = 0, /* It mines fds with acceptable support threshold
                                   * through dfs lattice traversal
                                   * then it mines patterns for this fds through dfs lattice traversal */
            bfs /* It mines fds with acceptable support threshold
                              * through dfs lattice traversal
                              * then it mines patterns for this fds through bfs lattice traversal */
);

} // namespace algos
