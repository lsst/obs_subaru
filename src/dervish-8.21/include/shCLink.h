#ifndef SHCLINK_H
#define SHCLINK_H

/** Definition of a little structure that is used to make a linked list of
** pointers.  It is the same thing as rhl's THING, but the latter is
** not implemented in a consistent fashion throughout his code.  Sigh */

/* RHL now defines TYPE in shema.h, which needs the dervish message .h file
** as well.  This is getting annoying */

#include "dervish_msg_c.h"
#include "shCList.h"
#include "shCSchema.h"
#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

/* Parameters for a single LINK */
typedef struct link {
   TYPE type;
   struct link *next, *prev;
   TYPE ltype;	/* RHL reverses the naming of type and ltype, for reasons
		** that I cannot fathom. */
   void *lptr;
} LINK;	/* pragma SCHEMA */


/*****************************************************************************/
/*
 * Prototypes for LINK manipulation functions
 */

LINK *shLinkNew(void);
RET_CODE shLinkDel(LINK *slink);
void *shLinkRem(LIST *list, void **walkPtr, char **type_name);
RET_CODE shLinkAdd (LIST *list, void *thing, ADD_FLAGS flag, char *type_name);
void *shLinkWalk(LIST *list, void **walkPtr, char *type_name);
void *shLinkWalkTypeGet(LIST *list, void **walkPtr, TYPE *type);
void *shLinkBackWalk(LIST *list, void **walkPtr, char *type_name);
RET_CODE shLinkPurge(LIST *list, RET_CODE (*dl_func)(void *), char *type_name);

void **shVctrList(LIST *list, int *count, TYPE *type);
void shVctrFree(void **array);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif
