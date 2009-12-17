#ifndef	SHTCLFITS_H
#define	SHTCLFITS_H

/*++
 * FACILITY:	Dervish
 *
 * ABSTRACT:	Definitions for using FITS files.
 *
 * ENVIRONMENT:	ANSI C.
 *		shTclFits.h
 *
 * AUTHOR:	Tom Nicinski, Creation date: 26-Jan-1994
 *--
 ******************************************************************************/

#include	"ftcl.h"

#include	"shCFitsIo.h"		/* For DEFDIRENUM                     */

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************/
/*
 * API for FITS Tables.
 *
 * Public  Function		Explanation
 * ----------------------------	------------------------------------------------
 *
 * Private Function		Explanation
 * ----------------------------	------------------------------------------------
 * p_shTclDIRSETexpand		Expand a file spec based on -dirset option.
 */

int		 p_shTclDIRSETexpand	(char* dirset,Tcl_Interp *interp, DEFDIRENUM defType, DEFDIRENUM *expType,
					 char *inSpec, char **expSpec, char **fullSpec);

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif	/* ifndef SHTCLFITS_H */
