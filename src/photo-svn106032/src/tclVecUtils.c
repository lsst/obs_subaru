#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dervish.h"
#include "phConsts.h"
#include "phVecUtils.h"

static char *module = "vecUtils";   /* name of this set of code */

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
  {"-nbin", FTCL_ARGV_INT, NULL, NULL,
	 "Number of rows to sum together while calculating desired statistic"},
  {"-clip", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Clip histogram"},
  {"-quartiles", FTCL_ARGV_CONSTANT, (void *)1, NULL,
					      "Return a list: {Q1 median Q3}"},
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
   int a_i;
   void *vptr;				/* used by shTclHandleExprEval */
   int dirn;
   int row0, row1, col0, col1;
   char *regName;
   REGION *reg;
   int use_med = 0;
   int nbin = 1;			/* number of rows to sum together */
   int clip = 0;			/* clip histogram? */
   int quartiles = 0;			/* return all 3 quartiles? */
   VECTOR *vec;				/* the desired result */
   VECTOR *Q_s[3];			/* space for the three quartiles */
   VECTOR **Q;				/* pointer to Q_s, or NULL */
/*
 * Parse the command
 */
   a_i = 1;
   vectorGetFromRegionTable[a_i++].dst = &regName;
   vectorGetFromRegionTable[a_i++].dst = &row0;
   vectorGetFromRegionTable[a_i++].dst = &row1;
   vectorGetFromRegionTable[a_i++].dst = &col0;
   vectorGetFromRegionTable[a_i++].dst = &col1;
   vectorGetFromRegionTable[a_i++].dst = &dirn;
   vectorGetFromRegionTable[a_i++].dst = &use_med;
   vectorGetFromRegionTable[a_i++].dst = &nbin;
   vectorGetFromRegionTable[a_i++].dst = &clip;
   vectorGetFromRegionTable[a_i++].dst = &quartiles;
   shAssert(vectorGetFromRegionTable[a_i].dst == NULL);
   
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
      Tcl_SetResult(interp,"vectorGetFromRegion: "
                    "first argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(use_med && reg->type != TYPE_PIX) {
      Tcl_SetResult(interp,"vectorGetFromRegion: "
                    "You can only use -median with PIX regions",TCL_STATIC);
      return(TCL_ERROR);
   }
   if(nbin <= 0) {
      Tcl_SetResult(interp,"vectorGetFromRegion: "
                    "-nbin must specify a value >= 1",TCL_STATIC);
      return(TCL_ERROR);
   } else if(nbin > 1 && !use_med) {
      Tcl_SetResult(interp,"vectorGetFromRegion: "
                    "-nbin doesn't make sense without -median",TCL_STATIC);
      return(TCL_ERROR);
   }

   if(quartiles) {
      if(!use_med) {
	 Tcl_SetResult(interp,"vectorGetFromRegion: "
		       "-quartiles doesn't make sense without -median",
		       TCL_STATIC);
	 return(TCL_ERROR);
      }
      Q = Q_s;
   } else {
      Q = NULL;
   }
/*
 * Finally do the work
 */
   if((vec = phVectorGetFromRegion(reg, row0,row1, col0,col1, dirn,
				   Q, clip, use_med, nbin)) == NULL) {
      shTclInterpAppendWithErrStack(interp);
     return(TCL_ERROR);
   }
/*
 * Return the answer
 */
   if(quartiles) {
      int i;
      
      Tcl_ResetResult(interp);
      
      for(i = 0; i < 3; i++) {
	 if(p_shTclHandleNew(interp,name) != TCL_OK) {
	    shTclInterpAppendWithErrStack(interp);
	    return(TCL_ERROR);
	 }
	 hand.ptr = Q[i];
	 hand.type = shTypeGetFromName("VECTOR");
	 
	 if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
	    Tcl_SetResult(interp,"can't bind to new VECTOR handle",TCL_STATIC);
	    shVectorDel(vec);
	    return(TCL_ERROR);
	 }
	 
	 Tcl_AppendElement(interp, name);
      }
   } else {
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
   }
   
  return TCL_OK;
}

/*****************************************************************************/
/*
 * add a vector to each row of a region
 */
static char *tclVectorAddToRegionRows_use =
  "USAGE: vectorAddToRegionRows <reg> <vec>";
#define tclVectorAddToRegionRows_hlp \
  "Add a vector to each row of a region"

static ftclArgvInfo vectorAddToRegionRows_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclVectorAddToRegionRows_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region to add to"},
   {"<vec>", FTCL_ARGV_STRING, NULL, NULL, "Vector to add"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define vectorAddToRegionRows_name "vectorAddToRegionRows"

static int
tclVectorAddToRegionRows(ClientData clientData,
			 Tcl_Interp *interp,
			 int ac,
			 char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region to add to */
   REGION *reg;
   char *vecStr = NULL;			/* Vector to add */
   VECTOR *vec;

   shErrStackClear();

   i = 1;
   vectorAddToRegionRows_opts[i++].dst = &regStr;
   vectorAddToRegionRows_opts[i++].dst = &vecStr;
   shAssert(vectorAddToRegionRows_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, vectorAddToRegionRows_opts,
		     0, vectorAddToRegionRows_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"vectorAddToRegionRows: "
                    "argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(shTclHandleExprEval(interp,vecStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("VECTOR")) {
      Tcl_SetResult(interp,"vectorAddToRegionRows: "
                    "argument is not a VECTOR",TCL_STATIC);
      return(TCL_ERROR);
   }
   vec = hand.ptr;
/*
 * work
 */
   if(phVectorAddToRegionRows(reg, vec) != 0) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * add a vector to each column of a region
 */
static char *tclVectorAddToRegionCols_use =
  "USAGE: vectorAddToRegionCols <reg> <vec>";
#define tclVectorAddToRegionCols_hlp \
  "Add a vector to each column of a region"

static ftclArgvInfo vectorAddToRegionCols_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclVectorAddToRegionCols_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region to add to"},
   {"<vec>", FTCL_ARGV_STRING, NULL, NULL, "Vector to add"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define vectorAddToRegionCols_name "vectorAddToRegionCols"

static int
tclVectorAddToRegionCols(ClientData clientData,
			 Tcl_Interp *interp,
			 int ac,
			 char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region to add to */
   REGION *reg;
   char *vecStr = NULL;			/* Vector to add */
   VECTOR *vec;

   shErrStackClear();

   i = 1;
   vectorAddToRegionCols_opts[i++].dst = &regStr;
   vectorAddToRegionCols_opts[i++].dst = &vecStr;
   shAssert(vectorAddToRegionCols_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, vectorAddToRegionCols_opts,
		     0, vectorAddToRegionCols_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"vectorAddToRegionCols: "
                    "argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(shTclHandleExprEval(interp,vecStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("VECTOR")) {
      Tcl_SetResult(interp,"vectorAddToRegionCols: "
                    "argument is not a VECTOR",TCL_STATIC);
      return(TCL_ERROR);
   }
   vec = hand.ptr;
/*
 * work
 */
   if(phVectorAddToRegionCols(reg, vec) != 0) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclVecUtilsDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp, vectorGetFromRegion_name,
               (Tcl_CmdProc *)tclVectorGetFromRegion,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module, shTclGetArgInfo(interp, vectorGetFromRegionTable, 0,
				       vectorGetFromRegion_name),
               shTclGetUsage(interp, vectorGetFromRegionTable, 0,
				vectorGetFromRegion_name));

   shTclDeclare(interp,vectorAddToRegionRows_name,
		(Tcl_CmdProc *)tclVectorAddToRegionRows, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclVectorAddToRegionRows_hlp,
		tclVectorAddToRegionRows_use);

   shTclDeclare(interp,vectorAddToRegionCols_name,
		(Tcl_CmdProc *)tclVectorAddToRegionCols, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclVectorAddToRegionCols_hlp,
		tclVectorAddToRegionCols_use);
}
