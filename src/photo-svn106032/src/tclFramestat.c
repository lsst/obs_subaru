/*
 * TCL support for FIELDSTAT type in photo.
 */
#include <string.h>
#include "dervish.h"
#include "atTrans.h"			/* for atFilternames */
#include "phUtils.h"
#include "phFramestat.h"

static char *module = "phTclStructsFacil";    /* name of this set of code */

/*****************************************************************************/
/*
 * return a handle to a new FIELDSTAT
 */
static char *tclFieldstatNew_use =
  "USAGE: fieldstatNew";
static char *tclFieldstatNew_hlp =
  "create a new FIELDSTAT";

static int
tclFieldstatNew(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE handle;
   char name[HANDLE_NAMELEN];

   shErrStackClear();

   if(argc != 1) {
      Tcl_SetResult(interp,tclFieldstatNew_use,TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * ok, get a handle for our new FIELDSTAT
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   handle.ptr = phFieldstatNew();
   handle.type = shTypeGetFromName("FIELDSTAT");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new fieldstat handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete an FIELDSTAT 
 */
static char *tclFieldstatDel_use =
  "USAGE: fieldstatDel fieldstat ";
static char *tclFieldstatDel_hlp =
  "Delete an FIELDSTAT";

static int
tclFieldstatDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *fieldstat;
   char *opts = "fieldstat";

   shErrStackClear();

   ftclParseSave("fieldstatDel");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      fieldstat = ftclGetStr("fieldstat");
      if(p_shTclHandleAddrGet(interp,fieldstat,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclFieldstatDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phFieldstatDel(handle->ptr);
   (void) p_shTclHandleDel(interp,fieldstat);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Clear a fieldstat
 */
static char *tclFieldstatClear_use =
  "USAGE: FieldstatClear <fieldstat>";
#define tclFieldstatClear_hlp \
  "Clear all of the fields in a fieldstat"

static ftclArgvInfo fieldstatClear_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFieldstatClear_hlp},
   {"<fieldstat>", FTCL_ARGV_STRING, NULL, NULL, "the FIELDSTAT to clear"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFieldstatClear(
		  ClientData clientDatag,
		  Tcl_Interp *interp,
		  int ac,
		  char **av
		  )
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *fieldstatStr = NULL;		/* the FIELDSTAT to clear */

   shErrStackClear();

   i = 1;
   fieldstatClear_opts[i++].dst = &fieldstatStr;
   shAssert(fieldstatClear_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,fieldstatClear_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * Process arguments
 */
   if(shTclHandleExprEval(interp,fieldstatStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FIELDSTAT")) {
      Tcl_SetResult(interp,"fieldstatClear: "
                    "argument is not a FIELDSTAT",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * and do the work
 */
   phFieldstatClear(hand.ptr);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Put the filter information in a FIELDSTAT
 */
static char *tclFieldstatFilterSet_use =
  "USAGE: fieldstatFilterSet fieldstat filterlist";
static char *tclFieldstatFilterSet_hlp =
  "Put the filter information in a FIELDSTAT";

static int
tclFieldstatFilterSet(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
  FIELDSTAT *fieldstat = NULL;
  char *string;
  char *opts = " fieldstat filterlist";
  int l_argc;
  char **l_argv;

  shErrStackClear();

  ftclParseSave("fieldstatFilterSet");
  if(ftclFullParseArg(opts,argc,argv) != 0) {
    string = ftclGetStr("fieldstat");
    if(shTclAddrGetFromName(interp,string,(void**)&fieldstat,"FIELDSTAT")
      !=TCL_OK) {
      return(TCL_ERROR);
    }
    /* Make the ascii list accessible to C */
    if (Tcl_SplitList(interp, argv[2], &l_argc, &l_argv) == TCL_ERROR) {
      Tcl_SetResult(interp,"Error parsing list of filters",TCL_VOLATILE);
      return (TCL_ERROR);
    }
  } else {
      Tcl_SetResult(interp,tclFieldstatFilterSet_use,TCL_STATIC);
      return(TCL_ERROR);
   }
#if 0
  for (i=0;i<l_argc;i++) {
    strncpy(fieldstat->filter[i], l_argv[i], FILTER_MAXNAME);
  }
#endif
  Tcl_SetResult(interp, argv[1], TCL_VOLATILE);
  return(TCL_OK);
}

/*****************************************************************************/
/*
 * Now FIELDPARAMS stuff
 */
static char *tclFieldparamsNew_use =
  "USAGE: FieldparamsNew ncolor";
#define tclFieldparamsNew_hlp \
  "Create a FIELDPARAMS"

static ftclArgvInfo fieldParamsNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFieldparamsNew_hlp},
   {"<filters>", FTCL_ARGV_STRING, NULL,NULL, "Names of bands being processed"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFieldparamsNew(
		  ClientData clientDatag,
		  Tcl_Interp *interp,
		  int ac,
		  char **av
		  )
{
   int i;
   FIELDPARAMS *fp;
   HANDLE hand;
   char name[HANDLE_NAMELEN];
   char *filters;			/* names of filters */

   shErrStackClear();

   i = 1;
   fieldParamsNew_opts[i++].dst = &filters;
   shAssert(fieldParamsNew_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,fieldParamsNew_opts) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * do the work
 */
   fp = phFieldparamsNew(filters);
/*
 * and return it to tcl
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = fp;
   hand.type = shTypeGetFromName("FIELDPARAMS");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new FIELDPARAMS handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclFieldparamsDel_use =
  "USAGE: FieldparamsDel <field_params>";
#define tclFieldparamsDel_hlp \
  "Destroy a FIELDPARAMS"

static ftclArgvInfo fieldParamsDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFieldparamsDel_hlp},
   {"<field_params>", FTCL_ARGV_STRING, NULL, NULL,
						  "The FIELDPARAMS to destroy"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFieldparamsDel(
		  ClientData clientDatag,
		  Tcl_Interp *interp,
		  int ac,
		  char **av
		  )
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *field_paramsStr = NULL;	/* The FIELDPARAMS to destroy */

   shErrStackClear();

   i = 1;
   fieldParamsDel_opts[i++].dst = &field_paramsStr;
   shAssert(fieldParamsDel_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,fieldParamsDel_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * process arguments
 */
   if(shTclHandleExprEval(interp,field_paramsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FIELDPARAMS")) {
      Tcl_SetResult(interp,"fieldParamsDel: "
                    "argument is not a FIELDPARAMS",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * and do the work
 */
   phFieldparamsDel(hand.ptr);
   p_shTclHandleDel(interp,field_paramsStr);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclFrameparamsNew_use =
  "USAGE: frameparamsNew <filter>";
#define tclFrameparamsNew_hlp \
  "make a new FRAMESTAT for band <filter> (can be \"0\")"

static ftclArgvInfo frameparamsNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFrameparamsNew_hlp},
   {"<filter>", FTCL_ARGV_STRING, NULL, NULL,
			 "Filter name for band, e.g. \"g\" or \"r\"; or \"0\""},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define frameparamsNew_name "frameparamsNew"

static int
tclFrameparamsNew(ClientData clientData,
		Tcl_Interp *interp,
		int ac,
		char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   char *filterStr = NULL;		/* name of band, e.g. g or r; or 0 */
   FRAMEPARAMS *fparams;		/* the FRAMEPARAMS to return */

   shErrStackClear();

   i = 1;
   frameparamsNew_opts[i++].dst = &filterStr;
   shAssert(frameparamsNew_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, frameparamsNew_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     frameparamsNew_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(strlen(filterStr) != 1) {
      Tcl_AppendResult(interp, "Filtername must have only one character: ",
		       filterStr, NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   fparams = phFrameparamsNew(*filterStr);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = fparams;
   hand.type = shTypeGetFromName("FRAMEPARAMS");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"fparams\" to new FRAMEPARAMS handle",
		    TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   return(TCL_OK);
}


/*****************************************************************************/
static char *tclFrameparamsDel_use =
  "USAGE: FrameparamsDel <fparams>";
#define tclFrameparamsDel_hlp \
  ""

static ftclArgvInfo frameparamsDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFrameparamsDel_hlp},
   {"<fparams>", FTCL_ARGV_STRING, NULL, NULL, "FRAMEPARAMS to delete"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclFrameparamsDel(
		  ClientData clientDatag,
		  Tcl_Interp *interp,
		  int ac,
		  char **av
		  )
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *fparamsStr = NULL;		/* FRAMEPARAMS to delete */

   shErrStackClear();

   i = 1;
   frameparamsDel_opts[i++].dst = &fparamsStr;
   shAssert(frameparamsDel_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,frameparamsDel_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,fparamsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FRAMEPARAMS")) {
      Tcl_SetResult(interp,"frameparamsDel: "
                    "argument is not a FRAMEPARAMS",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phFrameparamsDel(hand.ptr);
   p_shTclHandleDel(interp,fparamsStr);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclSoftBiasGet_use =
  "USAGE: softBiasGet";
#define tclSoftBiasGet_hlp \
  "Return the value of the SOFT_BIAS"

static ftclArgvInfo softBiasGet_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclSoftBiasGet_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define softBiasGet_name "softBiasGet"

static int
tclSoftBiasGet(ClientData clientData,
	       Tcl_Interp *interp,
	       int ac,
	       char **av)
{
   static char soft_bias[10] = { '\0' };
   int a_i;

   shErrStackClear();

   a_i = 1;
   shAssert(softBiasGet_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, softBiasGet_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     softBiasGet_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   if(*soft_bias == '\0') {
      sprintf(soft_bias, "%d", SOFT_BIAS);
   }

   Tcl_SetResult(interp, soft_bias, TCL_STATIC);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclPhGain_use =
  "USAGE: phGain <frameparams> <row> <col>";
#define tclPhGain_hlp \
  ""

static ftclArgvInfo phGain_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPhGain_hlp},
   {"<frameparams>", FTCL_ARGV_STRING, NULL, NULL, "FRAMEPARAMS for image"},
   {"<row>", FTCL_ARGV_DOUBLE, NULL, NULL, "Row position to evaluate gain at"},
   {"<col>", FTCL_ARGV_DOUBLE, NULL, NULL, "Column position to evaluate gain at"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define phGain_name "phGain"

static int
tclPhGain(ClientData clientData,
	  Tcl_Interp *interp,
	  int ac,
	  char **av)
{
     HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *frameparamsStr = NULL;		/* FRAMEPARAMS for image */
   FRAMEPARAMS *frameparams;
   double row = 0.0;			/* Row position to evaluate gain at */
   double col = 0.0;			/* Column position to evaluate gain at */

   shErrStackClear();

   a_i = 1;
   phGain_opts[a_i++].dst = &frameparamsStr;
   phGain_opts[a_i++].dst = &row;
   phGain_opts[a_i++].dst = &col;
   shAssert(phGain_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, phGain_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     phGain_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,frameparamsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FRAMEPARAMS")) {
      Tcl_AppendResult(interp,"phGain: "
                       "argument \"", frameparamsStr, "\" is not a FRAMEPARAMS",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   frameparams = hand.ptr;
/*
 * work
 */
   {
	float gain = phGain(frameparams, row, col);
	char gainStr[30];
	sprintf(gainStr, "%g", gain);

	Tcl_AppendResult(interp, gainStr, (char *)NULL);
   }
   
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclPhDarkVariance_use =
  "USAGE: phDarkVariance <frameparams> <row> <col>";
#define tclPhDarkVariance_hlp \
  ""

static ftclArgvInfo phDarkVariance_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPhDarkVariance_hlp},
   {"<frameparams>", FTCL_ARGV_STRING, NULL, NULL, "FRAMEPARAMS for image"},
   {"<row>", FTCL_ARGV_DOUBLE, NULL, NULL, "Row position to evaluate gain at"},
   {"<col>", FTCL_ARGV_DOUBLE, NULL, NULL, "Column position to evaluate gain at"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define phDarkVariance_name "phDarkVariance"

static int
tclPhDarkVariance(ClientData clientData,
	  Tcl_Interp *interp,
	  int ac,
	  char **av)
{
     HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *frameparamsStr = NULL;		/* FRAMEPARAMS for image */
   FRAMEPARAMS *frameparams;
   double row = 0.0;			/* Row position to evaluate gain at */
   double col = 0.0;			/* Column position to evaluate gain at */

   shErrStackClear();

   a_i = 1;
   phDarkVariance_opts[a_i++].dst = &frameparamsStr;
   phDarkVariance_opts[a_i++].dst = &row;
   phDarkVariance_opts[a_i++].dst = &col;
   shAssert(phDarkVariance_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, phDarkVariance_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     phDarkVariance_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,frameparamsStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("FRAMEPARAMS")) {
      Tcl_AppendResult(interp,"phDarkVariance: "
                       "argument \"", frameparamsStr, "\" is not a FRAMEPARAMS",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   frameparams = hand.ptr;
/*
 * work
 */
   {
	float gain = phDarkVariance(frameparams, row, col);
	char gainStr[30];
	sprintf(gainStr, "%g", gain);

	Tcl_AppendResult(interp, gainStr, (char *)NULL);
   }
   
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclFieldstatDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"fieldstatNew",
                (Tcl_CmdProc *)tclFieldstatNew,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclFieldstatNew_hlp, tclFieldstatNew_use);

   shTclDeclare(interp,"fieldstatDel",
                (Tcl_CmdProc *)tclFieldstatDel,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclFieldstatDel_hlp, tclFieldstatDel_use);

   shTclDeclare(interp,"fieldstatClear",
		(Tcl_CmdProc *)tclFieldstatClear, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFieldstatClear_hlp,
		tclFieldstatClear_use);

   shTclDeclare(interp,"fieldstatFilterSet",
                (Tcl_CmdProc *)tclFieldstatFilterSet,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclFieldstatFilterSet_hlp, 
		tclFieldstatFilterSet_use);

   shTclDeclare(interp,"fieldparamsNew",
		(Tcl_CmdProc *)tclFieldparamsNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFieldparamsNew_hlp,
		tclFieldparamsNew_use);

   shTclDeclare(interp,"fieldparamsDel",
		(Tcl_CmdProc *)tclFieldparamsDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFieldparamsDel_hlp,
		tclFieldparamsDel_use);

   shTclDeclare(interp,frameparamsNew_name,
		(Tcl_CmdProc *)tclFrameparamsNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFrameparamsNew_hlp,
		tclFrameparamsNew_use);

   shTclDeclare(interp,"frameparamsDel",
		(Tcl_CmdProc *)tclFrameparamsDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFrameparamsDel_hlp,
		tclFrameparamsDel_use);

   shTclDeclare(interp,softBiasGet_name,
		(Tcl_CmdProc *)tclSoftBiasGet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclSoftBiasGet_hlp,
		tclSoftBiasGet_use);

   shTclDeclare(interp,phGain_name,
		(Tcl_CmdProc *)tclPhGain, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPhGain_hlp,
		tclPhGain_use);

   shTclDeclare(interp,phDarkVariance_name,
		(Tcl_CmdProc *)tclPhDarkVariance, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPhDarkVariance_hlp,
		tclPhDarkVariance_use);
}

