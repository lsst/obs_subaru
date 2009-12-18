#include <stdlib.h>
#include <stdio.h>
#include "dervish.h"


void atTclSmoothRegDeclare(Tcl_Interp *interp);
void atTclDustGetvalDeclare(Tcl_Interp *interp);
void atTclQuasarDeclare(Tcl_Interp *interp);
void atTclSlalibDeclare(Tcl_Interp *interp);
void atTclLinearFitsDeclare(Tcl_Interp *interp);
void atTclMatchDeclare(Tcl_Interp *interp);
void atTclTransDeclare(Tcl_Interp *interp);
void atTclCaliDeclare(Tcl_Interp *interp);
void atTclHgMathDeclare(Tcl_Interp *interp);
void atTclSurveyGeometryDeclare(Tcl_Interp *interp);
void atTclExternalDeclare(Tcl_Interp *interp);
void atTclAirmassDeclare(Tcl_Interp *interp);
void atTclImSimDeclare(Tcl_Interp *interp);
void atTclGalaxyDeclare(Tcl_Interp *interp);
void atTclEphemerisDeclare(Tcl_Interp *interp);
void atTclRegSciDeclare(Tcl_Interp *interp);
void atTclPMatchDeclare(Tcl_Interp *interp);
void atTclObjectDeclare(Tcl_Interp *interp);
void atTclMatrixDeclare(Tcl_Interp *interp);
void tcl_aperture_declare(Tcl_Interp *interp);
void atSchemaLoadFromAstrotools(void);


void atVerbsDeclare(Tcl_Interp *interp) {

	char *script;
	char cmd[150];

/* --- Place your module declarations here --- */
  atTclSmoothRegDeclare(interp);
  atTclDustGetvalDeclare(interp);
  atTclQuasarDeclare(interp);
  atTclSlalibDeclare(interp);
  atTclLinearFitsDeclare(interp);  
  atTclMatchDeclare(interp); 
  atTclTransDeclare(interp);
  atTclCaliDeclare(interp);
  atTclHgMathDeclare(interp);
  atTclSurveyGeometryDeclare(interp);
  atTclExternalDeclare(interp);
  atTclAirmassDeclare(interp);
  atTclImSimDeclare(interp);
  atTclGalaxyDeclare(interp);
  atTclEphemerisDeclare(interp);
  atTclRegSciDeclare(interp);
  atTclPMatchDeclare(interp);
  atTclObjectDeclare(interp);
  atTclMatrixDeclare(interp);
  tcl_aperture_declare(interp);
/* loads schema from astrotools */
  (void)atSchemaLoadFromAstrotools();

 script = (char *)getenv("ASTROTOOLS_STARTUP");
  if (script != NULL) {
    sprintf(cmd, "source %s", script);
    printf("Executing commands in %s\n", script);
    Tcl_Eval(interp, cmd);
    printf("%s\n", interp->result);
  }


}


