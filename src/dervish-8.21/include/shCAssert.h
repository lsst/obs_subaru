#ifndef SHCASSERT_H
#define SHCASSERT_H
/*
 * A version of assert that calls shFatal
 */
# ifndef NDEBUG

#include "shCUtils.h"                      /* Gives us prototype for shFatal */

# define shAssert(ex) {if (!(ex)){shFatal("Assertion failed: \"%s\", file \"%s\", line %d", #ex, __FILE__, __LINE__);}}
# else
# define shAssert(ex) /* Nothing */

# endif
#endif
