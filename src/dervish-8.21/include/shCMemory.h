#ifndef _SHCMEMORY_H
#define _SHCMEMORY_H

/*****************************************************************************
******************************************************************************
**
** FILE:
**      shCMemory.h
**
** ABSTRACT:
**      This file contains all necessary definitions, macros, etc., for
**      the routines involving the memory management functions.
**
** ENVIRONMENT:
**      ANSI C.
**
** AUTHOR:
**      Creation date: Mar 17, 1994
**
******************************************************************************
******************************************************************************
*/


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

void p_shMemTreeApply(void (*func)(const IPTR key, const IPTR val, void *data));

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif










