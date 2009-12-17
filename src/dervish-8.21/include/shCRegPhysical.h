#ifndef SHCREGPHYSICAL_H
#define SHCREGPHYSICAL_H
/*****************************************************************************
******************************************************************************
**
** FILE:
**	shCRegPhysical.h
**
** ABSTRACT:
**	This file contains all necessary definitions, macros, etc., for
**	the routines in regPhysical.c
**
** ENVIRONMENT:
**	ANSI C.
**
** AUTHOR:
**	Creation date: May 25, 1993
**
******************************************************************************
******************************************************************************
*/
#include "region.h"

/*----------------------------------------------------------------------------
**
** Prototype
*/
#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

RET_CODE shPhysRegConfig
   (
   int  a_numregions,                /* IN : Number of regions to configure */
   char *(*a_pixelCall)(int, int),   /* IN: Routine called to get pixel data pointer */
   void (*a_pixelFree)(int, char *), /* IN: Routine called when done with pixel data */
   char *(*a_hdrCall)(int, int),     /* IN: Routine called to get header data pointer */
   void (*a_hdrFree)(int, char *),   /* IN: Routine called when done with header data */
   int  (*a_fillCall)(int, char *, char *)  /* IN: Routine called to fill
                                                   region with a frame */
   );

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif
