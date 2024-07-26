#pragma once

#ifndef NDEBUG
#include <cassert>
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
