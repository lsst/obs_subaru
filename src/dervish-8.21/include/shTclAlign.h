#ifndef	SHTCLALIGN_H
#define	SHTCLALIGN_H

/*++
 * FACILITY:	Dervish
 *
 * ABSTRACT:	Definitions for using shAlign.
 *
 * ENVIRONMENT:	ANSI C.
 *		shTclAlign.h
 *
 * AUTHOR:	Tom Nicinski, Creation date: 12-Mar-1994
 *--
 ******************************************************************************/

#include	"ftcl.h"

#include	"shCSchema.h"
#include	"shCAlign.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************/
/*
 * API for alignment information.
 *
 * Public  Function		Explanation
 * ----------------------------	------------------------------------------------
 *
 * Private Function		Explanation
 * ----------------------------	------------------------------------------------
 * p_shTclAlignPrimElemRet	Return a single primitive element.
 */

int		 p_shTclAlignPrimElemRet(Tcl_Interp *interp, SHALIGN_TYPE alignType, TYPE schemaType, long int alignStrMax,
					 unsigned char *elemPtr, char *prefix);

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif	/* ifndef SHTCLALIGN_H */
