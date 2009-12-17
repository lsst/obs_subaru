#if !defined(TEMPLATE_H)
#define TEMPLATE_H
/*
 * Definitions for template contribution.
 *
 * This one declares our TCL verbs to dervish
 */
void shTclTemplateDeclare(Tcl_Interp *interp);
void shTclCursesDeclare(Tcl_Interp *interp);

/*
 * Please note, oh best beloved, that you'll get into trouble with this
 * function if a prototype is not in scope
 */
float shFloatAdd(float x1, float x2);

#endif
