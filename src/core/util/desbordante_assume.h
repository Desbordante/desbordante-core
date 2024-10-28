#pragma once

#ifndef NDEBUG
#include <cassert>
// Only use if you're certain the expression will be optimized away.
#define DESBORDANTE_ASSUME(expr)                      \
    {                                                 \
        auto const holds = static_cast<bool>((expr)); \
        assert(holds);                                \
        if (!holds) __builtin_unreachable();          \
    }
#else
#define DESBORDANTE_ASSUME(expr)                      \
    {                                                 \
        auto const holds = static_cast<bool>((expr)); \
        if (!holds) __builtin_unreachable();          \
    }
#endif
