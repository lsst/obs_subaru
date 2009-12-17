#if !defined(FOO_H)
#define FOO_H
#include "shCList.h"
/*
 * A type definition used in testing diskio code
 */
typedef struct foo {
   TYPE type;
   struct foo *next, *prev;
   char *name;
   int i;
   long l;
   float f;
} FOO;

FOO *shFooNew(void);
RET_CODE shFooDel(FOO *foo);

#endif


