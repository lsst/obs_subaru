/*
 * Calls to declare contributed procedures
 */
#include "dervish.h"
#include "template.h"
#include "shCHg.h"
#include "shCLink.h"
#include "shCArray.h"

int shTclRegXDeclare(Tcl_Interp * interp);
void shTclGenericDeclare(Tcl_Interp * interp);
void shTclSchemaDefineDeclare(Tcl_Interp * interp);
void shTclChainOpsDeclare(Tcl_Interp * interp);
void shTclBinReadDeclare(Tcl_Interp * interp);
void shTclProgramCreateDeclare (Tcl_Interp *interp);
void shSchemaLoadFromContrib(void);

void
shTclContribDeclare(Tcl_Interp *interp)
{
   shTclTemplateDeclare(interp);
   shTclCursesDeclare(interp);
   shTclRegXDeclare(interp);
   shTclGenericDeclare(interp);
   shTclSchemaDefineDeclare(interp);
   shTclChainOpsDeclare(interp);
   shTclBinReadDeclare(interp);
   shTclProgramCreateDeclare (interp);
   /* shSchemaLoadFromContrib(); already called in shTclProgramCreateDeclare,
    so I comment out this call to it.  (Chris S.) */
}



