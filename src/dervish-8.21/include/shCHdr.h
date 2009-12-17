#ifndef SHCHDR_H
#define SHCHDR_H

/*****************************************************************************
******************************************************************************
**
** FILE:
**      shCHdr.h
**
** ABSTRACT:
**      This file contains all necessary definitions, macros, etc., for
**      the "Pure C" routines to manipulate the FITS headers
**
** ENVIRONMENT:
**      ANSI C.
**
** AUTHOR:
**      Creation date: May 07, 1993
**        Vijay K. Gurbani
**
** MODIFCATION HISTORY
** 05/13/1993  VKG    Initial creation
** 05/14/1992  VKG    Changed function names to correspond with convention
** 07/06/1993  EFB    Added prototypes for shRegHdrGetLineTotal, shRegHdrGet
** 01/17/1994  THN    Converted from REGION-specific to generic header code.
******************************************************************************
******************************************************************************
*/

#include "dervish_msg_c.h"

/*----------------------------------------------------------------------------
**
** DEFINITIONS
*
 * Define object schemas.
 *
 *    NOTE:	The HDR_VEC typedef prevents the user from accessing the
 *		header from the Tcl level through the object schema package.
 *		The problem is that make_io.c does not use the typedef decla-
 *		ration, and therefore the pointer to the header vector in
 *		HDR (HDR_VEC *hdrVec) is seen a pointer to an UNKNOWN.
 */

typedef char	*HDR_VEC;		/* This is necessary as the POOR par- */
					/* sing in do_struct () in make_io.c  */
					/* does not let the FITS header vector*/
					/* (char *hdrVec[(MAXHDRLINE)+1]) be  */
					/* directly used.  You can get around */
					/* parsing, but then you run into run-*/
					/* time problems unless you hardwire  */
					/* the array size as an explicit num- */
					/* ber!!! Good code!                  */

typedef struct	 hdr
   {
   unsigned  int	  modCnt;	/* Modification count                 */
   HDR_VEC		 *hdrVec;	/* Header vector                      */
   }		 HDR;			/*  pragma USER */

/*----------------------------------------------------------------------------
**
 * Function prototypes
 */
#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

HDR *shHdrNew(void); 

void shHdrDel(HDR *a_hdr);
 
RET_CODE shHdrGetAscii(const HDR *a_hdr, 
                       const char *a_keyword, 
                       char  *a_dest);

RET_CODE shHdrGetDbl(const  HDR *a_hdr, 
                     const  char *a_keyword, 
                     double *a_dest);

RET_CODE shHdrGetInt(const HDR *a_hdr,
                     const char *a_keyword,
                     int   *a_dest);

void   p_shHdrPrint(const HDR *a_hdr);

RET_CODE shHdrGetLineTotal(const HDR *a_hdr,
                           int *a_lineTotal);

RET_CODE shHdrGetLogical(const HDR *a_hdr,
                         const char *a_keyword, 
                         int   *a_dest);

RET_CODE shHdrGetLine(const HDR *a_hdr,
                      const char *a_keyword,
                      char  *a_dest);

RET_CODE shHdrGetLineCont(const HDR *a_hdr,
                          const int a_lineno,
                          char  *a_dest);

RET_CODE shHdrGetLineno(const HDR *a_hdr,
                        const char *a_keyword,
                        int   *a_dest);

RET_CODE shHdrDelByLine(HDR *a_hdr,
                        const int a_lineNo);

RET_CODE shHdrInsertLine(HDR *a_hdr,
                         const int  a_lineNo,
                         const char *a_line);

RET_CODE shHdrReplaceLine(HDR *a_hdr, 
                          const int a_lineNo,
                          const char *a_line);

RET_CODE shHdrInsertLogical(HDR *a_hdr, 
                            const  char *a_key,
                            const  int  a_value,
                            const  char *a_comment);

RET_CODE shHdrInsertDbl(HDR *a_hdr,
                        const char   *a_key,
                        const double a_value,
                        const char   *a_comment);

RET_CODE shHdrInsertInt(HDR *a_hdr,
                        const  char *a_key,
                        const  int  a_value,
                        const  char *a_comment);

RET_CODE shHdrInsertAscii(HDR *a_hdr,
                          const  char *a_key,
                          const  char *a_value,
                          const  char *a_comment);

RET_CODE shHdrCopy(const HDR *a_inHdr, HDR *a_outHdr);

RET_CODE shHdrDelByKeyword(HDR *a_hdr, const char *a_keyword);

void shHdrMakeLineWithAscii(const char *a_keyword,
                            const char *a_value,
                            const char *a_comment,
                            char  *a_dest);

void shHdrMakeLineWithDbl(const char   *a_keyword,
                          const double a_value,
                          const char   *a_comment,
                          char  *a_dest);

void shHdrMakeLineWithInt(const char *a_keyword,
                          const int  a_value,
                          const char *a_comment,
                          char  *a_dest);

void shHdrMakeLineWithLogical(const char *a_keyword,
                              const int  a_value,
                              const char *a_comment,
                              char  *a_dest);
/*
 * The following functions are not publicly documented since they deal directly
 * with the HDR structure. They exist so that the developers can use them,
 * but other users of Dervish should not know that they exist at all.
 */

void p_shHdrFree(HDR *a_hdr);

void p_shHdrMallocForVec(HDR *a_hdr);

void p_shHdrFreeForVec(HDR *a_hdr);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif
