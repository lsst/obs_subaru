/*
 * TCL support for evaluating vectors
 *
 * ENTRY POINT		SCOPE
 * tclVectorDeclare	public	declare all the verbs defined in this module
 * p_shVectorGet	private. Used by shVectorStringEval to lookup handles
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "shCVecExpr.h"
#include "shCArray.h"
#include "shCHg.h"				/* horrible AF stuff */
#include "tcl.h"
#include "ftcl.h"
#include "cpgplot.h"
#include "shTclHandle.h"
#include "shTclUtils.h" 		/* needed for shTclDeclare prototype */
#include "shTclVerbs.h"
#include "shTclParseArgv.h"  
#include "shTclErrStack.h"
#include "tclExtdInt.h"			/* for Tcl_GetOpenFileStruct */
#include "shCGarbage.h"

static char *module = "shVector";	/* name of this set of code */
static Tcl_Interp *i_interp;		/* global copy of interp */

/*****************************************************************************/
/*
 * Make a new VECTOR of a given size
 */
#define tclExprVectorNew_hlp \
  "Return a handle to a new VECTOR of dimension <dimen>, initialised to 0\n"

static ftclArgvInfo vectorNewAT[] = {
 { NULL, FTCL_ARGV_HELP, NULL, NULL, tclExprVectorNew_hlp },
 { "<dimen>", FTCL_ARGV_INT, NULL, NULL,
     "the dimension of the desired vector" },
 { NULL, FTCL_ARGV_END, NULL, NULL, NULL }
};

char g_vectorNew[] = "vectorExprNew";

static int
tclExprVectorNew(
	    ClientData clientData,
	    Tcl_Interp *interp,
	    int argc,
	    char **argv
	    )
{
   static int dimen;			/* desired dimension */
   int rstatus;
   int flags;
   int i;
   HANDLE handle;
   char handle_n[HANDLE_NAMELEN];	/* name of created handle */
   VECTOR *x;				/* returned value of expression */

   vectorNewAT[1].dst = &dimen;
   
   flags = FTCL_ARGV_NO_LEFTOVERS;
   if((rstatus = shTclParseArgv(interp, &argc, argv, vectorNewAT, flags,
				g_vectorNew)) != FTCL_ARGV_SUCCESS) {
     return(rstatus);
   }
/*
 * Done with arguments.
 *
 * create the VECTOR and bind it to a new handle
 */
   x = shVectorNew(dimen);

   if(p_shTclHandleNew(interp,handle_n) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      shVectorDel(x);
      return(TCL_ERROR);
   }
   handle.ptr = x;
   handle.type = shTypeGetFromName("VECTOR");
   
   if(p_shTclHandleAddrBind(interp,handle,handle_n) != TCL_OK) {
      Tcl_AppendResult(interp,"Can't bind to new handle",handle_n,
		       (char *)NULL);
      shVectorDel(x);
      return(TCL_ERROR);
   }
/*
 * Finally zero the new vector
 */
   for(i = 0;i < x->dimen;i++) {
      x->vec[i] = 0.0;
   }
   
   Tcl_SetResult(interp,handle_n,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete a VECTOR
 */
#define tclExprVectorDel_hlp \
  "Delete a vector <handle_v>\n"

static ftclArgvInfo vectorDelAT[] = {
 { NULL, FTCL_ARGV_HELP, NULL, NULL, tclExprVectorDel_hlp },
 { "<handle>", FTCL_ARGV_STRING, NULL, NULL,
     "The vector to be deleted" },
 { NULL, FTCL_ARGV_END, NULL, NULL, NULL }
};

char g_vectorDel[] = "vectorExprDel";

static int
tclExprVectorDel(
	    ClientData clientData,
	    Tcl_Interp *interp,
	    int argc,
	    char **argv
	    )
{
   static char *handle_n;		/* name of desired handle */
   int rstatus;
   int flags;
   HANDLE handle;
   
   vectorDelAT[1].dst = &handle_n;

   flags = FTCL_ARGV_NO_LEFTOVERS;
   if((rstatus = shTclParseArgv(interp, &argc, argv, vectorDelAT, flags,
				g_vectorDel)) != FTCL_ARGV_SUCCESS) {
     return(rstatus);
   }
/*
 * Done with arguments. First find the handle that we must kill, then
 * destroy it
 */
   if(shTclHandleExprEval(interp, handle_n, &handle, NULL) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(handle.type != shTypeGetFromName("VECTOR")) {
      Tcl_AppendResult(interp, 
		       "vectorExprDel: handle ", handle_n, " is a ", 
		       shNameGetFromType(handle.type), ", not a VECTOR\n",
		       (char *)NULL);
      return(TCL_ERROR);
   }
   shVectorDel((VECTOR*)handle.ptr);

   if (p_shTclHandleDel(interp, handle_n) != TCL_OK)  {
       Tcl_AppendResult(interp, "vectorExprDel: cannot delete handle name ", 
          handle_n, (char *)NULL);
       return TCL_ERROR;
   }
    
   return(TCL_OK);
}

/*****************************************************************************/

#define tclExprVectorEval_hlp \
  "Evaluate an expression, and return a handle to a VECTOR containing the result\n"

static ftclArgvInfo vectorEvalAT[] = {
 { NULL, FTCL_ARGV_HELP, NULL, NULL, tclExprVectorEval_hlp },
 { "<expression>", FTCL_ARGV_STRING, NULL, NULL,
     "The expression to be evaluated" },
 { NULL, FTCL_ARGV_END, NULL, NULL, NULL }
};

char g_vectorEval[] = "vectorExprEval";

static int
tclExprVectorEval(
	    ClientData clientData,
	    Tcl_Interp *interp,
	    int argc,
	    char **argv
	    )
{
   static char *expr;				/* expression to eval */
   int rstatus;
   int flags;
   HANDLE handle;
   char handle_n[HANDLE_NAMELEN];	/* name of desired handle */
   VECTOR *x;				/* returned value of expression */
   
   vectorEvalAT[1].dst = &expr;

   flags = FTCL_ARGV_NO_LEFTOVERS;
   if((rstatus = shTclParseArgv(interp, &argc, argv, vectorEvalAT, flags,
				g_vectorEval)) != FTCL_ARGV_SUCCESS) {
     return(rstatus);
   }
/*
 * Done with arguments
 */
   i_interp = interp;
   x = shVectorStringEval(expr);
   if(x == NULL) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * all went well, so make a handle to return the answer
 */
   if(p_shTclHandleNew(interp,handle_n) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   handle.ptr = x;
   handle.type = shTypeGetFromName("VECTOR");
   
   if(p_shTclHandleAddrBind(interp,handle,handle_n) != TCL_OK) {
      Tcl_AppendResult(interp,"Can't bind to new handle",handle_n,
		       (char *)NULL);
      shVectorDel(x);
      return(TCL_ERROR);
   }
   
   Tcl_SetResult(interp,handle_n,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/

#define tclExprVectorPrint_hlp \
  "Print a vector, given a handle\n"

static ftclArgvInfo vectorPrintAT[] = {
 { NULL, FTCL_ARGV_HELP, NULL, NULL, tclExprVectorPrint_hlp },
 { "<VECTOR>", FTCL_ARGV_STRING, NULL, NULL,
     "Handle to the vector to be printed" },
 { NULL, FTCL_ARGV_END, NULL, NULL, NULL }
};

char g_vectorPrint[] = "vectorExprPrint";

static int
tclExprVectorPrint(
	    ClientData clientData,
	    Tcl_Interp *interp,
	    int argc,
	    char **argv
	    )
{
   static char *handle_n;		/* name of handle, or expression */
   int rstatus;
   int flags;
   HANDLE handle;

   vectorPrintAT[1].dst = &handle_n;
   
   flags = FTCL_ARGV_NO_LEFTOVERS;
   if((rstatus = shTclParseArgv(interp, &argc, argv, vectorPrintAT, flags,
				g_vectorPrint)) != FTCL_ARGV_SUCCESS) {
     return(rstatus);
   }
/*
 * done with arguments
 */
   if(shTclHandleExprEval(interp, handle_n, &handle, NULL) != TCL_OK) {
      return(TCL_ERROR);
   }

   if(handle.type != shTypeGetFromName("VECTOR")) {
      Tcl_AppendResult(interp, 
		       "vectorExprPrint: handle ", handle_n, " is a ", 
		       shNameGetFromType(handle.type), ", not a VECTOR\n",
		       (char *)NULL);
      return(TCL_ERROR);
   }
   shVectorPrint((VECTOR*)handle.ptr);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Evaluate an expression and set a pre-existing VECTOR (passed as a handle)
 */
#define tclExprVectorSet_hlp \
  "Set a preexisting vector <handle_v> to the result of evaluating <expr>\n"

static ftclArgvInfo vectorSetAT[] = {
 { NULL, FTCL_ARGV_HELP, NULL, NULL, tclExprVectorSet_hlp },
 { "<handle>", FTCL_ARGV_STRING, NULL, NULL,
     "handle to the vector to set" },
 { "<expression>", FTCL_ARGV_STRING, NULL, NULL,
     "The expression to be evaluated" },
 { NULL, FTCL_ARGV_END, NULL, NULL, NULL }
};

char g_vectorSet[] = "vectorExprSet";

static int
tclExprVectorSet(
	    ClientData clientData,
	    Tcl_Interp *interp,
	    int argc,
	    char **argv
	    )
{
   static char *expr;			/* expression to eval */
   static char *handle_n;		/* name of desired handle */
   int rstatus;
   int flags;
   HANDLE handle;
   VECTOR *x;				/* returned value of expression */
   
   vectorSetAT[1].dst = &handle_n;
   vectorSetAT[2].dst = &expr;

   flags = FTCL_ARGV_NO_LEFTOVERS;
   if((rstatus = shTclParseArgv(interp, &argc, argv, vectorSetAT, flags,
				g_vectorSet)) != FTCL_ARGV_SUCCESS) {
     return(rstatus);
   }
/*
 * Done with arguments. First find the handle that we must bind, then
 * evaluate the expression
 */
   if(shTclHandleExprEval(interp, handle_n, &handle, NULL) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(handle.type != shTypeGetFromName("VECTOR")) {
      Tcl_AppendResult(interp, 
		       "vectorExprSet: handle ", handle_n, " is a ", 
		       shNameGetFromType(handle.type), ", not a VECTOR\n",
		       (char *)NULL);
      return(TCL_ERROR);
   }

   i_interp = interp;
   x = shVectorStringEval(expr);
   shVectorDel((VECTOR*)handle.ptr);		/* we can't do this before the eval! */

   if(x == NULL) {
      shTclInterpAppendWithErrStack(interp);
      if(p_shTclHandleDel(interp,handle_n) != TCL_OK)  {
	 Tcl_AppendResult(interp, "cannot delete handle name ",handle_n,
			  (char *)NULL);
      }
      return TCL_ERROR;
   }
/*
 * all went well, so rebind the handle to return the answer
 */
   handle.ptr = x;
   
   if(p_shTclHandleAddrBind(interp,handle,handle_n) != TCL_OK) {
      Tcl_AppendResult(interp,"Can't bind to ",handle_n,
		       (char *)NULL);
      shVectorDel(x);
      return(TCL_ERROR);
   }
   
   Tcl_SetResult(interp,handle_n,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclVectorsReadFromFile_use =
  "USAGE: vectorsReadFromFile <file> <veclist> -type -nrow";
#define tclVectorsReadFromFile_hlp \
  "Read some VECTORs from a file (which TCL opened for us)"

static ftclArgvInfo vectorsReadFromFile_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclVectorsReadFromFile_hlp},
   {"<file>", FTCL_ARGV_STRING, NULL, NULL,
				      "TCL file descriptor, returned by open"},
   {"<veclist>", FTCL_ARGV_STRING, NULL, NULL,
				 "TCL List of \"vector-handle column\" pairs"},
   {"-type", FTCL_ARGV_STRING, NULL, NULL,
			   "Optional type required at start of desired lines"},
   {"-nrow", FTCL_ARGV_INT, NULL, NULL, "Number of lines to read"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define vectorsReadFromFile_name "vectorsReadFromFile"

static int
tclVectorsReadFromFile(ClientData clientData,
		       Tcl_Interp *interp,
		       int ac,
		       char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *fileStr = NULL;		/* TCL file descriptor */
   char *veclistStr = NULL;		/* List of "vector column" pairs */
   char *typeStr = NULL;		/* Optional type required at start
					   of desired lines */
   int nrow = 0;			/* Number of lines to read */

   int *cols;				/* columns to read */
   FILE *fil;				/* input file descriptor */
   int nvec;				/* number of vectors to read */
   VECTOR **vectors;			/* vectors to read */
   
   shErrStackClear();

   i = 1;
   vectorsReadFromFile_opts[i++].dst = &fileStr;
   vectorsReadFromFile_opts[i++].dst = &veclistStr;
   vectorsReadFromFile_opts[i++].dst = &typeStr;
   vectorsReadFromFile_opts[i++].dst = &nrow;
   shAssert(vectorsReadFromFile_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, vectorsReadFromFile_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     vectorsReadFromFile_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   {
      OpenFile *tclFil;			/* TCL's wrapper for a FILE */
   
      if((tclFil = Tcl_GetOpenFileStruct(interp, fileStr)) == NULL) {
	 Tcl_AppendResult(interp,"vectorsReadFromFile: "
		       "argument \"", fileStr, "\" is not open", TCL_STATIC);
	 return(TCL_ERROR);
      }
      if((fil = tclFil->f) == NULL ||
	 !(tclFil->permissions & TCL_FILE_READABLE)) {
	 Tcl_AppendResult(interp,"vectorsReadFromFile: "
			  "argument \"", fileStr, "\" is not open",TCL_STATIC);
	 return(TCL_ERROR);
      }
   }
/*
 * We have been given a list of 2-element lists; split it all up and
 * confirm that each sublist consists of a VECTOR handle and an int
 */
   {
      int c;				/* a desired column */
      char *endptr;			/* needed by strtol */
      char **elems;			/* the main list */
      char **sub_elems;			/* elements of each sublist */
      int sub_nelem;			/* number of elements on sublist */
      
      if(Tcl_SplitList(interp, veclistStr, &nvec, &elems) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(nvec == 0) {
	 return(TCL_ERROR);
      }

      vectors = shMalloc(nvec*sizeof(VECTOR *));
      cols = shMalloc(nvec*sizeof(int));
      for(i = 0; i < nvec; i++) {
	 if(Tcl_SplitList(interp, elems[i], &sub_nelem, &sub_elems) !=TCL_OK ||
							      sub_nelem != 2) {
	    Tcl_AppendResult(interp,"vectorsReadFromFile: ",
			     "Expected a \"handle int\" pair, but saw",
			     elems[i], TCL_STATIC);
	    shFree(vectors); shFree(cols);
	    return(TCL_ERROR);
	 }

	 if(shTclHandleExprEval(interp,sub_elems[0],&hand,&vptr) != TCL_OK) {
	    shFree(vectors); shFree(cols);
	    return(TCL_ERROR);
	 }
	 if(hand.type != shTypeGetFromName("VECTOR")) {
	    Tcl_AppendResult(interp,"vectorsReadFromFile: ",
			     sub_elems[0], " is not a VECTOR", TCL_STATIC);
	    shFree(vectors); shFree(cols);
	    return(TCL_ERROR);
	 }
	 vectors[i] = hand.ptr;

	 c = strtol(sub_elems[1], &endptr, 0);
	 if(endptr == sub_elems[1]) {	/* not a valid int */
	    Tcl_AppendResult(interp,"vectorsReadFromFile: ",
			     sub_elems[1], " is not an integer", TCL_STATIC);
	    shFree(vectors); shFree(cols);
	    return(TCL_ERROR);
	 }
	 cols[i] = c;
      }
   }
/*
 * work
 */
   if(shVectorsReadFromFile(fil, vectors, cols, nvec, typeStr, nrow) < 0) {
      shTclInterpAppendWithErrStack(interp);
      shFree(vectors); shFree(cols);
      return(TCL_ERROR);
   }

   shFree(vectors); shFree(cols);
   return(TCL_OK);
}


/*****************************************************************************/
/*
 * A plotting primitive; no axes; no limits; just connect the points
 * based on the current pgstate
 */
#define tclExprVectorPlot_hlp \
  "Plot a pair of vectors, optionally plot parameters from the current pgstate\n"
#define NO_MARK -1000			/* -points wasn't specified */

static ftclArgvInfo vectorPlotAT[] = {
 { NULL, FTCL_ARGV_HELP, NULL, NULL, tclExprVectorPlot_hlp },
 { "<xhandle>", FTCL_ARGV_STRING, NULL, NULL,
     "handle to VECTOR of x values" },
 { "<yhandle>", FTCL_ARGV_STRING, NULL, NULL,
     "handle to VECTOR of y values" },
 { "-connect", FTCL_ARGV_CONSTANT, (void *)1, NULL,
     "plot values by connecting them, rather than drawing points" },
 { "-pgstate", FTCL_ARGV_STRING, NULL, NULL,
	  "PGSTATE handle" },
 { "-points", FTCL_ARGV_INT, NULL, NULL,
     "plot values as points, of type <mark>, rather than connecting them" },
 { "-uxerror", FTCL_ARGV_STRING, NULL, NULL,
     "handle to VECTOR of x upper error values" },
 { "-uyerror", FTCL_ARGV_STRING, NULL, NULL,
     "handle to VECTOR of y upper error values" },
 { "-lxerror", FTCL_ARGV_STRING, NULL, NULL,
     "handle to VECTOR of x lower error values" },
 { "-lyerror", FTCL_ARGV_STRING, NULL, NULL,
     "handle to VECTOR of y lower error values" },
 { "-eBarSize", FTCL_ARGV_DOUBLE, NULL, NULL,
     "relative size of error bars" },
 { NULL, FTCL_ARGV_END, NULL, NULL, NULL }
};

char g_vectorPlot[] = "vectorExprPlot";

static int
tclExprVectorPlot(
	    ClientData clientData,
	    Tcl_Interp *interp,
	    int argc,
	    char **argv
	    )
{
   char tmp1[20],tmp2[20];
   static int connect;			/* should we connect the points? */
   static int pg_mark;			/* should we draw data as points? */
   static char *pghandle_n;		/* name of input PGSTATE handle */
   static char *xhandle_n;		/* name of input x-VECTOR handle */
   static char *yhandle_n;		/* name of input y-VECTOR handle */
   static char *xEhandle_n;		/* name of input upper xerror-VECTOR handle */
   static char *yEhandle_n;		/* name of input upper yerror-VECTOR handle */
   static char *xlEhandle_n;		/* name of input lower xerror-VECTOR handle */
   static char *ylEhandle_n;		/* name of input lower yerror-VECTOR handle */
   static double dBarSize;		/* how big should error bars be? */
   int rstatus;
   int flags;
   PGSTATE pgsaved;			/* saved value of pgstate */
   PGSTATE *pgstate;			/* state of pgplot */
   HANDLE pghandle;
   HANDLE xhandle;
   HANDLE yhandle;
   HANDLE xEhandle;
   HANDLE yEhandle;
   HANDLE xlEhandle;
   HANDLE ylEhandle;
   VECTOR *x, *y;			/* desired VECTORs */
   VECTOR *xE, *yE;			/* desired VECTORs */
   VECTOR *xlE, *ylE;			/* desired VECTORs */
   float eBarSize;
   float *xtempV, *ytempV;			/* for converting type*/
   float *xtempE, *ytempE;			/* desired VECTORs  */
   float *xtemplE, *ytemplE;			/* desired VECTORs */
   int iPoint;
   
   flags = FTCL_ARGV_NO_LEFTOVERS;
   pgstate = NULL;
   pghandle_n = NULL;
   connect = 0;
   pg_mark = NO_MARK;
   xEhandle_n = NULL; yEhandle_n = NULL;
   xlEhandle_n = NULL; ylEhandle_n = NULL;
   dBarSize = 0.;
   vectorPlotAT[1].dst = &xhandle_n;
   vectorPlotAT[2].dst = &yhandle_n;
   vectorPlotAT[3].dst = &connect;
   vectorPlotAT[4].dst = &pghandle_n;
   vectorPlotAT[5].dst = &pg_mark;
   vectorPlotAT[6].dst = &xEhandle_n;
   vectorPlotAT[7].dst = &yEhandle_n;
   vectorPlotAT[8].dst = &xlEhandle_n;
   vectorPlotAT[9].dst = &ylEhandle_n;
   vectorPlotAT[10].dst = &dBarSize;

   if((rstatus = shTclParseArgv(interp, &argc, argv, vectorPlotAT, flags,
				g_vectorPlot)) != FTCL_ARGV_SUCCESS) {
     return(rstatus);
   }
   eBarSize = dBarSize;
/*
 * Done with arguments. First find the handles that we must process
 */
   if(pghandle_n == NULL) {
      if(connect && pg_mark != NO_MARK) {
	 Tcl_AppendResult(interp, 
			  "vectorExprVectorPlot: "
			  "You cannot specify both connect and points",
			  (char *)NULL);
	 return(TCL_ERROR);
      } else if(!connect && pg_mark == NO_MARK) {
	 connect = 1;
      }
   } else {
      if(connect || pg_mark != NO_MARK) {
	 Tcl_AppendResult(interp, 
			  "vectorExprVectorPlot: You cannot specify a PGSTATE",
			  " and ", connect ? "connect" : "points",
			  (char *)NULL);
	 return(TCL_ERROR);
      }

      if(shTclHandleExprEval(interp, pghandle_n, &pghandle, NULL) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(pghandle.type != shTypeGetFromName("PGSTATE")) {
	 Tcl_AppendResult(interp, 
			"vectorExprVectorPlot: handle ", pghandle_n, " is a ", 
			shNameGetFromType(pghandle.type), ", not a PGSTATE",
			(char *)NULL);
	 return(TCL_ERROR);
      }
      pgstate = (PGSTATE*)pghandle.ptr;
   }

/* get all the handles */

   if(shTclHandleExprEval(interp, yhandle_n, &yhandle, NULL) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(yhandle.type != shTypeGetFromName("VECTOR")) {
      Tcl_AppendResult(interp, 
		       "vectorExprVectorPlot: handle ", yhandle_n, " is a ", 
		       shNameGetFromType(yhandle.type), ", not a VECTOR\n",
		       (char *)NULL);
      return(TCL_ERROR);
   }
   y = (VECTOR*)yhandle.ptr;

   if(shTclHandleExprEval(interp, xhandle_n, &xhandle, NULL) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(xhandle.type != shTypeGetFromName("VECTOR")) {
      Tcl_AppendResult(interp, 
		       "vectorExprVectorPlot: handle ", xhandle_n, " is a ", 
		       shNameGetFromType(xhandle.type), ", not a VECTOR\n",
		       (char *)NULL);
      return(TCL_ERROR);
   }
   x = (VECTOR*)xhandle.ptr;

   if(yEhandle_n != NULL) {
   	if(shTclHandleExprEval(interp, yEhandle_n, &yEhandle, NULL) != TCL_OK) {
      		return(TCL_ERROR);
   	}
   	if(yEhandle.type != shTypeGetFromName("VECTOR")) {
      		Tcl_AppendResult(interp, 
		       "vectorExprVectorPlot: handle ", yEhandle_n, " is a ", 
		       shNameGetFromType(yEhandle.type), ", not a VECTOR\n",
		       (char *)NULL);
      		return(TCL_ERROR);
   	}
   	yE = (VECTOR*)yEhandle.ptr;
   } else {
	yE = NULL;
   }

   if(xEhandle_n != NULL) {
   	if(shTclHandleExprEval(interp, xEhandle_n, &xEhandle, NULL) != TCL_OK) {
      		return(TCL_ERROR);
   	}
   	if(xEhandle.type != shTypeGetFromName("VECTOR")) {
      		Tcl_AppendResult(interp, 
		       "vectorExprVectorPlot: handle ", xEhandle_n, " is a ", 
		       shNameGetFromType(xEhandle.type), ", not a VECTOR\n",
		       (char *)NULL);
      		return(TCL_ERROR);
   	}
   	xE = (VECTOR*)xEhandle.ptr;
   } else {
	xE = NULL;
   }

   if(ylEhandle_n != NULL) {
   	if(shTclHandleExprEval(interp, ylEhandle_n, &ylEhandle, NULL) != TCL_OK) {
      		return(TCL_ERROR);
   	}
   	if(ylEhandle.type != shTypeGetFromName("VECTOR")) {
      		Tcl_AppendResult(interp, 
		       "vectorExprVectorPlot: handle ", ylEhandle_n, " is a ", 
		       shNameGetFromType(ylEhandle.type), ", not a VECTOR\n",
		       (char *)NULL);
      		return(TCL_ERROR);
   	}
   	ylE = (VECTOR*)ylEhandle.ptr;
   } else {
	ylE = NULL;
   }

   if(xlEhandle_n != NULL) {
   	if(shTclHandleExprEval(interp, xlEhandle_n, &xlEhandle, NULL) != TCL_OK) {
      		return(TCL_ERROR);
   	}
   	if(xlEhandle.type != shTypeGetFromName("VECTOR")) {
      		Tcl_AppendResult(interp, 
		       "vectorExprVectorPlot: handle ", xlEhandle_n, " is a ", 
		       shNameGetFromType(xlEhandle.type), ", not a VECTOR\n",
		       (char *)NULL);
      		return(TCL_ERROR);
   	}
   	xlE = (VECTOR*)xlEhandle.ptr;
   } else {
	xlE = NULL;
   }

/*
 * Check that the vectors exist and are the same size. If I were nice I'd
 * allow the case where one dimension is 1 and the other >1, but it isn't worth
 * the trouble. You can always plot  x : 0*x + 100
 */
   if(x == NULL || x->vec == NULL) {
      Tcl_AppendResult(interp,
		       "Vector ",xhandle_n," is empty or NULL",
		       (char *)NULL);
      return(TCL_ERROR);
   }
   if(y == NULL || y->vec == NULL) {
      Tcl_AppendResult(interp,
		       "Vector ",yhandle_n," is empty or NULL",
		       (char *)NULL);
      return(TCL_ERROR);
   }

   if(x->dimen != y->dimen) {
      sprintf(tmp1,"%d",x->dimen); sprintf(tmp2,"%d",y->dimen);
      Tcl_AppendResult(interp,
		       "Vectors ",xhandle_n," and ",yhandle_n,
		       " have different sizes (",tmp1," and ", tmp2,")",
		       (char *)NULL);
      return(TCL_ERROR);
   }
   if(xE != NULL && xlE != NULL) {
      sprintf(tmp1,"%d",x->dimen);
      if(x->dimen != xE->dimen) {
	 sprintf(tmp2,"%d",xE->dimen);
      	   Tcl_AppendResult(interp,
			    "The error vector ",xEhandle_n,
			    " has a different size (",tmp2,
			    ") than ",xhandle_n," (",tmp1,")",(char *)NULL);
      	   return(TCL_ERROR);
      }
      if(x->dimen != xlE->dimen) {
	 sprintf(tmp2,"%d",xlE->dimen);
      	   Tcl_AppendResult(interp,
		       "The error vector ",xlEhandle_n,
		       " has a different size (",tmp2,
		       ") than ",xhandle_n," (",tmp1,")",(char *)NULL);
      	   return(TCL_ERROR);
      }
   } else {
        if(xE == NULL && xlE == NULL) {
	} else {
  		Tcl_AppendResult(interp,
		       "Both x error vectors must be input",(char *)NULL);
      		return(TCL_ERROR);
	}
   }
   if(yE != NULL && ylE != NULL) {
      sprintf(tmp1,"%d",y->dimen);
      if(y->dimen != yE->dimen) {
	 sprintf(tmp2,"%d",yE->dimen);
      	   Tcl_AppendResult(interp,
		       "The error vector ",yEhandle_n,
		       " has a different size (",tmp2,
		       ") than ",yhandle_n," (",tmp1,")",(char *)NULL);
      	   return(TCL_ERROR);
      }
      if(y->dimen != ylE->dimen) {
	 sprintf(tmp2,"%d",yE->dimen);
      	   Tcl_AppendResult(interp,
		       "The error vector ",ylEhandle_n,
		       " has a different size (",tmp2,
		       ") than ",yhandle_n," (",tmp1,")",(char *)NULL);
      	   return(TCL_ERROR);
      }
   } else {
        if(yE == NULL && ylE == NULL) {
	} else {
  		Tcl_AppendResult(interp,
		       "Both y error vectors must be input",(char *)NULL);
      		return(TCL_ERROR);
	}
   }

/*
 * We have the desired pointers; now proceed to plot the vectors
 *
 * First save the graphics state, and set our desired values
 */

   /* convert to floats from VECTOR_TYPE for pgplot */

   xtempV = (float *) shMalloc(x->dimen*sizeof(float));
   xtempE = (float *) shMalloc(x->dimen*sizeof(float));
   xtemplE = (float *) shMalloc(x->dimen*sizeof(float));
   ytempV = (float *) shMalloc(x->dimen*sizeof(float));
   ytempE = (float *) shMalloc(x->dimen*sizeof(float));
   ytemplE = (float *) shMalloc(x->dimen*sizeof(float));
   
   if(xtempV==NULL || xtempE==NULL || xtemplE==NULL ||
      ytempV==NULL || ytempE==NULL || ytemplE==NULL)
   {
      shErrStackPush("Failed to malloc memory for plotting array");

      /* Free anything that might have gotten malloced before the error */
      if (xtempV != NULL)
	shFree(xtempV);
      if (xtempE != NULL)
	shFree(xtempE);
      if (xtemplE != NULL)
	shFree(xtemplE);
      if (ytempV != NULL)
	shFree(ytempV);
      if (ytempE != NULL)
	shFree(ytempE);
      if (ytemplE != NULL)
	shFree(ytemplE);

      return TCL_ERROR;
  }

   for(iPoint = 0; iPoint < x->dimen; iPoint++) {
     xtempV[iPoint] = x->vec[iPoint];
     if(xE != NULL) {
       xtempE[iPoint] = xE->vec[iPoint];
     }
     if(xlE != NULL) {
       xtemplE[iPoint] = xlE->vec[iPoint];
     }
     ytempV[iPoint] = y->vec[iPoint];
     if(yE != NULL) { 
       ytempE[iPoint] = yE->vec[iPoint];
     }
     if(ylE != NULL) {
       ytemplE[iPoint] = ylE->vec[iPoint];
     }
   }

   if(pghandle_n == NULL) {
      if(connect) {
	 cpgline(x->dimen, xtempV, ytempV);
      } else if(pg_mark != NO_MARK) {
	 cpgpt(x->dimen,xtempV,ytempV,pg_mark);
      } else {
	 Tcl_AppendResult(interp,"Please specify -connect or -points",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
      if(xE != NULL) {
	 cpgerrx(x->dimen, xtemplE, xtempE, ytempV, eBarSize);
      }
      if(yE != NULL) {
	 cpgerry(x->dimen, xtempV, ytemplE, ytempE, eBarSize);
      }
   } else {				/* use pgstate to describe plot */
      cpgqci(&pgsaved.icLine);
      cpgqls(&pgsaved.lineStyle);
      cpgqlw(&pgsaved.lineWidth);

      cpgsci(pgstate->icLine);
      cpgsls(pgstate->lineStyle);
      cpgslw(pgstate->lineWidth);

/*
 * Actually do the work
 */
      if(pgstate->isLine) {
	 cpgline(x->dimen, xtempV, ytempV);
      } else {
	 cpgsci(pgstate->icMark);
	 cpgpt(x->dimen,xtempV,ytempV,pgstate->symb);
      }

      if(xE != NULL) {
	 cpgerrx(x->dimen, xtemplE, xtempE, ytempV, eBarSize);
      }
      if(yE != NULL) {
	 cpgerry(x->dimen, xtempV, ytemplE, ytempE, eBarSize);
      }
/*
 * Done; restore state
 */
      cpgsci(pgsaved.icLine);
      cpgsls(pgsaved.lineStyle);
      cpgslw(pgsaved.lineWidth);
/*
 * Update pgstate
 */
      strcpy(pgstate->plotType,"FLOAT");
      pgstate->hg = NULL;
      strncpy(pgstate->xSchemaName, xhandle_n, HG_SCHEMA_LENGTH);
      pgstate->xSchemaName[HG_SCHEMA_LENGTH-1] = '\0';
      strncpy(pgstate->ySchemaName, yhandle_n, HG_SCHEMA_LENGTH);
      pgstate->ySchemaName[HG_SCHEMA_LENGTH-1] = '\0';
   }
   shFree(xtempV); 
   shFree(xtempE); 
   shFree(xtemplE);
   shFree(ytempV);
   shFree(ytempE);
   shFree(ytemplE);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclVectorExprGet_use =
  "USAGE: vectorExprGet <expr>";
#define tclVectorExprGet_hlp \
  "Return the value of a (scalar) vector expression"

static ftclArgvInfo vectorExprGet_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclVectorExprGet_hlp},
   {"<expr>", FTCL_ARGV_STRING, NULL, NULL, "Expression to evaluate"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define vectorExprGet_name "vectorExprGet"

static int
tclVectorExprGet(ClientData clientData,
		 Tcl_Interp *interp,
		 int ac,
		 char **av)
{
   int i;
   char *exprStr = NULL;		/* Expression to evaluate */
   VECTOR *val;				/* returned value */

   shErrStackClear();

   i = 1;
   vectorExprGet_opts[i++].dst = &exprStr;
   shAssert(vectorExprGet_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, vectorExprGet_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     vectorExprGet_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   if((val = shVectorStringEval(exprStr)) == NULL) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   if(val->dimen == 0) {
      shVectorDel(val);
      Tcl_SetResult(interp,"expression has no elements",TCL_VOLATILE);
      return(TCL_ERROR);
   }

   {
      char buff[100];
      sprintf(buff, "%-.17g", val->vec[0]);
      Tcl_SetResult(interp,buff,TCL_VOLATILE);
   }

   shVectorDel(val);

   return(TCL_OK);
}


/*****************************************************************************/
/*
 * Now some words to interface to the world of ARRAYs.
 *
 * It would be perfectly feasible to build VECTORs on ARRAYs; as it is
 * vectors can be initialised from ARRAYs
 *
 * It would have been possible to use AFs, but I didn't do so as I feel that
 * AFs are too heavy weight for simple data manipulation, with masks and
 * errors carried along. These can be handled directly with the expression
 * package at the discretion of the user. RHL
 */


/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
shTclVectorDeclare(Tcl_Interp *interp) 
{
   int flags = FTCL_ARGV_NO_LEFTOVERS;

   shTclDeclare(interp,g_vectorNew, 
		(Tcl_CmdProc *)tclExprVectorNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module,
		shTclGetArgInfo(interp, vectorNewAT, flags, g_vectorNew),
		shTclGetUsage(interp, vectorNewAT, flags, g_vectorNew));

   shTclDeclare(interp,g_vectorDel, 
		(Tcl_CmdProc *)tclExprVectorDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module,
		shTclGetArgInfo(interp, vectorDelAT, flags, g_vectorDel),
		shTclGetUsage(interp, vectorDelAT, flags, g_vectorDel));

   shTclDeclare(interp,g_vectorEval, 
		(Tcl_CmdProc *)tclExprVectorEval, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module,
		shTclGetArgInfo(interp, vectorEvalAT, flags, g_vectorEval),
		shTclGetUsage(interp, vectorEvalAT, flags, g_vectorEval));

   shTclDeclare(interp,g_vectorPrint, 
		(Tcl_CmdProc *)tclExprVectorPrint, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module,
		shTclGetArgInfo(interp, vectorPrintAT, flags, g_vectorPrint),
		shTclGetUsage(interp, vectorPrintAT, flags, g_vectorPrint));

   shTclDeclare(interp,g_vectorSet, 
		(Tcl_CmdProc *)tclExprVectorSet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module,
		shTclGetArgInfo(interp, vectorSetAT, flags, g_vectorSet),
		shTclGetUsage(interp, vectorSetAT, flags, g_vectorSet));


   shTclDeclare(interp,g_vectorPlot, 
		(Tcl_CmdProc *)tclExprVectorPlot, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module,
		shTclGetArgInfo(interp, vectorPlotAT, flags, g_vectorPlot),
		shTclGetUsage(interp, vectorPlotAT, flags, g_vectorPlot));

   shTclDeclare(interp,vectorsReadFromFile_name,
		(Tcl_CmdProc *)tclVectorsReadFromFile, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclVectorsReadFromFile_hlp,
		tclVectorsReadFromFile_use);

   shTclDeclare(interp,vectorExprGet_name,
		(Tcl_CmdProc *)tclVectorExprGet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclVectorExprGet_hlp,
		tclVectorExprGet_use);
} 

/*****************************************************************************/
/*
 * This is an extra global function. It is used to prevent the vector
 * evaluation code from having to learn to speak TCL.
 *
 * It looks up a handle by name, extracts the answer into a VECTOR, and
 * returns the aforementioned vector. The handle may be an ARRAY of
 * FLOAT or REAL (grrr! why were AFs not implemented as an 3*n array of FLOAT?)
 *
 * The data is taken to be in the first column of the array. Extra syntax would
 * be needed to relax this restriction
 */
VECTOR *
p_shVectorGet(char *name,	/* name of desired vector */
	      int *allocated)	/* did I have to allocate the vector? */
{
   HANDLE handle;
   int i;

   *allocated = 0;

   if(shTclHandleExprEval(i_interp, name, &handle, NULL) != TCL_OK) {
      return(NULL);
   }
/*
 * Deal with acceptable handles; VECTORs are easy; ARRAYs are easy but a pain
 */
   if(handle.type == shTypeGetFromName("VECTOR")) {
      return((VECTOR*)handle.ptr);
   } else if(handle.type == shTypeGetFromName("ARRAY")) {
      ARRAY *arr = (ARRAY*)handle.ptr;
      VECTOR *vec;

      if(arr->data.schemaType != shTypeGetFromName("DOUBLE") &&
	 arr->data.schemaType != shTypeGetFromName("FLOAT") &&
	 arr->data.schemaType != shTypeGetFromName("CHAR") &&
	 arr->data.schemaType != shTypeGetFromName("UCHAR") &&
	 arr->data.schemaType != shTypeGetFromName("SHORT") &&
	 arr->data.schemaType != shTypeGetFromName("USHORT") &&
	 arr->data.schemaType != shTypeGetFromName("INT") &&
	 arr->data.schemaType != shTypeGetFromName("UINT") &&
	 arr->data.schemaType != shTypeGetFromName("LONG") &&
	 arr->data.schemaType != shTypeGetFromName("ULONG") &&
	 arr->data.schemaType != shTypeGetFromName("REAL")) {
	 Tcl_AppendResult(i_interp, 
			  "p_shVectorGet: handle ",name,
			  " must have a simple scalar type, not ",
			  shNameGetFromType(arr->data.schemaType),
			  (char *)NULL);
	 return(NULL);
      }

      if(arr->dimCnt <= 0 || arr->dim[0] == 0) {
	 Tcl_AppendResult(i_interp, 
			  "p_shVectorGet: handle ",name,
			  " has no associated data",(char *)NULL);
	 return(NULL);
      }
      vec = shVectorNew(arr->dim[0]); *allocated = 1;

      if(arr->data.schemaType == shTypeGetFromName("DOUBLE")) {
	 double *arr_data = (double *)arr->data.dataPtr;
  
	 for(i = 0;i < vec->dimen;i++) {
	    vec->vec[i] = arr_data[i];
	 }
      } else if(arr->data.schemaType == shTypeGetFromName("FLOAT")) {
	 float *arr_data = (float *)arr->data.dataPtr;
  
	 for(i = 0;i < vec->dimen;i++) {
	    vec->vec[i] = arr_data[i];
	 }
      } else if(arr->data.schemaType == shTypeGetFromName("CHAR") ||
		arr->data.schemaType == shTypeGetFromName("UCHAR")) {
	 char *arr_data = (char *)arr->data.dataPtr;
  
	 for(i = 0;i < vec->dimen;i++) {
	    vec->vec[i] = arr_data[i];
	 }
      } else if(arr->data.schemaType == shTypeGetFromName("SHORT") ||
		arr->data.schemaType == shTypeGetFromName("USHORT")) {
	 short *arr_data = (short *)arr->data.dataPtr;
  
	 for(i = 0;i < vec->dimen;i++) {
	    vec->vec[i] = arr_data[i];
	 }
      } else if(arr->data.schemaType == shTypeGetFromName("INT") ||
		arr->data.schemaType == shTypeGetFromName("UINT")) {
	 int *arr_data = (int *)arr->data.dataPtr;
  
	 for(i = 0;i < vec->dimen;i++) {
	    vec->vec[i] = arr_data[i];
	 }
      } else if(arr->data.schemaType == shTypeGetFromName("LONG") ||
		arr->data.schemaType == shTypeGetFromName("ULONG")) {
	 long *arr_data = (long *)arr->data.dataPtr;
  
	 for(i = 0;i < vec->dimen;i++) {
	    vec->vec[i] = arr_data[i];
	 }
      } else if(arr->data.schemaType == shTypeGetFromName("REAL")) {
	 REAL *arr_data = (REAL *)arr->data.dataPtr;
  
	 for(i = 0;i < vec->dimen;i++) {
	    vec->vec[i] = arr_data[i].value;
	 }
      } else {
	 Tcl_AppendResult(i_interp, 
		       "p_shVectorGet: Impossible place in code for handle ",
		       name, " ARRAY type ",
		       shNameGetFromType(arr->data.schemaType),
		       (char *)NULL);
	 shVectorDel(vec);
         return(NULL);
      }

      return(vec);
   } else {
      Tcl_AppendResult(i_interp, 
		       "p_shVectorGet: handle ", name, " is a ", 
		       shNameGetFromType(handle.type), ", not a VECTOR\n",
		       (char *)NULL);
      return(NULL);
   }
}
