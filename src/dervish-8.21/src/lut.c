/*****************************************************************************
******************************************************************************
**
** FILE:
**	tclLut.c
**
** ABSTRACT:
**	This file contains routines that support the definition of TCL verbs
**	that manipulate lookup tables.
**
** ENTRY POINT		SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
** lutNew		local	Create a new lookup table
** lutDel		local	Delete an lookup table
**
** ENVIRONMENT:
**	ANSI C.
**
** REQUIRED PRODUCTS:
**	FTCL
**
** AUTHORS:
**	Eileen Berman
**	Creation date:  Dec 16, 1993
**
******************************************************************************
******************************************************************************
*/
#include <string.h>
#include "ftcl.h"
#include "shCLut.h"
#include "shCUtils.h"
#include "shCGarbage.h"
#include "shCErrStack.h"

/*============================================================================
**============================================================================
**
** LOCAL DEFINITIONS
**
**============================================================================
*/
/*============================================================================
**============================================================================
**
** ROUTINE: shLutNew
**
** DESCRIPTION:
**	Delete a new lookup table.
**
** RETURN VALUES:
**      Success : A pointer the the new logical unit table.
**      Failure : NULL
**
**============================================================================
*/
LUT *shLutNew
   (
   char    *a_name,              /* IN : ascii name of lut */
   LUTTYPE a_type                /* IN : type of lut to create */
   )   
{
LUT      *lutPtr;

/* Use shMalloc to get space for the lut so it can later be dumped if
   desired */
if ((lutPtr = (LUT *)shMalloc(sizeof(LUT))) == NULL)
   {shFatal("shLutNew : cannot allocate space for new lut!!");}

/* Fill in the type of the region */
lutPtr->type = a_type;

/* Malloc enough space for the name. If none was entered, malloc a 1 byte 
   string. */
if (a_name == NULL)
   {
   if ((lutPtr->name = (char *)shMalloc(1)) == NULL)
     {shFatal("shlutNew : cannot malloc lookup table name of length 1!!");}
   lutPtr->name[0] = '\0';
   }
else
   {
   if ((lutPtr->name = (char *)shMalloc(strlen(a_name) + 1)) == NULL)
     {shFatal("shLutNew : cannot malloc lookup table name of length %d!!",
	      (strlen(a_name) + 1) );}
   strcpy(lutPtr->name, a_name);
   }


return(lutPtr);
}
/*============================================================================
**============================================================================
**
** ROUTINE: shLutDel
**
** DESCRIPTION:
**	Delete a lookup table.
**
** RETURN VALUES:
**
**============================================================================
*/
void shLutDel
   (
   LUT  *a_lutPtr              /* IN : pointer to lookup table to delete */
   )   
{
if (a_lutPtr != NULL)
   {
   /* Ok - we have a lut to free - free the name first */
   shFree((char *)a_lutPtr->name);
   a_lutPtr->name = NULL;          /* in case the address is somewhere else */

   a_lutPtr->type = LUTUNKNOWN;    /* in case the address is somewhere else */

   /* Now free the lut itself */
   shFree((char *)a_lutPtr);
   }
}


