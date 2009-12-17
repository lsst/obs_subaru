#ifndef _SHCMASK_H
#define _SHCMASK_H

/*****************************************************************************
******************************************************************************
**
** FILE:
**      shCMask.h
**
** ABSTRACT:
**      This file contains all necessary definitions, macros, etc., for
**      the routines that deal with masks. (Except those in region.h)
**
** ENVIRONMENT:
**      ANSI C.
**
** AUTHOR:
**      Creation date: Mar 10, 1994
**
******************************************************************************
******************************************************************************
*/

#include "dervish_msg_c.h"
#include "shCFitsIo.h"
#include "region.h"


/*
 * Mask header contains (for now at least) : SIMPLE, BITPIX, NAXIS, NAXIS1,
 * NAXIS2, END 
 */
#define MASK_HDR_SIZE   7

/*----------------------------------------------------------------------------
**
** FUNCTION PROTOTYPES
*/
#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

RET_CODE shMaskReadAsFits(MASK *a_maskPtr, char *a_file,
         DEFDIRENUM a_dirDefault, FILE *pipe, int a_readtape);
RET_CODE shMaskWriteAsFits(MASK *a_maskPtr, char *a_file,
         DEFDIRENUM a_dirDefault,  FILE *pipe, int a_writetape);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif
