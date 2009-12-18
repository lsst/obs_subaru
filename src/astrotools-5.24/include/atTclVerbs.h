#ifndef ATTCLVERBS_H
#define ATTCLVERBS_H

void atVerbsDeclare (Tcl_Interp *interp);      /* prototype for verbs */
void atTclDervishDeclare(Tcl_Interp *interp);
void atTclHgMathDeclare(Tcl_Interp *a_interp);
void atTclLinearFitsDeclare(Tcl_Interp *a_interp);
void atTclMatchDeclare(Tcl_Interp *a_interp);
void atTclTransDeclare(Tcl_Interp *a_interp);
void atTclEphemerisDeclare(Tcl_Interp *a_interp);
void atTclRegSciDeclare(Tcl_Interp *a_interp);
void atTclPMatchDeclare(Tcl_Interp *a_interp);

#endif

