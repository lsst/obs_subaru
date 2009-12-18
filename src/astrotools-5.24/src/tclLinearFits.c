/* <AUTO>
   FILE: tclLinearFits
   ABSTRACT:
<HTML>
   Routines for linear fitting:
<P>
Given a set of nVar arrays of independent variables and one array
of dependent variables, this set of tools does the linear fit
and displays the result.
</HTML>
   </AUTO>
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "tcl.h"
#include "shCUtils.h"
#include "atSlalib.h"
#include "shCErrStack.h"
#include "shTclHandle.h"
#include "shTclRegSupport.h"            /* needed for shTclDeclare prototype */
#include "tclMath.h"
#include "atSlalib.h"
#include "atLinearFits.h"
static char *module = "astrotools";    /* name of this set of code */

/****************************************************************************
 * <AUTO EXTRACT>
 *
 * TCL VERB: vLinEqn

 * <HTML>
 * C ROUTINE CALLED:
 * <A HREF=atLinearFits.html#atVLinEqn>atVLinEqn</A> in atLinearFits.c

 * <P>
 * DESCRIPTION:
 * Solve a set of linear equations: A X = B, where A is a matrix composed
 * of one vector (column) for the coeffients of each independent variable,
 * X is the vector of unknowns which are solved for, and B is a vector
 * containing the values of the dependent variable.
 * <P>
 * Each vector for an independent variable contains the constans for one
 * of the unknowns, e.g., to solve the set of equations
 * <PRE>
 *       2x - 3y = -13
 *       3x + 2y = -13
 * </PRE>
 * pass in two vectors: a1 = (2, 3) and a2 = (-3, 2), as well as the
 * vector B = (-13, -13).  The answer can be extracted from the FUNC
 * structure which is returned.  (See ..astrotools/test/testLinearFits.tcl
 * or the SDSSMATH home page for how to do this.)
 * </HTML>

 * </AUTO>
 */

static ftclArgvInfo vLinEqnArgTable[] = {
      { NULL,              FTCL_ARGV_HELP,   NULL, NULL,
            "Solve a set of linear equations\n"},
      { "<dep vector>",    FTCL_ARGV_STRING, NULL, NULL,
            "Vector with coefficients for dependent vector"},
      { "<ind vectors>",   FTCL_ARGV_STRING, NULL, NULL, 
            "List of vectors with coefficients for each independent variable"},
      { NULL,              FTCL_ARGV_END,    NULL, NULL, NULL }
};

char g_vLinEqn[] = "vLinEqn";

static int
tclVLinEqn(ClientData clientData, Tcl_Interp *interp, int argc, char **argv) {
  int ct, rstatus; 
  int l_argc;
  char errors[80], fitFuncName[HANDLE_NAMELEN];
  char *p_list = NULL, *depVectorPtr = NULL;
  char **l_argv;
  VECTOR *depVector = NULL;
  VECTOR **Vector;
  FUNC *fitFunc;

  vLinEqnArgTable[1].dst = &depVectorPtr;
  vLinEqnArgTable[2].dst = &p_list;

/* error check */
  if ( argc != 3 ) {
    sprintf(errors, "Incorrect number of inputs"); 
    Tcl_SetResult(interp, errors, TCL_VOLATILE);
    return(TCL_ERROR);
  }

  if ( (rstatus = shTclParseArgv( interp, &argc, argv, vLinEqnArgTable,
                                  FTCL_ARGV_NO_LEFTOVERS, g_vLinEqn ) )
    != FTCL_ARGV_SUCCESS) {

       return(rstatus);
   
  } else { 

/* if inputs are okay, parse the list of independent variable vectors */
    if (Tcl_SplitList(interp, p_list, &l_argc, &l_argv) == TCL_ERROR) {
      sprintf(errors, "Error parsing input list of independent vectors.\n");
      Tcl_SetResult(interp, errors, TCL_VOLATILE);
      return(TCL_ERROR);
    }

/* allocate space */
    Vector = (VECTOR **)shMalloc( (l_argc) * sizeof(VECTOR *) );
/*    depVector = (VECTOR *)shMalloc( sizeof(VECTOR *) ); */

/* actually put vectors from list of independent vectors into 'Vector' */
    for ( ct = 0; ct < l_argc; ct++ ) {
      if ( shTclAddrGetFromName(
              interp, l_argv[ct], (void **)&Vector[ct], "VECTOR") != TCL_OK ) {
        sprintf(errors, "Bad independent vector: %s", l_argv[ct]);  
        Tcl_SetResult(interp, errors, TCL_VOLATILE);
        free((char *)l_argv);
        shFree(Vector);
/*        shFree(depVector); */
        return(TCL_ERROR);
      }
    }

/* put the dependent vector into 'depVector' */
    if ( shTclAddrGetFromName(
             interp, depVectorPtr, (void **)&depVector, "VECTOR") != TCL_OK ) {
      sprintf(errors, "Bad dependent vector\n");  
      Tcl_SetResult(interp, errors, TCL_VOLATILE);
      free((char *)l_argv);
      shFree(Vector);
/*      shFree(depVector); */
      return(TCL_ERROR);
    }

/* make sure all the vectors are the same size */
    for ( ct = 0; ct < l_argc; ct++ ) {
      if ( Vector[ct]->dimen != depVector->dimen ) {
        free((char *)l_argv);
        shFree(Vector);
/*        shFree(depVector); */
        sprintf(errors, "Error parsing input list of independent vectors.\n");
        Tcl_SetResult(interp, errors, TCL_VOLATILE);
        return(TCL_ERROR);
      }
    }

/* call the C-function */
      fitFunc = atVLinEqn(depVector, Vector, l_argc );
  }

  if ( fitFunc == NULL ) { 
    sprintf(errors, "No FUNC came back from atVLinEqn, there may not be a valid solution\n");
    Tcl_SetResult(interp, errors, TCL_VOLATILE);
    free((char *)l_argv);
    shFree(Vector);
/*    shFree(depVector); */
    return(TCL_ERROR);
  }

/* if everything has worked, return the fitFunc */
  if (shTclHandleNew(interp, fitFuncName, "FUNC", fitFunc) != TCL_OK ) {
    sprintf(errors, "Unable to bind to the FUNC from atVLinEqn\n");
    Tcl_SetResult(interp, errors, TCL_VOLATILE);
    free((char *)l_argv);
    shFree(Vector);
/*    shFree(depVector); */
    return(TCL_ERROR);
  }

  Tcl_SetResult(interp, fitFuncName, TCL_VOLATILE);

  free((char *)l_argv);
  shFree(Vector);
/*  shFree(depVector); */

  return TCL_OK;
}

/*=========================================================================*/
			/* Jim Annis 	June 14, 1996 */
/*<AUTO EXTRACT>

  TCL VERB: vLinFitBces

  <HTML>
  BCES (Bivariate Correlated Errors and intrinsic Scatter) is a linear
  regression algorithm that allows for: 
  <UL>
  <li> measurement errors on both variables
  <li> measurement errors that depend on the measurement 
  <li> correlated measurement errors
  <li> scatter intrinsic to the relation being fitted, beyond measument error
  </UL>
  The routine performs four fits: y regressed on x, x regressed on y,
  the bisector, and orthogonal errors. Which answer is the "right" one
  depends on the situation. (A simplified guide would be: if you wish
  to predict a y given an x, use y regressed on x. If you wish to learn
  about the relationship of two quantities, use the bisector. See
  Feigelson and Babu, ApJ 397, 55 1992 for details.)
  <p>
  The algorithm and the base fortran code are from Akritas and Bershady,
  ApJ 470, ? 1996.
  <p> Also returned are the results of a bootstrap ananlysis.
  <p> The "slopeErr" and "slopeErrBoot" lists have two extra elements on them.
	These are variences for the bisector and orthogonal slopes calculated
	using a technique of wider applicability than the usual one 
	(which assumes that the residuals in Y about a line are independant
	of the value of X; see Isobe, Feigelson, Akritas, and Babu,  ApJ 364, 104 1990)
  <p> The covarience vector may be all zeros.
  <p> James Annis, June 14, 1996
  <p>

  C ROUTINE CALLED:
  <A HREF=atLinearFits.html#atBcesFit>atBcesFit</A> in atLinearFits.c
  </HTML>

  </AUTO>
*/
static ftclArgvInfo vLinFitBcesTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL,
  "Perform BCES linear regression of Y onto X using YErr, XErr, and\n"
  "covaraience XYErr. Return fits for Y on X, X on Y, bisector, and orthogonal.\n"},
  {"<X>",  FTCL_ARGV_STRING, NULL, NULL, "vector containing x values"},
  {"<XErr>",  FTCL_ARGV_STRING, NULL, NULL, "vector containing x error values"},
  {"<Y>",  FTCL_ARGV_STRING, NULL, NULL, "vector containing y values"},
  {"<YErr>",  FTCL_ARGV_STRING, NULL, NULL, "vector containing y error values"},
  {"<XYErr>",  FTCL_ARGV_STRING, NULL, NULL, "vector containing xy covariance error values"},
  {"-nsim",    FTCL_ARGV_INT, NULL, NULL, "number of bootstrap simulations [100]"},
  {"-seed",    FTCL_ARGV_INT, NULL, NULL, "integer seed for random number generator [1]"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *vLinFitBces_name = "vLinFitBces";

static int
tclVLinFitBces(ClientData cd, Tcl_Interp *interp, int argc, char **argv) {

  VECTOR *X = NULL;
  VECTOR *XErr = NULL;
  VECTOR *Y = NULL;
  VECTOR *YErr = NULL;
  VECTOR *XYErr = NULL;
  char *xName;
  char *yName;
  char *xeName;
  char *yeName;
  char *xyeName;
  int nsim = 100;
  int seed = 1;
			/* these are all malloced in the subroutine */
  double *slope;	/* bces fit */
  double *slopeErr;	/* bces fit sigma */
  double *yInt;		/* bces fit */
  double *yIntErr;	/* bces fit sigma */
  double *slopeBoot;  	/* bootstrap fit */
  double *slopeErrBoot;	/* bootstrap fit sigma */
  double *yIntBoot;  	/* bootstrap fit */
  double *yIntErrBoot; 	/* bootstrap fit sigma */
  double *slopeErrIF;	/* isobe et al. alt. slope variance method */
  double *slopeErrBootIF;	/* bootstrap of that */
  int i;
  int nmodels = ATLFNMODELS;  /* currently 4 models are used */
  int nmodelsBig;

  char answer[500];              /* sent back to Tcl */
  char shortAnswer[500];          /* used to make string sent back to Tcl */
  int returnFlag;
/*
  static char *function_name = "vLinFitBces";
*/

  vLinFitBcesTable[1].dst = &xName;
  vLinFitBcesTable[2].dst = &xeName;
  vLinFitBcesTable[3].dst = &yName;
  vLinFitBcesTable[4].dst = &yeName;
  vLinFitBcesTable[5].dst = &xyeName;
  vLinFitBcesTable[6].dst = &nsim;
  vLinFitBcesTable[7].dst = &seed;

  if((returnFlag = shTclParseArgv(
	interp, &argc, argv, vLinFitBcesTable, 0, vLinFitBces_name))
		!= FTCL_ARGV_SUCCESS) {
	return(returnFlag);
  }

  /* get X variable */
  if (shTclAddrGetFromName
      (interp, xName, (void **) &X, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "bad X VECTOR name",TCL_VOLATILE);
    return TCL_ERROR;
  }

  /* get XErr */
  if (shTclAddrGetFromName
      (interp, xeName, (void **) &XErr, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "bad XErr VECTOR name",TCL_VOLATILE);
    return TCL_ERROR;
  }

  /* get Y variable */
  if (shTclAddrGetFromName
      (interp, yName, (void **) &Y, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "bad Y VECTOR name",TCL_VOLATILE);
    return TCL_ERROR;
  }

  /* get YErr */
  if (shTclAddrGetFromName
      (interp, yeName, (void **) &YErr, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "bad YErr VECTOR name",TCL_VOLATILE);
    return TCL_ERROR;
  }

  /* get XYErr */
  if (shTclAddrGetFromName
      (interp, xyeName, (void **) &XYErr, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "bad XYErr VECTOR name",TCL_VOLATILE);
    return TCL_ERROR;
  }

  /* make sure everything makes sense */
  if (X->dimen != XErr->dimen || X->dimen != Y->dimen || X->dimen != YErr->dimen ||
          X->dimen != XYErr->dimen) {
     Tcl_SetResult(interp, "atVLinFitBces: vectors must be same size",TCL_VOLATILE);
     return TCL_ERROR;
  }

  /* make some space for the answers */
  nmodelsBig = nmodels +1;
  slope = (double *) shMalloc (nmodelsBig * sizeof(double *));
  slopeErr = (double *) shMalloc (nmodelsBig * sizeof(double *));
  yInt = (double *) shMalloc (nmodelsBig * sizeof(double *));
  yIntErr = (double *) shMalloc (nmodelsBig * sizeof(double *));
  slopeBoot = (double *) shMalloc (nmodelsBig * sizeof(double *));
  slopeErrBoot = (double *) shMalloc (nmodelsBig * sizeof(double *));
  yIntBoot = (double *) shMalloc (nmodelsBig * sizeof(double *));
  yIntErrBoot = (double *) shMalloc (nmodelsBig * sizeof(double *));
  slopeErrIF = (double *) shMalloc (nmodelsBig * sizeof(double *));
  slopeErrBootIF = (double *) shMalloc (nmodelsBig * sizeof(double *));


  returnFlag = atBcesFit(X->vec, Y->vec, XErr->vec, YErr->vec, XYErr->vec, 
		X->dimen, nsim, seed, slope, slopeErr, yInt, yIntErr,
		slopeBoot, slopeErrBoot, yIntBoot, yIntErrBoot, 
		slopeErrIF, slopeErrBootIF);

/* I fail to see a clean way of returning the number and names of the
 * 	models used for fitting by bcesFit. So, I will just hard code it
 *	here, and hope that someone else will clean it up in the future 
 */
  if (returnFlag == 0) {
        Tcl_SetResult(interp, "", TCL_VOLATILE);
        sprintf(answer, "model "); /* hardwire 4 models and their names */
        sprintf(shortAnswer, "{"); strcat(answer,shortAnswer);
        sprintf(shortAnswer, "yOnX "); strcat(answer,shortAnswer);
        sprintf(shortAnswer, "xOnY "); strcat(answer,shortAnswer);
        sprintf(shortAnswer, "bi "); strcat(answer,shortAnswer);
        sprintf(shortAnswer, "orth "); strcat(answer,shortAnswer);
        sprintf(shortAnswer, "} "); strcat(answer,shortAnswer);
        Tcl_AppendElement(interp, answer);

        sprintf(answer, "slope "); 
        sprintf(shortAnswer, "{"); strcat(answer,shortAnswer);
        for (i=0; i <4; i++) {		
        	sprintf(shortAnswer, "%.6g ",slope[i]); strcat(answer,shortAnswer);
	} 
        sprintf(shortAnswer, "} "); strcat(answer,shortAnswer);
        Tcl_AppendElement(interp, answer);

        sprintf(answer, "slopeErr "); 
        sprintf(shortAnswer, "{"); strcat(answer,shortAnswer);
        for (i=0; i <4; i++) {		
        	sprintf(shortAnswer, "%.6g ",slopeErr[i]); strcat(answer,shortAnswer);
	}
        sprintf(shortAnswer, "%.6g ",slopeErrIF[2]); strcat(answer,shortAnswer);
        sprintf(shortAnswer, "%.6g ",slopeErrIF[3]); strcat(answer,shortAnswer);
        sprintf(shortAnswer, "} "); strcat(answer,shortAnswer);
        Tcl_AppendElement(interp, answer);

        sprintf(answer, "yInt "); 
        sprintf(shortAnswer, "{"); strcat(answer,shortAnswer);
        for (i=0; i <4; i++) {		
        	sprintf(shortAnswer, "%.6g ",yInt[i]); strcat(answer,shortAnswer);
	} 
        sprintf(shortAnswer, "} "); strcat(answer,shortAnswer);
        Tcl_AppendElement(interp, answer);

        sprintf(answer, "yIntErr "); 
        sprintf(shortAnswer, "{"); strcat(answer,shortAnswer);
        for (i=0; i <4; i++) {		
        	sprintf(shortAnswer, "%.6g ",yIntErr[i]); strcat(answer,shortAnswer);
	}
        sprintf(shortAnswer, "} "); strcat(answer,shortAnswer);
        Tcl_AppendElement(interp, answer);

        sprintf(answer, "slopeBoot "); 
        sprintf(shortAnswer, "{"); strcat(answer,shortAnswer);
        for (i=0; i <4; i++) {		
        	sprintf(shortAnswer, "%.6g ",slopeBoot[i]); strcat(answer,shortAnswer);
	} 
        sprintf(shortAnswer, "} "); strcat(answer,shortAnswer);
        Tcl_AppendElement(interp, answer);

        sprintf(answer, "slopeErrBoot "); 
        sprintf(shortAnswer, "{"); strcat(answer,shortAnswer);
        for (i=0; i <4; i++) {		
        	sprintf(shortAnswer, "%.6g ",slopeErrBoot[i]); strcat(answer,shortAnswer);
	}
        sprintf(shortAnswer, "%.6g ",slopeErrBootIF[2]); strcat(answer,shortAnswer);
        sprintf(shortAnswer, "%.6g ",slopeErrBootIF[3]); strcat(answer,shortAnswer);
        sprintf(shortAnswer, "} "); strcat(answer,shortAnswer);
        Tcl_AppendElement(interp, answer);

        sprintf(answer, "yIntBoot "); 
        sprintf(shortAnswer, "{"); strcat(answer,shortAnswer);
        for (i=0; i <4; i++) {		
        	sprintf(shortAnswer, "%.6g ",yIntBoot[i]); strcat(answer,shortAnswer);
	} 
        sprintf(shortAnswer, "} "); strcat(answer,shortAnswer);
        Tcl_AppendElement(interp, answer);

        sprintf(answer, "yIntErrBoot "); 
        sprintf(shortAnswer, "{"); strcat(answer,shortAnswer);
        for (i=0; i <4; i++) {		
        	sprintf(shortAnswer, "%.6g ",yIntErrBoot[i]); strcat(answer,shortAnswer);
	}
        sprintf(shortAnswer, "} "); strcat(answer,shortAnswer);
        Tcl_AppendElement(interp, answer);
   }
   shFree(slope);
   shFree(slopeErr);
   shFree(yInt);
   shFree(yIntErr);
   shFree(slopeBoot);
   shFree(slopeErrBoot);
   shFree(yIntBoot);
   shFree(yIntErrBoot);
   shFree(slopeErrIF);
   shFree(slopeErrBootIF);

   if (returnFlag != 0) { return(TCL_ERROR); }
   return(TCL_OK);

}

/***************************************************************************/
/*
 * Declare tcl verbs
 */

static char *facil = "astrotools";

void atTclLinearFitsDeclare(Tcl_Interp *interp) {

  int flags = FTCL_ARGV_NO_LEFTOVERS;
 
  shTclDeclare ( interp, g_vLinEqn,
                 (Tcl_CmdProc *)tclVLinEqn,
                 (ClientData) 0, (Tcl_CmdDeleteProc *) NULL, facil,
                 shTclGetArgInfo(interp, vLinEqnArgTable, flags, g_vLinEqn),
                 shTclGetUsage(interp, vLinEqnArgTable, flags, g_vLinEqn)
               );
           
  shTclDeclare(interp,"vLinFitBces", (Tcl_CmdProc *)tclVLinFitBces,
	       (ClientData) 0, (Tcl_CmdDeleteProc *)NULL, module, 
	       shTclGetArgInfo(interp, vLinFitBcesTable, 0, vLinFitBces_name),
               shTclGetUsage(interp, vLinFitBcesTable, 0, vLinFitBces_name)
		);
}
