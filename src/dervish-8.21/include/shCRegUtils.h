#ifndef SHCREGUTILS_H
#define SHCREGUTILS_H
/*****************************************************************************
******************************************************************************
**
** FILE:
**	shCUtils.h
**
** ABSTRACT:
**	This file contains all necessary definitions, macros, etc., for
**	the routines in shCUtils.h
**
** ENVIRONMENT:
**	ANSI C.
**
** AUTHOR:
**	Creation date: Aug. 17, 1992
** 	Don Petravick
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

int p_samesize2(const REGION *reg1, const REGION *reg2);

RET_CODE shRegTypeSwitch(REGION *a_regPtr, PIXDATATYPE a_type);

void shRegSetWithDbl(double constant, REGION *reg);

void shRegSub(const REGION *minuend, const REGION *subtrahend, REGION *result);

void shRegXor(const REGION *pix1, const REGION *pix2, REGION *result);

void shRegAddWithDbl(double constant, const REGION *regIn,  REGION *result);

void shRegAdd(const REGION *addend1, const REGION *addend2, REGION *result);

void shRegAddRow(const REGION *addend1, const REGION *addend2, REGION *ans);

void shRegAddCol(const REGION *addend1, const REGION *addend2, REGION *ans);

void shRegSubRow(const REGION *addend1, const REGION *addend2, REGION *ans);

void shRegSubCol(const REGION *addend1, const REGION *addend2, REGION *ans);

void shRegPixCopy(const REGION *regSource, REGION *regDest);

void shRegPixSetWithDbl (REGION *reg, int r, int c, double value);

double shRegPixGetAsDbl (const REGION *reg, int r, int c);

/*returns 0 if region do compare, else non-zero.*/
int  shRegComp (const REGION *reg1, const REGION *reg2, int *rowADiff, int *colADiff);

REGION* shRegFlip (REGION  *reg);

void shRegRowFlip (REGION  *reg);

void shRegColFlip (REGION *reg);

void shRegMultWithDbl (double constant, const REGION *regIn, REGION *regout);
 
void shRegMult (double scale ,const REGION *mult1, const REGION *mult2, REGION *result);

void shRegMultWithRow (double scale ,const REGION *mult1, const REGION *mult2, REGION *result);

void shRegMultWithCol (double scale ,const REGION *mult1, const REGION *mult2, REGION *result);

void shRegDiv (double scale, const REGION *dividend, const REGION *divisor, REGION *result);

void shRegDivByRow (double scale, const REGION *dividend, const REGION *divisor, REGION *result);

void shRegDivByCol (double scale, const REGION *dividend, const REGION *divisor, REGION *result);

void shRegDivZero (double divZero, double scale, const REGION *dividend, const REGION *divisor, REGION *result);


/* these are not yet implemented*/

void shMaskRowFlip(MASK *mask);

void shMaskColFlip(MASK *mask);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif
