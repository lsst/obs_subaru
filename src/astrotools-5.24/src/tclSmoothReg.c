/* <AUTO>
   FILE: tclSmoothReg
   ABSTRACT:
<HTML>
   Routines to smooth images
</HTML>
   </AUTO>
*/
/***************************************************************
 *
 *              atSmoothReg
 *
 *              written: Sept 10, 1999
 *                       Douglas L. Tucker
 *
 *
 **************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "atSmoothReg.h"
#include "atGauss.h"

static char *module = "astrotools";   /* name of this set of code */

/**************************************************************
                      tclRegSmoothWithMedian
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: regSmoothWithMedian

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atSmoothReg.html#atRegSmoothWithMedian>atRegSmoothWithMedian</A> 
                 in atSmoothReg.c

<P>
  Smooth a region using a running median box filter.

  </HTML>

</AUTO>*/

static ftclArgvInfo regSmoothWithMedianTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL,
  "Smooth a region using a running median box filter.\n"},
  {"<inRegion>", FTCL_ARGV_STRING, NULL, NULL, "input region handle"},
  {"<outRegion>", FTCL_ARGV_STRING, NULL, NULL, "output region handle"},
  {"<nSize>", FTCL_ARGV_INT, NULL, NULL, "linear size of median box in pixels (must be odd number)"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *regSmoothWithMedian_name = "regSmoothWithMedian";

static int
tclRegSmoothWithMedian(
                ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                )
{
                                /* INPUT */
  REGION *inRegion  = (REGION *)0;		/* input region */
  int    nSize=0;                               /* linear size of median box in pixels */

                                /* OUTPUT */
  REGION *outRegion = (REGION *)0;		/* output region */

  char *inRegionName;
  char *outRegionName;
  char answer[80];              /* sent back to Tcl */


/* Parse the command */

  regSmoothWithMedianTable[1].dst = &inRegionName;
  regSmoothWithMedianTable[2].dst = &outRegionName;
  regSmoothWithMedianTable[3].dst = &nSize;

  if ((shTclParseArgv( interp, &argc, argv, regSmoothWithMedianTable, 0, 
	regSmoothWithMedian_name)) != FTCL_ARGV_SUCCESS) {
        return(TCL_ERROR);
  }

  if (shTclAddrGetFromName (interp, inRegionName, 
     (void **) &inRegion, "REGION") != TCL_OK) {
	Tcl_SetResult
	(interp, "regSmoothWithMedian: bad input region name", TCL_VOLATILE);
	return TCL_ERROR;
  }

  if (shTclAddrGetFromName (interp, outRegionName, 
     (void **) &outRegion, "REGION") != TCL_OK) {
	Tcl_SetResult
	(interp, "regSmoothWithMedian: bad output region name", TCL_VOLATILE);
	return TCL_ERROR;
  }

  if((inRegion->type != TYPE_FL32) || (outRegion->type != TYPE_FL32)) {
     Tcl_SetResult(interp, 
	"regSmoothWithMedian: command only works with FL32 regions", TCL_VOLATILE);
        return TCL_ERROR;
    }

  atRegSmoothWithMedian(inRegion, outRegion, nSize);

  Tcl_SetResult(interp, "", TCL_VOLATILE);
  sprintf(answer, "%s", outRegionName);
  Tcl_AppendElement(interp, answer);

  return TCL_OK;
}


/*********************************************************************/
void atTclSmoothRegDeclare(

Tcl_Interp *interp)
{
  shTclDeclare(
               interp,
               regSmoothWithMedian_name,
               (Tcl_CmdProc *)tclRegSmoothWithMedian,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, regSmoothWithMedianTable, 0,
				regSmoothWithMedian_name),
               shTclGetUsage(interp, regSmoothWithMedianTable, 0,
				regSmoothWithMedian_name));

}
