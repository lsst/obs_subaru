/*
 * SDSS utility functions connected with lists of objects.
 * These functions provide an interface to link objects that do not have
 * built-in list support (i.e. don't have  *next, *prev, TYPE fields).
 * An element called a "link" is used to supply this functionality.
 * You create a list of "links", and each link points to your object.
 * As of this writing, you must make sure that the LINK type is defined
 * in dervish.  This is done by including the links.h header file in the
 * list DISKIO_OBJS in your Makefile and generating a new version of
 * diskio_gen.c.

** shVctrList		public	Generate array of pointers to list elements
** shVctrFree		public	Delete above array

 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "shCAssert.h"
#include "shCUtils.h"
#include "shCSchema.h"
#include "shCList.h"
#include "shCLink.h"
#include "shCHg.h"
#include "shCGarbage.h"
/*****************************************************************************/
/****************************** shLinkAdd ************************************/
/*****************************************************************************/

RET_CODE
shLinkAdd(LIST *list, void *thing, ADD_FLAGS flag, char *type_name)

{
   LINK *link;
   if (list->type != shTypeGetFromName ("LINK")) return shListAdd (list, thing,
	flag);
/* Must create a new link */
   link = shLinkNew();
   if (link == NULL) {
      shFatal("shLinkAdd : Cannot allocate storage for new link");
      }
   link->ltype = shTypeGetFromName(type_name);
   link->lptr = thing;
   return(shListAdd (list, link, flag));
}

/*****************************************************************************/
/****************************** shLinkRem ************************************/
/*****************************************************************************/

/* type_name can be NULL */
void *
shLinkRem(LIST *list, void **walkPtr, char**type_name)
{
   void *temp;
   LINK *link = *(LINK **)walkPtr;
   if (link == NULL) return (NULL);	/* Attempted to unlink a NULL link */
   shLinkBackWalk(list, walkPtr, NULL);
			/* walkPtr now points to previous item*/
   link = (LINK*)shListRem(list, link);
   if (type_name != NULL) *type_name = shNameGetFromType(link->ltype);
/** If this is a list of LINKs, return the pointed-to thing and delete the
**  LINK
*/
   if (link->type == shTypeGetFromName("LINK")) {
	temp = link->lptr;
	link->lptr = NULL;
	shLinkDel(link);
   } else {
	temp = (void *)link;
	}
   return (temp);
}

/*****************************************************************************/
/****************************** shLinkNew ************************************/
/*****************************************************************************/

LINK *
shLinkNew(void)
{
   LINK *link = (LINK *)shMalloc(sizeof(LINK));
   if (link == NULL) return (NULL);
   link->next = link->prev = NULL;
   link->type = shTypeGetFromName("LINK");
   link->lptr = NULL;
   
   return(link);
}

/*****************************************************************************/
/****************************** shLinkDel ************************************/
/*****************************************************************************/

RET_CODE
shLinkDel(LINK *link)
{
   if (link == NULL) return SH_GENERIC_ERROR;
   if (link->next == NULL && link->prev == NULL && link->lptr == NULL)
      {shFree(link); return (SH_SUCCESS);}
   return SH_GENERIC_ERROR;
}

/*****************************************************************************/
/****************************** shLinkWalk ***********************************/
/*****************************************************************************/


void *
shLinkWalk(LIST *list, void **walkPtr, char *type_name)
{
   LIST_ELEM *elem = *(LIST_ELEM **)walkPtr;

   if (elem == NULL) {
      if(list == NULL) {
         return(NULL);
      }
      *walkPtr = (void *)list->first;
   } else {
      *walkPtr = (void *)elem->next;
   }
   if (*walkPtr == NULL) return (NULL);
/** If this is a list of LINKs, return the pointed-to things. **/
   if ( (*(LIST_ELEM **)walkPtr)->type != shTypeGetFromName("LINK"))
       {return (*walkPtr);}

/* Check to see if type is what was requested */
   if (type_name != NULL) {
	if ( (*(LINK **)walkPtr)->ltype != shTypeGetFromName(type_name))
	   return (NULL);
	}
   return ( (*(LINK **)walkPtr)->lptr);
}
/*****************************************************************************/
/****************************** shLinkWalkTypeGet ****************************/
/*****************************************************************************/


void *shLinkWalkTypeGet(LIST *list, void **walkPtr, TYPE *type) {
  LIST_ELEM *elem = *(LIST_ELEM **)walkPtr;

  if (elem == NULL) {
    if (list == NULL) {
      /* This is a blank list */
      *type = 0;
      return(NULL);
    }
    /* This is the beginning of a list */
    *walkPtr = (void *)list->first;
  } else {
    /* Walk to the next element on the list */
    *walkPtr = (void *)elem->next;
  }
  if (*walkPtr == NULL) {
    /* Reached the end of the list */
    *type = 0;
    return (NULL);
  }
  if ( (*(LIST_ELEM **)walkPtr)->type != shTypeGetFromName("LINK")) {
    /* This is a RHL list */
    *type = (*(LIST_ELEM **)walkPtr)->type;
    return (*walkPtr);
  }
  /* This is a SMK list */
  *type = ( (*(LINK **)walkPtr)->ltype);
  return  ( (*(LINK **)walkPtr)->lptr);
}

/*****************************************************************************/
/****************************** shLinkBackWalk *******************************/
/*****************************************************************************/

void *
shLinkBackWalk(LIST *list, void **walkPtr, char *type_name)
{
   LIST_ELEM *elem = *(LIST_ELEM **)walkPtr;

   if (elem == NULL) {
	if(list == NULL) {
	   return(NULL);
	   }
	*walkPtr = (void *)list->last;
   } else {
      *walkPtr = (void *)elem->prev;
   }
   if (*walkPtr == NULL) return (NULL);
/** If this is a list of LINKs, return the pointed-to things. **/
   if ( (*(LIST_ELEM **)walkPtr)->type != shTypeGetFromName("LINK"))
       {return (*walkPtr);}

/* Check to see if type is what was requested */
   if (type_name != NULL) {
	if ( (*(LINK **)walkPtr)->ltype != shTypeGetFromName(type_name))
	   return (NULL);
	}
   return ( (*(LINK **)walkPtr)->lptr);
}

/*****************************************************************************/
/****************************** shLinkPurge **********************************/
/*****************************************************************************/

RET_CODE 
shLinkPurge(LIST *list, RET_CODE (*del_func)(void *), char *type_name)
{
   void *walkPtr;
   void *tmp;

   if(list == NULL) return SH_SUCCESS;
   walkPtr = NULL;
   while(1) {
/* tmp points to item pointed to by the link */
	tmp = shLinkWalk(list, &walkPtr, type_name);
	if (walkPtr == NULL) break;
	if (tmp == NULL) return (SH_GENERIC_ERROR);
/* shLinkRem returns the type name, if not NULL.  Don't care here */
	tmp = shLinkRem (list, &walkPtr, NULL);
	(*del_func)(tmp);
	}
   return SH_SUCCESS;
}

/*============================================================================
**============================================================================
**
** ROUTINE: shVctrList
**
** DESCRIPTION:
**	Convert a linked list to an array of pointer to the items on the list
**	 Make sure that everything has the same type; else abort and return a
**	NULL pointer
**
** RETURN VALUES:
**	Pointer to array, or null if failure
**
** CALLS TO:
**	shMalloc
**
**
**============================================================================
*/


/*****************************************************************************/
/* Function to return the most useful type on a linked list */

static TYPE which (LIST_ELEM *elem) {
   if (elem->type == shTypeGetFromName("LINK")) {
	return (((LINK *)elem)->ltype);
	} else {
	return (elem->type);
	}
   }

/*****************************************************************************/

void **shVctrList(LIST *list, int *count, TYPE *type) {

/* Keep 2 iteration pointers */
   void *Ptr;
   void *linkPtr;
   void **array;
   LIST_ELEM *elem;
   int nlist;
   int i;

/* Begin code */

   Ptr=NULL;

/* Count the number of items on the list */
   nlist = shListSize(list);
   if (nlist == 0) {
	*count = 0;
	return(NULL);
	}
/* Malloc the array.  Note: I add one extra element that will be set to
** null; some routines (particularly libfits) need that extra null  */

   array = (void **)shMalloc((nlist+1)*sizeof (void *));
   if (array == NULL) return array;
   Ptr=NULL;
   linkPtr = NULL;

/* Set pointers */
/* This should work for all types of lists */
   for (i=0; i<nlist; i++) {
	array[i] = shLinkWalk (list, &Ptr, NULL);
/* First element, get the type; for the remainer, check for consistency */
	if (i == 0) {
		elem = (LIST_ELEM *)shListWalk (list, &linkPtr);
		*type = which(elem);
	   } else {
		elem = (LIST_ELEM *)shListWalk (list, &linkPtr);
		if (*type != which(elem)) {
			shVctrFree(array);
			return (NULL);
			}
		}
	}
   array [nlist] = NULL;
   if (count != NULL) *count = nlist;
   return array;
}

/*============================================================================
**============================================================================
**
** ROUTINE: shVctrFree
**
** DESCRIPTION:
**	Undo the above malloc
**
** RETURN VALUES:
**	None
**
** CALLS TO:
**	shFree
**
**
**============================================================================
*/

void shVctrFree(void **array) {
   shFree(array);
}

