/* TCL interface to hg utilities */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <math.h>
#include "shTclParseArgv.h"
#include "region.h"
#include "shCSao.h"
#include "shCDiskio.h"
#include "shCUtils.h"
#include "shCErrStack.h"
#include "shTclErrStack.h"
#include "shTclHandle.h"
#include "shTclRegSupport.h"
#include "shTclUtils.h"
#include "shCVecExpr.h"
#include "shCHg.h"

#define isgood(x)   ( ( ((x) < FLT_MAX)) && ((x) > -FLT_MAX) ? (1) : (0))
#define isgoodint(x)   ( ( ((x) < INT_MAX)) && ((x) > INT_MIN) ? (1) : (0))

extern HG *mnHg;


static char *tclFacil = "shHg";	/* name for this set of code */

/****************************************************************************
 **
 ** ROUTINE: tclHgNew
 **
 **<AUTO EXTRACT>
 ** TCL VERB: hgNew
 **
 **</AUTO>
 */
char g_hgNew[] = "hgNew";
ftclArgvInfo g_hgNewTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
     "Make a new HG\n" },
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclHgNew(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  char name[HANDLE_NAMELEN];
  int rstat;
  HG *hg;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_hgNewTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_hgNew)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  hg = shHgNew();
  if (shTclHandleNew(interp, name, "HG", hg) != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to new hg handle", TCL_VOLATILE);
    return(TCL_ERROR);
  }
  Tcl_SetResult(interp, name, TCL_VOLATILE);
  return(TCL_OK);
}



/****************************************************************************
**
** ROUTINE: tclHgDefine
**
**<AUTO EXTRACT>
** TCL VERB: hgDefine
**
**</AUTO>
*/
char g_hgDefine[] = "hgDefine";
ftclArgvInfo g_hgDefineTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Define an existing HG, with a name, label names, limits, and number of bins.\n" },
  {"<hg>", FTCL_ARGV_STRING, NULL, NULL, 
   "The HG to define"},
  {"-xLabel", FTCL_ARGV_STRING, NULL, NULL, 
   "The x-axis label"},
  {"-yLabel", FTCL_ARGV_STRING, NULL, NULL, 
   "The y-axis label"},
  {"-name", FTCL_ARGV_STRING, NULL, NULL, 
   "The name for the HG"},
  {"-min", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "Low edge of first bin"},
  {"-max", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "High edge of last bin"},
  {"-nbin", FTCL_ARGV_INT, NULL, NULL, 
   "Number of bins"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclHgDefine(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  HG *hg=NULL;
  int rstat;
  char *a_hg = NULL;
  char *a_xLabel = "none";
  char *a_yLabel = "none";
  char *a_name = "none";
  double a_minimum = 0.0;
  double a_maximum = 10.0;
  unsigned int a_nbin = 10;

  g_hgDefineTbl[1].dst = &a_hg;
  g_hgDefineTbl[2].dst = &a_xLabel;
  g_hgDefineTbl[3].dst = &a_yLabel;
  g_hgDefineTbl[4].dst = &a_name;
  g_hgDefineTbl[5].dst = &a_minimum;
  g_hgDefineTbl[6].dst = &a_maximum;
  g_hgDefineTbl[7].dst = &a_nbin;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_hgDefineTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_hgDefine)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName
      (interp, a_hg, (void **) &hg, "HG") != TCL_OK) {
    Tcl_SetResult(interp, "tclHgDefine:  bad HG name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  shHgDefine(hg, a_xLabel, a_yLabel, a_name, a_minimum, a_maximum, a_nbin);
  Tcl_SetResult(interp, a_hg, TCL_VOLATILE);
  return(TCL_OK);
}


/****************************************************************************
**
** ROUTINE: tclHgClear
**
**<AUTO EXTRACT>
** TCL VERB: hgClear
**
**</AUTO>
*/
char g_hgClear[] = "hgClear";
ftclArgvInfo g_hgClearTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Clear a HG, leaving the bin definitions.\n" },
  {"<hg>", FTCL_ARGV_STRING, NULL, NULL, 
   "The HG to clear"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclHgClear(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_hg = NULL;
  HG *hg=NULL;

  g_hgClearTbl[1].dst = &a_hg;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_hgClearTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_hgClear)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName
      (interp, a_hg, (void **) &hg, "HG") != TCL_OK) {
    Tcl_SetResult(interp, "tclHgClear:  bad HG name", TCL_VOLATILE);     
    return TCL_ERROR;
  }
  shHgClear(hg);
  Tcl_SetResult(interp,"",0);
  
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclHgFill
**
**<AUTO EXTRACT>
** TCL VERB: hgFill
**
**</AUTO>
*/
char g_hgFill[] = "hgFill";
ftclArgvInfo g_hgFillTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Fill a HG bin at value with weight.\n" },
  {"<hg>", FTCL_ARGV_STRING, NULL, NULL, 
   "The HG to fill"},
  {"<Value>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "The value of where to fill"},
  {"-weight", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "The weight to fill with"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclHgFill(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_hg = NULL;
  double a_value = 0.;
  double a_weight = 1.;
  HG *hg=NULL;
  
  g_hgFillTbl[1].dst = &a_hg;
  g_hgFillTbl[2].dst = &a_value;
  g_hgFillTbl[3].dst = &a_weight;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_hgFillTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_hgFill)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName
      (interp, a_hg, (void **) &hg, "HG") != TCL_OK) {
    Tcl_SetResult(interp, "tclHgFill:  bad HG name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  shHgFill(hg, a_value, a_weight);
  Tcl_SetResult(interp,"",0);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclHgPrint
**
**<AUTO EXTRACT>
** TCL VERB: hgPrint
**
**</AUTO>
*/
char g_hgPrint[] = "hgPrint";
ftclArgvInfo g_hgPrintTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Print a HG.\n" },
  {"<hg>", FTCL_ARGV_STRING, NULL, NULL, 
   "The HG to print"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclHgPrint(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_hg = NULL;
  HG *hg=NULL;

  g_hgPrintTbl[1].dst = &a_hg;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_hgPrintTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_hgPrint)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName
      (interp, a_hg, (void **) &hg, "HG") != TCL_OK) {
    Tcl_SetResult(interp, "tclHgPrint:  bad HG name", TCL_VOLATILE);    
    return TCL_ERROR;
  }
  shHgPrint(hg);
  Tcl_SetResult(interp,"",0);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclHgMean
**
**<AUTO EXTRACT>
** TCL VERB: hgMean
**
**</AUTO>
*/
char g_hgMean[] = "hgMean";
ftclArgvInfo g_hgMeanTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Print MEAN of a HG.\n" },
  {"<hg>", FTCL_ARGV_STRING, NULL, NULL, 
   "HG to print MEAN of"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclHgMean(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_hg = NULL;
  HG *hg=NULL;
  char answer[40];
  
  g_hgMeanTbl[1].dst = &a_hg;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_hgMeanTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_hgMean)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

    if (shTclAddrGetFromName
	(interp, a_hg, (void **) &hg, "HG") != TCL_OK) {
      Tcl_SetResult(interp, "tclHgMean:  bad HG name", TCL_VOLATILE);
      
      return TCL_ERROR;
    }
  Tcl_SetResult(interp,"",0);
  sprintf(answer, "%f", shHgMean(hg));
  Tcl_AppendElement(interp, answer);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclHgSigma
**
**<AUTO EXTRACT>
** TCL VERB: hgSigma
**
**</AUTO>
*/
char g_hgSigma[] = "hgSigma";
ftclArgvInfo g_hgSigmaTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Print SIGMA of a HG.\n" },
  {"<hg>", FTCL_ARGV_STRING, NULL, NULL, 
   "HG to print SIGMA of"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclHgSigma(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_hg = NULL;
  HG *hg=NULL;
  char answer[40];
  
  g_hgSigmaTbl[1].dst = &a_hg;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_hgSigmaTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_hgSigma)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

    if (shTclAddrGetFromName
	(interp, a_hg, (void **) &hg, "HG") != TCL_OK) {
      Tcl_SetResult(interp, "tclHgSigma:  bad HG name", TCL_VOLATILE);
      
      return TCL_ERROR;
    }
  Tcl_SetResult(interp,"",0);
  sprintf(answer, "%f", shHgSigma(hg));
  Tcl_AppendElement(interp, answer);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclHgOper
**
**<AUTO EXTRACT>
** TCL VERB: hgOper
**
**</AUTO>
*/
char g_hgOper[] = "hgOper";
ftclArgvInfo g_hgOperTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Operate on two HGs.  Valid operations are +, m, *, and /.   Note that you want to use m for minus instead of -.\n" },
  {"<hg1>", FTCL_ARGV_STRING, NULL, NULL, 
   "First HG"},
  {"<operation>", FTCL_ARGV_STRING, NULL, NULL, 
   "Operation hg1 does on hg2"},
  {"<hg2>", FTCL_ARGV_STRING, NULL, NULL, 
   "Second HG"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclHgOper(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_hg1 = NULL;
  char *a_operation = NULL;
  char *a_hg2 = NULL;
  HG *hg1=NULL, *hg2=NULL, *hgNew=NULL;
  char hgNewName[HANDLE_NAMELEN];

  g_hgOperTbl[1].dst = &a_hg1;
  g_hgOperTbl[2].dst = &a_operation;
  g_hgOperTbl[3].dst = &a_hg2;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_hgOperTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_hgOper)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

    if (shTclAddrGetFromName
	(interp, a_hg1, (void **) &hg1, "HG") != TCL_OK) {
      Tcl_SetResult(interp, "tclHgOper:  bad HG name", TCL_VOLATILE);
      
      return TCL_ERROR;
    }

    if (shTclAddrGetFromName
	(interp, a_hg2, (void **) &hg2, "HG") != TCL_OK) {
      Tcl_SetResult(interp, "tclHgOper:  bad HG name", TCL_VOLATILE);
      
      return TCL_ERROR;
    }
  if (  (strcmp(a_operation,"+") == 0) ||
        (strcmp(a_operation,"m") == 0) ||
        (strcmp(a_operation,"*") == 0) ||
        (strcmp(a_operation,"/") == 0)) {
    hgNew = shHgOper(hg1, a_operation, hg2); 
    if (shTclHandleNew(interp, hgNewName, "HG", hgNew) != TCL_OK ) {
      Tcl_SetResult(interp, "Can't bind to new hg handle",0);
      return(TCL_ERROR);
    }
    Tcl_SetResult(interp, hgNewName, TCL_VOLATILE);
    return TCL_OK;
  } else {
    Tcl_SetResult(interp, "tclHgOper: operation must be +, m, *, or /", TCL_VOLATILE);
    return TCL_ERROR;
  }
}

/****************************************************************************
**
** ROUTINE: tclHgDel
**
**<AUTO EXTRACT>
** TCL VERB: hgDel
**
**</AUTO>
*/
char g_hgDel[] = "hgDel";
ftclArgvInfo g_hgDelTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Delete a HG.\n" },
  {"<hg>", FTCL_ARGV_STRING, NULL, NULL, 
   "The HG to delete"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclHgDel(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_hg = NULL;
  HG *hg=NULL;

  g_hgDelTbl[1].dst = &a_hg;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_hgDelTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_hgDel)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName
      (interp, a_hg, (void **) &hg, "HG") != TCL_OK) {
    Tcl_SetResult(interp, "tclHgDel:  bad HG name", TCL_VOLATILE);     
    return TCL_ERROR;
  }
  shHgDel(hg);
  p_shTclHandleDel(interp, a_hg);
  Tcl_SetResult(interp,"",0);
  return(TCL_OK);
}
/****************************************************************************
 **
 ** ROUTINE: tclPgstateNew
 **
 **<AUTO EXTRACT>
 ** TCL VERB: pgstateNew
 **
 **</AUTO>
 */
char g_pgstateNew[] = "pgstateNew";
ftclArgvInfo g_pgstateNewTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
     "Return a handle to a new PGSTATE.\n" },
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgstateNew(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  char name[HANDLE_NAMELEN];
  int rstat;
  PGSTATE *pgstate;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgstateNewTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgstateNew)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  pgstate = shPgstateNew();
  if (shTclHandleNew(interp, name, "PGSTATE", pgstate) != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to new pgstate handle", TCL_VOLATILE);
    return(TCL_ERROR);
  }
  Tcl_SetResult(interp, name, TCL_VOLATILE);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgstateDefault
**
**<AUTO EXTRACT>
** TCL VERB: pgstateDefault
**
**</AUTO>
*/
char g_pgstateDefault[] = "pgstateDefault";
ftclArgvInfo g_pgstateDefaultTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Reset PGSTATE settings to the default.\n" },
  {"<pgstate>", FTCL_ARGV_STRING, NULL, NULL, 
   "The PGSTATE to reset to default"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgstateDefault(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_pgstate = NULL;
  PGSTATE *pgstate=NULL;

  g_pgstateDefaultTbl[1].dst = &a_pgstate;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgstateDefaultTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgstateDefault)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName
      (interp, a_pgstate, (void **) &pgstate, "PGSTATE") != TCL_OK) {      
    Tcl_SetResult(interp, "tclPgstateDefault:  bad PGSTATE name", TCL_VOLATILE);
    return(TCL_ERROR);
  }
  shPgstateDefault(pgstate);
 
  return(TCL_OK);
}
/****************************************************************************
**
** ROUTINE: tclPgstateSet
**
**<AUTO EXTRACT>
** TCL VERB: pgstateSet
**
**</AUTO>
*/
static ftclArgvInfo g_pgstateSetTbl[] = {
     {NULL,              FTCL_ARGV_HELP  , NULL, NULL,
	"Set values in PGSTATE\n"},
     {"<pgstate>", 	FTCL_ARGV_STRING, NULL, NULL, "The initial PGSTATE"},
     {"-device",   	FTCL_ARGV_STRING, NULL, NULL, "Plotting device"},
     {"-nxwindow",   	FTCL_ARGV_INT   , NULL, NULL, "Number of windows in x"},
     {"-nywindow",   	FTCL_ARGV_INT   , NULL, NULL, "Number of windows in y"},
     {"-xfract",   	FTCL_ARGV_DOUBLE, NULL, NULL, "fraction of space left for borders in x"},
     {"-yfract",   	FTCL_ARGV_DOUBLE, NULL, NULL, "fraction of space left for borders in y "},
     {"-ixwindow",   	FTCL_ARGV_INT   , NULL, NULL, "current window in x"},
     {"-iywindow",   	FTCL_ARGV_INT   , NULL, NULL, "current window in y"},
     {"-just",   	FTCL_ARGV_INT   , NULL, NULL, "parameter in cpgenv"},
     {"-axis",   	FTCL_ARGV_INT   , NULL, NULL, "parameter in cpgenv"},
     {"-xopt",   	FTCL_ARGV_STRING, NULL, NULL, "character string sent to pgbox for x"},
     {"-yopt",   	FTCL_ARGV_STRING, NULL, NULL, "character string sent to pgbox for y"},
     {"-xtick",   	FTCL_ARGV_DOUBLE, NULL, NULL, "also used in pgbox"},
     {"-ytick",   	FTCL_ARGV_DOUBLE, NULL, NULL, "also used in pgbox"},
     {"-nxsub",   	FTCL_ARGV_INT   , NULL, NULL, "also used in pgbox"},
     {"-nysub",   	FTCL_ARGV_INT   , NULL, NULL, "also used in pgbox"},
     {"-plotTitle",   	FTCL_ARGV_STRING, NULL, NULL, "Title of the plot"},
     {"-symb",   	FTCL_ARGV_INT   , NULL, NULL, "Symbol to use for plotting"},
     {"-isLine",   	FTCL_ARGV_INT   , NULL, NULL, "1 if points are connected by lines; 0 if just symbols "},
     {"-isTime",   	FTCL_ARGV_INT   , NULL, NULL, "1 to call PGTBOX to enable time labels"},
     {"-icMark",   	FTCL_ARGV_INT   , NULL, NULL, "color to make points"},
     {"-icLine",   	FTCL_ARGV_INT   , NULL, NULL, "color to draw lines connecting points"},
     {"-icBox",   	FTCL_ARGV_INT   , NULL, NULL, "color of the box"},
     {"-icLabel",   	FTCL_ARGV_INT   , NULL, NULL, "color of the labels"},
     {"-icError",   	FTCL_ARGV_INT   , NULL, NULL, "color of the error bars"},
     {"-lineWidth",   	FTCL_ARGV_INT   , NULL, NULL, "Width of line"},
     {"-lineStyle",   	FTCL_ARGV_INT   , NULL, NULL, "Style for line (1-5)"},
     {"-isNewplot",   	FTCL_ARGV_INT   , NULL, NULL, "1 to move to new plot; 0 to overplot on previous"},
     {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

char g_pgstateSet[] = "pgstateSet";

static int tclPgstateSet(ClientData clientData, Tcl_Interp *interp,
			 int argc, char **argv) 
{
  PGSTATE *pgstate=NULL;
  
  int rstat;
  char *a_pgstate = NULL;
  char *a_device = NULL;
  int a_nxwindow, a_nywindow;
  double a_xfract, a_yfract;
  int  a_ixwindow, a_iywindow;
  int a_just, a_axis;
  char* a_xopt = NULL, * a_yopt = NULL;
  double a_xtick, a_ytick;
  int a_nxsub, a_nysub;
  char* a_plotTitle = NULL;
  int a_symb, a_isLine, a_isTime, a_icMark, a_icLine, a_icBox;
  int a_icLabel, a_icError, a_lineWidth, a_lineStyle, a_isNewplot;
  
  int flags = FTCL_ARGV_NO_LEFTOVERS;

  g_pgstateSetTbl[1].dst = &a_pgstate;
  g_pgstateSetTbl[2].dst = &a_device;
  g_pgstateSetTbl[3].dst = &a_nxwindow;
  g_pgstateSetTbl[4].dst = &a_nywindow;
  g_pgstateSetTbl[5].dst = &a_xfract;
  g_pgstateSetTbl[6].dst = &a_yfract;
  g_pgstateSetTbl[7].dst = &a_ixwindow;
  g_pgstateSetTbl[8].dst = &a_iywindow;
  g_pgstateSetTbl[9].dst = &a_just;
  g_pgstateSetTbl[10].dst = &a_axis;
  g_pgstateSetTbl[11].dst = &a_xopt;
  g_pgstateSetTbl[12].dst = &a_yopt;
  g_pgstateSetTbl[13].dst = &a_xtick;
  g_pgstateSetTbl[14].dst = &a_ytick;
  g_pgstateSetTbl[15].dst = &a_nxsub;
  g_pgstateSetTbl[16].dst = &a_nysub;
  g_pgstateSetTbl[17].dst = &a_plotTitle;
  g_pgstateSetTbl[18].dst = &a_symb;
  g_pgstateSetTbl[19].dst = &a_isLine;
  g_pgstateSetTbl[20].dst = &a_isTime;
  g_pgstateSetTbl[21].dst = &a_icMark;
  g_pgstateSetTbl[22].dst = &a_icLine;
  g_pgstateSetTbl[23].dst = &a_icBox;
  g_pgstateSetTbl[24].dst = &a_icLabel;
  g_pgstateSetTbl[25].dst = &a_icError;
  g_pgstateSetTbl[26].dst = &a_lineWidth;
  g_pgstateSetTbl[27].dst = &a_lineStyle;
  g_pgstateSetTbl[28].dst = &a_isNewplot;
  
  a_nxwindow = INT_MAX;
  a_nywindow = INT_MAX;
  a_xfract = FLT_MAX;
  a_yfract = FLT_MAX;
  a_ixwindow = INT_MAX;
  a_iywindow = INT_MAX;
  a_just = INT_MAX;
  a_axis = INT_MAX;
  a_xtick = FLT_MAX;
  a_ytick = FLT_MAX;
  a_nxsub = INT_MAX;
  a_nysub = INT_MAX;
  a_symb = INT_MAX;
  a_isLine = INT_MAX;
  a_isTime = INT_MAX;
  a_icMark = INT_MAX;
  a_icLine = INT_MAX;
  a_icBox = INT_MAX;
  a_icLabel = INT_MAX;
  a_icError = INT_MAX;
  a_lineWidth = INT_MAX;
  a_lineStyle = INT_MAX;
  a_isNewplot = INT_MAX;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgstateSetTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgstateSet)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  if (shTclAddrGetFromName
      (interp, a_pgstate, (void **) &pgstate, "PGSTATE") != TCL_OK) {     
    Tcl_SetResult(interp, "Bad PGSTATE handle", TCL_VOLATILE);
    return(TCL_ERROR);
  }
  
  if (a_device!=NULL) {           	
    if ( strlen(a_device) > HG_DEVICE_NAMELEN) {
      Tcl_SetResult(interp, "Device name too long", TCL_VOLATILE);
      return(TCL_ERROR);
    }
    strncpy(pgstate->device, a_device, HG_DEVICE_NAMELEN);
    pgstate->device[HG_DEVICE_NAMELEN-1] = '\0';
  }
  if (isgoodint(a_nxwindow)) 	pgstate->nxwindow = a_nxwindow;
  if (isgoodint(a_nywindow)) 	pgstate->nywindow = a_nywindow;
  if (isgood(a_xfract))   	pgstate->xfract   = a_xfract;
  if (isgood(a_yfract))   	pgstate->yfract   = a_yfract;
  if (isgoodint(a_ixwindow)) 	pgstate->ixwindow = a_ixwindow;
  if (isgoodint(a_iywindow))	pgstate->iywindow = a_iywindow;
  if (isgoodint(a_just))     	pgstate->just     = a_just;
  if (isgoodint(a_axis))     	pgstate->axis     = a_axis;
  if (a_xopt!=NULL) {
    strncpy(pgstate->xopt, a_xopt, HG_OPT_LENGTH);
    pgstate->xopt[HG_OPT_LENGTH-1] = '\0';
  }
  if (a_yopt!=NULL) {
    strncpy(pgstate->yopt, a_yopt, HG_OPT_LENGTH);
    pgstate->yopt[HG_OPT_LENGTH-1] = '\0';
  }
  if (isgood(a_xtick))    	pgstate->xtick    = a_xtick;
  if (isgood(a_ytick))    	pgstate->ytick    = a_ytick;
  if (isgoodint(a_nxsub))    	pgstate->nxsub    = a_nxsub;
  if (isgoodint(a_nysub))    	pgstate->nysub    = a_nysub;
  if (a_plotTitle!=NULL) {
    strncpy(pgstate->plotTitle, a_plotTitle, HG_TITLE_LENGTH);
    pgstate->plotTitle[HG_TITLE_LENGTH-1] = '\0';
  }
  if (isgoodint(a_symb))       	pgstate->symb    = a_symb;
  if (isgoodint(a_isLine))    	pgstate->isLine    = a_isLine;
  if (isgoodint(a_isTime))    	pgstate->isTime    = a_isTime;
  if (isgoodint(a_icMark))    	pgstate->icMark    = a_icMark;
  if (isgoodint(a_icLine))    	pgstate->icLine    = a_icLine;
  if (isgoodint(a_icBox))    	pgstate->icBox    = a_icBox;
  if (isgoodint(a_icLabel))    	pgstate->icLabel    = a_icLabel;
  if (isgoodint(a_icError))    	pgstate->icError    = a_icError;
  if (isgoodint(a_lineWidth)) 	pgstate->lineWidth   = a_lineWidth;
  if (isgoodint(a_lineStyle)) 	pgstate->lineStyle   = a_lineStyle;
  if (isgoodint(a_isNewplot)) 	pgstate->isNewplot   = a_isNewplot;
  return(TCL_OK);
}
/****************************************************************************
**
** ROUTINE: tclPgstatePrint
**
**<AUTO EXTRACT>
** TCL VERB: pgstatePrint
**
**</AUTO>
*/
char g_pgstatePrint[] = "pgstatePrint";
ftclArgvInfo g_pgstatePrintTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Print values in PGSTATE.\n" },
  {"<pgstate>", FTCL_ARGV_STRING, NULL, NULL, 
   "The PGSTATE to print"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgstatePrint(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_pgstate = NULL;
  PGSTATE *pgstate=NULL;

  g_pgstatePrintTbl[1].dst = &a_pgstate;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgstatePrintTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgstatePrint)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName
      (interp, a_pgstate, (void **) &pgstate, "PGSTATE") != TCL_OK) {    
    Tcl_SetResult(interp, "tclPgstatePrint:  bad PGSTATE name", TCL_VOLATILE);
    return(TCL_ERROR);
  }

  shPgstatePrint(pgstate);
  return(TCL_OK);
}


/****************************************************************************
**
** ROUTINE: tclPgstateNextWindow
**
**<AUTO EXTRACT>
** TCL VERB: pgstateNextWindow
**
**</AUTO>
*/
char g_pgstateNextWindow[] = "pgstateNextWindow";
ftclArgvInfo g_pgstateNextWindowTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Go to the next window in PGSTATE.\n" },
  {"<pgstate>", FTCL_ARGV_STRING, NULL, NULL, 
   "The PGSTATE to use"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgstateNextWindow(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_pgstate = NULL;
  PGSTATE *pgstate=NULL;

  g_pgstateNextWindowTbl[1].dst = &a_pgstate;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgstateNextWindowTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgstateNextWindow)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName
      (interp, a_pgstate, (void **) &pgstate, "PGSTATE") != TCL_OK) {    
    Tcl_SetResult(interp, "tclPgstateNextWindow:  bad PGSTATE name", TCL_VOLATILE);
    return(TCL_ERROR);
  }

  shPgstateNextWindow(pgstate);
  return(TCL_OK);
}
/****************************************************************************
**
** ROUTINE: tclPgstateDel
**
**<AUTO EXTRACT>
** TCL VERB: pgstateDel
**
**</AUTO>
*/
char g_pgstateDel[] = "pgstateDel";
ftclArgvInfo g_pgstateDelTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Delete a PGSTATE.\n" },
  {"<pgstate>", FTCL_ARGV_STRING, NULL, NULL, 
   "The PGSTATE to delete"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgstateDel(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_pgstate = NULL;
  PGSTATE *pgstate=NULL;

  g_pgstateDelTbl[1].dst = &a_pgstate;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgstateDelTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgstateDel)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName
      (interp, a_pgstate, (void **) &pgstate, "PGSTATE") != TCL_OK) {
    Tcl_SetResult(interp, "tclPgstateDel:  bad PGSTATE name", TCL_VOLATILE);     
    return TCL_ERROR;
  }
  shPgstateDel(pgstate);
  p_shTclHandleDel(interp, a_pgstate);
  Tcl_SetResult(interp,"",0);
  return(TCL_OK);
}


/****************************************************************************
**
** ROUTINE: tclPgstateOpen
**
**<AUTO EXTRACT>
** TCL VERB: pgstateOpen
**
**</AUTO>
*/
char g_pgstateOpen[] = "pgstateOpen";
ftclArgvInfo g_pgstateOpenTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Open a PGSTATE.\n" },
  {"<pgstate>", FTCL_ARGV_STRING, NULL, NULL, 
   "The PGSTATE to open"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgstateOpen(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_pgstate = NULL;
  PGSTATE *pgstate=NULL;

  g_pgstateOpenTbl[1].dst = &a_pgstate;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgstateOpenTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgstateOpen)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName
      (interp, a_pgstate, (void **) &pgstate, "PGSTATE") != TCL_OK) {
    Tcl_SetResult(interp, "tclPgstateOpen:  bad PGSTATE name", TCL_VOLATILE);     
    return TCL_ERROR;
  }
  if ( shPgstateOpen(pgstate) != SH_SUCCESS) {
    Tcl_SetResult(interp, "tclPgstateOpen: Can't open plotting device", TCL_VOLATILE);
    return TCL_ERROR;
  }
  Tcl_SetResult(interp, a_pgstate, TCL_VOLATILE);
  return(TCL_OK);
}
/****************************************************************************
**
** ROUTINE: tclPgstateTitle
**
**<AUTO EXTRACT>
** TCL VERB: pgstateTitle
**
**</AUTO>
*/
char g_pgstateTitle[] = "pgstateTitle";
ftclArgvInfo g_pgstateTitleTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Put the title on the plot.\n" },
  {"<pgstate>", FTCL_ARGV_STRING, NULL, NULL, 
   "The PGSTATE to title"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgstateTitle(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_pgstate = NULL;
  PGSTATE *pgstate=NULL;

  g_pgstateTitleTbl[1].dst = &a_pgstate;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgstateTitleTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgstateTitle)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName
      (interp, a_pgstate, (void **) &pgstate, "PGSTATE") != TCL_OK) {
    Tcl_SetResult(interp, "tclPgstateTitle:  bad PGSTATE name", TCL_VOLATILE);     
    return TCL_ERROR;
  }
  shPgstateTitle(pgstate);
  return(TCL_OK);
}
/****************************************************************************
**
** ROUTINE: tclHgPlot
**
**<AUTO EXTRACT>
** TCL VERB: hgPlot
**
**</AUTO>
*/
char g_hgPlot[] = "hgPlot";
ftclArgvInfo g_hgPlotTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Plot a HG in a PGSTATE.\n" },
  {"<pgstate>", FTCL_ARGV_STRING, NULL, NULL, 
   "The PGSTATE"},
  {"<hg>", FTCL_ARGV_STRING, NULL, NULL, 
   "The HG to plot"},
  {"-xmin",  	FTCL_ARGV_DOUBLE,        NULL, NULL,
   "Minimum in the X direction"},
  {"-xmax", 	FTCL_ARGV_DOUBLE,        NULL, NULL,
   "Maximum in the X direction"},
  {"-ymin",  	FTCL_ARGV_DOUBLE,        NULL, NULL,
   "Minimum in the Y direction"},
  {"-ymax", 	FTCL_ARGV_DOUBLE,        NULL, NULL,
   "Maximum in the Y direction"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclHgPlot(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_pgstate = NULL;
  char *a_hg = NULL;
  static double xmin, xmax, ymin, ymax;
  int xyOptMask = 0;
  PGSTATE *pgstate=NULL;
  HG *hg=NULL;

  g_hgPlotTbl[1].dst = &a_pgstate;
  g_hgPlotTbl[2].dst = &a_hg;
  g_hgPlotTbl[3].dst = &xmin;
  g_hgPlotTbl[4].dst = &xmax;
  g_hgPlotTbl[5].dst = &ymin;
  g_hgPlotTbl[6].dst = &ymax;

  /* Set these four values to !igood values so we can see what got set */
  xmin = FLT_MAX;
  xmax = FLT_MAX;
  ymin = FLT_MAX;
  ymax = FLT_MAX;


  if ((rstat = shTclParseArgv(interp, &argc, argv, g_hgPlotTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_hgPlot)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  
  if (shTclAddrGetFromName
      (interp, a_pgstate, (void **) &pgstate, "PGSTATE") != TCL_OK) {
    
    Tcl_SetResult(interp, "Bad PGSTATE handle", TCL_VOLATILE);
    return(TCL_ERROR);
  }
  if (shTclAddrGetFromName
      (interp, a_hg, (void **) &hg, "HG") != TCL_OK) {
    
    Tcl_SetResult(interp, "Bad HG handle", TCL_VOLATILE);
    return(TCL_ERROR);
  }
    /* See which of the minimum and maximum options have been user supplied */
  if (isgood(xmin))
    xyOptMask = xyOptMask | XMINOPT;
  if (isgood(xmax))
    xyOptMask = xyOptMask | XMAXOPT;
  if (isgood(ymin))
    xyOptMask = xyOptMask | YMINOPT;
  if (isgood(ymax))
    xyOptMask = xyOptMask | YMAXOPT;


 if(hg==NULL || hg->contents == NULL || pgstate == NULL) {
    Tcl_SetResult(interp, "hgPlot: Illegal argument list", TCL_VOLATILE);
    return(TCL_ERROR);
  }

  shHgPlot(pgstate, hg, xmin, xmax, ymin, ymax, xyOptMask);
  
  return(TCL_OK);
}
/****************************************************************************
**
** ROUTINE: tclPgstateClose
**
**<AUTO EXTRACT>
** TCL VERB: pgstateClose
**
**</AUTO>
*/
char g_pgstateClose[] = "pgstateClose";
ftclArgvInfo g_pgstateCloseTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Close a PGSTATE.\n" },
  {"<pgstate>", FTCL_ARGV_STRING, NULL, NULL, 
   "The PGSTATE to close"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgstateClose(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_pgstate = NULL;
  PGSTATE *pgstate=NULL;

  g_pgstateCloseTbl[1].dst = &a_pgstate;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgstateCloseTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgstateClose)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName
      (interp, a_pgstate, (void **) &pgstate, "PGSTATE") != TCL_OK) {
    Tcl_SetResult(interp, "tclPgstateClose:  bad PGSTATE name", TCL_VOLATILE);     
    return TCL_ERROR;
  }
  shPgstateClose(pgstate);
  return(TCL_OK);
}
/****************************************************************************
**
** ROUTINE: tclHgReg
**
**<AUTO EXTRACT>
** TCL VERB: hgReg
**
**</AUTO>
*/
char g_hgReg[] = "hgReg";
ftclArgvInfo g_hgRegTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Fill the histogram hg with the pixel values of region reg.\n" },
  {"<hg>", FTCL_ARGV_STRING, NULL, NULL, 
   "The HG to fill"},
  {"<reg>", FTCL_ARGV_STRING, NULL, NULL, 
   "The region to take values from"},
  {"-maskBits", FTCL_ARGV_INT, NULL, NULL, 
   "Exclude pixels with matches to this mask"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclHgReg(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_hg = NULL;
  char *a_reg = NULL;
  REGION *region=NULL;
  HG *hg=NULL;
  int a_maskBits = 0;

  g_hgRegTbl[1].dst = &a_hg;
  g_hgRegTbl[2].dst = &a_reg;
  g_hgRegTbl[3].dst = &a_maskBits;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_hgRegTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_hgReg)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

    if (shTclAddrGetFromName
	(interp, a_reg, (void **) &region, "REGION") != TCL_OK) {
      
      Tcl_SetResult(interp, "Bad REGION handle", TCL_VOLATILE);
      return(TCL_ERROR);
    }
    if (shTclAddrGetFromName
	(interp, a_hg, (void **) &hg, "HG") != TCL_OK) {
      
      Tcl_SetResult(interp, "Bad HG handle", TCL_VOLATILE);
      return(TCL_ERROR);
    }

  if ( shHgReg(hg, region, a_maskBits) == SH_SUCCESS) {
    Tcl_SetResult(interp, a_hg, TCL_VOLATILE);
    return(TCL_OK);
  } else {
    shTclInterpAppendWithErrStack(interp);
    return TCL_ERROR;
  }
}


/****************************************************************************
 **
 ** ROUTINE: tclPtNew
 **
 **<AUTO EXTRACT>
 ** TCL VERB: ptNew
 **
 **</AUTO>
 */
char g_ptNew[] = "ptNew";
ftclArgvInfo g_ptNewTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
     "Make a new PT\n" },
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPtNew(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  char name[HANDLE_NAMELEN];
  int rstat;
  PT *pt;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_ptNewTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_ptNew)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  pt = shPtNew();
  if (shTclHandleNew(interp, name, "PT", pt) != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to new pt handle", TCL_VOLATILE);
    return(TCL_ERROR);
  }
  Tcl_SetResult(interp, name, TCL_VOLATILE);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPtDefine
**
**<AUTO EXTRACT>
** TCL VERB: ptDefine
**
**</AUTO>
*/
char g_ptDefine[] = "ptDefine";
ftclArgvInfo g_ptDefineTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Define an existing PT, at row, col and radius.\n" },
  {"<pt>", FTCL_ARGV_STRING, NULL, NULL, 
   "The PT to define"},
  {"<row>", FTCL_ARGV_DOUBLE,  NULL, NULL, 
   "The row value"},
  {"<col>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "The column value"},
  {"<radius>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "The radius value"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPtDefine(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  PT *pt=NULL;
  int rstat;
  char *a_pt = NULL; 
  double a_row, a_col, a_radius;

  g_ptDefineTbl[1].dst = &a_pt;
  g_ptDefineTbl[2].dst = &a_row;
  g_ptDefineTbl[3].dst = &a_col;
  g_ptDefineTbl[4].dst = &a_radius;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_ptDefineTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_ptDefine)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  if (shTclAddrGetFromName
      (interp, a_pt, (void **) &pt, "PT") != TCL_OK) {
    Tcl_SetResult(interp, "tclPtDefine:  bad PT name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  shPtDefine(pt, (float)a_row, (float)a_col, (float)a_radius);
  Tcl_SetResult(interp, a_pt, TCL_VOLATILE);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPtDel
**
**<AUTO EXTRACT>
** TCL VERB: ptDel
**
**</AUTO>
*/
char g_ptDel[] = "ptDel";
ftclArgvInfo g_ptDelTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Delete a PT.\n" },
  {"<pt>", FTCL_ARGV_STRING, NULL, NULL, 
   "The PT to delete"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPtDel(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_pt = NULL;
  PT *pt=NULL;

  g_ptDelTbl[1].dst = &a_pt;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_ptDelTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_ptDel)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName
      (interp, a_pt, (void **) &pt, "PT") != TCL_OK) {
    Tcl_SetResult(interp, "tclPtDel:  bad PT name", TCL_VOLATILE);     
    return TCL_ERROR;
  }
  shPtDel(pt);
  p_shTclHandleDel(interp, a_pt);
  Tcl_SetResult(interp,"",0);
  return(TCL_OK);
}


/****************************************************************************
**
** ROUTINE: tclChainFromPlot
**
**<AUTO EXTRACT>
** TCL VERB: chainFromPlot
**
**</AUTO>
*/
static ftclArgvInfo chainFromPlotArgTable[] = {
      {NULL,              FTCL_ARGV_HELP,         NULL, NULL,
	 "Make a chain of LINKS based on selection in the afPlot window\n"},
      {"<pgstate>",       FTCL_ARGV_STRING,       NULL, NULL, NULL},
      {"<inputChain>",     FTCL_ARGV_STRING,       NULL, NULL, NULL},
      {"-vMask", 	  FTCL_ARGV_STRING,       NULL, NULL, NULL},
      {NULL,    FTCL_ARGV_END,          NULL, NULL, NULL}
 };

char g_chainFromPlot[] = "chainFromPlot";

static int 
tclChainFromPlot
  (ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstatus;
  CHAIN *outputChain=NULL, *inputChain=NULL;
  PGSTATE *pgstate=NULL;
  VECTOR *vMask=NULL;
  char *pgstateNamePtr=NULL;
  char  *inputChainNamePtr=NULL;
  char outputChainName[HANDLE_NAMELEN];
  char *vMaskNamePtr=NULL;
  int flags = FTCL_ARGV_NO_LEFTOVERS;
  
  chainFromPlotArgTable[1].dst = &pgstateNamePtr;
  chainFromPlotArgTable[2].dst = &inputChainNamePtr;
  chainFromPlotArgTable[3].dst = &vMaskNamePtr;

  if ((rstatus = shTclParseArgv(interp, &argc, argv, chainFromPlotArgTable ,
				flags, g_chainFromPlot))
      ==FTCL_ARGV_SUCCESS) {
    
    if (shTclAddrGetFromName
	(interp, pgstateNamePtr, (void **) &pgstate, "PGSTATE") != TCL_OK) {
      
      Tcl_SetResult(interp, shTclGetUsage(interp, chainFromPlotArgTable, flags,
					  g_chainFromPlot), TCL_VOLATILE);
      return(TCL_ERROR);
    }
    
    if (shTclAddrGetFromName
	(interp, inputChainNamePtr, (void **) &inputChain, "CHAIN") != TCL_OK) {
      
      Tcl_SetResult(interp, shTclGetUsage(interp, chainFromPlotArgTable, flags,
					  g_chainFromPlot), TCL_VOLATILE);
      return TCL_ERROR;
    }
    
    if (vMaskNamePtr == NULL || strcmp(vMaskNamePtr,"none") == 0) {
      vMask = NULL;
    } else {
      if (shTclAddrGetFromName
	  (interp, vMaskNamePtr, (void **) &vMask, "VECTOR") != TCL_OK) {
	Tcl_SetResult(interp, "tclChainFromPlot:  bad vMask name", TCL_VOLATILE);
	
	return TCL_ERROR;
      }
    }
  } else {
    return (rstatus);
  }
  if (shTclChainHandleNew
      (interp, outputChainName, "GENERIC", &outputChain) != TCL_OK) {
    Tcl_SetResult(interp, "can not do shTclChainHandleNew ", TCL_VOLATILE);
    return TCL_ERROR;
  }
/* do the actual work here */
  shChainFromPlot(pgstate, outputChain, inputChain, vMask);

  Tcl_SetResult(interp, outputChainName, TCL_VOLATILE);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclVIndexFromPlot
**
**<AUTO EXTRACT>
** TCL VERB: vIndexFromPlot
**
**</AUTO>
*/
static ftclArgvInfo vIndexFromPlotArgTable[] = {
      {NULL,              FTCL_ARGV_HELP,         NULL, NULL,
	 "Make a new vector of the indices of vX and vY based on selection in the vPlot window\n"},
      {"<pgstate>",       FTCL_ARGV_STRING,       NULL, NULL, 
       "where to plot"},
      {"<vX>",     FTCL_ARGV_STRING,       NULL, NULL, 
       "x variable vector"},
      {"<vY>",     FTCL_ARGV_STRING,       NULL, NULL, 
       "y variable vector"},
      {"-vMask", 	  FTCL_ARGV_STRING,       NULL, NULL,
       "what points to use"},
      {NULL,    FTCL_ARGV_END,          NULL, NULL, NULL}
 };

char g_vIndexFromPlot[] = "vIndexFromPlot";

static int 
tclVIndexFromPlot
  (ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstatus;
  VECTOR *vIndex=NULL, *vX=NULL, *vY=NULL;
  PGSTATE *pgstate=NULL;
  VECTOR *vMask=NULL;
  char *pgstateNamePtr=NULL;
  char *vXNamePtr=NULL;
  char *vYNamePtr=NULL;
  char vIndexName[HANDLE_NAMELEN];
  char *vMaskNamePtr=NULL;
  int flags = FTCL_ARGV_NO_LEFTOVERS;
  
  vIndexFromPlotArgTable[1].dst = &pgstateNamePtr;
  vIndexFromPlotArgTable[2].dst = &vXNamePtr;
  vIndexFromPlotArgTable[3].dst = &vYNamePtr;
  vIndexFromPlotArgTable[4].dst = &vMaskNamePtr;
  
  if ((rstatus = shTclParseArgv(interp, &argc, argv, vIndexFromPlotArgTable ,
				flags, g_vIndexFromPlot))
      !=FTCL_ARGV_SUCCESS) {
    return (rstatus);
  }
  
  if (shTclAddrGetFromName
      (interp, pgstateNamePtr, (void **) &pgstate, "PGSTATE") != TCL_OK) { 
    Tcl_SetResult(interp, shTclGetUsage(interp, vIndexFromPlotArgTable, flags,
					g_vIndexFromPlot), TCL_VOLATILE);
    return(TCL_ERROR);
  }
  
  if (shTclAddrGetFromName
      (interp, vXNamePtr, (void **) &vX, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, shTclGetUsage(interp, vIndexFromPlotArgTable, flags,
					g_vIndexFromPlot), TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (shTclAddrGetFromName
      (interp, vYNamePtr, (void **) &vY, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, shTclGetUsage(interp, vIndexFromPlotArgTable, flags,
					g_vIndexFromPlot), TCL_VOLATILE);
    return TCL_ERROR;
  }
  
  if (vMaskNamePtr == NULL || strcmp(vMaskNamePtr,"none") == 0) {
    vMask = NULL;
  } else {
    if (shTclAddrGetFromName
	(interp, vMaskNamePtr, (void **) &vMask, "VECTOR") != TCL_OK) {
      Tcl_SetResult(interp,
		    "tclVectorFromPlot:  bad vMask name", TCL_VOLATILE);   
      return TCL_ERROR;
    }
  }

  vIndex = shVectorNew(vX->dimen);
  
  /* do the actual work here */
  if((rstatus = shVIndexFromPlot(pgstate, vIndex, vX, vY, vMask))
     !=SH_SUCCESS) {
    shVectorDel(vIndex);
    return(rstatus);
  }

  if (shTclHandleNew(interp, vIndexName, "VECTOR", vIndex) != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to new vector handle", TCL_VOLATILE);
    return(TCL_ERROR);
  }

  Tcl_SetResult(interp, vIndexName, TCL_VOLATILE);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclVIndexFromHg
**
**<AUTO EXTRACT>
** TCL VERB: vIndexFromHg
**
**</AUTO>
*/
static ftclArgvInfo vIndexFromHgArgTable[] = {
      {NULL,              FTCL_ARGV_HELP,         NULL, NULL,
	 "Make a new vector of the indices of vX based on selection in the vPlot window\n"},
      {"<pgstate>",       FTCL_ARGV_STRING,       NULL, NULL, 
       "where to plot"},
      {"<vX>",     FTCL_ARGV_STRING,       NULL, NULL, 
       "x variable vector"},
      {"-vMask", 	  FTCL_ARGV_STRING,       NULL, NULL,
       "what points of vX to consider"},
      {NULL,    FTCL_ARGV_END,          NULL, NULL, NULL}
 };

char g_vIndexFromHg[] = "vIndexFromHg";

static int 
tclVIndexFromHg
  (ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstatus;
  VECTOR *vIndex=NULL, *vX=NULL, *vY=NULL;
  PGSTATE *pgstate=NULL;
  VECTOR *vMask=NULL;
  char *pgstateNamePtr=NULL;
  char *vXNamePtr=NULL;
  char vIndexName[HANDLE_NAMELEN];
  char *vMaskNamePtr=NULL;
  int flags = FTCL_ARGV_NO_LEFTOVERS;
  
  vIndexFromHgArgTable[1].dst = &pgstateNamePtr;
  vIndexFromHgArgTable[2].dst = &vXNamePtr;
  vIndexFromHgArgTable[3].dst = &vMaskNamePtr;
  
  if ((rstatus = shTclParseArgv(interp, &argc, argv, vIndexFromHgArgTable ,
				flags, g_vIndexFromHg))
      !=FTCL_ARGV_SUCCESS) {
    return (rstatus);
  }
  
  if (shTclAddrGetFromName
      (interp, pgstateNamePtr, (void **) &pgstate, "PGSTATE") != TCL_OK) { 
    Tcl_SetResult(interp, shTclGetUsage(interp, vIndexFromHgArgTable, flags,
					g_vIndexFromHg), TCL_VOLATILE);
    return(TCL_ERROR);
  }
  
  if (shTclAddrGetFromName
      (interp, vXNamePtr, (void **) &vX, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, shTclGetUsage(interp, vIndexFromHgArgTable, flags,
					g_vIndexFromHg), TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (vMaskNamePtr == NULL || strcmp(vMaskNamePtr,"none") == 0) {
    vMask = NULL;
  } else {
    if (shTclAddrGetFromName
	(interp, vMaskNamePtr, (void **) &vMask, "VECTOR") != TCL_OK) {
      Tcl_SetResult(interp,
		    "tclVectorFromHg:  bad vMask name", TCL_VOLATILE);   
      return TCL_ERROR;
    }
  }

  vIndex = shVectorNew(vX->dimen);
  
  /* do the actual work here */
  if((rstatus = shVIndexFromHg(pgstate, vIndex, vX, vMask))
     !=SH_SUCCESS) {
    shVectorDel(vIndex);
    return(rstatus);
  }

  if (shTclHandleNew(interp, vIndexName, "VECTOR", vIndex) != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to new vector handle", TCL_VOLATILE);
    return(TCL_ERROR);
  }

  Tcl_SetResult(interp, vIndexName, TCL_VOLATILE);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclChainToSao
**
**<AUTO EXTRACT>
** TCL VERB: chainToSao
**
**</AUTO>
*/
static ftclArgvInfo chainToSaoArgTable[] = {
      {NULL,            FTCL_ARGV_HELP,         NULL, NULL,
	 "Mark the members of chain on the SAO window.  -radius is also a name.\n"},
      {"<chain>",        FTCL_ARGV_STRING,       NULL, NULL, NULL},
      {"-rowName",      FTCL_ARGV_STRING,       NULL, NULL, NULL},
      {"-colName", 	FTCL_ARGV_STRING,       NULL, NULL, NULL},
      {"-radius", 	FTCL_ARGV_STRING,       NULL, NULL, NULL},
      {NULL,    FTCL_ARGV_END,          NULL, NULL, NULL}
 };

char g_chainToSao[] = "chainToSao";

static int 
tclChainToSao
  (ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstatus;
  CHAIN *tchain=NULL;
  char *tchainNamePtr=NULL;
  char *rowNamePtr = "row", *colNamePtr = "col" , *radiusNamePtr= "10.0";
  int flags = FTCL_ARGV_NO_LEFTOVERS;

  chainToSaoArgTable[1].dst = &tchainNamePtr;
  chainToSaoArgTable[2].dst = &rowNamePtr;
  chainToSaoArgTable[3].dst = &colNamePtr;
  chainToSaoArgTable[4].dst = &radiusNamePtr;
  
  
  if ((rstatus = shTclParseArgv(interp, &argc, argv, chainToSaoArgTable, flags,
				g_chainToSao))
      ==FTCL_ARGV_SUCCESS) {
    
    if (shTclAddrGetFromName
	(interp, tchainNamePtr, (void **) &tchain, "CHAIN") != TCL_OK) {
      
      Tcl_SetResult(interp, shTclGetUsage(interp, chainToSaoArgTable, flags,
					  g_chainToSao), TCL_VOLATILE);
      return(TCL_ERROR);
    }
    
  } else {
    return (rstatus);
  }
/* do the actual work here */
  shChainToSao(clientData, tchain, rowNamePtr, colNamePtr, radiusNamePtr);
  
  Tcl_SetResult(interp, tchainNamePtr, TCL_VOLATILE);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclSeedSet
**
**<AUTO EXTRACT>
** TCL VERB: seedSet
**
**</AUTO>
*/
static ftclArgvInfo seedSetArgTable[] = {
      {NULL,            FTCL_ARGV_HELP,         NULL, NULL,
	 "Seed the random number generator rand with a call to srand\n"},
      {"<seed>",        FTCL_ARGV_INT,          NULL, NULL, NULL},
      {NULL,    	FTCL_ARGV_END,          NULL, NULL, NULL}
 };

char g_seedSet[] = "seedSet";

static int
tclSeedSet(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)  
{
  int rstatus;
  int seed;
  int flags = FTCL_ARGV_NO_LEFTOVERS;

  seedSetArgTable[1].dst = &seed;
  

/* parse the command */
  
  if ((rstatus = shTclParseArgv(interp, &argc, argv, seedSetArgTable, flags,
				g_seedSet))
      !=FTCL_ARGV_SUCCESS){
    return rstatus;
  }
  srand(seed);
/* Send back the answer */
  Tcl_SetResult(interp, "RANDOM SEEDED", TCL_VOLATILE);
  
  return TCL_OK;
}



/****************************************************************************
**
** ROUTINE: tclRandGet
**
**<AUTO EXTRACT>
** TCL VERB: randGet
**
**</AUTO>
*/
char g_randGet[] = "randGet";
ftclArgvInfo g_randGetTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Return a random number from a uniform distribution in (min,max]\n" },
  {"-min", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "minimum value.  Random number is min"},
  {"-max", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "maximum value.  Random number is max"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclRandGet(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_min = 0.0;
  double a_max = 1.0;

  double num;
  char answer[40];

  g_randGetTbl[1].dst = &a_min;
  g_randGetTbl[2].dst = &a_max;


  if ((rstat = shTclParseArgv(interp, &argc, argv, g_randGetTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_randGet)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (a_min >= a_max) {
    Tcl_SetResult(interp, 
		  "tclRandGet:  min should be greater than max", TCL_VOLATILE);
    
    return TCL_ERROR;
  }

  num = ((double)rand()+1.0) / ((double)RAND_MAX+1.0); /* random in (0,1] */
  sprintf(answer, "%f", a_min + num*(a_max-a_min));
/* Send back the answer */
  Tcl_SetResult(interp, answer, TCL_VOLATILE);
  
  return TCL_OK;
}
/****************************************************************************
**
** ROUTINE: tclRandNormalGet
**
**<AUTO EXTRACT>
** TCL VERB: randNormalGet
**
**</AUTO>
*/
char g_randNormalGet[] = "randNormalGet";
ftclArgvInfo g_randNormalGetTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Return a random number from a normal distribution in mean, sig\n" },
  {"-mean", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "mean value."},
  {"-sigma", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "sigma of the distribution"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclRandNormalGet(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_mean = 0.0;
  double a_sigma = 1.0;

  char answer[40];
  double r1, r2, delta;
  g_randNormalGetTbl[1].dst = &a_mean;
  g_randNormalGetTbl[2].dst = &a_sigma;


  if ((rstat = shTclParseArgv(interp, &argc, argv, g_randNormalGetTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_randNormalGet)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  r1 = (double) (rand()) / (double) (RAND_MAX);
  /* r2 is not allowed to be zero */
  r2=0;
  do {
    r2 = (double) (rand()) / (double) (RAND_MAX);
  } while (r2 == 0);
  delta = sin(3.14159*2.0*r1) * sqrt(-2.0*log(r2));
  sprintf(answer, "%f", a_mean + delta*a_sigma);
/* Send back the answer */
  Tcl_SetResult(interp, answer, TCL_VOLATILE);
  
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclRegFluctuateAsSqrt
**
**<AUTO EXTRACT>
** TCL VERB: regFluctuateAsSqrt
**
**</AUTO>
*/
static ftclArgvInfo regFluctuateAsSqrtArgTable[] = {
      {NULL,                FTCL_ARGV_HELP,         NULL, NULL, 
	 "Add noise to the region\n"},
      {"<region>",          FTCL_ARGV_STRING,       NULL, NULL, NULL},
      {"-gain",             FTCL_ARGV_DOUBLE,       NULL, NULL, NULL},
      {NULL,                FTCL_ARGV_END,          NULL, NULL, NULL}
 };

char g_regFluctuateAsSqrt[] = "regFluctuateAsSqrt";

static int
tclRegFluctuateAsSqrt(ClientData clientData, Tcl_Interp *interp,
			int argc, char **argv)  
{
				/* INPUT */
  char *regNamePtr=NULL;	/* region name */
  REGION *regPtr;		/* pter to region */
  double gain=1.0;			/* gain in electrons per ADU */
				/* INTERNAL */

  int rstatus;
  int flags = FTCL_ARGV_NO_LEFTOVERS;

  regFluctuateAsSqrtArgTable[1].dst = &regNamePtr;
  regFluctuateAsSqrtArgTable[2].dst = &gain;

/* parse the command */
  
  if ((rstatus = shTclParseArgv(interp, &argc, argv,
				regFluctuateAsSqrtArgTable , flags,
				g_regFluctuateAsSqrt))
      ==FTCL_ARGV_SUCCESS){
    
    if (shTclRegAddrGetFromName(interp, regNamePtr, &regPtr) != TCL_OK) {
      Tcl_SetResult(interp, "countsFind: bad region name", TCL_VOLATILE);
      
      return TCL_ERROR;
    }
    
  } else {
    return rstatus;
  }

/* now do that actuacl all */
  shRegFluctuateAsSqrt(regPtr, gain);

/* Send back the answer */
  Tcl_SetResult(interp, regNamePtr, TCL_VOLATILE);
  
  return TCL_OK;
}


/**********************************************************************/
/* Declare new tcl verbs for HG functions*/
void shTclHgDeclare(Tcl_Interp *interp, CMD_HANDLE *cmdHandlePtr) {

  int flags = FTCL_ARGV_NO_LEFTOVERS;

  shTclDeclare(interp, g_hgNew, 
               (Tcl_CmdProc *) tclHgNew,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_hgNewTbl, flags, g_hgNew),
               shTclGetUsage(interp, g_hgNewTbl, flags, g_hgNew));
  shTclDeclare(interp, g_hgDefine, 
               (Tcl_CmdProc *) tclHgDefine,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_hgDefineTbl, flags, g_hgDefine),
               shTclGetUsage(interp, g_hgDefineTbl, flags, g_hgDefine));
   shTclDeclare(interp, g_hgClear, 
               (Tcl_CmdProc *) tclHgClear,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_hgClearTbl, flags, g_hgClear),
               shTclGetUsage(interp, g_hgClearTbl, flags, g_hgClear));
  shTclDeclare(interp, g_hgFill, 
               (Tcl_CmdProc *) tclHgFill,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_hgFillTbl, flags, g_hgFill),
               shTclGetUsage(interp, g_hgFillTbl, flags, g_hgFill));

  shTclDeclare(interp, g_hgPrint, 
               (Tcl_CmdProc *) tclHgPrint,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_hgPrintTbl, flags, g_hgPrint),
               shTclGetUsage(interp, g_hgPrintTbl, flags, g_hgPrint));

  shTclDeclare(interp, g_hgMean, 
               (Tcl_CmdProc *) tclHgMean,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_hgMeanTbl, flags, g_hgMean),
               shTclGetUsage(interp, g_hgMeanTbl, flags, g_hgMean));
  shTclDeclare(interp, g_hgSigma, 
               (Tcl_CmdProc *) tclHgSigma,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_hgSigmaTbl, flags, g_hgSigma),
               shTclGetUsage(interp, g_hgSigmaTbl, flags, g_hgSigma));

  shTclDeclare(interp, g_hgOper, 
               (Tcl_CmdProc *) tclHgOper,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_hgOperTbl, flags, g_hgOper),
               shTclGetUsage(interp, g_hgOperTbl, flags, g_hgOper));
 shTclDeclare(interp, g_hgDel, 
               (Tcl_CmdProc *) tclHgDel,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_hgDelTbl, flags, g_hgDel),
               shTclGetUsage(interp, g_hgDelTbl, flags, g_hgDel));

/* Declare new tcl verbs for PGSTATE functions*/
  shTclDeclare(interp, g_pgstateNew, 
               (Tcl_CmdProc *) tclPgstateNew,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_pgstateNewTbl, flags, g_pgstateNew),
               shTclGetUsage(interp, g_pgstateNewTbl, flags, g_pgstateNew));
  shTclDeclare(interp, g_pgstateDefault, 
               (Tcl_CmdProc *) tclPgstateDefault,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_pgstateDefaultTbl, flags, g_pgstateDefault),
               shTclGetUsage(interp, g_pgstateDefaultTbl, flags, g_pgstateDefault));
  shTclDeclare(interp, g_pgstateSet, 
               (Tcl_CmdProc *) tclPgstateSet,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_pgstateSetTbl, flags, g_pgstateSet),
               shTclGetUsage(interp, g_pgstateSetTbl, flags, g_pgstateSet));
  shTclDeclare(interp, g_pgstatePrint, 
               (Tcl_CmdProc *) tclPgstatePrint,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_pgstatePrintTbl, flags, g_pgstatePrint),
               shTclGetUsage(interp, g_pgstatePrintTbl, flags, g_pgstatePrint));

  shTclDeclare(interp, g_pgstateNextWindow, 
               (Tcl_CmdProc *) tclPgstateNextWindow,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_pgstateNextWindowTbl, flags, g_pgstateNextWindow),
               shTclGetUsage(interp, g_pgstateNextWindowTbl, flags, g_pgstateNextWindow));

 shTclDeclare(interp, g_pgstateDel, 
               (Tcl_CmdProc *) tclPgstateDel,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_pgstateDelTbl, flags, g_pgstateDel),
               shTclGetUsage(interp, g_pgstateDelTbl, flags, g_pgstateDel));

/* Declare new tcl verbs for PLOT functions */
 shTclDeclare(interp, g_pgstateOpen, 
               (Tcl_CmdProc *) tclPgstateOpen,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_pgstateOpenTbl, flags, g_pgstateOpen),
               shTclGetUsage(interp, g_pgstateOpenTbl, flags, g_pgstateOpen));
 shTclDeclare(interp, g_pgstateTitle, 
               (Tcl_CmdProc *) tclPgstateTitle,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_pgstateTitleTbl, flags, g_pgstateTitle),
               shTclGetUsage(interp, g_pgstateTitleTbl, flags, g_pgstateTitle));
  shTclDeclare(interp, g_hgPlot, 
               (Tcl_CmdProc *) tclHgPlot,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_hgPlotTbl, flags, g_hgPlot),
               shTclGetUsage(interp, g_hgPlotTbl, flags, g_hgPlot));

 shTclDeclare(interp, g_pgstateClose, 
               (Tcl_CmdProc *) tclPgstateClose,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_pgstateCloseTbl, flags, g_pgstateClose),
               shTclGetUsage(interp, g_pgstateCloseTbl, flags, g_pgstateClose));

  shTclDeclare(interp, g_hgReg, 
               (Tcl_CmdProc *) tclHgReg,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_hgRegTbl, flags, g_hgReg),
               shTclGetUsage(interp, g_hgRegTbl, flags, g_hgReg));

  shTclDeclare(interp, g_ptNew, 
               (Tcl_CmdProc *) tclPtNew,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_ptNewTbl, flags, g_ptNew),
               shTclGetUsage(interp, g_ptNewTbl, flags, g_ptNew));
  shTclDeclare(interp, g_ptDefine, 
               (Tcl_CmdProc *) tclPtDefine,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_ptDefineTbl, flags, g_ptDefine),
               shTclGetUsage(interp, g_ptDefineTbl, flags, g_ptDefine));
  shTclDeclare(interp, g_ptDel, 
               (Tcl_CmdProc *) tclPtDel,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_ptDelTbl, flags, g_ptDel),
               shTclGetUsage(interp, g_ptDelTbl, flags, g_ptDel));
  shTclDeclare(interp, g_chainFromPlot,
	       (Tcl_CmdProc *)tclChainFromPlot,
	       (ClientData) 0,
	       (Tcl_CmdDeleteProc *)NULL,
	       tclFacil,
	       shTclGetArgInfo(interp, chainFromPlotArgTable, flags,
			       g_chainFromPlot),
	       shTclGetUsage(interp, chainFromPlotArgTable, flags,
			     g_chainFromPlot));

  shTclDeclare(interp, g_chainToSao,
	       (Tcl_CmdProc *)tclChainToSao,
	       (ClientData) cmdHandlePtr,
	       (Tcl_CmdDeleteProc *)NULL,
	       tclFacil,
	       shTclGetArgInfo(interp, chainToSaoArgTable, flags,
			       g_chainToSao),
	       shTclGetUsage(interp, chainToSaoArgTable, flags,
			     g_chainToSao));
  shTclDeclare(interp, g_seedSet,
               (Tcl_CmdProc *)tclSeedSet,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               tclFacil,
	       shTclGetArgInfo(interp, seedSetArgTable, flags, g_seedSet),
	       shTclGetUsage(interp, seedSetArgTable, flags, g_seedSet));


  shTclDeclare(interp, g_randGet, 
               (Tcl_CmdProc *) tclRandGet,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_randGetTbl, flags, g_randGet),
               shTclGetUsage(interp, g_randGetTbl, flags, g_randGet));
  shTclDeclare(interp, g_randNormalGet, 
               (Tcl_CmdProc *) tclRandNormalGet,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_randNormalGetTbl, flags, g_randNormalGet),
               shTclGetUsage(interp, g_randNormalGetTbl, flags, g_randNormalGet));



  shTclDeclare(interp, g_regFluctuateAsSqrt,
               (Tcl_CmdProc *)tclRegFluctuateAsSqrt,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               tclFacil,
	       shTclGetArgInfo(interp, regFluctuateAsSqrtArgTable, flags,
			       g_regFluctuateAsSqrt),
	       shTclGetUsage(interp, regFluctuateAsSqrtArgTable, flags,
			     g_regFluctuateAsSqrt));
  shTclDeclare(interp, g_vIndexFromPlot,
	       (Tcl_CmdProc *)tclVIndexFromPlot,
	       (ClientData) 0,
	       (Tcl_CmdDeleteProc *)NULL,
	       tclFacil,
	       shTclGetArgInfo(interp, vIndexFromPlotArgTable, flags,
			       g_vIndexFromPlot),
	       shTclGetUsage(interp, vIndexFromPlotArgTable, flags,
			     g_vIndexFromPlot));

  shTclDeclare(interp, g_vIndexFromHg,
	       (Tcl_CmdProc *)tclVIndexFromHg,
	       (ClientData) 0,
	       (Tcl_CmdDeleteProc *)NULL,
	       tclFacil,
	       shTclGetArgInfo(interp, vIndexFromHgArgTable, flags,
			       g_vIndexFromHg),
	       shTclGetUsage(interp, vIndexFromHgArgTable, flags,
			     g_vIndexFromHg));

}
