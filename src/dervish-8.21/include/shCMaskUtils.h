#ifndef SHCMASKUTILS_H
#define SHCMASKUTILS_H
/*****************************************************************************
******************************************************************************
**
** FILE:
**	shCMaskUtils.h
**
** ABSTRACT:
**	This file contains all necessary definitions, macros, etc., for
**	the routines in maskUtils.c
**
** ENVIRONMENT:
**	ANSI C.
**
** AUTHOR:
**	Creation date: Dec. 26, 1996
** 	Don Petravick
**      Eileen Berman
**
******************************************************************************
******************************************************************************
*/

#include "region.h"

/*----------------------------------------------------------------------------
**
** DEFINITIONS
*/



/*----------------------------------------------------------------------------
**
** Prototypes
*/

#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

/* these are not yet implemented*/

void shMaskCopy (const MASK *maskIn, MASK *maskOut);

void shMaskClear(MASK *mask);

void shMaskRowFlip(MASK *mask);

void shMaskColFlip(MASK *mask);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif
