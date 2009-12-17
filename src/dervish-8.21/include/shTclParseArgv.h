#ifndef _SHTCLPARSEARGV_H
#define _SHTCLPARSEARGV_H

/*****************************************************************************
******************************************************************************
**
** FILE:
**      shCHash.h
**
** ABSTRACT:
**      This file contains all necessary definitions, macros, etc., for
**      the routines that handle hash tables
**
** ENVIRONMENT:
**      ANSI C.
**
** AUTHOR:
**      Creation date: Jan 20, 1995
**
******************************************************************************
******************************************************************************
*/

#include "ftcl.h"

/*----------------------------------------------------------------------------
**
** FUNCTION PROTOTYPES
*/
#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

char       *shTclGetArgInfo(Tcl_Interp *a_interp, ftclArgvInfo *a_argTable,
			    int a_flags, char *a_cmdName);
char       *shTclGetUsage(Tcl_Interp *a_interp, ftclArgvInfo *a_argTable,
			    int a_flags, char *a_cmdName);
char       *shTclGetHelp(Tcl_Interp *a_interp, ftclArgvInfo *a_argTable,
			    int a_flags, char *a_cmdName);
int        shTclParseArgv(Tcl_Interp *a_interp, int *a_argc, char *a_argv[],
			  ftclArgvInfo *a_argTable, int a_flags,
			  char *a_cmdName);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif








