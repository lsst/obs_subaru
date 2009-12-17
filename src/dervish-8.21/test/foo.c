/*
 * Support for a type used to test diskio and lists, FOO
 */
#include <stdio.h>
#include <stdlib.h>
#include "shCAssert.h"
#include "shCUtils.h"
#include "shCSchema.h"
#include "shCList.h"
#include "foo.h"

FOO *
shFooNew(void)
{
   FOO *new = shMalloc(sizeof(FOO));
   new->type = shTypeGetFromName("FOO");
   new->next = new->prev = NULL;

   new->name = NULL;
   new->f = new->i = new->l = 0;
   return(new);
}

RET_CODE
shFooDel(FOO *foo)
{
   shAssert(foo->next == NULL && foo->prev == NULL);
   shFree(foo);
   return(SH_SUCCESS);
}
