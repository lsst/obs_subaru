/* <AUTO>
   FILE:
                tclVecChain.c
<HTML>
Interface between vectors and chains and regions, etc.
</HTML>
</AUTO> */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <math.h>
#include "shTclParseArgv.h"
#include "shCGarbage.h"
#include "region.h"
#include "shCSao.h"
#include "shCDiskio.h"
#include "shCUtils.h"
#include "shCErrStack.h"
#include "shTclErrStack.h"
#include "shTclHandle.h"
#include "shTclUtils.h"
#include "shCVecChain.h"

#define isgood(x)   ( ( ((x) < FLT_MAX)) && ((x) > -FLT_MAX) ? (1) : (0))
/* extern HG *mnHg; */


static char *tclFacil = "shVector";	/* name for this set of code */

/****************************************************************************
**
** ROUTINE: tclVFromChain
**
**<AUTO EXTRACT>
** TCL VERB: vFromChain
**
**</AUTO>
*/
char g_vFromChain[] = "vFromChain";
ftclArgvInfo g_vFromChainTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Return a vector of the values of the element of a chain\nRETURN:  handle to the vector\n" },
  {"<chain>", FTCL_ARGV_STRING, NULL, NULL, 
   "The chain "},
  {"<element>", FTCL_ARGV_STRING, NULL, NULL, 
   "The element"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVFromChain(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_chain = NULL;
  CHAIN *chain=NULL;
  char *a_element = NULL;
  VECTOR *vector;
  char vectorName[HANDLE_NAMELEN];

  g_vFromChainTbl[1].dst = &a_chain;
  g_vFromChainTbl[2].dst = &a_element;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_vFromChainTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_vFromChain)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName
      (interp, a_chain, (void **) &chain, "CHAIN") != TCL_OK) {
    Tcl_SetResult(interp, "Bad CHAIN handle", TCL_VOLATILE);
    return TCL_ERROR;
  }

  vector = shVFromChain(chain, a_element);
  if (vector == NULL) {
    Tcl_SetResult(interp, "Bad CHAIN element",TCL_VOLATILE);
    return(TCL_ERROR);
  }
  if (shTclHandleNew(interp, vectorName, "VECTOR", vector) != TCL_OK ) {
    Tcl_SetResult(interp, "Can't bind to new VECTOR handle",TCL_VOLATILE);
    return(TCL_ERROR);
  }
  Tcl_SetResult(interp, vectorName, TCL_VOLATILE);
  return(TCL_OK);

}

/*****************************************************************************/
/*
 * Convert a chain to a set of vectors
 */
static char g_vectorsFromChain[] = "vectorsFromChain";
static ftclArgvInfo g_vectorsFromChainTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Return an array of handles to vectors, one for each requested member of\n"
     "the chain's schema; each vector is of the same length as the chain" },
  {"<chain>", FTCL_ARGV_STRING, NULL, NULL, "The chain "},
  {"<elements>", FTCL_ARGV_STRING, NULL, NULL, "List of desired elements"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVectorsFromChain(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   char *chainStr = NULL;
   CHAIN *chain = NULL;
   char *elementsStr = NULL;		/* input string, specifying tcl list */
   char **elements;			/* elements in elementsStr */
   char handle[HANDLE_NAMELEN];
   int i;
   int nel = 0;				/* number of elements in elementsStr */
   VECTOR **vectors;			/* vectors extracted from the chain */
   
   g_vectorsFromChainTbl[1].dst = &chainStr;
   g_vectorsFromChainTbl[2].dst = &elementsStr;
   
   if((i = shTclParseArgv(interp, &argc, argv, g_vectorsFromChainTbl,
			       FTCL_ARGV_NO_LEFTOVERS, g_vectorsFromChain)) !=
							   FTCL_ARGV_SUCCESS) {
      return(i);
   }
/*
 * Get the chain pointer
 */
   if(shTclAddrGetFromName(interp, chainStr, (void **)&chain, "CHAIN")
								   != TCL_OK) {
      Tcl_SetResult(interp, "Bad CHAIN handle", TCL_VOLATILE);
      return(TCL_ERROR);
   }
/*
 * Unpack the list of elements
 */
   if(Tcl_SplitList(interp, elementsStr, &nel, &elements) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * Do the work
 */
   vectors = shVectorsFromChain(chain, nel, elements);

   free(elements); elements = NULL;

   if (vectors == NULL) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * Pack up the results as list of handles
 */
   for(i = 0; i < nel; i++) {
      if (shTclHandleNew(interp, handle, "VECTOR", vectors[i]) != TCL_OK ){
	 Tcl_SetResult(interp, "Can't bind to new VECTOR handle",TCL_VOLATILE);

	 for(; i >= 0; i--) {
	    shVectorDel(vectors[i]);
	 }
	 shFree(vectors);

	 return(TCL_ERROR);
      }

      Tcl_AppendElement(interp, handle);
   }

   shFree(vectors);

   return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclVPlot
**
**<AUTO EXTRACT>
** TCL VERB: vPlot
**
**</AUTO>
*/
char g_vPlot[] = "vPlot";
ftclArgvInfo g_vPlotTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Plot two vectors \n" },
  {"<pgstate>", FTCL_ARGV_STRING, NULL, NULL, 
   "pgstate to write on"},
  {"<vectorX>", FTCL_ARGV_STRING, NULL, NULL, 
   "X vector"},
  {"-vectorXErr", FTCL_ARGV_STRING, NULL, NULL, 
   "error on X"},
  {"<vectorY>", FTCL_ARGV_STRING, NULL, NULL, 
   "Y vector"},
  {"-vectorYErr", FTCL_ARGV_STRING, NULL, NULL, 
   "error on Y"},
  {"-vectorMask", FTCL_ARGV_STRING, NULL, NULL, 
   "mask values, set to 1 for good points"},
  {"-vectorSymbol", FTCL_ARGV_STRING, NULL, NULL, 
   "PGPLOT symbols"},
  {"-vectorColor", FTCL_ARGV_STRING, NULL, NULL, 
   "PGPLOT colors"},
  {"-xmin", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "minimum x of the plot"},
  {"-xmax", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "maximum x of the plot"},
  {"-ymin", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "minimum y of the plot"},
  {"-ymax", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "maximum y of the plot"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVPlot(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_pgstate = NULL;
  PGSTATE *pgstate = NULL;
  char *a_vectorX = NULL;
  VECTOR *vectorX = NULL;
  char *a_vectorXErr = NULL;
  VECTOR *vectorXErr = NULL;
  char *a_vectorY = NULL;
  VECTOR *vectorY = NULL;
  char *a_vectorYErr = NULL;
  VECTOR *vectorYErr = NULL;
  char *a_vectorMask = NULL;
  VECTOR *vectorMask = NULL;
  char *a_vectorSymbol = NULL;
  VECTOR *vectorSymbol = NULL;
  char *a_vectorColor = NULL;
  VECTOR *vectorColor = NULL;
  double a_xmin = FLT_MAX;
  double a_xmax = FLT_MAX;
  double a_ymin = FLT_MAX;
  double a_ymax = FLT_MAX;
  int xyOptMask = 0;

  g_vPlotTbl[1].dst = &a_pgstate;
  g_vPlotTbl[2].dst = &a_vectorX;
  g_vPlotTbl[3].dst = &a_vectorXErr;
  g_vPlotTbl[4].dst = &a_vectorY;
  g_vPlotTbl[5].dst = &a_vectorYErr;
  g_vPlotTbl[6].dst = &a_vectorMask;
  g_vPlotTbl[7].dst = &a_vectorSymbol;
  g_vPlotTbl[8].dst = &a_vectorColor;
  g_vPlotTbl[9].dst = &a_xmin;
  g_vPlotTbl[10].dst = &a_xmax;
  g_vPlotTbl[11].dst = &a_ymin;
  g_vPlotTbl[12].dst = &a_ymax;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_vPlotTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_vPlot)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  if (shTclAddrGetFromName
      (interp, a_pgstate, (void **) &pgstate, "PGSTATE") != TCL_OK) {
    Tcl_SetResult(interp, "Bad PGSTATE handle", TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (shTclAddrGetFromName
      (interp, a_vectorX, (void **) &vectorX, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "Bad VECTOR X handle", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (a_vectorXErr == NULL) {
    vectorXErr = NULL;
  } else {
    if (shTclAddrGetFromName
	(interp, a_vectorXErr, (void **) &vectorXErr, "VECTOR") != TCL_OK) {
      Tcl_SetResult(interp, "Bad VECTOR XErr handle", TCL_VOLATILE);
      return TCL_ERROR;
    }
  }

  if (shTclAddrGetFromName
      (interp, a_vectorY, (void **) &vectorY, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "Bad VECTOR Y handle", TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (a_vectorYErr == NULL) {
    vectorYErr = NULL;
  } else {
    if (shTclAddrGetFromName
	(interp, a_vectorYErr, (void **) &vectorYErr, "VECTOR") != TCL_OK) {
      Tcl_SetResult(interp, "Bad VECTOR YErr handle", TCL_VOLATILE);
      return TCL_ERROR;
    }
  }
  if (a_vectorMask == NULL) {
    vectorMask = NULL;
  } else {
    if (shTclAddrGetFromName
	(interp, a_vectorMask, (void **) &vectorMask, "VECTOR") != TCL_OK) {
      Tcl_SetResult(interp, "Bad VECTOR Mask handle", TCL_VOLATILE);
      return TCL_ERROR;
    }
  }
  if (a_vectorSymbol == NULL) {
    vectorSymbol = NULL;
  } else {
    if (shTclAddrGetFromName
	(interp, a_vectorSymbol, (void **) &vectorSymbol, "VECTOR") != TCL_OK) {
      Tcl_SetResult(interp, "Bad VECTOR Symbol handle", TCL_VOLATILE);
      return TCL_ERROR;
    }
  }
  if (a_vectorColor == NULL) {
    vectorColor = NULL;
  } else {
    if (shTclAddrGetFromName
	(interp, a_vectorColor, (void **) &vectorColor, "VECTOR") != TCL_OK) {
      Tcl_SetResult(interp, "Bad VECTOR Color handle", TCL_VOLATILE);
      return TCL_ERROR;
    }
  }
    /* See which of the minimum and maximum options have been user supplied */
    if (isgood(a_xmin))
      xyOptMask = xyOptMask | XMINOPT;
    if (isgood(a_xmax))
      xyOptMask = xyOptMask | XMAXOPT;
    if (isgood(a_ymin))
      xyOptMask = xyOptMask | YMINOPT;
    if (isgood(a_ymax))
      xyOptMask = xyOptMask | YMAXOPT;

  shVPlot(pgstate, vectorX, vectorXErr, vectorY, vectorYErr,
	  vectorMask, vectorSymbol, vectorColor, 
	  a_xmin, a_xmax, a_ymin, a_ymax, xyOptMask);

  return TCL_OK;

}

/****************************************************************************
**
** ROUTINE: tclVExtreme
**
**<AUTO EXTRACT>
** TCL VERB: vExtreme
**
**</AUTO>
*/
char g_vExtreme[] = "vExtreme";
ftclArgvInfo g_vExtremeTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
     "Return the extreme of the vector.  \n"
     "Set minORmax to either min or max."},
  {"<vector>", FTCL_ARGV_STRING, NULL, NULL, 
   "The vector"},
  {"<minORmax>", FTCL_ARGV_STRING, NULL, NULL, 
   "Return minimum or maximum?"},
  {"-vMask", FTCL_ARGV_STRING, NULL, NULL, 
   "The mask"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVExtreme(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_vector = NULL;
  char *a_minORmax = NULL;
  char *a_vMask = NULL;
  VECTOR *vector = NULL;
  VECTOR *vMask = NULL;
  char answer[40];
  VECTOR_TYPE extreme;

  g_vExtremeTbl[1].dst = &a_vector;
  g_vExtremeTbl[2].dst = &a_minORmax;
  g_vExtremeTbl[3].dst = &a_vMask;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_vExtremeTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_vExtreme)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName
      (interp, a_vector, (void **) &vector, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVExtreme:  Bad VECTOR handle", TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (a_vMask == NULL) {
    vMask = NULL;
  } else {
    if (shTclAddrGetFromName
	(interp, a_vMask, (void **) &vMask, "VECTOR") != TCL_OK) {
      Tcl_SetResult(interp, "tclVExtreme:  Bad vMask handle", TCL_VOLATILE);
      return TCL_ERROR;
    }
  }
  extreme = shVExtreme(vector, vMask, a_minORmax);
  sprintf(answer,"%.7g", extreme);
  /*attach a decimal point if there is none/jester@fnal.gov*/
  /*Only this might break things if people expect integers, so we'll take it out again/jester@fnal.gov*/
/*  if ((strstr(answer,".") == NULL) && (strstr(answer,"e") == NULL)) {
    strcat(answer,".");
    }
*/ 
  Tcl_SetResult(interp, answer, TCL_VOLATILE);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclVLimit
**
**<AUTO EXTRACT>
** TCL VERB: vLimit
**
**</AUTO>
*/
char g_vLimit[] = "vLimit";
ftclArgvInfo g_vLimitTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
     "Return the limit of the vector \n"
     "(extreme and 10 percent buffer).\n"
     "Set minORmax to either min or max."},
  {"<vector>", FTCL_ARGV_STRING, NULL, NULL, 
   "The vector"},
  {"<minORmax>", FTCL_ARGV_STRING, NULL, NULL, 
   "Return minimum or maximum?"},
  {"-vMask", FTCL_ARGV_STRING, NULL, NULL, 
   "The mask"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVLimit(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_vector = NULL;
  char *a_minORmax = NULL;
  char *a_vMask = NULL;
  VECTOR *vector = NULL;
  VECTOR *vMask = NULL;
  char answer[40];
  VECTOR_TYPE limit;

  g_vLimitTbl[1].dst = &a_vector;
  g_vLimitTbl[2].dst = &a_minORmax;
  g_vLimitTbl[3].dst = &a_vMask;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_vLimitTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_vLimit)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName
      (interp, a_vector, (void **) &vector, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVLimit:  Bad VECTOR handle", TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (a_vMask == NULL) {
    vMask = NULL;
  } else {
    if (shTclAddrGetFromName
	(interp, a_vMask, (void **) &vMask, "VECTOR") != TCL_OK) {
      Tcl_SetResult(interp, "tclVLimit:  Bad vMask handle", TCL_VOLATILE);
      return TCL_ERROR;
    }
  }
  limit = shVLimit(vector, vMask, a_minORmax);
  sprintf(answer,"%.7g", limit);
  Tcl_SetResult(interp, answer, TCL_VOLATILE);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclVNameSet
**
**<AUTO EXTRACT>
** TCL VERB: vNameSet
**
**</AUTO>
*/
char g_vNameSet[] = "vNameSet";
ftclArgvInfo g_vNameSetTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Set the name of an vector.\n" },
  {"<vector>", FTCL_ARGV_STRING, NULL, NULL, 
   "VECTOR to name"},
  {"<name>", FTCL_ARGV_STRING, NULL, NULL, 
   "Name to give the VECTOR"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVNameSet(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_vector = NULL;
  char *a_name = NULL;
  VECTOR *vector=NULL;

  g_vNameSetTbl[1].dst = &a_vector;
  g_vNameSetTbl[2].dst = &a_name;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_vNameSetTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_vNameSet)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName
      (interp, a_vector, (void **) &vector, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVNameSet:  bad VECTOR name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shVNameSet(vector, a_name) != SH_SUCCESS) {
    Tcl_SetResult(interp, "tclVNameSet:  can not set name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  Tcl_SetResult(interp, a_name, TCL_VOLATILE);
  return TCL_OK;
}


/****************************************************************************
**
** ROUTINE: tclVNameGet
**
**<AUTO EXTRACT>
** TCL VERB: vNameGet
**
**</AUTO>
*/
char g_vNameGet[] = "vNameGet";
ftclArgvInfo g_vNameGetTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Get the name of a vector.\n" },
  {"<vector>", FTCL_ARGV_STRING, NULL, NULL, 
   "VECTOR to get name from"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVNameGet(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_vector = NULL;
  VECTOR *vector=NULL;

  g_vNameGetTbl[1].dst = &a_vector;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_vNameGetTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_vNameGet)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName
      (interp, a_vector, (void **) &vector, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVNameGet:  bad VECTOR name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  Tcl_SetResult(interp, shVNameGet(vector), TCL_VOLATILE);
  return TCL_OK;
}


/****************************************************************************
**
** ROUTINE: tclVMean
**
**<AUTO EXTRACT>
** TCL VERB: vMean
**
**</AUTO>
*/
char g_vMean[] = "vMean";
ftclArgvInfo g_vMeanTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Calculate the mean of the values of a VECTOR.\n" },
  {"<vector>", FTCL_ARGV_STRING, NULL, NULL, 
   "VECTOR to print MEAN of"},
  {"-vMask", FTCL_ARGV_STRING, NULL, NULL, 
   "The mask"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVMean(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_vector = NULL;
  char *a_vMask = NULL;
  VECTOR *vector=NULL;
  VECTOR *vMask=NULL;
  char answer[40];
  VECTOR_TYPE value = 0.0;
  
  g_vMeanTbl[1].dst = &a_vector;
  g_vMeanTbl[2].dst = &a_vMask;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_vMeanTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_vMean)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName
      (interp, a_vector, (void **) &vector, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVMean:  bad VECTOR name", TCL_VOLATILE);
    
    return TCL_ERROR;
  }
  if (a_vMask == NULL) {
    vMask = NULL;
  } else {
    if (shTclAddrGetFromName
	(interp, a_vMask, (void **) &vMask, "VECTOR") != TCL_OK) {
      Tcl_SetResult(interp, "tclVExtreme:  Bad vMask handle", TCL_VOLATILE);
      return TCL_ERROR;
    }
  }

  /*Tcl_SetResult(interp,"",0);
  sprintf(answer, "%f", shHgMean(hg));
  Tcl_AppendElement(interp, answer);
  */

  if(shVStatistics(vector, vMask, "mean", &value)!=SH_SUCCESS)
  {
     shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
  }
  sprintf(answer, "%.7g", value);
  Tcl_AppendResult(interp, answer, (char*) 0);
  
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclVSigma
**
**<AUTO EXTRACT>
** TCL VERB: vSigma
**
**</AUTO>
*/
char g_vSigma[] = "vSigma";
ftclArgvInfo g_vSigmaTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Calculate the sigma of the values of a VECTOR.\n" },
  {"<vector>", FTCL_ARGV_STRING, NULL, NULL, 
   "VECTOR to print SIGMA of"},
  {"-vMask", FTCL_ARGV_STRING, NULL, NULL, 
   "The mask"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVSigma(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_vector = NULL;
  char *a_vMask = NULL;
  VECTOR *vector=NULL;
  VECTOR *vMask=NULL;
  char answer[40];
  VECTOR_TYPE value = 0.0;
  
  g_vSigmaTbl[1].dst = &a_vector;
  g_vSigmaTbl[2].dst = &a_vMask;


  if ((rstat = shTclParseArgv(interp, &argc, argv, g_vSigmaTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_vSigma)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName
      (interp, a_vector, (void **) &vector, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVSigma:  bad VECTOR name", TCL_VOLATILE);
    
    return TCL_ERROR;
  }
  if (a_vMask == NULL) {
    vMask = NULL;
  } else {
    if (shTclAddrGetFromName
	(interp, a_vMask, (void **) &vMask, "VECTOR") != TCL_OK) {
      Tcl_SetResult(interp, "tclVExtreme:  Bad vMask handle", TCL_VOLATILE);
      return TCL_ERROR;
    }
  }
  
  /*Tcl_SetResult(interp,"",0);
  sprintf(answer, "%f", shHgSigma(hg));
  Tcl_AppendElement(interp, answer);
  */

  if(shVStatistics(vector, vMask, "sigma", &value)!=SH_SUCCESS)
  {
     shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
  }
  sprintf(answer, "%.7g", value);
  Tcl_AppendResult(interp, answer, (char*) 0);
  
  return TCL_OK;
}




/****************************************************************************
**
** ROUTINE: tclVMedian
**
**<AUTO EXTRACT>
** TCL VERB: vMedian
**
**</AUTO>
*/
char g_vMedian[] = "vMedian";
ftclArgvInfo g_vMedianTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Return the median of the good values of the VECTOR.\n" },
  {"<vector>", FTCL_ARGV_STRING, NULL, NULL, 
   "VECTOR to median"},
  {"-vMask", FTCL_ARGV_STRING, NULL, NULL, 
   "Corresponding mask values"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVMedian(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_vector = NULL;
  VECTOR *vector = NULL;
  char *a_vMask = NULL;
  VECTOR *vMask = NULL;

  char answer[40];
  VECTOR_TYPE median;

  g_vMedianTbl[1].dst = &a_vector;
  g_vMedianTbl[2].dst = &a_vMask;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_vMedianTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_vMedian)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName
      (interp, a_vector, (void **) &vector, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVMedian:  bad VECTOR name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (a_vMask == NULL) {
    vMask = NULL;
  } else {
    
    if (shTclAddrGetFromName
	(interp, a_vMask, (void **) &vMask, "VECTOR") != TCL_OK) {
      Tcl_SetResult(interp, "tclVMedian:  bad vMask name", TCL_VOLATILE);
      return TCL_ERROR;
    }
  }
  median = shVMedian(vector, vMask);
  sprintf(answer,"%.7g", median);
  Tcl_SetResult(interp,answer,TCL_VOLATILE);
  return TCL_OK;
}


/****************************************************************************
**
** ROUTINE: tclVSigmaClip
**
**<AUTO EXTRACT>
** TCL VERB: vSigmaClip
**
**</AUTO>
*/
/* Take the average of a VECTOR*/
char g_vSigmaClip[] = "vSigmaClip";
ftclArgvInfo g_vSigmaClipTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Return a keyed list of the mean and sqrtVar (== sigma)\n"
   "of the values in the vector that are not masked by vMask,\n"
   "rejecting those values more than sigmaClip*sigma\n"
   "from the mean for nIter iterations.\n" },
  {"<vector>", FTCL_ARGV_STRING, NULL, NULL, 
   "Initial VECTOR"},
  {"-sigmaClip", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "Clip value"},
  {"-nIter", FTCL_ARGV_INT, NULL, NULL, 
   "Number of iterations"},
  {"-vMask", FTCL_ARGV_STRING, NULL, NULL, 
   "Vector mask"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int 
tclVSigmaClip(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstatus;
  char *vectorNamePtr= NULL;
  char *vMaskNamePtr = NULL;
  VECTOR *vector=NULL, *vMask=NULL;
  VECTOR_TYPE sigmaClip = 5.0;
  int nIter = 3;
  VECTOR_TYPE mean, sqrtVar;
  int flags = FTCL_ARGV_NO_LEFTOVERS;

  char answer[40];
  
  g_vSigmaClipTbl[1].dst = &vectorNamePtr;
  g_vSigmaClipTbl[2].dst = &sigmaClip;
  g_vSigmaClipTbl[3].dst = &nIter;
  g_vSigmaClipTbl[4].dst = &vMaskNamePtr;

  if ((rstatus = shTclParseArgv(interp, &argc, argv, g_vSigmaClipTbl,
				flags, g_vSigmaClip))
      !=FTCL_ARGV_SUCCESS) {
    return(rstatus);
  }    
  if (shTclAddrGetFromName
      (interp, vectorNamePtr, (void **) &vector, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVSigmaClip:  bad VECTOR name", TCL_VOLATILE);
    
    return TCL_ERROR;
  }
  
  
  if (vMaskNamePtr==NULL || strcmp(vMaskNamePtr, "none") == 0) {
    vMask = NULL;
  } else {
    if (shTclAddrGetFromName
	(interp, vMaskNamePtr, (void **) &vMask, "VECTOR") != TCL_OK) {
      Tcl_SetResult(interp, "tclVSigmaClip:  bad VECTOR mask name", TCL_VOLATILE);
      
      return TCL_ERROR;
    }
  }
  
  
  /* Chris S. -- in response to PR 2761, do the error handling more carefully */
  if (shVSigmaClip(vector, vMask, sigmaClip, nIter, &mean, &sqrtVar) != SH_SUCCESS) {
    shTclInterpAppendWithErrStack(interp);
    return TCL_ERROR;
  }
  
  /* send the answer back to tcl */
  Tcl_SetResult (interp, "", TCL_VOLATILE);
  sprintf(answer, "mean %.7g", mean);
  Tcl_AppendElement(interp, answer);
  sprintf(answer, "sqrtVar %.7g", sqrtVar);
  Tcl_AppendElement(interp, answer);
  return TCL_OK;
}
/****************************************************************************
**
** ROUTINE: tclVToChain
**
**<AUTO EXTRACT>
** TCL VERB: vToChain
**
**</AUTO>
*/
static ftclArgvInfo vToChainArgTable[] = {
      {NULL,            FTCL_ARGV_HELP,         NULL, NULL,
       "Put a vector in the values of member on the chain\n"},
      {"<vector>",          FTCL_ARGV_STRING,       NULL, NULL, 
	 "Vector to use"},
      {"<chain>",        FTCL_ARGV_STRING,       NULL, NULL,
	 "Chain to fill"},
      {"<member>",      FTCL_ARGV_STRING,       NULL, NULL, 
	 "Member of chain to hold vector"},
      {(char *) NULL,    FTCL_ARGV_END,   NULL, NULL, (char *) NULL}
 };

char g_vToChain[] = "vToChain";

static int 
tclVToChain(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstatus;
  VECTOR *vector=NULL;
  CHAIN *chain=NULL;
  char *vectorNamePtr=NULL;
  char *chainNamePtr=NULL;
  char *memberPtr = NULL;
  int flags = FTCL_ARGV_NO_LEFTOVERS;
 
  vToChainArgTable[1].dst = &vectorNamePtr;
  vToChainArgTable[2].dst = &chainNamePtr;
  vToChainArgTable[3].dst = &memberPtr;

 
  if ((rstatus = shTclParseArgv(interp, &argc, argv, 
				vToChainArgTable, flags, g_vToChain))
       != FTCL_ARGV_SUCCESS) {
    return (rstatus);
  }
    
  if (shTclAddrGetFromName
      (interp, chainNamePtr, (void **) &chain, "CHAIN") != TCL_OK) {
    Tcl_SetResult(interp, shTclGetUsage(interp, vToChainArgTable, flags,
					g_vToChain), TCL_VOLATILE);
    return(TCL_ERROR);
  }
  
  if (shTclAddrGetFromName
      (interp, vectorNamePtr, (void **) &vector, "VECTOR") != TCL_OK) {
    
    Tcl_SetResult(interp, shTclGetUsage(interp, vToChainArgTable, flags,
					g_vToChain), TCL_VOLATILE);
    return(TCL_ERROR);
  }

  chain = shVToChain(vector, chain, memberPtr);

  if (chain == NULL) 
  {
    shTclInterpAppendWithErrStack(interp);
    return(TCL_ERROR);
  }
  
  Tcl_SetResult(interp, chainNamePtr, TCL_VOLATILE);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclHgNewFromV
**
**<AUTO EXTRACT>
** TCL VERB: hgNewFromV
**
**</AUTO>
*/
/* Create a new HG from an VECTOR */

static ftclArgvInfo hgNewFromVArgTable[] = {
      {NULL,    FTCL_ARGV_HELP,         NULL, NULL, 
       "Create a new HG and fill it with the values in vector. \n"
       "Set min>max for auto scaling.  Use only the points in \n"
       "vMask that are set to 1.\n"},
      {"<vector>",  FTCL_ARGV_STRING,	NULL, NULL, 
	 "Vector to fill hg from"},
      {"-name", FTCL_ARGV_STRING,	NULL, NULL, 
	 "The name for the HG"},
      {"-min",  FTCL_ARGV_DOUBLE,	NULL, NULL, 
	 "Low edge of first bin"},
      {"-max",  FTCL_ARGV_DOUBLE,	NULL, NULL, 
	 "High edge of last bin"},
      {"-nbin", FTCL_ARGV_INT  ,	NULL, NULL,
	 "number of bins, default 20"},
      {"-vMask", FTCL_ARGV_STRING,	NULL, NULL, NULL},
      {"-vWeight", FTCL_ARGV_STRING,	NULL, NULL, NULL},
      {NULL,    FTCL_ARGV_END,	  	NULL, NULL, NULL}
 };

char g_hgNewFromV[] = "hgNewFromV";


      
static int tclHgNewFromV
(ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
  int rstatus;
  char *vectorNamePtr = NULL;
  char *vMaskNamePtr = NULL;
  char *vWeightNamePtr = NULL;
  char hgName[HANDLE_NAMELEN];
  VECTOR *vector=NULL, *vMask=NULL, *vWeight=NULL;
  HG *hg=NULL;
  VECTOR_TYPE min = 1.0, max = 0.0;
  int nbin = 20;
  char *a_name = NULL;
  int flags = FTCL_ARGV_NO_LEFTOVERS;


  hgNewFromVArgTable[1].dst = &vectorNamePtr;
  hgNewFromVArgTable[2].dst = &a_name;
  hgNewFromVArgTable[3].dst = &min;
  hgNewFromVArgTable[4].dst = &max;
  hgNewFromVArgTable[5].dst = &nbin;
  hgNewFromVArgTable[6].dst = &vMaskNamePtr;
  hgNewFromVArgTable[7].dst = &vWeightNamePtr;
  

  if ((rstatus = shTclParseArgv(interp, &argc, argv, hgNewFromVArgTable, flags,
				g_hgNewFromV))
      !=FTCL_ARGV_SUCCESS) {
    return(rstatus);
  }
  if (shTclAddrGetFromName
      (interp, vectorNamePtr, (void **) &vector, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclHgNewFromV:  bad VECTOR name", TCL_VOLATILE);
    
    return TCL_ERROR;
  }
  
  if (vMaskNamePtr==NULL || strcmp(vMaskNamePtr,"none") == 0) {
    vMask = NULL;
  } else {
    if (shTclAddrGetFromName
	(interp, vMaskNamePtr, (void **) &vMask, "VECTOR") != TCL_OK) {
      Tcl_SetResult(interp, "tclHgNewFromV:  Bad VECTOR handle", TCL_VOLATILE);
      return TCL_ERROR;
    }
  }

  if (vWeightNamePtr==NULL || strcmp(vWeightNamePtr,"none") == 0) {
    vWeight = NULL;
  } else {
    if (shTclAddrGetFromName
	(interp, vWeightNamePtr, (void **) &vWeight, "VECTOR") != TCL_OK) {
      Tcl_SetResult(interp, "tclHgNewFromV:  Bad VECTOR handle", TCL_VOLATILE);
      return TCL_ERROR;
    }
  }

  /* do the call */

  hg = shHgNewFromV(vector, vMask, vWeight, nbin, min, max, a_name);

  if (shTclHandleNew(interp, hgName, "HG", hg) != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to new hg handle", TCL_VOLATILE);
    return(TCL_ERROR);
  }
  
  Tcl_SetResult(interp, hgName, TCL_VOLATILE);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclHgFillFromV
**
**<AUTO EXTRACT>
** TCL VERB: hgFillFromV
**
**</AUTO>
*/
/* Create a new HG from an VECTOR */
#define tclHgFillFromV_help "Fill and existing HG with the values in vector.  Use only the points in vMask that are set to 1.\n"

static ftclArgvInfo hgFillFromVArgTable[] = {
   {NULL,   FTCL_ARGV_HELP,   NULL, NULL, tclHgFillFromV_help},
   {"<hg>", FTCL_ARGV_STRING, NULL, NULL, "HG to fill"},
   {"<vector>", FTCL_ARGV_STRING, NULL, NULL, "VECTOR to use for filling the HG"},
   {"-vMask", FTCL_ARGV_STRING, NULL, NULL, NULL},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
 };

char g_hgFillFromV[] = "hgFillFromV";
   
static int tclHgFillFromV
(ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
  int rstatus;
  char *vectorNamePtr = NULL;
  char *vMaskNamePtr = NULL;
  char *a_hg = NULL;
  VECTOR *vector=NULL, *vMask=NULL;
  HG *hg=NULL;
  int flags = FTCL_ARGV_NO_LEFTOVERS;

  hgFillFromVArgTable[1].dst = &a_hg;
  hgFillFromVArgTable[2].dst = &vectorNamePtr;
  hgFillFromVArgTable[3].dst = &vMaskNamePtr;
  
  
  if ((rstatus = shTclParseArgv(interp, &argc, argv, hgFillFromVArgTable,
				flags, g_hgFillFromV))
      ==FTCL_ARGV_SUCCESS  ) {
    
    if (shTclAddrGetFromName
	(interp, a_hg, (void **) &hg, "HG") != TCL_OK) {
      Tcl_SetResult(interp, "tclHgFillFromV:  bad HG name", TCL_VOLATILE);
      
      return TCL_ERROR;
    }
    
    if (shTclAddrGetFromName
	(interp, vectorNamePtr, (void **) &vector, "VECTOR") != TCL_OK) {
      Tcl_SetResult(interp, "tclHgFillFromV:  bad VECTOR name", TCL_VOLATILE);
      
      return TCL_ERROR;
    }
    
    if (vMaskNamePtr==NULL || strcmp(vMaskNamePtr,"none") == 0) {
      vMask = NULL;
    } else {
      if (shTclAddrGetFromName
	  (interp, vMaskNamePtr, (void **) &vMask, "VECTOR") != TCL_OK) {
	Tcl_SetResult(interp, "tclHgFillFromV:  bad vMask name", TCL_VOLATILE);
	
	return TCL_ERROR;
      }
    }
  } else {
    return(rstatus);
  }

  shHgFillFromV(hg, vector, vMask);

  Tcl_SetResult(interp, a_hg, TCL_VOLATILE);
  return TCL_OK;
}

/***************************************************************************/
/*
 * Collapse some rectangle in a region into a VECTOR
 */
static ftclArgvInfo vectorGetFromRegionTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL},
  {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "region to process"},
  {"<row0>", FTCL_ARGV_INT, NULL, NULL, "starting row"},
  {"<row1>", FTCL_ARGV_INT, NULL, NULL, "ending row"},
  {"<col0>", FTCL_ARGV_INT, NULL, NULL, "starting column"},
  {"<col1>", FTCL_ARGV_INT, NULL, NULL, "ending column"},
  {"<dirn>", FTCL_ARGV_INT, NULL, NULL,
			  "which direction? (0 => horizontal, 1 => vertical)"},
  {"-median", FTCL_ARGV_CONSTANT, (void *)1, NULL,
					"use a median, not a mean (U16 only)"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

#define vectorGetFromRegion_name "vectorGetFromRegion"

static int
tclVectorGetFromRegion(ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   int dirn;
   int row0, row1, col0, col1;
   char *regName;
   REGION *reg;
   int use_med = 0;
   VECTOR *vec;				/* the desired result */
/*
 * Parse the command
 */
   vectorGetFromRegionTable[1].dst = &regName;
   vectorGetFromRegionTable[2].dst = &row0;
   vectorGetFromRegionTable[3].dst = &row1;
   vectorGetFromRegionTable[4].dst = &col0;
   vectorGetFromRegionTable[5].dst = &col1;
   vectorGetFromRegionTable[6].dst = &dirn;
   vectorGetFromRegionTable[7].dst = &use_med;
   
   if(shTclParseArgv(interp, &argc, argv, vectorGetFromRegionTable,
		     0, vectorGetFromRegion_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * process and check those arguments
 */
   if(shTclHandleExprEval(interp,regName,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"quartilesToRegions: "
                    "first argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(use_med && reg->type != TYPE_U16) {
      Tcl_SetResult(interp,"quartilesToRegions: "
                    "You can only use -median with U16 regions",TCL_STATIC);
   }
/*
 * Finally do the work
 */
   if((vec = shVectorGetFromRegion(reg, row0,row1, col0,col1, dirn, use_med))
								     == NULL) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = vec;
   hand.type = shTypeGetFromName("VECTOR");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new VECTOR handle",TCL_STATIC);
      shVectorDel(vec);
      return(TCL_ERROR);
   }
   
   Tcl_SetResult(interp, name, TCL_VOLATILE);
   
   return TCL_OK;
}

/***************************************************************************/
/*
 * Put the contents of a vector into a region
 */
static ftclArgvInfo vectorSetInRegionTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL},
  {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "region in which to insert vector"},
  {"<vec>", FTCL_ARGV_STRING, NULL, NULL, "vector to insert"},
  {"<row>", FTCL_ARGV_INT, NULL, NULL, "starting row"},
  {"<col>", FTCL_ARGV_INT, NULL, NULL, "starting column"},
  {"<dirn>", FTCL_ARGV_INT, NULL, NULL,
			  "which direction? (0 => place row, 1 => place col)"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

#define vectorSetInRegion_name "vectorSetInRegion"

static int
tclVectorSetInRegion(ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand, vhand;
   void *vptr;				/* used by shTclHandleExprEval */
   int dirn;
   int row, col;
   char *regName;
   char *vecName;
   REGION *reg;
   VECTOR *vec;	
/*
 * Parse the command
 */
   vectorSetInRegionTable[1].dst = &regName;
   vectorSetInRegionTable[2].dst = &vecName;
   vectorSetInRegionTable[3].dst = &row;
   vectorSetInRegionTable[4].dst = &col;
   vectorSetInRegionTable[5].dst = &dirn;
   
   if(shTclParseArgv(interp, &argc, argv, vectorSetInRegionTable,
		     0, vectorSetInRegion_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * process and check those arguments
 */
   if(shTclHandleExprEval(interp,regName,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"vectorSetInRegion: "
                    "first argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;
   if(shTclHandleExprEval(interp,vecName,&vhand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(vhand.type != shTypeGetFromName("VECTOR")) {
      Tcl_SetResult(interp,"vectorSetInRegion: "
                    "second argument is not a VECTOR",TCL_STATIC);
      return(TCL_ERROR);
   }
   vec = vhand.ptr;

   if (dirn) {
	   	if (reg->nrow != vec->dimen) { /* replace a col */
      		Tcl_SetResult(interp,"vectorSetInRegion: "
                    "vector and region must be same size",TCL_STATIC);
      		return(TCL_ERROR);
		}
	} else {
	   	if (reg->ncol != vec->dimen) { /* replace a row */
      		Tcl_SetResult(interp,"vectorSetInRegion: "
                    "vector and region must be same size",TCL_STATIC);
      		return(TCL_ERROR);
		}
	}

/*
 * Finally do the work
 */
   if((shVectorSetInRegion(reg, vec, row, col, dirn))
								     == NULL) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   
   return TCL_OK;
}

/**********************************************************************/
/* Declare new tcl verbs for VECTOR-chain functions*/
void shTclVectorChainDeclare(Tcl_Interp *interp) {

  int flags = FTCL_ARGV_NO_LEFTOVERS;

  shTclDeclare(interp, g_vFromChain, 
               (Tcl_CmdProc *) tclVFromChain,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_vFromChainTbl, 
			       flags, g_vFromChain),
               shTclGetUsage(interp, g_vFromChainTbl, 
			     flags, g_vFromChain));

  shTclDeclare(interp, g_vectorsFromChain, 
               (Tcl_CmdProc *) tclVectorsFromChain,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_vectorsFromChainTbl, 
			       flags, g_vectorsFromChain),
               shTclGetUsage(interp, g_vectorsFromChainTbl, 
			     flags, g_vectorsFromChain));

  shTclDeclare(interp, g_vPlot, 
               (Tcl_CmdProc *) tclVPlot,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_vPlotTbl, flags, g_vPlot),
               shTclGetUsage(interp, g_vPlotTbl, flags, g_vPlot));

  shTclDeclare(interp, g_vExtreme, 
               (Tcl_CmdProc *) tclVExtreme,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_vExtremeTbl, 
			       flags, g_vExtreme),
               shTclGetUsage(interp, g_vExtremeTbl, 
			     flags, g_vExtreme));

  shTclDeclare(interp, g_vLimit, 
               (Tcl_CmdProc *) tclVLimit,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_vLimitTbl, 
			       flags, g_vLimit),
               shTclGetUsage(interp, g_vLimitTbl, 
			     flags, g_vLimit));
 
 shTclDeclare(interp, g_vNameSet, 
               (Tcl_CmdProc *) tclVNameSet,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_vNameSetTbl, flags, g_vNameSet),
               shTclGetUsage(interp, g_vNameSetTbl, flags, g_vNameSet));

 shTclDeclare(interp, g_vNameGet, 
               (Tcl_CmdProc *) tclVNameGet,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_vNameGetTbl, flags, g_vNameGet),
               shTclGetUsage(interp, g_vNameGetTbl, flags, g_vNameGet));


  shTclDeclare(interp, g_vMean, 
               (Tcl_CmdProc *) tclVMean,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_vMeanTbl, flags, g_vMean),
               shTclGetUsage(interp, g_vMeanTbl, flags, g_vMean));
  shTclDeclare(interp, g_vSigma, 
               (Tcl_CmdProc *) tclVSigma,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_vSigmaTbl, flags, g_vSigma),
               shTclGetUsage(interp, g_vSigmaTbl, flags, g_vSigma));


  shTclDeclare(interp, g_vMedian, 
               (Tcl_CmdProc *) tclVMedian,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_vMedianTbl, flags, g_vMedian),
               shTclGetUsage(interp, g_vMedianTbl, flags, g_vMedian));

  shTclDeclare(interp, g_vSigmaClip,
	       (Tcl_CmdProc *)tclVSigmaClip,
	       (ClientData) 0,
	       (Tcl_CmdDeleteProc *)NULL,
	       tclFacil,
	       shTclGetArgInfo(interp, g_vSigmaClipTbl, flags,
			       g_vSigmaClip),
	       shTclGetUsage(interp, g_vSigmaClipTbl, flags,
			     g_vSigmaClip));
  shTclDeclare(interp, g_vToChain,
	       (Tcl_CmdProc *)tclVToChain,
	       (ClientData) 0,
	       (Tcl_CmdDeleteProc *)NULL,
	       tclFacil,
	       shTclGetArgInfo(interp, vToChainArgTable, flags,
			       g_vToChain),
	       shTclGetUsage(interp, vToChainArgTable, flags, g_vToChain));

  shTclDeclare(interp, g_hgNewFromV,
	       (Tcl_CmdProc *)tclHgNewFromV,
	       (ClientData) 0,
	       (Tcl_CmdDeleteProc *)NULL,
	       tclFacil,
	       shTclGetArgInfo(interp, hgNewFromVArgTable, flags, g_hgNewFromV),
	       shTclGetUsage(interp, hgNewFromVArgTable, flags, g_hgNewFromV));
  shTclDeclare(interp, g_hgFillFromV,
	       (Tcl_CmdProc *)tclHgFillFromV,
	       (ClientData) 0,
	       (Tcl_CmdDeleteProc *)NULL,
	       tclFacil,
	       shTclGetArgInfo(interp, hgFillFromVArgTable, flags,
			       g_hgFillFromV),
	       shTclGetUsage(interp, hgFillFromVArgTable, flags,
			     g_hgFillFromV));
   shTclDeclare(interp, vectorGetFromRegion_name,
               (Tcl_CmdProc *)tclVectorGetFromRegion,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               tclFacil, shTclGetArgInfo(interp, vectorGetFromRegionTable, 0,
				       vectorGetFromRegion_name),
               shTclGetUsage(interp, vectorGetFromRegionTable, 0,
				vectorGetFromRegion_name));
   shTclDeclare(interp, vectorSetInRegion_name,
               (Tcl_CmdProc *)tclVectorSetInRegion,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               tclFacil, shTclGetArgInfo(interp, vectorSetInRegionTable, 0,
				       vectorSetInRegion_name),
               shTclGetUsage(interp, vectorSetInRegionTable, 0,
				vectorSetInRegion_name));
}







