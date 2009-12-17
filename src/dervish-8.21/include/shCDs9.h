#if !defined(SHCDS9_H)
#define SHCDS9_H
#include "region.h"
#if defined(HAVE_DS9)
#include "xpa.h"
#endif

const char *shXpaGet(const char *cmd);
int shXpaSet(const char *cmd, char *buf);
int shXpaFits(const REGION *reg, const MASK *mask);
#endif
