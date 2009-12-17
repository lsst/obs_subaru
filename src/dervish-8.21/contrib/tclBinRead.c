/*
 * TCL support for reading a binary file into a structure
 */
#include <stdio.h>
#include <string.h>
#include "dervish.h"
#include "shChain.h"

static char *module = "contrib";	/* name of this set of code */

/*****************************************************************************/
/*************************** tclBinRead **********************************/
/*****************************************************************************/
/*
 * Read binary data from a file into a buffer pointed to by a handle
 */
static char *tclBinRead_use =
  "USAGE: binRead fileId handle [size]";
static char *tclBinRead_hlp =
"Read binary data from file; <size> bytes if specified; else size from schema";

static int
tclBinRead(
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{
   HANDLE handle;
   void *userPtr;
   int size;
   int nread;
   char buf[80];
   const SCHEMA *schema;
   FILE *file;
 
   if(argc != 3 && argc != 4) {
	Tcl_SetResult(interp,tclBinRead_use,TCL_STATIC);
	return(TCL_ERROR);
	}

/* Translate file handle */
   if (Tcl_GetOpenFile (interp, argv[1], 0, 0, &file) != TCL_OK)
	return TCL_ERROR;

/* Get Handle */
   if (shTclHandleExprEval (interp, argv[2], &handle, &userPtr)
	!= TCL_OK) return TCL_ERROR;

/* Get size if input */
   if (argc == 4) {
	if (Tcl_GetInt (interp, argv[3], &size) != TCL_OK) return TCL_ERROR;
   } else {
	schema = shSchemaGet (shNameGetFromType(handle.type));
	if (schema == NULL) {
	   Tcl_SetResult (interp, "Can't get schema for input handle",
		TCL_STATIC);
	   return TCL_ERROR;
	   }
	size = schema->size;
	}
   nread = fread (handle.ptr, 1, size, file);
   sprintf (buf, "%d", nread);
   Tcl_SetResult (interp, buf, TCL_VOLATILE);
   return TCL_OK;
}
/*****************************************************************************/
/*************************** tclBinWrite **********************************/
/*****************************************************************************/
/*
 * Write binary data to a file from a buffer pointed to by a handle
 */
static char *tclBinWrite_use =
  "USAGE: binWrite fileId handle [size]";
static char *tclBinWrite_hlp =
"Write binary data to a file; <size> bytes if specified; else size from schema";

static int
tclBinWrite(
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{
   HANDLE handle;
   void *userPtr;
   int size;
   int nwrite;
   char buf[80];
   const SCHEMA *schema;
   FILE *file;
 
   if(argc != 3 && argc != 4) {
	Tcl_SetResult(interp,tclBinWrite_use,TCL_STATIC);
	return(TCL_ERROR);
	}

/* Translate file handle */
   if (Tcl_GetOpenFile (interp, argv[1], 0, 0, &file) != TCL_OK)
	return TCL_ERROR;

/* Get Handle */
   if (shTclHandleExprEval (interp, argv[2], &handle, &userPtr)
	!= TCL_OK) return TCL_ERROR;

/* Get size if input */
   if (argc == 4) {
	if (Tcl_GetInt (interp, argv[3], &size) != TCL_OK) return TCL_ERROR;
   } else {
	schema = shSchemaGet (shNameGetFromType(handle.type));
	if (schema == NULL) {
	   Tcl_SetResult (interp, "Can't get schema for input handle",
		TCL_STATIC);
	   return TCL_ERROR;
	   }
	size = schema->size;
	}
   nwrite = fwrite (handle.ptr, 1, size, file);
   sprintf (buf, "%d", nwrite);
   Tcl_SetResult (interp, buf, TCL_VOLATILE);
   return TCL_OK;
}
/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
shTclBinReadDeclare(Tcl_Interp *interp) 
{
   shTclDeclare(interp,"binRead", 
		(Tcl_CmdProc *)tclBinRead, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclBinRead_hlp, tclBinRead_use);

   shTclDeclare(interp,"binWrite", 
		(Tcl_CmdProc *)tclBinWrite, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclBinWrite_hlp, tclBinWrite_use);

   return;
}

