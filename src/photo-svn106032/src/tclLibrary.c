/*
 * TCL support for LIBRARY routines in photo.
 *
 * ENTRY POINT		SCOPE
 * tclPhotparDeclare	public	Declare all the verbs defined in this module
 *
 * REQUIRED PRODUCTS:
 *	FTCL		TCL + XTCL + Fermilab extensions
 *
 * 
 * The following TCL verbs are declared herein:
 *       
 *      histU16FromRegion */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "phConsts.h"
#include "phLibrary.h"
#include "phUtils.h"

#define BIGSTR    5000                   /* a long, long string */
#define ANNULEN   200          /* length of string which hold annulus radii */

static char *module = "phTclLibrary";    /* name of this set of code */
static char err_msg[BIGSTR];		/* used for parsing error messages */

static char *regstr = NULL;
static char *maskstr = NULL;
static int clip_flag = 0;
static double nsigma = 0;

/*************************************************************************
 * call the phHistU16FromRegion function; bind a handle to the returned
 * PHHIST structure, and print out the handle's name.
 */

static char *tclHistU16FromRegion_use =
  "USAGE: histU16FromRegion region -mask";
static char *tclHistU16FromRegion_hlp =
  "create a new PHHIST structure using pixels from the given REGION. If objmask is used, masked pixels are ignored";

static ftclArgvInfo histU16FromRegion_argTable[] = {
   { (char *) NULL, FTCL_ARGV_HELP, NULL, NULL,
         "create a new PHHIST structure using pixels from the given REGION\nnot include in the given mask" },
   { "<region>", FTCL_ARGV_STRING, NULL, &regstr,
         "region from which to build histogram" },
   { "-mask", FTCL_ARGV_STRING, NULL, &maskstr,
         "ignore pixels in this (obj)mask" },
   { (char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL }
};


static int
tclHistU16FromRegion(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   char name[HANDLE_NAMELEN];
   HANDLE *regHandle, histHandle;
   REGION *reg;
   PHHIST *hist;
   HANDLE *maskHandle;
   CHAIN *mask=NULL;

   shErrStackClear();

   if(get_FtclOpts(interp, &argc, argv, histU16FromRegion_argTable) != TCL_OK){
      return(TCL_ERROR);
   }

   /* get the input region */
   if (p_shTclHandleAddrGet(interp, regstr, &regHandle) != TCL_OK) {
      return(TCL_ERROR);
   }
   if (regHandle->type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp, "tclHistU16FromRegion: first arg is not a REGION",
            TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = regHandle->ptr;
   if (reg->type != TYPE_U16) {
      Tcl_SetResult(interp, "tclHistU16FromRegion: region must be TYPE_U16",
		    TCL_STATIC);
      return(TCL_ERROR);
  }
   /* get the mask */
   if (maskstr != NULL) {
       if (p_shTclHandleAddrGet(interp, maskstr, &maskHandle) != TCL_OK) {
	   return(TCL_ERROR);
       }
       if (maskHandle->type != shTypeGetFromName("CHAIN")) {
	   Tcl_SetResult(interp, "tclHistU16FromRegion: second arg is not a CHAIN",
			 TCL_STATIC);
	   return(TCL_ERROR);
       }
       mask = maskHandle->ptr;
       if(mask->type != shTypeGetFromName("OBJMASK")) {
	   Tcl_SetResult(interp,"tclHistU16FromRegion: "
			 "CHAIN is not of type \"OBJMASK\"",TCL_STATIC);
	   return(TCL_ERROR);
       }
   }
   /* now, call the C function */
   if ((hist = phHistU16FromRegion(reg, mask)) == NULL) {
      Tcl_SetResult(interp, "tclHistU16FromRegion: phHistU16FromRegion returns NULL", TCL_STATIC);
      return(TCL_ERROR);
   }

   /* create a new handle and bind it to the new histogram structure */
   if (p_shTclHandleNew(interp, name) != TCL_OK) {
      phPhhistDel(hist);
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   histHandle.ptr = hist;
   histHandle.type = shTypeGetFromName("PHHIST");
   if (p_shTclHandleAddrBind(interp, histHandle, name) != TCL_OK) {
      Tcl_SetResult(interp, "can't bind to new PHHIST handle", TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);
   return(TCL_OK);
}

/*************************************************************************
 * call the phRegStats function; return the output values
 * in the form of a keyed list.
 */

static char *tclRegStats_use =
  "USAGE: regStats region -mask -clip nsigma";
static char *tclRegStats_hlp =
  "given a REGION, calculate the quartiles, mean, mode and sigma using all pixels in the given REGION. If an objmask is used, masked pixesl are ignored. If -clip specified, use only pixels within +/- nsigma of the median.";

static ftclArgvInfo regStats_argTable[] = {
   { (char *) NULL, FTCL_ARGV_HELP, NULL, NULL,
         "given a REGION, calculate the quartiles, mean, mode and sigma using all pixels in the given REGION. If an objmask is used, masked pixesl are ignored. If -clip specified, use only pixels within +/- nsigma of the median." },
   { "<region>", FTCL_ARGV_STRING, NULL, &regstr,
         "region from which to build histogram" },
   { "-mask", FTCL_ARGV_STRING, NULL, &maskstr,
         "ignore pixels in this (obj)mask" },
   { "-clip", FTCL_ARGV_DOUBLE, 0, &nsigma,
	 "use only points within nsigma of median of region value" },
   { (char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL }
};


static int
tclRegStats(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE hand;
   REGION *reg;
   float mean, mode, sigma, quartiles[3];
   CHAIN *mask=NULL;
   void *vptr;

   /* 
    * we have to set this by hand upon entering function; otherwise,
    * it retains value that user last set.
    */
   clip_flag = 0;
   nsigma = 0;
   shErrStackClear();

   if(get_FtclOpts(interp, &argc, argv, regStats_argTable) != TCL_OK) {
      return(TCL_ERROR);
   }

   /* get the input region */
   if(shTclHandleExprEval(interp,regstr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_SetResult(interp,"regStats: "
                    "argument \"reg\" is not a REGION",TCL_STATIC);
      return(TCL_ERROR);
   }
   reg = hand.ptr;
   /* get the mask */
   if (maskstr != NULL) {
      if(shTclHandleExprEval(interp,maskstr,&hand,&vptr) != TCL_OK) {
	 return(TCL_ERROR);
      }
      if(hand.type != shTypeGetFromName("CHAIN")) {
	 Tcl_SetResult(interp,"regStats: "
		       "argument \"mask\" is not a CHAIN",TCL_STATIC);
	 return(TCL_ERROR);
      }
      mask = hand.ptr;
      if(mask->type != shTypeGetFromName("OBJMASK")) {
	 Tcl_SetResult(interp,"regStats: "
		       "CHAIN is not of type \"OBJMASK\"",TCL_STATIC);
	 return(TCL_ERROR);
      }
   }
   /* now, call the C function */
   if (phRegStats(reg, nsigma, mask, 
	       quartiles, &mean, &mode, &sigma) != SH_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   /* 
    * now put the output -- quartiles, mean, mode and sigma --
    * into a keyed list 
    */
   sprintf(err_msg, "quartiles \"%.2f %.2f %.2f\"", 
	       quartiles[0], quartiles[1], quartiles[2]);
   Tcl_AppendElement(interp, err_msg);
   sprintf(err_msg, "mean \"%.2f\"", mean);
   Tcl_AppendElement(interp, err_msg);
   sprintf(err_msg, "mode \"%.2f\"", mode);
   Tcl_AppendElement(interp, err_msg);
   sprintf(err_msg, "sigma \"%.2f\"", sigma);
   Tcl_AppendElement(interp, err_msg);

   return(TCL_OK);
}

/**************************************************************************
 * Declare my new tcl verbs to tcl
 */
void
phTclLibraryDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"histU16FromRegion",
                (Tcl_CmdProc *)tclHistU16FromRegion,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclHistU16FromRegion_hlp, tclHistU16FromRegion_use);

   shTclDeclare(interp,"regStats",
                (Tcl_CmdProc *)tclRegStats,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclRegStats_hlp, tclRegStats_use);
}

