/*
 * TCL support for CALIB1BYFRAME type in photo.
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "phCalib1.h"
#include "phUtils.h"

#define MAXCOLOR 5

static char *module = "phTclStructsFacil";    /* name of this set of code */

/*****************************************************************************/
/*
 * make a new CALIB1BYFRAME
 */
static char *tclCalib1byframeNew_use =
  "USAGE: calib1byframeNew <filters>";
#define tclCalib1byframeNew_hlp \
  "Make a new CALIB1BYFRAME"

static ftclArgvInfo calib1byframeNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCalib1byframeNew_hlp},
   {"<filters>", FTCL_ARGV_STRING, NULL, NULL,
				"Filters in CALIB1BYFRAME, e.g. \"ugriz\""},   
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define calib1byframeNew_name "calib1byframeNew"

static int
tclCalib1byframeNew(ClientData clientData,
		    Tcl_Interp *interp,
		    int ac,
		    char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i, j;
   char *filtersStr = NULL;		/* Filter list, e.g. "u g r i z" */
   char filters[MAXCOLOR + 1];		/* filterStr without whitespace */
   CALIB1BYFRAME *calib1byframe;

   shErrStackClear();

   i = 1;
   calib1byframeNew_opts[i++].dst = &filtersStr;
   shAssert(calib1byframeNew_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, calib1byframeNew_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     calib1byframeNew_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * convert filtersStr to a string, removing whitespace
 */
    for(i = j = 0; filtersStr[i] != '\0'; i++) {
       if(!isspace(filtersStr[i])) {
	  filters[j++] = filtersStr[i];
       }
    }
    shAssert(j <= MAXCOLOR);
    filters[j] = '\0';
/*
 * work
 */
   calib1byframe = phCalib1byframeNew(filters);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = calib1byframe;
   hand.type = shTypeGetFromName("CALIB1BYFRAME");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"calib1bframe\" to new "
		    "CALIB1BYFRAME handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete an CALIB1BYFRAME 
 */
static char *tclCalib1byframeDel_use =
  "USAGE: calib1byframeDel calib1byframe ";
static char *tclCalib1byframeDel_hlp =
  "Delete an CALIB1BYFRAME";

static int
tclCalib1byframeDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *calib1byframe;
   char *opts = "calib1byframe";

   shErrStackClear();

   ftclParseSave("calib1byframeDel");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      calib1byframe = ftclGetStr("calib1byframe");
      if(p_shTclHandleAddrGet(interp,calib1byframe,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclCalib1byframeDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phCalib1byframeDel(handle->ptr);
   (void) p_shTclHandleDel(interp,calib1byframe);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}


/*****************************************************************************/
/*****************************************************************************/
/*
 * Make a CALIB1BYFRAME from a CALIB_IO 
 */

/*
 * variables for argument parsing
 */
static char *calibIoStr;
static char *tclFilters;
static char *indexStr;

static char *tclCalib1byframeNewFromCalibIo_use =
  "USAGE: calib1byframeNewFromCalibIo calibIo";
#define tclCalib1byframeNewFromCalibIo_hlp \
  "Create a CALIB1BYFRAME from a CALIB_IO."

static ftclArgvInfo calib1byframeNewFromCalibIo_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCalib1byframeNewFromCalibIo_hlp},
   {"<calibIo>", FTCL_ARGV_STRING, NULL, &calibIoStr,
      "The CALIBIO to convert to an CALIB1BYFRAME"},
   {"<filterlist>", FTCL_ARGV_STRING, NULL, &tclFilters,
      "tcl list of filternames for CALIB1BYFRAME"},
   {"<index>", FTCL_ARGV_STRING, NULL, &indexStr,
      "tcl array of positions in calibIo for each filter in filterlist"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclCalib1byframeNewFromCalibIo(
	      ClientData clientDatag,
	      Tcl_Interp *interp,
	      int ac,
	      char **av
	      )
{
    int i, j, argc, index[MAXCOLOR];
    char filters[MAXCOLOR + 1];
    char *string;
    char **argv;
    char name[HANDLE_NAMELEN];
    HANDLE hand;
    CALIB1BYFRAME *cframe;
    void *vptr;				/* used by shTclHandleExprEval */
    
    shErrStackClear();
		      
    if(get_FtclOpts(interp,&ac,av,calib1byframeNewFromCalibIo_opts) != TCL_OK) {
	return(TCL_ERROR);
    }
    /*
     * deal with arguments
     */
    if(shTclHandleExprEval(interp,calibIoStr,&hand,&vptr) != TCL_OK) {
	return(TCL_ERROR);
    }
    if(hand.type != shTypeGetFromName("CALIB_IO")) {
	Tcl_SetResult(interp,"calib1byframeNewFromCalibIo: argument is not a CALIB_IO",TCL_STATIC);
	return(TCL_ERROR);
    }

    /* convert tclFilters to a string, removing whitespace */
    for(i = j = 0; tclFilters[i] != '\0'; i++) {
       if(!isspace(tclFilters[i])) {
	  filters[j++] = tclFilters[i];
       }
    }
    shAssert(j <= MAXCOLOR);
    filters[j] = '\0';

    /* Get index as array of ints */
    
    if(Tcl_SplitList(interp, tclFilters, &argc, &argv) == TCL_ERROR) {
	Tcl_SetResult(interp,"Error parsing input list of filters",TCL_STATIC);
	return (TCL_ERROR);
    }
		      
    for (i = 0; i < argc; i++) {
	string = Tcl_GetVar2(interp,indexStr,argv[i],0);
	if (Tcl_GetInt(interp, string, &(index[i]))!=TCL_OK) {
	    return (TCL_ERROR);
	}
    }

    /* Get the new calib1byframe */
   cframe = phCalib1byframeNewFromCalibIo(hand.ptr,filters,index);

/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = cframe;
   hand.type = shTypeGetFromName("CALIB1BYFRAME");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new CALIB_IO handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Make a CALIB_IO from a CALIB1BYFRAME
 */

static char *tclCalibIoNewFromCalib1byframe_use =
  "USAGE: CalibIoNewFromCalib1byframe calib1byframe";
#define tclCalibIoNewFromCalib1byframe_hlp \
  "Create a CALIB_IO from a CALIB1BYFRAME."

static ftclArgvInfo calibIoNewFromCalib1byframe_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCalibIoNewFromCalib1byframe_hlp},
   {"<calib1byframe>", FTCL_ARGV_STRING, NULL, NULL,
				 "The CALIB1BYFRAME to convert to an CALIB_IO"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclCalibIoNewFromCalib1byframe(
	      ClientData clientDatag,
	      Tcl_Interp *interp,
	      int ac,
	      char **av
	      )
{
   int i;
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   CALIB_IO *calibIo;
   void *vptr;				/* used by shTclHandleExprEval */
   char *calib1byframeStr = NULL;	/* The CALIB1BYFRAME to convert to an CALIB_IO */

   shErrStackClear();


   i = 1;
   calibIoNewFromCalib1byframe_opts[i++].dst = &calib1byframeStr;
   shAssert(calibIoNewFromCalib1byframe_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,calibIoNewFromCalib1byframe_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,calib1byframeStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CALIB1BYFRAME")) {
       Tcl_SetResult(interp,"calibIoNewFromCalib1byframe: argument is not a CALIB1BYFRAME",TCL_STATIC);
       return(TCL_ERROR);
   }
/*
 * work
 */
   calibIo = phCalibIoNewFromCalib1byframe(hand.ptr);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = calibIo;
   hand.type = shTypeGetFromName("CALIB_IO");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new CALIB_IO handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclCalib1byframeDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,calib1byframeNew_name,
		(Tcl_CmdProc *)tclCalib1byframeNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclCalib1byframeNew_hlp,
		tclCalib1byframeNew_use);

   shTclDeclare(interp,"calib1byframeDel",
                (Tcl_CmdProc *)tclCalib1byframeDel,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclCalib1byframeDel_hlp, tclCalib1byframeDel_use);

   shTclDeclare(interp,"calibIoNewFromCalib1byframe",
                (Tcl_CmdProc *)tclCalibIoNewFromCalib1byframe,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclCalibIoNewFromCalib1byframe_hlp, 
		tclCalibIoNewFromCalib1byframe_use);

   shTclDeclare(interp,"calib1byframeNewFromCalibIo",
                (Tcl_CmdProc *)tclCalib1byframeNewFromCalibIo,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclCalib1byframeNewFromCalibIo_hlp, 
		tclCalib1byframeNewFromCalibIo_use);
}

