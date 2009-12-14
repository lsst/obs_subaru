/*
 * TCL support for CALIB1BYTIME type in photo.
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
 * return a handle to a new CALIB1BYTIME
 */
static char *tclCalib1bytimeNew_use =
  "USAGE: calib1bytimeNew <filters>";
#define tclCalib1bytimeNew_hlp \
  "create a new CALIB1BYTIME"

static ftclArgvInfo calib1bytimeNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCalib1bytimeNew_hlp},
   {"<filters>", FTCL_ARGV_STRING, NULL, NULL,
				    "Filters in CALIB1BYTIME, e.g. \"ugriz\""},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define calib1bytimeNew_name "calib1bytimeNew"

static int
tclCalib1bytimeNew(ClientData clientData,
		   Tcl_Interp *interp,
		   int ac,
		   char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i, j;
   char *filtersStr = NULL;		/* Filters e.g. "ugriz" */
   char filters[MAXCOLOR + 1];		/* filterStr without whitespace */
   CALIB1BYTIME *calib1bytime;

   shErrStackClear();

   i = 1;
   calib1bytimeNew_opts[i++].dst = &filtersStr;
   shAssert(calib1bytimeNew_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, calib1bytimeNew_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     calib1bytimeNew_name) != FTCL_ARGV_SUCCESS) {
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
   calib1bytime = phCalib1bytimeNew(filters);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = calib1bytime;
   hand.type = shTypeGetFromName("CALIB1BYTIME");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"calib1btime\" to new "
		    "CALIB1BYTIME handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Make a CALIB1BYTIME from a CALIB_IO 
 */

/*
 * variables for argument parsing
 */
static char *calibIoStr;
static char *tclFilters;
static char *indexStr;

static char *tclCalib1bytimeNewFromCalibIo_use =
  "USAGE: calib1bytimeNewFromCalibIo calibIo";
#define tclCalib1bytimeNewFromCalibIo_hlp \
  "Create a CALIB1BYTIME from a CALIB_IO."

static ftclArgvInfo calib1bytimeNewFromCalibIo_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCalib1bytimeNewFromCalibIo_hlp},
   {"<calibIo>", FTCL_ARGV_STRING, NULL, &calibIoStr,
      "The CALIBIO to convert to an CALIB1BYTIME"},
   {"<filterlist>", FTCL_ARGV_STRING, NULL, &tclFilters,
      "tcl list of filternames for CALIB1BYTIME"},
   {"<index>", FTCL_ARGV_STRING, NULL, &indexStr,
      "tcl array of positions in calibIo for each filter in filterlist"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclCalib1bytimeNewFromCalibIo(
	      ClientData clientDatag,
	      Tcl_Interp *interp,
	      int ac,
	      char **av
	      )
{
    char **argv;
    int i, j, argc, index[MAXCOLOR];
    char filters[MAXCOLOR + 1];
    char name[HANDLE_NAMELEN];
    HANDLE hand;
    CALIB1BYTIME *ctime;
    char *string;
    void *vptr;				/* used by shTclHandleExprEval */
    
    shErrStackClear();
		      
    if(get_FtclOpts(interp,&ac,av,calib1bytimeNewFromCalibIo_opts) != TCL_OK) {
	return(TCL_ERROR);
    }
    /*
     * deal with arguments
     */
    if(shTclHandleExprEval(interp,calibIoStr,&hand,&vptr) != TCL_OK) {
	return(TCL_ERROR);
    }
    if(hand.type != shTypeGetFromName("CALIB_IO")) {
	Tcl_SetResult(interp,"calib1bytimeNewFromCalibIo: argument is not a CALIB_IO",TCL_STATIC);
	return(TCL_ERROR);
    }
    /* convert tclFilters to a string */
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

    /* Get the new calib1bytime */
   ctime = phCalib1bytimeNewFromCalibIo(hand.ptr,filters,index);

/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = ctime;
   hand.type = shTypeGetFromName("CALIB1BYTIME");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new CALIB_IO handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/
/*****************************************************************************/
/*
 * Make a CALIB_IO from a CALIB1BYTIME
 */

static char *tclCalibIoNewFromCalib1bytime_use =
  "USAGE: CalibIoNewFromCalib1bytime calib1bytime";
#define tclCalibIoNewFromCalib1bytime_hlp \
  "Create a CALIB_IO from a CALIB1BYTIME."

static ftclArgvInfo calibIoNewFromCalib1bytime_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCalibIoNewFromCalib1bytime_hlp},
   {"<calib1bytime>", FTCL_ARGV_STRING, NULL, NULL,
				  "The CALIB1BYTIME to convert to an CALIB_IO"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclCalibIoNewFromCalib1bytime(
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
   char *calib1bytimeStr = NULL;	/* The CALIB1BYTIME to convert to an CALIB_IO */

   shErrStackClear();


   i = 1;
   calibIoNewFromCalib1bytime_opts[i++].dst = &calib1bytimeStr;
   shAssert(calibIoNewFromCalib1bytime_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,calibIoNewFromCalib1bytime_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,calib1bytimeStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CALIB1BYTIME")) {
       Tcl_SetResult(interp,"calibIoNewFromCalib1bytime: argument is not a CALIB1BYTIME",TCL_STATIC);
       return(TCL_ERROR);
   }
/*
 * work
 */
   calibIo = phCalibIoNewFromCalib1bytime(hand.ptr);
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
 * Delete an CALIB1BYTIME 
 */
static char *tclCalib1bytimeDel_use =
  "USAGE: calib1bytimeDel calib1bytime ";
static char *tclCalib1bytimeDel_hlp =
  "Delete an CALIB1BYTIME";

static int
tclCalib1bytimeDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *calib1bytime;
   char *opts = "calib1bytime";

   shErrStackClear();

   ftclParseSave("calib1bytimeDel");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      calib1bytime = ftclGetStr("calib1bytime");
      if(p_shTclHandleAddrGet(interp,calib1bytime,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclCalib1bytimeDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phCalib1bytimeDel(handle->ptr);
   (void) p_shTclHandleDel(interp,calib1bytime);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclCalib1bytimeDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,calib1bytimeNew_name,
		(Tcl_CmdProc *)tclCalib1bytimeNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclCalib1bytimeNew_hlp,
		tclCalib1bytimeNew_use);

   shTclDeclare(interp,"calib1bytimeDel",
                (Tcl_CmdProc *)tclCalib1bytimeDel,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclCalib1bytimeDel_hlp, tclCalib1bytimeDel_use);

   shTclDeclare(interp,"calibIoNewFromCalib1bytime",
                (Tcl_CmdProc *)tclCalibIoNewFromCalib1bytime,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclCalibIoNewFromCalib1bytime_hlp, 
		tclCalibIoNewFromCalib1bytime_use);

   shTclDeclare(interp,"calib1bytimeNewFromCalibIo",
                (Tcl_CmdProc *)tclCalib1bytimeNewFromCalibIo,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclCalib1bytimeNewFromCalibIo_hlp, 
		tclCalib1bytimeNewFromCalibIo_use);
}

