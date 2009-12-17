#ifndef SHTCLHDR_H
#define SHTCLHDR_H
/*****************************************************************************
******************************************************************************
**
** FILE:
**      shTclHdr.h
**
** ABSTRACT:
**      This file contains all necessary definitions, macros, etc., for
**      the dervish TCL header support routines
**
** ENVIRONMENT:
**      ANSI C.
**
** AUTHOR:
**      Creation date: Jan 26, 1995
**
******************************************************************************
******************************************************************************
*/
#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

int shTclHdrAddrGetFromExpr(Tcl_Interp *a_interp,
			    char       *a_hdrExpr,
			    HDR     **a_hdr);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif
