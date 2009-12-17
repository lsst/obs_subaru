/*
 * FILE:
 *   tclSaoDeclare.c
 *
 * ABSTRACT:
 *   This file contains an important routine to declare all SAO related
 *   TCL verbs to the world. In addition, it also contains some misc-
 *   ellaneous functions that for the lack of a better place, were put
 *   in here.
 *
 * ENTRY POINT            SCOPE    DESCRIPTION
 * ----------------------------------------------------------------------------
 * tclSaoDeclare          public   World's interface to TCL SAO verbs
 * tclSaoInit             public   Initilization function for SAO/TCL
 */

#include <stdio.h>
#include <stdlib.h>       /* For prototype of getenv() */
#include <unistd.h>       /* For prototype of unlink() */
#include <string.h>

#include "tcl.h"
#include "region.h"
#include "shTclSao.h"
#include "shTclVerbs.h"

/* External variable s referenced here */
extern SAOCMDHANDLE g_saoCmdHandle;

/*
 * Global variables defined here and used outside (and inside) this source file
 */
char g_saoTCLList[CMD_NUM_SAOBUTT][TCL_BUTT_FUNC_NAME_LEN];
int  g_saoMouseVerbose;

/*
 * A table of fsao processes their dervish id numbers.
 */
SHSAOPROC g_saoProc[CMD_SAO_MAX];


void tclSaoDeclare(Tcl_Interp *interp)
{
   /*
    * Now declare all TCL verbs defined in various SAO files.
    */
   shTclSaoDisplayDeclare(interp);
   shTclSaoDrawDeclare(interp);
   shTclSaoRegionDeclare(interp);
   shTclSaoMouseDeclare(interp);
   shTclSaoMaskDisplayDeclare(interp);
   return;
}

/*
 * ROUTINE: 
 *   tclSaoInit   
 *
 * CALL:
 *   (RET_CODE) tclSaoInit(
 *                      Tcl_Interp *a_interp
 *                   );
 *
 * DESCRIPTION:
 *	Initialize the structure (handle) used by Cmd routines.
 *
 * RETURN VALUES:
 *	SH_SUCCESS
 *
 * GLOBALS REFERENCED:
 *	g_saoTCLList
 */
RET_CODE 
tclSaoInit(Tcl_Interp *a_interp)
{
   RET_CODE rstatus = SH_SUCCESS;
   int	    i;

   /*
    * There are no tcl functions assigned to any of the saoimage buttons yeT
    */
   for (i=0; i < CMD_NUM_SAOBUTT; i++)
      g_saoTCLList[i][0] = (char)0;

   /* Setup structure pointers */
   for (i=0; i < CMD_SAO_MAX; i++)  {
      shSaoClearMem(&(g_saoCmdHandle.saoHandle[i]));
   }

   g_saoCmdHandle.initflag = (unsigned int)CMD_INIT;

   g_saoMouseVerbose = 0;

   /*
    * Initialize the structure that records the pid values of forked
    * saoimage processes.
    */
   for ( i = 0 ; i < CMD_SAO_MAX ; ++i)   {
      g_saoProc[i].saonum = INIT_SAONUM_VAL;
   }

   return(rstatus);
}
