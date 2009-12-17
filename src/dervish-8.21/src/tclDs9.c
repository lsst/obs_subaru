/*
 * Support for DS9
 *
 * The sao commands and their equivalents are:
 *			xpaGet C
 *			xpaSet C
 *			ds9DisplayPrim C
 *
 * saoCenter		ds9Center T
 * saoDel		ds9Del U
 * saoDisplay		ds9Display C
 * saoDrawArrow		ds9DrawArrow U
 * saoDrawBox		ds9DrawBox U
 * saoDrawCircle	ds9DrawCircle T
 * saoDrawEllipse	ds9DrawEllipse U
 * saoDrawPoint		ds9DrawPoint U
 * saoDrawPolygon	ds9DrawPolygon T
 * saoDrawText		ds9DrawText T
 * saoGetRegion		ds9GetRegion U
 * saoLabel		ds9Label D
 * saoListRegion	ds9ListRegion T
 * saoMaskColorSet	ds9MaskColorSet D
 * saoMaskDisplay	ds9MaskDisplay D
 * saoMaskGlyphSet	ds9MaskGlyphSet D
 * saoPan		ds9Pan T
 * saoReset		ds9Reset T
 * saoSaveMouseCommands	ds9SaveMouseCommands U
 * saoZoom		ds9Zoom T
 *
 * Commands marked "C" are C primitives;
 * ones marked "D" have dummy equivalents
 * commands marked T are implemented in tcl (in ds9.tcl),
 * and commands marked "U" are not (yet?) implemented
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dervish.h"
#include "shCDs9.h"

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
char *module = "shTclDs9";	/* name of this set of code */

/*****************************************************************************/
/*
 * Two primitives, the equivalent of exec xpa[sg]et, but handling
 * Zombies correctly.  Also, if I ever link in the xpa libraries
 * I can don't need to change any of the tcl layer
 */
static char *tclXpaSet_use =
  "USAGE: xpaSet <cmd> <buf>";
#define tclXpaSet_hlp \
  "Run an xpaSet command"

static ftclArgvInfo xpaSet_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclXpaSet_hlp},
   {"<cmd>", FTCL_ARGV_STRING, NULL, NULL, "The command to execute"},
   {"[buf]", FTCL_ARGV_STRING, NULL, NULL, "Arguments to the command"},   
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define xpaSet_name "xpaSet"

static int
tclXpaSet(ClientData clientData,
	  Tcl_Interp *interp,
	  int ac,
	  char **av)
{
   int a_i;
   char *cmdStr = NULL;			/* The command to execute */
   char *bufStr = "";			/* Arguments to the command */

   shErrStackClear();

   a_i = 1;
   xpaSet_opts[a_i++].dst = &cmdStr;
   xpaSet_opts[a_i++].dst = &bufStr;
   shAssert(xpaSet_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, xpaSet_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     xpaSet_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   if(shXpaSet(cmdStr, bufStr) < 0) {
      Tcl_SetResult(interp, "xpaSet failed", TCL_STATIC);
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclXpaGet_use =
  "USAGE: xpaGet <cmd>";
#define tclXpaGet_hlp \
  ""

static ftclArgvInfo xpaGet_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclXpaGet_hlp},
   {"<cmd>", FTCL_ARGV_STRING, NULL, NULL, "Command specifying desired items"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define xpaGet_name "xpaGet"

static int
tclXpaGet(ClientData clientData,
	  Tcl_Interp *interp,
	  int ac,
	  char **av)
{
   int a_i;
   char *cmdStr = NULL;			/* Command specifying desired items */
   const char *reply;			/* reply to command */

   shErrStackClear();

   a_i = 1;
   xpaGet_opts[a_i++].dst = &cmdStr;
   shAssert(xpaGet_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, xpaGet_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     xpaGet_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   if((reply = shXpaGet(cmdStr)) == NULL) {
      Tcl_SetResult(interp, "xpaGet failed", TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, (char *)reply, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclDs9DisplayPrim_use =
  "USAGE: ds9DisplayPrim <region> -mask";
#define tclDs9DisplayPrim_hlp \
  "Request ds9 to display an image.  This is the primitive used by ds9Display"

static ftclArgvInfo ds9DisplayPrim_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclDs9DisplayPrim_hlp},
   {"<region>", FTCL_ARGV_STRING, NULL, NULL, "Region to display"},
   {"-mask", FTCL_ARGV_CONSTANT, (void *)1, NULL,"Region is really a mask"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define ds9DisplayPrim_name "ds9DisplayPrim"

static int
tclDs9DisplayPrim(ClientData clientData,
	      Tcl_Interp *interp,
	      int ac,
	      char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regionStr = NULL;		/* Region to display */
   REGION *region = NULL;		/* one of these is */
   MASK *mask = NULL;			/*    bound to handle regionStr */
   int is_mask = 0;			/* Region is really a mask */

   shErrStackClear();

   a_i = 1;
   ds9DisplayPrim_opts[a_i++].dst = &regionStr;
   ds9DisplayPrim_opts[a_i++].dst = &is_mask;
   shAssert(ds9DisplayPrim_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, ds9DisplayPrim_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     ds9DisplayPrim_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regionStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(is_mask) {
      if(hand.type != shTypeGetFromName("MASK")) {
	 Tcl_AppendResult(interp,"ds9DisplayPrim: "
			  "argument \"", regionStr, "\" is not a MASK",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
      mask = hand.ptr;
   } else {
      if(hand.type != shTypeGetFromName("REGION")) {
	 Tcl_AppendResult(interp,"ds9DisplayPrim: "
			  "argument \"", regionStr, "\" is not a REGION",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
      region = hand.ptr;
   }
/*
 * work
 */
   if(shXpaFits(region, mask) < 0) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/

void
shTclDs9Declare(Tcl_Interp *interp)
{
   shTclDeclare(interp,xpaSet_name,
		(Tcl_CmdProc *)tclXpaSet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclXpaSet_hlp,
		tclXpaSet_use);

   shTclDeclare(interp,xpaGet_name,
		(Tcl_CmdProc *)tclXpaGet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclXpaGet_hlp,
		tclXpaGet_use);

   shTclDeclare(interp,ds9DisplayPrim_name,
		(Tcl_CmdProc *)tclDs9DisplayPrim, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclDs9DisplayPrim_hlp,
		tclDs9DisplayPrim_use);
}
