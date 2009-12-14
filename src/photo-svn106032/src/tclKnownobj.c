/*
 * TCL support for KNOWNOBJ type in photo.
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <alloca.h>
#include "dervish.h"
#include "phUtils.h"
#include "phKnownobj.h"

static char *module = "phKnownobj";    /* name of this set of code */

/*****************************************************************************/
/*
 * return a handle to a new KNOWNOBJ
 */
static char *tclKnownobjNew_use =
  "USAGE: knownobjNew";
static char *tclKnownobjNew_hlp =
  "create a new KNOWNOBJ";

static int
tclKnownobjNew(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE handle;
   KNOWNOBJ *ko;
   char name[HANDLE_NAMELEN];

   shErrStackClear();

   if(argc != 1) {
      Tcl_SetResult(interp,tclKnownobjNew_use,TCL_STATIC);
      return(TCL_ERROR);
   }

/*
 * ok, get a handle for our new KNOWNOBJ
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   ko = phKnownobjNew();
   handle.ptr = ko;
   handle.type = shTypeGetFromName("KNOWNOBJ");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new knownobj handle",TCL_STATIC);
      return(TCL_ERROR);
   }


   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete a KNOWNOBJ 
 */
static char *tclKnownobjDel_use =
  "USAGE: knownobjDel knownobj ";
static char *tclKnownobjDel_hlp =
  "Delete an KNOWNOBJ";

static int
tclKnownobjDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *knownobj;
   char *opts = "knownobj";

   shErrStackClear();

   ftclParseSave("knownobjDel");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      knownobj = ftclGetStr("knownobj");
      if(p_shTclHandleAddrGet(interp,knownobj,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclKnownobjDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phKnownobjDel(handle->ptr);
   (void) p_shTclHandleDel(interp,knownobj);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclInfofileRead_use =
  "USAGE: infofileRead ";
#define tclInfofileRead_hlp \
  ""

static ftclArgvInfo infofileRead_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclInfofileRead_hlp},
   {"<file>", FTCL_ARGV_STRING, NULL, NULL, "Info file to read"},
   {"<fmt>", FTCL_ARGV_STRING, NULL, NULL, "Format to use to read file"},
   {"<which>", FTCL_ARGV_STRING, NULL, NULL,
				       "string giving order of fields in file"},
   {"[binsize]", FTCL_ARGV_DOUBLE, NULL, NULL,
					    "Binning to use in row and column"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclInfofileRead(
		ClientData clientDatag,
		Tcl_Interp *interp,
		int ac,
		char **av
		)
{
   int i;
   int which[3 + INFO_OBJ_NPROP];
   char *fileStr = NULL;		/* Info file to read */
   char *fmtStr = NULL;			/* Format to use to read file */
   char *whichStr = NULL;		/* order of fields in file */
   double binsize = 0.1;		/* Binning to use in row and column */

   shErrStackClear();

   i = 1;
   infofileRead_opts[i++].dst = &fileStr;
   infofileRead_opts[i++].dst = &fmtStr;
   infofileRead_opts[i++].dst = &whichStr;
   infofileRead_opts[i++].dst = &binsize;
   shAssert(infofileRead_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,infofileRead_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   {
      char *el;				/* element of string whichStr */
      int i;
      char *tmp = alloca(strlen(whichStr) + 1);
      
      strcpy(tmp, whichStr);
      for(i = 0; (el = strtok(tmp, " \t\n")) != NULL; i++) {
	 if(i == 3 + INFO_OBJ_NPROP) {
	    break;
	 }
	 which[i] = atoi(el);

	 tmp = NULL;			/* as needed by strtok */
      }

      while(i < 3 + INFO_OBJ_NPROP) {
	 which[i++] = -1;
      }
   }
/*
 * work
 */
   if(phInfofileRead(fileStr, fmtStr, which, binsize) != SH_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclInfofileFini_use =
  "USAGE: InfofileFini ";
#define tclInfofileFini_hlp \
  "Clean up after infofileRead and infofileMatch"

static ftclArgvInfo infofileFini_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclInfofileFini_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclInfofileFini(
		ClientData clientDatag,
		Tcl_Interp *interp,
		int ac,
		char **av
		)
{
   shErrStackClear();

   if(get_FtclOpts(interp,&ac,av,infofileFini_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * work
 */
   phInfofileFini();

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclInfofileMatch_use =
  "USAGE: InfofileMatch ";
#define tclInfofileMatch_hlp \
  "Return a best-match INFO_OBJ"

static ftclArgvInfo infofileMatch_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclInfofileMatch_hlp},
   {"<row>", FTCL_ARGV_DOUBLE, NULL, NULL, "Row centre of object"},
   {"<column>", FTCL_ARGV_DOUBLE, NULL, NULL, "Column centre of object"},
   {"<dmax>", FTCL_ARGV_DOUBLE, NULL, NULL, "Maximum allowable separation"},
   {"-dist", FTCL_ARGV_STRING, NULL, NULL,
		       "Name of variable to return distance to matched object"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclInfofileMatch(
		 ClientData clientDatag,
		 Tcl_Interp *interp,
		 int ac,
		 char **av
		 )
{
   int i;
   float dist;				/* distance between position and
					   that of object in catalog */
   HANDLE hand;
   const INFO_OBJ *info;		/* returned info */
   char name[HANDLE_NAMELEN];
   double row = 0.0;			/* Row centre of object */
   double column = 0.0;			/* Column centre of object */
   double dmax = 0.0;			/* Maximum allowable separation */
   char *distStr = NULL;		/* variable to return distance */
   
   shErrStackClear();

   i = 1;
   infofileMatch_opts[i++].dst = &row;
   infofileMatch_opts[i++].dst = &column;
   infofileMatch_opts[i++].dst = &dmax;
   infofileMatch_opts[i++].dst = &distStr;
   shAssert(infofileMatch_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,infofileMatch_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * work
 */
   if((info = phInfofileMatch(row, column, dmax, &dist)) == NULL) {
      Tcl_SetResult(interp, "phInfofileMatch returns NULL" ,TCL_STATIC);
      return(TCL_ERROR);
   }

   if(distStr != NULL) {
      char str[40];
      
      sprintf(str,"%g",dist);
      if(Tcl_SetVar(interp,distStr,str,TCL_LEAVE_ERR_MSG) == NULL) {
	 Tcl_SetResult(interp,"Failed to return dist",TCL_VOLATILE);
	 return(TCL_ERROR);
      }
   }
/*
 * return a handle to info
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = (INFO_OBJ *)info;
   hand.type = shTypeGetFromName("INFO_OBJ");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new INFO_OBJ handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclInfofileSetFromChain_use =
  "USAGE: infofileSetFromChain <chain> [binsize]";
#define tclInfofileSetFromChain_hlp \
  "Given a chain of INFO_OBJs, copy the objects into internal memory and\n"\
"index it for use with infofileMatch.  The chain may be safely destroyed\n"\
"after this routine returns"

static ftclArgvInfo infofileSetFromChain_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclInfofileSetFromChain_hlp},
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL, "Chain of INFO_OBJs"},
   {"[binsize]", FTCL_ARGV_DOUBLE, NULL, NULL, "Size of bins to use"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define infofileSetFromChain_name "infofileSetFromChain"

static int
tclInfofileSetFromChain(ClientData clientData,
			Tcl_Interp *interp,
			int ac,
			char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *chainStr = NULL;		/* Chain of INFO_OBJs */
   CHAIN *chain;
   double binsize = 0.1;		/* Size of bins to use */

   shErrStackClear();

   a_i = 1;
   infofileSetFromChain_opts[a_i++].dst = &chainStr;
   infofileSetFromChain_opts[a_i++].dst = &binsize;
   shAssert(infofileSetFromChain_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, infofileSetFromChain_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     infofileSetFromChain_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_AppendResult(interp,"infofileSetFromChain: "
                       "argument \"", chainStr, "\" is not a CHAIN",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   chain = hand.ptr;

   if(chain->type != shTypeGetFromName("INFO_OBJ")) {
      Tcl_AppendResult(interp,
                       "CHAIN \"", chainStr, "\" doesn't have type INFO_OBJ",
								 (char *)NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   if(phInfofileSetFromChain(chain, binsize) != SH_SUCCESS) {
      Tcl_SetResult(interp, "infofileSetFromChain: ", TCL_STATIC);
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
phTclKnownobjDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"knownobjNew",
                (Tcl_CmdProc *)tclKnownobjNew,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclKnownobjNew_hlp, tclKnownobjNew_use);

   shTclDeclare(interp,"knownobjDel",
                (Tcl_CmdProc *)tclKnownobjDel,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclKnownobjDel_hlp, tclKnownobjDel_use);

   shTclDeclare(interp,"infofileRead",
		(Tcl_CmdProc *)tclInfofileRead, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclInfofileRead_hlp,
		tclInfofileRead_use);

   shTclDeclare(interp,"infofileFini",
		(Tcl_CmdProc *)tclInfofileFini, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclInfofileFini_hlp,
		tclInfofileFini_use);

   shTclDeclare(interp,"infofileMatch",
		(Tcl_CmdProc *)tclInfofileMatch, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclInfofileMatch_hlp,
		tclInfofileMatch_use);

   shTclDeclare(interp,infofileSetFromChain_name,
		(Tcl_CmdProc *)tclInfofileSetFromChain, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclInfofileSetFromChain_hlp,
		tclInfofileSetFromChain_use);
}
