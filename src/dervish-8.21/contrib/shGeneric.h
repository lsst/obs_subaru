#ifndef SHGENERIC_H
#define SHGENERIC_H

#include "shChain.h"

void *shGenericNew(char *typeName);
void shGenericDel(void *object);
void shGenericChainDel(CHAIN *chain);
#endif
