

/*
 * <AUTO>
 * FILE: tclPMatch.c
 *
 * ABSTRACT:
 * This file contains TCL-handling routines which call C 
 * functions which, in turn, try to match up items
 * in two different CHAINs, which might have very different
 * coordinate systems. 
 * </HTML>
 *
 * </AUTO>
 *
 */



#include <stdio.h>
#include <string.h>
#include "dervish.h"
#include "atTrans.h"
#include "atPMatch.h"

static char *module = "astrotools";    /* name of this set of code */


/************************************************************************
 * <AUTO EXTRACT>
 *
 * TCL VERB: atFindTrans
 * 
 * <HTML>
 * C ROUTINE CALLED: <a href="atPMatch.html#atFindTrans">atFindTrans</a> in atPMatch.c
 * </HTML>
 *
 * DESCRIPTION:
 * given two CHAINs of items, and the names of the fields we use to
 * perform the match,
 * call the C-code routine that performs the match and yields
 * a TRANS structure converting the coords of CHAIN A to the
 * coords of CHAIN B.
 *
 * Only the affine elemnts of the TRANS are set.  The higer-order distortion
 * and color terms are all set to 0.
 *
 * RETURN:
 *    TCL_OK             if all goes well
 *    TCL_ERROR          if an error occurs
 *
 * </AUTO>
 */


static ftclArgvInfo tclAtFindTrans_opts[] = {
   { NULL, FTCL_ARGV_HELP, NULL, NULL, 
       "Given two chains of items, and the names of the "\
       "fields in both that contain 'x', 'y' and 'mag' values, "\
       "find the coordinate transformation that converts items "\
       "in chain A to chain B's coordinate system. " },
   { "<chainA>", FTCL_ARGV_STRING, NULL, NULL, "first chain of items" },
   { "<xnameA>", FTCL_ARGV_STRING, NULL, NULL, 
                 "name of field in chain A which contains X coordinate" },
   { "<ynameA>", FTCL_ARGV_STRING, NULL, NULL, 
                 "name of field in chain A which contains Y coordinate" },
   { "<magnameA>", FTCL_ARGV_STRING, NULL, NULL, 
                 "name of field in chain A which contains magnitude " },
   { "<chainB>", FTCL_ARGV_STRING, NULL, NULL, "second chain of items" },
   { "<xnameB>", FTCL_ARGV_STRING, NULL, NULL, 
                 "name of field in chain B which contains X coordinate" },
   { "<ynameB>", FTCL_ARGV_STRING, NULL, NULL, 
                 "name of field in chain B which contains Y coordinate" },
   { "<magnameB>", FTCL_ARGV_STRING, NULL, NULL, 
                 "name of field in chain B which contains magnitude " },
   { "-radius",  FTCL_ARGV_DOUBLE, NULL, NULL, 
                 "max allowed radius in triangle space for match;\n"
                 "Default: 0.002" },
   { "-nobj",    FTCL_ARGV_INT, NULL, NULL, 
                 "number of objects from each set to use in matching process;\n"
                 "Default: 20" },
   { "-maxdist", FTCL_ARGV_DOUBLE, NULL, NULL, 
                 "max dist in coord system B allowed for corresponding pairs;\n"
                 "Default: 500.0" },
   { "-scale",   FTCL_ARGV_DOUBLE, NULL, NULL, 
                 "ratio of coord size in list B to coord size in list A;\n"
                 "Default: -1.0" },
   { NULL, FTCL_ARGV_END, NULL, NULL, NULL }
};

static char *g_atFindTrans = "atFindTrans";


static int
tclAtFindTrans(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE chainA_handle, chainB_handle, trans_handle;
   char name[HANDLE_NAMELEN];
   char xnameA[30], ynameA[30], magnameA[30];
   char xnameB[30], ynameB[30], magnameB[30];
   char *sval0, *sval1, *sval2, *sval3, *sval4, *sval5;
   char *sval6, *sval7;
   CHAIN *chainA, *chainB;
   TRANS *trans;
   void *vptr;
   double dval0, dval1, dval2;
   float radius, maxdist, scale;
   int nobj;
   int ival0;
   
   shErrStackClear();

   /* give some default values for the optional arguments */
   dval0 = AT_MATCH_RADIUS;
   ival0 = AT_MATCH_NBRIGHT;
   dval1 = AT_MATCH_MAXDIST;
   dval2 = -1.0;

   tclAtFindTrans_opts[1].dst = &sval0;
   tclAtFindTrans_opts[2].dst = &sval1;
   tclAtFindTrans_opts[3].dst = &sval2;
   tclAtFindTrans_opts[4].dst = &sval3;
   tclAtFindTrans_opts[5].dst = &sval4;
   tclAtFindTrans_opts[6].dst = &sval5;
   tclAtFindTrans_opts[7].dst = &sval6;
   tclAtFindTrans_opts[8].dst = &sval7;
   tclAtFindTrans_opts[9].dst = &dval0;
   tclAtFindTrans_opts[10].dst = &ival0;
   tclAtFindTrans_opts[11].dst = &dval1;
   tclAtFindTrans_opts[12].dst = &dval2;
   

   if (shTclParseArgv(interp, &argc, argv, tclAtFindTrans_opts,
	       FTCL_ARGV_NO_LEFTOVERS, g_atFindTrans) != FTCL_ARGV_SUCCESS) {
      return(TCL_ERROR);
   }

   /* now process the arguments */
   if (shTclHandleExprEval(interp, sval0, &chainA_handle, &vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if (chainA_handle.type != shTypeGetFromName("CHAIN")) {
      Tcl_SetResult(interp, "atFindTrans: first arg is not a CHAIN", TCL_STATIC);
      return(TCL_ERROR);
   }
   chainA = chainA_handle.ptr;

   strcpy(xnameA, sval1);
   strcpy(ynameA, sval2);
   strcpy(magnameA, sval3);

   if (shTclHandleExprEval(interp, sval4, &chainB_handle, &vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if (chainB_handle.type != shTypeGetFromName("CHAIN")) {
      Tcl_SetResult(interp, "atFindTrans: fifth arg is not a CHAIN", TCL_STATIC);
      return(TCL_ERROR);
   }
   chainB = chainB_handle.ptr;

   strcpy(xnameB, sval5);
   strcpy(ynameB, sval6);
   strcpy(magnameB, sval7);

   /* 
    * these are the optional arguments.  We have given them default
    * values above, at the very top of this function.
    */
   radius = (float) dval0;
   nobj = ival0;
   maxdist = (float) dval1;
   scale = (float) dval2;

   trans = (TRANS *) atTransNew();


   /* now call the C function that performs the matching. */
 
   if (atFindTrans(chainA, xnameA, ynameA, magnameA, 
                   chainB, xnameB, ynameB, magnameB, 
                   radius, nobj, maxdist, scale,
                   trans) != SH_SUCCESS) {
      Tcl_SetResult(interp, "atFindTrans: atFindTrans returns with error",
	       TCL_STATIC);
      atTransDel(trans);
      return(TCL_ERROR);
   }

   /* all went well, so we must create a handle for the new TRANS structure */
 
   if (p_shTclHandleNew(interp, name) != TCL_OK) {
      Tcl_SetResult(interp, "Can't get a new handle", TCL_STATIC);
      atTransDel(trans);
      return(TCL_ERROR);
   }

   trans_handle.ptr = trans;
   trans_handle.type = shTypeGetFromName("TRANS");

   if (p_shTclHandleAddrBind(interp, trans_handle, name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind TRANS to new handle", TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   return(TCL_OK);
}


/************************************************************************
 * <AUTO EXTRACT>
 *
 * TCL VERB: atApplyTrans
 *
 * <HTML>
 * C ROUTINE CALLED: <a href="atPMatch.html#atApplyTrans">atApplyTrans</a> in atPMatch.c
 * </HTML>
 *
 * DESCRIPTION:
 * Transform the coordinates of items on the given CHAIN, according
 * to the given TRANS structure.
 * Only the affine terms of the TRANS structure are used.  The higher-order
 * distortion and color terms are ignfored.  Thus:
 *
 *       x' = a + bx + cx
 *       y' = d + ex + fy
 *
 * RETURN:
 *    TCL_OK             if all goes well
 *    TCL_ERROR          if an error occurs
 *
 * </AUTO>
 */

static ftclArgvInfo tclAtApplyTrans_opts[] = {
  { NULL, FTCL_ARGV_HELP, NULL, NULL,
      "Given a CHAIN of items, and the names of the "\
      "fields in that CHAIN which contain 'x' and 'y' values, "\
      "and a TRANS structure, apply the given TRANS coefficients "\
      "to every item in the CHAIN, calculating new 'x' and 'y' values." },
  { "<chain>", FTCL_ARGV_STRING, NULL, NULL, "chain of items" },
  { "<xname>", FTCL_ARGV_STRING, NULL, NULL,
               "name of field in chain which contains X coordinate" },
  { "<yname>", FTCL_ARGV_STRING, NULL, NULL,
               "name of field in chain which contains Y coordinate" },
  { "<trans>", FTCL_ARGV_STRING, NULL, NULL,
               "TRANS structure to be applied to items in CHAIN" }, 
  { NULL, FTCL_ARGV_END, NULL, NULL, NULL } 
};

/***** old version which, for some reason did not produce proper HTML
static ftclArgvInfo tclAtApplyTrans_opts[] = {
   { NULL, FTCL_ARGV_HELP, NULL, NULL, 
      "Given a CHAIN of items, and the names of the "\
      "fields in that CHAIN that contain 'x' and 'y' values, "\
      "and a TRANS structure, apply the given TRANS coefficients "\
      "to every item in the CHAIN, calculating new 'x' and 'y' values. " },
   { "<chain>" , FTCL_ARGV_STRING, NULL, NULL, "chain of items" },
   { "<xname>" , FTCL_ARGV_STRING, NULL, NULL, 
                 "name of field in chain which contains X coordinate" },
   { "<yname>" , FTCL_ARGV_STRING, NULL, NULL, 
                 "name of field in chain which contains Y coordinate" },
   { "<trans>" , FTCL_ARGV_STRING, NULL, NULL, 
                 "TRANS structure to be applied to items in CHAIN " },
   { NULL, FTCL_ARGV_END, NULL, NULL, NULL }
};
**/

static char *g_atApplyTrans = "atApplyTrans";

static int
tclAtApplyTrans(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE chain_handle, trans_handle;
   char *sval0, *sval1, *sval2, *sval3;
   char xname[30], yname[30];
   CHAIN *chain;
   TRANS *trans;
   void *vptr;
   
   shErrStackClear();

   tclAtApplyTrans_opts[1].dst = &sval0;
   tclAtApplyTrans_opts[2].dst = &sval1;
   tclAtApplyTrans_opts[3].dst = &sval2;
   tclAtApplyTrans_opts[4].dst = &sval3;

   if (shTclParseArgv(interp, &argc, argv, tclAtApplyTrans_opts,
	       FTCL_ARGV_NO_LEFTOVERS, g_atApplyTrans) != FTCL_ARGV_SUCCESS) {
      return(TCL_ERROR);
   }

   /* now process the arguments */
   if (shTclHandleExprEval(interp, sval0, &chain_handle, &vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if (chain_handle.type != shTypeGetFromName("CHAIN")) {
      Tcl_SetResult(interp, "applyTrans: first arg is not a CHAIN", TCL_STATIC);
      return(TCL_ERROR);
   }
   chain = chain_handle.ptr;

   strcpy(xname, sval1);
   strcpy(yname, sval2);

   if (shTclHandleExprEval(interp, sval3, &trans_handle, &vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if (trans_handle.type != shTypeGetFromName("TRANS")) {
      Tcl_SetResult(interp, "applyTrans: fifth arg is not a TRANS", TCL_STATIC);
      return(TCL_ERROR);
   }
   trans = trans_handle.ptr;

   /* now call the C function that performs the transformation. */
 
   if (atApplyTrans(chain, xname, yname, trans) != SH_SUCCESS) {
      Tcl_SetResult(interp, "applyTrans: atApplyTrans returns with error",
	       TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, "", TCL_STATIC);
   return(TCL_OK);
}

/************************************************************************
 * <AUTO EXTRACT>
 *
 * TCL VERB: atMatchChains
 *
 * <HTML>
 * C ROUTINE CALLED: <a href="atPMatch.html#atMatchChains">atMatchChains</a> in atPMatch.c
 * </HTML>
 *
 * DESCRIPTION:
 * given two CHAINs of items, the names of the fields we use to
 * perform the match, and a matching radius,
 * find all "matches", where a "match" is defined as follows:
 * We calculate the Euclidean distance between pairs of items
 * from each CHAIN.  For each item of CHAIN A, we find all
 * possible matches (such that distance < radius) on CHAIN B.
 * We then select the _single_ item that yields the minimum distance.
 *
 * Thus, matches are exclusive; no single item is allowed to belong
 * to more than one match.
 *
 * place the elems of A that are matches into output chain J
 *                    B that are matches into output chain K
 *                    A that are not matches into output chain L
 *                    B that are not matches into output chain M
 *
 * RETURN:
 *    TCL_OK             if all goes well
 *    TCL_ERROR          if an error occurs
 *
 * </AUTO>
 */


static ftclArgvInfo tclAtMatchChains_opts[] = {
   { NULL, FTCL_ARGV_HELP, NULL, NULL, 
      "Given two chains of items (A and B), and the names of the "\
      "fields in both that contain 'x' and 'y' values, "\
      "and a matching radius, "\
      "find all exclusive matches of items in the two chains. "\
      "Create output chains as follows: "\
      "place the elems of A that are matches into output chain J "\
      "place the elems of B that are matches into output chain K "\
      "place the elems of A that are not matches into output chain L "\
      "place the elems of B that are not matches into output chain M " },
   { "<chainA>", FTCL_ARGV_STRING, NULL, NULL, "first input chain of items" },
   { "<xnameA>", FTCL_ARGV_STRING, NULL, NULL, 
                 "name of field in chain A which contains X coordinate" },
   { "<ynameA>", FTCL_ARGV_STRING, NULL, NULL, 
                 "name of field in chain A which contains Y coordinate" },
   { "<chainB>", FTCL_ARGV_STRING, NULL, NULL, "second input chain of items" },
   { "<xnameB>", FTCL_ARGV_STRING, NULL, NULL, 
                 "name of field in chain B which contains X coordinate" },
   { "<ynameB>", FTCL_ARGV_STRING, NULL, NULL, 
                 "name of field in chain B which contains Y coordinate" },
   { "<radius>", FTCL_ARGV_DOUBLE, NULL, NULL, 
                 "max allowed radius for match" },
   { "<chainJ>", FTCL_ARGV_STRING, NULL, NULL, 
                 "output chain of items, those in A that matched" },
   { "<chainK>", FTCL_ARGV_STRING, NULL, NULL, 
                 "output chain of items, those in B that matched" },
   { "<chainL>", FTCL_ARGV_STRING, NULL, NULL, 
                 "output chain of items, those in A that didn't match" },
   { "<chainM>", FTCL_ARGV_STRING, NULL, NULL, 
                 "output chain of items, those in B that didn't match" },
   { NULL, FTCL_ARGV_END, NULL, NULL, NULL }
};

static char *g_atMatchChains = "atMatchChains";

static int
tclAtMatchChains(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE chainA_handle, chainB_handle;
   char *sval0, *sval1, *sval2, *sval3, *sval4, *sval5;
   char *sval6, *sval7, *sval8, *sval9;
   char nameJ[HANDLE_NAMELEN], nameK[HANDLE_NAMELEN], nameL[HANDLE_NAMELEN];
   char nameM[HANDLE_NAMELEN];
   char xnameA[30], ynameA[30];
   char xnameB[30], ynameB[30];
   CHAIN *chainA, *chainB;
   CHAIN *chainJ, *chainK, *chainL, *chainM;
   void *vptr;
   double dval0;
   float radius;
   
   shErrStackClear();

   tclAtMatchChains_opts[1].dst = &sval0;
   tclAtMatchChains_opts[2].dst = &sval1;
   tclAtMatchChains_opts[3].dst = &sval2;
   tclAtMatchChains_opts[4].dst = &sval3;
   tclAtMatchChains_opts[5].dst = &sval4;
   tclAtMatchChains_opts[6].dst = &sval5;
   tclAtMatchChains_opts[7].dst = &dval0;
   tclAtMatchChains_opts[8].dst = &sval6;
   tclAtMatchChains_opts[9].dst = &sval7;
   tclAtMatchChains_opts[10].dst = &sval8;
   tclAtMatchChains_opts[11].dst = &sval9;

   if (shTclParseArgv(interp, &argc, argv, tclAtMatchChains_opts,
	    FTCL_ARGV_NO_LEFTOVERS, g_atMatchChains) != FTCL_ARGV_SUCCESS) {
      return(TCL_ERROR);
   }

   /* now process the arguments */
   if (shTclHandleExprEval(interp, sval0, &chainA_handle, &vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if (chainA_handle.type != shTypeGetFromName("CHAIN")) {
      Tcl_SetResult(interp, "matchChains: first arg is not a CHAIN", 
	       TCL_STATIC);
      return(TCL_ERROR);
   }
   chainA = chainA_handle.ptr;

   strcpy(xnameA, sval1);
   strcpy(ynameA, sval2);

   if (shTclHandleExprEval(interp, sval3, &chainB_handle, &vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if (chainB_handle.type != shTypeGetFromName("CHAIN")) {
      Tcl_SetResult(interp, "matchChains: fourth arg is not a CHAIN", 
	       TCL_STATIC);
      return(TCL_ERROR);
   }
   chainB = chainB_handle.ptr;

   strcpy(xnameB, sval4);
   strcpy(ynameB, sval5);

   radius = (float) dval0;


   /* now call the C function that performs the matching. */
 
   if (atMatchChains(chainA, xnameA, ynameA, chainB, xnameB, ynameB,
                     radius,
                     &chainJ, &chainK, &chainL, &chainM) != SH_SUCCESS) {
      Tcl_SetResult(interp, "matchChains: atMatchChains returns with error",
	       TCL_STATIC);
      return(TCL_ERROR);
   }

   /* 
    * all went well, so we must create handles for the four 
    * new output CHAINs 
    */

   if (shTclHandleNew(interp, nameJ, "CHAIN", (void *) chainJ) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      Tcl_SetResult(interp, "can't bind to new chainJ", TCL_STATIC);
      shChainDel(chainJ); shChainDel(chainK);
      shChainDel(chainL); shChainDel(chainM);
      return(TCL_ERROR);
   }
   if (Tcl_SetVar(interp, sval6, nameJ, 0) == NULL) {
      shTclInterpAppendWithErrStack(interp);
      Tcl_SetResult(interp, "can't set chainJ arg to new handle name", 
	       TCL_STATIC);
      shChainDel(chainJ); shChainDel(chainK);
      shChainDel(chainL); shChainDel(chainM);
      return(TCL_ERROR);
   }
   if (shTclHandleNew(interp, nameK, "CHAIN", (void *) chainK) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      Tcl_SetResult(interp, "can't bind to new chainK", TCL_STATIC);
      shChainDel(chainJ); shChainDel(chainK);
      shChainDel(chainL); shChainDel(chainM);
      return(TCL_ERROR);
   }
   if (Tcl_SetVar(interp, sval7, nameK, 0) == NULL) {
      shTclInterpAppendWithErrStack(interp);
      Tcl_SetResult(interp, "can't set chainK arg to new handle name",
	       TCL_STATIC);
      shChainDel(chainJ); shChainDel(chainK);
      shChainDel(chainL); shChainDel(chainM);
      return(TCL_ERROR);
   }
   if (shTclHandleNew(interp, nameL, "CHAIN", (void *) chainL) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      Tcl_SetResult(interp, "can't bind to new chainL", TCL_STATIC);
      shChainDel(chainJ); shChainDel(chainK);
      shChainDel(chainL); shChainDel(chainM);
      return(TCL_ERROR);
   }
   if (Tcl_SetVar(interp, sval8, nameL, 0) == NULL) {
      shTclInterpAppendWithErrStack(interp);
      Tcl_SetResult(interp, "can't set chainL arg to new handle name",
	       TCL_STATIC);
      shChainDel(chainJ); shChainDel(chainK);
      shChainDel(chainL); shChainDel(chainM);
      return(TCL_ERROR);
   }
   if (shTclHandleNew(interp, nameM, "CHAIN", (void *) chainM) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      Tcl_SetResult(interp, "can't bind to new chainM", TCL_STATIC);
      shChainDel(chainJ); shChainDel(chainK);
      shChainDel(chainL); shChainDel(chainM);
      return(TCL_ERROR);
   }
   if (Tcl_SetVar(interp, sval9, nameM, 0) == NULL) {
      shTclInterpAppendWithErrStack(interp);
      Tcl_SetResult(interp, "can't set chainM arg to new handle name",
	       TCL_STATIC);
      shChainDel(chainJ); shChainDel(chainK);
      shChainDel(chainL); shChainDel(chainM);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}


/************************************************************************
 * 
 *
 * ROUTINE: atTclPMatchDeclare
 *
 * DESCRIPTION:
 * Declares the TCL verbs in this file to the interpreter.
 *
 * RETURNS:
 *   nothing
 *
 * </AUTO>
 */


void
atTclPMatchDeclare(Tcl_Interp *interp)
{

   int flags = FTCL_ARGV_NO_LEFTOVERS;

   shTclDeclare(interp, "atFindTrans",
                (Tcl_CmdProc *) tclAtFindTrans,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *) NULL,
                module, 
                shTclGetArgInfo(interp, tclAtFindTrans_opts, flags, 
                          g_atFindTrans),
                shTclGetUsage(interp, tclAtFindTrans_opts, flags, 
                          g_atFindTrans));

   shTclDeclare(interp, "atApplyTrans",
                (Tcl_CmdProc *) tclAtApplyTrans,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *) NULL,
                module, 
                shTclGetArgInfo(interp, tclAtApplyTrans_opts, flags, 
                          g_atApplyTrans),
                shTclGetUsage(interp, tclAtApplyTrans_opts, flags, 
                          g_atApplyTrans));


   shTclDeclare(interp, "atMatchChains",
                (Tcl_CmdProc *) tclAtMatchChains,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *) NULL,
                module, 
                shTclGetArgInfo(interp, tclAtMatchChains_opts, flags, 
                          g_atMatchChains),
                shTclGetUsage(interp, tclAtMatchChains_opts, flags, 
                          g_atMatchChains));


}
