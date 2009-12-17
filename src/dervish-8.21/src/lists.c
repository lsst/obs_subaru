/*
 * SDSS utility functions connected with lists of objects
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shCAssert.h"
#include "shCUtils.h"
#include "shCSchema.h"
#include "shCList.h"
#include "shCLink.h"
#if !defined(NOTCL)
#include "shCHg.h"
#endif
#include "shCGarbage.h"

/*****************************************************************************/

RET_CODE
shListAdd(LIST *list, void *thing, ADD_FLAGS flag)
{
   LIST_ELEM *elem = (LIST_ELEM *)thing;
   
   shAssert(elem != NULL);

   if(list == NULL) {
      shError("shListAdd : attempt to add to an empty list");
      return(SH_GENERIC_ERROR);
   }
   if(list->type != elem->type && list->type != shTypeGetFromName("PTR")) {
      shError("shListAdd : attempt to add a %s to a list of %ss",
	      shNameGetFromType(elem->type),shNameGetFromType(list->type));
      return(SH_GENERIC_ERROR);
   }
/* S. Kent - make sure element is not already linked */
   if(elem->next != NULL || elem->next != NULL) {
      shError("shListAdd : Element is already linked to a list.");
      return(SH_GENERIC_ERROR);
   }
   if(flag == ADD_FRONT) {
      elem->next = list->first;
      if(list->last == NULL) {
	 list->last = elem;
      } else {
	 list->first->prev = elem;
      }
      list->first = elem;
   } else {
      elem->prev = list->last;
      if(list->first == NULL) {
	 list->first = elem;
      } else {
	 list->last->next = elem;
      }
      list->last = elem;
   }
   return(SH_SUCCESS);
}

void *
shListRem(LIST *list, void *thing)
{
   LIST_ELEM *elem = (LIST_ELEM *)thing;
   
   if(list == NULL) return(NULL);

   if (list->first == NULL)
      return(NULL);
   if (elem == NULL)
      return(NULL);

   if(elem->prev == NULL) {
      shAssert(list->first == elem);
      list->first = elem->next;
   } else {
      elem->prev->next = elem->next;
   }
   if(elem->next == NULL) {
      shAssert(list->last == elem);
      list->last = elem->prev;
   } else {
      elem->next->prev = elem->prev;
   }
   elem->prev = elem->next = NULL;
   
   return(elem);
}

LIST *
shListNew(TYPE type)
{
   LIST *newList;
   
   if((newList = (LIST *)shMalloc(sizeof(LIST))) == NULL) {
      shFatal("shListNew : Cannot allocate storage for newList");
   }
   newList->type = type;
   newList->first = newList->last = NULL;
   
   return(newList);
}

void
shListDel(LIST *list, RET_CODE (*del_func)(LIST_ELEM *ptr))
{
   LIST_ELEM *ptr;
   LIST_ELEM *tmp;
   
   if(list == NULL) return;
   ptr = list->first;
   while(ptr != NULL) {
      tmp = ptr->next;		     /* cannot reference a freed pointer */
      ptr->next = ptr->prev = NULL;  /* else shListDel complains */
      (*del_func)(ptr);
      ptr = tmp;
   }
   shFree((char *)list);
}

/* S. Kent.  Walk a linked list.  Usage is comparable to HandleWalk routine
** of extended Tcl.  walkPtr is initially set to NULL by the calling
** program.  Each call to shListWalk returns the next thing on the list.
** NULL is returned once the end of the list is reached.  walkPtr should
** not be touched during the walk. */

void *
shListWalk(LIST *list, void **walkPtr)
{
   LIST_ELEM *elem = (LIST_ELEM *)*walkPtr;

   if (elem == NULL) {
      if(list == NULL) {
         return(NULL);
      }
      *walkPtr = (void *)list->first;
   } else {
      *walkPtr = (void *)elem->next;
   }
   return (*walkPtr);
}
/*****************************************************************************/

unsigned int shListSize(LIST *list) {

   void *listitem;
   void *Ptr;
   int nlist = 0;
/* Begin code */

   Ptr=NULL;
/* Count the number of items on the list */
   /* while ( (listitem = shLinkWalk (list, &Ptr, NULL)) != NULL) {nlist++;} */
   return nlist;
}

/****************************************************************************/
RET_CODE shListJoin(LIST *list1, LIST *list2) {
  LIST_ELEM *le1, *le2;
  unsigned int size1, size2, sizej;

  size1=shListSize(list1); size2=shListSize(list2);

/* if list2 is empty, return without doing anything */
  if (list2->first == NULL) {return SH_SUCCESS;}

/* if list1 is empty, set the first and last pointers from list2 */
  if (list1->first == NULL) {
    list1->first = list2->first;
    list1->last  = list2->last;
    return SH_SUCCESS;
  }

/* something on each list, so have the end of 1 point to beginning of 2 */
  le1 = (LIST_ELEM *)list1->last;
  le2 = (LIST_ELEM *)list2->first;
  le1->next = le2;
  le2->prev = le1;
  list1->last = list2->last;

  sizej=shListSize(list1);

  return SH_SUCCESS;
}

