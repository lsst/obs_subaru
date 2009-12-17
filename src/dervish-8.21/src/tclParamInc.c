#include <stdio.h>
#include "tcl.h"
#include "shCErrStack.h"
#include "shTclErrStack.h"
#include "shTclUtils.h"

FILE *paramIncOpen( char *fileName, long int seekSet, long int *seekEOF);

#define MAXLINELENGTH 2048

/*
 * ROUTINE:
 *   shTclParamIncOpen
 *
 *<AUTO>
 * TCL VERB:
 *   paramIncOpen
 *
 * DESCRIPTION:
 *    This routine open a param file up to a given seek position
 *
 * SYNTAX:
 *   paramIncOpen <fileName> <seekPosition>
 *
 *</AUTO>
 */

static
int shTclParamIncOpen(
                      ClientData clientData,  /* nada for now */
                      Tcl_Interp *interp,     /* the tcl interpreter */
                      int argc,               /* number of arguments */
                      char **argv)            /* argv[0] = basename */
                                              /* argv[1] = fileName */
                                              /* argv[2] = initial seek position */
{
  long int seekSet;
  long int seekEOF;
  FILE *file;
  char emsg[MAXLINELENGTH];

  if (argc!=3)                          { sprintf(emsg,"ERROR: usage is %s fileName seekPosition\n",argv[0]); goto ERROR; }
  if (sscanf(argv[2],"%d",&seekSet)!=1) { sprintf(emsg,"ERROR: usage is %s fileName seekPosition\n",argv[0]); goto ERROR; }

  if ( (file = paramIncOpen(argv[1],seekSet,&seekEOF))==NULL) goto ERROR2;

  Tcl_EnterFile(interp, file, TCL_FILE_READABLE);

  sprintf(emsg,"{FILE %s} {BYTES %d}",interp->result,seekEOF);
  sprintf(interp->result,"%s",emsg);
  return TCL_OK;

 ERROR:
  shErrStackPush(emsg);
 ERROR2:
  shTclInterpAppendWithErrStack(interp);
  return TCL_ERROR;
}


/*============================================================================
**============================================================================
**
** ROUTINE: shTclParamIncDeclare
**
** DESCRIPTION:
**      Declare TCL verbs for this module.
**
** RETURN VALUES:
**      N/A
**
**============================================================================
*/
void shTclParamIncDeclare(Tcl_Interp *interp)
{
  shTclDeclare(interp, "paramIncOpen", (Tcl_CmdProc *)shTclParamIncOpen,
               (ClientData)0, (Tcl_CmdDeleteProc *)NULL,
               "shParam", "Open a param file incrementally", "paramIncOpen <fileName> <seekPosition>");
}
