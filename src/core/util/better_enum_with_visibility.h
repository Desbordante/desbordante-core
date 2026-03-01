/*
 * BETTER_ENUM creates a class for each defined enum. Some of the classes need to be visible outside
 * the library so that their type_info symbols are preserved for Python conversions to work on Mac.
 * This header includes the better-enums header and defines default visibility on all these classes.
 *
 * For now, use this instead of #include <enum.h>.
 */
#pragma once
#include "core/util/export.h"

#define BETTER_ENUMS_CLASS_ATTRIBUTE DESBORDANTE_EXPORT
#include <enum.h>
