#pragma once

// clang produces warning on unused private fields, so they need to be marked as [[maybe_unused]],
// but g++ doesn't recognize [[maybe_unused]] on class fields and produces warning.
// This macro expands to [[maybe_unused]], when compiler is clang, nop otherwise
// (see https://stackoverflow.com/questions/50646334/maybe-unused-on-member-variable-gcc-warns-
// incorrectly-that-attribute-is and https://gcc.gnu.org/bugzilla/show_bug.cgi?id=72789)
#ifdef __clang__
#define MAYBE_UNUSED [[maybe_unused]]
#else
#define MAYBE_UNUSED /* Ignore */
#endif
