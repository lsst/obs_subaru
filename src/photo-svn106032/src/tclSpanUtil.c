/*
 * TCL interface for SPANMASK functions.
 */

#include "dervish.h"
#include "phUtils.h"
#include "phSpanUtil.h"
#include "phDataIo.h"

static char *module = "tclSpanUtil";

static char *tclObjmaskNew_use = "USAGE: objmaskNew";
#define tclObjmaskNew_hlp "Create an empty OBJMASK."\
"Returns an OBJMASK."

static ftclArgvInfo objmaskNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjmaskNew_hlp},
   {"-nspan", FTCL_ARGV_INT, NULL, NULL, "Initial number of spans"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjmaskNew(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
    int i;
    OBJMASK *om;
    HANDLE maskHandle;
    char maskName[HANDLE_NAMELEN];
    int nspan = 0;			/* Initial number of spans */

    shErrStackClear();

    i = 1;
    objmaskNew_opts[i++].dst = &nspan;
    shAssert(objmaskNew_opts[i].type == FTCL_ARGV_END);

    if(get_FtclOpts(interp, &argc, argv, objmaskNew_opts) != TCL_OK) {
       return(TCL_ERROR);
    }
/*
 * Not too hard.
 */
    om = phObjmaskNew(nspan);
/*
 * Now pack up the results.
 */
   if(p_shTclHandleNew(interp, maskName) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      phObjmaskDel(om);
      
      return(TCL_ERROR);
   }
   maskHandle.ptr = om;
   maskHandle.type = shTypeGetFromName("OBJMASK");

   if (p_shTclHandleAddrBind(interp, maskHandle, maskName) != TCL_OK) {
      Tcl_SetResult(interp, "can't bind to new OBJMASK handle", TCL_STATIC);
      phObjmaskDel(om);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, maskName, TCL_VOLATILE);
   return(TCL_OK);
}

static char *tclObjmaskDel_use = "USAGE: objmaskDel mask";
#define tclObjmaskDel_hlp "Delete a Objmask."

static ftclArgvInfo objmaskDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjmaskDel_hlp},
   {"<mask>", FTCL_ARGV_STRING, NULL, NULL, "Mask to delete"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjmaskDel(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
    int i;
    OBJMASK *om;
    HANDLE inputHandle;
    void *vptr;
    char *maskStr = NULL;		/* Mask to delete */

    shErrStackClear();

    i = 1;
    objmaskDel_opts[i++].dst = &maskStr;
    shAssert(objmaskDel_opts[i].type == FTCL_ARGV_END);

    if(get_FtclOpts(interp, &argc, argv, objmaskDel_opts) != TCL_OK) {

       return(TCL_ERROR);
    }
/*
 * Process the arguments
 */
    if(shTclHandleExprEval(interp, maskStr, &inputHandle, &vptr) != TCL_OK) {
       return(TCL_ERROR);
    }
    if(inputHandle.type != shTypeGetFromName("OBJMASK")) {
       Tcl_SetResult(interp,"tclObjmaskDel: arg is not an OBJMASK",
		     TCL_STATIC);
       return(TCL_ERROR);
    }
    om = inputHandle.ptr;
/*
 * Not too hard.
 */
    p_shTclHandleDel(interp,maskStr);
    phObjmaskDel(om);

   return(TCL_OK);
}

static char *tclSpanmaskNew_use = "USAGE: spanmaskNew [-nrow # -ncol #]";
#define tclSpanmaskNew_hlp "Create an empty spanmask; optionally set its"\
"(logical) size to nrow*ncol. Returns a Spanmask."

static ftclArgvInfo spanmaskNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclSpanmaskNew_hlp},
   {"-nrow", FTCL_ARGV_INT, NULL, NULL, "Number of rows in SPANMASK"},
   {"-ncol", FTCL_ARGV_INT, NULL, NULL, "Number of columns in SPANMASK"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclSpanmaskNew(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
    int i;
    SPANMASK *sm;
    HANDLE maskHandle;
    char maskName[HANDLE_NAMELEN];
    int nrow = 0;			/* Number of rows in SPANMASK */
    int ncol = 0;			/* Number of columns in SPANMASK */

    shErrStackClear();

    i = 1;
    spanmaskNew_opts[i++].dst = &nrow;
    spanmaskNew_opts[i++].dst = &ncol;
    shAssert(spanmaskNew_opts[i].type == FTCL_ARGV_END);
    
    if(get_FtclOpts(interp, &argc, argv, spanmaskNew_opts) != TCL_OK) {

       return(TCL_ERROR);
    }
/*
 * Not too hard.
 */
    sm = phSpanmaskNew(nrow, ncol);
/*
 * Now pack up the results.
 */
   if(p_shTclHandleNew(interp, maskName) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      phSpanmaskDel(sm);
      
      return(TCL_ERROR);
   }
   maskHandle.ptr = sm;
   maskHandle.type = shTypeGetFromName("SPANMASK");

   if (p_shTclHandleAddrBind(interp, maskHandle, maskName) != TCL_OK) {
      Tcl_SetResult(interp, "can't bind to new SPANMASK handle", TCL_STATIC);
      phSpanmaskDel(sm);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, maskName, TCL_VOLATILE);
   return(TCL_OK);
}

static char *tclSpanmaskDel_use = "USAGE: spanmaskDel mask";
#define tclSpanmaskDel_hlp "Delete a Spanmask."

static ftclArgvInfo spanmaskDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclSpanmaskDel_hlp},
   {"<mask>", FTCL_ARGV_STRING, NULL, NULL, "Mask to delete"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclSpanmaskDel(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
    int i;
    SPANMASK *sm;
    HANDLE inputHandle;
    void *vptr;
    char *maskStr = NULL;		/* Mask to delete */

    shErrStackClear();

    i = 1;
    spanmaskDel_opts[i++].dst = &maskStr;
    shAssert(spanmaskDel_opts[i].type == FTCL_ARGV_END);

    if(get_FtclOpts(interp, &argc, argv, spanmaskDel_opts) != TCL_OK) {

       return(TCL_ERROR);
    }
/*
 * Process the arguments
 */
    if(shTclHandleExprEval(interp, maskStr, &inputHandle, &vptr) != TCL_OK) {
       return(TCL_ERROR);
    }
    if(inputHandle.type != shTypeGetFromName("SPANMASK")
	&& inputHandle.type != shTypeGetFromName("MASK")) {
       Tcl_SetResult(interp,"tclSpanmaskDel: arg is not a SPANMASK or MASK",
		     TCL_STATIC);
       return(TCL_ERROR);
    }
    sm = inputHandle.ptr;
    if(!sm) {
       Tcl_SetResult(interp,"tclSpanmaskDel: arg is NULL.",
		     TCL_STATIC);
       return(TCL_ERROR);
    }
    if(sm->cookie != SPAN_COOKIE) {
       Tcl_SetResult(interp,"tclSpanmaskDel: arg is not a SPANMASK.",
		     TCL_STATIC);
       return(TCL_ERROR);
    }
/*
 * Not too hard.
 */
    p_shTclHandleDel(interp,maskStr);
    phSpanmaskDel(sm);

   return(TCL_OK);
}

static char *tclSpanmaskSetAsPix_use = "USAGE: spanmaskSetAsPix mask row col type";
#define tclSpanmaskSetAsPix_hlp "Set a given pixel to a given type in a SPANMASK."

static ftclArgvInfo spanmaskSetAsPix_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclSpanmaskSetAsPix_hlp},
   {"<mask>", FTCL_ARGV_STRING, NULL, NULL, "Mask to use"},
   {"<row>", FTCL_ARGV_INT, NULL, NULL, "row of pixel"},
   {"<col>", FTCL_ARGV_INT, NULL, NULL, "col of pixel"},
   {"<type>", FTCL_ARGV_INT, NULL, NULL, "type of mask to set"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclSpanmaskSetAsPix(
	       ClientData clientData,
	       Tcl_Interp *interp,
	       int argc,
	       char **argv
	       )
{
   int i;
    SPANMASK *sm;
    HANDLE inputHandle;
    void *vptr;
   char *maskStr = NULL;		/* Mask to use */
   int row = 0;				/* row of pixel */
   int col = 0;				/* col of pixel */
   int type = 0;			/* type of mask to set */

    shErrStackClear();

   i = 1;
   spanmaskSetAsPix_opts[i++].dst = &maskStr;
   spanmaskSetAsPix_opts[i++].dst = &row;
   spanmaskSetAsPix_opts[i++].dst = &col;
   spanmaskSetAsPix_opts[i++].dst = &type;
   shAssert(spanmaskSetAsPix_opts[i].type == FTCL_ARGV_END);

    if(get_FtclOpts(interp, &argc, argv, spanmaskSetAsPix_opts) != TCL_OK) {

       return(TCL_ERROR);
    }
/*
 * Process the arguments
 */
    if(shTclHandleExprEval(interp, maskStr, &inputHandle, &vptr) != TCL_OK) {
       return(TCL_ERROR);
    }
    if(inputHandle.type != shTypeGetFromName("SPANMASK")
	&& inputHandle.type != shTypeGetFromName("MASK")) {
       Tcl_SetResult(interp,"tclSpanmaskSetAsPix: arg is not a SPANMASK.",
		     TCL_STATIC);
       return(TCL_ERROR);
    }
    sm = inputHandle.ptr;
    if(!sm) {
       Tcl_SetResult(interp,"tclSpanmaskDel: arg is NULL.",
		     TCL_STATIC);
       return(TCL_ERROR);
    }
    if(sm->cookie != SPAN_COOKIE) {
       Tcl_SetResult(interp,"tclSpanmaskSetAsPix: arg is not a SPANMASK.",
		     TCL_STATIC);
       return(TCL_ERROR);
    }

    phSpanmaskSetAsPix(sm, row, col, (S_MASKTYPE)type);
   return(TCL_OK);
}

/*****************************************************************************/
static char *tclMaskSetFromObjmaskChain_use =
  "USAGE: MaskSetFromObjmaskChain chain mask val";
#define tclMaskSetFromObjmaskChain_hlp \
  "OR <val> into the specified mask"

static ftclArgvInfo maskSetFromObjmaskChain_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclMaskSetFromObjmaskChain_hlp},
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL, "An OBJMASK chain"},
   {"<mask>", FTCL_ARGV_STRING, NULL, NULL, "The mask to set"},
   {"<val>", FTCL_ARGV_INT, NULL, NULL, "The value to OR into the mask"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclMaskSetFromObjmaskChain(
			   ClientData clientDatag,
			   Tcl_Interp *interp,
			   int ac,
			   char **av
			   )
{
   int i;
   CHAIN *chain;
   HANDLE hand;
   MASK *mask;
   void *vptr;				/* used by shTclHandleExprEval */
   char *chainStr = NULL;		/* An OBJMASK chain */
   char *maskStr = NULL;		/* The mask to set */
   int val = 0;				/* The value to OR into the mask */

   shErrStackClear();

   i = 1;
   maskSetFromObjmaskChain_opts[i++].dst = &chainStr;
   maskSetFromObjmaskChain_opts[i++].dst = &maskStr;
   maskSetFromObjmaskChain_opts[i++].dst = &val;
   shAssert(maskSetFromObjmaskChain_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,maskSetFromObjmaskChain_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,chainStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_SetResult(interp,"maskSetFromObjmaskChain: "
                    "first argument is not a CHAIN",TCL_STATIC);
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_SetResult(interp,"maskSetFromObjmaskChain: "
                    "first argument is not a CHAIN",TCL_STATIC);
      return(TCL_ERROR);
   }
   chain = hand.ptr;
   if(chain->type != shTypeGetFromName("OBJMASK")) {
      Tcl_SetResult(interp,"maskSetFromObjmaskChain: "
                    "CHAIN is not of type OBJMASK",TCL_STATIC);
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,maskStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("MASK")) {
      Tcl_SetResult(interp,"maskSetFromObjmaskChain: "
                    "second argument is not a MASK",TCL_STATIC);
      return(TCL_ERROR);
   }
   mask = hand.ptr;
/*
 * do the work
 */
   phMaskSetFromObjmaskChain(chain,mask,val);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclRegSetFromObjmaskChain_use =
  "USAGE: regSetFromObjmaskChain <chain> <reg> <val>";
#define tclRegSetFromObjmaskChain_hlp \
  "Set values of an integer region from a chain of OBJMASKs"

static ftclArgvInfo regSetFromObjmaskChain_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegSetFromObjmaskChain_hlp},
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL, "An OBJMASK chain"},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "The region to set"},
   {"<val>", FTCL_ARGV_INT, NULL, NULL, "The value to OR into the mask"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define regSetFromObjmaskChain_name "regSetFromObjmaskChain"

static int
tclRegSetFromObjmaskChain(ClientData clientData,
			  Tcl_Interp *interp,
			  int ac,
			  char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *chainStr = NULL;		/* An OBJMASK chain */
   CHAIN *chain;
   char *regStr = NULL;			/* The region to set */
   REGION *reg;
   unsigned int val = 0;		/* The value to OR into the mask */

   shErrStackClear();

   a_i = 1;
   regSetFromObjmaskChain_opts[a_i++].dst = &chainStr;
   regSetFromObjmaskChain_opts[a_i++].dst = &regStr;
   regSetFromObjmaskChain_opts[a_i++].dst = (int *)&val;
   shAssert(regSetFromObjmaskChain_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, regSetFromObjmaskChain_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     regSetFromObjmaskChain_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_AppendResult(interp,"regSetFromObjmaskChain: "
                       "argument \"", chainStr, "\" is not a CHAIN",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   chain = hand.ptr;

   if(chain->type != shTypeGetFromName("OBJMASK")) {
      Tcl_AppendResult(interp,
                       "CHAIN \"", chainStr, "\" doesn't have type OBJMASK",
								 (char *)NULL);
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_AppendResult(interp,"regSetFromObjmaskChain: "
                       "argument \"", regStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(reg->type != TYPE_U16) {
      Tcl_SetResult(interp, "regSetFromObjmaskChain: "
		    "REGION must be of type U16", TCL_STATIC);
      return(TCL_ERROR);
   }
   if(val > MAX_U16) {
      Tcl_SetResult(interp, "regSetFromObjmaskChain: "
		    "Value is too large to fit into a U16 region", TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phRegSetFromObjmaskChain(chain, reg, val);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclMaskSetFromObjmask_use =
  "USAGE: MaskSetFromObjmask <objmask> <mask> <val>";
#define tclMaskSetFromObjmask_hlp \
  ""

static ftclArgvInfo maskSetFromObjmask_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclMaskSetFromObjmask_hlp},
   {"<objmask>", FTCL_ARGV_STRING, NULL, NULL, "An OBJMASK"},
   {"<mask>", FTCL_ARGV_STRING, NULL, NULL, "The mask to set"},
   {"<val>", FTCL_ARGV_INT, NULL, NULL, "The value to OR into the mask"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclMaskSetFromObjmask(
		      ClientData clientDatag,
		      Tcl_Interp *interp,
		      int ac,
		      char **av
		      )
{
   int i;
   HANDLE hand;
   MASK *mask;
   OBJMASK *om;
   void *vptr;				/* used by shTclHandleExprEval */
   char *objmaskStr = NULL;		/* An OBJMASK */
   char *maskStr = NULL;		/* The mask to set */
   int val = 0;				/* The value to OR into the mask */

   shErrStackClear();

   i = 1;
   maskSetFromObjmask_opts[i++].dst = &objmaskStr;
   maskSetFromObjmask_opts[i++].dst = &maskStr;
   maskSetFromObjmask_opts[i++].dst = &val;
   shAssert(maskSetFromObjmask_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,maskSetFromObjmask_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,objmaskStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJMASK")) {
      Tcl_SetResult(interp,"maskSetFromObjmask: "
                    "first argument is not a OBJMASK",TCL_STATIC);
      return(TCL_ERROR);
   }
   om = hand.ptr;

   if(shTclHandleExprEval(interp,maskStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("MASK")) {
      Tcl_SetResult(interp,"maskSetFromObjmask: "
                    "second argument is not a MASK",TCL_STATIC);
      return(TCL_ERROR);
   }
   mask = hand.ptr;
/*
 * work
 */
   phMaskSetFromObjmask(om, mask, val);

   return(TCL_OK);
}


/*****************************************************************************/
/*
 * merge a chain of OBJMASKs into a single OBJMASK
 */
static char *tclMergeObjmaskChain_use =
  "USAGE: MergeObjmaskChain chain";
#define tclMergeObjmaskChain_hlp \
  "merge a chain of OBJMASKs into a single OBJMASK, which is returned"

static ftclArgvInfo mergeObjmaskChain_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclMergeObjmaskChain_hlp},
   {"<ochain>", FTCL_ARGV_STRING, NULL, NULL, "A chain of OBJMASKs"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclMergeObjmaskChain(
		     ClientData clientDatag,
		     Tcl_Interp *interp,
		     int ac,
		     char **av
		     )
{
   int i;
   CHAIN *ochain;
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   OBJMASK *om;
   void *vptr;				/* used by shTclHandleExprEval */
   char *ochainStr = NULL;		/* A chain of OBJMASKs */

   shErrStackClear();

   i = 1;
   mergeObjmaskChain_opts[i++].dst = &ochainStr;
   shAssert(mergeObjmaskChain_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,mergeObjmaskChain_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * process arguments
 */
   if(shTclHandleExprEval(interp,ochainStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_SetResult(interp,"mergeObjmaskChain: "
                    "argument is not a CHAIN",TCL_STATIC);
      return(TCL_ERROR);
   }
   ochain = hand.ptr;
   if(ochain->type != shTypeGetFromName("OBJMASK")) {
      Tcl_SetResult(interp,"mergeObjmaskChain: "
                    "CHAIN is not of type OBJMASK",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * do the work
 */
   om = phMergeObjmaskChain(ochain);
/*
 * and return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = om;
   hand.type = shTypeGetFromName("OBJMASK");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new OBJMASK handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclObjmaskSetFromReg_use =
  "USAGE: ObjmaskSetFromReg <om> <region>";
#define tclObjmaskSetFromReg_hlp \
  "Set the OBJMASK <om>'s data field from the <region>"

static ftclArgvInfo objmaskSetFromReg_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjmaskSetFromReg_hlp},
   {"<om>", FTCL_ARGV_STRING, NULL, NULL, "OBJMASK to set"},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region to set values from"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjmaskSetFromReg(
			ClientData clientDatag,
			Tcl_Interp *interp,
			int ac,
			char **av
			)
{
   int i;
   HANDLE hand;
   OBJMASK *om;
   REGION *reg;
   void *vptr;				/* used by shTclHandleExprEval */
   char *omStr = NULL;			/* OBJMASK to set */
   char *regStr = NULL;			/* Region to set values from */

   shErrStackClear();

   i = 1;
   objmaskSetFromReg_opts[i++].dst = &omStr;
   objmaskSetFromReg_opts[i++].dst = &regStr;
   shAssert(objmaskSetFromReg_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objmaskSetFromReg_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,omStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJMASK")) {
      Tcl_SetResult(interp,"objmaskSetFromReg: "
                    "first argument is not a OBJMASK",TCL_STATIC);
      return(TCL_ERROR);
   }
   om = hand.ptr;

   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"objmaskSetFromReg: "
                    "second argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;
/*
 * work
 */
   phObjmaskSetFromRegion(om,reg);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclRegSetFromObjmask_use =
  "USAGE: RegSetFromObjmask <region> <om>";
#define tclRegSetFromObjmask_hlp \
  "Set values in <region> from the data in OBJMASK <om>"

static ftclArgvInfo regSetFromObjmask_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegSetFromObjmask_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region to set"},
   {"<om>", FTCL_ARGV_STRING, NULL, NULL, "OBJMASK with values"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclRegSetFromObjmask(
		     ClientData clientDatag,
		     Tcl_Interp *interp,
		     int ac,
		     char **av
		     )
{
   int i;
   HANDLE hand;
   OBJMASK *om;
   REGION *reg;
   void *vptr;				/* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region to set */
   char *omStr = NULL;			/* OBJMASK with values */

   shErrStackClear();

   i = 1;
   regSetFromObjmask_opts[i++].dst = &regStr;
   regSetFromObjmask_opts[i++].dst = &omStr;
   shAssert(regSetFromObjmask_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,regSetFromObjmask_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"objmaskSetFromReg: "
                    "first argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(shTclHandleExprEval(interp,omStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJMASK")) {
      Tcl_SetResult(interp,"objmaskSetFromReg: "
                    "second argument is not a OBJMASK",TCL_STATIC);
      return(TCL_ERROR);
   }
   om = hand.ptr;
/*
 * work
 */
   phRegionSetFromObjmask(reg,om);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclRegSetValFromObjmask_use =
  "USAGE: RegSetValFromObjmask <region> <om> <val>";
#define tclRegSetValFromObjmask_hlp \
  "Set values in <region> to <val> wherever OBJMASK <om> is set"

static ftclArgvInfo regSetValFromObjmask_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegSetValFromObjmask_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region to set"},
   {"<om>", FTCL_ARGV_STRING, NULL, NULL, "OBJMASK with values"},
   {"<val>", FTCL_ARGV_INT, NULL, NULL, "OBJMASK with values"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclRegSetValFromObjmask(
		     ClientData clientDatag,
		     Tcl_Interp *interp,
		     int ac,
		     char **av
		     )
{
   int i;
   HANDLE hand;
   OBJMASK *om;
   REGION *reg;
   void *vptr;				/* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region to set */
   char *omStr = NULL;			/* OBJMASK with values */
   int val = 0;				/* OBJMASK with values */

   shErrStackClear();

   i = 1;
   regSetValFromObjmask_opts[i++].dst = &regStr;
   regSetValFromObjmask_opts[i++].dst = &omStr;
   regSetValFromObjmask_opts[i++].dst = &val;
   shAssert(regSetValFromObjmask_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,regSetValFromObjmask_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"objmaskSetValFromReg: "
                    "first argument is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(shTclHandleExprEval(interp,omStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJMASK")) {
      Tcl_SetResult(interp,"objmaskSetValFromReg: "
                    "second argument is not a OBJMASK",TCL_STATIC);
      return(TCL_ERROR);
   }
   om = hand.ptr;
/*
 * work
 */
   phRegionSetValFromObjmask(reg,om,val);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclObjmaskGrowToSuperpixel_use =
  "USAGE: ObjmaskGrowToSuperpixel <objmask> <n>";
#define tclObjmaskGrowToSuperpixel_hlp \
  "Return an objmask that is the initial one, grown so that it includes all"\
"the pixels in <n>x<n> superpixels containing the initial mask"

static ftclArgvInfo objmaskGrowToSuperpixel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjmaskGrowToSuperpixel_hlp},
   {"<objmask>", FTCL_ARGV_STRING, NULL, NULL, "The initial objmask"},
   {"<n>", FTCL_ARGV_INT, NULL, NULL, "Size of superpixels"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjmaskGrowToSuperpixel(
			   ClientData clientDatag,
			   Tcl_Interp *interp,
			   int ac,
			   char **av
			   )
{
   int i;
   OBJMASK *om;
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *objmaskStr = NULL;		/* The initial objmask */
   int n = 0;				/* Size of superpixels */

   shErrStackClear();

   i = 1;
   objmaskGrowToSuperpixel_opts[i++].dst = &objmaskStr;
   objmaskGrowToSuperpixel_opts[i++].dst = &n;
   shAssert(objmaskGrowToSuperpixel_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objmaskGrowToSuperpixel_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,objmaskStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJMASK")) {
      Tcl_SetResult(interp,"objmaskGrowToSuperpixel: "
                    "first argument is not a OBJMASK",TCL_STATIC);
      return(TCL_ERROR);
   }
   om = hand.ptr;
/*
 * work
 */
   om = phObjmaskGrowToSuperpixel(om, n);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = om;
   hand.type = shTypeGetFromName("OBJMASK");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new OBJMASK handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * test flattening/inflating OBJMASKs
 */
static char *tclObjmaskFlatten_use =
  "USAGE: objmaskFlatten <om>";
#define tclObjmaskFlatten_hlp \
  "Flatten an objmask into a network-byteorder char array"

static ftclArgvInfo objmaskFlatten_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjmaskFlatten_hlp},
   {"<om>", FTCL_ARGV_STRING, NULL, NULL, "The OBJMASK to flatten"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define objmaskFlatten_name "objmaskFlatten"

static int
tclObjmaskFlatten(ClientData clientData,
		  Tcl_Interp *interp,
		  int ac,
		  char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *omStr = NULL;			/* The OBJMASK to flatten */
   unsigned char *buff;			/* buffer to contain om */
   int len;				/* length of buff */
   OBJMASK *om;

   shErrStackClear();

   i = 1;
   objmaskFlatten_opts[i++].dst = &omStr;
   shAssert(objmaskFlatten_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, objmaskFlatten_opts,
		     0, objmaskFlatten_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,omStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJMASK")) {
      Tcl_SetResult(interp,"objmaskFlatten: "
                    "argument \"om\" is not a OBJMASK",TCL_STATIC);
      return(TCL_ERROR);
   }
   om = hand.ptr;
/*
 * work
 */
   len = phObjmaskFlatten(om, NULL, 0);
   buff = shMalloc(len);
   phObjmaskFlatten(om, buff, len);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = buff;
   hand.type = shTypeGetFromName("PTR");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"name\" to new PTR handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   
   return(TCL_OK);
}

static char *tclObjmaskInflate_use =
  "USAGE: objmaskInflate <buff>";
#define tclObjmaskInflate_hlp \
  "Given a handle (or type PTR) containing a flattened OBJMASK, return the OBJMASK"

static ftclArgvInfo objmaskInflate_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjmaskInflate_hlp},
   {"<buff>", FTCL_ARGV_STRING, NULL, NULL, "A PTR with a flattened OBJMASK"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define objmaskInflate_name "objmaskInflate"

static int
tclObjmaskInflate(ClientData clientData,
		  Tcl_Interp *interp,
		  int ac,
		  char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *buffStr = NULL;		/* A PTR with a flattened OBJMASK */
   unsigned char *buff;
   OBJMASK *om;				/* OBJMASK to return */

   shErrStackClear();

   i = 1;
   objmaskInflate_opts[i++].dst = &buffStr;
   shAssert(objmaskInflate_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, objmaskInflate_opts,
		     0, objmaskInflate_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,buffStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("PTR")) {
      Tcl_SetResult(interp,"objmaskInflate: "
                    "argument \"buff\" is not a PTR",TCL_STATIC);
      return(TCL_ERROR);
   }
   buff = hand.ptr;
/*
 * work
 */
   om = phObjmaskInflate(NULL, buff, NULL);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = om;
   hand.type = shTypeGetFromName("OBJMASK");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"name\" to new OBJMASK handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   
   return(TCL_OK);
}

/*****************************************************************************/
static char *tclObjmaskChainGrow_use =
  "USAGE: objmaskChainGrow <chain> <reg> <n>";
#define tclObjmaskChainGrow_hlp \
  "Grow all the OBJMASKs on a chain by <n> pixels in each direction"

static ftclArgvInfo objmaskChainGrow_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjmaskChainGrow_hlp},
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL, "CHAIN of OBJMASKs to be grown"},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "REGION wherein the OBJMASK dwell"},
   {"<n>", FTCL_ARGV_INT, NULL, NULL, "Number of pixels to grow OBJMASKs"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define objmaskChainGrow_name "objmaskChainGrow"

static int
tclObjmaskChainGrow(ClientData clientData,
		    Tcl_Interp *interp,
		    int ac,
		    char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *chainStr = NULL;		/* CHAIN of OBJMASKs to be grown */
   CHAIN *chain;
   char *regStr = NULL;			/* REGION wherein the OBJMASK dwell */
   REGION *reg;
   int n = 0;				/* Number of pixels to grow OBJMASKs */

   shErrStackClear();

   i = 1;
   objmaskChainGrow_opts[i++].dst = &chainStr;
   objmaskChainGrow_opts[i++].dst = &regStr;
   objmaskChainGrow_opts[i++].dst = &n;
   shAssert(objmaskChainGrow_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, objmaskChainGrow_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     objmaskChainGrow_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_AppendResult(interp,"objmaskChainGrow: "
                       "argument \"", chainStr, "\" is not a CHAIN",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   chain = hand.ptr;

   if(chain->type != shTypeGetFromName("OBJMASK")) {
      Tcl_AppendResult(interp,
                       "CHAIN \"", chainStr, "\" doesn't have type OBJMASK",
								 (char *)NULL);
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,regStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_AppendResult(interp,"objmaskChainGrow: "
                       "argument \"", regStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(reg->type != TYPE_PIX) {
      Tcl_AppendResult(interp, "REGION ", regStr, " is not of type U16",
		       (char *)NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phObjmaskChainGrow(chain, reg, n);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclObjmaskFromRect_use =
  "USAGE: objmaskFromRect <row0> <col0> <row1> <col1>";
#define tclObjmaskFromRect_hlp \
  "Return an objmask specifying a rectangle"

static ftclArgvInfo objmaskFromRect_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjmaskFromRect_hlp},
   {"<row0>", FTCL_ARGV_INT, NULL, NULL, "Starting row"},
   {"<col0>", FTCL_ARGV_INT, NULL, NULL, "Starting column"},
   {"<row1>", FTCL_ARGV_INT, NULL, NULL, "Ending row"},
   {"<col1>", FTCL_ARGV_INT, NULL, NULL, "Ending column"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define objmaskFromRect_name "objmaskFromRect"

static int
tclObjmaskFromRect(ClientData clientData,
		   Tcl_Interp *interp,
		   int ac,
		   char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int a_i;
   int row0 = 0;			/* Starting row */
   int col0 = 0;			/* Starting column */
   int row1 = 0;			/* Ending row */
   int col1 = 0;			/* Ending column */
   OBJMASK *om;

   shErrStackClear();

   a_i = 1;
   objmaskFromRect_opts[a_i++].dst = &row0;
   objmaskFromRect_opts[a_i++].dst = &col0;
   objmaskFromRect_opts[a_i++].dst = &row1;
   objmaskFromRect_opts[a_i++].dst = &col1;
   shAssert(objmaskFromRect_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, objmaskFromRect_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     objmaskFromRect_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(row1 < row0 || col1 < col0) {
      Tcl_SetResult(interp,
		    "objmaskFromRect: UR corner must be >= LL corner",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   om = phObjmaskFromRect(col0, row0, col1, row1);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = om;
   hand.type = shTypeGetFromName("OBJMASK");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"om\" to new OBJMASK handle",
                                                                   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclObjmaskFromCircle_use =
  "USAGE: objmaskFromCircle <rowc> <colc> <radius>";
#define tclObjmaskFromCircle_hlp \
  "Return an OBJMASK defining a circle of given centre and radius"

static ftclArgvInfo objmaskFromCircle_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjmaskFromCircle_hlp},
   {"<rowc>", FTCL_ARGV_INT, NULL, NULL, "Row-centre of mask"},
   {"<colc>", FTCL_ARGV_INT, NULL, NULL, "Column-centre of mask"},
   {"<radius>", FTCL_ARGV_INT, NULL, NULL, "Radius of mask"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define objmaskFromCircle_name "objmaskFromCircle"

static int
tclObjmaskFromCircle(ClientData clientData,
		     Tcl_Interp *interp,
		     int ac,
		     char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int a_i;
   int rowc = 0;			/* Row-centre of mask */
   int colc = 0;			/* Column-centre of mask */
   int radius = 0;			/* Radius of mask */
   OBJMASK *om;

   shErrStackClear();

   a_i = 1;
   objmaskFromCircle_opts[a_i++].dst = &rowc;
   objmaskFromCircle_opts[a_i++].dst = &colc;
   objmaskFromCircle_opts[a_i++].dst = &radius;
   shAssert(objmaskFromCircle_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, objmaskFromCircle_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     objmaskFromCircle_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   om = phObjmaskFromCircle(rowc, colc, radius);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = om;
   hand.type = shTypeGetFromName("OBJMASK");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"om\" to new OBJMASK handle",
                                                                   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclObjmaskChainAndObjmaskChain_use =
  "USAGE: objmaskChainAndObjmaskChain <ch1> <ch2>";
#define tclObjmaskChainAndObjmaskChain_hlp \
  "AND the second OBJMASK CHAIN into the first"

static ftclArgvInfo objmaskChainAndObjmaskChain_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjmaskChainAndObjmaskChain_hlp},
   {"<ch1>", FTCL_ARGV_STRING, NULL, NULL, "First OBJMASK chain (will be modified)"},
   {"<ch2>", FTCL_ARGV_STRING, NULL, NULL, "OBJMASK chain to AND into <ch1>"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define objmaskChainAndObjmaskChain_name "objmaskChainAndObjmaskChain"

static int
tclObjmaskChainAndObjmaskChain(ClientData clientData,
				   Tcl_Interp *interp,
				   int ac,
				   char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *ch1Str = NULL;	  /* First OBJMASK chain (will be modified) */
   CHAIN *ch1;
   char *ch2Str = NULL;		 /* OBJMASK chain to AND into <ch1> */
   CHAIN *ch2;

   shErrStackClear();

   a_i = 1;
   objmaskChainAndObjmaskChain_opts[a_i++].dst = &ch1Str;
   objmaskChainAndObjmaskChain_opts[a_i++].dst = &ch2Str;
   shAssert(objmaskChainAndObjmaskChain_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, objmaskChainAndObjmaskChain_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     objmaskChainAndObjmaskChain_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,ch1Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_AppendResult(interp,"objmaskChainAndObjmaskChain: "
                       "argument \"", ch1Str, "\" is not a CHAIN",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   ch1 = hand.ptr;
   if(ch1->type != shTypeGetFromName("OBJMASK")) {
      Tcl_AppendResult(interp,
                       "CHAIN \"", ch1Str, "\" doesn't have type OBJMASK",
								 (char *)NULL);
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,ch2Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_AppendResult(interp,"objmaskChainAndObjmaskChain: "
                       "argument \"", ch2Str, "\" is not a CHAIN",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   ch2 = hand.ptr;

   if(ch2->type != shTypeGetFromName("OBJMASK")) {
      Tcl_AppendResult(interp,
                       "CHAIN \"", ch2Str, "\" doesn't have type OBJMASK",
								 (char *)NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phObjmaskChainAndObjmaskChain(ch1, ch2);

   Tcl_AppendResult(interp, ch1, (char *)NULL);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclObjmaskMerge_use =
  "USAGE: objmaskMerge <objmask1> <objmask2> -drow dval -dcol dval";
#define tclObjmaskMerge_hlp \
  "Merge the second OBJMASK into the first"

static ftclArgvInfo objmaskMerge_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjmaskMerge_hlp},
   {"<objmask1>", FTCL_ARGV_STRING, NULL, NULL, "OBJMASK to be merged into"},
   {"<objmask2>", FTCL_ARGV_STRING, NULL, NULL, "OBJMASK to merge into objmask1"},
   {"-drow", FTCL_ARGV_DOUBLE, NULL, NULL, "Shift objmask2 by this many rows"},
   {"-dcol", FTCL_ARGV_DOUBLE, NULL, NULL, "Shift objmask2 by this many columns"},   
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define objmaskMerge_name "objmaskMerge"

static int
tclObjmaskMerge(ClientData clientData,
		Tcl_Interp *interp,
		int ac,
		char **av)
{
    HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *objmask1Str = NULL;		/* OBJMASK to be merged into */
   OBJMASK *objmask1;
   char *objmask2Str = NULL;		/* OBJMASK to merge into objmask1 */
   OBJMASK *objmask2;
   double drow = 0.0;			/* Shift objmask2 by this many rows */
   double dcol = 0.0;			/* Shift objmask2 by this many columns */

   shErrStackClear();

   a_i = 1;
   objmaskMerge_opts[a_i++].dst = &objmask1Str;
   objmaskMerge_opts[a_i++].dst = &objmask2Str;
   objmaskMerge_opts[a_i++].dst = &drow;
   objmaskMerge_opts[a_i++].dst = &dcol;
   shAssert(objmaskMerge_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, objmaskMerge_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     objmaskMerge_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,objmask1Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJMASK")) {
      Tcl_AppendResult(interp,"objmaskMerge: "
                       "argument \"", objmask1Str, "\" is not an OBJMASK",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   objmask1 = hand.ptr;

   if(shTclHandleExprEval(interp,objmask2Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJMASK")) {
      Tcl_AppendResult(interp,"objmaskMerge: "
                       "argument \"", objmask2Str, "\" is not an OBJMASK",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   objmask2 = hand.ptr;
/*
 * work
 */
   {
       OBJMASK *ret = phObjmaskMerge(objmask1, objmask2, drow, dcol);
       shAssert(ret == objmask1);
   }

   return(TCL_OK);
}

/************************************************************************************************************/

static char *tclPixIntersectObjmask_use =
  "USAGE: pixIntersectObjmask <objmask> <row> <col>";
#define tclPixIntersectObjmask_hlp \
  ""

static ftclArgvInfo pixIntersectObjmask_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPixIntersectObjmask_hlp},
   {"<objmask>", FTCL_ARGV_STRING, NULL, NULL, "OBJMASK to check for intersection"},
   {"<row>", FTCL_ARGV_DOUBLE, NULL, NULL, "Row-position of point"},
   {"<col>", FTCL_ARGV_DOUBLE, NULL, NULL, "Column-position of point"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define pixIntersectObjmask_name "pixIntersectObjmask"

static int
tclPixIntersectObjmask(ClientData clientData,
		       Tcl_Interp *interp,
		       int ac,
		       char **av)
{
    HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *objmaskStr = NULL;		/* OBJMASK to check for intersection */
   OBJMASK *objmask;
   double row = 0;			/* Row-position of point */
   double col = 0;			/* Column-position of point */

   shErrStackClear();

   a_i = 1;
   pixIntersectObjmask_opts[a_i++].dst = &objmaskStr;
   pixIntersectObjmask_opts[a_i++].dst = &row;
   pixIntersectObjmask_opts[a_i++].dst = &col;
   shAssert(pixIntersectObjmask_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, pixIntersectObjmask_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     pixIntersectObjmask_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,objmaskStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJMASK")) {
      Tcl_AppendResult(interp,"pixIntersectObjmask: "
                       "argument \"", objmaskStr, "\" is not an OBJMASK",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   objmask = hand.ptr;   
/*
 * work
 */
   {
       char buff[100];
       sprintf(buff, "%d", phPixIntersectObjmask(objmask, col, row));

       Tcl_AppendResult(interp, buff, (char *)NULL);
   }

   return(TCL_OK);
}

/************************************************************************************************************/

static char *tclObjmaskCopy_use =
  "USAGE: objmaskCopy <objmask> -drow dval -dcol dval";
#define tclObjmaskCopy_hlp \
  ""

static ftclArgvInfo objmaskCopy_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjmaskCopy_hlp},
   {"<objmask>", FTCL_ARGV_STRING, NULL, NULL, "Objmask to copy"},
   {"-drow", FTCL_ARGV_DOUBLE, NULL, NULL, "How much to shift objmask in row direction"},
   {"-dcol", FTCL_ARGV_DOUBLE, NULL, NULL, "How much to shift objmask in column direction"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define objmaskCopy_name "objmaskCopy"

static int
tclObjmaskCopy(ClientData clientData,
	       Tcl_Interp *interp,
	       int ac,
	       char **av)
{
   char name[HANDLE_NAMELEN];
    HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *objmaskStr = NULL;		/* Objmask to copy */
   OBJMASK *objmask;
   double drow = 0.0;			/* How much to shift objmask in row direction */
   double dcol = 0.0;			/* How much to shift objmask in column direction */

   shErrStackClear();

   a_i = 1;
   objmaskCopy_opts[a_i++].dst = &objmaskStr;
   objmaskCopy_opts[a_i++].dst = &drow;
   objmaskCopy_opts[a_i++].dst = &dcol;
   shAssert(objmaskCopy_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, objmaskCopy_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     objmaskCopy_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,objmaskStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJMASK")) {
      Tcl_AppendResult(interp,"objmaskCopy: "
                       "argument \"", objmaskStr, "\" is not an OBJMASK",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   objmask = hand.ptr;
/*
 * work
 */
   objmask = phObjmaskCopy(objmask, drow, dcol);

/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = objmask;
   hand.type = shTypeGetFromName("OBJMASK");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"objmask\" to new OBJMASK handle",
                                                                   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/************************************************************************************************************/

static char *tclObjmaskAndNotObjmask_use =
  "USAGE: objmaskAndNotObjmask <objmask1> <objmask2>";
#define tclObjmaskAndNotObjmask_hlp \
  ""

static ftclArgvInfo objmaskAndNotObjmask_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjmaskAndNotObjmask_hlp},
   {"<objmask1>", FTCL_ARGV_STRING, NULL, NULL, "Objmask which is to be modified"},
   {"<objmask2>", FTCL_ARGV_STRING, NULL, NULL, "Remove all pixels in this mask from objmask1"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define objmaskAndNotObjmask_name "objmaskAndNotObjmask"

static int
tclObjmaskAndNotObjmask(ClientData clientData,
			Tcl_Interp *interp,
			int ac,
			char **av)
{
    HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *objmask1Str = NULL;		/* Objmask which is to be modified */
   OBJMASK *objmask1;
   char *objmask2Str = NULL;		/* Remove all pixels in this mask from objmask1 */
   OBJMASK *objmask2;

   shErrStackClear();

   a_i = 1;
   objmaskAndNotObjmask_opts[a_i++].dst = &objmask1Str;
   objmaskAndNotObjmask_opts[a_i++].dst = &objmask2Str;
   shAssert(objmaskAndNotObjmask_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, objmaskAndNotObjmask_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     objmaskAndNotObjmask_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,objmask1Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJMASK")) {
      Tcl_AppendResult(interp,"objmaskAndNotObjmask: "
                       "argument \"", objmask1Str, "\" is not an OBJMASK",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   objmask1 = hand.ptr;

   if(shTclHandleExprEval(interp,objmask2Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJMASK")) {
      Tcl_AppendResult(interp,"objmaskAndNotObjmask: "
                       "argument \"", objmask2Str, "\" is not an OBJMASK",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   objmask2 = hand.ptr;
/*
 * work
 */
   (void)phObjmaskAndNotObjmask(objmask1, objmask2);

   return(TCL_OK);
}

/************************************************************************************************************/

static char *tclObjmaskAndObjmask_use =
  "USAGE: objmaskAndObjmask <objmask1> <objmask2>";
#define tclObjmaskAndObjmask_hlp \
  ""

static ftclArgvInfo objmaskAndObjmask_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjmaskAndObjmask_hlp},
   {"<objmask1>", FTCL_ARGV_STRING, NULL, NULL, "Objmask which is to be modified"},
   {"<objmask2>", FTCL_ARGV_STRING, NULL, NULL, "Remove all pixels in this mask from objmask1"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define objmaskAndObjmask_name "objmaskAndObjmask"

static int
tclObjmaskAndObjmask(ClientData clientData,
			Tcl_Interp *interp,
			int ac,
			char **av)
{
    HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *objmask1Str = NULL;		/* Objmask which is to be modified */
   OBJMASK *objmask1;
   char *objmask2Str = NULL;		/* Remove all pixels in this mask from objmask1 */
   OBJMASK *objmask2;

   shErrStackClear();

   a_i = 1;
   objmaskAndObjmask_opts[a_i++].dst = &objmask1Str;
   objmaskAndObjmask_opts[a_i++].dst = &objmask2Str;
   shAssert(objmaskAndObjmask_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, objmaskAndObjmask_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     objmaskAndObjmask_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,objmask1Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJMASK")) {
      Tcl_AppendResult(interp,"objmaskAndObjmask: "
                       "argument \"", objmask1Str, "\" is not an OBJMASK",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   objmask1 = hand.ptr;

   if(shTclHandleExprEval(interp,objmask2Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJMASK")) {
      Tcl_AppendResult(interp,"objmaskAndObjmask: "
                       "argument \"", objmask2Str, "\" is not an OBJMASK",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   objmask2 = hand.ptr;
/*
 * work
 */
   (void)phObjmaskAndObjmask(objmask1, objmask2);

   return(TCL_OK);
}

/*****************************************************************************/

void phTclSpanmaskDeclare(Tcl_Interp *interp)
{
    shTclDeclare(interp, "objmaskNew",
		 (Tcl_CmdProc *)tclObjmaskNew,
		 (ClientData) 0,
		 (Tcl_CmdDeleteProc *) NULL,
		 module,
		 tclObjmaskNew_hlp, tclObjmaskNew_use);
    shTclDeclare(interp, "objmaskDel",
		 (Tcl_CmdProc *)tclObjmaskDel,
		 (ClientData) 0,
		 (Tcl_CmdDeleteProc *) NULL,
		 module,
		 tclObjmaskDel_hlp, tclObjmaskDel_use);
    shTclDeclare(interp, "spanmaskNew",
		 (Tcl_CmdProc *)tclSpanmaskNew,
		 (ClientData) 0,
		 (Tcl_CmdDeleteProc *) NULL,
		 module,
		 tclSpanmaskNew_hlp, tclSpanmaskNew_use);
    shTclDeclare(interp, "spanmaskDel",
		 (Tcl_CmdProc *)tclSpanmaskDel,
		 (ClientData) 0,
		 (Tcl_CmdDeleteProc *) NULL,
		 module,
		 tclSpanmaskDel_hlp, tclSpanmaskDel_use);
    shTclDeclare(interp, "spanmaskSetAsPix",
		 (Tcl_CmdProc *)tclSpanmaskSetAsPix,
		 (ClientData) 0,
		 (Tcl_CmdDeleteProc *) NULL,
		 module,
		 tclSpanmaskSetAsPix_hlp, tclSpanmaskSetAsPix_use);
   shTclDeclare(interp,"maskSetFromObjmaskChain",
		(Tcl_CmdProc *)tclMaskSetFromObjmaskChain, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclMaskSetFromObjmaskChain_hlp,
		tclMaskSetFromObjmaskChain_use);

   shTclDeclare(interp,"mergeObjmaskChain",
		(Tcl_CmdProc *)tclMergeObjmaskChain, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclMergeObjmaskChain_hlp,
		tclMergeObjmaskChain_use);

   shTclDeclare(interp,"objmaskSetFromReg",
		(Tcl_CmdProc *)tclObjmaskSetFromReg, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjmaskSetFromReg_hlp,
		tclObjmaskSetFromReg_use);

   shTclDeclare(interp,"regSetFromObjmask",
		(Tcl_CmdProc *)tclRegSetFromObjmask, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegSetFromObjmask_hlp,
		tclRegSetFromObjmask_use);

   shTclDeclare(interp,"regSetValFromObjmask",
		(Tcl_CmdProc *)tclRegSetValFromObjmask, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegSetValFromObjmask_hlp,
		tclRegSetValFromObjmask_use);

   shTclDeclare(interp,"objmaskGrowToSuperpixel",
		(Tcl_CmdProc *)tclObjmaskGrowToSuperpixel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjmaskGrowToSuperpixel_hlp,
		tclObjmaskGrowToSuperpixel_use);

   shTclDeclare(interp,"maskSetFromObjmask",
		(Tcl_CmdProc *)tclMaskSetFromObjmask, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclMaskSetFromObjmask_hlp,
		tclMaskSetFromObjmask_use);

   shTclDeclare(interp,objmaskFlatten_name,
		(Tcl_CmdProc *)tclObjmaskFlatten, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjmaskFlatten_hlp,
		tclObjmaskFlatten_use);

   shTclDeclare(interp,objmaskInflate_name,
		(Tcl_CmdProc *)tclObjmaskInflate, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjmaskInflate_hlp,
		tclObjmaskInflate_use);

   shTclDeclare(interp,objmaskChainGrow_name,
		(Tcl_CmdProc *)tclObjmaskChainGrow, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjmaskChainGrow_hlp,
		tclObjmaskChainGrow_use);

   shTclDeclare(interp,objmaskFromRect_name,
		(Tcl_CmdProc *)tclObjmaskFromRect, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjmaskFromRect_hlp,
		tclObjmaskFromRect_use);

   shTclDeclare(interp,objmaskFromCircle_name,
		(Tcl_CmdProc *)tclObjmaskFromCircle, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjmaskFromCircle_hlp,
		tclObjmaskFromCircle_use);

   shTclDeclare(interp,regSetFromObjmaskChain_name,
		(Tcl_CmdProc *)tclRegSetFromObjmaskChain, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegSetFromObjmaskChain_hlp,
		tclRegSetFromObjmaskChain_use);

   shTclDeclare(interp,objmaskChainAndObjmaskChain_name,
		(Tcl_CmdProc *)tclObjmaskChainAndObjmaskChain, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjmaskChainAndObjmaskChain_hlp,
		tclObjmaskChainAndObjmaskChain_use);

   shTclDeclare(interp,objmaskMerge_name,
		(Tcl_CmdProc *)tclObjmaskMerge, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjmaskMerge_hlp,
		tclObjmaskMerge_use);

   shTclDeclare(interp,pixIntersectObjmask_name,
		(Tcl_CmdProc *)tclPixIntersectObjmask, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPixIntersectObjmask_hlp,
		tclPixIntersectObjmask_use);

   shTclDeclare(interp,objmaskCopy_name,
		(Tcl_CmdProc *)tclObjmaskCopy, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjmaskCopy_hlp,
		tclObjmaskCopy_use);

   shTclDeclare(interp,objmaskAndNotObjmask_name,
		(Tcl_CmdProc *)tclObjmaskAndNotObjmask, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjmaskAndNotObjmask_hlp,
		tclObjmaskAndNotObjmask_use);

   shTclDeclare(interp,objmaskAndObjmask_name,
		(Tcl_CmdProc *)tclObjmaskAndObjmask, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjmaskAndObjmask_hlp,
		tclObjmaskAndObjmask_use);
}
