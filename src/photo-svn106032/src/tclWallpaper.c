#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dervish.h"
#include "phConsts.h"
#include "phWallpaper.h"

/*****************************************************************************/
/*
 * Convert a U16 to a U8 region
 */
static char *tclRegIntToU8Linear_use =
  "USAGE: regIntToU8Linear <reg> <base> <top>";
#define tclRegIntToU8Linear_hlp \
  "Create a U8 region from a U16 one, using a linear mapping"

static ftclArgvInfo regIntToU8Linear_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegIntToU8Linear_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Input region"},
   {"<base>", FTCL_ARGV_INT, NULL, NULL, "Pixel value to map to 0x0"},
   {"<top>", FTCL_ARGV_INT, NULL, NULL, "Pixel value to map to 0xff"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define regIntToU8Linear_name "regIntToU8Linear"

static int
tclRegIntToU8Linear(ClientData clientData,
		    Tcl_Interp *interp,
		    int ac,
		    char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Input region */
   REGION *reg;
   int base = 0;			/* Pixel value to map to 0x0 */
   int top = 0;				/* Pixel value to map to 0xff */
   REGION *ureg;			/* U8 region to return */

   shErrStackClear();

   i = 1;
   regIntToU8Linear_opts[i++].dst = &regStr;
   regIntToU8Linear_opts[i++].dst = &base;
   regIntToU8Linear_opts[i++].dst = &top;
   shAssert(regIntToU8Linear_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, regIntToU8Linear_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     regIntToU8Linear_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_SetResult(interp,"regIntToU8Linear: "
                    "argument \"reg\" is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(reg->type != TYPE_PIX) {
      Tcl_SetResult(interp,"regIntToU8Linear: "
                    "Region \"reg\" is not U16",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   ureg = phRegIntToU8Linear(reg, base, top);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = ureg;
   hand.type = shTypeGetFromName("REGION");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"name\" to new REGION handle",
								   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * convert U16 to U8 via a pre-computed LUT
 */
static char *tclRegIntToU8LUT_use =
  "USAGE: regIntToU8LUT <reg> <lut>";
#define tclRegIntToU8LUT_hlp \
  "Create a U8 region from a U16 one, using an LUT built by u16ToU8LUTGet"

static ftclArgvInfo regIntToU8LUT_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegIntToU8LUT_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "input region"},
   {"<lut>", FTCL_ARGV_STRING, NULL, NULL, "U8 region containing the LUT"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define regIntToU8LUT_name "regIntToU8LUT"

static int
tclRegIntToU8LUT(ClientData clientData,
		 Tcl_Interp *interp,
		 int ac,
		 char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region to convert */
   REGION *reg;
   char *lutStr = NULL;			/* U8 region containing the LUT */
   REGION *lut;
   REGION *ureg;			/* U8 region to return */

   shErrStackClear();

   i = 1;
   regIntToU8LUT_opts[i++].dst = &regStr;
   regIntToU8LUT_opts[i++].dst = &lutStr;
   shAssert(regIntToU8LUT_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, regIntToU8LUT_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     regIntToU8LUT_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_SetResult(interp,"regIntToU8LUT: "
                    "argument \"reg\" is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(reg->type != TYPE_U16) {
      Tcl_SetResult(interp,"regIntToU8LUT: "
                    "Region \"reg\" is not U16",TCL_STATIC);
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,lutStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"regIntToU8LUT: "
                    "argument \"lut\" is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   lut = hand.ptr;

   if(lut->type != TYPE_U8) {
      Tcl_SetResult(interp,"regIntToU8Linear: "
                    "Region \"lut\" is not U8",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   ureg = phRegIntToU8LUT(reg, lut);
/*
 * Return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = ureg;
   hand.type = shTypeGetFromName("REGION");
   
   if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
      Tcl_SetResult(interp,"can't bind \"name\" to new REGION handle",
								   TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Create three U8 regions from three U16 ones, using a LUT on the total
 * intensity
 */
static char *tclRegIntRGBToU8LUT_use =
  "USAGE: regIntRGBToU8LUT <reg1> <reg2> <reg3> <lut> -separate";
#define tclRegIntRGBToU8LUT_hlp \
  "Return a list of three U8 regions corresponding to three U16 ones, using"\
"a lookup table for the total intensity"

static ftclArgvInfo regIntRGBToU8LUT_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegIntRGBToU8LUT_hlp},
   {"<reg1>", FTCL_ARGV_STRING, NULL, NULL, "U16 region with R data"},
   {"<reg2>", FTCL_ARGV_STRING, NULL, NULL, "U16 region with G data"},
   {"<reg3>", FTCL_ARGV_STRING, NULL, NULL, "U16 region with B data"},
   {"<lut>", FTCL_ARGV_STRING, NULL, NULL, "U8 region with LUT"},
   {"-separate", FTCL_ARGV_CONSTANT, (void *)1, NULL,
      "Scale each band separately? (Not recommended)"},   
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define regIntRGBToU8LUT_name "regIntRGBToU8LUT"

static int
tclRegIntRGBToU8LUT(ClientData clientData,
		    Tcl_Interp *interp,
		    int ac,
		    char **av)
{
   char name[HANDLE_NAMELEN];
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *reg1Str = NULL;		/* U16 region with R data */
   REGION *reg1;
   char *reg2Str = NULL;		/* U16 region with G data */
   REGION *reg2;
   char *reg3Str = NULL;		/* U16 region with B data */
   REGION *reg3;
   char *lutStr = NULL;			/* U8 region with LUT */
   REGION *lut;
   REGION **uregions;			/* regions to return */
   int separate = 0;			/* Scale each band separately? */
   int i;

   shErrStackClear();

   a_i = 1;
   regIntRGBToU8LUT_opts[a_i++].dst = &reg1Str;
   regIntRGBToU8LUT_opts[a_i++].dst = &reg2Str;
   regIntRGBToU8LUT_opts[a_i++].dst = &reg3Str;
   regIntRGBToU8LUT_opts[a_i++].dst = &lutStr;
   regIntRGBToU8LUT_opts[a_i++].dst = &separate;
   shAssert(regIntRGBToU8LUT_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, regIntRGBToU8LUT_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     regIntRGBToU8LUT_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,reg1Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"regIntRGBToU8LUT: "
                    "argument \"reg1\" is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg1 = hand.ptr;

   if(reg1->type != TYPE_U16) {
      Tcl_SetResult(interp,"regIntRGBToU8LUT: "
                    "Region \"reg1\" is not U16",TCL_STATIC);
      return(TCL_ERROR);
   }
   
   if(shTclHandleExprEval(interp,reg2Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"regIntRGBToU8LUT: "
                    "argument \"reg2\" is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg2 = hand.ptr;

   if(reg2->type != TYPE_U16) {
      Tcl_SetResult(interp,"regIntRGBToU8LUT: "
                    "Region \"reg2\" is not U16",TCL_STATIC);
      return(TCL_ERROR);
   }
   
   if(shTclHandleExprEval(interp,reg3Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"regIntRGBToU8LUT: "
                    "argument \"reg3\" is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg3 = hand.ptr;

   if(reg3->type != TYPE_U16) {
      Tcl_SetResult(interp,"regIntRGBToU8LUT: "
                    "Region \"reg3\" is not U16",TCL_STATIC);
      return(TCL_ERROR);
   }
   
   if(shTclHandleExprEval(interp,lutStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"regIntRGBToU8LUT: "
                    "argument \"lut\" is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   lut = hand.ptr;

   if(lut->type != TYPE_U8) {
      Tcl_SetResult(interp,"regIntRGBToU8Linear: "
                    "Region \"lut\" is not U8",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   uregions = phRegIntRGBToU8LUT(reg1, reg2, reg3, lut, separate);
/*
 * Return the answer. If we fail to bind the handles, we clean up the regions,
 * but allow any successful handles to form a memory leak. Yes, I'm lazy,
 * but I've never seen p_shTclHandleNew() fail. RHL
 */
   for(i = 0; i < 3; i++) {
      if(p_shTclHandleNew(interp,name) != TCL_OK) {
	 shTclInterpAppendWithErrStack(interp);
	 for(i = 0; i < 3; i++) {
	    shRegDel(uregions[i]);
	 }

	 return(TCL_ERROR);
      }
      hand.ptr = uregions[i];
      hand.type = shTypeGetFromName("REGION");
      
      if(p_shTclHandleAddrBind(interp,hand,name) != TCL_OK) {
	 for(i = 0; i < 3; i++) {
	    shRegDel(uregions[i]);
	 }
	 Tcl_SetResult(interp,"can't bind \"name\" to new REGION handle",
								   TCL_STATIC);
	 return(TCL_ERROR);
      }
      
      Tcl_AppendElement(interp, name);
   }

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * write a postscript file from a U8 region
 */
static char *tclOnecolorPostscriptWrite_use =
  "USAGE: onecolorPostscriptWrite <file> <reg> -size -xoff -yoff";
#define tclOnecolorPostscriptWrite_hlp \
  "Write a postscript file from a U8 region"

static ftclArgvInfo onecolorPostscriptWrite_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclOnecolorPostscriptWrite_hlp},
   {"<file>", FTCL_ARGV_STRING, NULL, NULL, "File to write"},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL,"U8 region to convert to postscript"},
   {"-size", FTCL_ARGV_DOUBLE, NULL, NULL, "Size of bounding box (inches)"},
   {"-xoff", FTCL_ARGV_DOUBLE, NULL, NULL, "x-origin of bounding box (inches)"},
   {"-yoff", FTCL_ARGV_DOUBLE, NULL, NULL, "y-origin of bounding box (inches)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define onecolorPostscriptWrite_name "onecolorPostscriptWrite"

static int
tclOnecolorPostscriptWrite(ClientData clientData,
			   Tcl_Interp *interp,
			   int ac,
			   char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *fileStr = NULL;		/* File to write */
   char *regStr = NULL;			/* U8 region to convert to postscript */
   REGION *reg;
   double size = 8.0;			/* Size of bounding box (inches) */
   double xoff = 0.25;			/* x-origin of bounding box (inches) */
   double yoff = 1.25;			/* y-origin of bounding box (inches) */

   shErrStackClear();

   i = 1;
   onecolorPostscriptWrite_opts[i++].dst = &fileStr;
   onecolorPostscriptWrite_opts[i++].dst = &regStr;
   onecolorPostscriptWrite_opts[i++].dst = &size;
   onecolorPostscriptWrite_opts[i++].dst = &xoff;
   onecolorPostscriptWrite_opts[i++].dst = &yoff;
   shAssert(onecolorPostscriptWrite_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, onecolorPostscriptWrite_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     onecolorPostscriptWrite_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_AppendResult(interp,"onecolorPostscriptWrite: "
                       "argument \"", regStr, "\" is not an REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(reg->type != TYPE_U8) {
      Tcl_SetResult(interp,"onecolorPostscriptWrite: "
                    "Region must be U8 REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   if(phOnecolorPostscriptWrite(fileStr, reg, size, xoff, yoff) != 0) {
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * write true-colour postscript from three regions
 */
static char *tclTruecolorPostscriptWrite_use =
  "USAGE: truecolorPostscriptWrite <file> <regR> <regG> <regB> -size -xoff -yoff";
#define tclTruecolorPostscriptWrite_hlp \
  "Given three regions, write true-colour postscript"

static ftclArgvInfo truecolorPostscriptWrite_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclTruecolorPostscriptWrite_hlp},
   {"<file>", FTCL_ARGV_STRING, NULL, NULL, "File to write"},
   {"<reg1>", FTCL_ARGV_STRING, NULL, NULL, "Region to provide R data"},
   {"<reg2>", FTCL_ARGV_STRING, NULL, NULL, "Region to provide G data"},
   {"<reg3>", FTCL_ARGV_STRING, NULL, NULL, "Region to provide B data"},
   {"-size", FTCL_ARGV_DOUBLE, NULL, NULL, "Size of bounding box (inches)"},
   {"-xoff", FTCL_ARGV_DOUBLE, NULL, NULL,"x-origin of bounding box (inches)"},
   {"-yoff", FTCL_ARGV_DOUBLE, NULL, NULL,"y-origin of bounding box (inches)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define truecolorPostscriptWrite_name "truecolorPostscriptWrite"

static int
tclTruecolorPostscriptWrite(ClientData clientData,
			    Tcl_Interp *interp,
			    int ac,
			    char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *fileStr = NULL;		/* File to write */
   char *reg1Str = NULL;		/* Region to provide R data */
   REGION *reg1;
   char *reg2Str = NULL;		/* Region to provide G data */
   REGION *reg2;
   char *reg3Str = NULL;		/* Region to provide B data */
   REGION *reg3;
   double size = 8.0;			/* Size of bounding box (inches) */
   double xoff = 0.25;			/* x-origin of bounding box (inches) */
   double yoff = 1.25;			/* y-origin of bounding box (inches) */

   shErrStackClear();

   i = 1;
   truecolorPostscriptWrite_opts[i++].dst = &fileStr;
   truecolorPostscriptWrite_opts[i++].dst = &reg1Str;
   truecolorPostscriptWrite_opts[i++].dst = &reg2Str;
   truecolorPostscriptWrite_opts[i++].dst = &reg3Str;
   truecolorPostscriptWrite_opts[i++].dst = &size;
   truecolorPostscriptWrite_opts[i++].dst = &xoff;
   truecolorPostscriptWrite_opts[i++].dst = &yoff;
   shAssert(truecolorPostscriptWrite_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, truecolorPostscriptWrite_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     truecolorPostscriptWrite_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,reg1Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"truecolorPostscriptWrite: "
                    "argument \"reg1\" is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg1 = hand.ptr;

   if(shTclHandleExprEval(interp,reg2Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"truecolorPostscriptWrite: "
                    "argument \"reg2\" is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg2 = hand.ptr;

   if(shTclHandleExprEval(interp,reg3Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"truecolorPostscriptWrite: "
                    "argument \"reg3\" is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg3 = hand.ptr;

   if(reg1->type != TYPE_U8 || reg2->type != TYPE_U8 ||
						      reg3->type != TYPE_U8) {
      Tcl_SetResult(interp,"truecolorPostscriptWrite: "
                    "All regions must be U8 REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   if(reg1->nrow != reg2->nrow || reg1->nrow != reg3->nrow) {
      Tcl_SetResult(interp,"truecolorPostscriptWrite: "
                    "All regions must have the same number of rows",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
   if(reg1->ncol != reg2->ncol || reg1->ncol != reg3->ncol) {
      Tcl_SetResult(interp,"truecolorPostscriptWrite: "
                    "All regions must have the same number of columns",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   if(phTruecolorPostscriptWrite(fileStr, reg1, reg2, reg3,
						      size, xoff, yoff) != 0) {
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Write a PGM file
 */
static char *tclOnecolorPGMWrite_use =
  "USAGE: onecolorPGMWrite <file> <reg>";
#define tclOnecolorPGMWrite_hlp \
  "Write a PGM file from a U8 region"

static ftclArgvInfo onecolorPGMWrite_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclOnecolorPGMWrite_hlp},
   {"<file>", FTCL_ARGV_STRING, NULL, NULL, "File to write"},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "U8 region to convert"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define onecolorPGMWrite_name "onecolorPGMWrite"

static int
tclOnecolorPGMWrite(ClientData clientData,
		    Tcl_Interp *interp,
		    int ac,
		    char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *fileStr = NULL;		/* File to write */
   char *regStr = NULL;			/* U8 region to convert to postscript */
   REGION *reg;

   shErrStackClear();

   i = 1;
   onecolorPGMWrite_opts[i++].dst = &fileStr;
   onecolorPGMWrite_opts[i++].dst = &regStr;
   shAssert(onecolorPGMWrite_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, onecolorPGMWrite_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     onecolorPGMWrite_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_AppendResult(interp,"onecolorPGMWrite: "
                       "argument \"", regStr, "\" is not an REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(reg->type != TYPE_U8) {
      Tcl_SetResult(interp,"onecolorPPMWrite: "
                    "Region must be U8",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   if(phOnecolorPGMWrite(fileStr, reg) != 0) {
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * write a PPM file
 */
static char *tclTruecolorPPMWrite_use =
  "USAGE: truecolorPPMWrite <file> <reg1> <reg2> <reg3>";
#define tclTruecolorPPMWrite_hlp \
  "Write a PPM file from three U8 REGIONs"

static ftclArgvInfo truecolorPPMWrite_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclTruecolorPPMWrite_hlp},
   {"<file>", FTCL_ARGV_STRING, NULL, NULL, "Name of file to write"},
   {"<reg1>", FTCL_ARGV_STRING, NULL, NULL, "Region to provide R data"},
   {"<reg2>", FTCL_ARGV_STRING, NULL, NULL, "Region to provide G data"},
   {"<reg3>", FTCL_ARGV_STRING, NULL, NULL, "Region to provide B data"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define truecolorPPMWrite_name "truecolorPPMWrite"

static int
tclTruecolorPPMWrite(ClientData clientData,
		     Tcl_Interp *interp,
		     int ac,
		     char **av)
{
   HANDLE hand;
   int i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *fileStr = NULL;		/* Name of file to write */
   char *reg1Str = NULL;		/* Region to provide R data */
   REGION *reg1;
   char *reg2Str = NULL;		/* Region to provide G data */
   REGION *reg2;
   char *reg3Str = NULL;		/* Region to provide B data */
   REGION *reg3;

   shErrStackClear();

   i = 1;
   truecolorPPMWrite_opts[i++].dst = &fileStr;
   truecolorPPMWrite_opts[i++].dst = &reg1Str;
   truecolorPPMWrite_opts[i++].dst = &reg2Str;
   truecolorPPMWrite_opts[i++].dst = &reg3Str;
   shAssert(truecolorPPMWrite_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, truecolorPPMWrite_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     truecolorPPMWrite_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,reg1Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"truecolorPPMWrite: "
                    "argument \"reg1\" is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg1 = hand.ptr;

   if(shTclHandleExprEval(interp,reg2Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"truecolorPPMWrite: "
                    "argument \"reg2\" is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg2 = hand.ptr;

   if(shTclHandleExprEval(interp,reg3Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"truecolorPPMWrite: "
                    "argument \"reg3\" is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg3 = hand.ptr;

   if(reg1->type != TYPE_U8 || reg2->type != TYPE_U8 ||
						      reg3->type != TYPE_U8) {
      Tcl_SetResult(interp,"truecolorPPMWrite: "
                    "All regions must be U8 REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   if(reg1->nrow != reg2->nrow || reg1->nrow != reg3->nrow) {
      Tcl_SetResult(interp,"truecolorPPMWrite: "
                    "All regions must have the same number of rows",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
   if(reg1->ncol != reg2->ncol || reg1->ncol != reg3->ncol) {
      Tcl_SetResult(interp,"truecolorPPMWrite: "
                    "All regions must have the same number of columns",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   if(phTruecolorPPMWrite(fileStr, reg1, reg2, reg3) != 0) {
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclFixSaturatedU8Regs_use =
  "USAGE: fixSaturatedU8Regs <regR> <regG> <regB> -neighbors [regU16R] [regU16G] [regU16B] -minU16 ival -fixupLevel ival -fixAllSaturated";
#define tclFixSaturatedU8Regs_hlp \
  "Fix all regions with a pixel value of MAX_U8 in at least one band to have\n\
the mean colour of (U8)-saturated pixels.\n\
\n\
If you specify -edge, the colour is taken from the non-saturated pixels\n\
neighbouring the saturated ones\n\
"

static ftclArgvInfo fixSaturatedU8Regs_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclFixSaturatedU8Regs_hlp},
   {"<regR>", FTCL_ARGV_STRING, NULL, NULL, "R-band U8 REGION"},
   {"<regG>", FTCL_ARGV_STRING, NULL, NULL, "G-band U8 REGION"},
   {"<regB>", FTCL_ARGV_STRING, NULL, NULL, "B-band U8 REGION"},
   {"[regU16R]", FTCL_ARGV_STRING, NULL, NULL,
      "R-band U16 region (used where regR is saturated)"},
   {"[regU16G]", FTCL_ARGV_STRING, NULL, NULL,
      "G-band U16 region (used where regG is saturated)"},
   {"[regU16B]", FTCL_ARGV_STRING, NULL, NULL,
      "B-band U16 region (used where regB is saturated)"},
   {"-minU16", FTCL_ARGV_INT, NULL, NULL, "U16 value that corresponds to U8 == 0"},
   {"-neighbors", FTCL_ARGV_CONSTANT, (void *)1, NULL,
			      "Use colour of neighbours, not the mean colour"},
   {"-fixupLevel", FTCL_ARGV_INT, NULL, NULL, "Fix regions with pixels exceeding <level>"},
   {"-fixAllSaturated", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Fix _all_ saturated pixels"},   
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define fixSaturatedU8Regs_name "fixSaturatedU8Regs"

static int
tclFixSaturatedU8Regs(ClientData clientData,
		      Tcl_Interp *interp,
		      int ac,
		      char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regRStr = NULL;		/* R-band U8 REGION */
   REGION *regR;
   char *regGStr = NULL;		/* G-band U8 REGION */
   REGION *regG;
   char *regBStr = NULL;		/* B-band U8 REGION */
   REGION *regB;
   char *regU16RStr = NULL;		/* R-band U16 region */
   REGION *regU16R;
   char *regU16GStr = NULL;		/* G-band U16 region */
   REGION *regU16G;
   char *regU16BStr = NULL;		/* B-band U16 region */
   REGION *regU16B;
   int minU16 = SOFT_BIAS;		/* U16 value that maps to U8 == 0 */
   int neighbors = 0;			/* Use colour of neighbours,
					   not the mean colour */
   int fixupLevel = 0;			/* Fix regions with pixels exceeding <level> */
   int fixAllSaturated = 0;	      /* Fix _all_ saturated pixels */

   shErrStackClear();

   a_i = 1;
   fixSaturatedU8Regs_opts[a_i++].dst = &regRStr;
   fixSaturatedU8Regs_opts[a_i++].dst = &regGStr;
   fixSaturatedU8Regs_opts[a_i++].dst = &regBStr;
   fixSaturatedU8Regs_opts[a_i++].dst = &regU16RStr;
   fixSaturatedU8Regs_opts[a_i++].dst = &regU16GStr;
   fixSaturatedU8Regs_opts[a_i++].dst = &regU16BStr;
   fixSaturatedU8Regs_opts[a_i++].dst = &minU16;
   fixSaturatedU8Regs_opts[a_i++].dst = &neighbors;
   fixSaturatedU8Regs_opts[a_i++].dst = &fixupLevel;
   fixSaturatedU8Regs_opts[a_i++].dst = &fixAllSaturated;
   shAssert(fixSaturatedU8Regs_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, fixSaturatedU8Regs_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     fixSaturatedU8Regs_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,regRStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_AppendResult(interp,"fixSaturatedU8Regs: "
                       "argument \"", regRStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   regR = hand.ptr;

   if(regR->type != TYPE_U8) {
      Tcl_SetResult(interp, "fixSaturatedU8Regs: regions must be U8:",
		    TCL_STATIC);
      Tcl_AppendResult(interp, regRStr, (char *)NULL);
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,regGStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_AppendResult(interp,"fixSaturatedU8Regs: "
                       "argument \"", regGStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   regG = hand.ptr;

   if(regG->type != TYPE_U8) {
      Tcl_SetResult(interp, "fixSaturatedU8Regs: regions must be U8:",
		    TCL_STATIC);
      Tcl_AppendResult(interp, regGStr, (char *)NULL);
      return(TCL_ERROR);
   }


   if(shTclHandleExprEval(interp,regBStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_AppendResult(interp,"fixSaturatedU8Regs: "
                       "argument \"", regBStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   regB = hand.ptr;

   if(regB->type != TYPE_U8) {
      Tcl_SetResult(interp, "fixSaturatedU8Regs: regions must be U8:",
		    TCL_STATIC);
      Tcl_AppendResult(interp, regBStr, (char *)NULL);
      return(TCL_ERROR);
   }
/*
 * Now optional U16 region arguments
 */
   if(regU16RStr == NULL ||
      *regU16RStr == '\0' || strcmp(regU16RStr, "NULL") == 0) {
      if(regU16GStr != NULL &&
	 *regU16GStr != '\0' && strcmp(regU16GStr, "NULL") != 0) {
	 Tcl_AppendResult(interp,"fixSaturatedU8Regs: "
			  "if regU16 is absent, please omit regU16G",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
      if(regU16BStr != NULL &&
	 *regU16BStr != '\0' && strcmp(regU16BStr, "NULL") != 0) {
	 Tcl_AppendResult(interp,"fixSaturatedU8Regs: "
			  "if regU16 is absent, please omit regU16B",
			  (char *)NULL);
	 return(TCL_ERROR);
      }

      regU16R = regU16G = regU16B = NULL;
   } else {
      if(regU16GStr == NULL ||
	 *regU16GStr == '\0' || strcmp(regU16GStr, "NULL") == 0) {
	 Tcl_AppendResult(interp,"fixSaturatedU8Regs: "
			  "if regU16 is specified, please provide regU16G too",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
      if(regU16BStr == NULL ||
	 *regU16BStr == '\0' || strcmp(regU16BStr, "NULL") == 0) {
	 Tcl_AppendResult(interp,"fixSaturatedU8Regs: "
			  "if regU16 is specified, please provide regU16B too",
			  (char *)NULL);
	 return(TCL_ERROR);
      }

      if(shTclHandleExprEval(interp,regU16RStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("REGION")) {
	 Tcl_AppendResult(interp,"fixSaturatedU8Regs: "
			  "argument \"", regU16RStr, "\" is not a REGION",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
      regU16R = hand.ptr;

      if(regU16R->type != TYPE_PIX) {
	 Tcl_SetResult(interp, "fixSaturatedU8Regs: U16 regions must be U16:",
		       TCL_STATIC);
	 Tcl_AppendResult(interp, regRStr, (char *)NULL);
	 return(TCL_ERROR);
      }

      if(regR->nrow != regU16R->nrow || regR->ncol != regU16R->ncol) {
	 Tcl_SetResult(interp, "fixSaturatedU8Regs: "
		       "U8 regions must be same size as U16:",
		       TCL_STATIC);
	 Tcl_AppendResult(interp, regRStr, (char *)NULL);
	 Tcl_AppendResult(interp, regU16RStr, (char *)NULL);
	 return(TCL_ERROR);
      }

      if(shTclHandleExprEval(interp,regU16GStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("REGION")) {
	 Tcl_AppendResult(interp,"fixSaturatedU8Regs: "
			  "argument \"", regU16GStr, "\" is not a REGION",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
      regU16G = hand.ptr;

      if(regU16G->type != TYPE_PIX) {
	 Tcl_SetResult(interp, "fixSaturatedU8Regs: U16 regions must be U16:",
		       TCL_STATIC);
	 Tcl_AppendResult(interp, regGStr, (char *)NULL);
	 return(TCL_ERROR);
      }

      if(regG->nrow != regU16G->nrow || regG->ncol != regU16G->ncol) {
	 Tcl_SetResult(interp, "fixSaturatedU8Regs: "
		       "U8 regions must be same size as U16:",
		       TCL_STATIC);
	 Tcl_AppendResult(interp, regGStr, (char *)NULL);
	 Tcl_AppendResult(interp, regU16GStr, (char *)NULL);
	 return(TCL_ERROR);
      }

      if(shTclHandleExprEval(interp,regU16BStr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("REGION")) {
	 Tcl_AppendResult(interp,"fixSaturatedU8Regs: "
			  "argument \"", regU16BStr, "\" is not a REGION",
			  (char *)NULL);
	 return(TCL_ERROR);
      }
      regU16B = hand.ptr;

      if(regU16B->type != TYPE_PIX) {
	 Tcl_SetResult(interp, "fixSaturatedU8Regs: U16 regions must be U16:",
		       TCL_STATIC);
	 Tcl_AppendResult(interp, regBStr, (char *)NULL);
	 return(TCL_ERROR);
      }

      if(regB->nrow != regU16B->nrow || regB->ncol != regU16B->ncol) {
	 Tcl_SetResult(interp, "fixSaturatedU8Regs: "
		       "U8 regions must be same size as U16:",
		       TCL_STATIC);
	 Tcl_AppendResult(interp, regBStr, (char *)NULL);
	 Tcl_AppendResult(interp, regU16BStr, (char *)NULL);
	 return(TCL_ERROR);
      }
   }
/*
 * work
 */
   phFixSaturatedU8Regs(regR, regG, regB, regU16R, regU16G, regU16B,
			minU16, fixupLevel, fixAllSaturated,
			(neighbors ? 0 : 1));
   
   return(TCL_OK);
}

/*****************************************************************************/
static char *tclObjectChainSetInRegion_use =
  "USAGE: objectChainSetInRegion <reg> <objChain> -val ival -drow ival -dcol ival";
#define tclObjectChainSetInRegion_hlp \
  ""

static ftclArgvInfo objectChainSetInRegion_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclObjectChainSetInRegion_hlp},
   {"<reg>", FTCL_ARGV_STRING, NULL, NULL, "Region to set pixels in"},
   {"<objChain>", FTCL_ARGV_STRING, NULL, NULL, "Chain of OBJECTs"},
   {"-val", FTCL_ARGV_INT, NULL, NULL, "Value to set <reg> to"},
   {"-drow", FTCL_ARGV_INT, NULL, NULL, "Number of rows to offset OBJECTs"},
   {"-dcol", FTCL_ARGV_INT, NULL, NULL, "Number of columns to offset OBJECTs"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define objectChainSetInRegion_name "objectChainSetInRegion"

static int
tclObjectChainSetInRegion(ClientData clientData,
			  Tcl_Interp *interp,
			  int ac,
			  char **av)
{
   HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *regStr = NULL;			/* Region to set pixels in */
   REGION *reg;
   char *objChainStr = NULL;		/* Chain of OBJECTs */
   CHAIN *objChain;
   int val = 0;				/* Value to set <reg> to */
   int drow = 0;			/* Number of rows to offset OBJECTs */
   int dcol = 0;			/* Number of columns to offset OBJECTs */

   shErrStackClear();

   a_i = 1;
   objectChainSetInRegion_opts[a_i++].dst = &regStr;
   objectChainSetInRegion_opts[a_i++].dst = &objChainStr;
   objectChainSetInRegion_opts[a_i++].dst = &val;
   objectChainSetInRegion_opts[a_i++].dst = &drow;
   objectChainSetInRegion_opts[a_i++].dst = &dcol;
   shAssert(objectChainSetInRegion_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, objectChainSetInRegion_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     objectChainSetInRegion_name) != FTCL_ARGV_SUCCESS) {
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
      Tcl_AppendResult(interp,"objectChainSetInRegion: "
                       "argument \"", regStr, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg = hand.ptr;

   if(shTclHandleExprEval(interp,objChainStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("CHAIN")) {
      Tcl_AppendResult(interp,"objectChainSetInRegion: "
                       "argument \"", objChainStr, "\" is not a CHAIN",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   objChain = hand.ptr;

   if(objChain->type != shTypeGetFromName("OBJECT")) {
      Tcl_AppendResult(interp,
                       "CHAIN \"", objChainStr, "\" doesn't have type OBJECT",
								 (char *)NULL);
      return(TCL_ERROR);
   }   
/*
 * work
 */
   phObjectChainSetInRegion(reg, objChain, val, drow, dcol);

   return(TCL_OK);
}

static char *tclRegU8Median_use =
  "USAGE: regU8Median <reg1> <reg2> <reg3> -size ival -intensity -chromaticity -saturation";
#define tclRegU8Median_hlp \
  "Median filter a set of 3 U8 regions"

static ftclArgvInfo regU8Median_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRegU8Median_hlp},
   {"<reg1>", FTCL_ARGV_STRING, NULL, NULL, "R-region"},
   {"<reg2>", FTCL_ARGV_STRING, NULL, NULL, "G-region"},
   {"<reg3>", FTCL_ARGV_STRING, NULL, NULL, "B-region"},   
   {"-size", FTCL_ARGV_INT, NULL, NULL, "Size of filter"},
   {"-hue", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Median filter hue"},
   {"-saturation", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Median filter saturation"},   
   {"-intensity", FTCL_ARGV_CONSTANT, (void *)1, NULL, "Median filter intensity (default)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define regU8Median_name "regU8Median"

static int
tclRegU8Median(ClientData clientData,
	       Tcl_Interp *interp,
	       int ac,
	       char **av)
{
    HANDLE hand;
   int a_i;
   void *vptr;                          /* used by shTclHandleExprEval */
   char *reg1Str = NULL;		/* R-region */
   REGION *reg1;
   char *reg2Str = NULL;		/* G-region */
   REGION *reg2;
   char *reg3Str = NULL;		/* B-region */
   REGION *reg3;
   int size = 0;			/* Size of filter */
   int intensity = 0;			/* Median filter intensity */
   int hue = 0;		/* Median filter hue */
   int saturation = 0;			/* Median filter saturation */

   shErrStackClear();

   a_i = 1;
   regU8Median_opts[a_i++].dst = &reg1Str;
   regU8Median_opts[a_i++].dst = &reg2Str;
   regU8Median_opts[a_i++].dst = &reg3Str;
   regU8Median_opts[a_i++].dst = &size;
   regU8Median_opts[a_i++].dst = &intensity;
   regU8Median_opts[a_i++].dst = &hue;
   regU8Median_opts[a_i++].dst = &saturation;
   shAssert(regU8Median_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, regU8Median_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     regU8Median_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * deal with arguments
 */
   if(shTclHandleExprEval(interp,reg1Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_AppendResult(interp,"regU8Median: "
                       "argument \"", reg1Str, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg1 = hand.ptr;

   if(shTclHandleExprEval(interp,reg2Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_AppendResult(interp,"regU8Median: "
                       "argument \"", reg2Str, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg2 = hand.ptr;

   if(shTclHandleExprEval(interp,reg3Str,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_AppendResult(interp,"regU8Median: "
                       "argument \"", reg3Str, "\" is not a REGION",
                       (char *)NULL);
      return(TCL_ERROR);
   }
   reg3 = hand.ptr;

   if(size == 0) {
       size = 3;
   }
   switch (intensity + hue + saturation) {
     case 0:
       intensity = 1;
       break;
     case 1:
       break;
     default:
       Tcl_SetResult(interp, "regU8Median: Please choose only one of -intensity -hue -saturation", TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * work
 */
   if(phRegU8Median(reg1, reg2, reg3, size,
		    (hue ? 0 : (saturation ? 1 : 2))) < 0) {
       Tcl_SetResult(interp, "regU8Median: ", TCL_STATIC);
   }

   return(TCL_OK);
}



/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclWallpaperDeclare(Tcl_Interp *interp)
{
   char *module = "phTclWallpaper";	/* name of this set of code */

   shTclDeclare(interp,truecolorPostscriptWrite_name,
		(Tcl_CmdProc *)tclTruecolorPostscriptWrite, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclTruecolorPostscriptWrite_hlp,
		tclTruecolorPostscriptWrite_use);

   shTclDeclare(interp,regIntToU8Linear_name,
		(Tcl_CmdProc *)tclRegIntToU8Linear, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegIntToU8Linear_hlp,
		tclRegIntToU8Linear_use);

   shTclDeclare(interp,regIntToU8LUT_name,
		(Tcl_CmdProc *)tclRegIntToU8LUT, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegIntToU8LUT_hlp,
		tclRegIntToU8LUT_use);

   shTclDeclare(interp,truecolorPPMWrite_name,
		(Tcl_CmdProc *)tclTruecolorPPMWrite, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclTruecolorPPMWrite_hlp,
		tclTruecolorPPMWrite_use);

   shTclDeclare(interp,regIntRGBToU8LUT_name,
		(Tcl_CmdProc *)tclRegIntRGBToU8LUT, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegIntRGBToU8LUT_hlp,
		tclRegIntRGBToU8LUT_use);

   shTclDeclare(interp,onecolorPostscriptWrite_name,
		(Tcl_CmdProc *)tclOnecolorPostscriptWrite, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclOnecolorPostscriptWrite_hlp,
		tclOnecolorPostscriptWrite_use);

   shTclDeclare(interp,onecolorPGMWrite_name,
		(Tcl_CmdProc *)tclOnecolorPGMWrite, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclOnecolorPGMWrite_hlp,
		tclOnecolorPGMWrite_use);

   shTclDeclare(interp,fixSaturatedU8Regs_name,
		(Tcl_CmdProc *)tclFixSaturatedU8Regs, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclFixSaturatedU8Regs_hlp,
		tclFixSaturatedU8Regs_use);

   shTclDeclare(interp,objectChainSetInRegion_name,
		(Tcl_CmdProc *)tclObjectChainSetInRegion, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclObjectChainSetInRegion_hlp,
		tclObjectChainSetInRegion_use);

   shTclDeclare(interp,regU8Median_name,
		(Tcl_CmdProc *)tclRegU8Median, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRegU8Median_hlp,
		tclRegU8Median_use);
}
