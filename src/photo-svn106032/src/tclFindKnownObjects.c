/*****************************************************************************
 * 
 *
 * FILE: tclFindKnownObjects.c
 *
 * DESCRIPTION:
 *  TCL support for FindKownObjects module
 *      
 * AUTHORS:	                        Creation date: March 22, 1995
 *     Nan Ellman 
 *
 * 
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "phKnownobj.h"
#include "phUtils.h"

static char *module = "phTclFindKnownObjects";   /* name of this set of code */

/***************************************************************************
**<AUTO EXTRACT>
** TCL VERB: findKnownObjects
**
** Description:
**
** (not used anymore)
**
**</AUTO>
**********************************************************************/

static char *catstr = NULL;
static char *framestr = NULL;
static char *surveystr = NULL;
static int nrow = 0;
static int ncol = 0;
static int overlap = 0;

static char *tclFindKnownObjects_use =
  "USAGE: findKnownObjects catalog frametrans surveytrans";
static char *tclFindKnownObjects_hlp =
  "Find locations on refcolor frame for all objects in input catalog";

static ftclArgvInfo findKnownObjects_argTable[] = {
   { (char *) NULL, FTCL_ARGV_HELP, NULL, NULL,
         "Find locations on refcolor frame for all objects in input catalog" },
   { "<catalog>", FTCL_ARGV_STRING, NULL, &catstr,
         "known object list" },
   { "<frametrans>", FTCL_ARGV_STRING, NULL, &framestr,
         "TRANS from refcolor to survey coord" },
   { "<surveytrans>", FTCL_ARGV_STRING, NULL, &surveystr,
         "TRANS from survey to equatorial coordinates." },
   { "<nrow>", FTCL_ARGV_INT, NULL, &nrow,
         "number of rows in raw frame" },
   { "<ncol>", FTCL_ARGV_INT, NULL, &ncol,
         "number of cols in raw frame" },
   { "<overlap>", FTCL_ARGV_INT, NULL, &overlap,
         "number of rows in overlap region" },
   { (char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL }
};



static int
tclFindKnownObjects(
          ClientData clientData,

          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
  HANDLE handle;
  CHAIN *catalog = NULL,  *knownobjs = NULL;
  TRANS *surveytrans = NULL, *frametrans = NULL;
  char name [HANDLE_NAMELEN];
   /* 
    * we have to set this by hand upon entering function; otherwise,
    * it retains value that user last set.
    */
  catstr=NULL;
  framestr=NULL;
  surveystr=NULL;

  shErrStackClear();

  if(get_FtclOpts(interp,&argc,argv,findKnownObjects_argTable) != TCL_OK) {
     return(TCL_ERROR);
  }

  if (shTclAddrGetFromName(interp, catstr, (void **)&catalog, "CHAIN") 
      != TCL_OK) {
      shError("tclFindKnownObjects: can't get catalog");
      return (TCL_ERROR);
  } 
  if (shTclAddrGetFromName(interp, framestr, (void **)&frametrans, "TRANS") 
      != TCL_OK) {
      shError("tclFindKnownObjects: can't get frametrans");
      return (TCL_ERROR);
  }
  if (shTclAddrGetFromName(interp, surveystr, (void **)&surveytrans, "TRANS") 
      != TCL_OK) {
      shError("tclFindKnownObjects: can't get surveytrans");
      return (TCL_ERROR);
  }
				  
  if ((knownobjs = phFindKnownObjects(catalog,frametrans,surveytrans,
				      nrow,ncol,overlap)) == NULL) {
      Tcl_SetResult(interp, "phFindKnownObjects returns null",TCL_STATIC);
      return(TCL_ERROR);
  } else {

/*
 * ok, get a handle for our new KNOWNOBJ
 */
      if(p_shTclHandleNew(interp,name) != TCL_OK) {
	  shTclInterpAppendWithErrStack(interp);
	  return(TCL_ERROR);
      }
      handle.ptr = knownobjs;
      handle.type = shTypeGetFromName("CHAIN");
      
      if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
	  Tcl_SetResult(interp,"Can't bind to new chain handle",TCL_STATIC);
	  return(TCL_ERROR);
      }


   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);

  }
}

/***************************************************************************
**<AUTO EXTRACT>
** TCL VERB: cutKnownObjects
**
** Description:
**
** Make OBJCs for all objects in the input knownObject list.
**
**</AUTO>
**********************************************************************/

static char *tclCutKnownObjects_use =
  "USAGE: cutKnownObjects <knownobj> <fiparams>";
#define tclCutKnownObjects_hlp \
  "Return a chain of OBJCs for all knownobjects"

static ftclArgvInfo cutKnownObjects_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCutKnownObjects_hlp},
   {"<knownobj>", FTCL_ARGV_STRING, NULL, NULL, "Chain of KNOWNOBJs"},
   {"<fiparams>", FTCL_ARGV_STRING, NULL, NULL, "FIELDPARAMS for astrometry"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define cutKnownObjects_name "cutKnownObjects"

static int
tclCutKnownObjects(ClientData clientData,
		   Tcl_Interp *interp,
		   int ac,
		   char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *knownobjStr = NULL;		/* Chain of KNOWNOBJs */
   CHAIN *knownobj;
   char *fiparamsStr = NULL;		/* FIELDPARAMS with e.g. TRANS */
   FIELDPARAMS *fiparams;
   CHAIN *objs;				/* returned chain of OBJCs */

   shErrStackClear();

   i = 1;
   cutKnownObjects_opts[i++].dst = &knownobjStr;
   cutKnownObjects_opts[i++].dst = &fiparamsStr;
   shAssert(cutKnownObjects_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, cutKnownObjects_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     cutKnownObjects_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,knownobjStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_AppendResult(interp,"cutKnownObjects: "
                       "argument \"%s\" is not a CHAIN", knownobjStr, NULL);
      return(TCL_ERROR);
   }
   knownobj = hand.ptr;
   if(knownobj->type != shTypeGetFromName("KNOWNOBJ")) {
      Tcl_AppendResult(interp,"cutKnownObjects: "
                       "argument \"%s\" is not of type KNOWNOBJ",
		       knownobjStr, NULL);
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,fiparamsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FIELDPARAMS")) {
      Tcl_AppendResult(interp,"cutKnownObjects: "
                       "argument \"%s\" is not a FIELDPARAMS",
		       fiparamsStr, NULL);
      return(TCL_ERROR);
   }
   fiparams = hand.ptr;
/*
 * work
 */
   objs = phCutKnownObjects(knownobj, fiparams);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = objs;
   hand.type = shTypeGetFromName("CHAIN");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"objs\" to new CHAIN handle",
		    TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************
 * Declare my new tcl verbs to tcl
 */
void
phTclFindKnownObjectsDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"findKnownObjects",
                (Tcl_CmdProc *)tclFindKnownObjects,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclFindKnownObjects_hlp, tclFindKnownObjects_use);

   shTclDeclare(interp,cutKnownObjects_name,
		(Tcl_CmdProc *)tclCutKnownObjects, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclCutKnownObjects_hlp,
		tclCutKnownObjects_use);
}
