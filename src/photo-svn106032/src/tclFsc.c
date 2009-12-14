#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dervish.h"
#include "phFsc.h"

/*****************************************************************************/

static char *tclStampNew_use =
  "USAGE: stampNew";
#define tclStampNew_hlp \
  "Return a new SCSTAMP"

static ftclArgvInfo stampNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclStampNew_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define stampNew_name "stampNew"

static int
tclStampNew(ClientData clientData,
	    Tcl_Interp *interp,
	    int ac,
	    char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int a_i;
   SCSTAMP *stamp;

   shErrStackClear();

   a_i = 1;
   shAssert(stampNew_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, stampNew_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     stampNew_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   stamp = phFscStampNew();
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = stamp;
   hand.type = shTypeGetFromName("SCSTAMP");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"stamp\" to new SCSTAMP handle",
                                                                   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   
   return(TCL_OK);
}

static char *tclStampDel_use =
  "USAGE: stampDel <stamp>";
#define tclStampDel_hlp \
  "Delete an SCSTAMP"

static ftclArgvInfo stampDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclStampDel_hlp},
   {"<stamp>", FTCL_ARGV_STRING, NULL, NULL, "SCSTAMP to delete"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define stampDel_name "stampDel"

static int
tclStampDel(ClientData clientData,
	    Tcl_Interp *interp,
	    int ac,
	    char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *stampStr = NULL;		/* SCSTAMP to delete */
   SCSTAMP *stamp;

   shErrStackClear();

   a_i = 1;
   stampDel_opts[a_i++].dst = &stampStr;
   shAssert(stampDel_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, stampDel_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     stampDel_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,stampStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("SCSTAMP")) {
      Tcl_AppendResult(interp,"stampDel: "
                       "argument \"", stampStr, "\" is not a SCSTAMP",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   stamp = hand.ptr;
/*
 * work
 */
   phFscStampDel(stamp);
   p_shTclHandleDel(interp,stampStr);

   return(TCL_OK);
}

static char *tclStampChainDel_use =
  "USAGE: stampChainDel <chain>";
#define tclStampChainDel_hlp \
  "Delete a chain of SCSTAMPs, deleting the stamps along with the chain"

static ftclArgvInfo stampChainDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclStampChainDel_hlp},
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL, "Chain of SCSTAMPs to destroy"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define stampChainDel_name "stampChainDel"

static int
tclStampChainDel(ClientData clientData,
		 Tcl_Interp *interp,
		 int ac,
		 char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *chainStr = NULL;		/* Chain of SCSTAMPs to destroy */
   CHAIN *chain;

   shErrStackClear();

   a_i = 1;
   stampChainDel_opts[a_i++].dst = &chainStr;
   shAssert(stampChainDel_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, stampChainDel_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     stampChainDel_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,chainStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_AppendResult(interp,"stampChainDel: "
                       "argument \"", chainStr, "\" is not a CHAIN",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   chain = hand.ptr;

   if(chain->type != shTypeGetFromName("SCSTAMP")) {
      Tcl_AppendResult(interp,
                       "CHAIN \"", chainStr, "\" doesn't have type SCSTAMP",
								 (char *)NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phFscStampChainDel(chain);
   p_shTclHandleDel(interp,chainStr);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclFstampNew_use =
  "USAGE: fstampNew";
#define tclFstampNew_hlp \
  "Create a new SCFSTAMP"

static ftclArgvInfo fstampNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFstampNew_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define fstampNew_name "fstampNew"

static int
tclFstampNew(ClientData clientData,
	     Tcl_Interp *interp,
	     int ac,
	     char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int a_i;
   SCFSTAMP *fstamp;

   shErrStackClear();

   a_i = 1;
   shAssert(fstampNew_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, fstampNew_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     fstampNew_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   fstamp = phFscFstampNew();
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = fstamp;
   hand.type = shTypeGetFromName("SCFSTAMP");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"fstamp\" to new SCFSTAMP handle",
                                                                   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   
   return(TCL_OK);
}

static char *tclFstampDel_use =
  "USAGE: fstampDel <fstamp>";
#define tclFstampDel_hlp \
  "Destroy an SCFSTAMP"

static ftclArgvInfo fstampDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFstampDel_hlp},
   {"<fstamp>", FTCL_ARGV_STRING, NULL, NULL, "SCFSTAMP to delete"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define fstampDel_name "fstampDel"

static int
tclFstampDel(ClientData clientData,
	     Tcl_Interp *interp,
	     int ac,
	     char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *fstampStr = NULL;		/* SCFSTAMP to delete */
   SCFSTAMP *fstamp;

   shErrStackClear();

   a_i = 1;
   fstampDel_opts[a_i++].dst = &fstampStr;
   shAssert(fstampDel_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, fstampDel_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     fstampDel_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
if(shTclHandleExprEval(interp,fstampStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("SCFSTAMP")) {
      Tcl_AppendResult(interp,"fstampDel: "
                       "argument \"", fstampStr, "\" is not a SCFSTAMP",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   fstamp = hand.ptr;
/*
 * work
 */
   phFscFstampDel(fstamp);
   p_shTclHandleDel(interp,fstampStr);

   return(TCL_OK);
}

static char *tclFstampChainDel_use =
  "USAGE: fstampChainDel <chain>";
#define tclFstampChainDel_hlp \
  "Destroy a chain of SCFSTAMPs, deleting the stamps as well as the chain"

static ftclArgvInfo fstampChainDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFstampChainDel_hlp},
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL, "Chain of SCFSTAMPs to destroy"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define fstampChainDel_name "fstampChainDel"

static int
tclFstampChainDel(ClientData clientData,
		  Tcl_Interp *interp,
		  int ac,
		  char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *chainStr = NULL;		/* Chain of SCFSTAMPs to destroy */
   CHAIN *chain;

   shErrStackClear();

   a_i = 1;
   fstampChainDel_opts[a_i++].dst = &chainStr;
   shAssert(fstampChainDel_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, fstampChainDel_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     fstampChainDel_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,chainStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_AppendResult(interp,"fstampChainDel: "
                       "argument \"", chainStr, "\" is not a CHAIN",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   chain = hand.ptr;

   if(chain->type != shTypeGetFromName("SCFSTAMP")) {
      Tcl_AppendResult(interp,
                       "CHAIN \"", chainStr, "\" doesn't have type SCFSTAMP",
								 (char *)NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phFscFstampChainDel(chain);
   p_shTclHandleDel(interp,chainStr);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclStampToFstamp_use =
  "USAGE: stampToFstamp <stamp>";
#define tclStampToFstamp_hlp \
  "Return an SCFTAMP corresponding to an SCSTAMP"

static ftclArgvInfo stampToFstamp_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclStampToFstamp_hlp},
   {"<stamp>", FTCL_ARGV_STRING, NULL, NULL, "SCSTAMP to convert"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define stampToFstamp_name "stampToFstamp"

static int
tclStampToFstamp(ClientData clientData,
		 Tcl_Interp *interp,
		 int ac,
		 char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *stampStr = NULL;		/* SCSTAMP to convert */
   SCFSTAMP *fstamp;
   SCSTAMP *stamp;

   shErrStackClear();

   a_i = 1;
   stampToFstamp_opts[a_i++].dst = &stampStr;
   shAssert(stampToFstamp_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, stampToFstamp_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     stampToFstamp_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,stampStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("SCSTAMP")) {
      Tcl_AppendResult(interp,"stampToFstamp: "
                       "argument \"", stampStr, "\" is not a SCSTAMP",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   stamp = hand.ptr;
/*
 * work
 */
   fstamp = phFscStampToFstamp(stamp);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = fstamp;
   hand.type = shTypeGetFromName("SCFSTAMP");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"fstamp\" to new SCFSTAMP handle",
                                                                   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   
   return(TCL_OK);
}

static char *tclStampChainToFstampChain_use =
  "USAGE: stampChainToFstampChain <chain>";
#define tclStampChainToFstampChain_hlp \
  "Given a chain of SCSTAMPs, return a chain of SCFSTAMPs"

static ftclArgvInfo stampChainToFstampChain_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclStampChainToFstampChain_hlp},
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL, "Chain of SCSTAMPs"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define stampChainToFstampChain_name "stampChainToFstampChain"

static int
tclStampChainToFstampChain(ClientData clientData,
			   Tcl_Interp *interp,
			   int ac,
			   char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *chainStr = NULL;		/* Chain of SCSTAMPs */
   CHAIN *chain;
   CHAIN *fchain;

   shErrStackClear();

   a_i = 1;
   stampChainToFstampChain_opts[a_i++].dst = &chainStr;
   shAssert(stampChainToFstampChain_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, stampChainToFstampChain_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     stampChainToFstampChain_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,chainStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_AppendResult(interp,"stampChainToFstampChain: "
                       "argument \"", chainStr, "\" is not a CHAIN",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   chain = hand.ptr;

   if(chain->type != shTypeGetFromName("SCSTAMP")) {
      Tcl_AppendResult(interp,
                       "CHAIN \"", chainStr, "\" doesn't have type SCSTAMP",
								 (char *)NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   fchain = phFscStampChainToFstampChain(chain);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = fchain;
   hand.type = shTypeGetFromName("CHAIN");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"fchain\" to new CHAIN handle",
                                                                   TCL_STATIC);
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
phTclFscDeclare(Tcl_Interp *interp)
{
   char *module = "phTclFsc";	/* name of this set of code */

   shTclDeclare(interp,stampNew_name,
		(Tcl_CmdProc *)tclStampNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclStampNew_hlp,
		tclStampNew_use);

   shTclDeclare(interp,stampDel_name,
		(Tcl_CmdProc *)tclStampDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclStampDel_hlp,
		tclStampDel_use);

   shTclDeclare(interp,fstampNew_name,
		(Tcl_CmdProc *)tclFstampNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFstampNew_hlp,
		tclFstampNew_use);

   shTclDeclare(interp,fstampDel_name,
		(Tcl_CmdProc *)tclFstampDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFstampDel_hlp,
		tclFstampDel_use);

   shTclDeclare(interp,stampToFstamp_name,
		(Tcl_CmdProc *)tclStampToFstamp, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclStampToFstamp_hlp,
		tclStampToFstamp_use);

   shTclDeclare(interp,stampChainToFstampChain_name,
		(Tcl_CmdProc *)tclStampChainToFstampChain, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclStampChainToFstampChain_hlp,
		tclStampChainToFstampChain_use);

   shTclDeclare(interp,stampChainDel_name,
		(Tcl_CmdProc *)tclStampChainDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclStampChainDel_hlp,
		tclStampChainDel_use);

   shTclDeclare(interp,fstampChainDel_name,
		(Tcl_CmdProc *)tclFstampChainDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFstampChainDel_hlp,
		tclFstampChainDel_use);
}
