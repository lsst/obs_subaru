/*
 * TCL support for OBJ1 type in photo.
 *
 * ENTRY POINT		SCOPE
 * tclObjcDeclare	public	Declare all the verbs defined in this module
 *
 * REQUIRED PRODUCTS:
 *	FTCL		TCL + XTCL + Fermilab extensions
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "phObjc.h"
#include "phUtils.h"
#include "phTestInfo.h"
#include "phDataIo.h"

static char *module = "phTclStructsFacil";    /* name of this set of code */

/*****************************************************************************/
/*
 * return a handle to a new OBJC
 */
static char *tclObjcNew_use =
  "USAGE: objcNew ncolor";
static char *tclObjcNew_hlp =
  "create a new OBJC";

static int
tclObjcNew(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE handle;
   char name[HANDLE_NAMELEN];
   int ncolor;
   char *opts = "ncolor";
   
   shErrStackClear();
   
   ftclParseSave("objcNew");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      ncolor = ftclGetInt("ncolor");
   } else {
      Tcl_SetResult(interp,tclObjcNew_use,TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * ok, get a handle for our new OBJC
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   handle.ptr = phObjcNew(ncolor);
   handle.type = shTypeGetFromName("OBJC");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new objc handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete a OBJC
 */
static char *tclObjcDel_use =
  "USAGE: objcDel <objc> [-shallow]";
#define tclObjcDel_hlp \
  "Delete an OBJC, by default including siblings/children"

static ftclArgvInfo objcDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjcDel_hlp},
   {"<objc>", FTCL_ARGV_STRING, NULL, NULL, "The OBJC to delete"},
   {"-shallow", FTCL_ARGV_CONSTANT, (void *)1, NULL,
					  "Do a shallow deletion (default: 0)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjcDel(
	   ClientData clientDatag,
	   Tcl_Interp *interp,
	   int ac,
	   char **av
	   )
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *objcStr = NULL;		/* The OBJC to delete */
   int shallow = 0;			/* Do a shallow deletion (default: 0) */

   shErrStackClear();

   i = 1;
   objcDel_opts[i++].dst = &objcStr;
   objcDel_opts[i++].dst = &shallow;
   shAssert(objcDel_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objcDel_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,objcStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJC")) {
      Tcl_SetResult(interp,"objcDel: "
                    "first argument is not a OBJC",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   p_shTclHandleDel(interp,objcStr);
   phObjcDel(hand.ptr, !shallow);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Create or destroy an OBJC_IO
 */
static char *tclObjcIoNew_use =
  "USAGE: objcIoNew objc -ncolor ncolor";
#define tclObjcIoNew_hlp \
  "Create an OBJC_IO for ncolor OBJCs (default: 0)"

static ftclArgvInfo objcIoNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjcIoNew_hlp},
   {"-ncolor", FTCL_ARGV_INT, NULL, NULL,
			   "Number of colours desired in OBJC_IO (default: 0)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjcIoNew(
	      ClientData clientDatag,
	      Tcl_Interp *interp,
	      int ac,
	      char **av
	      )
{
   int i;
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   OBJC_IO *objcIo;
   int ncolor = 0;			/* No. of colours desired in OBJC_IO */

   shErrStackClear();

   i = 1;
   objcIoNew_opts[i++].dst = &ncolor;
   shAssert(objcIoNew_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objcIoNew_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * work
 */
   objcIo = phObjcIoNew(ncolor);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = objcIo;
   hand.type = shTypeGetFromName("OBJC_IO");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new OBJC_IO handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Create an OBJC_IO from an OBJC
 */
static char *tclObjcIoNewFromObjc_use =
  "USAGE: objcIoNewFromObjc objc";
#define tclObjcIoNewFromObjc_hlp \
  "Create an OBJC_IO from an OBJC. Note that the REGIONs and OBJMASKs in the "\
"OBJC are not copied, only the pointers to them. If objc is NULL, then "\
"return an OBJC_IO created from some child of the last OBJC set, or a handle "\
"bound to NULL. This may be used to write an OBJC and all its descendents to disk."

static ftclArgvInfo objcIoNewFromObjc_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjcIoNewFromObjc_hlp},
   {"<objc>", FTCL_ARGV_STRING, NULL, NULL,"The OBJC to convert to an OBJC_IO"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjcIoNewFromObjc(
	      ClientData clientDatag,
	      Tcl_Interp *interp,
	      int ac,
	      char **av
	      )
{
   int i;
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   OBJC *objc;
   OBJC_IO *objcIo;
   void *vptr;				/* used by shTclHandleExprEval */
   char *objcStr = NULL;		/* The OBJC to convert to an OBJC_IO */

   shErrStackClear();

   i = 1;
   objcIoNewFromObjc_opts[i++].dst = &objcStr;
   shAssert(objcIoNewFromObjc_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objcIoNewFromObjc_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(strcmp(objcStr,"null") == 0 || strcmp(objcStr,"NULL") == 0) {
      objc = NULL;
   } else {
      if(shTclHandleExprEval(interp,objcStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("OBJC")) {
	 Tcl_SetResult(interp,"objcIoNewFromObjc: "
		       "argument is not an OBJC or NULL",TCL_STATIC);
	 return(TCL_ERROR);
      }
      objc = hand.ptr;
   }
/*
 * work
 */
   objcIo = phObjcIoNewFromObjc(objc);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = objcIo;
   hand.type = shTypeGetFromName("OBJC_IO");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind to new OBJC_IO handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}


static char *tclObjcIoDel_use =
  "USAGE: objcIoDel <objcIo>";
#define tclObjcIoDel_hlp \
  "Delete an OBJC_IO; if -deep is true, also destroy the included OBJMASKs "\
"and REGIONs (note: this is probably not what you want)"

static ftclArgvInfo objcIoDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjcIoDel_hlp},
   {"<objcIo>", FTCL_ARGV_STRING, NULL, NULL, "The OBJC_IO to destroy"},
   {"-deep", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Do a deep deletion"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjcIoDel(
	      ClientData clientDatag,
	      Tcl_Interp *interp,
	      int ac,
	      char **av
	      )
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *objcIoStr = NULL;		/* The OBJC_IO to destroy */
   int deep = 0;			/* Do a deep deletion */

   shErrStackClear();

   i = 1;
   objcIoDel_opts[i++].dst = &objcIoStr;
   objcIoDel_opts[i++].dst = &deep;
   shAssert(objcIoDel_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objcIoDel_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * Process args
 */
   if(shTclHandleExprEval(interp,objcIoStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJC_IO")) {
      Tcl_SetResult(interp,"objcIoDel: "
                    "argument is not a OBJC_IO",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * Do the work
 */
   (void)p_shTclHandleDel(interp,objcIoStr);
   phObjcIoDel(hand.ptr,deep);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Destroy a CHAIN of OBJCs
 */
static char *tclObjcChainDel_use =
  "USAGE: objcChainDel chain [-shallow]";
#define tclObjcChainDel_hlp \
  "Destroy all OBJCs on a chain, and then the chain itself. \
If -shallow, don't delete the OBJC's relatives (siblings and children)"

static ftclArgvInfo objcChainDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjcChainDel_hlp},
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL, "the chain to destroy"},
   {"-shallow", FTCL_ARGV_CONSTANT, (void *)1, NULL,
					  "Do a shallow deletion (default: 0)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjcChainDel(
		ClientData clientDatag,
		Tcl_Interp *interp,
		int ac,
		char **av
		)
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *chainStr = NULL;		/* the chain to destroy */
   int shallow = 0;			/* Do a shallow deletion (default: 0) */

   shErrStackClear();

   i = 1;
   objcChainDel_opts[i++].dst = &chainStr;
   objcChainDel_opts[i++].dst = &shallow;
   shAssert(objcChainDel_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objcChainDel_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,chainStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_SetResult(interp,"objcChainDel: "
                    "first argument is not a CHAIN",TCL_STATIC);
      return(TCL_ERROR);
   }
   if(shChainTypeGet(hand.ptr) != shTypeGetFromName("OBJC")) {
      Tcl_SetResult(interp,"objcChainDel: "
                    "CHAIN is not of type OBJC",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * and do the work
 */
   phObjcChainDel(hand.ptr, !shallow);

   if(p_shTclHandleDel(interp, chainStr) != TCL_OK)  {
      Tcl_AppendResult(interp, "objcChainDel: cannot delete handle ", 
		       chainStr, (char *)NULL);
      return(TCL_ERROR);
   }
   
   return(TCL_OK);
}


/*****************************************************************************/
/*
 * Print out information on several fields in each color OBJ1
 * structure of each OBJC in the given list.  If the "-output=stem"
 * argument is present, put the output into a set of files: for
 * each color N (=0, 1, 2, ..), put info on that color into file
 *         stem.N
 * if the "-output=" argument is not given, send all to stdout.
 */

static char *tclObjcListPrint_use =
  "USAGE: objcListPrint objclist -output=stem";
static char *tclObjcListPrint_hlp =
  "Print info on each OBJC in given list; first all color[0], then all color[1], etc.  Place output in files of name 'stem.0', 'stem.1', etc., or to stdout.";

static int
tclObjcListPrint(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   char *objcstr;
   void *pUnused;
   char fname[50];
   char *opts = "objclist -output=stem";
   HANDLE hChain;
   CHAIN *objclist;

   shErrStackClear();

   ftclParseSave("objcListPrint");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      objcstr = ftclGetStr("objclist");

      if (shTclHandleExprEval(interp, objcstr, &hChain, &pUnused) != TCL_OK)
          return TCL_ERROR;

      if (hChain.type != shTypeGetFromName("CHAIN"))  {
          Tcl_AppendResult(interp, "handle ", objcstr, " is a ",
             shNameGetFromType(hChain.type), " not a CHAIN", (char *) NULL);
          return TCL_ERROR;
      }

      objclist = (CHAIN *) hChain.ptr;

      if (shChainTypeGet(objclist) != shTypeGetFromName("OBJC")) {
         Tcl_SetResult(interp, "given argument is not a list of OBJC",
                 TCL_STATIC);
         return(TCL_ERROR);
      }

      if (ftclPresent("output")) {
         sprintf(fname, "%s", ftclGetStr("output"));
      }
      else {
         sprintf(fname, "%s", "");
      }

   } else {
      Tcl_SetResult(interp, tclObjcListPrint_use, TCL_STATIC);
      return(TCL_ERROR);
   }

   phObjcListPrint(objclist, fname);

   Tcl_SetResult(interp, "", TCL_STATIC);
   return(TCL_OK);
}




/*****************************************************************************
 * Print out information on several fields in each color OBJECT1
 * structure of the given OBJC.  If the "-output=stem"
 * argument is present, put the output into a set of files: for
 * each color N (=0, 1, 2, ..), put info on that color into file
 *         stem.N
 * if the "-output=" argument is not given, send all to stdout.
 */

static char *tclObjcPrintPretty_use =
  "USAGE: objcPrintPretty objc -output=stem";
static char *tclObjcPrintPretty_hlp =
  "Print info the given OBJC; first color[0], then color[1], etc.  Place output in files of name 'stem.0', 'stem.1', etc., or to stdout.";

static int
tclObjcPrintPretty(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   char *objcstr;
   char fname[50];
   char *opts = "objc -output=stem";
   OBJC *objc;
   HANDLE *objcHandle;

   shErrStackClear();

   ftclParseSave("objcPrintPretty");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      objcstr = ftclGetStr("objc");
      if (p_shTclHandleAddrGet(interp, objcstr , &objcHandle) != TCL_OK) {
         Tcl_SetResult(interp, "can't get handle for given OBJC", TCL_STATIC);
         return(TCL_ERROR);
      }
      objc = objcHandle->ptr;
      if (objcHandle->type != shTypeGetFromName("OBJC")) {
         Tcl_SetResult(interp, "given argument is not an OBJC", TCL_STATIC);
         return(TCL_ERROR);
      }
      if (ftclPresent("output")) {
         sprintf(fname, "%s", ftclGetStr("output"));
      }
      else {
         sprintf(fname, "%s", "");
      }
   } else {
      ftclParseRestore("objcPrintPretty");
      Tcl_SetResult(interp, tclObjcPrintPretty_use, TCL_STATIC);
      return(TCL_ERROR);
   }

   phObjcPrintPretty(objc, fname);

   Tcl_SetResult(interp, "", TCL_STATIC);
   return(TCL_OK);
}



/*****************************************************************************
 * Compare two OBJCs, setting the Tcl_Result to the string "0" if they are 
 * "equal", or some non-"0" string if they are not.  Define "equal" to 
 * mean that a comparison of certain fields of their OBJECT1 structures
 * shows that they are identical (to within some tolerance).
 * 
 * note that we return TCL_OK if the user supplies the correct arguments,
 * but the two OBJCs just aren't equal.
 */

static char *tclObjcCompare_use =
  "USAGE: objcCompare objc1 objc2";
static char *tclObjcCompare_hlp =
  "Compare two OBJCs by looking at certain fields in their correspoding OBJECT1s;  return 0 if they are the same, or 1 if not.";

static int
tclObjcCompare(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   char *objcstr;
   char *opts = "objc1 objc2";
   OBJC *objc1, *objc2;
   HANDLE *handle1, *handle2;

   shErrStackClear();

   ftclParseSave("objcCompare");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      objcstr = ftclGetStr("objc1");
      if (p_shTclHandleAddrGet(interp, objcstr, &handle1) != TCL_OK) {
         Tcl_SetResult(interp, "can't get handle for first OBJC", TCL_STATIC);
         return(TCL_ERROR);
      }
      objc1 = handle1->ptr;
      if (handle1->type != shTypeGetFromName("OBJC")) {
         Tcl_SetResult(interp, "first argument is not an OBJC", TCL_STATIC);
         return(TCL_ERROR);
      }

      objcstr = ftclGetStr("objc2");
      if (p_shTclHandleAddrGet(interp, objcstr, &handle2) != TCL_OK) {
         Tcl_SetResult(interp, "can't get handle for second OBJC", TCL_STATIC);
         return(TCL_ERROR);
      }
      objc2 = handle2->ptr;
      if (handle2->type != shTypeGetFromName("OBJC")) {
         Tcl_SetResult(interp, "second argument is not an OBJC", TCL_STATIC);
         return(TCL_ERROR);
      }
   }
   else {
      ftclParseRestore("objcCompare");
      Tcl_SetResult(interp, tclObjcCompare_use, TCL_STATIC);
      return(TCL_ERROR);
   }

   if (phObjcCompare(objc1, objc2) == 0) {
      Tcl_SetResult(interp, "0", TCL_STATIC);
      return(TCL_OK);
   }
   else {
      Tcl_SetResult(interp, "1", TCL_STATIC);
      return(TCL_OK);
   }
}

/*****************************************************************************
 * Given an LIST of OBJC, an 'x' coord, and a 'y' coord, and color to look in 
 * find the OBJC which is
 * closest to the given coords and return a handle to the OBJC.
 */

static char *tclObjcClosest_use =
  "USAGE: objcClosest objc x y color";
static char *tclObjcClosest_hlp =
  "Find the closest OBJC in the given LIST of OBJCs to the (x, y) position, and return a handle to it.";

static int
tclObjcClosest(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   char *objcstr;
   char name[HANDLE_NAMELEN];
   char *opts = "objclist x y color";
   void *pUnused;
   int color;
   float xc, yc;
   CHAIN *objclist;
   OBJC *objc;
   HANDLE newHandle,
          hChain;

   shErrStackClear();

   ftclParseSave("objcClosest");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      objcstr = ftclGetStr("objclist");

      if (shTclHandleExprEval(interp, objcstr, &hChain, &pUnused) != TCL_OK)
          return TCL_ERROR;

      if (hChain.type != shTypeGetFromName("CHAIN"))  {
          Tcl_AppendResult(interp, "handle ", objcstr, " is a ",
             shNameGetFromType(hChain.type), ", not a CHAIN", (char *) NULL);
          return TCL_ERROR;
      }

      objclist = (CHAIN *) hChain.ptr;
   
      if (shChainTypeGet(objclist) != shTypeGetFromName("OBJC")) {
         Tcl_SetResult(interp, "given argument is not a list of OBJC",
                 TCL_STATIC);
         return(TCL_ERROR);
      }
      xc = ftclGetDouble("x");
      yc = ftclGetDouble("y");
      color = ftclGetInt("color");
   }
   else {
      ftclParseRestore("objcClosest");
      Tcl_SetResult(interp, tclObjcClosest_use, TCL_STATIC);
      return(TCL_ERROR);
   }

   if ((objc = phObjcClosest(objclist, xc, yc,color)) == NULL) { 
      Tcl_SetResult(interp, "", TCL_STATIC);
      return(TCL_OK);
   }
   else {
      if (p_shTclHandleNew(interp, name) != TCL_OK) {
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
      newHandle.ptr = objc;
      newHandle.type = shTypeGetFromName("OBJC");
      if (p_shTclHandleAddrBind(interp, newHandle, name) != TCL_OK) {
	 Tcl_SetResult(interp, "can't bind to new OBJC handle", TCL_STATIC);
	 return(TCL_ERROR);
      }
   }
   Tcl_SetResult(interp, name, TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclObjcDescendentNext_use =
  "USAGE: ObjcDescendentNext objc";
#define tclObjcDescendentNext_hlp \
  "Return all an OBJCs children in order"

static ftclArgvInfo objcDescendentNext_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjcDescendentNext_hlp},
   {"<objc>", FTCL_ARGV_STRING, NULL, NULL, "The parent OBJC, or NULL"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclObjcDescendentNext(
		      ClientData clientDatag,
		      Tcl_Interp *interp,
		      int ac,
		      char **av
		      )
{
   int i;
   HANDLE hand;
   char name[HANDLE_NAMELEN];
   OBJC *objc;
   void *vptr;				/* used by shTclHandleExprEval */
   char *objcStr = NULL;		/* The parent OBJC, or NULL */

   shErrStackClear();

   i = 1;
   objcDescendentNext_opts[i++].dst = &objcStr;
   shAssert(objcDescendentNext_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,objcDescendentNext_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(*objcStr == '\0' ||
      strcmp(objcStr,"null") == 0 || strcmp(objcStr,"NULL") == 0) {
      objc = NULL;
   } else {
      if(shTclHandleExprEval(interp,objcStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("OBJC")) {
	 Tcl_SetResult(interp,"objcDescendentNext: "
		       "argument is not a OBJC",TCL_STATIC);
	 return(TCL_ERROR);
      }
      objc = hand.ptr;
   }
/*
 * work
 */
   objc = phObjcDescendentNext(objc);
/*
 * Return the answer
 */
   if(objc == NULL) {
      *name = '\0';			/* return empty string */
   } else {
      if(p_shTclHandleNew(interp,name) != TCL_OK) {
	 shTclInterpAppendWithErrStack(interp);
	 return(TCL_ERROR);
      }
      hand.ptr = objc;
      hand.type = shTypeGetFromName("OBJC");
      
      if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
	 Tcl_SetResult(interp,"can't bind to new OBJC handle",TCL_STATIC);
	 return(TCL_ERROR);
      }
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * atlas image con/destructors
 */
static char *tclAtlasImageNew_use =
  "USAGE: atlasImageNew -ncolor";
#define tclAtlasImageNew_hlp \
  "Create a new atlas image"

static ftclArgvInfo atlasImageNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclAtlasImageNew_hlp},
   {"-ncolor", FTCL_ARGV_INT, NULL, NULL, "Number of colours"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define atlasImageNew_name "atlasImageNew"

static int
tclAtlasImageNew(ClientData clientData,
		 Tcl_Interp *interp,
		 int ac,
		 char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   int ncolor = 0;			/* Number of colours */
   ATLAS_IMAGE *ai;			/* the new atlas image */

   shErrStackClear();

   i = 1;
   atlasImageNew_opts[i++].dst = &ncolor;
   shAssert(atlasImageNew_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, atlasImageNew_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     atlasImageNew_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(ncolor < 0) {
      ncolor = 0;
   }
/*
 * work
 */
   ai = phAtlasImageNew(ncolor);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = ai;
   hand.type = shTypeGetFromName("ATLAS_IMAGE");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"name\" to new ATLAS_IMAGE handle",
								   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclAtlasImageDel_use =
  "USAGE: atlasImageDel <ai> -deep";
#define tclAtlasImageDel_hlp \
  "destroy an atlas image"

static ftclArgvInfo atlasImageDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclAtlasImageDel_hlp},
   {"<ai>", FTCL_ARGV_STRING, NULL, NULL, "The atlas image to delete"},
   {"-deep", FTCL_ARGV_CONSTANT, (void *)1, NULL,
					   "Do a deep deletion? {default: 0)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define atlasImageDel_name "atlasImageDel"

static int
tclAtlasImageDel(ClientData clientData,
		 Tcl_Interp *interp,
		 int ac,
		 char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *aiStr = NULL;			/* The atlas image to delete */
   ATLAS_IMAGE *ai;
   int deep = 0;			/* Do a deep deletion? */

   shErrStackClear();

   i = 1;
   atlasImageDel_opts[i++].dst = &aiStr;
   atlasImageDel_opts[i++].dst = &deep;
   shAssert(atlasImageDel_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, atlasImageDel_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     atlasImageDel_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,aiStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("ATLAS_IMAGE")) {
      Tcl_SetResult(interp,"atlasImageDel: "
                    "argument \"ai\" is not a ATLAS_IMAGE",TCL_STATIC);
      return(TCL_ERROR);
   }
   ai = hand.ptr;
/*
 * work
 */
   p_shTclHandleDel(interp,aiStr);
   phAtlasImageDel(ai, deep);
   
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Test atlas image flattening/inflating to/from strings
 */
static char *tclAtlasImageFlatten_use =
  "USAGE: atlasImageFlatten <ai>";
#define tclAtlasImageFlatten_hlp \
  "Flatten an atlas image into a network-byte-order stream of bytes"

static ftclArgvInfo atlasImageFlatten_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclAtlasImageFlatten_hlp},
   {"<ai>", FTCL_ARGV_STRING, NULL, NULL, "The atlas image to squash"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define atlasImageFlatten_name "atlasImageFlatten"

static int
tclAtlasImageFlatten(ClientData clientData,
		     Tcl_Interp *interp,
		     int ac,
		     char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *aiStr = NULL;			/* The atlas image to squash */
   ATLAS_IMAGE *ai;
   unsigned char *buff;			/* buffer to flatten ai into */
   int len;				/* size of buff */

   shErrStackClear();

   i = 1;
   atlasImageFlatten_opts[i++].dst = &aiStr;
   shAssert(atlasImageFlatten_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, atlasImageFlatten_opts,
		     0, atlasImageFlatten_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,aiStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("ATLAS_IMAGE")) {
      Tcl_SetResult(interp,"atlasImageFlatten: "
                    "argument \"ai\" is not a ATLAS_IMAGE",TCL_STATIC);
      return(TCL_ERROR);
   }
   ai = hand.ptr;   
/*
 * work
 */
   len = phAtlasImageFlatten(ai, NULL, 0);
   buff = shMalloc(len);
   len = phAtlasImageFlatten(ai, buff, len);
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

/*****************************************************************************/

static char *tclAtlasImageInflate_use =
  "USAGE: atlasImageInflate <buff>";
#define tclAtlasImageInflate_hlp \
  "Inflate a network -byte-order string into an atlas image"

static ftclArgvInfo atlasImageInflate_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclAtlasImageInflate_hlp},
   {"<buff>", FTCL_ARGV_STRING, NULL, NULL, "Buffer to inflate"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define atlasImageInflate_name "atlasImageInflate"

static int
tclAtlasImageInflate(ClientData clientData,
		     Tcl_Interp *interp,
		     int ac,
		     char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *buffStr = NULL;		/* Buffer to inflate */
   unsigned char *buff;
   ATLAS_IMAGE *ai;

   shErrStackClear();

   i = 1;
   atlasImageInflate_opts[i++].dst = &buffStr;
   shAssert(atlasImageInflate_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, atlasImageInflate_opts,
		     0, atlasImageInflate_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_SetResult(interp,"atlasImageInflate: "
                    "argument \"buff\" is not a PTR",TCL_STATIC);
      return(TCL_ERROR);
   }
   buff = hand.ptr;
/*
 * work
 */
   ai = phAtlasImageInflate(NULL, buff, NULL);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = ai;
   hand.type = shTypeGetFromName("ATLAS_IMAGE");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"name\" to new ATLAS_IMAGE handle",
		    TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclAtlasImageDelFromObjc_use =
  "USAGE: atlasImageDelFromObjc <objc> -deep";
#define tclAtlasImageDelFromObjc_hlp \
  "Delete all atlas images associated with an OBJC"

static ftclArgvInfo atlasImageDelFromObjc_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclAtlasImageDelFromObjc_hlp},
   {"<objc>", FTCL_ARGV_STRING, NULL, NULL,
				  "Objc whose atlas images are to be deleted"},
   {"-deep", FTCL_ARGV_CONSTANT, (void *)1, NULL,
			    "Delete siblings and children's atlas images too"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define atlasImageDelFromObjc_name "atlasImageDelFromObjc"

static int
tclAtlasImageDelFromObjc(ClientData clientData,
			 Tcl_Interp *interp,
			 int ac,
			 char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *objcStr = NULL;		/* objc whose AIs are to be deleted */
   OBJC *objc;
   int deep = 0;			/* Del siblings' and children's AIs */

   shErrStackClear();

   i = 1;
   atlasImageDelFromObjc_opts[i++].dst = &objcStr;
   atlasImageDelFromObjc_opts[i++].dst = &deep;
   shAssert(atlasImageDelFromObjc_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, atlasImageDelFromObjc_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     atlasImageDelFromObjc_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,objcStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJC")) {
      Tcl_SetResult(interp,"atlasImageDelFromObjc: "
                    "argument \"objc\" is not a OBJC",TCL_STATIC);
      return(TCL_ERROR);
   }
   objc = hand.ptr;
/*
 * work
 */
   phAtlasImageDelFromObjc(objc, deep);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclRegDelFromObjc_use =
  "USAGE: regDelFromObjc <objc> -deep";
#define tclRegDelFromObjc_hlp \
  "Delete all the REGIONs associated with an OBJC, "\
"and maybe its children and siblings"

static ftclArgvInfo regDelFromObjc_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegDelFromObjc_hlp},
   {"<objc>", FTCL_ARGV_STRING, NULL, NULL,"Objc to have its regions deleted"},
   {"-deep", FTCL_ARGV_CONSTANT, (void *)1, NULL,
				      "Delete the OBJC's family's region too"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define regDelFromObjc_name "regDelFromObjc"

static int
tclRegDelFromObjc(ClientData clientData,
		  Tcl_Interp *interp,
		  int ac,
		  char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *objcStr = NULL;		/* Objc to have its regions deleted */
   OBJC *objc;
   int deep = 0;			/* Delete family's regions too */

   shErrStackClear();

   i = 1;
   regDelFromObjc_opts[i++].dst = &objcStr;
   regDelFromObjc_opts[i++].dst = &deep;
   shAssert(regDelFromObjc_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, regDelFromObjc_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     regDelFromObjc_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,objcStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJC")) {
      Tcl_SetResult(interp,"regDelFromObjc: "
                    "argument \"objc\" is not a OBJC",TCL_STATIC);
      return(TCL_ERROR);
   }
   objc = hand.ptr;
/*
 * work
 */
   phRegDelFromObjc(objc, deep);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclAtlasImagesPlusEquals_use =
  "USAGE: atlasImagesPlusEquals <ai> <cai> [c] -bias";
#define tclAtlasImagesPlusEquals_hlp \
  "Add the pixels in one atlas image to another,\n"\
"\t\"ai->pix[] += cai->pix[] + bias;\""

static ftclArgvInfo atlasImagesPlusEquals_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclAtlasImagesPlusEquals_hlp},
   {"<ai>", FTCL_ARGV_STRING, NULL, NULL, "Atlas image to be added to"},
   {"<cai>", FTCL_ARGV_STRING, NULL, NULL, "Atlas image to add to cai"},
   {"[c]", FTCL_ARGV_INT, NULL, NULL, "Which colour to add (default: all)"},
   {"-bias", FTCL_ARGV_INT, NULL, NULL, "value to add to each pixel in ai"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define atlasImagesPlusEquals_name "atlasImagesPlusEquals"

static int
tclAtlasImagesPlusEquals(ClientData clientData,
			 Tcl_Interp *interp,
			 int ac,
			 char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *aiStr = NULL;			/* Atlas image to be added to */
   ATLAS_IMAGE *ai;
   char *caiStr = NULL;			/* Atlas image to add to cai */
   ATLAS_IMAGE *cai;
   int c = -1;				/* Which colour to add (default: all)*/
   int bias = 0;			/* Bias to add to each pixel in cai */

   shErrStackClear();

   i = 1;
   atlasImagesPlusEquals_opts[i++].dst = &aiStr;
   atlasImagesPlusEquals_opts[i++].dst = &caiStr;
   atlasImagesPlusEquals_opts[i++].dst = &c;
   atlasImagesPlusEquals_opts[i++].dst = &bias;
   shAssert(atlasImagesPlusEquals_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, atlasImagesPlusEquals_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     atlasImagesPlusEquals_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,aiStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("ATLAS_IMAGE")) {
      Tcl_AppendResult(interp,"atlasImagesPlusEquals: "
                       "argument \"", aiStr, "\" is not an ATLAS_IMAGE",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   ai = hand.ptr;

   if(shTclHandleExprEval(interp,caiStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("ATLAS_IMAGE")) {
      Tcl_AppendResult(interp,"atlasImagesPlusEquals: "
                       "argument \"", caiStr, "\" is not an ATLAS_IMAGE",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   cai = hand.ptr;

   if(ai->ncolor != cai->ncolor || ai->npix != cai->npix) {
      Tcl_AppendResult(interp, "ATLAS_IMAGEs ", aiStr, " and ", caiStr,
		       " have different sizes/numbers of colours",
		       (char *)NULL);
      return(TCL_ERROR);
   }
   if(c < -1 || c >= ai->ncolor) {
      Tcl_AppendResult(interp, "Specified colour index is out of range",
		       (char *)NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phAtlasImagesPlusEquals(ai, c, cai, c, bias);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclAtlasImagesMinusEquals_use =
  "USAGE: atlasImagesMinusEquals <ai> <cai> [c] -bias";
#define tclAtlasImagesMinusEquals_hlp \
"Subtract the pixels in one atlas image from another,\n"\
"\t\"ai->pix[] -= cai->pix[] + bias;\""

static ftclArgvInfo atlasImagesMinusEquals_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclAtlasImagesMinusEquals_hlp},
   {"<ai>", FTCL_ARGV_STRING, NULL, NULL, "Atlas image to be subtracted from"},
   {"<cai>", FTCL_ARGV_STRING, NULL, NULL, "Atlas image to subtract from cai"},
   {"[c]", FTCL_ARGV_INT, NULL, NULL,
				    "Which colour to subtract (default: all)"},
   {"-bias", FTCL_ARGV_INT, NULL, NULL,
				    "Value to subtract from each pixel in ai"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define atlasImagesMinusEquals_name "atlasImagesMinusEquals"

static int
tclAtlasImagesMinusEquals(ClientData clientData,
			 Tcl_Interp *interp,
			 int ac,
			 char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *aiStr = NULL;			/* Atlas image to be subtracted from */
   ATLAS_IMAGE *ai;
   char *caiStr = NULL;			/* Atlas image to subtract from cai */
   ATLAS_IMAGE *cai;
   int c = -1;				/* Which colour to subtract
					   (default: all)*/
   int bias = 0;			/* value to subtract from each pixel
					   in ai */

   shErrStackClear();

   i = 1;
   atlasImagesMinusEquals_opts[i++].dst = &aiStr;
   atlasImagesMinusEquals_opts[i++].dst = &caiStr;
   atlasImagesMinusEquals_opts[i++].dst = &c;
   atlasImagesMinusEquals_opts[i++].dst = &bias;
   shAssert(atlasImagesMinusEquals_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, atlasImagesMinusEquals_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     atlasImagesMinusEquals_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,aiStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("ATLAS_IMAGE")) {
      Tcl_AppendResult(interp,"atlasImagesMinusEquals: "
                       "argument \"", aiStr, "\" is not an ATLAS_IMAGE",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   ai = hand.ptr;

   if(shTclHandleExprEval(interp,caiStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("ATLAS_IMAGE")) {
      Tcl_AppendResult(interp,"atlasImagesMinusEquals: "
                       "argument \"", caiStr, "\" is not an ATLAS_IMAGE",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   cai = hand.ptr;

   if(ai->ncolor != cai->ncolor || ai->npix != cai->npix) {
      Tcl_AppendResult(interp, "ATLAS_IMAGEs ", aiStr, " and ", caiStr,
		       " have different sizes/numbers of colours",
		       (char *)NULL);
      return(TCL_ERROR);
   }
   if(c < -1 || c >= ai->ncolor) {
      Tcl_AppendResult(interp, "Specified colour index is out of range",
		       (char *)NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   phAtlasImagesMinusEquals(ai, c, cai, c, bias);

   return(TCL_OK);
}

/*****************************************************************************/
static char *tclTestInfoNew_use =
  "USAGE: testInfoNew <ncolor>";
#define tclTestInfoNew_hlp \
  "Return a new TEST_INFO"

static ftclArgvInfo testInfoNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclTestInfoNew_hlp},
   {"<ncolor>", FTCL_ARGV_INT, NULL, NULL, "Number of colours present"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define testInfoNew_name "testInfoNew"

static int
tclTestInfoNew(ClientData clientData,
	       Tcl_Interp *interp,
	       int ac,
	       char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   int ncolor = 0;			/* Number of colours present */
   TEST_INFO *tst;			/* the desired TEST_INFO */

   shErrStackClear();

   i = 1;
   testInfoNew_opts[i++].dst = &ncolor;
   shAssert(testInfoNew_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, testInfoNew_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     testInfoNew_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(ncolor <= 0 || ncolor > NCOLOR) {
      char n[10]; sprintf(n, "%d", ncolor);
      Tcl_AppendResult(interp, "Illegal value of ncolor: ", n, (char *)NULL);
      return(TCL_ERROR);
   }
/*
 * work
 */
   tst = phTestInfoNew(ncolor);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = tst;
   hand.type = shTypeGetFromName("TEST_INFO");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"tst\" to new TEST_INFO handle",
		    TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclTestInfoDel_use =
  "USAGE: testInfoDel <testInfo> -deep";
#define tclTestInfoDel_hlp \
  "delete a TEST_INFO"

static ftclArgvInfo testInfoDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclTestInfoDel_hlp},
   {"<testInfo>", FTCL_ARGV_STRING, NULL, NULL, "TEST_INFO to destroy"},
   {"-deep", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Do a deep deletion?"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define testInfoDel_name "testInfoDel"

static int
tclTestInfoDel(ClientData clientData,
	       Tcl_Interp *interp,
	       int ac,
	       char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *testInfoStr = NULL;		/* TEST_INFO to destroy */
   TEST_INFO *testInfo;
   int deep = 0;			/* Do a deep deletion? */

   shErrStackClear();

   i = 1;
   testInfoDel_opts[i++].dst = &testInfoStr;
   testInfoDel_opts[i++].dst = &deep;
   shAssert(testInfoDel_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, testInfoDel_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     testInfoDel_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,testInfoStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("TEST_INFO")) {
      Tcl_AppendResult(interp,"testInfoDel: "
                       "argument \"", testInfoStr, "\" is not a TEST_INFO",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   testInfo = hand.ptr;
/*
 * work
 */
   phTestInfoDel(testInfo, deep);

   p_shTclHandleDel(interp,testInfoStr);

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclObjcNewFromObjc_use =
  "USAGE: objcNewFromObjc <objc> -deep -copy_ai";
#define tclObjcNewFromObjc_hlp \
  ""

static ftclArgvInfo objcNewFromObjc_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjcNewFromObjc_hlp},
   {"<objc>", FTCL_ARGV_STRING, NULL, NULL, "The OBJC to copy"},
   {"-deep", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Make a copy of OBJECT1s, ATLAS_IMAGEs, etc"},
   {"-copy_ai", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Copy atlas images?"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},   
};

#define objcNewFromObjc_name "objcNewFromObjc"

static int
tclObjcNewFromObjc(ClientData clientData,
		   Tcl_Interp *interp,
		   int ac,
		   char **av)
{
   char name[HANDLE_NAMELEN];
    HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *objcStr = NULL;		/* The OBJC to copy */
   const OBJC *objc;			/* input OBJC */
   OBJC *objc_copy = NULL;		/* output OBJC */
   int deep = 0;			/* Make a copy of OBJECT1s, ATLAS_IMAGEs, etc */
   int copy_ai = 0;			/* Copy atlas images? */

   shErrStackClear();

   a_i = 1;
   objcNewFromObjc_opts[a_i++].dst = &objcStr;
   objcNewFromObjc_opts[a_i++].dst = &deep;
   objcNewFromObjc_opts[a_i++].dst = &copy_ai;
   shAssert(objcNewFromObjc_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, objcNewFromObjc_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     objcNewFromObjc_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,objcStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("OBJC")) {
      Tcl_AppendResult(interp,"objcNewFromObjc: "
                       "argument \"", objcStr, "\" is not an OBJC",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   objc = hand.ptr;
/*
 * work
 */
   objc_copy = phObjcNewFromObjc(objc, deep, copy_ai);

   if (objc_copy == NULL) {
       Tcl_SetResult(interp, "objcNewFromObjc: copy is NULL", TCL_STATIC);
       Tcl_AppendResult(interp, (char *)NULL);
       return(TCL_ERROR);
   }
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = objc_copy;
   hand.type = shTypeGetFromName("OBJC");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"objc_copy\" to new OBJC handle",
                                                                   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);       

   return(TCL_OK);
}


/****************************************************************************
 * Declare my new tcl verbs to tcl
 */
void
phTclObjcDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"objcNew",
                (Tcl_CmdProc *)tclObjcNew,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclObjcNew_hlp, tclObjcNew_use);
   shTclDeclare(interp,"objcListPrint",
                (Tcl_CmdProc *)tclObjcListPrint,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclObjcListPrint_hlp, tclObjcListPrint_use);

   shTclDeclare(interp,"objcPrintPretty",
                (Tcl_CmdProc *)tclObjcPrintPretty,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclObjcPrintPretty_hlp, tclObjcPrintPretty_use);

   shTclDeclare(interp,"objcCompare",
                (Tcl_CmdProc *)tclObjcCompare,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclObjcCompare_hlp, tclObjcCompare_use);
   shTclDeclare(interp,"objcClosest",
                (Tcl_CmdProc *)tclObjcClosest,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclObjcClosest_hlp, tclObjcClosest_use);

   shTclDeclare(interp,"objcChainDel",
		(Tcl_CmdProc *)tclObjcChainDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjcChainDel_hlp,
		tclObjcChainDel_use);

   shTclDeclare(interp,"objcIoNew",
		(Tcl_CmdProc *)tclObjcIoNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjcIoNew_hlp,
		tclObjcIoNew_use);

   shTclDeclare(interp,"objcIoNewFromObjc",
		(Tcl_CmdProc *)tclObjcIoNewFromObjc, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjcIoNewFromObjc_hlp,
		tclObjcIoNewFromObjc_use);

   shTclDeclare(interp,"objcIoDel",
		(Tcl_CmdProc *)tclObjcIoDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjcIoDel_hlp,
		tclObjcIoDel_use);

   shTclDeclare(interp,"objcDel",
		(Tcl_CmdProc *)tclObjcDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjcDel_hlp,
		tclObjcDel_use);

   shTclDeclare(interp,"objcDescendentNext",
		(Tcl_CmdProc *)tclObjcDescendentNext, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjcDescendentNext_hlp,
		tclObjcDescendentNext_use);

   shTclDeclare(interp,atlasImageFlatten_name,
		(Tcl_CmdProc *)tclAtlasImageFlatten, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclAtlasImageFlatten_hlp,
		tclAtlasImageFlatten_use);

   shTclDeclare(interp,atlasImageInflate_name,
		(Tcl_CmdProc *)tclAtlasImageInflate, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclAtlasImageInflate_hlp,
		tclAtlasImageInflate_use);

   shTclDeclare(interp,atlasImageDelFromObjc_name,
		(Tcl_CmdProc *)tclAtlasImageDelFromObjc, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclAtlasImageDelFromObjc_hlp,
		tclAtlasImageDelFromObjc_use);

   shTclDeclare(interp,regDelFromObjc_name,
		(Tcl_CmdProc *)tclRegDelFromObjc, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegDelFromObjc_hlp,
		tclRegDelFromObjc_use);

   shTclDeclare(interp,atlasImageNew_name,
		(Tcl_CmdProc *)tclAtlasImageNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclAtlasImageNew_hlp,
		tclAtlasImageNew_use);

   shTclDeclare(interp,atlasImageDel_name,
		(Tcl_CmdProc *)tclAtlasImageDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclAtlasImageDel_hlp,
		tclAtlasImageDel_use);

   shTclDeclare(interp,atlasImagesPlusEquals_name,
		(Tcl_CmdProc *)tclAtlasImagesPlusEquals, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclAtlasImagesPlusEquals_hlp,
		tclAtlasImagesPlusEquals_use);

   shTclDeclare(interp,atlasImagesMinusEquals_name,
		(Tcl_CmdProc *)tclAtlasImagesMinusEquals, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclAtlasImagesMinusEquals_hlp,
		tclAtlasImagesMinusEquals_use);

   shTclDeclare(interp,testInfoNew_name,
		(Tcl_CmdProc *)tclTestInfoNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclTestInfoNew_hlp,
		tclTestInfoNew_use);

   shTclDeclare(interp,testInfoDel_name,
		(Tcl_CmdProc *)tclTestInfoDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclTestInfoDel_hlp,
		tclTestInfoDel_use);

   shTclDeclare(interp,objcNewFromObjc_name,
		(Tcl_CmdProc *)tclObjcNewFromObjc, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjcNewFromObjc_hlp,
		tclObjcNewFromObjc_use);
}

