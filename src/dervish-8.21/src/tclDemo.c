/*
**	tclDemo.c
**
**	Contains TCL verb definition for the demo.  
**
**      NOTE:  The d_regSkyFind routine exists only for the demo.  The REAL
**             sky finding routine is part of the ASTROTOOLS product.
******************************************************************************
******************************************************************************
*/
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include "tcl.h"
#include "ftcl.h"
#include "region.h"
#include "shCSao.h"                      /* prototype of CMD_HANDLE */
#include "shTclRegSupport.h"
#include "shTclUtils.h"
#include "dervish_msg_c.h"
#include "shCDemo.h"
#include "shCRegUtils.h"
#include "shCErrStack.h"
#include "shTclErrStack.h"
#include "shCUtils.h"
#include "shTclVerbs.h"
#include "shTclParseArgv.h"
/*============================================================================
**============================================================================
*/

static char *shtclD_RegSkyFind_hlp = "For demo purposes only. See astrotools for the supported sky finding routine.";
static char *shtclD_RegSkyFind_use = "For demo purposes only. See astrotools for the supported sky finding routine.";

static int shTclD_RegSkyFind (ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{

        double sky;
        REGION *reg;
        char s[20];
	char tclRegionName[MAXTCLREGNAMELEN];
	char errors[80];
	const char *p_errors;

	if(argc==2) {

	  strncpy(tclRegionName,argv[1],MAXTCLREGNAMELEN);

	  if (shTclRegAddrGetFromName(interp, tclRegionName, &reg) 
	      != TCL_OK) {
	    sprintf(errors,"\nCannot find region named: %s",
		    tclRegionName);
	    Tcl_SetResult(interp ,errors,TCL_VOLATILE);
	    return(TCL_ERROR);
	  }

	} else {
	
	  sprintf(errors,"\n%s",shtclD_RegSkyFind_use);
	  Tcl_AppendResult(interp,errors,(char *)NULL);
	  return(TCL_ERROR);

	}

        sky = shD_RegSkyFind(reg);

	p_errors = shErrStackGetEarliest();
	if ( p_errors != NULL) {
	  sprintf(errors,"%s",p_errors);
	  Tcl_SetResult(interp,errors,TCL_VOLATILE);
	  shTclInterpAppendWithErrStack(interp);
	  return(TCL_ERROR);
	}
	


        sprintf (s,"%g",sky);
        Tcl_SetResult (interp, s, TCL_VOLATILE);
        return (TCL_OK);

}
/***************** Declaration of TCL verbs in this module *****************/
static char *shtclD_RegSciFacil = "shDemo";

void shTclDemoDeclare (Tcl_Interp *interp) 
{
       shTclDeclare (interp, "d_regSkyFind", 
                      (Tcl_CmdProc *)shTclD_RegSkyFind,
                      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
                      shtclD_RegSciFacil,
                      shtclD_RegSkyFind_hlp,
                      shtclD_RegSkyFind_use
                      );
}


