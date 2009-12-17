/*<AUTO>
******************************************************************************
**
** FILE:  tclSaoPan.c
**
**	This file contains TCL verb routines used
**	to pan on FSAOimage displays.
**</AUTO>
**   ENTRY POINT	    SCOPE   DESCRIPTION
**   -------------------------------------------------------------------------
**   tclSaoZoom	            local   TCL routine to zoom the image
**   tclSaoCenter	    local   TCL routine to center an image
**   shTclSaoPanDeclare     public  World's interface to SAO pan TCL verbs
**
** ENVIRONMENT:
**	ANSI C.
**
** AUTHORS:     Creation date: June 5, 1997
**
******************************************************************************
******************************************************************************
*/
#include "tcl.h"
#include "region.h"
#include "shCSao.h"
#include "dervish_msg_c.h"        /* Murmur message file for Dervish */
#include "shTclUtils.h"
#include "shCUtils.h"
#include "shCErrStack.h"
#include "shTclErrStack.h"
#include "shTclVerbs.h"
#include "shTclParseArgv.h"

/*============================================================================
**============================================================================
**
** LOCAL PROTOTYPES, DEFINITIONS, MACROS, ETC.
** (NOTE: NONE OF THESE WILL BE VISIBLE OUTSIDE THIS MODULE)
**
**============================================================================
*/

/*----------------------------------------------------------------------------
**
** LOCAL FUNCTION PROTOTYPES
*/
static RET_CODE tclSaoCenter(ClientData, Tcl_Interp *, int, char **);
static RET_CODE tclSaoPan(ClientData, Tcl_Interp *, int, char **);
static RET_CODE tclSaoZoom(ClientData, Tcl_Interp *, int, char **);

/*----------------------------------------------------------------------------
**
** LOCAL DEFINITIONS
*/

/* External variable s referenced here */
extern SAOCMDHANDLE g_saoCmdHandle;

/****************************************************************************
**
** ROUTINE: tclSaoCenter
**
**<AUTO EXTRACT>
** TCL VERB: saoCenter
**
**</AUTO>
*/
char g_saoCenter[] = "saoCenter";
ftclArgvInfo g_saoCenterTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Center the specified image.\n\n" },
  {"-s", FTCL_ARGV_INT, NULL, NULL,
   "FSAOimage display number"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static RET_CODE tclSaoCenter
  (
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
  )   
{ /* tclSaoCenter */

int    retStat = TCL_OK;
int    saoindex = SAOINDEXINITVAL;

/*---------------------------------------------------------------------------
**
** CHECK COMMAND ARGUMENTS
*/
g_saoCenterTbl[1].dst = &saoindex;

if ((retStat = shTclParseArgv(interp, &argc, argv, g_saoCenterTbl,
                              FTCL_ARGV_NO_LEFTOVERS, g_saoCenter))
    == FTCL_ARGV_SUCCESS)
   {
   if (saoindex != SAOINDEXINITVAL)
     {
     --saoindex;                  /* Index starts at 0 not 1 */
     if (saoindex < 0)
       {
       /* Invalid number, must be greater than 0 */
       Tcl_SetResult(interp, "ERROR : Invalid SAO number - must be > zero.",
		     TCL_VOLATILE);
       return(TCL_ERROR);
       }
     else
       {
       if ((saoindex > CMD_SAO_MAX) || 
	   (g_saoCmdHandle.saoHandle[ saoindex].state == SAO_UNUSED))
	 {
	 Tcl_SetResult(interp, "ERROR : No fSAO associated with SAO number.",
		       TCL_VOLATILE);
	 return(TCL_ERROR);
	 }
       }
     }

   if (shSaoCenter(saoindex) != SH_SUCCESS)
     {
     shTclInterpAppendWithErrStack(interp);
     retStat = TCL_ERROR;
     }
   else
     {
     retStat = TCL_OK;
     }
   }

return(retStat);

} /* tclSaoCenter */

/****************************************************************************
**
** ROUTINE: tclSaoPan
**
**<AUTO EXTRACT>
** TCL VERB: saoPan
**
**</AUTO>
*/
char g_saoPan[] = "saoPan";
ftclArgvInfo g_saoPanTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Pan the specified image.\n\n" },
  {"<r>", FTCL_ARGV_INT, NULL, NULL,
   " row number (int)"},
  {"<c>", FTCL_ARGV_INT, NULL, NULL,
   "column number (int)"},
  {"-s", FTCL_ARGV_INT, NULL, NULL,
   "FSAOimage display number"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static RET_CODE tclSaoPan
  (
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
  )   
{ /* tclSaoPan */

int    retStat = TCL_OK;
int    saoindex = SAOINDEXINITVAL;
int    r,c;

/*---------------------------------------------------------------------------
**
** CHECK COMMAND ARGUMENTS
*/
g_saoPanTbl[1].dst = &r;
g_saoPanTbl[2].dst = &c;
g_saoPanTbl[3].dst = &saoindex;

if ((retStat = shTclParseArgv(interp, &argc, argv, g_saoPanTbl,
                              FTCL_ARGV_NO_LEFTOVERS, g_saoPan))
    == FTCL_ARGV_SUCCESS)
   {
   if (saoindex != SAOINDEXINITVAL)
     {
     --saoindex;                  /* Index starts at 0 not 1 */
     if (saoindex < 0)
       {
       /* Invalid number, must be greater than 0 */
       Tcl_SetResult(interp, "ERROR : Invalid SAO number - must be > zero.",
		     TCL_VOLATILE);
       return(TCL_ERROR);
       }
     else
       {
       if ((saoindex > CMD_SAO_MAX) || 
	   (g_saoCmdHandle.saoHandle[ saoindex].state == SAO_UNUSED))
	 {
	 Tcl_SetResult(interp, "ERROR : No fSAO associated with SAO number.",
		       TCL_VOLATILE);
	 return(TCL_ERROR);
	 }
       }
     }

   if (shSaoPan(saoindex, c, r) != SH_SUCCESS)
     {
     shTclInterpAppendWithErrStack(interp);
     retStat = TCL_ERROR;
     }
   else
     {
     retStat = TCL_OK;
     }
   }

return(retStat);

} /* tclSaoPan */

/****************************************************************************
**
** ROUTINE: tclSaoZoom
**
**<AUTO EXTRACT>
** TCL VERB: saoZoom
**
**</AUTO>
*/
char g_saoZoom[] = "saoZoom";
ftclArgvInfo g_saoZoomTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Zoom the specified image by the specified amount.\n\n" },
  {"[magnification]", FTCL_ARGV_DOUBLE, NULL, NULL,
   "Zoom factor (.25, .5, 1., 2., 4. or a multiple of 2)."},
  {"-reset", FTCL_ARGV_CONSTANT, (void *)1, NULL,
   "Reset the Zoom to 1 (same as fsao zoom 1 button)"},
  {"-s", FTCL_ARGV_INT, NULL, NULL,
   "FSAOimage display number"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static RET_CODE tclSaoZoom
  (
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[]
  )   
{ /* tclSaoZoom */

int    retStat, reset = 0;
int    saoindex = SAOINDEXINITVAL;
double mag = 2.0;

/*---------------------------------------------------------------------------
**
** CHECK COMMAND ARGUMENTS
*/

g_saoZoomTbl[1].dst = &mag;
g_saoZoomTbl[2].dst = &reset;
g_saoZoomTbl[3].dst = &saoindex;

if ((retStat = shTclParseArgv(interp, &argc, argv, g_saoZoomTbl,
                              FTCL_ARGV_NO_LEFTOVERS, g_saoZoom))
    == FTCL_ARGV_SUCCESS)
   {
   if (saoindex != SAOINDEXINITVAL) {
     --saoindex;    /* Index starts at 0 not 1 */
     if (saoindex < 0)
       {
	 /* Invalid number, must be greater than 0 */
	 Tcl_SetResult(interp, "Invalid SAO number - must be > zero.",
		       TCL_VOLATILE);
	 return(TCL_ERROR);
       }
     else
       {
	 if ((saoindex > CMD_SAO_MAX) || 
	     (g_saoCmdHandle.saoHandle[saoindex].state == SAO_UNUSED))
	   {
	     Tcl_SetResult(interp, "No fSAO associated with SAO number.",
			   TCL_VOLATILE);
	     return(TCL_ERROR);
	   }
       }
     }
   
   if (shSaoZoom(saoindex, mag, reset) != SH_SUCCESS)
     {
     shTclInterpAppendWithErrStack(interp);
     retStat = TCL_ERROR;
     }
   else
     {
     retStat = TCL_OK;
     }
   }

return(retStat);
} /* tclSaoZoom */


/*
 * ROUTINE:
 *   shTclSaoPanDeclare
 *
 * CALL:
 *   (void) shTclSaoPanDeclare(
 *             Tcl_Interp *interp
 *             CMD_HANDLE *cmdHandlePtr
 *          );
 *
 * DESCRIPTION:
 *   This routine makes availaible to rest of the world all of SAO's
 *   panning verbs
 *
 * RETURN VALUE:
 *   Nothing
 */
void shTclSaoPanDeclare(Tcl_Interp *interp)
{

   char *helpFacil = "shFsao";
   int saoflags = FTCL_ARGV_NO_LEFTOVERS;


   shTclDeclare(interp, g_saoCenter, (Tcl_CmdProc *) tclSaoCenter,
               (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
               shTclGetArgInfo(interp, g_saoCenterTbl, saoflags, g_saoCenter),
               shTclGetUsage(interp, g_saoCenterTbl, saoflags, g_saoCenter));

   shTclDeclare(interp, g_saoPan, (Tcl_CmdProc *) tclSaoPan,
               (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                shTclGetArgInfo(interp, g_saoPanTbl, saoflags, g_saoPan),
                shTclGetUsage(interp, g_saoPanTbl, saoflags, g_saoPan));

   shTclDeclare(interp, g_saoZoom, (Tcl_CmdProc *) tclSaoZoom,
               (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                shTclGetArgInfo(interp, g_saoZoomTbl, saoflags, g_saoZoom),
                shTclGetUsage(interp, g_saoZoomTbl, saoflags, g_saoZoom));
   return;
}
