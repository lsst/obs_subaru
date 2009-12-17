#ifndef	SHCFITS_H
#define	SHCFITS_H

/*++
 * FACILITY:	Dervish
 *
 * ABSTRACT:	Definitions for using FITS files.
 *
 * ENVIRONMENT:	ANSI C.
 *		shCFits.h
 *
 * AUTHOR:	Tom Nicinski, Creation date: 30-Nov-1993
 *--
 ******************************************************************************/

#include	"libfits.h"		/* For FITSHDUTYPE                    */

#include	"dervish_msg_c.h"		/* For RET_CODE                       */
#include	"shTclHandle.h"		/* For HANDLE                         */
#include	"shCFitsIo.h"		/* For DEFDIRENUM, FITSFILEYTPE       */
#include	"shCTbl.h"		/* For TBLCOL                         */

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************/
/*
 * Define stuff that controls how FITS files are handled.
 *
 *   o	Do not change the values of shFits_swap enums.  They are hardwired for
 *	efficiency by the FITS readers (such a s_shFitsTBLCOLreadBin ()).
 */

typedef enum
   {
   SH_FITS_SWAP   = -1,		/*        Swap bytes read in                  */
   SH_FITS_NOSWAP = +1		/* Do not swap bytes read in                  */
   }		 SHFITSSWAP;

typedef enum
   {
   SH_FITS_PDU_NONE,		/* No PDU to be written                       */
   SH_FITS_PDU_MINIMAL		/* Write minimal PDU for extensions           */
   }		 SHFITSPDU;



/******************************************************************************/
/*
 * API for FITS Tables.
 *
 * Public  Function		Explanation
 * ----------------------------	------------------------------------------------
 *   shFitsTypeName		Return   HDU name from FITS HDU type.
 *   shFitsRead			Read  an HDU from a FITS file.
 *   shFitsBinOpen		Open a FITS file and position file marker.
 *   shFitsReadNext		Read  the next HDU from a FITS file.
 *   shFitsBinClose		Close a FITS file.
 *   shFitsWrite		Write an HDU to   a FITS file.
 *   shFitsTBLCOLcomply		TBLCOL Table complies with FITS Standard?
 *
 * Private Function		Explanation
 * ----------------------------	------------------------------------------------
 * p_shFitsHdrLocate		Locate FITS header based on selection criteria.
 * p_shFitsPostHdrLocate		Locate FITS header based on selection criteria.
 * p_shFitsHdrClean		Remove header lines to hide them from the user.
 * p_shFitsSeek			Change the FITS file position.
 * p_shFitsLRecMultiple		FITS file length is multiple of logical records?
 */

char		*  shFitsTypeName	(FITSHDUTYPE FITStype);
RET_CODE	   shFitsRead		(HANDLE *handle, char *FITSfile, DEFDIRENUM defType, FITSHDUTYPE FITStype, int hduWanted,
					 char *xtension, HDR **hdr, int typeCheck, int typeRetain);
RET_CODE	   shFitsReadNext	(HANDLE *handle, FILE **FITSfp);
RET_CODE	   shFitsBinOpen	(FILE **FITSfp, char *FITSfile, DEFDIRENUM defType, int hduWanted);
RET_CODE	   shFitsBinClose	(FILE *FITSfp);
RET_CODE	   shFitsWrite		(HANDLE *handle, char *FITSfile, DEFDIRENUM defType, FITSHDUTYPE FITStype,
					 SHFITSPDU pduWanted, int append, FITSFILETYPE FITSstd);
RET_CODE	   shFitsTBLCOLcomply	(TBLCOL *tblCol, FITSHDUTYPE hduType, FITSFILETYPE *FITSstd);

RET_CODE	 p_shFitsHdrLocate	(FILE *FITSfp, HDR_VEC *hdrVec, unsigned int hdrVecSize, int *hduFound,
					 FITSHDUTYPE *hduType,  unsigned long int *dataAreaSize, unsigned long int *rsaSize,
					 unsigned long int *heapOff, unsigned long int *heapSize, int hduWanted,
					 char *xtension);
RET_CODE	 p_shFitsPostHdrLocate	(FILE *FITSfp, HDR_VEC *hdrVec, unsigned int hdrVecSize, int *hduFound,
					 int hduWanted);
RET_CODE	 p_shFitsHdrClean	(HDR *hdr);
RET_CODE	 p_shFitsSeek		(FILE *FITSfp, long int posCur, long int posDest);
RET_CODE	 p_shFitsLRecMultiple	(FILE *FITSfp);

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif	/* ifndef SHFITS_H */
