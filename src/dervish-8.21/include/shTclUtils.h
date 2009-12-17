#ifndef SHTCLUTILS_H
#define SHTCLUTILS_H
/*****************************************************************************
******************************************************************************
**
** FILE:
**      shTclUtils.h
**
** ABSTRACT:
**      This file contains all necessary definitions, macros, etc., for
**      the dervish TCL utility routines 
**
** ENVIRONMENT:
**      ANSI C.
**
** AUTHOR:
**      Creation date: Nov 01, 1995
**
******************************************************************************
******************************************************************************
*/

/*----------------------------------------------------------------------------
**
** DEFINITIONS
*/
#include "ftcl.h"

/*----------------------------------------------------------------------------
**
** STRUCTURE DEFINITIONS
*/

/* 
** The user MUST ENSURE that help and prototype are static
*/

#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

int shTclDeclare (Tcl_Interp        *interp, 
                  char              *cmdName, 
                  Tcl_CmdProc       *cmdProc, 
                  ClientData        data,
	          Tcl_CmdDeleteProc *proc, 
                  char              *helpFacility, 
                  char              *helpText, 
                  char              *cmdPrototype);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif
