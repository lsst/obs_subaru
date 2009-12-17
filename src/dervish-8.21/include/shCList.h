#if !defined(SHCLISTS_H)
#define SHCLISTS_H
/*
 * This is the support code for lists. All list-capable types are
 * `derived' from LIST_ELEM; that is their definitions _must_ look like:
 struct foo {
    TYPE type;
    struct foo *next, *prev;
    ...;
 };
 */
#include "shCSchema.h"

typedef enum {
   ADD_ANY,				/* don't care where we add it */
   ADD_FRONT,				/* add at front of list */
   ADD_BACK				/* add at back of list */
} ADD_FLAGS;

typedef struct list_elem {
   TYPE type;
   struct list_elem *next, *prev;
} LIST_ELEM;				/*
					  pragma SCHEMA
					 */

typedef struct {
   TYPE type;				/* type of list */
   LIST_ELEM *first, *last;		/* first and last elements */
} LIST;					/*
					  pragma NOCONSTRUCTOR
					  pragma USER
					 */

/*****************************************************************************/
/*
 * Prototypes
 */
#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

void shListDel(LIST *list, RET_CODE (*del_func)(LIST_ELEM *ptr));
LIST *shListNew(TYPE type);
void *shListRem(LIST *list, void *thing);
RET_CODE shListAdd(LIST *list, void *thing, ADD_FLAGS flag);
RET_CODE shListJoin(LIST *list1, LIST *list2);
void *shListWalk(LIST *list, void **walkPtr);
unsigned int shListSize(LIST *list);
RET_CODE shListSort(LIST *list, char *memberName, int increasing);
#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif

