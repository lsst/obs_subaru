/*****************************************************************************
******************************************************************************
**
** FILE:
**	tclGeneric.c
**
** ABSTRACT:
**	This file contains TCL support to create an initialize generic
**	objects.   One supplies a type.  An object of it size is created
**	and all bytes set to 0.
**
** ENTRY POINT			SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
** tclGenericDeclare		public  Declare new verbs to tcl
** tclGenericNew		local	Action routine to create new object
** tclGenericDel		local	Action routine to delete object
** tclGenericChain		local	Action routine to delete chain of objs.
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

/* GLOBAL Variables */

static char *module = "contrib";	/* name of this set of code */

static char *tclGenericNew_use =
  "USAGE: genericNew <TYPE>";
static char *tclGenericNew_hlp =
  "Create a new generic object of the specified type; returns a handle";
static char *tclGenericDel_use =
  "USAGE: genericDel <handle>";
static char *tclGenericDel_hlp =
  "Delete a generic object; checks to see if shMalloced";
static char *tclGenericChainDel_use =
  "USAGE: genericChainDel <handle>";
static char *tclGenericChainDel_hlp =
  "Delete a chain of generic objects";


/*============================================================================  
**============================================================================
**
** ROUTINE: tclGenericNew
**
** DESCRIPTION:
**	Create new Generic object.  If a constructor exists, use it; else,
**	initialize the object based on its size and set all bytes to 0.
**
** RETURN VALUES:
**	TCL_OK    -   Successful completion.
**	TCL_ERROR -   Failed miserably.
**
** CALLS TO:
**	shErrStackClear
**	Tcl_SetResult
**	shTclHandleNew
**	shSchemaGet
**
** GLOBALS REFERENCED:
**	tclGenericNew_use
**
**============================================================================
*/
static int
tclGenericNew(
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{
   char name[HANDLE_NAMELEN];
   void *object;
   
   shErrStackClear();
   
   if(argc != 2) {
      Tcl_SetResult(interp,tclGenericNew_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   object = shGenericNew(argv[1]);
   if (object == NULL) {
	Tcl_AppendResult(interp, "Cannot allocate storage for object",NULL);
	return TCL_ERROR;
	}
   if (shTclHandleNew (interp, name, argv[1], object) != TCL_OK) {
	shGenericDel(object);
        return(TCL_ERROR);
        }
   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}


/*============================================================================  
**============================================================================
**
** ROUTINE: tclGenericDel
**
** DESCRIPTION:
**	Delete a generic object
**	
**
** RETURN VALUES:
**	TCL_OK    -   Successful completion.
**	TCL_ERROR -   Failed miserably.
**
** CALLS TO:
**	shErrStackClear
**	ftclParseSave
**	Tcl_SetResult
**	shTclAddrGetFromName
**	p_shTclHandleDel
**
** GLOBALS REFERENCED:
**	tclGenericDel_use
**
**============================================================================
*/

static int
tclGenericDel(
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{
   HANDLE handle;

   if (argc != 2) {
        Tcl_SetResult(interp,tclGenericDel_use,TCL_STATIC);
        return(TCL_ERROR);
        }

/* Since we don't know the type of the supplied object, get it and and use
 * it for the translation.  If we had been clever, we would not have done
 * type checking if a NULL were supplied for the type name to shTclAddrGet...
*/
   if (shTclHandleGetFromName (interp, argv[1], &handle) != TCL_OK)
	return TCL_ERROR;

   shGenericDel(handle.ptr);

/* Delete handle */

   p_shTclHandleDel(interp, argv[1]);
   return(TCL_OK);
}

/*============================================================================  
**============================================================================
**
** ROUTINE: tclGenericChainDel
**
** DESCRIPTION:
**	Delete a Chain of generic things.
**	
**
** RETURN VALUES:
**	TCL_OK    -   Successful completion.
**	TCL_ERROR -   Failed miserably.
**
** CALLS TO:
**	p_shTclHandleDel
**
** GLOBALS REFERENCED:
**	tclGenericDel_use
**
**============================================================================
*/

static int
tclGenericChainDel(
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{
   CHAIN *chain=NULL;

   if (argc != 2) {
        Tcl_SetResult(interp,tclGenericChainDel_use,TCL_STATIC);
        return(TCL_ERROR);
        }

/* Translate Chain handle */
   if (shTclAddrGetFromName (interp, argv[1], (void *)&chain,
	   "CHAIN") != TCL_OK) {
        return(TCL_ERROR);
        }

/* Clear out and Delete the chain */
   shGenericChainDel(chain);

/* Delete handle */
   p_shTclHandleDel(interp, argv[1]);

   return(TCL_OK);
}


/*============================================================================  
**============================================================================
**
** ROUTINE: shTclGenericDeclare
**
** DESCRIPTION:
**	Declare verbs to create and delete generic objects to tcl
**	
**
** RETURN VALUES:
**	0   -   Successful completion.
**	1   -   Failed miserably.
**
** CALLS TO:
**	shTclDeclare
**
** GLOBALS REFERENCED:
**	tclGenericNew
**	tclGenericNew_hlp
**	tclGenericNew_use
**	tclGenericDel
**	tclGenericDel_hlp
**	tclGenericDel_use
**
**============================================================================
*/
void
shTclGenericDeclare(Tcl_Interp *interp) 
{
   shTclDeclare(interp,"genericNew",
		(Tcl_CmdProc *)tclGenericNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclGenericNew_hlp, tclGenericNew_use);

   shTclDeclare(interp,"genericDel", 
		(Tcl_CmdProc *)tclGenericDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclGenericDel_hlp, tclGenericDel_use);

   shTclDeclare(interp,"genericChainDel",
		(Tcl_CmdProc *)tclGenericChainDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclGenericChainDel_hlp, tclGenericChainDel_use);
} 
