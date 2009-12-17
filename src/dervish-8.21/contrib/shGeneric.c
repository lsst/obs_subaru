/*****************************************************************************
******************************************************************************
**
** FILE:
**	tclGeneric.c
**
** ABSTRACT:
**	This file contains code to create and initialize generic
**	objects.   One supplies a type.  An object of it size is created
**	and all bytes set to 0.
**
** ENTRY POINT			SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
** shGenericNew			public	Action routine to create new object
** shGenericDel			public	Action routine to delete object
** shGenericChainDel		public	Action Delete a chain and objects
**					thereon.
**
** ENVIRONMENT:
**	ANSI C.
**
** REQUIRED PRODUCTS:
**	dervish
**
** AUTHORS:
**	Creation date:
**	
**
******************************************************************************
******************************************************************************
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dervish.h"
#include "shGeneric.h"


/*============================================================================  
**
** ROUTINE: shGenericNew
**
** DESCRIPTION:
**	Create new Generic object.  Ignore all constructors.
**
**============================================================================
*/
void *shGenericNew(char *typeName) {

   void *object;
   const SCHEMA *schema;

   if ((schema = shSchemaGet(typeName)) == NULL) return NULL;
/* Allocate storage for object */
   object = (void *)shMalloc(schema->size);
   memset (object, '\0', (size_t)(schema->size));
   return object;
}

/*============================================================================  
**
** ROUTINE: shGenericDel
**
** DESCRIPTION:
**	Delete a generic object
**	
**============================================================================
*/

void shGenericDel(void *object) {

   if (shIsShMallocPtr(object) != 0) {
	shFree(object);
	}
}
/*============================================================================  
**
** ROUTINE: shGenericChainDel
**
** DESCRIPTION:
**	Delete a chain of generic objects
**      Its main difference with shChainDel is that it DOES delete the
**      object themselves!
**	
**============================================================================
*/

void shGenericChainDel(CHAIN *chain) {

   CURSOR_T cursor;
   void *object;

   cursor = shChainCursorNew(chain);
   while (1) {
	object = shChainWalk (chain, cursor, NEXT);
	if (object == NULL) break;
	object = shChainElementRemByCursor (chain, cursor);
	if (shIsShMallocPtr(object) != 0) {
		shFree(object);
		}
	}
   shChainCursorDel(chain, cursor);
   shChainDel(chain);
}
