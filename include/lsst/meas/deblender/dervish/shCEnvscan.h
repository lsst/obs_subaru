#ifndef _SHCENVSCAN_H
#define _SHCENVSCAN_H

/*****************************************************************************
******************************************************************************
**
** FILE:
**      shCEnvscan.h
**
** ABSTRACT:
**      This file contains all necessary definitions, macros, etc., for
**      the routines shEnvscan and shEnvfree.
**
** ENVIRONMENT:
**      ANSI C.
**
** AUTHOR:
**      Creation date: May 19, 1993
**
******************************************************************************
******************************************************************************
*/

#include "dervish_msg_c.h"

/*----------------------------------------------------------------------------
**
** DEFINITIONS
*/

/*----------------------------------------------------------------------------
**
** STRUCTURE DEFINITIONS
*/

/*----------------------------------------------------------------------------
**
** FUNCTION PROTOTYPES
*/
#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

char *shEnvscan(char *path);
void shEnvfree(char *result);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif
