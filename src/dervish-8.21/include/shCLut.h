#ifndef _SHCLUT_H
#define _SHCLUT_H

/*****************************************************************************
******************************************************************************
**
** FILE:
**      shCLut.h
**
** ABSTRACT:
**      This file contains all necessary definitions, macros, etc., for
**      the routines that work with lookup tables.
**
** ENVIRONMENT:
**      ANSI C.
**
** AUTHOR:
**      Creation date: Dec 16, 1993
**
******************************************************************************
******************************************************************************
*/

#include "dervish_msg_c.h"
#include "shTclRegSupport.h"

/*----------------------------------------------------------------------------
**
** DEFINITIONS
*/
/* 8 bit lookup table constants*/
#define LUT8TOP 256               /* Size */
#define LUT8MAX 255               /* maximum allowed value in the table */
#define LUT8MIN 0                 /* minimum allowed value in the table */

#define MAXTCLLUTNAMELEN MAXTCLREGNAMELEN

typedef enum
   {
   LUTUNKNOWN = 0,
   LUT8                          /* type8 bit lookup table */
   } LUTTYPE;
/*----------------------------------------------------------------------------
**
** STRUCTURE DEFINITIONS
*/
typedef struct Lut {
  char      *name;
  LUTTYPE   type;
  int       lutArray[LUT8TOP];
} LUT;

/*----------------------------------------------------------------------------
**
** FUNCTION PROTOTYPES
*/
#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

LUT *shLutNew(char *a_name, LUTTYPE a_type);
void shLutDel(LUT *a_lutPtr);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif










