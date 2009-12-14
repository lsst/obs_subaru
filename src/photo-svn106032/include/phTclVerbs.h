#ifndef PHTCLVERBS_H
#define PHTCLVERBS_H

#include "astrotools.h"

void phTclDervishDeclare(Tcl_Interp *interp);
void phTclVerbsDeclare(Tcl_Interp *interp);

/*
 * Prototypes for functions declaring TCL extensions should go directly
 * into phTclVerbs.c; it's the only file which includes this include
 * file and cares about them (the tclFoo.c files don't #include this file,
 * and probably don't need to; it'd make adding a new extension recompile
 * all the tclBar.c files, and there's really no need).
 */

#endif
