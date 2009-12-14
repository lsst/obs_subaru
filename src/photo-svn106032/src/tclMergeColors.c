/*
 * ENTRY POINT		    SCOPE
 * tclMergeColorsDeclare public  Declare all the verbs defined in this module
 *
 * REQUIRED PRODUCTS:
 *	FTCL		TCL + XTCL + Fermilab extensions
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "phMergeColors.h"
#include "phUtils.h"

static char *module = "phTclMergeColors";    /* name of this set of code */

/*****************************************************************************/
/*
 * Merge a several LISTs of OBJECT1's into a LIST of OBJCs 
 *
 * Clean up and return
 */
#define RETURN(VAL) \
   if(objects != NULL) shFree(objects); \
   return(VAL)

#define tclMergeColors_hlp \
"transfer objects from single-color chains of OBJECT1s onto a merge, multi-color chain of OBJCs, which is created in this function; find all overlapping objects and stick them together; create a master mask for each OBJC.  Return the name of the merged OBJC chain handle. If -merged is specified, merge OBJECT1s into list and return it"

static char *phTclMergeColors_use = 
   "USAGE: mergeColors filter_names objects torefcolor fromrefcolor -merged list <list> -cut_atlas -no_restore_ai -val <val> -sigma <sigma> -rand <rand>";

static ftclArgvInfo phMergeColors_opts[] = {
   { (char *) NULL, FTCL_ARGV_HELP, NULL, NULL, tclMergeColors_hlp },
   {"<filters>", FTCL_ARGV_STRING, NULL, NULL, "TCL array of all the filter names"},
   {"<objects>", FTCL_ARGV_STRING, NULL, NULL,
				 "array of chains of OBJECT1s, one per filter"},
   {"<fparams>", FTCL_ARGV_STRING, NULL, NULL, "FIELDPARAMS for frame"},
   {"-merged", FTCL_ARGV_STRING, NULL, NULL,
	     "a pre-existing OBJC list that the objects are to be merged into"},
   {"-cut_atlas", FTCL_ARGV_CONSTANT, (void *)1, NULL, "cut atlas images"},
   {"-no_restore_ai", FTCL_ARGV_CONSTANT, (void *)1, NULL, "don't restore old atlas images"},
   {"-value", FTCL_ARGV_INT, NULL, NULL, "value to replace cut atlas images"},
   {"-sigma", FTCL_ARGV_DOUBLE, NULL, NULL,
				      "standard deviation of val (default: 0)"},
   {"-rand", FTCL_ARGV_STRING, NULL, NULL, "A RANDOM"},
   {"-bkgd", FTCL_ARGV_CONSTANT, (void *)1, NULL,
    "Use SOFT_BIAS + FRAMEPARAMS.bkgd as the value of -value in each band"},
   { (char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL }
};

static int
phTclMergeColors(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   int a_i;
   void *vptr;				/* used by shTclHandleExprEval */
   CHAIN **objects = NULL;
   int nobj_chain;			/* dimension of objects */
   FIELDPARAMS *fparams = NULL;
   HANDLE handle;			/* new handle for the result */
   char name[HANDLE_NAMELEN];		/* name of new handle */
   RANDOM *rand;
   char *filtersStr = NULL;		/* TCL array of all the filter names */
   char *objectsStr = NULL;		/* array of chains of OBJECT1s,
					   one per filter */
   char *fparamsStr = NULL;		/* FIELDPARAMS for frame */
   char *mergedStr = NULL;		/* a pre-existing OBJC list that
					   the objects are to be merged into */
   int cut_atlas = 0;			/* cut atlas images */
   int no_restore_ai = 0;		/* don't restore old atlas images */
   int value = -1;			/* value to replace cut atlas images */
   double sigma = -1.0;			/* standard deviation of val */
   char *randStr = NULL;		/* A RANDOM */
   int bkgd = 0;		    /* Use SOFT_BIAS + FRAMEPARAMS.bkgd as the value of -value in each band */
   
   shErrStackClear();

   a_i = 1;
   phMergeColors_opts[a_i++].dst = &filtersStr;
   phMergeColors_opts[a_i++].dst = &objectsStr;
   phMergeColors_opts[a_i++].dst = &fparamsStr;
   phMergeColors_opts[a_i++].dst = &mergedStr;
   phMergeColors_opts[a_i++].dst = &cut_atlas;
   phMergeColors_opts[a_i++].dst = &no_restore_ai;
   phMergeColors_opts[a_i++].dst = &value;
   phMergeColors_opts[a_i++].dst = &sigma;
   phMergeColors_opts[a_i++].dst = &randStr;
   phMergeColors_opts[a_i++].dst = &bkgd;
   shAssert(phMergeColors_opts[a_i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp, &argc, argv, phMergeColors_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * process arguments
 */
   if (bkgd) {
       if(value >= 0) {
	   Tcl_SetResult(interp, "phMergeColors: "
			 "please don't specify -value and -bkgd", TCL_STATIC);
	   return(TCL_ERROR);
       }
       value = -SOFT_BIAS;		/* i.e. use SOFT_BIAS + FRAMEPARAMS.bkgd */
   }

   if(shTclHandleExprEval(interp,fparamsStr,&handle,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(handle.type != shTypeGetFromName("FIELDPARAMS")) {
      Tcl_SetResult(interp,"mergeColors: "
                    "argument is not a FIELDPARAMS",TCL_STATIC);
      return(TCL_ERROR);
   }
   fparams = handle.ptr;

   if(phPtrArrayGetFromTclArrayOfHandles(interp, objectsStr,
	 filtersStr, "CHAIN", (void ***)&objects, &nobj_chain) != SH_SUCCESS) { 
      shErrStackPush("mergeColors: Trouble getting objects");
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   if(fparams->ncolor != nobj_chain) {
      Tcl_AppendResult(interp,"mergeColors: object1 chain ncolor "
		       " != number of colours in FIELD",NULL);
      RETURN(TCL_ERROR);
   }

   if(sigma <= 0) {			/* no need for rand */
      rand = NULL;
   } else {				/* must specify -rand */
      if(randStr == NULL) {
	 Tcl_SetResult(interp,"mergeColors: "
		       "you _must_ provide -rand if sigma > 0",TCL_STATIC);
	 return(TCL_ERROR);
      }
      if(shTclHandleExprEval(interp,randStr,&handle,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(handle.type != shTypeGetFromName("RANDOM")) {
	 Tcl_SetResult(interp,"mergeColors: "
		       "-rand's argument is not a RANDOM",TCL_STATIC);
	 return(TCL_ERROR);
      }
      rand = handle.ptr;
   }
/*
 * If needed, create a new CHAIN for the output OBJCs
 */
   if(mergedStr == NULL) {
      handle.ptr = shChainNew("OBJC");
      handle.type = shTypeGetFromName("CHAIN");
   } else {
      if(shTclHandleExprEval(interp,mergedStr,&handle,&vptr) != TCL_OK) {
	 RETURN(TCL_ERROR);
      }
      if(handle.type != shTypeGetFromName("CHAIN") ||
	 ((CHAIN *)handle.ptr)->type != shTypeGetFromName("OBJC")) {
	 Tcl_SetResult(interp,"mergeColors: "
		   "argument to -merged flag is not an OBJC CHAIN",TCL_STATIC);
	 RETURN(TCL_ERROR);
      }
   }
/* 
 * call the C function to perform the work
 */
   if(phMergeColors(objects, fparams, handle.ptr, cut_atlas, no_restore_ai,
		    value, sigma, rand) != SH_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      RETURN(TCL_ERROR);
   }
/*
 * If one wasn't passed in, get a handle for the result
 */
   if(mergedStr != NULL) {
      Tcl_SetResult(interp,mergedStr,TCL_VOLATILE);
   } else {
      if(p_shTclHandleNew(interp, name) != TCL_OK) {
	 shTclInterpAppendWithErrStack(interp);
	 RETURN(TCL_ERROR);
      }
      if(p_shTclHandleAddrBind(interp, handle, name) != TCL_OK) {
	 Tcl_SetResult(interp,"Can't bind to new list handle",TCL_VOLATILE);
	 RETURN(TCL_ERROR);
      }
      Tcl_SetResult(interp,name,TCL_VOLATILE);
   }
   
   RETURN(TCL_OK);
}

/*************************************************************************
 * call the initialization function for Merge Colors module.
 */
static char *phTclInitMergeColors_use =
  "USAGE: initMergeColors ncolors";
static char *phTclInitMergeColors_hlp =
  "Initialize some variables used by the MergeColors module";

static ftclArgvInfo phInitMergeColors_opts[] = {
   { (char *) NULL, FTCL_ARGV_HELP, NULL, NULL,
					  "initialize the MergeColors module" },
   {"<ncolors>", FTCL_ARGV_INT, NULL, NULL, "number of colors (== filters)"},
   { (char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL }
};


static int
phTclInitMergeColors(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   int i;
   int ncolors = 0;			/* number of colors (== filters) */

   shErrStackClear();

   i = 1;
   phInitMergeColors_opts[i++].dst = &ncolors;
   shAssert(phInitMergeColors_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp, &argc, argv, phInitMergeColors_opts) != TCL_OK){

      return(TCL_ERROR);
   }

   if (phInitMergeColors(ncolors) != SH_SUCCESS) {
      Tcl_SetResult(interp, 
            "phTclInitMergeColors: phInitMergeColors returns with error",
            TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, "", TCL_STATIC);
   return(TCL_OK);
}


/*************************************************************************
 * call the Fini function for Merge Colors module.
 */

static char *phTclFiniMergeColors_use =
  "USAGE: finiColorsMerge";
static char *phTclFiniMergeColors_hlp =
  "Clean up variables and memory used by the MergeColors module";

static ftclArgvInfo phFiniMergeColors_opts[] = {
   { (char *) NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Clean up vars and memory used by the MergeColors module" },
   { (char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL }
};


static int
phTclFiniMergeColors(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   shErrStackClear();

   if(get_FtclOpts(interp, &argc, argv, phFiniMergeColors_opts) != TCL_OK){
      return(TCL_ERROR);
   }

   if (phFiniMergeColors() != SH_SUCCESS) {
      Tcl_SetResult(interp, 
            "phTclFiniMergeColors: phFiniMergeColors returns with error",
            TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, "", TCL_STATIC);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclMergeColorsDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"mergeColors",
                (Tcl_CmdProc *)phTclMergeColors,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclMergeColors_hlp, phTclMergeColors_use);

   shTclDeclare(interp,"initMergeColors",
                (Tcl_CmdProc *)phTclInitMergeColors,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, phTclInitMergeColors_hlp, phTclInitMergeColors_use);

   shTclDeclare(interp,"finiMergeColors",
                (Tcl_CmdProc *)phTclFiniMergeColors,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, phTclFiniMergeColors_hlp, phTclFiniMergeColors_use);
}

