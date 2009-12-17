#ifndef _SHCFITSIO_H
#define _SHCFITSIO_H

/*****************************************************************************
******************************************************************************
**
** FILE:
**      shCFitsIo.h
**
** ABSTRACT:
**      This file contains all necessary definitions, macros, etc., for
**      the routines that read/write the Fits file
**
** ENVIRONMENT:
**      ANSI C.
**
** AUTHOR:
**      Creation date: May 07, 1993
**
******************************************************************************
******************************************************************************
*/

#include "dervish_msg_c.h"
#include "region.h"

/*----------------------------------------------------------------------------
**
** DEFINITIONS
*/
#define MAXDDIRLEN      132     /* Maximum length of default directory names */
#define MAXDEXTLEN      10      /* Maximum length of default file extension */
#define HDRLINECHAR	80	/* Number of characters in a header line */
#define HDRLINESIZE     82      /* FITS header line length - This value */
                                /* MUST match what's used in libfits (as a */
				/* hardwired value at that). */
#define RECORDSIZE     2880     /* FITS record length - This value */
                                /* MUST match what's used in libfits (as a */
				/* hardwired value at that). */
#define MAXHDRLINE      (5000) /* Maximum number of lines in a header */
#define MAXPIXTEMPSIZE	5000	/* Size of temporary buffer used to transform
 				   pixels into. */

/*----------------------------------------------------------------------------
**
** STRUCTURE DEFINITIONS
**
** NOTE:  DEFDIRENUM and TCLDIRTYPES (in tclFitsIo.c)   M U S T   match in
**        quantity and order.  Only DEF_NUM_OF_ELEMS should not be in
**        TCLDIRTYPES.
*/
typedef enum
   {
   DEF_REGION_FILE = 0,
   DEF_POOL_GROUP,
   DEF_PLOT_FILE,
   DEF_MASK_FILE,
   DEF_NUM_OF_ELEMS
   } DEFDIRENUM;
#define DEF_TYPEMAX      DEF_NUM_OF_ELEMS
#define	DEF_NONE	 DEF_NUM_OF_ELEMS	/* Don't apply any default */
#define	DEF_DEFAULT	(DEF_NUM_OF_ELEMS + 1)	/* Determine default type  */
						/* later.                  */

typedef enum
   {
   STANDARD,
   NONSTANDARD,
   IMAGE
   } FITSFILETYPE;

/*----------------------------------------------------------------------------
**
** FUNCTION PROTOTYPES
*/
#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

RET_CODE shDirGet(DEFDIRENUM a_type, char *a_dir, char *a_ext);
RET_CODE shDirSet(DEFDIRENUM a_type, char *a_dir, char *a_ext, int a_slash);
RET_CODE shRegReadAsFits(REGION *a_regPtr, char *a_file, int a_checktype,
			 int a_keeptype, DEFDIRENUM a_FITSdef,
                         FILE *a_inPipePtr, int a_readtape, int naxis3_is_OK,
			 int hdu);
RET_CODE shHdrReadAsFits(HDR *a_hdrPtr, char *a_file, DEFDIRENUM a_FITSdef,
			 FILE *a_inPipePtr, int a_readtape, int hdu);
RET_CODE shFileSpecExpand (char *a_fileName, char **a_fileSpec, char **a_fileTran, DEFDIRENUM a_defType);
RET_CODE shRegWriteAsFits(REGION *a_regPtr, char *a_file,
			  FITSFILETYPE a_fitsType, int a_naxis,
			  DEFDIRENUM a_FITSdef, FILE *a_outPipePtr, int a_writetape);
RET_CODE shHdrWriteAsFits(HDR *a_hdrPtr, char *a_file,
			  FITSFILETYPE a_fitsType, int a_naxis,
			  DEFDIRENUM a_FITSdef, REGION *a_regPtr,
			  FILE *a_outPipePtr, int a_writetape);
RET_CODE p_shFitsAddToHdr(int a_naxis, int a_nr, int a_nc,
			  PIXDATATYPE a_filePixType, FITSFILETYPE a_fileType,
			  double *a_bzero, double *a_bscale, int *a_transform);
RET_CODE p_shFitsDelHdrLines(char **a_hdr, int a_simple);
RET_CODE p_shMaskTest(MASK *a_maskPtr);


#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif

