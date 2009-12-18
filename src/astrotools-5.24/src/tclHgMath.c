/* <AUTO>
   FILE: tclHgMath
   ABSTRACT:
<HTML>
   Routines which operate on histograms (HG) and two dimensional
histograms (HG2).
</HTML>
   </AUTO>
*/
/* TCL interface to HG math routines */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"

#include "atHgMath.h"
#include "atTrans.h"

static char *tclFacil = "astrotools";    /* name of this set of code */

/****************************************************************************
**
** ROUTINE: tclHgGaussFit
**
**<AUTO EXTRACT>
** TCL VERB: hgGaussFit
**
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atHgMath.html#atHgGaussianFit>atHgGaussianFit</A> in atHgMath.c
** </HTML>
**
** Fit a gaussian to a histogram.  Returns a FUNC of type "gauss".
** maxit allows on to limit the number of iterations.
** iprt does nothing in this version, printing the status of the fit
** may be added in a future version.
**</AUTO>
*/
char g_hgGaussFit[] = "hgGaussFit";
ftclArgvInfo g_hgGaussFitTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Fit a gaussian to the histogram." },
  {"<hg>", FTCL_ARGV_STRING, NULL, NULL, 
   "the histogram to fit"},
  {"-maxit", FTCL_ARGV_INT, NULL, NULL, 
   "maximum number of iterations, Default: 100"},
  {"-iprt", FTCL_ARGV_INT, NULL, NULL, 
   "how much status to print, Default: 0, no printing"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclHgGaussFit(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_hg = NULL;
  HG *hg=NULL;
  int a_maxit = 100;
  int a_iprt = 0;
  FUNC *fitFunc=NULL;
  char smFitName[HANDLE_NAMELEN];

  g_hgGaussFitTbl[1].dst = &a_hg;
  g_hgGaussFitTbl[2].dst = &a_maxit;
  g_hgGaussFitTbl[3].dst = &a_iprt;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_hgGaussFitTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_hgGaussFit)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

/* do some error checking */
  if (shTclAddrGetFromName(interp, a_hg, (void **) &hg, "HG") != TCL_OK) {
    Tcl_SetResult(interp, "Unable to get HG address", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if ( hg == NULL ) {
    Tcl_SetResult(interp, "HG is NULL", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if ( hg->nbin <= 0 ) {
    Tcl_SetResult(interp, "HG has no bins", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if ( a_maxit < 0 ) {
    Tcl_SetResult(interp, "maxit must be positive", TCL_VOLATILE);
    return TCL_ERROR;
  }

/* make the call */
  fitFunc = atHgGaussianFit(hg, a_maxit, a_iprt);

/* make sure what came back is okay then return the answer */
  if (shTclHandleNew(interp, smFitName, "FUNC", fitFunc) != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to new FUNC handle", TCL_VOLATILE);
    return TCL_ERROR;
  }

  if ( fitFunc == NULL ) {
    Tcl_SetResult(interp, "atHgGaussianFit returned a NULL FUNC", TCL_VOLATILE);
    return TCL_ERROR; 
  }

  Tcl_SetResult(interp, smFitName, TCL_VOLATILE);
  return(TCL_OK);
}
/****************************************************************************
**
** ROUTINE: tclHgPolyFit
**
**<AUTO EXTRACT>
** TCL VERB: hgPolyFit
**
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atHgMath.html#atHgPolyFit>atHgPolyFit</A> in atHgMath.c
** </HTML>
**
** Fits a polynomial of arbitrary order to a histogram.  Returns a
** keyed list of the form "{ {FUNC h0} {COEF h1} }" where h0 is a
** FUNC of type "dpol", and h1 is a VECTOR with the coefficients of
** the Taylor Series expansion of the best fit function.  I.e., the
** VECTOR h1 contains the a_i in:  
** <PRE>a_0  +  a_1 * x  +  a_2 * x * x  +  a_3 * x * x * x + ... </PRE>
**</AUTO>
*/

char g_hgPolyFit[] = "hgPolyFit";
ftclArgvInfo g_hgPolyFitTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Fit a polynomial to the histogram." },
  {"<hg>", FTCL_ARGV_STRING, NULL, NULL, 
   "the histogram to fit"},
  {"<order>", FTCL_ARGV_INT, NULL, NULL, 
   "0 for constant, 1 for linear, 2 for quadratic, 3 for cubic, etc."},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclHgPolyFit(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_hg = NULL;
  int a_mode = 0;
  VECTOR *coef = NULL;
  HG *hg=NULL;
  FUNC *fitFunc=NULL;
  char fitName[ HANDLE_NAMELEN ], coefName[ HANDLE_NAMELEN ];
  char answer[100];

  g_hgPolyFitTbl[1].dst = &a_hg;
  g_hgPolyFitTbl[2].dst = &a_mode;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_hgPolyFitTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_hgPolyFit)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

/* check the inputs */
  if (shTclAddrGetFromName(interp, a_hg, (void **) &hg, "HG") != TCL_OK) {
    Tcl_SetResult(interp, "bad HG name", TCL_VOLATILE);
    return TCL_ERROR;
  }
 
  if ( a_mode < 0 ) {
    Tcl_SetResult(interp, "order must be positive integer", TCL_VOLATILE);
    return TCL_ERROR;
  }
 
/* create a VECTOR which will be filled with the coefficients of the Taylor Series 
   expansion of the function, i.e., the { a_i } in: a_0 + a_1 x + a_2 x*x + ...
*/ 
  coef = shVectorNew( a_mode+1 );
 
  fitFunc = atHgPolyFit(hg, a_mode, coef);

  if (shTclHandleNew(interp, fitName, "FUNC", fitFunc) != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to new FUNC handle", TCL_VOLATILE);
    return(TCL_ERROR);
  }
  if (shTclHandleNew(interp, coefName, "VECTOR", coef) != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to coef vector handle", TCL_VOLATILE);
    return(TCL_ERROR);
  }

/* create a keyed list with the handles to the FUNC and VECTOR */ 
  sprintf(answer, "FUNC %s", fitName);
  Tcl_AppendElement(interp, answer);

  sprintf(answer, "COEF %s", coefName);
  Tcl_AppendElement(interp, answer);


  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclProco
**
**<AUTO EXTRACT>
** TCL VERB: proco
**
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atHgMath.html#atProco>atProco</A> in atHgMath.c
** </HTML>
**</AUTO>
*/

char g_proco[] = "proco";
ftclArgvInfo g_procoTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "convert from grid to sky coordinates and back again" },
  {"<xin>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "input x coordinate (longitude in degrees of x grids)"},
  {"<yin>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "input y coordinate (latitude in degrees of y grids)"},
  {"-a0", FTCL_ARGV_DOUBLE, NULL, NULL,
   "projection center longitude (degrees), Default: 180.0"},
  {"-d0", FTCL_ARGV_DOUBLE, NULL, NULL,
   "projection center latitude (degrees), Default: 0.0"},
  {"-dx", FTCL_ARGV_DOUBLE, NULL, NULL,
   "x grid separation (degrees), Default: 0.1"},
  {"-dy", FTCL_ARGV_DOUBLE, NULL, NULL,
   "y grid separation (degrees), Default: 0.1"},
  {"-rho", FTCL_ARGV_DOUBLE, NULL, NULL,
   "rotation angle (degrees), Default: 0.0"},
  {"-sys", FTCL_ARGV_INT, NULL, NULL,
   "type of projection (1=AITOFF, 2=Equivalent\nCylindrical, 3=Flat 4=Gnomonic, 5=Orthographic,\n6=Rectangular, 7=Global Sinusoidal,\n8=North Celestial Pole, 9=Stereographic\n10=Mercator), Default: 1"},
  {"-mode", FTCL_ARGV_INT, NULL, NULL,
   "mode (0=lon,lat->x,y; 1=x,lat->lon,y; 2=lon,y->x,lat; 3=x,y->lon,lat), Default: 0"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclProco(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  char answer[100];
  int rstat;
  double xin = 11.0;
  double yin = 22.0;
  double xout, yout;
  double a0 = 180.0;
  double d0 = 0.0;
  double dx = 0.1;
  double dy = 0.1;
  double rho = 0.0;
  int sys = 1;
  int mode = 0;
  g_procoTbl[1].dst = &xin;
  g_procoTbl[2].dst = &yin;
  g_procoTbl[3].dst = &a0;
  g_procoTbl[4].dst = &d0;
  g_procoTbl[5].dst = &dx;
  g_procoTbl[6].dst = &dy;
  g_procoTbl[7].dst = &rho;
  g_procoTbl[8].dst = &sys;
  g_procoTbl[9].dst = &mode;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_procoTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_proco)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  if (atProco(&xin, &yin, &xout, &yout, 
	      &a0, &d0, &dx, &dy, &rho, &sys, &mode) != 0) {
    return TCL_ERROR;
  }
  sprintf(answer, "xout %13.8f", xout); Tcl_AppendElement(interp, answer);
  sprintf(answer, "yout %13.8f", yout); Tcl_AppendElement(interp, answer);
  return TCL_OK;

}

/****************************************************************************
**
** ROUTINE: tclRegPlot
**
**<AUTO EXTRACT>
** TCL VERB: regPlot
**
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atHgMath.html#atRegPlot>atRegPlot</A> in atHgMath.c
** </HTML>
**</AUTO>
*/

char g_regPlot[] = "regPlot";
ftclArgvInfo g_regPlotTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Plot the region using pgPlot\n" },
  {"<pgstate>", FTCL_ARGV_STRING, NULL, NULL, 
   "The PGSTATE to use"},
  {"<reg>", FTCL_ARGV_STRING, NULL, NULL, 
   "The REGION to plot"},
  {"<fg>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "array value to map to c1"},
  {"<bg>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "array value to map to c2"},
  {"<trans>", FTCL_ARGV_STRING, NULL, NULL, 
   "The TRANS from row,col to x,y position"},
  {"[mode]", FTCL_ARGV_INT, NULL, NULL, 
   "mode for scaling:  0=linear 1=log 2=sqrt, Default: 0"},
  {"-xlab", FTCL_ARGV_STRING, NULL, NULL, 
   "label for the x axis, Default: no label"},
  {"-ylab", FTCL_ARGV_STRING, NULL, NULL, 
   "label for the y axis, Default: no label"},
  {"-noBox", FTCL_ARGV_INT, NULL, NULL, 
   "if 1, do not plot box, Default: 0"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclRegPlot(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_pgstate = NULL;
  PGSTATE *pgstate = NULL;
  char *a_reg = NULL;
  REGION *reg=NULL;
  double a_fg, a_bg;
  int a_mode = 0;
  char *a_trans=NULL;
  TRANS *trans=NULL;
  char *a_xlab=NULL;
  char *a_ylab=NULL;
  int a_nobox=0;
  char a_xlabTemp[100];
  char a_ylabTemp[100];

  g_regPlotTbl[1].dst = &a_pgstate;
  g_regPlotTbl[2].dst = &a_reg;
  g_regPlotTbl[3].dst = &a_fg;
  g_regPlotTbl[4].dst = &a_bg;
  g_regPlotTbl[5].dst = &a_trans;
  g_regPlotTbl[6].dst = &a_mode;
  g_regPlotTbl[7].dst = &a_xlab;
  g_regPlotTbl[8].dst = &a_ylab;
  g_regPlotTbl[9].dst = &a_nobox;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_regPlotTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_regPlot)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_pgstate, 
			   (void **) &pgstate, "PGSTATE") != TCL_OK) {
    Tcl_SetResult(interp, "tclregPlot:  bad pgstate name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, a_reg, (void **) &reg, "REGION") 
      != TCL_OK) {
    Tcl_SetResult(interp, "tclRegPlot:  bad REGION name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, a_trans, (void **) &trans, "TRANS") 
      != TCL_OK) {
    Tcl_SetResult(interp, "tclRegPlot:  bad TRANS name", TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (a_xlab == NULL) {
    strcpy(a_xlabTemp," ");
  } else {
    strncpy(a_xlabTemp, a_xlab, 99);
  }

  if (a_ylab == NULL) {
    strcpy(a_ylabTemp," ");
  } else {
    strncpy(a_ylabTemp, a_ylab, 99);
  }

  atRegPlot(pgstate, reg, (float) a_fg, (float) a_bg, trans, a_mode,
	    a_xlabTemp, a_ylabTemp, a_nobox);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclRegColorPlot
**
**<AUTO EXTRACT>
** TCL VERB: regColorPlot
**
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atHgMath.html#atRegColorPlot>atRegColorPlot</A> in atHgMath.c
** </HTML>
**</AUTO>
*/
char g_regColorPlot[] = "regColorPlot";
ftclArgvInfo g_regColorPlotTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Plot the region in color\n" },
  {"<pgstate>", FTCL_ARGV_STRING, NULL, NULL, 
   "The PGSTATE to use"},
  {"<reg>", FTCL_ARGV_STRING, NULL, NULL, 
   "The REGION to plot"},
  {"<fg>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "array value to map to c1"},
  {"<bg>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "array value to map to c2"},
  {"<trans>", FTCL_ARGV_STRING, NULL, NULL, 
   "The TRANS from row,col to x,y position"},
  {"[smode]", FTCL_ARGV_INT, NULL, NULL, 
   "mode for scaling:  0=linear 1=log 2=sqrtn;\n"
   "Default: 0"},
  {"[cmode]", FTCL_ARGV_INT, NULL, NULL, 
   "mode for color:  1=gray; 2=rainbow;\n" 
   "3=heat; 4=weird IRAF; 5=AIPS; 6=TJP; 7=red;\n"
   "8=green; 9=blue; 10=yellow; 11=cyan;\n"
   "12=magenta; 13=SAO B; (negative values are\n"
   "inverse values, e.g., -1=inverse gray); Default : 1"},
  {"-xlab", FTCL_ARGV_STRING, NULL, NULL, 
   "label for the x axis, Default: no label"},
  {"-ylab", FTCL_ARGV_STRING, NULL, NULL, 
   "label for the y axis, Default: no label"},
  {"-noBox", FTCL_ARGV_INT, NULL, NULL, 
   "if 1, do not plot box, Default: 0"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclRegColorPlot(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_pgstate = NULL;
  PGSTATE *pgstate = NULL;
  char *a_reg = NULL;
  REGION *reg=NULL;
  double a_fg, a_bg;
  int a_smode = 0;
  int a_cmode = 1;
  char *a_trans=NULL;
  TRANS *trans=NULL;
  char *a_xlab=NULL;
  char *a_ylab=NULL;
  int a_nobox=0;
  char a_xlabTemp[100];
  char a_ylabTemp[100];

  g_regColorPlotTbl[1].dst = &a_pgstate;
  g_regColorPlotTbl[2].dst = &a_reg;
  g_regColorPlotTbl[3].dst = &a_fg;
  g_regColorPlotTbl[4].dst = &a_bg;
  g_regColorPlotTbl[5].dst = &a_trans;
  g_regColorPlotTbl[6].dst = &a_smode;
  g_regColorPlotTbl[7].dst = &a_cmode;
  g_regColorPlotTbl[8].dst = &a_xlab;
  g_regColorPlotTbl[9].dst = &a_ylab;
  g_regColorPlotTbl[10].dst = &a_nobox;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_regColorPlotTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_regColorPlot)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_pgstate, 
			   (void **) &pgstate, "PGSTATE") != TCL_OK) {
    Tcl_SetResult(interp, "tclregColorPlot:  bad pgstate name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, a_reg, (void **) &reg, "REGION") 
      != TCL_OK) {
    Tcl_SetResult(interp, "tclRegColorPlot:  bad REGION name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, a_trans, (void **) &trans, "TRANS") 
      != TCL_OK) {
    Tcl_SetResult(interp, "tclRegColorPlot:  bad TRANS name", TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (a_xlab == NULL) {
    strcpy(a_xlabTemp," ");
  } else {
    strncpy(a_xlabTemp, a_xlab, 99);
  }

  if (a_ylab == NULL) {
    strcpy(a_ylabTemp," ");
  } else {
    strncpy(a_ylabTemp, a_ylab, 99);
  }
  atRegColorPlot(pgstate, reg, (float) a_fg, (float) a_bg, trans, 
		 a_smode, a_cmode, a_xlabTemp, a_ylabTemp, a_nobox);
  return TCL_OK;
}


/****************************************************************************
**
** ROUTINE: tclHg2New
**
**<AUTO EXTRACT>
** TCL VERB: hg2New
**
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atHgMath.html#atHg2New>atHg2New</A> in atHgMath.c
** </HTML>
**
**</AUTO>
*/
char g_hg2New[] = "hg2New";
ftclArgvInfo g_hg2NewTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Create a new HG2\n" },
  {"<xmin>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "minimum x value"},
  {"<xmax>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "maximum x value"},
  {"<nx>", FTCL_ARGV_INT, NULL, NULL, 
   "number of x bins"},
  {"<ymin>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "minimum y value"},
  {"<ymax>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "maximum y value"},
  {"<ny>", FTCL_ARGV_INT, NULL, NULL, 
   "number of y bins"},
  {"-xlab", FTCL_ARGV_STRING, NULL, NULL, 
   "label for the x axis, Default: no label"},
  {"-ylab", FTCL_ARGV_STRING, NULL, NULL, 
   "label for the y axis, Default: no label"},
  {"-name", FTCL_ARGV_STRING, NULL, NULL, 
   "name for the HG2, Default: none"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclHg2New(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  char name[HANDLE_NAMELEN];
  int rstat;
  double a_xmin = 0.0;
  double a_xmax = 0.0;
  int a_nx = 0;
  double a_ymin = 0.0;
  double a_ymax = 0.0;
  int a_ny = 0;
  char *a_xlab = NULL;
  char *a_ylab = NULL;
  char *a_name = NULL;
  char a_xlabTemp[100];
  char a_ylabTemp[100];
  char a_nameTemp[100];

  HG2 *hg2 = NULL;

  g_hg2NewTbl[1].dst = &a_xmin;
  g_hg2NewTbl[2].dst = &a_xmax;
  g_hg2NewTbl[3].dst = &a_nx;
  g_hg2NewTbl[4].dst = &a_ymin;
  g_hg2NewTbl[5].dst = &a_ymax;
  g_hg2NewTbl[6].dst = &a_ny;
  g_hg2NewTbl[7].dst = &a_xlab;
  g_hg2NewTbl[8].dst = &a_ylab;
  g_hg2NewTbl[9].dst = &a_name;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_hg2NewTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_hg2New)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if ( a_xmin > a_xmax ) {
    Tcl_SetResult(interp, "xmin greater than xmax", TCL_VOLATILE);
    return TCL_ERROR;
  }

  if ( a_ymin > a_ymax ) {
    Tcl_SetResult(interp, "ymin greater than ymax", TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (a_xlab == NULL) {
    strcpy(a_xlabTemp, " ");
  } else {
    strncpy(a_xlabTemp, a_xlab, 99);
  }

  if (a_ylab == NULL) {
    strcpy(a_ylabTemp, " ");
  } else {
    strncpy(a_ylabTemp, a_ylab, 99);
  }

  if (a_name == NULL) {
    strcpy(a_nameTemp, " ");
  } else {
    strncpy(a_nameTemp, a_name, 99);
  }

  hg2 = atHg2New(a_xmin, a_xmax, a_nx, a_ymin, a_ymax, a_ny,
	       a_xlabTemp, a_ylabTemp, a_nameTemp);
  if (shTclHandleNew(interp, name, "HG2", hg2) != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to new hg2 handle", TCL_STATIC);
    return(TCL_ERROR);
  }
  Tcl_SetResult(interp, name, TCL_VOLATILE);
  return TCL_OK;

}


/****************************************************************************
**
** ROUTINE: tclHg2Del
**
**
**<AUTO EXTRACT>
** TCL VERB: hg2Del
**
** <HTML>
** C ROUTINE USED:
** <A HREF=atHgMath.html#atHg2Del>atHg2Del</A> in atHgMath.c
** </HTML>
**
**</AUTO>
*/
char g_hg2Del[] = "hg2Del";
ftclArgvInfo g_hg2DelTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Delete the HG2 and its handle\n" },
  {"<hndl>", FTCL_ARGV_STRING, NULL, NULL, 
   "The HG2's handle"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclHg2Del(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_hndl = NULL;
  HG2 *hg2=NULL;

  g_hg2DelTbl[1].dst = &a_hndl;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_hg2DelTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_hg2Del)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_hndl, (void **) &hg2, "HG2") != TCL_OK) {
    Tcl_SetResult(interp, "tclHg2Del:  bad HG2 name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  atHg2Del(hg2);
  p_shTclHandleDel(interp, a_hndl);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclHg2Fill
**
**
**<AUTO EXTRACT>
** TCL VERB: hg2Fill
**
** <HTML>
** C ROUTINE USED:
** <A HREF=atHgMath.html#atHg2Fill>atHg2Fill</A> in atHgMath
** </HTML>
**
**</AUTO>
*/
char g_hg2Fill[] = "hg2Fill";
ftclArgvInfo g_hg2FillTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Fill the HG2 and its handle\n" },
  {"<hndl>", FTCL_ARGV_STRING, NULL, NULL, 
   "The HG2's handle"},
  {"<x>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "x position in the HG2"},
  {"<y>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "y position in the HG2"},
  {"[w]", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "weight; Default: 0"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclHg2Fill(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_hndl = NULL;
  HG2 *hg2=NULL;
  double a_x = 0;
  double a_y = 0;
  double a_w = 0;

  g_hg2FillTbl[1].dst = &a_hndl;
  g_hg2FillTbl[2].dst = &a_x;
  g_hg2FillTbl[3].dst = &a_y;
  g_hg2FillTbl[4].dst = &a_w;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_hg2FillTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_hg2Fill)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_hndl, (void **) &hg2, "HG2") != TCL_OK) {
    Tcl_SetResult(interp, "tclHg2Fill:  bad HG2 name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  atHg2Fill(hg2, a_x, a_y, a_w);
  return TCL_OK;
}
/****************************************************************************
**
** ROUTINE: tclHg2Get
**
**
**<AUTO EXTRACT>
** TCL VERB: hg2Get
**
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atHgMath.html#atHg2Get>atHg2Get</A> in atHgMath.c
** </HTML>
**
**</AUTO>
*/
char g_hg2Get[] = "hg2Get";
ftclArgvInfo g_hg2GetTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Get the contents of the hg2\n" },
  {"<hndl>", FTCL_ARGV_STRING, NULL, NULL, 
   "The HG2's handle"},
  {"<x>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "x position in the HG2"},
  {"<y>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "y position in the HG2"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclHg2Get(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_hndl = NULL;
  HG2 *hg2=NULL;
  double a_x = 0;
  double a_y = 0;
  char answer[100];

  g_hg2GetTbl[1].dst = &a_hndl;
  g_hg2GetTbl[2].dst = &a_x;
  g_hg2GetTbl[3].dst = &a_y;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_hg2GetTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_hg2Get)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_hndl, (void **) &hg2, "HG2") != TCL_OK) {
    Tcl_SetResult(interp, "tclHg2Get:  bad HG2 name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  sprintf(answer,"%f", atHg2Get(hg2, a_x, a_y));
  Tcl_SetResult(interp, answer, TCL_VOLATILE);
  return TCL_OK;
}
/****************************************************************************
**
** ROUTINE: tclHg2VFill
**
**
**<AUTO EXTRACT>
** TCL VERB: hg2VFill
**
** <HTML>
** C ROUTINE CALLED: 
** <A HREF=atHgMath.html#atHg2VFill>atHg2VFill</A> in atHgMath.c
** </HTML>
**
**</AUTO>
*/
char g_hg2VFill[] = "hg2VFill";
ftclArgvInfo g_hg2VFillTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Fill the HG2 and its handle\n" },
  {"<hndl>", FTCL_ARGV_STRING, NULL, NULL, 
   "The HG2's handle"},
  {"<x>", FTCL_ARGV_STRING, NULL, NULL, 
   "vector with x positions in the HG2"},
  {"<y>", FTCL_ARGV_STRING, NULL, NULL, 
   "vector with y positions in the HG2"},
  {"[w]", FTCL_ARGV_STRING, NULL, NULL, 
   "vector with weights; Default: all weights = 1"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclHg2VFill(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_hndl = NULL;
  HG2 *hg2=NULL;
  char *a_x = NULL;
  char *a_y = NULL;
  char *a_w = NULL;
  VECTOR *vx = NULL;
  VECTOR *vy = NULL;
  VECTOR *vw = NULL;

  g_hg2VFillTbl[1].dst = &a_hndl;
  g_hg2VFillTbl[2].dst = &a_x;
  g_hg2VFillTbl[3].dst = &a_y;
  g_hg2VFillTbl[4].dst = &a_w;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_hg2VFillTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_hg2VFill)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_hndl, (void **) &hg2, "HG2") != TCL_OK) {
    Tcl_SetResult(interp, "tclHg2VFill:  bad HG2 name", TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (shTclAddrGetFromName(interp, a_x, (void **) &vx, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclHg2VFill:  bad vx name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, a_y, (void **) &vy, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclHg2VFill:  bad vy name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (a_w != NULL) {
    if (shTclAddrGetFromName(interp, a_w, (void **) &vw, "VECTOR") != TCL_OK) {
      Tcl_SetResult(interp, "tclHg2VFill:  bad vw name", TCL_VOLATILE);
      return TCL_ERROR;
    }
  } else {
    vw = (VECTOR *) NULL;
  }
  atHg2VFill(hg2, vx, vy, vw);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclHg2Cont
**
**
**<AUTO EXTRACT>
** TCL VERB: hg2Cont
**
** <HTML>
** C ROUTINE USED:
** <A HREF=atHgMath.html#atHg2Cont>atHg2Cont</A> in atHgMath.c
** </HTML>
**
**</AUTO>
*/
char g_hg2Cont[] = "hg2Cont";
ftclArgvInfo g_hg2ContTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Plot the HG2 using pgCont\n" },
  {"<pgstate>", FTCL_ARGV_STRING, NULL, NULL, 
   "The PGSTATE to use"},
  {"<hg2>", FTCL_ARGV_STRING, NULL, NULL, 
   "The HG2 to plot"},
  {"<cVec>", FTCL_ARGV_STRING, NULL, NULL, 
   "VECTOR of contour levels"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclHg2Cont(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_pgstate = NULL;
  PGSTATE *pgstate = NULL;
  char *a_hg2 = NULL;
  HG2 *hg2=NULL;
  char *a_cVec = NULL;
  VECTOR *cVec = NULL;

  g_hg2ContTbl[1].dst = &a_pgstate;
  g_hg2ContTbl[2].dst = &a_hg2;
  g_hg2ContTbl[3].dst = &a_cVec;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_hg2ContTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_hg2Cont)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_pgstate, 
			   (void **) &pgstate, "PGSTATE") != TCL_OK) {
    Tcl_SetResult(interp, "tclHg2Cont:  bad pgstate name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, a_hg2, (void **) &hg2, "HG2") != TCL_OK) {
    Tcl_SetResult(interp, "tclHg2Cont:  bad HG2 name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, a_cVec, 
			   (void **) &cVec, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclHg2Cont:  bad cVec name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  atHg2Cont(pgstate, hg2, cVec);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclHg2Gray
**
**
**<AUTO EXTRACT>
** TCL VERB: hg2Gray
**
** <HTML>
** C ROUTINE USED:
** <A HREF=atHgMath.html#atHg2Gray>atHg2Gray</A> in atHgMath.c
** </HTML>
**
**</AUTO>
*/
char g_hg2Gray[] = "hg2Gray";
ftclArgvInfo g_hg2GrayTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Plot the HG2 using pgGray\n" },
  {"<pgstate>", FTCL_ARGV_STRING, NULL, NULL, 
   "The PGSTATE to use"},
  {"<hg2>", FTCL_ARGV_STRING, NULL, NULL, 
   "The HG2 to plot"},
  {"<a1>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "array value to map to c1"},
  {"<a2>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "array value to map to c2"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclHg2Gray(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_pgstate = NULL;
  PGSTATE *pgstate = NULL;
  char *a_hg2 = NULL;
  HG2 *hg2=NULL;
  double a_fg, a_bg;

  g_hg2GrayTbl[1].dst = &a_pgstate;
  g_hg2GrayTbl[2].dst = &a_hg2;
  g_hg2GrayTbl[3].dst = &a_fg;
  g_hg2GrayTbl[4].dst = &a_bg;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_hg2GrayTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_hg2Gray)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_pgstate, 
			   (void **) &pgstate, "PGSTATE") != TCL_OK) {
    Tcl_SetResult(interp, "tclHg2Gray:  bad pgstate name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, a_hg2, (void **) &hg2, "HG2") != TCL_OK) {
    Tcl_SetResult(interp, "tclHg2Gray:  bad HG2 name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  atHg2Gray(pgstate, hg2, (float) a_fg, (float) a_bg);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclregHistEq
**
**
**<AUTO EXTRACT>
** TCL VERB: regHistEq
**
** <HTML>
** C ROUTINE USED:
** <A HREF=atHgMath.html#atregHistEq>atRegHistEq</A> in atHgMath.c
** </HTML>
**
**</AUTO>
*/
char g_regHistEq[] = "regHistEq";
ftclArgvInfo g_regHistEqTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "histogram equalization of a region\n" },
  {"<region>", FTCL_ARGV_STRING, NULL, NULL, 
   "The REGION to equalize"},
  {"<hg>", FTCL_ARGV_STRING, NULL, NULL, 
   "The HG to use for equalization"},
  {"<min>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "minimum equalized value"},
  {"<max>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "maximum equalized value"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclregHistEq(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_reg = NULL;
  REGION *reg = NULL;
  char *a_hg = NULL;
  HG *hg=NULL;
  double min, max;

  g_regHistEqTbl[1].dst = &a_reg;
  g_regHistEqTbl[2].dst = &a_hg;
  g_regHistEqTbl[3].dst = &min;
  g_regHistEqTbl[4].dst = &max;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_regHistEqTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_regHistEq)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_reg, 
			   (void **) &reg, "REGION") != TCL_OK) {
    Tcl_SetResult(interp, "tclRegHistEq:  bad REGION name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, a_hg, (void **) &hg, "HG") != TCL_OK) {
    Tcl_SetResult(interp, "tclRegHistEq:  bad HG name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  atRegHistEq(reg, hg, min, max);
  return TCL_OK;
}

/**********************************************************************/
/* Declare new tcl verbs for ASTROTOOLS HG functions*/
void atTclHgMathDeclare(Tcl_Interp *interp) {
  
  
  int flags = FTCL_ARGV_NO_LEFTOVERS;

  shTclDeclare(interp, g_hgGaussFit, 
               (Tcl_CmdProc *) tclHgGaussFit,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_hgGaussFitTbl, flags, 
			       g_hgGaussFit),
               shTclGetUsage(interp, g_hgGaussFitTbl, flags, 
			     g_hgGaussFit));

  shTclDeclare(interp, g_hgPolyFit, 
               (Tcl_CmdProc *) tclHgPolyFit,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_hgPolyFitTbl, flags, 
			       g_hgPolyFit),
               shTclGetUsage(interp, g_hgPolyFitTbl, flags, 
			     g_hgPolyFit));

  shTclDeclare(interp, g_proco, 
               (Tcl_CmdProc *) tclProco,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_procoTbl, flags, 
			       g_proco),
               shTclGetUsage(interp, g_procoTbl, flags, 
			     g_proco));

 shTclDeclare(interp, g_regPlot,
               (Tcl_CmdProc *) tclRegPlot,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_regPlotTbl,
                               flags, g_regPlot),
               shTclGetUsage(interp, g_regPlotTbl,
                             flags, g_regPlot));

 
 shTclDeclare(interp, g_regColorPlot,
               (Tcl_CmdProc *) tclRegColorPlot,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_regColorPlotTbl,
                               flags, g_regColorPlot),
               shTclGetUsage(interp, g_regColorPlotTbl,
                             flags, g_regColorPlot));

  shTclDeclare(interp, g_hg2New, 
               (Tcl_CmdProc *) tclHg2New,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_hg2NewTbl, flags, g_hg2New),
               shTclGetUsage(interp, g_hg2NewTbl, flags, g_hg2New));

  shTclDeclare(interp, g_hg2Del, 
               (Tcl_CmdProc *) tclHg2Del,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_hg2DelTbl, flags, g_hg2Del),
               shTclGetUsage(interp, g_hg2DelTbl, flags, g_hg2Del));

  shTclDeclare(interp, g_hg2Fill, 
               (Tcl_CmdProc *) tclHg2Fill,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_hg2FillTbl, flags, g_hg2Fill),
               shTclGetUsage(interp, g_hg2FillTbl, flags, g_hg2Fill));

  shTclDeclare(interp, g_hg2Get, 
               (Tcl_CmdProc *) tclHg2Get,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_hg2GetTbl, flags, g_hg2Get),
               shTclGetUsage(interp, g_hg2GetTbl, flags, g_hg2Get));

  shTclDeclare(interp, g_hg2VFill, 
               (Tcl_CmdProc *) tclHg2VFill,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_hg2VFillTbl, flags, g_hg2VFill),
               shTclGetUsage(interp, g_hg2VFillTbl, flags, g_hg2VFill));

  shTclDeclare(interp, g_hg2Cont, 
               (Tcl_CmdProc *) tclHg2Cont,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_hg2ContTbl, flags, g_hg2Cont),
               shTclGetUsage(interp, g_hg2ContTbl, flags, g_hg2Cont));

  shTclDeclare(interp, g_hg2Gray, 
               (Tcl_CmdProc *) tclHg2Gray,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_hg2GrayTbl, flags, g_hg2Gray),
               shTclGetUsage(interp, g_hg2GrayTbl, flags, g_hg2Gray));

  shTclDeclare(interp, g_regHistEq, 
               (Tcl_CmdProc *) tclregHistEq,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_regHistEqTbl, flags, g_regHistEq),
               shTclGetUsage(interp, g_regHistEqTbl, flags, g_regHistEq));

}
