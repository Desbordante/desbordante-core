#pragma once

/*
 * Make sure to specify the attribute for all classes that are types of options to avoid issues
 * similar to https://github.com/Desbordante/desbordante-core/pull/652.
 */
#ifdef _WIN32
#define DESBORDANTE_EXPORT __declspec(dllexport)
#else
#define DESBORDANTE_EXPORT __attribute__((visibility("default")))
#endif
