#ifndef	SHTCLARRAY_H
#define	SHTCLARRAY_H

/*++
 * FACILITY:	Dervish
 *
 * ABSTRACT:	Definitions for using ARRAYs.
 *
 * ENVIRONMENT:	ANSI C.
 *		shTclArray.h
 *
 * AUTHOR:	Tom Nicinski, Creation date: 18-Feb-1994
 *--
 ******************************************************************************/

#include	"ftcl.h"

#include	"shCArray.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************/
/*
 *   o	p_shTclListValRet options.
 */

typedef	enum	 shArray_trailOp	/* M U S T   be a boolean value       */
   {
   SH_ARRAY_TRAIL_IGNORE = 0,		/* Leave unspecified values alone     */
   SH_ARRAY_TRAIL_ZERO   = 1		/* Set unspecified trailing value to 0*/
   }		   ARRAY_TRAILOP;

typedef enum	 shArray_valNonNeg	/* M U S T   be a boolean value       */
   {
   SH_ARRAY_VAL_ALL      = 0,		/* Value can  be positive or negative */
   SH_ARRAY_VAL_NON_NEG   = 1		/* Value must be positive (>= 0)      */
   }		   ARRAY_VALNONNEG;

typedef	enum	 shArray_strTrunc	/* M U S T   be a boolean value       */
   {
   SH_ARRAY_STR_FULL     = 0,		/* Do not truncate strings            */
   SH_ARRAY_STR_TRUNC    = 1		/* Truncate strings if necessary      */
   }		   ARRAY_STRTRUNC;

/******************************************************************************/
/*
 * API for ARRAYs.
 *
 * Public  Function		Explanation
 * ----------------------------	------------------------------------------------
 *
 * Private Function		Explanation
 * ----------------------------	------------------------------------------------
 * p_shTclListValRet		Return an array of values.
 * p_shTclArrayPrimRet		Return an array of primitive types.
 */

int		 p_shTclListValRet	(Tcl_Interp *interp, TYPE valuesType, char *values, char *valuesEmpty,
					 int valCntMin, int valCntMax, ARRAY_TRAILOP trailZero, int *valCnt, void *val,
					 ARRAY_VALNONNEG valNonNeg, void *valMax, int valMaxIdxLo, int valMaxIdxHi,
					 char *valLabelName, char *valLabelNames, char *valLabel1st,
					 int strLen1stMax, int strLenMax, ARRAY_STRTRUNC strTrunc, int *strLenLast);
int		 p_shTclArrayPrimRet	(Tcl_Interp *interp, ARRAY *array, int level, unsigned char **elemPtr, long int *elemCnt,
					 unsigned int depth);

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif	/* ifndef SHTCLARRAY_H */
