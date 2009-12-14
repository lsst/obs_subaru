/*
 * TCL support for phFlatfieldsCalc in photo.
 *
 * ENTRY POINT		SCOPE
 * tclFlatfieldsCalcDeclare	public	Declare all the verbs defined in this module
 *
 * REQUIRED PRODUCTS:
 *	FTCL		TCL + XTCL + Fermilab extensions
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "phQuartiles.h"
#include "phCorrectFrames.h"

static char *module = "phTclPhotoFacil";    /* name of this set of code */

/*****************************************************************************/
/*
 * Convert a QUARTILES to a 2-D flatfield array
 */
static char *tclFlatfieldsCalc_use =
  "USAGE: flatfieldsCalc quartiles bias bias_scale left_buffer right_buffer ccdpars";
static char *tclFlatfieldsCalc_hlp =
  "Convert a QUARTILES structure to a 2-D flatfield vector REGION and produces odd and"
  "even drift bias vectors.  A tcl list {flat2d skylevel1d biasdrift1d Obiasdrift1d Ebiasdrift1d}"
  "is returned as a keyed list";

static int
tclFlatfieldsCalc(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   int bias_scale;			/* factor bias2d is multiplied by */
   int left_buffer;                     /* do not include last left_buffer
                                           pixels in left overscan region when
                                           determining bias drift */
   int right_buffer;                    /* do not include first right_buffer
                                           pixels in right overscan region when
                                           determining bias drift */ 

   HANDLE handle2, handle3, handle4, handle5, handle6;
   QUARTILES *quartiles = NULL;
   REGION *flat2D, *bias = NULL, *skylevel1D, *biasdrift1D, *Obiasdrift1D, *Ebiasdrift1D;
   CCDPARS *ccdpars = NULL;
   char name2[HANDLE_NAMELEN], name3[HANDLE_NAMELEN];
   char name4[HANDLE_NAMELEN], name5[HANDLE_NAMELEN], name6[HANDLE_NAMELEN];
   char *string;
   char s[300];
   char *opts = "quartiles bias bias_scale left_buffer right_buffer ccdpars";
   int ncol;

   shErrStackClear();

   ftclParseSave("flatfieldsCalc");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      string = ftclGetStr("quartiles");
      if(shTclAddrGetFromName(interp,string,(void **)&quartiles,"QUARTILES")
         != TCL_OK) {
         return(TCL_ERROR);
      }
      string = ftclGetStr("bias");
      if(shTclAddrGetFromName(interp,string,(void **)&bias,"REGION")
         != TCL_OK) {
         return(TCL_ERROR);
      }
      bias_scale = ftclGetInt("bias_scale");
      left_buffer = ftclGetInt("left_buffer");
      right_buffer = ftclGetInt("right_buffer");
      string = ftclGetStr("ccdpars");
      if(shTclAddrGetFromName(interp,string,(void **)&ccdpars,"CCDPARS")
         != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclFlatfieldsCalc_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   /* Check to see that the structs are the right size */
   if (ccdpars->ncols != quartiles->nc) {
      Tcl_SetResult(interp,"ccdpars ncol does not match quartiles nc",
            TCL_STATIC);
      return(TCL_ERROR);
   }
   if (bias->ncol != quartiles->nc) {
      Tcl_SetResult(interp,"bias ncol does not match quartiles nc",
            TCL_STATIC);
      return(TCL_ERROR);
   }

   /* Make output regions */
   if (ccdpars->namps > 1) {
       ncol = ccdpars->nData0 + ccdpars->nData1;
       shAssert(ccdpars->namps == 2);
   } else {
       ncol = ccdpars->nData0;       
   }

   if ((flat2D = shRegNew("flat2d", quartiles->nr, ncol, TYPE_U16)) == NULL) {
      Tcl_SetResult(interp,"Could not allocate flat2D", TCL_STATIC);
      return(TCL_ERROR);
   }
   /* We will store skysigma in the second column */
   if ((skylevel1D = shRegNew("skylevel1d", quartiles->nr, 3, TYPE_FL32)) == NULL) {
      Tcl_SetResult(interp,"Could not allocate skylevel1D", TCL_STATIC);
      return(TCL_ERROR);
   }
   if ((biasdrift1D = shRegNew("biasdrift1d", quartiles->nr, 2, TYPE_S16)) == NULL) {
      Tcl_SetResult(interp,"Could not allocate biasdrift1D", TCL_STATIC);
      return(TCL_ERROR);
   }
   if ((Obiasdrift1D = shRegNew("Obiasdrift1d", quartiles->nr, 2, TYPE_S16)) == NULL) {
      Tcl_SetResult(interp,"Could not allocate Obiasdrift1D", TCL_STATIC);
      return(TCL_ERROR);
   }
   if ((Ebiasdrift1D = shRegNew("Ebiasdrift1d", quartiles->nr, 2, TYPE_S16)) == NULL) {
      Tcl_SetResult(interp,"Could not allocate Ebiasdrift1D", TCL_STATIC);
      return(TCL_ERROR);
   }

   if ((phFlatfieldsCalc(quartiles, bias, bias_scale, left_buffer, right_buffer, ccdpars,
			 flat2D, skylevel1D, biasdrift1D, Obiasdrift1D, Ebiasdrift1D)) != SH_SUCCESS) {
      Tcl_SetResult(interp,"Error in phFlatfieldsCalc", TCL_STATIC);
      return TCL_ERROR;
   }

   if (p_shTclHandleNew(interp, name2) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   handle2.ptr = flat2D;
   handle2.type = shTypeGetFromName("REGION");
   if (p_shTclHandleAddrBind(interp, handle2, name2) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new region handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   if (p_shTclHandleNew(interp, name3) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   handle3.ptr = skylevel1D;
   handle3.type = shTypeGetFromName("REGION");
   if (p_shTclHandleAddrBind(interp, handle3, name3) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new region handle 3",TCL_STATIC);
      return(TCL_ERROR);
   }

   if (p_shTclHandleNew(interp, name4) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   handle4.ptr = biasdrift1D;
   handle4.type = shTypeGetFromName("REGION");
   if (p_shTclHandleAddrBind(interp, handle4, name4) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new region handle 4",TCL_STATIC);
      return(TCL_ERROR);
   }

   if (p_shTclHandleNew(interp, name5) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   handle5.ptr = Obiasdrift1D;
   handle5.type = shTypeGetFromName("REGION");
   if (p_shTclHandleAddrBind(interp, handle5, name5) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new region handle 5",TCL_STATIC);
      return(TCL_ERROR);
   }

   if (p_shTclHandleNew(interp, name6) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   handle6.ptr = Ebiasdrift1D;
   handle6.type = shTypeGetFromName("REGION");
   if (p_shTclHandleAddrBind(interp, handle6, name6) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new region handle 6",TCL_STATIC);
      return(TCL_ERROR);
   }

   /* format keyed list */
   sprintf(s, "{flat2d %s} {skylevel1d %s} {biasdrift1d %s} {Obiasdrift1d %s} {Ebiasdrift1d %s}", 
	    name2, name3, name4, name5, name6);
   Tcl_SetResult(interp, s, TCL_VOLATILE);

   ftclParseRestore("flatfieldsCalc");

   return(TCL_OK);
}




/****************************************************************************
 * Declare my new tcl verbs to tcl
 */
void
phTclFlatfieldsCalcDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"flatfieldsCalc",
                (Tcl_CmdProc *)tclFlatfieldsCalc,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclFlatfieldsCalc_hlp, tclFlatfieldsCalc_use);

}

