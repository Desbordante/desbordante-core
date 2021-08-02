#pragma once

// Customize the namespace (default is `kafka`) if necessary
#ifndef KAFKA_API
#define KAFKA_API kafka
#endif

// Here is the MACRO to enable internal stubs for UT
// #ifndef KAFKA_API_ENABLE_UNIT_TEST_STUBS
// #define KAFKA_API_ENABLE_UNIT_TEST_STUBS
// #endif

#if defined(WIN32)
#define NOMINMAX
#endif
