/*++
 * FACILITY:	Dervish
 *
 * ABSTRACT:	FITS routines to read and write FITS files.
 *
 *		NOTE:	This code is based on the NOST FITS Standard (NOST
 *			100-1.0, June 18, 1993, "Definition of the Flexible
 *			Image Transport System (FITS)".
 *
 *			Binary Table handling is based on the "Draft Proposal
 *			for Binary Table Extension" in Appendix A of the NOST
 *			FITS Standard.  The Binary Table Extension is not part
 *			of the FITS Standard.
 *
 *   Public
 *   --------------------------	------------------------------------------------
 *     shFitsTypeName		Return   HDU name from FITS HDU type.
 *     shFitsRead		Read  an HDU from a FITS file.
 *     shFitsReadNext		Read the next HDU from a FITS file.
 *     shFitsBinOpen		Open a FITS file and position to an HDU.
 *     shFitsBinClose		Close a FITS file.
 *     shFitsWrite		Write an HDU to   a FITS file.
 *     shFitsTBLCOLcomply	TBLCOL Table complies with FITS Standard?
 *
 *   Private
 *   --------------------------	------------------------------------------------
 *   p_shFitsHdrLocate		Locate FITS header based on selection criteria.
 *   p_shFitsPostHdrLocate	Locate FITS header based on selection criteria.
 *   p_shFitsHdrClean		Remove header lines to hide them from the user.
 *   p_shFitsSeek		Change the FITS file position.
 *   p_shFitsLRecMultiple	FITS file length is multiple of logical records?
 *
 *   s_shFitsPlatformCheck	Make sure machine and coding assumptions match.
 *   s_shFitsHDRwrite		Write HDR (header) object schema to FITS file.
 *   s_shFitsTBLCOLread		Read Table HDU into TBLCOL format.
 *   s_shFitsTBLCOLreadAsc	Read ASCII  Table data into TBLCOL format.
 *   s_shFitsTBLCOLreadAscInt	Read ASCII  Table data: integer value.
 *   s_shFitsTBLCOLreadAscReal	Read ASCII  Table data: read    value.
 *   s_shFitsTBLCOLreadBin	Read Binary Table data into TBLCOL format.
 *   s_shFitsTBLCOLwrite	Write TBLCOL object schema to Table HDU.
 *   s_shFitsTBLCOLwriteAsc	Write TBLCOL object schema to ASCII  Table HDU.
 *   s_shFitsTBLCOLwriteBin	Write TBLCOL object schema to Binary Table HDU.
 *   s_shFitsTBLCOLtdimGen	Generate a FITS TDIMn keyword value.
 *
 * ENVIRONMENT:	ANSI C.
 *		shFits.c
 *
 * AUTHOR: 	Tom Nicinski, Creation date: 18-Nov-1993
 *              Bruce Greenawalt, added s_shFitsReadNext, s_shFitsBinOpen,
 *                     s_shFitsBinClose, and p_shFitsPostHdrLocate: 13-May-1999
 *--
 ******************************************************************************/

/******************************************************************************/
/*						.----------------------------.
 * Outstanding Issues / Problems / Etc.		|  Last Update:  1-Mar-1994  |
 *						`----------------------------'
 *
 * This section lists out all the current problems with FITS processing and any
 * suggested fixes and enhancements.  Please keep this section   C U R R E N T.
 *
 *- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * ROUTINE:	s_shFitsTBLCOLreadXXX, p_shFitsTBLCOLwriteXXX
 *
 * PROBLEM:	The number of bytes to read from/write to the FITS HDU is based
 *		on shAlign information.  That, in turn, is based on sizeof ().
 *		Technically, this is   N O T   the safe.  The problem is that
 *		sizeof () is defined to reflect not only the size of the data,
 *		but also any padding before or after the data.
 *
 * SOLUTION:	In practice, none of the primitive data types have padding
 *		applied to them (structures do, but we're not reading in struc-
 *		tures except for TBLHEAPDSC, and that's handled specially to
 *		avoid problems).
 *
 *		If the sizeof () value and the byte count to read value are
 *		actually different, the code will require considerable work.
 *		The problem is that the code will now need to understand the
 *		underlying architecture.  sizeof () does not given any indica-
 *		tion as to where the padding is placed.  Therefore, the reader
 *		would need to understand that for the particular platform. This
 *		a much bigger problem than it appears to be.
 *
 *		The current protection is that s_shFitsTBLCOLread () performs
 *		shAsserts to guarantee size assumptions.
 *
 *- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * ROUTINE:	s_shFitsTBLCOLread
 *
 * PROBLEM:	NAXIS1 represents the number of storage units of BITPIX bits in
 *		size.  The code assumes this   A N D   that BITPIX's value is
 *		the size of an unsigned char (not necessarily 8 bits).
 *
 * SOLUTION:	Add code to check that BITPIX is 8 bits, or add code to handle
 *		an arbitrary BITPIX (this is more cumbersome, since the C file
 *		I/O interface works in byte units -- and unnecessary, since the
 *		FITS Standard specifies that BITPIX for ASCII and Binary Table
 *		extensions must be 8).
 *
 *- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * ROUTINE:	s_shFitsTBLCOLreadBin, s_shFitsTBLCOLwriteBin
 *
 * PROBLEM:	TBLHEAPDSC is assumed to be two members, a 4 byte integer and a
 *		pointer.  This is hardwired in the code.
 *
 * SOLUTION:	This is not a bad assumption, since it is based on the FITS
 *		Standard for Binary Tables.  As FITS is not allowed to change
 *		("once FITS, always FITS"), this assumption will be correct
 *		in perpetuity.
 *
 *- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * ROUTINE:	s_shFitsTBLCOLwriteAsc
 *
 * PROBLEM:	The TBLFLD.prvt.TFORM specifications are ignored when deter-
 *		mining field width, offset and data type (that's why it's in
 *		the private area).
 *
 * SOLUTION:	The problem with using TBLFLD.prvt.TFORM.ascFldFmt is that is
 *		may conflict with ARRAY.data.schemaType, requiring data conver-
 *		sion.  This is NOT how the FITS Table writers are supposed to
 *		behave.  Likewise, the specified field width may be inadequate
 *		for the data type and the data value going into the field.
 *
 *		The only benefits provided by allowing users to specifiy field
 *		width and offset are:
 *
 *		   o	Can save some space by not using maximal field widths.
 *
 *		   o	Can have overlapped fields.
 *
 *		The only benefit provided by allowing users to specify the field
 *		data type is:
 *
 *		   o	Finer tuning as to how floating point numbers are
 *			represented.
 *
 *		These benefits, namely offering greater flexibility, need to be
 *		weighed against the disadvantages of improperly formed fields.
 *		Using TBLFLD.TFORM requires that the user have a good working
 *		knowledge of FITS ASCII Table structure.
 *
 *- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 ******************************************************************************/

/******************************************************************************/
/*
 * Global declarations.
 */

#include	<alloca.h>
#include	<errno.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#include	"libfits.h"

#include	"dervish_msg_c.h"
#include	"shCAssert.h"
#include	"shCSchema.h"
#include	"shCErrStack.h"
#include	"shCUtils.h"
#include        "shCGarbage.h"
#include	"shCAlign.h"
#include	"shCArray.h"
#include	"shCEnvscan.h"		/* For shEnvfree ()                   */
#include	"shCFitsIo.h"		/* For DEFDIRENUM, shRegRead/Write,...*/
#include	"shCMask.h"		/* For            shMaskRead/Write    */
#include	"shCHdr.h"
#include	"region.h"		/* For REGION, MASK                   */
#include	"shCTbl.h"

#include	"shCFits.h"

/******************************************************************************/
/*
 * Very private routines.
 */

#define	shFitsTDIMsize ((shFitsHdrStrSize) - 2)	/* Max size of TDIMn value    */
						/* ... (- 2 for quote chars in*/
						/* ... FITS keyword value).   */

static void	 s_shFitsPlatformCheck	(void);
static RET_CODE	 s_shFitsHDRwrite	(HDR *hdr, char *FITSname, DEFDIRENUM FITSdef, FITSHDUTYPE FITStype, int append,
					 FITSFILETYPE FITSstd);
static RET_CODE	 s_shFitsTBLCOLread 	(TBLCOL *tblCol, FILE *FITSfp, FITSHDUTYPE hduType, SHFITSSWAP datumSwap,
					 unsigned long int dataAreaSize, unsigned long int rsaSize, unsigned long int heapOff,
					 unsigned long int heapSize);
static RET_CODE	 s_shFitsTBLCOLreadAsc	(TBLCOL *tblCol, FILE *FITSfp, int TFIELDS, int rowSize,
					 unsigned long int dataAreaSize, unsigned long int rsaSize);
static RET_CODE	 s_shFitsTBLCOLreadAscInt	(char *buf, int fldWidth, long int *outLong);
static RET_CODE	 s_shFitsTBLCOLreadAscReal	(char *buf, int fldWidth, int decPt, double *outDouble);
static RET_CODE	 s_shFitsTBLCOLreadBin	(TBLCOL *tblCol, FILE *FITSfp, int TFIELDS, int rowSize, SHFITSSWAP datumSwap,
					 unsigned long int dataAreaSize, unsigned long int rsaSize, unsigned long int heapOff,
					 unsigned long int heapSize);
static RET_CODE	 s_shFitsTBLCOLwrite	(TBLCOL *tblCol, FILE *FITSfp, FITSHDUTYPE hduType, SHFITSSWAP datumSwap,
					 FITSFILETYPE FITSstd);
static RET_CODE	 s_shFitsTBLCOLwriteAsc	(TBLCOL *tblCol, FILE *FITSfp);
static RET_CODE	 s_shFitsTBLCOLwriteBin	(TBLCOL *tblCol, FILE *FITSfp, SHFITSSWAP datumSwap, FITSFILETYPE FITSstd);
static RET_CODE	 s_shFitsTBLCOLtdimGen	(ARRAY *array, char TDIM[(shFitsTDIMsize) + 1]);

/******************************************************************************/

char		*  shFitsTypeName
   (
   FITSHDUTYPE		 FITStype	/* IN:    FITS HDU type               */
   )

/*++
 * ROUTINE:	   shFitsTypeName
 *
 *	Return the name of a FITS HDU type.
 *
 * RETURN VALUES:
 *
 *   o	Pointer to a static character string.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	The returned strings are kept in static storage.  Nothing prevents users
 *	from modifying these, causing "invalid" strings to be returned.
 *--
 ******************************************************************************/

   {	/*   shFitsTypeName */

   /*
    * Declarations.
    */

static            char	*lcl_hduUnknown  = "(unknown)";
static            char	*lcl_hduPrimary  = "Primary";
static            char	*lcl_hduGroups   = "Random Groups";
static            char	*lcl_hduTABLE    = "TABLE";
static            char	*lcl_hduBINTABLE = "BINTABLE";
static            char	*lcl_hduInvalid  = "(invalid)";

                  char	*lcl_FITStypeName;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   switch (FITStype)
      {
      case f_hduUnknown  : lcl_FITStypeName = lcl_hduUnknown;  break;
      case f_hduPrimary  : lcl_FITStypeName = lcl_hduPrimary;  break;
      case f_hduGroups   : lcl_FITStypeName = lcl_hduGroups;   break;
      case f_hduTABLE    : lcl_FITStypeName = lcl_hduTABLE;    break;
      case f_hduBINTABLE : lcl_FITStypeName = lcl_hduBINTABLE; break;
      default            : lcl_FITStypeName = lcl_hduInvalid;  break;
      }

   return (lcl_FITStypeName);

   }	/*   shFitsTypeName */

/******************************************************************************/

RET_CODE	   shFitsRead
   (
   HANDLE		 *handle,	/* IN:    Dervish handle to read in to  */
                  char	 *FITSname,	/* IN:    FITS file specification     */
   DEFDIRENUM		  FITSdef,	/* IN:    Which default to apply      */
   FITSHDUTYPE		  FITStype,	/* IN:    Type of HDU to read         */
                  int	  hduWanted,	/* IN:    HDU to read (0-indexed)     */
                  char	 *xtension,	/* IN:    XTENSION to read            */
   HDR			**hdr,		/* OUT:   Dervish header                */
                  int	  typeCheck,	/* IN:    Check data types   (REGION) */
                  int	  typeRetain	/* IN:    Keep  data types   (REGION) */
   )

/*++
 * ROUTINE:	   shFitsRead
 *
 *	Read a header and data unit (HDU) from the FITS file.  The HDU is
 *	selected with either hduWanted or xtension:
 *
 *	   hduWanted	If this is positive, the hduWanted'th HDU is read.
 *			xtension must be zero.
 *
 *	   xtension	If this is a string (even a zero-length string), the
 *			FITS extension matching this name is read.
 *
 *	If both hduWanted and xtension are not specified, the primary HDU is
 *	read (hduWanted is set to 0).
 *
 *	The type of FITS HDU to read can be specified.  If the type to be read
 *	is to be determined by default from either FITS file position and handle
 *	type, then FITStype should be set to f_hduUnknown.
 *
 *	The following object schema types are restricted to be read from the
 *	primary HDU:
 *
 *	   REGION
 *	   MASK
 *
 *	The FITS file specification, FITSname, can be modified with a default
 *	path and extension (FITSdef of DEF_NONE skips applying defaults, FITSdef
 *	of DEF_DEFAULT selects the default type based on the handle type).  The
 *	file specification has metacharacters and environment variables trans-
 *	lated, even if FITSdef is DEF_NONE.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Succcess.
 *			The specified HDU was read properly from the FITS file.
 *
 *   SH_CONFLICT_ARG	Failure.
 *			The hduWanted and xtension arguments were both used, or
 *			the FITStype and handle type did not match.
 *
 *   SH_NOT_SUPPORTED	Failure.
 *			The handle (object schema) type is not supported.
 *
 *   SH_HEADER_IS_NULL	Failure.
 *			The header vector (used by LIBFITS) could not be allo-
 *			cated.
 *
 *   SH_FITS_OPEN_ERR	Failure.
 *			The FITS file could not be opened.
 *
 *   SH_FITS_NOEXTEND	Failure.
 *			The FITS file does not contain the requested extension
 *			HDU.
 *
 *   SH_TYPE_MISMATCH	Failure.
 *			The HDU type and the FITStype are incompatible.
 *
 *   SH_INV_FITS_FILE	Failure.
 *			The FITS file structure was invalid.  This includes an
 *			invalid FITS header or data area that does not match
 *			what is expected (for example, the file ends before
 *			the data does).
 *
 *   SH_MALLOC_ERR	Failure.
 *			Space could not be allocated for the FITS file specifi-
 *			cation.
 *
 * SIDE EFFECTS:
 *
 *   o	The FITS file is closed and left positioned at the end of the data por-
 *	tion of the HDU.  If the FITS file is on tape, the tape position will
 *	be changed, even if the HDU could not be read (or found, etc.).
 *
 *   o	The handling of FITS headers depends upon the object schema type of the
 *	handle:
 *
 *	   TBLCOL	The header vector is assumed to have been allocated by
 *			the construction routine, shTblcolNew (). Therefore, it
 *			is used directly.
 *
 *	   REGION	REGIONs are read with shRegReadAsFits ().
 *
 *	   MASK		MASKs   are read with shMaskReadAsFits ().
 *
 *	   other	The header vector is allocted here (with malloc ()).
 *
 *   o	In case of an error, if the object schema provides its own header vector
 *	(such as TBLCOL), the original contents of that header vector may be
 *	destroyed or invalid.
 *
 *   o	Under TBLCOL reading, if TBLFLDs already exist in the TBLCOL, they are
 *	freed.  If users have pointers to these TBLFLDs, they will need to treat
 *	them as invalid.
 *
 *   o	If a passed header vector has header entries, they are freed.
 *
 *   o  The FITS file is closed using fclose instead of f_fclose because the
 *      routines f_fopen and f_fclose are not meant to be used as a pair.
 *
 * SYSTEM CONCERNS:
 *
 *   o	The Dervish error stack is assumed to be empty (or have appropriate prior
 *	messages related to this operation).
 *--
 ******************************************************************************/

   {	/*   shFitsRead */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status         = SH_SUCCESS;
   FILE			*lcl_FITSfp         = ((FILE    *)0);
                  char	*lcl_FITSspec       = ((char    *)0);
                  char	*lcl_FITSspecFull   = ((char    *)0);
                  char	*lcl_FITStypeName   = ((char    *)0);
                  char	*lcl_handleTypeName = ((char    *)0);
   HANDLE                lcl_handle;	        /* used if input handle is NULL */
   TBLCOL		*lcl_tblCol         = ((TBLCOL  *)0);
   HDR			*lcl_hdr            = ((HDR     *)0);
   HDR_VEC		*lcl_hdrVec         = ((HDR_VEC *)0);
   HDR_VEC		*lcl_hdrVecMalloc   = ((HDR_VEC *)0);	/*We malloc()?*/
                  int	 lcl_hdrIdx;
                  int	 lcl_hduFound;
   FITSHDUTYPE		 lcl_hduType;
                  char	*lcl_hduTypeName    = ((char    *)0);
   unsigned long  int	 lcl_dataAreaSize;	/* HDU Data Area size (w/pad) */
   unsigned long  int	 lcl_rsaSize;		/* Record Storage Area size   */
   unsigned long  int	 lcl_heapOff;		/* Heap offset from data area */
   unsigned long  int	 lcl_heapSize;		/* Heap size                  */
                  int    readTape = 0;          /* shRegReadAsFits tape read option */

   TYPE			 lcl_schemaTypeREGION = shTypeGetFromName ("REGION");
   TYPE			 lcl_schemaTypeMASK   = shTypeGetFromName ("MASK");
   TYPE			 lcl_schemaTypeTBLCOL = shTypeGetFromName ("TBLCOL");

   FILE                 *lcl_inPipePtr        = NULL;	/* Never using a pipe */


   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   s_shFitsPlatformCheck ();

   /*
    * Perform some syntax and argument checking.
    */

   lcl_FITStypeName   = shFitsTypeName    (FITStype);
   if(handle == NULL) {
      shAssert(*hdr != NULL);		/* we must read data or header */

      lcl_handleTypeName = "NULL";
      handle = &lcl_handle;
      handle->ptr = NULL;
      handle->type = lcl_schemaTypeTBLCOL;
   } else {
      lcl_handleTypeName = shNameGetFromType (handle->type);
   }

   if ((hduWanted >= 0) && (xtension != ((char *)0)))
      {
      lcl_status = SH_CONFLICT_ARG;
      shErrStackPush ("hduWanted and xtension parameter values conflict");
      goto rtn_return;
      }

   if ((hduWanted <  0) && (xtension == ((char *)0)))
      {
        hduWanted  = 0;			/* Default to primary HDU             */
      }

   if ((FITStype == f_hduPrimary) && (hduWanted != 0))
      {
      lcl_status = SH_CONFLICT_ARG;
      if (xtension != ((char *)0))
         {
         shErrStackPush ("FITS extensions (%s) cannot be read from the Primary HDU", xtension);
         }
      else
         {
         shErrStackPush ("hduWanted (non-zero) and FITStype (Primary HDU) parameter values conflict");
         }
      goto rtn_return;
      }

   /*
    * See if the handle type and requested FITS HDU type are compatible.
    */

   switch (FITStype)
      {
      case f_hduUnknown  :
      case f_hduPrimary  :
                           break;
      case f_hduTABLE    :
      case f_hduBINTABLE :
                           if (handle->type != lcl_schemaTypeTBLCOL)
                              {
                              lcl_status = SH_NOT_SUPPORTED;
                              shErrStackPush ("%s HDUs cannot be read into %s object schemas", lcl_FITStypeName,
                                               lcl_handleTypeName);
                              goto rtn_return;
                              }
                           break;
      default            :
                           lcl_status = SH_NOT_SUPPORTED;
                           shErrStackPush ("%s HDU is not supported", lcl_FITStypeName);
                           goto rtn_return;
                           break;
      }

   if (   (handle->type == lcl_schemaTypeREGION)
       || (handle->type == lcl_schemaTypeMASK)
      )
      {
      if (hduWanted > 0)
         {
         lcl_status = SH_CONFLICT_ARG;
         shErrStackPush ("%s object schemas can only be read from Primary HDU (not HDU %d)", lcl_handleTypeName, hduWanted);
         }
      if (xtension != ((char *)0))
         {
         lcl_status = SH_CONFLICT_ARG;
         shErrStackPush ("%s object schemas can only be read from Primary HDU (not %s extensions)", lcl_handleTypeName, xtension);
         }
      if ((FITStype != f_hduUnknown) && (FITStype != f_hduPrimary))
         {
         lcl_status = SH_CONFLICT_ARG;
         shErrStackPush ("%s object schemas can only be read from Primary HDU (not HDU %d)", lcl_handleTypeName, lcl_FITStypeName);
         }
      if (lcl_status != SH_SUCCESS)
         {
         goto rtn_return;
         }
      }


   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Perform handle-specific initial processing.
    *
    * The following handles read from the Primary HDU only.
    *
    *   REGIONs
    *   MASKs
    */

   if      (handle->type == lcl_schemaTypeREGION)
      {
      if ((lcl_status = shRegReadAsFits (((REGION *)handle->ptr), FITSname, typeCheck, typeRetain, FITSdef, lcl_inPipePtr, readTape, 0, 0))
                     != SH_SUCCESS)
         {
         shErrStackPush ("Error encountered in HDU 0 (Primary HDU)");
         }
      goto rtn_return;
      }
   else if (handle->type == lcl_schemaTypeMASK)
      {
      if ((lcl_status = shMaskReadAsFits (((MASK  *)handle->ptr), FITSname, FITSdef, 
					  lcl_inPipePtr, readTape)) != SH_SUCCESS)
         {
         shErrStackPush ("Error encountered in HDU 0 (Primary HDU)");
         }
      goto rtn_return;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Read in an HDU that needs to be located.
    *
    *   o   Read in an ASCII or Binary Table.
    */

   if (hduWanted == 0)
      {
      lcl_status = SH_NOT_SUPPORTED;
      shErrStackPush ("%s HDUs cannot be read from the primary HDU", (FITStype != f_hduUnknown) ? lcl_FITStypeName
                                                                                                : "TABLE or BINTABLE");
      goto rtn_return;
      }

   if (handle->type != lcl_schemaTypeTBLCOL)
      {
      /*
       * Any other object schemas are not supported.
       */

      lcl_status = SH_NOT_SUPPORTED;
      shErrStackPush ("Reading into general object schemas (%s) is not supported", lcl_handleTypeName);
      shErrStackPush ("Cannot process HDU %d in FITS file %s", hduWanted, lcl_FITSspec);
      goto rtn_return;
      }

   /* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */
   /*
    * From here on, TBLCOL object schema handling.
    *
    * Read an ASCII or Binary Table into a TBLCOL object schema.
    *
    *   o   Get the full FITS file specification.
    */

   if (FITSdef == DEF_DEFAULT)		/* We're to default the default       */
      {
       FITSdef  = DEF_REGION_FILE;
      }

   if ((lcl_status = shFileSpecExpand (FITSname, &lcl_FITSspec, &lcl_FITSspecFull, FITSdef)) != SH_SUCCESS)
      {
      shErrStackPush ("Error defaulting/translating FITS file specification \"%s\"", FITSname);
      goto rtn_return;
      }

   lcl_tblCol = ((TBLCOL *)handle->ptr);
   if(lcl_tblCol == NULL) {		/* we don't really want any data */
      lcl_hdr = *hdr;
   } else {
      lcl_hdr    = &(lcl_tblCol->hdr);
   }

   lcl_hdrVec =   lcl_hdr->hdrVec;

   if (xtension != ((char *)0)) if (   (strcmp ("BINTABLE", xtension) != 0)
                                    && (strcmp ("TABLE",    xtension) != 0)
                                   )
      {
      lcl_status = SH_NOT_SUPPORTED;
      shErrStackPush ("%s HDUs cannot be read into TBLCOL object schemas", xtension);
      goto rtn_return;
      }

   if (lcl_hdrVec == ((HDR_VEC *)0))
      {
      lcl_status = SH_INTERNAL_ERR;
      shErrStackPush ("Internal error (%s line %d): no preallocated header vector in TBLCOL", __FILE__, __LINE__);
      shFatal        ("Internal error (%s line %d): no preallocated header vector in TBLCOL", __FILE__, __LINE__ - 1);
      goto rtn_return;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Allocate space for a FITS header vector in case there is none.
    *
    *   o   At this point lcl_hdr will correctly point to a header.
    *
    *   o   At this point lcl_hdrVec points to an existing header vector or
    *       to zero (0), indicating that a header vector needs to be allocated.
    *
    *   o   This section (allocation of the FITS header vector) is not dependent
    *       upon the handle type or the HDU type.
    */

   if (lcl_hdrVec == ((HDR_VEC *)0))
      {
      if ((lcl_hdrVec = lcl_hdrVecMalloc = ((HDR_VEC *)shMalloc (((MAXHDRLINE) + 1) * sizeof (HDR_VEC)))) == ((HDR_VEC *)0))
         {
         lcl_status = SH_HEADER_IS_NULL;
         shErrStackPush ("Unable to allocate space for Dervish header vector");
         goto rtn_return;
         }
      for (lcl_hdrIdx = MAXHDRLINE; lcl_hdrIdx >= 0; lcl_hdrIdx--)
         {
           lcl_hdrVec[lcl_hdrIdx] = NULL;
         }
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Open the FITS file and locate the desired HDU.
    *
    *   o   This section (locating the desired HDU) is not dependent upon the
    *       handle type.
    */

   if ((lcl_FITSfp = f_fopen (lcl_FITSspecFull, "r")) == ((FILE *)0))
      {
      lcl_status = SH_FITS_OPEN_ERR;
      shErrStackPush ("Error opening FITS file %s", lcl_FITSspecFull);
      goto rtn_return;
      }

   if ((lcl_status = p_shFitsHdrLocate (lcl_FITSfp, lcl_hdrVec, (MAXHDRLINE), &lcl_hduFound, &lcl_hduType, &lcl_dataAreaSize,
                                       &lcl_rsaSize, &lcl_heapOff, &lcl_heapSize, hduWanted, xtension)) != SH_SUCCESS)
      {
      if (hduWanted >= 0)
         {
         shErrStackPush ("Error locating HDU %d in FITS file %s", hduWanted, lcl_FITSspec);
         }
      else
         {
         shErrStackPush ("Error locating %s extension HDU in FITS file %s", xtension, lcl_FITSspec);
         }
      goto rtn_return;
      }

   /*
    *   o   Check to see that the desired HDU type and what was located are
    *       compatible.
    */

   switch (lcl_hduType)
   {
   case f_hduPrimary  :
   case f_hduTABLE    :
   case f_hduBINTABLE :
                        break;
   case f_hduGroups   : lcl_status = SH_NOT_SUPPORTED; shErrStackPush ("Random Groups reading is not supported"); goto rtn_return;
                        break;
   default            : lcl_status = SH_NOT_SUPPORTED; shErrStackPush ("Unknown FITS HDU type cannot be read");   goto rtn_return;
                        break;
   }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Did we only want the header? If so, we're done
    */
   if(lcl_tblCol == NULL) {
      goto rtn_return;
   }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Read in the ASCII or Binary Table.
    *
    *   o   At this point, only an ASCII or Binary Table is read into a TBLCOL
    *       object schema (any other Table type or object schema has been
    *       handled earlier).
    *
    *   o   lcl_tblCol, lcl_hdr, and lcl_hdrVec all point to valid objects.
    */

   switch (lcl_hduType)
      {
      case f_hduTABLE    :
      case f_hduBINTABLE :
                           lcl_hduTypeName = shFitsTypeName (lcl_hduType);
                           break;
      default            :
                           lcl_status = SH_FITS_NOEXTEND;
                           if (hduWanted >= 0)
                              {
                              shErrStackPush ("FITS HDU %d is not a %s extension", lcl_hduFound,
                                              (FITStype != f_hduUnknown) ? lcl_FITStypeName : "TABLE or BINTABLE");
                              }
                           else
                              {
                              shErrStackPush ("FITS HDU %d is not a %s extension", lcl_hduFound, xtension);
                              }
                           goto rtn_return;
                           break;
      }

   if ((FITStype != f_hduUnknown) && (FITStype != lcl_hduType))
      {
      lcl_status = SH_TYPE_MISMATCH;
      shErrStackPush ("FITS HDU %d %s extension does not match requested FITStype %s", lcl_hduFound, lcl_hduTypeName,
                                                                                                     lcl_FITStypeName);
      goto rtn_return;
      }

   if ((lcl_status = s_shFitsTBLCOLread (lcl_tblCol, lcl_FITSfp, lcl_hduType,
                                         (shEndedness () == SH_BIG_ENDIAN) ? SH_FITS_NOSWAP : SH_FITS_SWAP,
                                         lcl_dataAreaSize, lcl_rsaSize, lcl_heapOff, lcl_heapSize)) != SH_SUCCESS)
      {
	shErrStackPush ("Error encountered in HDU %d (%s extension)", lcl_hduFound, lcl_hduTypeName);
	if (lcl_status == SH_INV_FITS_FILE) {
	  shErrStackPush ("Invalid FITS file %s", lcl_FITSspec);
	} else if (lcl_status == SH_MALLOC_ERR) {
	  shErrStackPush ("No space for TBLFLD field descriptors for file %s", lcl_FITSspec);
	}
       goto rtn_return;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    */

rtn_return : ;

   if (lcl_FITSfp       != ((FILE *)0)) { fclose    (lcl_FITSfp);       }
   if (lcl_FITSspec     != ((char *)0)) {    shFree (lcl_FITSspec);     }
   if (lcl_FITSspecFull != ((char *)0)) { shEnvfree (lcl_FITSspecFull); }

   if (lcl_status != SH_SUCCESS)
      {
      if (lcl_hdrVec != ((HDR_VEC *)0))
         {
         f_hdel (lcl_hdrVec);			/* Clean up any hdr lines     */
         if (lcl_hdrVecMalloc != ((HDR_VEC *)0))
            {
            shFree (lcl_hdrVec);		/* Free up space we alloced   */
                    lcl_hdrVec = ((HDR_VEC *)0);/* For return value           */
            }
         }
      }

   if (hdr != ((HDR **)0) && *hdr != lcl_hdr)
      {
      *hdr  = lcl_hdr;
      }

   return (lcl_status);

   }	/*   shFitsRead */

/******************************************************************************/

RET_CODE	   shFitsBinOpen
   (
    FILE		 **FITSfp,	/* OUT:   FITS file position pointer  */
                  char	 *FITSname,	/* IN:    FITS file specification     */
   DEFDIRENUM		  FITSdef,	/* IN:    Which default to apply      */
                  int	  hduWanted	/* IN:    HDU to read (0-indexed)     */
   )

/*++
 * ROUTINE:	   shFitsBinOpen
 *
 *      Open a FITS file for future reading.  Additionally position the file 
 *      marker to be at the end of the data section for the hduWanted'th HDU.
 *
 *	If hduWanted is not specified, the file marker is positioned at the end
 *      of the primary HDU, i.e. hduWanted is set to 0.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Succcess.
 *			The specified HDU was read properly from the FITS file.
 *
 *   SH_HEADER_IS_NULL	Failure.
 *			The header vector (used by LIBFITS) could not be allo-
 *			cated.
 *
 *   SH_FITS_OPEN_ERR	Failure.
 *			The FITS file could not be opened.
 *
 *   SH_MALLOC_ERR	Failure.
 *			Space could not be allocated for the FITS file specifi-
 *			cation.
 *
 * SIDE EFFECTS:
 *
 *   o	The FITS file is left positioned at the end of the data portion of the 
 *      HDU.  If the FITS file is on tape, the tape position will
 *	be changed, even if the HDU could not be found.
 *
 *
 *   o	In case of an error, if the object schema provides its own header vector
 *	(such as TBLCOL), the original contents of that header vector may be
 *	destroyed or invalid.
 *
 *   o	Under TBLCOL reading, if TBLFLDs already exist in the TBLCOL, they are
 *	freed.  If users have pointers to these TBLFLDs, they will need to treat
 *	them as invalid.
 *
 *   o	If a passed header vector has header entries, they are freed.
 *
 * SYSTEM CONCERNS:
 *
 *   o	The Dervish error stack is assumed to be empty (or have appropriate prior
 *	messages related to this operation).
 *--
 ******************************************************************************/

   {	/*   shFitsBinOpen */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status         = SH_SUCCESS;
   FILE			*lcl_FITSfp         = ((FILE    *)0);
                  char	*lcl_FITSspec       = ((char    *)0);
                  char	*lcl_FITSspecFull   = ((char    *)0);
   HDR_VEC		*lcl_hdrVec         = ((HDR_VEC *)0);
   HDR_VEC		*lcl_hdrVecMalloc   = ((HDR_VEC *)0);	/*We malloc()?*/
                  int	 lcl_hdrIdx;
                  int	 lcl_hduFound;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   s_shFitsPlatformCheck ();

   /*
    * Perform some syntax and argument checking.
    */

   if ( hduWanted <  0 ) {
     hduWanted  = 0;			/* Default to primary HDU             */
   }

   /* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */
   /*
    * From here on, TBLCOL object schema handling.
    *
    * Read an ASCII or Binary Table into a TBLCOL object schema.
    *
    *   o   Get the full FITS file specification.
    */

   if (FITSdef == DEF_DEFAULT)		/* We're to default the default       */
      {
       FITSdef  = DEF_REGION_FILE;
      }

   if ((lcl_status = shFileSpecExpand (FITSname, &lcl_FITSspec, &lcl_FITSspecFull, FITSdef)) != SH_SUCCESS)
      {
      shErrStackPush ("Error defaulting/translating FITS file specification \"%s\"", FITSname);
      goto rtn_return;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Allocate space for a FITS header vector in case there is none.
    *
    *   o   At this point lcl_hdrVec points to an existing header vector or
    *       to zero (0), indicating that a header vector needs to be allocated.
    *
    *   o   This section (allocation of the FITS header vector) is not dependent
    *       upon the handle type or the HDU type.
    */

      if ((lcl_hdrVec = lcl_hdrVecMalloc = ((HDR_VEC *)shMalloc (((MAXHDRLINE) + 1) * sizeof (HDR_VEC)))) == ((HDR_VEC *)0))
         {
         lcl_status = SH_HEADER_IS_NULL;
         shErrStackPush ("Unable to allocate space for Dervish header vector");
         goto rtn_return;
         }
      for (lcl_hdrIdx = MAXHDRLINE; lcl_hdrIdx >= 0; lcl_hdrIdx--)
         {
           lcl_hdrVec[lcl_hdrIdx] = NULL;
         }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Open the FITS file and locate the desired HDU.
    *
    *   o   This section (locating the desired HDU) is not dependent upon the
    *       handle type.
    */

   if ((lcl_FITSfp = f_fopen (lcl_FITSspecFull, "r")) == ((FILE *)0))
      {
      lcl_status = SH_FITS_OPEN_ERR;
      shErrStackPush ("Error opening FITS file %s", lcl_FITSspecFull);
      goto rtn_return;
      }

   if ((lcl_status = p_shFitsPostHdrLocate (lcl_FITSfp, lcl_hdrVec, (MAXHDRLINE), &lcl_hduFound, hduWanted)) != SH_SUCCESS)
     {
       shErrStackPush ("Error locating HDU %d in FITS file %s", hduWanted, lcl_FITSspec);
       goto rtn_return;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Delete the header we read in
    */

   f_hdel (lcl_hdrVec);		/* Delete hdr lines          */

   if (lcl_hdrVecMalloc != ((HDR_VEC *)0)) {
     shFree (lcl_hdrVec);	    /* Free up space we alloced   */
     lcl_hdrVec = ((HDR_VEC *)0);   /* For return value           */
   }

   goto rtn_return;


   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    */

rtn_return : ;

   *FITSfp = lcl_FITSfp;
   if (lcl_FITSspec     != ((char *)0)) {    shFree (lcl_FITSspec);     }
   if (lcl_FITSspecFull != ((char *)0)) { shEnvfree (lcl_FITSspecFull); }

   if (lcl_hdrVec != ((HDR_VEC *)0)) {
     f_hdel (lcl_hdrVec);			/* Clean up any hdr lines     */
     if (lcl_hdrVecMalloc != ((HDR_VEC *)0)) {
       shFree (lcl_hdrVec);		/* Free up space we alloced   */
       lcl_hdrVec = ((HDR_VEC *)0);/* For return value           */
     }
   }
   
   return (lcl_status);

   }	/*   shFitsBinOpen */

/******************************************************************************/

RET_CODE	   shFitsReadNext
   (
   HANDLE		 *handle,	/* IN:    Dervish handle to read in to  */
   FILE		         **FITSfp	/* IN:    FITS file position pointer  */
   )

/*++
 * ROUTINE:	   shFitsReadNext
 *
 *	Read a header and data unit (HDU) from the FITS file.  
 *
 *
 *	The FITS file specification, FITSname, can be modified with a default
 *	path and extension (FITSdef of DEF_NONE skips applying defaults, FITSdef
 *	of DEF_DEFAULT selects the default type based on the handle type).  The
 *	file specification has metacharacters and environment variables trans-
 *	lated, even if FITSdef is DEF_NONE.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Succcess.
 *			The specified HDU was read properly from the FITS file.
 *
 *   SH_NOT_SUPPORTED	Failure.
 *			The handle (object schema) type is not supported.
 *
 *   SH_HEADER_IS_NULL	Failure.
 *			The header vector (used by LIBFITS) could not be allo-
 *			cated.
 *
 *   SH_INV_FITS_FILE	Failure.
 *			The FITS file structure was invalid.  This includes an
 *			invalid FITS header or data area that does not match
 *			what is expected (for example, the file ends before
 *			the data does).
 *
 *   SH_MALLOC_ERR	Failure.
 *			Space could not be allocated for the FITS file specifi-
 *			cation.
 *
 * SIDE EFFECTS:
 *
 *   o	The FITS file is closed and left positioned at the end of the data por-
 *	tion of the HDU.  If the FITS file is on tape, the tape position will
 *	be changed, even if the HDU could not be read (or found, etc.).
 *
 *   o	The handling of FITS headers depends upon the object schema type of the
 *	handle:
 *
 *	   TBLCOL	The header vector is assumed to have been allocated by
 *			the construction routine, shTblcolNew (). Therefore, it
 *			is used directly.
 *
 *	   REGION	REGIONs are read with shRegReadAsFits ().
 *
 *	   MASK		MASKs   are read with shMaskReadAsFits ().
 *
 *	   other	The header vector is allocted here (with malloc ()).
 *
 *   o	In case of an error, if the object schema provides its own header vector
 *	(such as TBLCOL), the original contents of that header vector may be
 *	destroyed or invalid.
 *
 *   o	Under TBLCOL reading, if TBLFLDs already exist in the TBLCOL, they are
 *	freed.  If users have pointers to these TBLFLDs, they will need to treat
 *	them as invalid.
 *
 *   o	If a passed header vector has header entries, they are freed.
 *
 *   o  The FITS file is closed using fclose instead of f_fclose because the
 *      routines f_fopen and f_fclose are not meant to be used as a pair.
 *
 * SYSTEM CONCERNS:
 *
 *   o	The Dervish error stack is assumed to be empty (or have appropriate prior
 *	messages related to this operation).
 *--
 ******************************************************************************/

   {	/*   shFitsReadNext */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status         = SH_SUCCESS;
   FILE			*lcl_FITSfp         = ((FILE    *)0);
                  char	*lcl_handleTypeName = ((char    *)0);
   HANDLE                lcl_handle;	        /* used if input handle is NULL */
   TBLCOL		*lcl_tblCol         = ((TBLCOL  *)0);
   HDR			*lcl_hdr            = ((HDR     *)0);
   HDR_VEC		*lcl_hdrVec         = ((HDR_VEC *)0);
   FITSHDUTYPE		 lcl_hduType;
                  char	*lcl_hduTypeName    = ((char    *)0);
   unsigned long  int	 lcl_dataAreaSize;	/* HDU Data Area size (w/pad) */
   unsigned long  int	 lcl_rsaSize;		/* Record Storage Area size   */
   unsigned long  int	 lcl_heapOff;		/* Heap offset from data area */
   unsigned long  int	 lcl_heapSize;		/* Heap size                  */

   TYPE			 lcl_schemaTypeTBLCOL = shTypeGetFromName ("TBLCOL");

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   s_shFitsPlatformCheck ();

   /*
    * Perform some syntax and argument checking.
    */

   if(handle == NULL) {
      lcl_handleTypeName = "NULL";
      handle = &lcl_handle;
      handle->ptr = NULL;
      handle->type = lcl_schemaTypeTBLCOL;
   } else {
      lcl_handleTypeName = shNameGetFromType (handle->type);
   }


   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Read in an HDU that needs to be located.
    *
    *   o   Read in an ASCII or Binary Table.
    */

   if (handle->type != lcl_schemaTypeTBLCOL) {
     /*
      * Any other object schemas are not supported.
      */

     lcl_status = SH_NOT_SUPPORTED;
     shErrStackPush ("Reading into general object schemas (%s) is not supported", lcl_handleTypeName);
     goto rtn_return;
   }

   /* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */
   /*
    * From here on, TBLCOL object schema handling.
    *
    * Read an ASCII or Binary Table into a TBLCOL object schema.
    *
    *   o   Get the full FITS file specification.
    */

   lcl_tblCol = ((TBLCOL *)handle->ptr);
   lcl_hdr    = &(lcl_tblCol->hdr);
   
   lcl_hdrVec =   lcl_hdr->hdrVec;

   if (lcl_hdrVec == ((HDR_VEC *)0)) {
     lcl_status = SH_INTERNAL_ERR;
     shErrStackPush ("Internal error (%s line %d): no preallocated header vector in TBLCOL", __FILE__, __LINE__);
     shFatal        ("Internal error (%s line %d): no preallocated header vector in TBLCOL", __FILE__, __LINE__ - 1);
     goto rtn_return;
   }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Open the FITS file and locate the desired HDU.
    *
    *   o   This section (locating the desired HDU) is not dependent upon the
    *       handle type.
    */

   lcl_FITSfp = *FITSfp;

   if (!f_rdheadi (lcl_hdrVec, (MAXHDRLINE), lcl_FITSfp)) {
     lcl_status = SH_INV_FITS_FILE;
     shErrStackPush ("Error reading FITS header");
     goto rtn_return;
   }
   
   if (! f_hinfo (lcl_hdrVec, 1, &lcl_hduType, &lcl_dataAreaSize, &lcl_rsaSize, &lcl_heapOff, &lcl_heapSize)) {
     lcl_status = SH_INV_FITS_FILE;
     shErrStackPush ("Invalid FITS %s header", shFitsTypeName (lcl_hduType) );
     goto rtn_return;
   }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Read in the ASCII or Binary Table.
    *
    *   o   At this point, only an ASCII or Binary Table is read into a TBLCOL
    *       object schema (any other Table type or object schema has been
    *       handled earlier).
    *
    *   o   lcl_tblCol, lcl_hdr, and lcl_hdrVec all point to valid objects.
    */

   switch (lcl_hduType) {
   case f_hduTABLE    :
   case f_hduBINTABLE :
     lcl_hduTypeName = shFitsTypeName (lcl_hduType);
     break;
   default            :
     lcl_status = SH_FITS_NOEXTEND;
     shErrStackPush ("FITS HDU is not a TABLE or BINTABLE extension");
     goto rtn_return;
     break;
   }

   if ((lcl_status = s_shFitsTBLCOLread (lcl_tblCol, lcl_FITSfp, lcl_hduType,
                                         (shEndedness () == SH_BIG_ENDIAN) ? SH_FITS_NOSWAP : SH_FITS_SWAP,
                                         lcl_dataAreaSize, lcl_rsaSize, lcl_heapOff, lcl_heapSize)) != SH_SUCCESS)
     {
	shErrStackPush ("Error encountered in HDU (%s extension)", lcl_hduTypeName);
	if (lcl_status == SH_INV_FITS_FILE) {
	  shErrStackPush ("shFitsReadNext: Invalid FITS file");
	} else if (lcl_status == SH_MALLOC_ERR) {
	  shErrStackPush ("shFitsReadNext: No space for TBLFLD field descriptors");
	}
       goto rtn_return;
     }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    */

rtn_return : ;

   if (lcl_status != SH_SUCCESS) {
     if (lcl_hdrVec != ((HDR_VEC *)0)) {
       f_hdel (lcl_hdrVec);			/* Clean up any hdr lines     */
     }
   }

   return (lcl_status);

   }	/*   shFitsReadNext */


/******************************************************************************/

RET_CODE	   shFitsBinClose
   (
    FILE		 *FITSfp	/* IN:    Dervish handle to read in to  */
   )

/*++
 * ROUTINE:	   shFitsBinClose
 *
 *	Close a FITS file that was previously opened with shFitsBinOpen.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Succcess.
 *			The specified HDU was read properly from the FITS file.
 *
 *   SH_FITS_OPEN_ERR	Failure.
 *			The FITS file could not be opened.
 *
 *
 * SIDE EFFECTS:
 *
 *   o	The FITS file is closed and left positioned at the end of the data por-
 *	tion of the HDU.  If the FITS file is on tape, the tape position will
 *	be changed, even if the HDU could not be read (or found, etc.).
 *
 *   o	The handling of FITS headers depends upon the object schema type of the
 *	handle:
 *
 *	   TBLCOL	The header vector is assumed to have been allocated by
 *			the construction routine, shTblcolNew (). Therefore, it
 *			is used directly.
 *
 *	   REGION	REGIONs are read with shRegReadAsFits ().
 *
 *	   MASK		MASKs   are read with shMaskReadAsFits ().
 *
 *	   other	The header vector is allocted here (with malloc ()).
 *
 *   o	In case of an error, if the object schema provides its own header vector
 *	(such as TBLCOL), the original contents of that header vector may be
 *	destroyed or invalid.
 *
 *   o	Under TBLCOL reading, if TBLFLDs already exist in the TBLCOL, they are
 *	freed.  If users have pointers to these TBLFLDs, they will need to treat
 *	them as invalid.
 *
 *   o	If a passed header vector has header entries, they are freed.
 *
 *   o  The FITS file is closed using fclose instead of f_fclose because the
 *      routines f_fopen and f_fclose are not meant to be used as a pair.
 *
 * SYSTEM CONCERNS:
 *
 *   o	The Dervish error stack is assumed to be empty (or have appropriate prior
 *	messages related to this operation).
 *--
 ******************************************************************************/

   {	/*   shFitsBinClose */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status         = SH_SUCCESS;
   FILE			*lcl_FITSfp         = ((FILE    *)0);

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   s_shFitsPlatformCheck ();

   lcl_FITSfp = FITSfp;

   if (lcl_FITSfp       != ((FILE *)0)) { fclose    (lcl_FITSfp);       }

   return (lcl_status);

   }	/*   shFitsBinClose */

/******************************************************************************/

RET_CODE	   shFitsWrite
   (
   HANDLE		 *handle,	/* IN:    Dervish handle to write from  */
                  char	 *FITSname,	/* IN:    FITS file specification     */
   DEFDIRENUM		  FITSdef,	/* IN:    Which default to apply      */
   FITSHDUTYPE		  FITStype,	/* IN:    Type of HDU to write        */
   SHFITSPDU		  pduWanted,	/* IN:    Put out a PDU too?          */
                  int	  append,	/* IN:    Boolean: append to file     */
   FITSFILETYPE		  FITSstd	/* IN:    SIMPLE=?/chk FITS compliance*/
   )

/*++
 * ROUTINE:	   shFitsWrite
 *
 *	Write a header and data unit (HDU) to the FITS file.  An optional PDU
 *	(Primary Data Unit) can be written if the HDU is not being appended to
 *	the FITS file.
 *
 *	The type of FITS HDU to write can be specified.  If the type to be
 *	written is to be determined by default from the handle type, then
 *	FITStype should be set to f_hduUnknown.  Some handle types, such as
 *	TBLCOL, require the type of FITS HDU to be passed.
 *
 *	The following object schema types are restricted to be written to the
 *	primary HDU:
 *
 *	   HDR
 *	   REGION
 *	   MASK
 *
 *	The FITS file specification, FITSname, can be modified with a default
 *	path and extension (FITSdef of DEF_NONE skips applying defaults, FITSdef
 *	of DEF_DEFAULT selects the default type based on the handle type).  The
 *	file specification has metacharacters and environment variables trans-
 *	lated, even if FITSdef is DEF_NONE.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Succcess.
 *			The specified HDU was written properly to the FITS file.
 *
 *   SH_INVARG		Failure.
 *			Invalid arguments.  pduWanted and/or FITSstd are not
 *			valid.
 *
 *   SH_CONFLICT_ARG	Failure.
 *			The append and pduWanted arguments were both used, or
 *			the FITStype and handle type did not match.
 *
 *   SH_NOT_SUPPORTED	Failure.
 *			The handle (object schema) type is not supported.
 *
 *   SH_FITS_OPEN_ERR	Failure.
 *			The FITS file could not be opened.
 *
 *   SH_TYPE_MISMATCH	Failure.
 *			The HDU type and the FITStype are incompatible.
 *
 *   SH_INV_FITS_FILE	Failure.
 *			The FITS file structure was invalid.  This includes an
 *			invalid FITS header or data area that does not match
 *			what is expected (for example, the file length is not
 *			a multiple of the FITS record length).
 *
 *   SH_MALLOC_ERR	Failure.
 *			Space could not be allocated for the FITS file specifi-
 *			cation.
 *
 * SIDE EFFECTS:
 *
 *   o	The FITS file is closed and left positioned at the end of the data por-
 *	tion of the HDU.  If the FITS file is on tape, the tape position will
 *	be changed, even if the HDU could not be written.
 *
 *   o	The handling of FITS headers depends upon the object schema type of the
 *	handle:
 *
 *	   TBLCOL
 *
 *	   REGION	REGIONs are written with   shRegWriteAsFits  ().
 *
 *	   MASK		MASKs   are written with   shMaskWriteAsFits ().
 *
 *	   HDR		HDRs    are written with s_shFitsHDRwrite    ().
 *
 *	   other	Any other object schema is not handled by this routine.
 *
 *   o	In case of an error, the Dervish header may be left in a modified state.
 *
 *   o  The FITS file is closed using fclose instead of f_fclose because the
 *      routines f_fopen and f_fclose are not meant to be used as a pair.
 *
 * SYSTEM CONCERNS:
 *
 *   o	The Dervish error stack is assumed to be empty (or have appropriate prior
 *	messages related to this operation).
 *
 *   o	Any FITS compliance and object schema checking should be reflected in
 *	any other compliance testing routines.  For example, TBLCOL compliance
 *	is tested by shFitsTBLCOLcomply ().
 *--
 ******************************************************************************/

   {	/*   shFitsWrite */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status         = SH_SUCCESS;
   RET_CODE		 lcl_statusTmp;
   FILE			*lcl_FITSfp         = ((FILE   *)0);
            long  int	 lcl_FITSpos;
                  char	*lcl_FITSspec       = ((char   *)0);
                  char	*lcl_FITSspecFull   = ((char   *)0);
                  char	*lcl_FITStypeName   = ((char   *)0);
   FITSFILETYPE		 lcl_FITSstd;
                  char	*lcl_handleTypeName = ((char   *)0);
   TBLCOL		*lcl_tblCol         = ((TBLCOL *)0);
   HDR			 lcl_pduHdr;	/* Header used for minimal PDU        */
                  int	 lcl_pduHdrLineCnt;
                  int	 lcl_regNAXIS;	/* REGION-only: NAXIS value           */
                  int    tapeWrite = 0; /* shRegWriteAsFits tape write option */

   TYPE			 lcl_schemaTypeREGION = shTypeGetFromName ("REGION");
   TYPE			 lcl_schemaTypeMASK   = shTypeGetFromName ("MASK");
   TYPE			 lcl_schemaTypeTBLCOL = shTypeGetFromName ("TBLCOL");
   TYPE			 lcl_schemaTypeHDR    = shTypeGetFromName ("HDR");

   FILE                 *outPipePtr           = NULL;  /* Never using a pipe  */

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   memset (((void *)&lcl_pduHdr), 0, sizeof (HDR)); /* Clear for rtn_return   */

   s_shFitsPlatformCheck ();

   /*
    * Perform some syntax and argument checking.
    */

   if ((pduWanted != SH_FITS_PDU_NONE) && (pduWanted != SH_FITS_PDU_MINIMAL))
      {
      lcl_status = SH_INVARG;
      shErrStackPush ("pduWanted is invalid");
      goto rtn_return;
      }

   if (append && (pduWanted != SH_FITS_PDU_NONE))
      {
      lcl_status = SH_CONFLICT_ARG;
      shErrStackPush ("append and pduWanted parameter values conflict");
      goto rtn_return;
      }

   lcl_FITStypeName   = shFitsTypeName    (FITStype);
   lcl_handleTypeName = shNameGetFromType (handle->type);

   if ((FITStype == f_hduPrimary) && (pduWanted != SH_FITS_PDU_NONE))
      {
      lcl_status = SH_CONFLICT_ARG;
      shErrStackPush ("FITStype and pduWanted parameter values conflict");
      goto rtn_return;
      }

   /*
    * See if the handle type and requested FITS HDU type are compatible.
    */

   switch (FITStype)
      {
      case f_hduUnknown  :
      case f_hduPrimary  :
                           break;
      case f_hduTABLE    :
      case f_hduBINTABLE :
                           if (handle->type != lcl_schemaTypeTBLCOL)
                              {
                              lcl_status = SH_NOT_SUPPORTED;
                              shErrStackPush ("%s objects schemas cannot be written to %s HDUs", lcl_handleTypeName,
                                               lcl_FITStypeName);
                              goto rtn_return;
                              }
                           break;
      default            :
                           lcl_status = SH_NOT_SUPPORTED;
                           shErrStackPush ("Writing to %s HDUs is not supported", lcl_FITStypeName);
                           goto rtn_return;
                           break;
      }

   if (   (handle->type == lcl_schemaTypeREGION)
       || (handle->type == lcl_schemaTypeMASK)
       || (handle->type == lcl_schemaTypeHDR)
      )
      {
      switch (FITStype)
         {
         case f_hduUnknown :
         case f_hduPrimary :
                             break;
         default           :
                             lcl_status = SH_CONFLICT_ARG;
                             shErrStackPush ("%s object schema can only be written to Primary HDU (not %s)", lcl_handleTypeName,
                                             lcl_FITStypeName);
                             goto rtn_return;
                             break;
         }
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Perform handle-specific initial processing.
    *
    *   REGION	shRegWriteAsFits' naxis parameter is defaulted to 2.  The only
    *		other possible value is 0 when the REGION is empty.
    *
    *   MASK	shMaskWriteAsFits () is used.
    *
    *   HDR	s_shFitsHDRwrite () is used.
    */

        if (handle->type == lcl_schemaTypeREGION)
      {
      lcl_regNAXIS = ((((REGION *)handle->ptr)->nrow == 0) && (((REGION *)handle->ptr)->ncol == 0)) ? 0 : 2;
      if ((lcl_status = shRegWriteAsFits (((REGION *)handle->ptr), FITSname, FITSstd, lcl_regNAXIS, FITSdef, outPipePtr, tapeWrite)) != SH_SUCCESS)
         {
         shErrStackPush ("Error encountered writing HDU 0 (Primary HDU)");
         }
      goto rtn_return;
      }
   else if (handle->type == lcl_schemaTypeMASK)
      {
      if ((lcl_status = shMaskWriteAsFits (((MASK  *)handle->ptr), FITSname, FITSdef, 
					   outPipePtr, tapeWrite)) != SH_SUCCESS)
         {
         shErrStackPush ("Error encountered writing HDU 0 (Primary HDU)");
         }
      goto rtn_return;
      }
   else if (handle->type == lcl_schemaTypeHDR)
      {
      if (pduWanted != SH_FITS_PDU_NONE)
         {
         lcl_status = SH_CONFLICT_ARG;
         shErrStackPush ("Additional PDU writing not supported for HDR object schemas");
         goto rtn_return;
         }
      if ((lcl_status = s_shFitsHDRwrite (((HDR   *)handle->ptr), FITSname, FITSdef, FITStype, append, FITSstd)) != SH_SUCCESS)
         {
         shErrStackPush ("Error encountered writing HDU 0 (Primary HDU)");
         }
      goto rtn_return;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Write out an ASCII or Binary Table.
    */

   if (handle->type != lcl_schemaTypeTBLCOL)
      {
      /*
       * Any other object schemas are not supported.
       */

      lcl_status = SH_NOT_SUPPORTED;
      shErrStackPush ("Writing out general object schemas (%s) is not supported", lcl_handleTypeName);
      shErrStackPush ("Cannot write %s HDU to FITS file %s", lcl_FITStypeName, lcl_FITSspec);
      goto rtn_return;
      }

   /*
    *   o   Check to see that the desired HDU type and what is to be written
    *       are compatible.
    */

   switch (FITStype)
   {
   case f_hduTABLE    :
   case f_hduBINTABLE :
                        break;
   case f_hduPrimary  : lcl_status = SH_INV_FITS_FILE; shErrStackPush ("%s cannot be written as Primary HDU", lcl_FITStypeName); 
                        goto rtn_return;
                        break;
   case f_hduGroups   : lcl_status = SH_NOT_SUPPORTED; shErrStackPush ("Random Groups writing is not supported");  goto rtn_return;
                        break;
   default            : lcl_status = SH_NOT_SUPPORTED; shErrStackPush ("Unknown FITS HDU type cannot be written"); goto rtn_return;
                        break;
   }


   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * At this point, we're to write a TBLCOL object schema to an ASCII or Binary
    * Table HDU.
    */

   lcl_tblCol = ((TBLCOL *)handle->ptr);

   if (FITSstd == STANDARD)		/* Check whether Table can be written */
      {					/* to comply with FITS.               */
      lcl_statusTmp = shFitsTBLCOLcomply (lcl_tblCol, FITStype, &lcl_FITSstd);
      if (   ((lcl_statusTmp != SH_SUCCESS) && (lcl_statusTmp != SH_INVOBJ))
          ||  (lcl_FITSstd   != STANDARD))
         {
         lcl_status = SH_INV_FITS_FILE;
         shErrStackPush ("TBLCOL Table cannot be written to comply with the FITS Standard");
         goto rtn_return;
         }
      }

   /*
    * Open the FITS file and position correctly within that file.
    *
    *   o   Check to see that the file is somewhat valid in FITS' eyes, namely
    *       that its length is a multiple of the FITS record length.
    */

   if (FITSdef == DEF_DEFAULT)		/* We're to default the default       */
      {
       FITSdef  = DEF_REGION_FILE;
      }

   if ((lcl_status = shFileSpecExpand (FITSname, &lcl_FITSspec, &lcl_FITSspecFull, FITSdef)) != SH_SUCCESS)
      {
      shErrStackPush ("Error defaulting/translating FITS file specification \"%s\"", FITSname);
      goto rtn_return;
      }

   if ((lcl_FITSfp = f_fopen (lcl_FITSspecFull, (append) ? "a" : "w")) == ((FILE *)0))
      {
      lcl_status = SH_FITS_OPEN_ERR;
      shErrStackPush ("Error opening FITS file %s for %s", lcl_FITSspecFull, (append) ? "appending" : "writing");
      goto rtn_return;
      }

   if ((lcl_FITSpos = ftell (lcl_FITSfp)) <= -1L) /* IRIX ret any neg num ### */
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("Unable to determine position in FITS file %s", lcl_FITSspecFull);
      goto rtn_return;
      }

   if ((lcl_FITSpos % (shFitsRecSize)) != 0)
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("FITS file %s not a multiple of FITS record size (%d bytes)", lcl_FITSspecFull, (shFitsRecSize));
      goto rtn_return;
      }

   /* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */
   /*
    * Write out a Primary HDU if requested.
    */

   if (pduWanted != SH_FITS_PDU_NONE)
      {
      if (lcl_FITSpos != 0)
         {
         lcl_status = SH_INV_FITS_FILE;
         shErrStackPush ("PDU can only be written at the beginning of FITS file %s", lcl_FITSspecFull);
         goto rtn_return;
         }
      if (pduWanted != SH_FITS_PDU_MINIMAL)
         {
         lcl_status = SH_NOT_SUPPORTED;
         shErrStackPush ("Only minimal PDU writing supported");
         goto rtn_return;
         }

      p_shHdrMallocForVec (&lcl_pduHdr);	/* Get header vector space    */
      if (lcl_pduHdr.hdrVec == ((HDR_VEC *)0))
         {
         lcl_status = SH_MALLOC_ERR;
         shErrStackPush ("Unable to allocate space for PDU header vector");
         goto rtn_return;
         }
                                                                                      lcl_pduHdrLineCnt = 0;
      shHdrInsertLogical (&lcl_pduHdr, "SIMPLE", (FITSstd == STANDARD), ((char *)0)); lcl_pduHdrLineCnt++;
      shHdrInsertInt     (&lcl_pduHdr, "BITPIX",                     8, ((char *)0)); lcl_pduHdrLineCnt++;
      shHdrInsertInt     (&lcl_pduHdr, "NAXIS",                      0, ((char *)0)); lcl_pduHdrLineCnt++;
      shHdrInsertLogical (&lcl_pduHdr, "EXTEND",                     1, ((char *)0)); lcl_pduHdrLineCnt++;
      shHdrInsertLine    (&lcl_pduHdr, lcl_pduHdrLineCnt, "END");                     lcl_pduHdrLineCnt++;

      if (!f_wrhead (lcl_pduHdr.hdrVec, lcl_FITSfp))  /* Write out PDU header */
         {
         lcl_status = SH_INV_FITS_FILE;
         shErrStackPush ("Error writing PDU header to FITS file %s", lcl_FITSspecFull);
         goto rtn_return;
         }
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Write out the ASCII or Binary Table.
    *
    *   o   At this point, only a TBLCOL object schema is written to an ASCII
    *       or Binary Table (any other Table type or object schema has been
    *       handled earlier).
    *
    *   o   lcl_tblCol points to a valid object.
    */

   if ((lcl_status = s_shFitsTBLCOLwrite (lcl_tblCol, lcl_FITSfp, FITStype,
                                          (shEndedness () == SH_BIG_ENDIAN) ? SH_FITS_NOSWAP : SH_FITS_SWAP, FITSstd)) != SH_SUCCESS)
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("Error encountered writing %s extension HDU", lcl_FITStypeName);
      goto rtn_return;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    *
    *   o   fflush () is called to guarantee that the file goes out to secondary
    *       storage (fclose () does NOT guarantee this).
    */

rtn_return : ;

   if (lcl_FITSfp        != ((FILE    *)0)) { fflush    (lcl_FITSfp); fclose  (lcl_FITSfp); }
   if (lcl_FITSspec      != ((char    *)0)) {    shFree (lcl_FITSspec);        }
   if (lcl_FITSspecFull  != ((char    *)0)) { shEnvfree (lcl_FITSspecFull);    }
   if (lcl_pduHdr.hdrVec != ((HDR_VEC *)0)) { p_shHdrFreeForVec (&lcl_pduHdr); }

   return (lcl_status);

   }	/*   shFitsWrite */

/******************************************************************************/

RET_CODE	   shFitsTBLCOLcomply
   (
   TBLCOL			*tblCol,	/* IN:    Table to check      */
   FITSHDUTYPE			 hduType,	/* IN:    FITS Extension type */
   FITSFILETYPE			*FITSstd	/* OUT:   FITS compliant?     */
   )

/*++
 * ROUTINE:	   shFitsTBLCOLcomply
 *
 *	Indicate whether the Table is FITS compliant (actually, has the poten-
 *	tial to be FITS compliant).
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_NOT_SUPPORTED	Failure.
 *			The FITS HDU type cannot be checked by this routine.
 *
 *   SH_INVOBJ		Warning.
 *			The Table itself is invalid (inconsistent).  The FITS
 *			compliance status is returned.
 *
 *   SH_INV_FITS_FILE	Failure.
 *			The Table cannot be written as the requested HDU type
 *			without being an illegal FITS file (different than
 *			just non-compliance with the FITS Standard).
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	The ordering of entries in the p_shTblDataTypeBINTABLEmap mapping table
 *	is important when used to determine the TFORM data type given an object
 *	schema/alignment type. Place the object schema/alignment types that are
 *	to translate to a TFORM data type near the start of the map table. This
 *	still does not prevent the need to test for particular object schema
 *	types explicitly.
 *
 *	   o	For example, place "compound" data types, such as a heap des-
 *		criptor (TFORM data type of 'P') near the end, since their
 *		alignment type in the map table is SHALIGNTYPE_S32, but an
 *		alignment type of SHALIGNTYPE_S32 (equivalent to an object
 *		schema type of INT) should be mapped to a TFORM data type of
 *		'J' (Binary Table).  The object schema type TBLHEAPDSC needs
 *		to be tested for explicitly before going through a mapping.
 *
 *   o	For Binary Tables, it is possible for non-standard integer data types to
 *	still have the potential to comply with the FITS standard. If TSCALn and
 *	TZEROn are unused or simply 1.0/0.0, TSCALn and TZEROn can be set to
 *	particular values that, by a FITS community convention, indicate that
 *	the sign of the data type is to be flipped.
 *
 *   o	The FITS HDU writers perform many of these tests.  If those tests are
 *	updated, those changes should also be reflected here.
 *--
 ******************************************************************************/

   {	/*   shFitsTBLCOLcomply */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status  = SH_SUCCESS;
   FITSFILETYPE		 lcl_FITSstd = STANDARD;	/* Assume compliance  */
   ARRAY		*lcl_array;
   TBLFLD		*lcl_tblFld;
                  int	 lcl_forBreak;		/* Break out of for () loop   */
                  int	 lcl_fldIdx;		/* Field index (0-indexed)    */
   SHALIGN_TYPE		 lcl_alignType;
                  int	 lcl_dataTypeIdx;
   TYPE			 lcl_schemaType;
   TYPE			 lcl_flipType;
   double		 lcl_flipTSCAL;
   double		 lcl_flipTZERO;

   TYPE			 lcl_schemaTypeTBLFLD     = shTypeGetFromName ("TBLFLD");	/* For performance */
   TYPE			 lcl_schemaTypeSTR        = shTypeGetFromName ("STR");		/* For performance */
   TYPE			 lcl_schemaTypeLOGICAL    = shTypeGetFromName ("LOGICAL");	/* For performance */
   TYPE			 lcl_schemaTypeTBLHEAPDSC = shTypeGetFromName ("TBLHEAPDSC");	/* For performance */
   TYPE			 lcl_schemaTypeUNKNOWN    = shTypeGetFromName ("UNKNOWN");	/* For performance */

/* These macros are dangerous. The "breaks" after those macro in switch
 * statements are removed to silence the compiler.
 */

#define	shFitsTBLCOLcomplyINVOBJ	lcl_status  = SH_INVOBJ
#define	shFitsTBLCOLcomplyNONSTANDARD	lcl_FITSstd = NONSTANDARD; goto rtn_return
#define	shFitsTBLCOLcomplyINV_FITS	lcl_FITSstd = NONSTANDARD; lcl_status = SH_INV_FITS_FILE; goto rtn_return

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Check whether the requested HDU type is supported.
    */

   switch (hduType)
      {
      case f_hduTABLE    : /* . . . . . . . . . . . . . . . . . . . . . . . . */

           for (lcl_fldIdx = 0; lcl_fldIdx < shChainSize(&(tblCol->fld)); lcl_fldIdx++)
              {

              lcl_array = (ARRAY *) shChainElementGetByPos(&(tblCol->fld), lcl_fldIdx);

              if ((lcl_array->dimCnt <= 0) || (lcl_array->dim[0] != tblCol->rowCnt)) { shFitsTBLCOLcomplyINVOBJ; }
              if  (lcl_array->data.schemaType == lcl_schemaTypeSTR)
                 {
                 if   (lcl_array->dimCnt  > 2) { shFitsTBLCOLcomplyINV_FITS; }
                 if   (lcl_array->dimCnt  < 2) { shFitsTBLCOLcomplyINV_FITS; }
                 }
              else if (lcl_array->data.schemaType == lcl_schemaTypeLOGICAL)
                 {
                 if   (lcl_array->dimCnt != 1) { shFitsTBLCOLcomplyINV_FITS; }
                 }
              else
                 {
                 if   (lcl_array->dimCnt != 1) { shFitsTBLCOLcomplyINV_FITS; }
                 if  ((lcl_alignType = shAlignSchemaToAlignMap (lcl_array->data.schemaType)) == SHALIGNTYPE_unknown)
                    {
                    shFitsTBLCOLcomplyINV_FITS;
                    }

                 /*
                  * Match the alignment type with the table of supported ASCII
                  * Table data types.  The Binary Table (that's right, Binary
                  * Table) data types are used, since they encompass more prim-
                  * itive data types.
                  */

                 for (lcl_dataTypeIdx = 0, lcl_forBreak = 0; ; lcl_dataTypeIdx++)
                    {
                    switch (p_shTblDataTypeBINTABLEmap[lcl_dataTypeIdx].dataType)
                       {
                       case '\000' : shFitsTBLCOLcomplyINV_FITS; break;

                       case 'P'    :		/* Ignore cases that were     */
                       case 'L'    :		/* tested specially above.    */
                                     break;

                       default     : if (p_shTblDataTypeBINTABLEmap[lcl_dataTypeIdx].fldType == lcl_alignType)
                                        {
                                        lcl_forBreak = 1;  /* Found a match   */
                                        if (p_shTblDataTypeBINTABLEmap[lcl_dataTypeIdx].ascFldFmtDef == '\000')
                                           {
                                           shFitsTBLCOLcomplyNONSTANDARD;
                                           }
                                        }
                                     break;
                       }
                    if (lcl_forBreak != 0)
                       {
                       break;
                       }
                    }
                 }
              }
           break;

      case f_hduBINTABLE : /* . . . . . . . . . . . . . . . . . . . . . . . . */

           for (lcl_fldIdx = 0; lcl_fldIdx < shChainSize(&(tblCol->fld)); lcl_fldIdx++)
              {
              lcl_array  = (ARRAY *) shChainElementGetByPos(&(tblCol->fld), lcl_fldIdx);
              lcl_tblFld = (lcl_array->infoType == lcl_schemaTypeTBLFLD) ? ((TBLFLD *)lcl_array->info) : ((TBLFLD *)0);

              if ((lcl_array->dimCnt <= 0) || (lcl_array->dim[0] != tblCol->rowCnt))
                 {
                 shFitsTBLCOLcomplyINVOBJ;
                 }
              lcl_schemaType = lcl_array->data.schemaType;
              if  (lcl_schemaType == lcl_schemaTypeSTR)
                 {
                 if   (lcl_array->dimCnt  < 2) { shFitsTBLCOLcomplyINV_FITS; }
                 }
              else if (lcl_schemaType == lcl_schemaTypeLOGICAL)
                 {
                 /* This space intentionally left blank */
                 }
              else
                 {
                 if   (lcl_schemaType == lcl_schemaTypeTBLHEAPDSC)
                    {
                    if (lcl_array->dimCnt != 1) { shFitsTBLCOLcomplyINV_FITS; }
                    if (lcl_tblFld == ((TBLFLD *)0))
                       {
                       continue;		/* Heap data type is UNKNOWN   */
                       }
                    lcl_schemaType = lcl_tblFld->heap.schemaType;
                    if (   (lcl_schemaType == lcl_schemaTypeLOGICAL)
                        || (lcl_schemaType == lcl_schemaTypeSTR)
                        || (lcl_schemaType == lcl_schemaTypeUNKNOWN) )
                       {
                       continue;		/* Valid heap data type       */
                       }
                    lcl_alignType = shAlignSchemaToAlignMap (lcl_schemaType);
                    }
                 else
                    {
                    if ((lcl_alignType = shAlignSchemaToAlignMap (lcl_schemaType)) == SHALIGNTYPE_unknown)
                       {
                       shFitsTBLCOLcomplyINV_FITS;
                       }
                    }

                 /*
                  * Match the alignment type with the table of supported Binary
                  * Table data types.
                  *
                  *   o   The alignment type reflects either RSA (record storage
                  *       area) data or heap data for a 'P' RSA field.
                  *
                  *   o   It is possible for non-standard integer data types to
                  *       still have the potential to comply with the FITS stan-
                  *       dard.  If TSCALn/TZEROn are unused or simply 1.0/0.0,
                  *       TSCALn/TZEROn can be set to particular values that, by
                  *       a FITS community convention, indicate that the sign of
                  *       the data type is to be flipped.
                  */

                 for (lcl_dataTypeIdx = 0, lcl_forBreak = 0; ; lcl_dataTypeIdx++)
                    {
                    switch (p_shTblDataTypeBINTABLEmap[lcl_dataTypeIdx].dataType)
                       {
                       case '\000' : shFitsTBLCOLcomplyINV_FITS;break;

                       case 'P'    :		/* Ignore cases that were     */
                       case 'L'    :		/* tested specially above.    */
                                     break;

                       default     : if (p_shTblDataTypeBINTABLEmap[lcl_dataTypeIdx].fldType == lcl_alignType)
                                        {
                                        lcl_forBreak = 1;  /* Found a match   */
                                        if (!p_shTblDataTypeBINTABLEmap[lcl_dataTypeIdx].dataTypeComply)
                                           {
                                           if (   !p_shTblDataTypeBINTABLEmap[lcl_dataTypeIdx].fldTSCALok
                                               || !p_shTblDataTypeBINTABLEmap[lcl_dataTypeIdx].fldTZEROok)
                                              {
                                              shFitsTBLCOLcomplyNONSTANDARD;
                                              }
                                           if (p_shTblFldSignFlip (lcl_schemaType, &lcl_flipType, &lcl_flipTSCAL, &lcl_flipTZERO)
                                               != SH_SUCCESS)
                                              {
                                              shFitsTBLCOLcomplyNONSTANDARD;
                                              }
                                           if (lcl_tblFld != ((TBLFLD *)0))
                                              {
                                              if (   (shInSet (lcl_tblFld->Tpres, SH_TBL_TSCAL) && (lcl_tblFld->TSCAL != 1.0))
                                                  || (shInSet (lcl_tblFld->Tpres, SH_TBL_TZERO) && (lcl_tblFld->TZERO != 0.0)) )
                                                 {
                                                 shFitsTBLCOLcomplyNONSTANDARD;
                                                 }
                                              }
                                           }
                                        }
                                     break;
                       }
                    if (lcl_forBreak != 0)
                       {
                       break;
                       }
                    }
                 }
              }
           break;

      default            : /* . . . . . . . . . . . . . . . . . . . . . . . . */

           lcl_status = SH_NOT_SUPPORTED;
           goto rtn_return;
           break;
      }

   /*
    * All done.
    */

rtn_return : ;

   if (FITSstd != ((FITSFILETYPE *)0)) { *FITSstd = lcl_FITSstd; }

   return (lcl_status);

#undef	shFitsTBLCOLcomplyINVOBJ
#undef	shFitsTBLCOLcomplyNONSTANDARD
#undef	shFitsTBLCOLcomplyINV_FITS

   }	/*   shFitsTBLCOLcomply */

/******************************************************************************/

RET_CODE	 p_shFitsHdrLocate
   (
   FILE			 *FITSfp,	/* INOUT: FITS file file pointer      */
   HDR_VEC		 *hdrVec,	/* OUT:   Header vector for handle    */
   unsigned       int	  hdrVecSize,	/* IN:    Vector count in hdrVec      */
                  int	 *hduFound,	/* OUT:   HDU located (0-indexed)     */
   FITSHDUTYPE		 *hduType,	/* OUT:   Located HDU type            */
   unsigned long  int	 *dataAreaSize,	/* OUT:   HDU data area size (w/ pad) */
   unsigned long  int	 *rsaSize,	/* OUT:   Record Storage area size    */
   unsigned long  int	 *heapOff,	/* OUT:   Heap offset from data start */
   unsigned long  int	 *heapSize,	/* OUT:   Heap area size              */
                  int	  hduWanted,	/* IN:    HDU to read (0-indexed)     */
                  char	 *xtension	/* IN:    XTENSION to read            */
   )

/*++
 * ROUTINE:	 p_shFitsHdrLocate
 *
 *	Locate a FITS header based on selection criteria.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *			The desired header was located and is legitimate.
 *
 *   SH_FITS_NOEXTEND	Failure.
 *			The FITS file does not contain the requested extension
 *			HDU.
 *
 *   SH_INV_FITS_FILE	Failure.
 *			The FITS file structure was invalid in some way.
 *
 * SIDE EFFECTS:
 *
 *   o	Prior to beginning the search, the FITS file is positioned at the
 *	start of the primary HDU.
 *
 *   o	The FITS file pointer will be positioned at the start of the data
 *	for the desired HDU or, in case of an error, at an arbitrary point
 *	in the FITS file.
 *
 *   o	Any existing header lines in the header vector (hdrVec) are deleted.
 *
 * SYSTEM CONCERNS:
 *
 *   o	The selection criteria arguments are assumed to have been checked
 *	previously (as in fitsRead ()).  The precedence of selection criteria
 *	is (from highest precedence to lowest):
 *
 *	   hduWanted	(if it's positive)
 *	   xtension	(if it's non-zero (empty string   I S   valid!))
 *
 *	This decision was made since the caller (or above) is likely to check
 *	the syntactic and semantic validity of the selection criteria.
 *
 *   o	This routine does not check all aspects of FITS file validity.  Only
 *	the primary header and the desired header are checked for complete
 *	validity.  Intermediate headers are not necessarily fully checked.
 *--
 ******************************************************************************/

   {	/* p_shFitsHdrLocate */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
                  int	 lcl_hduIdx = -1;	/* -1 ==> not located         */
   FITSHDUTYPE		 lcl_hduType;
   unsigned long  int	 lcl_dataAreaSize;
   unsigned long  int	 lcl_rsaSize;
   unsigned long  int	 lcl_heapOff;
   unsigned long  int	 lcl_heapSize;
                  int	 lcl_logical;		/* Temporary (used locally)   */
                  char	 lcl_hdrLine[shFitsHdrLineSize+1];  /* FITS hdr line  */

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Read the primary header and check whether it's legal.
    */

   *hduFound = -1;				/* In case we don't find it   */
   *hduType  = f_hduUnknown;			/* In case we don't find it   */

   rewind (FITSfp);				/* Position to BOF            */

   f_hdel (hdrVec);				/* Delete any old hdr lines   */

   if (!f_rdheadi (hdrVec, hdrVecSize, FITSfp))
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("Error reading FITS primary header");
      goto rtn_return;
      }

   if (!f_hinfo (hdrVec, 1, &lcl_hduType, &lcl_dataAreaSize, &lcl_rsaSize, &lcl_heapOff, &lcl_heapSize))
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("Invalid FITS primary header");
      goto rtn_return;
      }

   /*
    * If the primary header is not the target, locate the desired HDU.
    */

   lcl_status = SH_FITS_NOEXTEND;	/* Assume we will not locate HDU      */

   if (hduWanted != 0)			/* Want data outside primary HDU?     */
      {
               lcl_logical = 0;		/* Assume EXTEND not in primary header*/
      f_lkey (&lcl_logical, hdrVec, "EXTEND");
      if (!lcl_logical)
         {
         lcl_status = SH_FITS_NOEXTEND;
         shErrStackPush ("No extensions in FITS file");
         goto rtn_return;
         }

      /*
       * Now, bounce down the remaining HDUs trying to locate the desired one.
       */

      for (lcl_hduIdx = 1; ; lcl_hduIdx++)
         {
         /*
          * Skip past data area of current (soon to be previous) HDU.
          */

         if (p_shFitsSeek (FITSfp, 0, lcl_dataAreaSize) != SH_SUCCESS)
            {
            lcl_status = SH_INV_FITS_FILE;
            shErrStackPush ("Data area for HDU %d too short", lcl_hduIdx);
            shErrStackPush ("Invalid FITS file");
            goto rtn_return;
            }

         /*
          * Read in next header.
          */

         f_hdel (hdrVec);		/* Delete previous hdr lines          */

         if (!f_rdheadi (hdrVec, hdrVecSize, FITSfp))
            {
            lcl_status = SH_INV_FITS_FILE;
            shErrStackPush ("Error reading FITS header %d", lcl_hduIdx);
            goto rtn_return;
            }

         if (! f_hinfo (hdrVec, lcl_hduIdx, &lcl_hduType, &lcl_dataAreaSize, &lcl_rsaSize, &lcl_heapOff, &lcl_heapSize))
            {
            lcl_status = SH_INV_FITS_FILE;
            shErrStackPush ("Invalid FITS %s header %d", shFitsTypeName (lcl_hduType), lcl_hduIdx);
            goto rtn_return;
            }

         if (lcl_hduIdx == hduWanted)
            {
            lcl_status = SH_SUCCESS;	/* Located desired HDU by position    */
            break;
            }
         if (xtension != ((char *)0))
            {
            sprintf (lcl_hdrLine, "XTENSION= '%-8s'", xtension);
            if (f_smatch (hdrVec[0], lcl_hdrLine) != 0)
               {
               lcl_status = SH_SUCCESS;	/* Located desired HDU by extension   */
               break;
               }
            }
         }
      }

   if (lcl_status == SH_SUCCESS)
      {
      *hduFound     = lcl_hduIdx;
      *hduType      = lcl_hduType;
      *dataAreaSize = lcl_dataAreaSize;
      *rsaSize      = lcl_rsaSize;
      *heapOff      = lcl_heapOff;
      *heapSize     = lcl_heapSize;
      }
   else
      {
      if (lcl_status == SH_FITS_NOEXTEND)
         {
         shErrStackPush ("Desired FITS extension HDU could not be located");
         }
      }

   /*
    * All done.
    */

rtn_return : ;

   return (lcl_status);

   }	/* p_shFitsHdrLocate */

/******************************************************************************/

RET_CODE	 p_shFitsPostHdrLocate
   (
   FILE			 *FITSfp,	/* INOUT: FITS file file pointer      */
   HDR_VEC		 *hdrVec,	/* OUT:   Header vector for handle    */
   unsigned       int	  hdrVecSize,	/* IN:    Vector count in hdrVec      */
                  int	 *hduFound,	/* OUT:   HDU located (0-indexed)     */
                  int	  hduWanted	/* IN:    HDU to read (0-indexed)     */
   )

/*++
 * ROUTINE:	 p_shFitsPostHdrLocate
 *
 *	Locate a FITS header based on selection criteria.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *			The desired header was located and is legitimate.
 *
 *   SH_FITS_NOEXTEND	Failure.
 *			The FITS file does not contain the requested extension
 *			HDU.
 *
 *   SH_INV_FITS_FILE	Failure.
 *			The FITS file structure was invalid in some way.
 *
 * SIDE EFFECTS:
 *
 *   o	Prior to beginning the search, the FITS file is positioned at the
 *	start of the primary HDU.
 *
 *   o	The FITS file pointer will be positioned at the start of the data
 *	for the desired HDU or, in case of an error, at an arbitrary point
 *	in the FITS file.
 *
 *   o	Any existing header lines in the header vector (hdrVec) are deleted.
 *
 * SYSTEM CONCERNS:
 *
 *   o	The selection criteria arguments are assumed to have been checked
 *	previously (as in fitsRead ()).  The precedence of selection criteria
 *	is (from highest precedence to lowest):
 *
 *	   hduWanted	(if it's positive)
 *	   xtension	(if it's non-zero (empty string   I S   valid!))
 *
 *	This decision was made since the caller (or above) is likely to check
 *	the syntactic and semantic validity of the selection criteria.
 *
 *   o	This routine does not check all aspects of FITS file validity.  Only
 *	the primary header and the desired header are checked for complete
 *	validity.  Intermediate headers are not necessarily fully checked.
 *--
 ******************************************************************************/

   {	/* p_shFitsPostHdrLocate */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
                  int	 lcl_hduIdx = -1;	/* -1 ==> not located         */
                  int	 lcl_logical;		/* Temporary (used locally)   */

   FITSHDUTYPE		 lcl_hduType;
   unsigned long  int	 lcl_dataAreaSize;
   unsigned long  int	 lcl_rsaSize;
   unsigned long  int	 lcl_heapOff;
   unsigned long  int	 lcl_heapSize;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Read the primary header and check whether it's legal.
    */

   *hduFound = -1;				/* In case we don't find it   */

   rewind (FITSfp);				/* Position to BOF            */

   f_hdel (hdrVec);				/* Delete any old hdr lines   */

   if (!f_rdheadi (hdrVec, hdrVecSize, FITSfp))
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("Error reading FITS primary header");
      goto rtn_return;
      }

   if (!f_hinfo (hdrVec, 1, &lcl_hduType, &lcl_dataAreaSize, &lcl_rsaSize, &lcl_heapOff, &lcl_heapSize))
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("Invalid FITS primary header");
      goto rtn_return;
      }

   /*
    * If the primary header is not the target, locate the desired HDU.
    */

   lcl_status = SH_FITS_NOEXTEND;	/* Assume we will not locate HDU      */

   if (hduWanted != 0)	{		/* Want data outside primary HDU?     */
     
     lcl_logical = 0;		/* Assume EXTEND not in primary header*/
     f_lkey (&lcl_logical, hdrVec, "EXTEND");
     if (!lcl_logical) {
       lcl_status = SH_FITS_NOEXTEND;
       shErrStackPush ("No extensions in FITS file");
       goto rtn_return;
     }

     /*
      * Now, bounce down the remaining HDUs trying to locate the desired one.
      */

     for (lcl_hduIdx = 1; ; lcl_hduIdx++)  {
       /*
	* Skip past data area of current (soon to be previous) HDU.
	*/

       if (p_shFitsSeek (FITSfp, 0, lcl_dataAreaSize) != SH_SUCCESS)  {
	 lcl_status = SH_INV_FITS_FILE;
	 shErrStackPush ("Data area for HDU %d too short", lcl_hduIdx);
	 shErrStackPush ("Invalid FITS file");
	 goto rtn_return;
       }

       /*
	* Read in next header.
	*/
	    
       f_hdel (hdrVec);		/* Delete previous hdr lines          */
       
       if (!f_rdheadi (hdrVec, hdrVecSize, FITSfp)) {
	 lcl_status = SH_INV_FITS_FILE;
	 shErrStackPush ("Error reading FITS header %d", lcl_hduIdx);
	 goto rtn_return;
       }
       
       if (! f_hinfo (hdrVec, lcl_hduIdx, &lcl_hduType, &lcl_dataAreaSize, &lcl_rsaSize, &lcl_heapOff, &lcl_heapSize))
	 {
	   lcl_status = SH_INV_FITS_FILE;
	   shErrStackPush ("Invalid FITS %s header %d", shFitsTypeName (lcl_hduType), lcl_hduIdx);
	   goto rtn_return;
	 }

       if (lcl_hduIdx == hduWanted)  {
	 lcl_status = SH_SUCCESS;	/* Located desired HDU by position    */
	 break;
       }
     }           /*  for loop */
   } else {    /* if (hduWanted != 0) */
     lcl_status = SH_SUCCESS;
   }

   /* 
    * At the header requested, now just skip the data.
    */

   if (p_shFitsSeek (FITSfp, 0, lcl_dataAreaSize) != SH_SUCCESS) {
     lcl_status = SH_INV_FITS_FILE;
     shErrStackPush ("Data area for HDU %d too short", lcl_hduIdx);
     shErrStackPush ("Invalid FITS file");
     goto rtn_return;
   }
 


   if (lcl_status == SH_SUCCESS) {
     *hduFound     = lcl_hduIdx;
   } else {
     if (lcl_status == SH_FITS_NOEXTEND) {
       shErrStackPush ("Desired FITS extension HDU could not be located");
     }
   }

   /*
    * All done.
    */

rtn_return : ;

   return (lcl_status);

   }	/* p_shFitsPostHdrLocate */

/******************************************************************************/

RET_CODE	 p_shFitsHdrClean
   (
   HDR			 *hdr			/* INOUT: Header to clean     */
   )

/*++
 * ROUTINE:	 p_shFitsHdrClean
 *
 *	Clean a Dervish header of certain keywords, making them unavailable to the
 *	user.  The keywords are:
 *
 *	   SIMPLE		TFORMn
 *	   XTENSION		TTYPEn
 *	   BITPIX		TUNITn
 *	   NAXIS		TDISPn
 *	   NAXISn		TSCALn
 *	   TFIELDS		TZEROn
 *	   PCOUNT		TNULLn
 *	   GCOUNT		TDIMn
 *	   THEAP		TBCOLn
 *	   END
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* p_shFitsHdrClean */

   /*
    * Declarations.
    */

                  int	 lcl_TFIELDS;
                  int	 lcl_NAXIS;
                  int	 lcl_delIdx;		/* Field index (0-indexed)    */
                  char	 lcl_keyword[11+1];	/* FITS keyword(only need 8+1)*/

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Remove FITS keywords from the Dervish header.
    *
    *   o   Keywords are removed in reverse order, since it's likely that higher
    *       indexed keywords are placed later in the header.  This takes into
    *       account LIBFITS' structuring of headers; namely, that when a keyword
    *       is removed, keywords after it are moved up one line.  Overall, the
    *       removal of keywords in reverse order should reduce the amount of
    *       header line movement.
    */

   if (hdr->hdrVec == ((HDR_VEC *)0)) { goto rtn_return; }

   if (!f_ikey (&lcl_TFIELDS, hdr->hdrVec, "TFIELDS")) { lcl_TFIELDS = 0; }
   if (!f_ikey (&lcl_NAXIS,   hdr->hdrVec, "NAXIS"))   { lcl_NAXIS   = 0; }

   f_kdel (hdr->hdrVec, "END");

   for (lcl_delIdx = lcl_TFIELDS; lcl_delIdx >= 1; lcl_delIdx--)
      {
      sprintf (lcl_keyword, "TDIM%d",  lcl_delIdx); f_kdel (hdr->hdrVec, lcl_keyword);
      sprintf (lcl_keyword, "TNULL%d", lcl_delIdx); f_kdel (hdr->hdrVec, lcl_keyword);
      sprintf (lcl_keyword, "TZERO%d", lcl_delIdx); f_kdel (hdr->hdrVec, lcl_keyword);
      sprintf (lcl_keyword, "TSCAL%d", lcl_delIdx); f_kdel (hdr->hdrVec, lcl_keyword);
      sprintf (lcl_keyword, "TDISP%d", lcl_delIdx); f_kdel (hdr->hdrVec, lcl_keyword);
      sprintf (lcl_keyword, "TUNIT%d", lcl_delIdx); f_kdel (hdr->hdrVec, lcl_keyword);
      sprintf (lcl_keyword, "TTYPE%d", lcl_delIdx); f_kdel (hdr->hdrVec, lcl_keyword);
      sprintf (lcl_keyword, "TBCOL%d", lcl_delIdx); f_kdel (hdr->hdrVec, lcl_keyword);
      sprintf (lcl_keyword, "TFORM%d", lcl_delIdx); f_kdel (hdr->hdrVec, lcl_keyword);
      }

   f_kdel (hdr->hdrVec, "THEAP");
   f_kdel (hdr->hdrVec, "GCOUNT");
   f_kdel (hdr->hdrVec, "PCOUNT");
   f_kdel (hdr->hdrVec, "TFIELDS");

   for (lcl_delIdx = lcl_NAXIS;   lcl_delIdx >= 1; lcl_delIdx--)
      {
      sprintf (lcl_keyword, "NAXIS%d", lcl_delIdx); f_kdel (hdr->hdrVec, lcl_keyword);
      }

   f_kdel (hdr->hdrVec, "NAXIS");
   f_kdel (hdr->hdrVec, "BITPIX");
   f_kdel (hdr->hdrVec, "XTENSION");
   f_kdel (hdr->hdrVec, "SIMPLE");

   /*
    * All done.
    */

rtn_return : ;

   return (SH_SUCCESS);

   }	/* p_shFitsHdrClean */

/******************************************************************************/

RET_CODE	 p_shFitsSeek
   (
   FILE			 *FITSfp,	/* INOUT: FITS file file pointer      */
            long  int	  posCur,	/* IN:    Current     relative pos    */
            long  int	  posDest	/* IN:    Destination relative pos    */
   )

/*++
 * ROUTINE:	 p_shFitsSeek
 *
 *	Change the file position based on two relative values.  These values do
 *	not necessarily have to reflect an ftell () position within the file.
 *	The positions are byte-oriented.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_INV_FITS_FILE	Failure.
 *			The file could not be positioned.
 *
 * SIDE EFFECTS:
 *
 *   o	If the destination position is not reachable, the file may be positioned
 *	indeterminately.
 *
 * SYSTEM CONCERNS:
 *
 *   o	Positioning is done   O N L Y   in the   F O R W A R D   direction.
 *--
 ******************************************************************************/

   {	/* p_shFitsSeek */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
                  char	 lcl_char = 0;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    *   o   fread () is used to force the file to be positioned (fseek () will
    *       not necessarily return an error if the file is positioned past EOF).
    */

   if (posDest > posCur)
      {
      if (fseek (FITSfp, (posDest - posCur - 1), SEEK_CUR) != 0)
         {
         lcl_status = SH_INV_FITS_FILE;
         }
      else
         {
         if (fread (((void *)&lcl_char), 1, 1, FITSfp) != 1)
            {
            lcl_status = SH_INV_FITS_FILE;
            }
         }
      }
						/*                         ###*/
   if (lcl_status != SH_SUCCESS)		/* This is not quite right.###*/
      {						/* ANSI-C fseek and fread  ###*/
      if (errno != 0)				/* not document as setting ###*/
         {					/* errno.  But, we're not  ###*/
         shErrStackPush (strerror (errno));	/* permitted to use non-   ###*/
         }					/* ANSI-C routines (such   ###*/
      }						/* as ferror or feof).     ###*/

   return (lcl_status);

   }	/* p_shFitsSeek */

/******************************************************************************/

RET_CODE	 p_shFitsLRecMultiple
   (
   FILE			 *FITSfp	/* INOUT: FITS file file pointer      */
   )

/*++
 * ROUTINE:	 p_shFitsLRecMultiple
 *
 *	Determine whether the file's length is a multiple of the FITS logical
 *	record size.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_INV_FITS_FILE	Failure.
 *			The file is not a multiple of the FITS logical record
 *			size.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	Position information is saved and restored.
 *--
 ******************************************************************************/

   {	/* p_shFitsLRecMultiple */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
            long  int	 lcl_filePos;
            long  int	 lcl_fileEnd;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   if ((lcl_filePos = ftell (FITSfp)) <= -1L)
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("Unable to determine file position");
      goto rtn_return;
      }

   if (fseek (FITSfp, 0L, SEEK_END) != 0)
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("Unable to determine file position");
      fseek (FITSfp, lcl_filePos, SEEK_SET);	/* Try to restore file pos    */
      goto rtn_return;
      }

   if ((lcl_fileEnd = ftell (FITSfp)) <= -1L)
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("Unable to determine file position");
      fseek (FITSfp, lcl_filePos, SEEK_SET);	/* Try to restore file pos    */
      goto rtn_return;
      }

   if ((lcl_fileEnd % shFitsRecSize) != 0)
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("FITS file not a multiple of FITS record size (%d bytes)", shFitsRecSize);
      }

   if (fseek (FITSfp, lcl_filePos, SEEK_SET) != 0)
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("Unable to restore file position");
      }

rtn_return : ;

   return (lcl_status);

   }	/* p_shFitsLRecMultiple */

/******************************************************************************/

static void	 s_shFitsPlatformCheck
   (
   void
   )

/*++
 * ROUTINE:	 s_shFitsPlatformCheck
 *
 *	Determine whether assumptions made by the code are comaptible with the
 *	platform this code is being run on.  The checks are:
 *
 *	   o	The number of bytes to read from/write to the FITS HDU is based
 *		on shAlign information.  That, in turn, is based on sizeof ().
 *		Technically, this is   N O T   the safe.  The problem is that
 *		sizeof () is defined to reflect not only the size of the data,
 *		but also any padding before or after the data.
 *
 *		In practice, none of the primitive data types have padding
 *		applied to them, (except for structures, and TBLHEADPDSCs are
 *		read in).  But, the read/write sizes and the in-memory data
 *		type sizes are checked for consistency.
 *
 * RETURN VALUES:	None
 *
 * SIDE EFFECTS:
 *
 *   o	If the checks do not succeed, Dervish is aborted (through shAssert ()).
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* s_shFitsPlatformCheck */

   /*
    * Declarations.
    */

   size_t		 lcl_size;	/* Used to get around a compilation   */
					/* warning complaining against the    */
					/* use of a constant expression in    */
					/* a conditional context. The prob-   */
					/* lem is that sizeof () cannot be    */
					/* used in the preprocessor environ-  */
					/* ment either (for conditional com-  */
					/* pilation.                          */

   TBLHEAPDSC		 lcl_tblHeapDsc;	/* For shAssert sanity checks */

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Make sure the platform we're on is compatible between FITS binary data
    * types and the machine's representation of those data types.
    */

   shAssert (shAlignment.type[SHALIGNTYPE_U8].size   == 1); /* B, L data type */
   shAssert (shAlignment.type[SHALIGNTYPE_S16].size  == 2); /* I    data type */
   shAssert (shAlignment.type[SHALIGNTYPE_S32].size  == 4); /* J    data type */
   shAssert (shAlignment.type[SHALIGNTYPE_FL32].size == 4); /* E    data type */
   shAssert (shAlignment.type[SHALIGNTYPE_FL64].size == 8); /* D    data type */

   /*
    * Make sure that Table-specific structures are certain sizes.
    */

   shAssert (sizeof (LOGICAL)            == (lcl_size = 1)); /* L   data type */
   shAssert (sizeof (lcl_tblHeapDsc.cnt) ==  shAlignment.type[SHALIGNTYPE_S32].size); /* P   data type */
   shAssert (sizeof (lcl_tblHeapDsc.ptr) >=  shAlignment.type[SHALIGNTYPE_S32].size); /* P   data type */

   }	/* s_shFitsPlatformCheck */

/******************************************************************************/

static RET_CODE	 s_shFitsHDRwrite
   (
   HDR			 *hdr,		/* INOUT: Dervish header handle         */
                  char	 *FITSname,	/* IN:    FITS file specification     */
   DEFDIRENUM		  FITSdef,	/* IN:    Which default to apply      */
   FITSHDUTYPE		  FITStype,	/* IN:    Type of HDU to write        */
                  int	  append,	/* IN:    Boolean: append to file     */
   FITSFILETYPE		  FITSstd	/* IN:    SIMPLE=?/chk FITS compliance*/
   )

/*++
 * DESCRIPTION:
 *
 *	Write only a FITS header.  Checks are made to see that the header will
 *	result in a valid FITS header (and file).
 *
 *	If missing, the following FITS keywords are inserted temporarily:
 *
 *	   SIMPLE	(for Primary   HDUs)
 *	   XTENSION	(for extension HDUs)
 *	   BITPIX	(defaults to 8)
 *	   NAXIS
 *	   END
 *
 *	NOTE:	HDRs are currently restricted to being written as the Primary
 *		HDU.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_NOT_SUPPORTED	Failure.
 *			The HDU type requested is not supported.
 *
 *   SH_MALLOC_ERR	Failure.
 *			Space could not be allocated for a temporary copy of the
 *			header.
 *
 *   SH_INV_FITS_FILE	Failure.
 *			The header would result in an invalid FITS file.
 *
 *   SH_CONFLICT_ARG	Failure.
 *			The FITSstd and SIMPLE keyword conflict.
 *
 * SIDE EFFECTS:
 *
 *   o	The order of the header keywords may not be preserved.  An attempt is
 *	made to order keywords to follow the FITS Standard.
 *
 *   o  The FITS file is closed using fclose instead of f_fclose because the
 *      routines f_fopen and f_fclose are not meant to be used as a pair.
 *
 * SYSTEM CONCERNS:
 *
 *   o	Only Primary HDU headers can be written.
 *--
 ******************************************************************************/

   {	/* s_shFitsHDRwrite */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
   FILE			*lcl_FITSfp         = ((FILE *)0);
            long  int	 lcl_FITSpos;
                  char	*lcl_FITSspec       = ((char *)0);
                  char	*lcl_FITSspecFull   = ((char *)0);
   HDR			 lcl_hdr;		/* Header to be written out   */
                  int	 lcl_hdrLineCnt;
                  char	*lcl_hdrLine;
                  int	 lcl_hdrLineIdx;
   /* char lcl_keyword[11+1]; */	/* FITS keyword(only need 8+1)*/
                  int	 lcl_keylVal;
                  int	 lcl_keyiVal;

#define	lcl_shHdrKeywordDefault(  keywordType, keywordTypeInitial, keyword, keywordVal)\
   if (f_lnum (hdr->hdrVec, #keyword) == -1)\
      {\
      shHdrInsert ## keywordType (&lcl_hdr, #keyword, 8,                                    ((char *)0)); lcl_hdrLineCnt++;\
      }\
   else\
      {\
      if (!f_ ## keywordTypeInitial ## key (&lcl_key ## keywordTypeInitial ## Val, hdr->hdrVec, #keyword))\
         {\
         lcl_status = SH_INV_FITS_FILE;\
         shErrStackPush (#keyword " keyword is invalid");\
         goto rtn_return;\
         }\
      shHdrInsert ## keywordType (&lcl_hdr, #keyword, lcl_key ## keywordTypeInitial ## Val, ((char *)0)); lcl_hdrLineCnt++;\
      }

#define	lcl_shHdrKeywordGuarantee(keywordType, keywordTypeInitial, keyword, keywordVal)\
   shHdrInsert ## keywordType (&lcl_hdr, #keyword, keywordVal, ((char *)0)); lcl_hdrLineCnt++;\
   if (f_lnum (hdr->hdrVec, #keyword) != -1)\
      {\
      if (!f_ ## keywordTypeInitial ## key (&lcl_key ## keywordTypeInitial ## Val, hdr->hdrVec, #keyword))\
         {\
         lcl_status = SH_INV_FITS_FILE;\
         shErrStackPush (#keyword " keyword is invalid");\
         goto rtn_return;\
         }\
      if (lcl_key ## keywordTypeInitial ## Val != 0)\
         {\
         lcl_status = SH_INV_FITS_FILE;\
         shErrStackPush (#keyword " keyword value must be " #keywordVal " for HDU without data");\
         goto rtn_return;\
         }\
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Check parameters.
    */

   memset (((void *)&lcl_hdr), 0, sizeof (HDR)); /* Clear for rtn_return test */

   if (append)
      {
      lcl_status = SH_NOT_SUPPORTED;
      shErrStackPush ("appending is not supported (yet)");
      }

   if ((FITStype != f_hduPrimary) && (FITStype != f_hduUnknown))
      {
      lcl_status = SH_NOT_SUPPORTED;
      shErrStackPush ("Only Primary HDUs are supported");
      }

   if (lcl_status != SH_SUCCESS)
      {
      goto rtn_return;
      }

   /*
    * Get memory for header copy.
    */

   p_shHdrMallocForVec (&lcl_hdr);		/* Get header vector space    */
   if (lcl_hdr.hdrVec == ((HDR_VEC *)0))
      {
      lcl_status = SH_MALLOC_ERR;
      shErrStackPush ("Unable to allocate space for header vector");
      goto rtn_return;
      }

   /*
    * Check the Dervish header and insert keywords.
    */
                                                                                   lcl_hdrLineCnt = 0;
   if ((hdr->hdrVec == ((HDR_VEC *)0)) && (f_lnum (hdr->hdrVec, "SIMPLE") == -1))
      {
      shHdrInsertLogical (&lcl_hdr, "SIMPLE", (FITSstd == STANDARD), ((char *)0)); lcl_hdrLineCnt++;
      }
   else
      {
      if ((hdr->hdrVec == ((HDR_VEC *)0)) || !f_lkey (&lcl_keylVal, hdr->hdrVec, "SIMPLE"))
         {
         lcl_status = SH_INV_FITS_FILE;
         shErrStackPush ("SIMPLE keyword is invalid");
         goto rtn_return;
         }
      if (!lcl_keylVal && (FITSstd == STANDARD))
         {
         lcl_status = SH_CONFLICT_ARG;
         shErrStackPush ("FITSstd == STANDARD and SIMPLE=F conflict");
         goto rtn_return;
         }
      shHdrInsertLogical (&lcl_hdr, "SIMPLE", lcl_keylVal,           ((char *)0)); lcl_hdrLineCnt++;
      }

   lcl_shHdrKeywordDefault   (Int, i, BITPIX, 8);
   lcl_shHdrKeywordGuarantee (Int, i, NAXIS,  0);

   /*
    *   o   Copy Dervish header lines into FITS header.
    */

   if (hdr->hdrVec != ((HDR_VEC *)0))
      {
      for (lcl_hdrLineIdx = 0; (lcl_hdrLine = hdr->hdrVec[lcl_hdrLineIdx]) != ((char *)0); lcl_hdrLineIdx++)
         {
              if (strncmp (lcl_hdrLine, "SIMPLE  ", 8) == 0) {}
         else if (strncmp (lcl_hdrLine, "BITPIX  ", 8) == 0) {}
         else if (strncmp (lcl_hdrLine, "NAXIS   ", 8) == 0) {}
         else if (strncmp (lcl_hdrLine, "END     ", 8) == 0) { break; }	/* Done copying */
         else
            {
            shHdrInsertLine (&lcl_hdr, lcl_hdrLineCnt, lcl_hdrLine); lcl_hdrLineCnt++;
            }
         }
      }

   /*
    *   o   Put in the END keyword.
    */

   shHdrInsertLine (&lcl_hdr, lcl_hdrLineCnt, "END"); lcl_hdrLineCnt++;

   /*
    * Write out the header.
    */

   if (FITSdef == DEF_DEFAULT)		/* We're to default the default       */
      {
       FITSdef  = DEF_REGION_FILE;
      }

   if ((lcl_status = shFileSpecExpand (FITSname, &lcl_FITSspec, &lcl_FITSspecFull, FITSdef)) != SH_SUCCESS)
      {
      shErrStackPush ("Error defaulting/translating FITS file specification \"%s\"", FITSname);
      goto rtn_return;
      }

   if ((lcl_FITSfp = f_fopen (lcl_FITSspecFull, (append) ? "a" : "w")) == ((FILE *)0))
      {
      lcl_status = SH_FITS_OPEN_ERR;
      shErrStackPush ("Error opening FITS file %s for %s", lcl_FITSspecFull, (append) ? "appending" : "writing");
      goto rtn_return;
      }

   if ((lcl_FITSpos = ftell (lcl_FITSfp)) <= -1L) /* IRIX ret any neg num ### */
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("Unable to determine position in FITS file %s", lcl_FITSspecFull);
      goto rtn_return;
      }

   if ((lcl_FITSpos % (shFitsRecSize)) != 0)
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("FITS file %s not a multiple of FITS record size (%d bytes)", lcl_FITSspecFull, (shFitsRecSize));
      goto rtn_return;
      }

   if (lcl_FITSpos != 0)
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("Header can only be written at the beginning of FITS file %s", lcl_FITSspecFull);
      goto rtn_return;
      }

   if (!f_wrhead (lcl_hdr.hdrVec, lcl_FITSfp))	/* Write out header           */
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("Error writing header to FITS file %s", lcl_FITSspecFull);
      goto rtn_return;
      }

   /*
    * All done.
    */

rtn_return : ;

   if (lcl_FITSfp       != ((FILE    *)0)) { fflush    (lcl_FITSfp); fclose (lcl_FITSfp); }
   if (lcl_FITSspec     != ((char    *)0)) {    shFree (lcl_FITSspec);     }
   if (lcl_FITSspecFull != ((char    *)0)) { shEnvfree (lcl_FITSspecFull); }
   if (lcl_hdr.hdrVec   != ((HDR_VEC *)0)) { p_shHdrFreeForVec (&lcl_hdr); }

   return (lcl_status);

#undef	lcl_shHdrKeywordDefault
#undef	lcl_shHdrKeywordGuarantee

   }	/* s_shFitsHDRwrite */

/******************************************************************************/

static RET_CODE	 s_shFitsTBLCOLread
   (
   TBLCOL		 *tblCol,	/* INOUT: Table handle                */
   FILE			 *FITSfp,	/* INOUT: FITS file file pointer      */
   FITSHDUTYPE		  hduType,	/* IN:    Type of HDU                 */
   SHFITSSWAP		  datumSwap,	/* IN:    Swap datum bytes?           */
   unsigned long  int	  dataAreaSize,	/* IN:    Data area size (w/ padding) */
   unsigned long  int	  rsaSize,	/* IN:    Record Storage Area size    */
   unsigned long  int	  heapOff,	/* IN:    Heap area offset            */
   unsigned long  int	  heapSize	/* IN:    Heap area size              */
   )

/*++
 * ROUTINE:	 s_shFitsTBLCOLread
 *
 *	Read a FITS ASCII or Binary Table into a TBLCOL object schema.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_NOT_SUPPORTED	Failure.
 *			The HDU type requested is not supported.
 *
 *   SH_MALLOC_ERR	Failure.
 *			Space could not be allocated for the TBLFLD field des-
 *			criptors.
 *
 *   SH_INV_FITS_FILE	Failure.
 *			The data area was not large enough or improperly
 *			blocked (into shFitsRecSize chunks).  The TBLCOL
 *			data areas may be partially filled.
 *
 * SIDE EFFECTS:
 *
 *   o	If the header is a valid FITS ASCII or Binary Table extension header,
 *	data is read from the file.  This changes the file position.  If the
 *	read is successful, the file will be positioned at the start of the
 *	next HDU (or EOF).  Otherwise, the file position is indeterminate.
 *
 * SYSTEM CONCERNS:
 *
 *   o	The HDU must have already been checked to make sure that a ASCII or
 *	Binary Table extension is being read.  The FITS file must be positioned
 *	at the first byte of data in the HDU.
 *
 *   o	The size of row data computed from TFORMns must not be larger than
 *	NAXIS1.
 *
 *   o	Some FITS keywords are removed from the Dervish header:
 *
 *	   XTENSION		TFORMn		TBCOLn
 *	   BITPIX		TTYPEn
 *	   NAXIS		TUNITn		
 *	   NAXISn		TDISPn
 *	   TFIELDS		TSCALn
 *	   PCOUNT		TZEROn
 *	   GCOUNT		TNULLn
 *	   THEAP		TDIMn
 *	   END
 *
 *   o	Reading from the FITS file is done sequentially.  This allows for a
 *	more efficient read from magnetic tapes, as there will be no bouncing
 *	around the tape file (if the file is large enough to need buffering
 *	into more than one buffer by the I/O system).
 *
 *   o	Alignment space between the pointer and data areas (per field) is added
 *	with the assumption that malloc (shMalloc) will allocate storage with
 *	proper aligment so that any data type on the machine can be stored at
 *	the start of the allocated space).
 *
 *   o	A performance hit is taken because the allocated data space is zeroed
 *	out.  This helps when reading in data where the data has padding or
 *	alignment after it.
 *--
 ******************************************************************************/

   {	/* s_shFitsTBLCOLread */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
   ARRAY		*lcl_array;
   TBLFLD		*lcl_tblFld;		/* TBLFLD structure           */
                  int	 lcl_NAXIS1;		/* Number of bytes per row    */
                  int	 lcl_TFIELDS;
                  int	 lcl_fldIdx;		/* Field index (0-indexed)    */
                  int	 lcl_hdrClean = 0;	/* Clean up header?           */

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   switch (hduType)
      {
      case f_hduTABLE    :
      case f_hduBINTABLE :
                           break;

      case f_hduPrimary  : lcl_status = SH_NOT_SUPPORTED;
                           shErrStackPush ("Primary HDUs cannot be read into TBLCOL object schema");
                           goto rtn_return;
                           break;

      case f_hduGroups   : lcl_status = SH_NOT_SUPPORTED;
                           shErrStackPush ("Random Groups HDUs cannot be read into TBLCOL object schema");
                           goto rtn_return;
                           break;

      default            : lcl_status   = SH_NOT_SUPPORTED;
                           shErrStackPush ("Unknown HDU type cannot be read into TBLCOL object schema");
                           goto rtn_return;
                           break;
      }

   /*
    * Most legalities of the ASCII or Binary Table header have been checked
    * (ultimately by f_hinfo () called by p_shFitsHdrLocate ()).
    *
    *   o   All required keywords are present: XTENSION, BITPIX, NAXIS, NAXIS1,
    *       NAXIS2, PCOUNT, GCOUNT, and TFIELDS.
    *
    *   o   All TFORMn keywords (where n is the TFIELDS value) are present.
    *
    *   o   Not all required keyword values have been checked.
    *
    * NOTE:  If a keyword that was (assumed to be) checked previously is gotten
    *        again in this routine, then its validity will not be checked. This
    *        requires some coordination with p_shFitsHdrLocate () and f_hinfo().
    *
    * NOTE:  NAXIS1 actually represents the number of storage units of BITPIX
    *        bits in size.  This needs to be checked (it isn't yet)!!!
    */

   if ((lcl_status = p_shTblFldGen ((&tblCol->fld), tblCol->hdr.hdrVec, hduType, &lcl_TFIELDS, &lcl_NAXIS1, &tblCol->rowCnt))
                  != SH_SUCCESS)
      {
      goto rtn_return;
      }

   lcl_hdrClean = 1;			/* Indicate header needs to be cleaned*/

   /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/
   /*
    * Allocate space for the data from the Record Storage Area (RSA).
    *
    *   o   Space is allocated for NAXIS2 rows.
    *
    *   o   Space is allocated for the hierarchical set of pointers (to permit
    *       C array referencing notation without knowing array bounds) and the
    *       data itself.  If necessary, some space is allocated for alignment
    *       between the pointers and data area.
    */

   for (lcl_fldIdx = 0; ((lcl_fldIdx < shChainSize(&(tblCol->fld))) && (lcl_fldIdx < lcl_TFIELDS)); lcl_fldIdx++)
      {
      lcl_array = (ARRAY *) shChainElementGetByPos(&(tblCol->fld), lcl_fldIdx);

      shAssert ((lcl_tblFld = ((TBLFLD *)lcl_array->info)) != ((TBLFLD *)0)); /* shTblFldGen should have done this */

      if (lcl_tblFld->prvt.elemCnt != 0)
         {
         if ((lcl_status = shArrayDataAlloc (lcl_array, ((char *)0))) != SH_SUCCESS)
            {
            shErrStackPush ("Unable to allocate space for Table field ARRAY data (and pointers)");
            goto rtn_return;
            }
         }
      else
         {
         lcl_array->arrayPtr     = ((void          *)0);	/* For safety */
         lcl_array->data.dataPtr = ((unsigned char *)0);	/* For safety */
         }
      lcl_array->nStar          = 0;	/* All data maintained by ARRAY       */
      lcl_tblFld->prvt.datumPtr = lcl_array->data.dataPtr;
      }

   /*
    * Read in data from the TBLFLD fields from the FITS file.
    */

   switch (hduType)
      {
      case f_hduTABLE    : if ((lcl_status = s_shFitsTBLCOLreadAsc (tblCol, FITSfp, lcl_TFIELDS, lcl_NAXIS1, dataAreaSize, rsaSize))
                                          != SH_SUCCESS)
                              {
                              goto rtn_return;
                              }
                           break;
      case f_hduBINTABLE : if ((lcl_status = s_shFitsTBLCOLreadBin (tblCol, FITSfp, lcl_TFIELDS, lcl_NAXIS1, datumSwap,
                                                                    dataAreaSize, rsaSize, heapOff, heapSize)) != SH_SUCCESS)
                              {
                              goto rtn_return;
                              }
                           break;

      default            : lcl_status = SH_INTERNAL_ERR;
                           shErrStackPush ("FITS HDU type not caught earlier (%s line %d)", __FILE__, __LINE__);
                           shFatal        ("FITS HDU type not caught earlier (%s line %d)", __FILE__, __LINE__);
                           goto rtn_return;
                           break;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    *
    *   o   If there were any errors, clean up after ourselves.
    */

rtn_return : ;

   if (lcl_hdrClean != 0)
      {
      p_shFitsHdrClean (&tblCol->hdr);	/* Remove FITS keywords from Dervish hdr*/
      }

   if (lcl_status != SH_SUCCESS)
      {
      while (p_shTblFldDelByIdx (&(tblCol->fld), HEAD) == SH_SUCCESS) {}
      }

   return (lcl_status);

   }	/* s_shFitsTBLCOLread */

/******************************************************************************/

static RET_CODE	 s_shFitsTBLCOLreadAsc
   (
   TBLCOL		 *tblCol,	/* INOUT: Table handle                */
   FILE			 *FITSfp,	/* INOUT: FITS file file pointer      */
                  int	  TFIELDS,	/* IN:    Number of Table fields      */
                  int	  rowSize,	/* IN:    Size of a Table row (bytes) */
   unsigned long  int	  dataAreaSize,	/* IN:    Data area size (w/ padding) */
   unsigned long  int	  rsaSize	/* IN:    Record Storage Area size    */
   )

/*++
 * ROUTINE:	 s_shFitsTBLCOLreadAsc
 *
 *	Read ASCII Table data into a TBLCOL object schema.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_MALLOC_ERR	Failure.
 *			Space could not be allocated for reading a Table row.
 *
 *   SH_INV_FITS_FILE	Failure.
 *			The data area was not large enough or improperly
 *			blocked (into shFitsRecSize chunks).  The TBLCOL
 *			data areas may be partially filled.
 *
 * SIDE EFFECTS:
 *
 *   o	Data is read from the FITS file.  This changes the file position.  If
 *	the read is successful, the file will be positioned at the start of the
 *	next HDU (or EOF).  Otherwise, the file position is indeterminate.
 *
 * SYSTEM CONCERNS:
 *
 *   o	The HDU must have already been checked to make sure that a ASCII Table
 *	extension is being read.  The FITS file must be positioned at the first
 *	byte of data in the HDU.
 *
 *   o	The size of row data computed from TFORMns and TBCOLns must not be
 *	larger than NAXIS1.
 *
 *   o	It is assumed that TBLFLD_FLD's elemCnt is 1 (there are no ASCII Table
 *	data types (FORTRAN FORMAT specifiers) that read more than 1 number).
 *	If this changes (p_shTblDataTypeTABLEmap's fldCnt field is set to a
 *	value other than 1), this code will need be changed.
 *--
 ******************************************************************************/

   {	/* s_shFitsTBLCOLreadAsc */

   /*
    * Declarations.
    */

     RET_CODE		 lcl_status = SH_SUCCESS;
     ARRAY		*lcl_array;
     TBLFLD		*lcl_tblFld;		/* TBLFLD structure           */
     int	 lcl_rowIdx;		/* RSA record/row index       */
     int	 lcl_fldIdx;		/* Field index (0-indexed)    */
     int	 lcl_datumSize;		/* Size of datum in bytes     */
     int	 lcl_datumIncr;		/* Space between datums: bytes*/
     unsigned       char	*lcl_datum;
     char	*lcl_fldBeg;
     long  int	 lcl_long;		/* Used to read `I' format    */
     double		 lcl_double;		/* Used to read real number   */
     char	*lcl_buf = ((char *)0);	/* One Table row              */
     size_t		 lcl_bufLen;
     long  int	 lcl_FITSfpRead = 0;	/* Amount of bytes read       */
     int	 lcl_FITSfpEOF  = FALSE; /* EOF detected?             */
     /*long  int	 lcl_FITSpos;*/
     
   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Correct arguments:
    *
    *   o   dataAreaSize is assumed to be a multiple of shFitsRecSize.
    */

   if ((lcl_bufLen  = dataAreaSize % (shFitsRecSize)) != 0)
      {
      dataAreaSize += ((shFitsRecSize) - lcl_bufLen);
      }

   /*
    * Read in data from the TBLFLD fields from the FITS file.
    *
    *   o   This reading is very simple (not even double buffered).  Space is
    *       allocated to hold one record (plus an additional character to let
    *       fields be null-terminated safely).
    */

   if ((lcl_buf = (char*)shMalloc (rowSize + 1)) == ((char *)0))
      {
      lcl_status = SH_MALLOC_ERR;
      shErrStackPush ("Unable to allocate space for reading ASCII Table row");
      goto rtn_return;
      }

   for (lcl_rowIdx = 0; lcl_rowIdx < tblCol->rowCnt; lcl_rowIdx++)
      {

      lcl_FITSfpRead += (lcl_bufLen = fread (lcl_buf, 1, rowSize, FITSfp));
      if (lcl_bufLen != rowSize)
         {
         lcl_status = SH_INV_FITS_FILE;
         if (lcl_bufLen != 0) { shErrStackPush ("End of file detected"); lcl_FITSfpEOF = TRUE; }
                        else  { shErrStackPush ("Read error"); }
         goto rtn_return;
         }

      for (lcl_fldIdx = 0; ((lcl_fldIdx  < shChainSize(&(tblCol->fld))) && (lcl_fldIdx < TFIELDS)); lcl_fldIdx++)
         {
         lcl_array = (ARRAY *) shChainElementGetByPos(&(tblCol->fld), lcl_fldIdx);

         shAssert ((lcl_tblFld = ((TBLFLD *)lcl_array->info)) != ((TBLFLD *)0)); /* Ok cast */
         lcl_datum     =  lcl_tblFld->prvt.datumPtr;
         lcl_datumSize =  lcl_array->data.size;
         lcl_datumIncr =  lcl_array->data.incr;
         lcl_fldBeg    = &lcl_buf[lcl_tblFld->prvt.TFORM.ascFldOff];

         switch (lcl_tblFld->prvt.TFORM.ascFldFmt)
            {
            case 'A' : lcl_datumIncr =                           lcl_tblFld->prvt.elemCnt;	/* elemCnt reflects extra char    */
                       strncpy (((char *)lcl_datum), lcl_fldBeg, lcl_tblFld->prvt.elemCnt - 1);	/* ...used for null termination.  */
                                ((char *)lcl_datum)[             lcl_tblFld->prvt.elemCnt - 1] = 0; /* Null terminate explicitly  */
                       break;

            case 'I' : if (s_shFitsTBLCOLreadAscInt (lcl_fldBeg, lcl_tblFld->prvt.TFORM.ascFldWidth, &lcl_long) != SH_SUCCESS)
                          {
                          lcl_status = SH_INV_FITS_FILE;
                          shErrStackPush ("Invalid integer '%.*s' in ASCII Table row %d, field %d",
                                           lcl_tblFld->prvt.TFORM.ascFldWidth, lcl_fldBeg, lcl_rowIdx, lcl_fldIdx);
                          }
                       else
                          {
                          switch (lcl_datumSize)
                             {
                             case sizeof (S32) : *((S32 *)lcl_datum) = ((S32)lcl_long); break;
                             case sizeof (S16) : *((S16 *)lcl_datum) = ((S16)lcl_long); break;
                             case sizeof (S8)  : *((S8  *)lcl_datum) = ((S8) lcl_long); break;
                             default : lcl_status = SH_INTERNAL_ERR;
                                       shErrStackPush ("Can't cast long int to TBLFLD data area (%s line %d)", __FILE__, __LINE__);
                                       shFatal        ("Can't cast long int to TBLFLD data area (%s line %d)", __FILE__, __LINE__);
                                       goto rtn_return;
                                       break;
                             }
                          }
                       break;
            case 'F' :
            case 'E' :
            case 'D' :  if (s_shFitsTBLCOLreadAscReal (lcl_fldBeg, lcl_tblFld->prvt.TFORM.ascFldWidth,
                                                                   lcl_tblFld->prvt.TFORM.ascFldDecPt, &lcl_double) != SH_SUCCESS)
                          {
                          lcl_status = SH_INV_FITS_FILE;
                          shErrStackPush ("Invalid real number '%.*s' in ASCII Table row %d, field %d",
                                           lcl_tblFld->prvt.TFORM.ascFldWidth, lcl_fldBeg, lcl_rowIdx, lcl_fldIdx);
                          }
                       else
                          {
                          switch (lcl_datumSize)
                             {
                             case sizeof (double) : *((double *)lcl_datum) =         lcl_double;  break;
                             case sizeof (float)  : *((float  *)lcl_datum) = shNaNDoubleTest(lcl_double) ? 
						                             shNaNFloat() : ((float)lcl_double); break;
                             default : lcl_status = SH_INTERNAL_ERR;
                                       shErrStackPush ("Don't know how to cast double to TBLFLD data area (%s line %d)",
                                                        __FILE__, __LINE__);
                                       shFatal        ("Don't know how to cast double to TBLFLD data area (%s line %d)",
                                                        __FILE__, __LINE__);
                                       goto rtn_return;
                                       break;
                             }
                          }
                       break;

            default  :
                       lcl_status = SH_INTERNAL_ERR;
                       shErrStackPush ("ASCII Table data type '%c' not handled (%s line %d)", lcl_tblFld->prvt.TFORM.ascFldFmt,
                                        __FILE__, __LINE__);
                       shFatal        ("ASCII Table data type '%c' not handled (%s line %d)", lcl_tblFld->prvt.TFORM.ascFldFmt,
                                        __FILE__, __LINE__);
                       goto rtn_return;
                       break;
            }
         lcl_tblFld->prvt.datumPtr = lcl_datum + lcl_datumIncr;
         }
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    */

rtn_return : ;

   /*
    * Push the file position to the start of the next FITS HDU.
    *
    *   o   dataAreaSize is assumed to be a multiple of shFitsRecSize.
    */

   if (p_shFitsSeek (FITSfp, lcl_FITSfpRead, dataAreaSize) != SH_SUCCESS)
      {
      lcl_status = SH_INV_FITS_FILE;
      if (!lcl_FITSfpEOF) { shErrStackPush ("End of file or read error"); }
      p_shFitsLRecMultiple (FITSfp);
      shErrStackPush ("FITS file too short");
      }

   if (lcl_buf != ((char *)0)) { shFree (lcl_buf); }

   return (lcl_status);

   }	/* s_shFitsTBLCOLreadAsc */

/******************************************************************************/

static RET_CODE	 s_shFitsTBLCOLreadAscInt
   (
                  char	*buf,		/* IN:    Input buffer                */
                  int	 fldWidth,	/* IN:    Field width                 */
            long  int	*outLong	/* OUT:   Resultant integer           */
   )

/*++
 * ROUTINE:	 s_shFitsTBLCOLreadAscInt
 *
 *	Read an ASCII encoded integer value.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_INVARG		Failure.
 *			The integer value in buf is invalid.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	The buffer must be at least fldWidth + 1 characters in length.
 *
 *	   o	The field in the buffer is temporarily null terminated.  This
 *		may affect other asynchronous tasks that use buf.
 *
 *	   o	Embedded blanks in the field (after a sign indicator (+/-)) may
 *		be converted to zeros (0). This permits FORTRAN 77 compliance.
 *		This may affect other asynchronous tsks that use buf.
 *--
 ******************************************************************************/

   {	/* s_shFitsTBLCOLreadAscInt */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
                  char	 lcl_charSav;
                  char	*lcl_buf;
                  char	*lcl_bufPtr;
                  int	 lcl_spaceCnt;

static            char	*lcl_space = " \011";	/* White space set            */

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Convert the ASCII representation to a signed integer.
    */

   lcl_charSav = buf[fldWidth];
                 buf[fldWidth] = ((char)0);  	/* Null terminate             */

   lcl_spaceCnt = strspn (buf, lcl_space);	/* Find first non-whitespace  */

   if (lcl_spaceCnt == fldWidth)
      {
      *outLong = 0;				/* All blanks equiv. to zero  */
      goto rtn_return;
      }

   lcl_buf = &buf[lcl_spaceCnt];

   if ((*lcl_buf == '-') || (*lcl_buf == '+'))	/* Fill embedded blanks as    */
      {						/* ... zeros (0).             */
      if ((lcl_spaceCnt = strspn (&lcl_buf[1], lcl_space)) != 0)
         {
         memset (((void *)&lcl_buf[1]), '0', lcl_spaceCnt);
         }
      }

   *outLong    = strtol (buf, &lcl_bufPtr, 10);
   if (lcl_bufPtr == buf)
      {
      lcl_status = SH_INVARG;			/* Don't understand number    */
      }

   /*
    * All done.
    */

rtn_return : ;
                 buf[fldWidth] = lcl_charSav;	/* Restore                    */

   return (lcl_status);

   }	/* s_shFitsTBLCOLreadAscInt */

/******************************************************************************/

static RET_CODE	 s_shFitsTBLCOLreadAscReal
   (
                  char	*buf,		/* IN:    Input buffer                */
                  int	 fldWidth,	/* IN:    Field width                 */
                  int	 decPt,		/* IN:    Decimal point position      */
   double		*outDouble	/* OUT:   Resultant double            */
   )

/*++
 * ROUTINE:	 s_shFitsTBLCOLreadAscReal
 *
 *	Read an ASCII encoded real value.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_INVARG		Failure.
 *			The real value in buf is invalid.
 *
 *   SH_MALLOC_ERR	Failure.
 *			Space could not be allocated for the temporary string to
 *			hold the number representation with the implicit decimal
 *			point put in explicitly.
 *
 * SIDE EFFECTS:
 *
 *   o	If an implicit decimal point needs to placed into the string explictly,
 *	the resulting character representation of the real number may be dif-
 *	ferent, especially when the original representation (buf) is invalid
 *	(when it contains embedded blanks (not leading or trailing blanks));
 *	the resulting representation with an explicit decimal point may become
 *	valid.
 *
 *   o	Fields starting with "NaN" (after any initial whitespace) are treated as
 *	containing an IEEE floating-point quiet (non-signalling) Not-a-Number.
 *
 * SYSTEM CONCERNS:
 *
 *   o	The buffer must be at least fldWidth + 1 characters in length.
 *
 *	   o	The field in the buffer is temporarily null terminated.  This
 *		may affect other asynchronous tasks that use buf.
 *
 *	   o	Embedded blanks in the field (after a sign indicator (+/-)) may
 *		be converted to zeros (0). This permits FORTRAN 77 compliance.
 *		This may affect other asynchronous tsks that use buf.
 *--
 ******************************************************************************/

   {	/* s_shFitsTBLCOLreadAscReal */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
                  char	 lcl_charSav;
                  char	*lcl_buf;
                  char	*lcl_bufPtr;
                  int	 lcl_bufAllocSize;
                  char	*lcl_bufAlloc = ((char *)0);
                  int	 lcl_spaceCnt;
                  int	 lcl_digitCnt;
                  int	 lcl_minus = 0;
                  int	 lcl_leadZero;

static            char	*lcl_space = " \011";	/* White space set            */

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Check whether an explicit decimal point is provided.
    *
    *   o   If the field contains neither a decimal point nor an exponent, it is
    *       treated as a real number of fldWidth digits, in which the rightmost
    *       decPt digits are to the right of the (implicit) decimal point, with
    *       leading zeros assumed, if necessary.
    *
    *   o   If the field contains a decimal point or an exponent, any embedded
    *       blanks are converted to zeros (0).  This permits strtod to work.
    */

   lcl_charSav = buf[fldWidth];
                 buf[fldWidth] = ((char)0);	/* Null terminate             */

   lcl_spaceCnt = strspn (buf, lcl_space);	/* Find first non-whitespace  */

   if (lcl_spaceCnt == fldWidth)
      {
      *outDouble = 0.0;				/* All blanks equiv. to zero  */
      goto rtn_return;
      }

   if (shStrMatch (&buf[lcl_spaceCnt], "NaN*", shStrWildSet, SH_CASE_INSENSITIVE) == SH_SUCCESS)
      {
      *outDouble = shNaNDouble ();		/* Hit an NaN                 */
      goto rtn_return;
      }

   if ((lcl_bufPtr = strpbrk (&buf[lcl_spaceCnt], ".EeDd")) != ((char *)0))
      {
      lcl_buf      = &buf[lcl_spaceCnt];	/* Tmp to find D/d specifier  */
      if (*lcl_bufPtr == '.')			/* Convert D/d specifier to   */
         {					/* E, as C doesn't understand */
           lcl_bufPtr  = strpbrk (lcl_buf, "Dd"); /* D/d format specifiers.   */
	 }
      if ( lcl_bufPtr != ((char *)0)) if (*lcl_bufPtr != '.')
	 {
          *lcl_bufPtr  = 'E';
         }
      if ((*lcl_buf == '-') || (*lcl_buf == '+')) /* Fill embedded blanks as  */
         {					  /* ... zeros (0).           */
         if ((lcl_spaceCnt = strspn (&lcl_buf[1], lcl_space)) != 0)
            {
            memset (((void *)&lcl_buf[1]), '0', lcl_spaceCnt);
            }
         }
      }
   else
      {
      lcl_bufAllocSize = ((fldWidth > decPt) ? fldWidth : decPt) + 1 /* '.' */ + 1 /* '-' */ + 1;
      if ((lcl_buf = lcl_bufAlloc = ((char *)alloca (lcl_bufAllocSize))) == ((char *)0))
         {
         lcl_status = SH_MALLOC_ERR;
         shErrStackPush ("Unable to allocate temporary space for putting in decimal point");
         goto rtn_return;
         }
      switch (buf[lcl_spaceCnt])
	{
	case '-' : 
	case '+' :
	  lcl_spaceCnt++;		/* Skip explict sign control  */
	  if(buf[lcl_spaceCnt] == '-') {
	    lcl_minus = 1;		/* Show negative number       */
	  }	   
	  break;
	default  :
	  break;
	}
      lcl_bufPtr = lcl_bufAlloc;		/* Can't touch lcl_bufAlloc   */
      if (lcl_minus) { *lcl_bufPtr++ = '-'; }	/* Put in explict minus sign  */
      if ((lcl_leadZero = (decPt - (lcl_digitCnt = strcspn (&buf[lcl_spaceCnt], lcl_space)))) > 0)
         {
                          *lcl_bufPtr++ = '.';	/* Implicit decimal point     */
         memset  (((void *)lcl_bufPtr), '0', lcl_leadZero); /* Leading zeros  */
                           lcl_bufPtr  +=    lcl_leadZero;
         strncpy (         lcl_bufPtr, &buf[lcl_spaceCnt], lcl_digitCnt);
                           lcl_bufPtr[lcl_digitCnt] = 0;    /* Null terminate */
         }
      else
         {
         strncpy (lcl_bufPtr, &buf[lcl_spaceCnt], -lcl_leadZero);
                  lcl_bufPtr  +=                  -lcl_leadZero;
                 *lcl_bufPtr++ = '.';		/* Implicit decimal point     */
         strncpy (lcl_bufPtr, &buf[lcl_spaceCnt + -lcl_leadZero], decPt);
                  lcl_bufPtr[lcl_digitCnt + 1] = 0;	/* Null terminate     */
         }
      }

   /*
    * Explicit decimal point is available now.
    *
    *   o   A value of all blanks is considered to be 0.0.
    */

   *outDouble  = strtod (lcl_buf, &lcl_bufPtr);
   if (lcl_bufPtr == lcl_buf)
      {
      lcl_status = SH_INVARG;			/* Don't understand number    */
      }

   /*
    * All done.
    */

rtn_return : ;
                 buf[fldWidth] = lcl_charSav;	/* Restore                    */

   return (lcl_status);

   }	/* s_shFitsTBLCOLreadAscReal */

/******************************************************************************/

static RET_CODE	 s_shFitsTBLCOLreadBin
   (
   TBLCOL		 *tblCol,	/* INOUT: Table handle                */
   FILE			 *FITSfp,	/* INOUT: FITS file file pointer      */
                  int	  TFIELDS,	/* IN:    Number of Table fields      */
                  int	  rowSize,	/* IN:    Size of a Table row (bytes) */
   SHFITSSWAP		  datumSwap,	/* IN:    Swap datum bytes?           */
   unsigned long  int	  dataAreaSize,	/* IN:    Data area size (w/ padding) */
   unsigned long  int	  rsaSize,	/* IN:    Record Storage Area size    */
   unsigned long  int	  heapOff,	/* IN:    Heap area offset            */
   unsigned long  int	  heapSize	/* IN:    Heap area size              */
   )

/*++
 * ROUTINE:	 s_shFitsTBLCOLreadBin
 *
 *	Read Binary Table data into a TBLCOL object schema.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_MALLOC_ERR	Failure.
 *			Space could not be allocated for reading a Table row
 *			or the heap area.
 *
 *   SH_INV_FITS_FILE	Failure.
 *			The data area was not large enough or improperly
 *			blocked (into shFitsRecSize chunks).  The TBLCOL
 *			data areas may be partially filled.
 *
 * SIDE EFFECTS:
 *
 *   o	Data is read from the FITS file.  This changes the file position.  If
 *	the read is successful, the file will be positioned at the start of the
 *	next HDU (or EOF).  Otherwise, the file position is indeterminate.
 *
 * SYSTEM CONCERNS:
 *
 *   o	The HDU must have already been checked to make sure that a Binary Table
 *	extension is being read.  The FITS file must be positioned at the first
 *	byte of data in the HDU.
 *
 *   o	The size of row data computed from TFORMns must not be larger than
 *	NAXIS1.
 *
 *   o	Reading from the FITS file is done sequentially.  This allows for a
 *	more efficient read from magnetic tapes, as there will be no bouncing
 *	around the tape file (if the file is large enough to need buffering
 *	into more than one buffer by the I/O system).
 *--
 ******************************************************************************/

   {	/* s_shFitsTBLCOLreadBin */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
   ARRAY		*lcl_array;
   TBLFLD		*lcl_tblFld;		/* TBLFLD structure           */
                  int	 lcl_rowIdx;		/* RSA record/row index       */
                  int	 lcl_fldIdx;		/* Field index (0-indexed)    */
                  int	 lcl_elemIdx;
                  int	 lcl_datumSize;		/* Size of datum in bytes     */
                  int	 lcl_datumIncr;		/* Space between datums: bytes*/
                  int	 lcl_datumIdx;		/* Byte index into datum      */
                  int	 lcl_datumIdxIni;	/* Initial value              */
                  int	 lcl_datumCtr;		/* Counter for copying over   */
   unsigned       char	*lcl_datum;
   unsigned       char	 lcl_datumSignFlip;	/* XOR mask for sign flip     */
                  int	 lcl_fldElemCnt;	/* Number primitive types per */
						/* lcl_array->data.schemaType */
   TBLHEAPDSC		*lcl_heapDsc;   
            long  int	 lcl_heapCntAll;
            long  int	 lcl_heapCntMax;
            long  int	 lcl_heapNeeded;
                  int	 lcl_heapPtrSizeDiff = (sizeof (unsigned char *) - sizeof (int));
						/* Size diff btwn 4 byte heap */
						/* offset (int) and in-memory */
						/* pointer. Some code assumes */
						/* this is NEVER negative.### */
                  int	 lcl_heapDscPadBeg = offsetof (TBLHEAPDSC, cnt);
                  int	 lcl_heapDscPadMid = offsetof (TBLHEAPDSC, ptr) - lcl_heapDscPadBeg - sizeof (lcl_heapDsc->cnt);
                  int	 lcl_heapDscPadEnd = sizeof   (TBLHEAPDSC)      - lcl_heapDscPadBeg - sizeof (lcl_heapDsc->cnt)
			                                                - lcl_heapDscPadMid - sizeof (lcl_heapDsc->ptr);
#if (UINT_MAX != U32_MAX)
#error 'P' heap descriptor heap offset field   M U S T   be 4 bytes in size
#endif

                  char	*lcl_buf = ((char *)0);	/* One Table row              */
   size_t		 lcl_bufLen;
   unsigned long  int	 lcl_bufIdx;		/* Index into lcl_buf         */
            long  int	 lcl_FITSfpRead = 0;	/* Amount of bytes read       */
                  int	 lcl_FITSfpEOF  = FALSE; /* EOF detected?             */

   TYPE			 lcl_schemaTypeSTR        = shTypeGetFromName ("STR");		/* For performance */
   TYPE			 lcl_schemaTypeLOGICAL    = shTypeGetFromName ("LOGICAL");	/* For performance */
   TYPE			 lcl_schemaTypeTBLHEAPDSC = shTypeGetFromName ("TBLHEAPDSC");	/* For performance */
   TYPE			 lcl_schemaTypeUNKNOWN    = shTypeGetFromName ("UNKNOWN");	/* For performance */

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Correct arguments:
    *
    *   o   dataAreaSize is assumed to be a multiple of shFitsRecSize.
    */

   if ((lcl_bufLen  = dataAreaSize % (shFitsRecSize)) != 0)
      {
      dataAreaSize += ((shFitsRecSize) - lcl_bufLen);
      }

   /*
    * Read in Record Storage Area (RSA) data from the TBLFLD fields from the FITS
    * file.
    *
    *   o   This reading is very simple (not even double buffered).  Still, it
    *       is quite a bit more efficient that the LIBFITS f_getXX () routines,
    *       since those only work well when reading in large amounts of data
    *       (and they're also only single buffered).  Our reading of different
    *       data types doesn't permit effective use of f_getXX () routines.
    *       The f_getXX () routines also do not take alignment into consider-
    *       ation.
    *
    *       Space is allocated to hold one record (plus an additional character
    *       to let fields be null-terminated safely).
    *
    *   o   datumSwap and lcl_datumSize/Ctr/Idx are used to implement byte swap-
    *       ping (if it's needed) without resorting to too many conditionals.
    *
    *   o   'A' ("STR") data types do not need any byte swapping.
    *
    *   o   If an invalid value is detected, reading is  N O T  aborted immed-
    *       iately.
    */

   if ((tblCol->rowCnt > 0) && (rowSize > 0))
      {
      if ((lcl_buf = (char*)shMalloc (rowSize)) == ((char *)0))
         {
         lcl_status = SH_MALLOC_ERR;
         shErrStackPush ("Unable to allocate space for reading Binary Table row");
         goto rtn_return;
         }
      }

   for (lcl_rowIdx = 0; lcl_rowIdx < tblCol->rowCnt; lcl_rowIdx++)
      {
      lcl_FITSfpRead += (lcl_bufLen = fread (lcl_buf, 1, rowSize, FITSfp));
      if (lcl_bufLen != rowSize)
         {
         lcl_status = SH_INV_FITS_FILE;
         if (lcl_bufLen != 0) { shErrStackPush ("End of file detected"); lcl_FITSfpEOF = TRUE; }
                        else  { shErrStackPush ("Read error"); }
         goto rtn_return;
         }
      lcl_bufIdx = 0;

      for (lcl_fldIdx = 0; ((lcl_fldIdx < shChainSize(&(tblCol->fld))) && (lcl_fldIdx < TFIELDS)); lcl_fldIdx++)
         {
         lcl_array = (ARRAY *) shChainElementGetByPos(&(tblCol->fld), lcl_fldIdx);

         shAssert ((lcl_tblFld = ((TBLFLD *)lcl_array->info)) != ((TBLFLD *)0)); /* Ok cast */

         lcl_datum         = lcl_tblFld->prvt.datumPtr;
         lcl_datumSize     = lcl_array->data.size;
         lcl_datumIncr     = lcl_array->data.incr;
         lcl_fldElemCnt    = 1;	/* Assume 1 elem per array->data.schemaType   */
         lcl_datumSignFlip = lcl_tblFld->prvt.signFlip;

              if (lcl_array->data.schemaType == lcl_schemaTypeSTR)
            {
            /*
             * Character strings (data type 'A' and object schema type "STR")
             * require special handling. Dervish allocates an additional character
             * per string to guarantee space for a null terminator.  tblFld->
             * prvt.schemaCnt, tblFld->prvt.TFORM.ascFldWidth and tblFld->
             * array->dim[...->array->dimCnt] reflect these additional charac-
             * ters.
             */

            for (lcl_elemIdx = lcl_tblFld->prvt.schemaCnt; lcl_elemIdx > 0; lcl_elemIdx--)
               {
               for (lcl_datumCtr = lcl_array->dim[lcl_array->dimCnt - 1] - 1; lcl_datumCtr > 0;
                    lcl_datumCtr--)		/*                       `-'                            */
                   {				/*                        `--> Extra char is NOT in RSA */
                   *lcl_datum  = lcl_buf[lcl_bufIdx++];
                    lcl_datum += lcl_datumIncr;
                   }
                *lcl_datum  = 0;			/* Null terminate     */
                 lcl_datum += lcl_datumIncr;		/* ... explicitly.    */
               }
            }
         else if (lcl_array->data.schemaType == lcl_schemaTypeLOGICAL)
            {
            /*
             * LOGICAL fields are converted from a 1-byte 'T' or 'F' to a
             * respective 1 or 0 integer.
             *								  ###
             * NOTE:  LOGICALs are assumed to be 1 character in size.	  ###
             *        This assumption is "protected" by calling s_shFits- ###
             *        PlatformCheck () by shFitsRead ().		  ###
             */

            for (lcl_elemIdx = lcl_tblFld->prvt.schemaCnt; lcl_elemIdx > 0; lcl_elemIdx--)
               {
               switch (((char)lcl_buf[lcl_bufIdx++]))
                  {
                  case 'T' : case 't' : *((unsigned char *)lcl_datum) = 1; break;
                  case 'F' : case 'f' : *((unsigned char *)lcl_datum) = 0; break;
                  default  : lcl_status = SH_INV_FITS_FILE;
                             shErrStackPush ("Invalid logical value (TFORM%d data type 'L') in Binary Table row %d",
                                              lcl_fldIdx, lcl_rowIdx);
                             break;
                  }
               lcl_datum += lcl_datumIncr;
               }
            }
         else if (lcl_array->data.schemaType == lcl_schemaTypeTBLHEAPDSC)
            {
            /*
             * Read in a heap descriptor (for a variable length array).
             *
             * NOTE:  TBLHEAPDSC is composed of 2 members, a count and a heap
             *        offset.  Both are 4 byte integers in the BINTABLE. But,
             *        within TBLHEAPDSC, the offset is contained in a pointer
             *        which may be a different size then 4 bytes (the offset
             *        is converted to a pointer later).
             *
             *           o   lcl_datumSize/Incr are overridden from their ARRAY
             *               information, as that may not necessarily match the
             *               information in the BINTABLE.
             */

            lcl_datumSize   = shAlignment.type[SHALIGNTYPE_S32].size;
            lcl_datumIncr   = shAlignment.type[SHALIGNTYPE_S32].incr;

            lcl_datumIdxIni = (datumSwap == SH_FITS_NOSWAP) ? 0 : lcl_datumSize - 1;

            for (lcl_elemIdx = lcl_tblFld->prvt.schemaCnt; lcl_elemIdx > 0; lcl_elemIdx--)
               {
               /*
                * Read in the heap element count.
                */

               for (lcl_datumIdx = lcl_heapDscPadBeg; lcl_datumIdx > 0; lcl_datumIdx--)
                  {
                  *lcl_datum++ = 0;		/* Inter-member padding       */
                  }

               lcl_heapDsc = ((TBLHEAPDSC *)lcl_datum);

               for (lcl_datumIdx  = lcl_datumIdxIni, lcl_datumCtr = lcl_datumSize; lcl_datumCtr > 0;
                    lcl_datumIdx +=     datumSwap,   lcl_datumCtr--)
                  {
                  lcl_datum[lcl_datumIdx] = lcl_buf[lcl_bufIdx++]; /* Fill datum byte by byte */
                  }
               lcl_datum += lcl_datumIncr;

               /*
                * Read in the heap offset.
                *
                *   o   This code is conditionally compiled for the case where
                *       the BINTABLE heap offset size (4 bytes) does not match
                *       the TBLHEAPDSC ptr field size.  The TBLHEAPDSC pointer
                *       MUST be the size of the heap offset (4 bytes) or larger.
                */

               for (lcl_datumIdx = lcl_heapDscPadMid; lcl_datumIdx > 0; lcl_datumIdx--)
                  {
                  *lcl_datum++ = 0;		/* Inter-member padding       */
                  }

               #if defined(SDSS_BIG_ENDIAN)
               for (lcl_datumIdx = 0; lcl_datumIdx < lcl_heapPtrSizeDiff; lcl_datumIdx++)
                  {
                  *lcl_datum++ = 0;		/* Pad offset w/ lead  zeros  */
                  }
               #endif

               for (lcl_datumIdx  = lcl_datumIdxIni, lcl_datumCtr = lcl_datumSize; lcl_datumCtr > 0;
                    lcl_datumIdx +=     datumSwap,   lcl_datumCtr--)
                  {
                  lcl_datum[lcl_datumIdx] = lcl_buf[lcl_bufIdx++]; /* Fill datum byte by byte */
                  }
               lcl_datum += lcl_datumIncr;

               #if SDSS_LITTLE_ENDIAN
               for (lcl_datumIdx = 0; lcl_datumIdx < lcl_heapPtrSizeDiff; lcl_datumIdx++)
                  {
                  *lcl_datum++ = 0;		/* Pad offset w/ trail zeros  */
                  }
               #endif


               for (lcl_datumIdx = lcl_heapDscPadEnd; lcl_datumIdx > 0; lcl_datumIdx--)
                  {
                  *lcl_datum++ = 0;		/* Inter-struct padding       */
                  }

               /*
                * The heap descriptor is checked for a legal count and offset
                * (both must be positive).  The offset is contained in the ptr
                * member of TBLHEAPDSC.
                *
                *   o   The offset is   N O T  completely checked, as its bounds
                *       may lie outside the heap (or a combination of the offset
                *       and count may lie outside the heap area).
                *
                *   o   If the heap data type is a character string ('A' data
                *       type or "STR" object schema), then the array length
                *       (count) is bumped up by one to guarantee space for
                *       null termination.
                */

               if (  (lcl_heapDsc->cnt < 0)
                 || ((lcl_heapDsc->cnt > 0) && (((long  int)lcl_heapDsc->ptr) < 0)) )				/*### */
                  {
                  lcl_status = SH_INV_FITS_FILE;

                  shErrStackPush ("Invalid heap array descriptor in Binary Table row %d, field %d", lcl_rowIdx, lcl_fldIdx);
                  }
               if ((lcl_heapDsc->cnt > 0) && ((((long  int)lcl_heapDsc->ptr) + lcl_heapDsc->cnt) > heapSize))	/*### */
                  {
                  lcl_status = SH_INV_FITS_FILE;
                  shErrStackPush ("Invalid heap (array descriptor extends beyond heap) in Binary Table row %d, field %d",
                                   lcl_rowIdx, lcl_fldIdx);
                  }
               if (lcl_tblFld->heap.schemaType == lcl_schemaTypeSTR)
                  {
                  lcl_heapDsc->cnt++;		/* Guarantee null terminator  */
                  }
               }
            }
         else
            {
            /*
             * A data type that is NOT to be adjusted during reading.
             *								  ###
             * NOTE:  The use of lcl_fldElemCnt is retained for the case  ###
             *        where other data types, such a complex numbers,	  ###
             *        will be supported in the future.			  ###
             */

/*###*	            if (lcl_tblFld->heap.schemaType == lcl_schemaTypeCOMPLEX)
 *###*	               {
 *###*	               lcl_fldElemCnt  = 2;  *###*/ /* # elems/schemaType: Hardwired     ### */
/*###*	               lcl_datumSize  /= lcl_fldElemCnt;	*###*/ /*  Hardwired     ### */
/*###*	               lcl_datumIncr  /= lcl_fldElemCnt;	*###*/ /*  Hardwired     ### */
/*###*	               }
 *###*/

            lcl_datumIdxIni = (datumSwap == SH_FITS_NOSWAP) ? 0 : lcl_datumSize - 1;

            for (lcl_elemIdx = (lcl_tblFld->prvt.schemaCnt * lcl_fldElemCnt); lcl_elemIdx > 0; lcl_elemIdx--)
               {
               for (lcl_datumIdx  = lcl_datumIdxIni, lcl_datumCtr = lcl_datumSize; lcl_datumCtr > 0;
                    lcl_datumIdx +=     datumSwap,   lcl_datumCtr--)
                  {
                  lcl_datum[lcl_datumIdx] = lcl_buf[lcl_bufIdx++]; /* Fill datum byte by byte */
                  }
               lcl_datum[lcl_datumIdxIni] ^= lcl_datumSignFlip;	/* Flip sign? */
               lcl_datum += lcl_datumIncr;	/* Onto next datum in memory  */
               }
            }

         /*
          * Done with data type-specific reading.
          */

         lcl_tblFld->prvt.datumPtr = lcl_datum;	/* Save for next row          */
         }
      }

   if (lcl_status != SH_SUCCESS)
      {
      goto rtn_return;
      }

   /* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */

   /* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */
   /*
    * Read in heap data, if any.
    *
    *   o   The reading is simplistic.  Space for the entire heap area (in the
    *       HDU) is allocated and one read is performed.  Then, memory bounces
    *       are used to read out the heap area into the TBLFLD heap data areas.
    *       This is done for two reasons; 1) sequential reads from the file are
    *       inadequate since the heap descriptors ('P' data types) can reference
    *       heap randomly, and 2) fseek ()s in the file can be very slow and
    *       might not even work for sequential devices.
    */

   if (heapSize <= 0)
      {
      goto rtn_return;				/* Nothing to read            */
      }

   /*
    * Position the file to the start of the heap area.
    *
    *   o   This is done first, since if it fails, memory fragmentation can be
    *       possibly avoided by not allocating and then immediately freeing
    *       memory.
    */

   if (p_shFitsSeek (FITSfp, lcl_FITSfpRead, heapOff) != SH_SUCCESS)
      {
      lcl_status = SH_INV_FITS_FILE;
      if (!lcl_FITSfpEOF) { shErrStackPush ("End of file or read error"); }
      p_shFitsLRecMultiple (FITSfp);
      shErrStackPush ("FITS file too short");
      shErrStackPush ("Heap could not be located");
      goto rtn_return;
      }
   lcl_FITSfpRead +=  (heapOff - lcl_FITSfpRead);	/* Show we've skipped */

   /*
    * Allocate space for the data from the Heap Area.
    *
    *   o   Space allocation includes any element padding.
    *
    *   o   Character strings (object schema type of "STR") are extended by one
    *       character (just as RSA character strings) to guarantee space for
    *       null-termination.  This extension was done by s_shFitsTBLCOLreadBin
    *       when reading in the heap array descriptors (TBLHEAPDSC).
    *
    *   o   Unknown data types are handled specially.  Additional space is allo-
    *       cated for alignment space between rows.
    */

   lcl_heapNeeded = 0;			/* Though heap is present in the HDU, */
					/* maybe none of the heap descriptors */
					/* indicate its use.                  */

   for (lcl_fldIdx = 0; ((lcl_fldIdx < shChainSize(&(tblCol->fld))) && (lcl_fldIdx < TFIELDS)); lcl_fldIdx++)
      {
      lcl_array = shChainElementGetByPos(&(tblCol->fld), lcl_fldIdx);

      shAssert ((lcl_tblFld = ((TBLFLD *)lcl_array->info)) != ((TBLFLD *)0)); /* shTblFldGen should have done this */

      lcl_tblFld->heap.dataPtr = ((unsigned char *)0);		/* For safety */

      if (!shInSet (lcl_tblFld->Tpres, SH_TBL_HEAP))
         {
         continue;				/* Onto next field            */
         }

      p_shTblFldHeapCnt (lcl_array, ((long int *)0), &lcl_heapCntAll, &lcl_heapCntMax); /* RSA reader did more complete checks */

      if (lcl_heapCntAll > 0)
         {
         if ((lcl_tblFld->heap.dataPtr =
               ((unsigned char *)shMalloc (  (lcl_tblFld->heap.incr  * lcl_heapCntAll)
                                           + (lcl_tblFld->heap.align *
                                             (lcl_tblFld->heap.schemaType == lcl_schemaTypeUNKNOWN) ? tblCol->rowCnt : 1
             ) )                          )  ) == ((unsigned char *)0))
            {
            lcl_status = SH_MALLOC_ERR;
            shErrStackPush ("Unable to allocate space for Table field heap data");
            goto rtn_return;
            }
         lcl_heapNeeded = 1;			/* Heap used through RSA      */
         }
      }

   if (!lcl_heapNeeded)
      {
      goto rtn_return;				/* Nothing to read in         */
      }

   /*
    * Get space for reading and read in the heap.
    */

   if (heapSize > rowSize)
      {
      if ( lcl_buf != ((char *)0)) { shFree (lcl_buf); }

      if ((lcl_buf  = (char*)shMalloc (heapSize)) == ((char *)0))
         {
         lcl_status = SH_MALLOC_ERR;
         shErrStackPush ("Unable to allocate space for reading Binary Table heap");
         goto rtn_return;
         }
      }

   lcl_FITSfpRead += (lcl_bufLen = fread (lcl_buf, 1, heapSize, FITSfp));

   if (lcl_bufLen != heapSize)
      {
      lcl_status = SH_INV_FITS_FILE;
      if (lcl_bufLen != 0) { shErrStackPush ("End of file detected"); lcl_FITSfpEOF = TRUE; }
                     else  { shErrStackPush ("Read error"); }
      goto rtn_return;
      }

   /*
    * Move the heap area data to the TBLFLD heap data areas.
    *
    *   o   Each field is done completely (all rows) before moving onto the next
    *       field.
    *
    *   o   The TBLHEAPDSC descriptors are modified. The pointer (ptr) field is
    *       converted from the heap offset (what was read in) to the address of
    *       the heap data.
    *
    *          o   If the heap count is zero for non-STR data, the heap pointer
    *              is set to a null address.  STR data will always have at least
    *              one character for the null-terminator (reflected in the heap
    *              count).
    */

   for (lcl_fldIdx = 0; ((lcl_fldIdx < shChainSize(&(tblCol->fld))) && (lcl_fldIdx < TFIELDS)); lcl_fldIdx++)
      {
      lcl_array = (ARRAY *) shChainElementGetByPos(&(tblCol->fld), lcl_fldIdx);

      shAssert ((lcl_tblFld = ((TBLFLD *)lcl_array->info)) != ((TBLFLD *)0)); /* Ok cast */

      if (!shInSet (lcl_tblFld->Tpres, SH_TBL_HEAP))
         {
         continue;				/* Onto next field            */
         }

      lcl_datum = lcl_tblFld->heap.dataPtr;

      if (lcl_tblFld->heap.schemaType == lcl_schemaTypeSTR)
         {
         /*
          * Character strings (data type 'A' and object schema type "STR")
          * require special handling. Dervish allocates an additional character
          * per string to guarantee space for a null terminator.  The heap
          * descriptor (element count) reflects this additional character.
          */

         lcl_datumIncr   = lcl_tblFld->heap.incr;

         for (lcl_rowIdx = 0; lcl_rowIdx < tblCol->rowCnt; lcl_rowIdx++)
            {
            lcl_heapDsc = &((TBLHEAPDSC *)lcl_array->data.dataPtr)[lcl_rowIdx];
            lcl_bufIdx  = ((long int)lcl_heapDsc->ptr);
                                     lcl_heapDsc->ptr = lcl_datum;

            for (lcl_datumCtr = lcl_heapDsc->cnt - 1; lcl_datumCtr > 0; lcl_datumCtr--)
                {		/* Extra char is NOT in heap <-----'          */
                *lcl_datum  = lcl_buf[lcl_bufIdx++];
                 lcl_datum += lcl_datumIncr;
                }
            *lcl_datum  = 0;			/* Null terminate explicitly  */
             lcl_datum += lcl_datumIncr;
            }
         }
      else
         {
         /*
          * A data type that is NOT to be adjusted during reading.
          *
          *   o   LOGICAL falls into here naturally.
          *
          * NOTE: lcl_fldElemCnt is retained and used, even though none of the
          *       (Dervish) supported data types will set it to a non-one value.
          *       value. But, when Dervish starts supporting the complex numbers,
          *       lcl_fldElemCnt will be needed.
          */

         lcl_datumSize     = lcl_tblFld->heap.size;
         lcl_datumIncr     = lcl_tblFld->heap.incr;
         lcl_fldElemCnt    = 1;	/* Assume 1 elem per tblFld->heap.schemaType  */
         lcl_datumIdxIni   = (datumSwap == SH_FITS_NOSWAP) ? 0 : lcl_datumSize - 1;
         lcl_datumSignFlip = lcl_tblFld->prvt.signFlip;

/*###*	            if (lcl_tblFld->heap.schemaType == lcl_schemaTypeCOMPLEX)
 *###*	               {
 *###*	               lcl_fldElemCnt  = 2;  *###*/ /* # elems/schemaType: Hardwired     ### */
/*###*	               lcl_datumSize  /= lcl_fldElemCnt;	*###*/ /*  Hardwired     ### */
/*###*	               lcl_datumIncr  /= lcl_fldElemCnt;	*###*/ /*  Hardwired     ### */
/*###*	               }
 *###*/

         for (lcl_rowIdx = 0; lcl_rowIdx < tblCol->rowCnt; lcl_rowIdx++)
            {
            lcl_heapDsc = &((TBLHEAPDSC *)lcl_array->data.dataPtr)[lcl_rowIdx];

            /*
             *   o   Unknown data types are handled specially.  Alignment is
             *       applied between each row.
             */

            if (lcl_tblFld->heap.schemaType == lcl_schemaTypeUNKNOWN)
               {
               lcl_datum = ((unsigned char *)p_shAlignUp (lcl_datum, lcl_tblFld->heap.align));
               }

            /*
             *   o   Update in-memory copy of heap array descriptor.
             */

            lcl_bufIdx  = ((long int)lcl_heapDsc->ptr);
                                     lcl_heapDsc->ptr = (lcl_heapDsc->cnt > 0) ? lcl_datum : ((unsigned char *)0);

            /*
             *   o   Move file heap data to in-memory copy (with possible byte
             *       swapping).
             */

            for (lcl_elemIdx = (lcl_heapDsc->cnt * lcl_fldElemCnt); lcl_elemIdx > 0; lcl_elemIdx--)
               {
               for (lcl_datumIdx  = lcl_datumIdxIni, lcl_datumCtr = lcl_datumSize; lcl_datumCtr > 0;
                    lcl_datumIdx +=     datumSwap,   lcl_datumCtr--)
                  {
                  /*
                   * Fill in datum, byte by byte.  This takes care of any
                   * alignment and byte swapping issues.
                   */

                  lcl_datum[lcl_datumIdx] = lcl_buf[lcl_bufIdx++];
                  }
               lcl_datum[lcl_datumIdxIni] ^= lcl_datumSignFlip;	/* Flip sign? */
               lcl_datum += lcl_datumIncr;	/* Onto next datum in memory  */
               }

            /*
             * Check legalities and correct any data.
             *
             *   o   These checks are similar to those performed on RSA data.
             */

            if (lcl_tblFld->heap.schemaType == lcl_schemaTypeLOGICAL)
               {
               for (lcl_elemIdx = (lcl_heapDsc->cnt * lcl_fldElemCnt); lcl_elemIdx > 0; lcl_elemIdx--)
                  {
                  switch (*(((char *)lcl_tblFld->prvt.datumPtr) - lcl_elemIdx))
                     {
                     case 'T' : case 't' : *(((unsigned char *)lcl_tblFld->prvt.datumPtr) - lcl_elemIdx) = 1; break;
                     case 'F' : case 'f' : *(((unsigned char *)lcl_tblFld->prvt.datumPtr) - lcl_elemIdx) = 0; break;
                     default  : lcl_status = SH_INV_FITS_FILE;
                                shErrStackPush ("Invalid logical value (TFORM%d data type 'L') in Binary Table heap (row %d)",
                                                 lcl_fldIdx, lcl_rowIdx);
                                break;
                     }
                  }
               }
            }
         }
      }

   /* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */
   /*
    * All done.
    */

rtn_return : ;

   /*
    * Push the file position:
    *
    *   o   to the start of the next FITS HDU if no heap is present.
    *   o   to the start of the heap area     if    heap is present.
    *
    *   o   dataAreaSize is assumed to be a multiple of shFitsRecSize.
    */

   if (p_shFitsSeek (FITSfp, lcl_FITSfpRead, dataAreaSize) != SH_SUCCESS)
      {
      lcl_status = SH_INV_FITS_FILE;
      if (!lcl_FITSfpEOF) { shErrStackPush ("End of file or read error"); }
      p_shFitsLRecMultiple (FITSfp);
      shErrStackPush ("FITS file too short");
      }

   if (lcl_buf != ((char *)0)) { shFree (lcl_buf); }

   return (lcl_status);

   }	/* s_shFitsTBLCOLreadBin */

/******************************************************************************/

static RET_CODE	 s_shFitsTBLCOLwrite
   (
   TBLCOL		 *tblCol,	/* IN:    Table handle                */
   FILE			 *FITSfp,	/* INOUT: FITS file file pointer      */
   FITSHDUTYPE		  hduType,	/* IN:    Type of HDU                 */
   SHFITSSWAP		  datumSwap,	/* IN:    Swap datum bytes?           */
   FITSFILETYPE		  FITSstd	/* IN:    Write FITS compliant?       */
   )

/*++
 * ROUTINE:	 s_shFitsTBLCOLwrite
 *
 *	Write a TBLCOL object schema to a FITS ASCII or Binary Table.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_NOT_SUPPORTED	Failure.
 *			The HDU type requested is not supported.
 *
 *   SH_INVOBJ		Failure.
 *			The TBLCOL Table is not legal.  The row counts between
 *			the fields do not match the TBLCOL row count.
 *
 * SIDE EFFECTS:
 *
 *   o	The file position is changed.  If the write is successful, the file
 *	file will be positioned at the end of the file, with appropriate
 *	filling to satisfy the FITS file standard.  Otherwise, the file
 *	position is indeterminate.
 *
 * SYSTEM CONCERNS:
 *
 *   o	The current file position is assumed to be correct (at a FITS record
 *	boundary).
 *
 *   o	Any FITS compliance and object schema checking should be reflected in
 *	shFitsTBLCOLcomply ().  The check for FITS compliance as reflected by
 *	the FITSstd parameter should have already been performed by the caller.
 *--
 ******************************************************************************/

   {	/* s_shFitsTBLCOLwrite */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
   ARRAY		*lcl_array;
                  int	 lcl_fldIdx;		/* Field index (0-indexed)    */

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Check whether the requested HDU type is supported.
    */

   switch (hduType)
      {
      case f_hduTABLE    :
      case f_hduBINTABLE :
                           break;

      case f_hduPrimary  : lcl_status = SH_NOT_SUPPORTED;
                           shErrStackPush ("TBLCOL object schemas cannot be written as the Primary HDU");
                           goto rtn_return;
                           break;

      case f_hduGroups   : lcl_status = SH_NOT_SUPPORTED;
                           shErrStackPush ("TBLCOL object schemas cannot be written as Random Groups HDUs");
                           goto rtn_return;
                           break;

      default            : lcl_status   = SH_NOT_SUPPORTED;
                           shErrStackPush ("TBLCOL object schemas cannot be written as unknown HDU types");
                           goto rtn_return;
                           break;
      }

   /*
    * Check some table legalities.
    *
    *   o   All fields must have the same number of rows, as specified in
    *       TBLCOL.
    */

   for (lcl_fldIdx = 0; lcl_fldIdx < shChainSize(&(tblCol->fld)); lcl_fldIdx++)
      {

      lcl_array = (ARRAY *) shChainElementGetByPos(&(tblCol->fld), lcl_fldIdx);

      if ((lcl_array->dimCnt <= 0) || (lcl_array->dim[0] != tblCol->rowCnt))
         {
         lcl_status = SH_INVOBJ;
         if (lcl_array->dimCnt <= 0)
            {
            shErrStackPush ("Table field %d does not have a row count to match Table row count (%d)", lcl_fldIdx, tblCol->rowCnt);
            }
            {
            shErrStackPush ("Table field %d row count (%d) does not match Table row count (%d)",      lcl_fldIdx,
                                                                                               lcl_array->dim[0], tblCol->rowCnt);
            }
         }
      }

   if (lcl_status != SH_SUCCESS)
      {
      goto rtn_return;
      }

   /*
    * Write data out to the FITS file.
    */

   switch (hduType)
      {
      case f_hduTABLE    : if ((lcl_status = s_shFitsTBLCOLwriteAsc (tblCol, FITSfp)) != SH_SUCCESS)
                              {
                              goto rtn_return;
                              }
                           break;
      case f_hduBINTABLE : if ((lcl_status = s_shFitsTBLCOLwriteBin (tblCol, FITSfp, datumSwap, FITSstd)) != SH_SUCCESS)
                              {
                              goto rtn_return;
                              }
                           break;

      default            : lcl_status = SH_INTERNAL_ERR;
                           shErrStackPush ("FITS HDU type not caught earlier (%s line %d)", __FILE__, __LINE__);
                           shFatal        ("FITS HDU type not caught earlier (%s line %d)", __FILE__, __LINE__);
                           goto rtn_return;
                           break;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    */

rtn_return : ;

   return (lcl_status);

   }	/* s_shFitsTBLCOLwrite */

/******************************************************************************/

static RET_CODE	 s_shFitsTBLCOLwriteAsc
   (
   TBLCOL		 *tblCol,	/* IN:    Table handle                */
   FILE			 *FITSfp	/* INOUT: FITS file file pointer      */
   )

/*++
 * ROUTINE:		 s_shFitsTBLCOLwriteAsc
 *
 *	Write a TBLCOL Table as a FITS TABLE ASCII Table extension.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_NOT_SUPPORTED	Failure.
 *			The ARRAY data type cannot be written to a ASCII Table.
 *
 *   SH_MALLOC_ERR	Failure.
 *
 * SIDE EFFECTS:
 *
 *   o	Data is written to the FITS file.  This changes the file position.  If
 *	the write is successful, the file will be positioned at EOF.  Otherwise,
 *	the file position is indeterminate.
 *
 *   o	No single numeric value, when converted to its character representation,
 *	may be larger than 255 characters.  If this does occur, data will be
 *	overwritten beyond the automatic variable lcl_num with unpredictable
 *	results.
 *
 *   o	If some per-field information is set, but not permitted by the data
 *	type, then that per-field information is NOT written out as a FITS
 *	header keyword.  For example, TSCALn and TZEROn are not permitted
 *	for `A' data types.  If these values are present (in the TBLFLD),
 *	they are NOT written out in the FITS header.  No warning is given
 *	either.
 *									   ###
 *   o	The TBLFLD.prvt.TFORM specifications are ignored when determining  ###
 *	field width and offset and data type.				   ###
 *
 * SYSTEM CONCERNS:
 *
 *   o	The ordering of entries in the p_shTblDataTypeBINTABLEmap mapping table
 *	is important when used to determine the TFORM data type given an object
 *	schema/alignment type. Place the object schema/alignment types that are
 *	to translate to a TFORM data type near the start of the map table. This
 *	still does not prevent the need to test for particular object schema
 *	types explicitly.
 *
 *	   o	For example, place "compound" data types, such as a heap des-
 *		criptor (TFORM data type of 'P') near the end, since their
 *		alignment type in the map table is SHALIGNTYPE_S32, but an
 *		alignment type of SHALIGNTYPE_S32 (equivalent to an object
 *		schema type of INT) should be mapped to a TFORM data type of
 *		'J' (Binary Table).  The object schema type TBLHEAPDSC needs
 *		to be tested for explicitly before going through a mapping.
 *
 *   o	Any FITS compliance and object schema checking should be reflected in
 *	shFitsTBLCOLcomply ().
 *--
 ******************************************************************************/

   {	/* s_shFitsTBLCOLwriteAsc */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
                  int	 lcl_forBreak;		/* Break out of for () loop   */
   ARRAY		*lcl_array;
   TBLFLD		*lcl_tblFld;		/* TBLFLD structure           */
                  int	 lcl_TFIELDS;		/* Number of Table fields     */
                  int	 lcl_NAXIS1 = 0;	/* FITS Table row size        */
                  int	 lcl_NAXIS1cand;	/* Candidate NAXIS1 value     */
                  char	 lcl_keyAval[(shFitsHdrStrSize) + 1];
   SHALIGN_TYPE		 lcl_alignType;
                  int	 lcl_dataTypeIdx;

                  int	 lcl_rowIdx;		/* RSA record/row index       */
                  int	 lcl_fldIdx;		/* Field index (0-indexed)    */
            long  int	 lcl_fldOffMax;		/* Maximum field offset found */
   unsigned long  int	 lcl_fldWidth;
                  int	 lcl_datumIncr;		/* Space between datums: bytes*/
   unsigned       char	*lcl_datum;
                  char	*lcl_buf = ((char *)0);	/* One Table row              */
            long  int	 lcl_bufSize;		/* Maximum size of buffer     */
   size_t		 lcl_bufLen;
   unsigned long  int	 lcl_bufIdx;		/* Index into lcl_buf         */
                  char	 lcl_num[255 + 1];	/* Make it huge to handle any */
                  int	 lcl_strLen;		/* size character representa- */
						/* tion of a number.          */
            long  int	 lcl_FITSfpWritten = 0;	/* Amount of bytes written    */
   HDR			 lcl_hdr;		/* Header to be written out   */
                  int	 lcl_hdrLineCnt;
                  char	*lcl_hdrLine;
                  int	 lcl_hdrLineIdx;
   HDR			 lcl_tblHdr;		/* Copy of TBLCOL header      */
                  char	 lcl_keyword[11+1];	/* FITS keyword(only need 8+1)*/

   struct		 lcl_fldInfoStruct
      {
      SHALIGN_TYPE		 alignType;	/* In-memory data type        */
      unsigned long  int	 incr;		/* Increment between in-memory*/
						/* ... data.                  */
      unsigned       int	 fldWidth;	/* Field width                */
      unsigned       int	 fldDecPt;	/* Position of decimal point  */
      unsigned long  int	 fldOff;	/* Field offset               */
                     char	 fldFmt;	/* Field TFORM format         */
                     int	 fldTNULLok;	/* TNULLn allowed for format? */
                     int	 fldTSCALok;	/* TSCALn allowed for format? */
                     int	 fldTZEROok;	/* TZEROn allowed for format? */
      unsigned       char	*dataPtr;	/* Pointer to in-memory data  */
      }			*lcl_fldInfo;

   TYPE			 lcl_schemaTypeSTR     = shTypeGetFromName ("STR");	/* For performance */
   TYPE			 lcl_schemaTypeLOGICAL = shTypeGetFromName ("LOGICAL");	/* For performance */
   TYPE			 lcl_schemaTypeTBLFLD  = shTypeGetFromName ("TBLFLD");	/* For performance */

#define	lcl_shHdrKeywordInsert(keywordType, keyword, keywordFld)\
   if (shInSet (lcl_tblFld->Tpres, SH_TBL_##keywordFld))\
      {\
      sprintf (lcl_keyword, "%s%d", #keyword, lcl_fldIdx + 1);\
      shHdrInsert##keywordType (&lcl_hdr, lcl_keyword, lcl_tblFld->keywordFld, ((char *)0)); lcl_hdrLineCnt++;\
      }

#define	lcl_caseInt(  alignType, Ctype, convSpec)\
   case SHALIGNTYPE_##alignType : sprintf (lcl_num, "%*"   #convSpec, (int)lcl_fldWidth, *((Ctype *)lcl_datum))

#define	lcl_caseFloat( convSpec)\
   case SHALIGNTYPE_FL32 : if (shNaNFloatTest  (*((float  *)lcl_datum))) {\
                                  sprintf (lcl_num, "%-*.*s", (int)lcl_fldWidth, (int)lcl_fldWidth, "NaN");\
                             } else {\
                                  sprintf (lcl_num, "%*.*" #convSpec, (int)lcl_fldWidth, (int)lcl_fldInfo[lcl_fldIdx].fldDecPt,\
                                                                                    *((float  *)lcl_datum));\
                             }
#define	lcl_caseDouble(convSpec)\
   case SHALIGNTYPE_FL64 : if (shNaNDoubleTest (*((double *)lcl_datum))) {\
                                  sprintf (lcl_num, "%-*.*s", (int)lcl_fldWidth, (int)lcl_fldWidth, "NaN");\
                             } else {\
                                  sprintf (lcl_num, "%*.*" #convSpec, (int)lcl_fldWidth, (int)lcl_fldInfo[lcl_fldIdx].fldDecPt,\
                                                                                    *((double *)lcl_datum));\
                             }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   memset (((void *)&lcl_hdr),    0, sizeof (HDR));  /* Clear for rtn_return  */
   memset (((void *)&lcl_tblHdr), 0, sizeof (HDR));  /* ... test.             */

   s_shFitsPlatformCheck ();

   /*
    * Compute storage needs for writing out the ASCII Table.
    *
    *   o   An array is allocated with data size and increments for each field.
    *       This is much more efficient than translating lcl_array->data.
    *       schemaType for each for each row.
    */

   lcl_fldIdx = shChainSize(&(tblCol->fld));
   lcl_TFIELDS = lcl_fldIdx;

   /* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */
   /*
    * Allocate an array of per-field information.
    */

   if ((lcl_fldInfo = ((struct lcl_fldInfoStruct *)calloc (lcl_TFIELDS, sizeof (struct lcl_fldInfoStruct))))
                   == ((struct lcl_fldInfoStruct *)0))
      {
      lcl_status = SH_MALLOC_ERR;
      shErrStackPush ("Unable to allocate space temporarily for FITS file writing");
      goto rtn_return;
      }

   lcl_fldOffMax = 0;			/* Start at the beginning of the row  */

   for (lcl_fldIdx = 0; lcl_fldIdx < shChainSize(&(tblCol->fld)); lcl_fldIdx++)
      {
      /*
       * Check some legalities and support issues.
       */

      lcl_array = shChainElementGetByPos(&(tblCol->fld), lcl_fldIdx);
      if (lcl_array->nStar != 0)
         {
         lcl_status = SH_INVOBJ;
         shErrStackPush ("Indirect access (pointers) to data not supported (field %d)", lcl_fldIdx + 1);
         continue;				/* Onto next field            */
         }

      /*
       * Determine the field width and decimal point position.
       *
       * Compute NAXIS1 and per-field information.
       *
       *   o   For character strings, the last (fastest varying) dimension is
       *       reduced by 1.  This counters the FITS reader's increment of this
       *       dimension, which was used to guarantee space for a null termina-
       *       tor.
       */

      lcl_fldInfo[lcl_fldIdx].dataPtr = lcl_array->data.dataPtr;

           if (lcl_array->data.schemaType == lcl_schemaTypeSTR)
         {
         if   (lcl_array->dimCnt > 2)
            {
            lcl_status = SH_NOT_SUPPORTED;
            shErrStackPush ("Multidimension arrays not supported by TABLE HDUs (field %d)", lcl_fldIdx);
            continue;				/* Onto next field            */
            }
         if   (lcl_array->dimCnt < 2)
            {
            lcl_status = SH_NOT_SUPPORTED;	/*                        ### */
            shErrStackPush ("STR object schema and ARRAY dimension count inconsistent (field %d)", lcl_fldIdx);
            continue;
            }
         lcl_fldInfo[lcl_fldIdx].fldDecPt  = 0;
         lcl_fldInfo[lcl_fldIdx].fldWidth  = lcl_array->dim[lcl_array->dimCnt - 1] - 1;
         lcl_fldInfo[lcl_fldIdx].incr      = lcl_array->dim[lcl_array->dimCnt - 1];
         lcl_fldInfo[lcl_fldIdx].fldOff    = lcl_fldOffMax;
                                             lcl_fldOffMax += lcl_fldInfo[lcl_fldIdx].fldWidth;
         lcl_fldInfo[lcl_fldIdx].fldFmt    = 'A';
         lcl_fldInfo[lcl_fldIdx].alignType = SHALIGNTYPE_S8;
         }
      else if (lcl_array->data.schemaType == lcl_schemaTypeLOGICAL)
         {
         if  (lcl_array->dimCnt > 1)
            {
            lcl_status = SH_NOT_SUPPORTED;
            shErrStackPush ("Multidimension arrays not supported by TABLE HDUs (field %d)", lcl_fldIdx);
            continue;				/* Onto next field            */
            }
         lcl_fldInfo[lcl_fldIdx].fldDecPt  = 0;
         lcl_fldInfo[lcl_fldIdx].fldWidth  = 1;	/* One character/byte         */
         lcl_fldInfo[lcl_fldIdx].incr      = lcl_array->data.incr;
         lcl_fldInfo[lcl_fldIdx].fldOff    = lcl_fldOffMax;
                                             lcl_fldOffMax += lcl_fldInfo[lcl_fldIdx].fldWidth;
         lcl_fldInfo[lcl_fldIdx].fldFmt    = 'I';
         lcl_fldInfo[lcl_fldIdx].alignType = SHALIGNTYPE_U8;
         }
      else
         {
         if  (lcl_array->dimCnt > 1)
            {
            lcl_status = SH_NOT_SUPPORTED;
            shErrStackPush ("Multidimension arrays not supported by TABLE HDUs (field %d)", lcl_fldIdx);
            continue;				/* Onto next field            */
            }
         if ((lcl_alignType = shAlignSchemaToAlignMap (lcl_array->data.schemaType)) == SHALIGNTYPE_unknown)
            {
            lcl_status = SH_NOT_SUPPORTED;
            shErrStackPush ("Object schema %s not supported (field %d)",shNameGetFromType (lcl_array->data.schemaType), lcl_fldIdx);
            continue;				/* Onto next field            */
            }
         lcl_fldInfo[lcl_fldIdx].incr     = lcl_array->data.incr;

         /*
          * Match the alignment type with the table of supported ASCII Table
          * data types.  The Binary Table (that's right, Binary Table) data
          * types are used, since they encompass more primitive data types.
          */

         for (lcl_dataTypeIdx = 0, lcl_forBreak = 0; ; lcl_dataTypeIdx++)
            {
            switch (p_shTblDataTypeBINTABLEmap[lcl_dataTypeIdx].dataType)
               {
               case '\000' : lcl_forBreak = 1;	/* Ran out of data types      */
                             lcl_status   = SH_INVOBJ;
                             shErrStackPush ("Object schema %s does not map to ASCII Table data type (field %d)",
                                              shNameGetFromType (lcl_array->data.schemaType), lcl_fldIdx);
                             break;
               case 'P'    :			/* Ignore some cases (they're */
               case 'L'    :			/* unsupported or handld above*/
                             break;

               default     : if (p_shTblDataTypeBINTABLEmap[lcl_dataTypeIdx].fldType == lcl_alignType)
                                {
                                if ((lcl_fldInfo[lcl_fldIdx].fldFmt    = p_shTblDataTypeBINTABLEmap[lcl_dataTypeIdx].ascFldFmtDef)
                                                                      != '\000')
                                   {
                                     lcl_forBreak = 1;	/* Found a match      */
                                     lcl_fldInfo[lcl_fldIdx].fldDecPt  = p_shTblDataTypeBINTABLEmap[lcl_dataTypeIdx].ascFldDecPtDef;
                                     lcl_fldInfo[lcl_fldIdx].fldWidth  = p_shTblDataTypeBINTABLEmap[lcl_dataTypeIdx].ascFldWidthDef;
                                     lcl_fldInfo[lcl_fldIdx].fldOff    = lcl_fldOffMax;
                                                                         lcl_fldOffMax += lcl_fldInfo[lcl_fldIdx].fldWidth;
                                     lcl_fldInfo[lcl_fldIdx].alignType = lcl_alignType;
                                   }
                                }
                             break;
               }
            if (lcl_forBreak != 0)
               {
               break;
               }
            }

         /*
          *   o   Although the Binary Table data types table was used above, it
          *       is NOT appropriate for all information.  Locate the correct
          *       information in the ASCII Table data types table.
          */

         for (lcl_dataTypeIdx = 0, lcl_forBreak = 0; ; lcl_dataTypeIdx++)
            {
            switch (p_shTblDataTypeTABLEmap[lcl_dataTypeIdx].dataType)
               {
               case '\000' : lcl_forBreak = 1;	/* Ran out of data types      */
                             lcl_status   = SH_INTERNAL_ERR;
                             shErrStackPush ("%s %s (ASCII data type '%c' not in p_shTblDataTypeTABLEmap) (field %d, %s line %d)",
                                             "Binary Table/ASCII Table Data Types Table mismatch for data type",
                                              shNameGetFromType (lcl_array->data.schemaType), lcl_fldInfo[lcl_fldIdx].fldFmt,
                                              lcl_fldIdx + 1, __FILE__, __LINE__);
                             shFatal        ("%s %s (ASCII data type '%c' not in p_shTblDataTypeTABLEmap) (field %d, %s line %d)",
                                             "Binary Table/ASCII Table Data Types Table mismatch for data type",
                                              shNameGetFromType (lcl_array->data.schemaType), lcl_fldInfo[lcl_fldIdx].fldFmt,
                                              lcl_fldIdx + 1, __FILE__, __LINE__);
                             break;

               default     : if (p_shTblDataTypeTABLEmap[lcl_dataTypeIdx].dataType == lcl_fldInfo[lcl_fldIdx].fldFmt)
                                {
                                lcl_forBreak = 1;	/* Found a match      */
                                lcl_fldInfo[lcl_fldIdx].fldTNULLok = p_shTblDataTypeTABLEmap[lcl_dataTypeIdx].fldTNULLok;
                                lcl_fldInfo[lcl_fldIdx].fldTSCALok = p_shTblDataTypeTABLEmap[lcl_dataTypeIdx].fldTSCALok;
                                lcl_fldInfo[lcl_fldIdx].fldTZEROok = p_shTblDataTypeTABLEmap[lcl_dataTypeIdx].fldTZEROok;
                                }
                             break;
               }
            if (lcl_forBreak != 0)
               {
               break;
               }
            }
         }

      /*
       *   o   Some degenerate cases require special handling.
       *
       *          o   When no data is allocated to the ARRAY and the row count
       *              is greater than zero, the element count is set to zero.
       *              But, if no data is allocated, but the row count is zero,
       *              the element count is retained, as there is no danger of
       *              writing an invalid FITS file (as in the case of a non-zero
       *              NAXIS2 (row count) and non-zero element count causing the
       *              reading of non-existant data in the RSA (record storage
       *              area)).
       */

      if (((lcl_array->data.dataPtr == ((unsigned char *)0)) && (tblCol->rowCnt > 0)) || (lcl_array->dimCnt <= 0))
         {
         lcl_fldInfo[lcl_fldIdx].fldWidth = 0;
         }

      if (lcl_NAXIS1 < (lcl_NAXIS1cand = lcl_fldInfo[lcl_fldIdx].fldOff + lcl_fldInfo[lcl_fldIdx].fldWidth))
         {
          lcl_NAXIS1 =  lcl_NAXIS1cand;
         }
      }

   if (lcl_status != SH_SUCCESS)
      {
      shErrStackPush ("Unable to write FITS HDU");
      goto rtn_return;
      }

   /*
    * Allocate space for the output buffer.  This is done here, so that if it
    * fails, the FITS header is not written out to the file.
    *
    *   o   A minimum size, shFitsRecSize, is allocated so that I/O is efficient
    *       when zero padding the last FITS record.
    */

   lcl_bufSize = (lcl_NAXIS1 >= (shFitsRecSize)) ? lcl_NAXIS1 : (shFitsRecSize);

   if ((lcl_buf = ((char *)shMalloc (lcl_bufSize))) == ((char *)0))
      {
      lcl_status = SH_MALLOC_ERR;
      shErrStackPush ("Unable to allocate space for writing ASCII Table row");
      goto rtn_return;
      }

   /* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */
   /*
    * Generate the FITS header.
    */
                                                                          lcl_hdrLineCnt = 0;
   shHdrInsertAscii   (&lcl_hdr, "XTENSION",       "TABLE", ((char *)0)); lcl_hdrLineCnt++;
   shHdrInsertInt     (&lcl_hdr, "BITPIX",              8,  ((char *)0)); lcl_hdrLineCnt++;
   shHdrInsertInt     (&lcl_hdr, "NAXIS",               2,  ((char *)0)); lcl_hdrLineCnt++;
   shHdrInsertInt     (&lcl_hdr, "NAXIS1",     lcl_NAXIS1,  ((char *)0)); lcl_hdrLineCnt++;
   shHdrInsertInt     (&lcl_hdr, "NAXIS2", tblCol->rowCnt,  ((char *)0)); lcl_hdrLineCnt++;
   shHdrInsertInt     (&lcl_hdr, "PCOUNT",              0,  ((char *)0)); lcl_hdrLineCnt++;
   shHdrInsertInt     (&lcl_hdr, "GCOUNT",              1,  ((char *)0)); lcl_hdrLineCnt++;
   shHdrInsertInt     (&lcl_hdr, "TFIELDS",   lcl_TFIELDS,  ((char *)0)); lcl_hdrLineCnt++;

   /*
    *   o   Copy Dervish header lines into FITS header.
    *
    *          o   A local copy of the TBLCOL header is made so that certain
    *              keywords can be cleaned out.
    */

   if ((lcl_status = shHdrCopy (&tblCol->hdr, &lcl_tblHdr)) != SH_SUCCESS)
      {
      shErrStackPush ("Error temporarily duplicating TBLCOL header");
      goto rtn_return;
      }

   p_shFitsHdrClean (&lcl_tblHdr);	/* Clean out things we generate       */

   for (lcl_hdrLineIdx = 0; (lcl_hdrLine = lcl_tblHdr.hdrVec[lcl_hdrLineIdx]) != ((char *)0); lcl_hdrLineIdx++)
      {
      shHdrInsertLine (&lcl_hdr, lcl_hdrLineCnt, lcl_hdrLine); lcl_hdrLineCnt++;
      }

   /*
    *   o   Generate per-field keywords.
    */

   for (lcl_fldIdx = 0; lcl_fldIdx < shChainSize(&(tblCol->fld)); lcl_fldIdx++)
      {

      lcl_array = (ARRAY *) shChainElementGetByPos(&(tblCol->fld), lcl_fldIdx);

      lcl_tblFld   = (lcl_array->infoType == lcl_schemaTypeTBLFLD) ? ((TBLFLD *)lcl_array->info) : ((TBLFLD *)0);

      /*
       * TBCOLn.
       */

      sprintf (lcl_keyword, "TBCOL%d", lcl_fldIdx + 1);
      shHdrInsertInt    (&lcl_hdr, lcl_keyword, lcl_fldInfo[lcl_fldIdx].fldOff + 1, ((char *)0)); lcl_hdrLineCnt++;

      /*
       * TFORMn.
       */

      sprintf (lcl_keyword, "TFORM%d", lcl_fldIdx + 1);
      if (lcl_fldInfo[lcl_fldIdx].fldDecPt > 0)
         {
         sprintf (lcl_keyAval, "%c%d.%d", lcl_fldInfo[lcl_fldIdx].fldFmt, lcl_fldInfo[lcl_fldIdx].fldWidth,
                                                                          lcl_fldInfo[lcl_fldIdx].fldDecPt);
         }
      else
         {
         sprintf (lcl_keyAval, "%c%d",    lcl_fldInfo[lcl_fldIdx].fldFmt, lcl_fldInfo[lcl_fldIdx].fldWidth);
         }
      shHdrInsertAscii (&lcl_hdr, lcl_keyword, lcl_keyAval, ((char *)0)); lcl_hdrLineCnt++;

      /*
       * Other keywords based on TBLFLD.
       */

      if (lcl_tblFld != ((TBLFLD *)0))
         {
                                                   lcl_shHdrKeywordInsert (Ascii, TTYPE, TTYPE);
                                                   lcl_shHdrKeywordInsert (Ascii, TUNIT, TUNIT);
         if (lcl_fldInfo[lcl_fldIdx].fldTSCALok) { lcl_shHdrKeywordInsert (Dbl,   TSCAL, TSCAL);    }
         if (lcl_fldInfo[lcl_fldIdx].fldTZEROok) { lcl_shHdrKeywordInsert (Dbl,   TZERO, TZERO);    }
         if (lcl_fldInfo[lcl_fldIdx].fldTNULLok) { lcl_shHdrKeywordInsert (Ascii, TNULL, TNULLSTR); }
         }
      }

   shHdrInsertLine    (&lcl_hdr, lcl_hdrLineCnt, "END");    lcl_hdrLineCnt++;

   /*
    * Write out the FITS header.
    */

   if (!f_wrhead (lcl_hdr.hdrVec, FITSfp))	/* Write out FITS header      */
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("Error writing TABLE header to FITS file");
      goto rtn_return;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Write out the ASCII Table RSA (Record Storage Area).
    */

   for (lcl_rowIdx = 0; lcl_rowIdx < tblCol->rowCnt; lcl_rowIdx++)
      {
      lcl_bufIdx = 0;				/* Start at beginning of row  */

      for (lcl_fldIdx = 0; ((lcl_fldIdx < shChainSize(&(tblCol->fld))) && (lcl_fldIdx < lcl_TFIELDS)); lcl_fldIdx++)
         {
         lcl_array       = (ARRAY *) shChainElementGetByPos(&(tblCol->fld), lcl_fldIdx);

         lcl_datum       = lcl_fldInfo[lcl_fldIdx].dataPtr; /* Copied for     */
         lcl_datumIncr   = lcl_fldInfo[lcl_fldIdx].incr;    /* performance.   */
         lcl_fldWidth    = lcl_fldInfo[lcl_fldIdx].fldWidth;

         switch (lcl_fldInfo[lcl_fldIdx].fldFmt)
            {
            case 'A' : if ((lcl_strLen = shStrnlen (((char *)lcl_datum), lcl_fldWidth)) > lcl_fldWidth)
                          {
                            lcl_strLen =                                 lcl_fldWidth; /* Truncate */
                            lcl_status = SH_INVOBJ;
                            shErrStackPush ("TFORM%d 'A' field width inadequate (row %d)", lcl_fldIdx + 1, lcl_rowIdx);
                          }
                       memcpy (((void *)&lcl_buf[lcl_fldInfo[lcl_fldIdx].fldOff]), ((void *)lcl_datum), lcl_strLen);
                       if (lcl_strLen < lcl_fldWidth)
                          {
                          memset (((void *)&lcl_buf[lcl_fldInfo[lcl_fldIdx].fldOff + lcl_strLen]), ' ', lcl_fldWidth - lcl_strLen);
                          }
                       break;

            case 'I' :
                       if (lcl_array->data.schemaType == lcl_schemaTypeLOGICAL)
                          {
                          sprintf (lcl_num, "%*s", (int)lcl_fldWidth, (((LOGICAL *)lcl_datum)) ? "1" : "0");
                          }
                       else switch (lcl_fldInfo[lcl_fldIdx].alignType)
                          {
                          lcl_caseInt  (U8,   unsigned       char, u); break;
                          lcl_caseInt  (S8,     signed       char, d); break;
                          lcl_caseInt  (U16,  unsigned short int, hu); break;
                          lcl_caseInt  (S16,           short int, hd); break;
                          lcl_caseInt  (U32,  unsigned       int,  u); break;
                          lcl_caseInt  (S32,                 int,  d); break;

                          default :
                               lcl_status = SH_INTERNAL_ERR;
                               shErrStackPush ("Unsupported data type, %s (ASCII data type '%c') (field %d, %s line %d)",
                                                shNameGetFromType (lcl_array->data.schemaType), lcl_fldInfo[lcl_fldIdx].fldFmt,
                                                lcl_fldIdx + 1, __FILE__, __LINE__);
                               shFatal        ("Unsupported data type, %s (ASCII data type '%c') (field %d, %s line %d)",
                                                shNameGetFromType (lcl_array->data.schemaType), lcl_fldInfo[lcl_fldIdx].fldFmt,
                                                lcl_fldIdx + 1, __FILE__, __LINE__);
                               goto rtn_return;
                               break;
                          }
                       if (strlen (lcl_num) > lcl_fldWidth)
                          {
                          lcl_status = SH_INVOBJ;
                          shErrStackPush ("TFORM%d 'I' field width inadequate (row %d)", lcl_fldIdx + 1, lcl_rowIdx);
                          }
                       else
                          {
                          memcpy (((void *)&lcl_buf[lcl_fldInfo[lcl_fldIdx].fldOff]), ((void *)lcl_num), lcl_fldWidth);
                          }
                       break;

            case 'F' :
            case 'E' :
            case 'D' :
                       switch (lcl_fldInfo[lcl_fldIdx].alignType)
                          {
                          lcl_caseFloat  (f);  break;
                          lcl_caseDouble (E);  break;

                          default :
                               lcl_status = SH_INTERNAL_ERR;
                               shErrStackPush ("Unsupported data type, %s (ASCII data type '%c') (field %d, %s line %d",
                                                lcl_fldInfo[lcl_fldIdx].fldFmt, shNameGetFromType (lcl_array->data.schemaType),
                                                            lcl_fldIdx + 1, __FILE__, __LINE__);
                               shFatal        ("Unsupported data type, %s (ASCII data type '%c') (field %d, %s line %d",
                                                lcl_fldInfo[lcl_fldIdx].fldFmt, shNameGetFromType (lcl_array->data.schemaType),
                                                            lcl_fldIdx + 1, __FILE__, __LINE__);
                               goto rtn_return;
                               break;
                          }
                       if (strlen (lcl_num) > lcl_fldWidth)
                          {
                          lcl_status = SH_INVOBJ;
                          shErrStackPush ("TFORM%d '%c' field width inadequate (row %d)", lcl_fldInfo[lcl_fldIdx].fldFmt,
                                           lcl_fldIdx + 1, lcl_rowIdx);
                          }
                       else
                          {
                          memcpy (((void *)&lcl_buf[lcl_fldInfo[lcl_fldIdx].fldOff]), ((void *)lcl_num), lcl_fldWidth);
                          }
                       break;

            default  :
                       lcl_status = SH_INTERNAL_ERR;
                       shErrStackPush ("ASCII Table data type '%c' not handled (%s line %d)", lcl_fldInfo[lcl_fldIdx].fldFmt,
                                       __FILE__, __LINE__);
                       shFatal        ("ASCII Table data type '%c' not handled (%s line %d)", lcl_fldInfo[lcl_fldIdx].fldFmt,
                                       __FILE__, __LINE__);
                       goto rtn_return;
                       break;
            }

         lcl_fldInfo[lcl_fldIdx].dataPtr = lcl_datum + lcl_datumIncr;
         }

      if (lcl_status != SH_SUCCESS)
         {
         goto rtn_return;
         }

      lcl_FITSfpWritten += (lcl_bufLen = fwrite (lcl_buf, 1, lcl_NAXIS1, FITSfp));
      if (lcl_bufLen != lcl_NAXIS1)
         {
         lcl_status = SH_INV_FITS_FILE;
         shErrStackPush ("Write error");
         goto rtn_return;
         }
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    */

rtn_return : ;

   /*
    * Clean up the header.  Remove anything added to generate a legitimate FITS
    * header.
    */

   p_shFitsHdrClean (&tblCol->hdr);

   /*
    * Push the file position to the start of the next FITS record by padding
    * with zeros.
    *
    *   o   lcl_bufLen is used for a different purpose.
    */

   if ((lcl_bufLen = lcl_FITSfpWritten % (shFitsRecSize)) != 0)
      {
        lcl_bufLen = (shFitsRecSize) - lcl_bufLen;
        memset     (((void *)lcl_buf), 0, lcl_bufLen);	/* Padding constant   */
        if (fwrite (         lcl_buf,  1, lcl_bufLen, FITSfp) != lcl_bufLen)
           {
           lcl_status = SH_INV_FITS_FILE;
           shErrStackPush ("Write error");
           shErrStackPush ("FITS file not a multiple of FITS record size (%d bytes)", (shFitsRecSize));
           }
      }

   if (lcl_buf           != ((char    *)0)) { shFree (lcl_buf);                }
   if (lcl_hdr.hdrVec    != ((HDR_VEC *)0)) { p_shHdrFreeForVec (&lcl_hdr);    }
   if (lcl_tblHdr.hdrVec != ((HDR_VEC *)0)) { p_shHdrFreeForVec (&lcl_tblHdr); }

   return (lcl_status);

#undef	lcl_shHdrKeywordInsert
#undef	lcl_caseInt
#undef	lcl_caseFloat
#undef	lcl_caseDouble

   }	/* s_shFitsTBLCOLwriteAsc */

/******************************************************************************/

static RET_CODE	 s_shFitsTBLCOLwriteBin
   (
   TBLCOL		 *tblCol,	/* IN:    Table handle                */
   FILE			 *FITSfp,	/* INOUT: FITS file file pointer      */
   SHFITSSWAP		  datumSwap,	/* IN:    Swap datum bytes?           */
   FITSFILETYPE		  FITSstd	/* IN:    Write FITS compliant?       */
   )

/*++
 * ROUTINE:		 s_shFitsTBLCOLwriteBin
 *
 *	Write a TBLCOL Table as a FITS BINTABLE Binary Table extension.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_NOT_SUPPORTED	Failure.
 *			The ARRAY data type cannot be written to a Binary Table
 *			or the ARRAY cannot be written to comply with FITS (if
 *			FITSstd is STANDARD).
 *
 *   SH_MALLOC_ERR	Failure.
 *
 * SIDE EFFECTS:
 *
 *   o	Data is written to the FITS file.  This changes the file position.  If
 *	the write is successful, the file will be positioned at EOF.  Otherwise,
 *	the file position is indeterminate.
 *
 * SYSTEM CONCERNS:
 *
 *   o	The ordering of entries in the p_shTblDataTypeBINTABLEmap mapping table
 *	is important when used to determine the TFORM data type given an object
 *	schema/alignment type. Place the object schema/alignment types that are
 *	to translate to a TFORM data type near the start of the map table. This
 *	still does not prevent the need to test for particular object schema
 *	types explicitly.
 *
 *	   o	For example, place "compound" data types, such as a heap des-
 *		criptor (TFORM data type of 'P') near the end, since their
 *		alignment type in the map table is SHALIGNTYPE_S32, but an
 *		alignment type of SHALIGNTYPE_S32 (equivalent to an object
 *		schema type of INT) should be mapped to a TFORM data type of
 *		'J' (Binary Table).  The object schema type TBLHEAPDSC needs
 *		to be tested for explicitly before going through a mapping.
 *
 *   o	If some per-field information is set, but not permitted by the data
 *	type, then that per-field information is NOT written out as a FITS
 *	header keyword.  For example, TSCALn and TZEROn are not permitted
 *	for `A' data types (well, actually, that's true for ASCII Tables only).
 *	If these values are present (in the TBLFLD), they are NOT written out
 *	in the FITS header.  No warning is given either.
 *
 *   o	Any FITS compliance and object schema checking should be reflected in
 *	shFitsTBLCOLcomply ().  The check for FITS compliance as reflected by
 *	the FITSstd parameter should have already been performed by the caller.
 *
 *	   o	The FITS community convention of setting TSCALn/TZEROn to indi-
 *		cate flipping the "signedness" of a field is used, if possible.
 *		TSCALn/TZEROn must not be set (in TBLFLD), or they can only be
 *		1.0 and 0.0 (respectively).
 *--
 ******************************************************************************/

   {	/* s_shFitsTBLCOLwriteBin */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
                  int	 lcl_forBreak;		/* Break out of for () loop   */
   ARRAY		*lcl_array;
   TBLFLD		*lcl_tblFld;		/* TBLFLD structure           */
                  int	 lcl_TFIELDS;		/* Number of Table fields     */
                  int	 lcl_NAXIS1 = 0;	/* FITS Table row size        */
                  int	 lcl_dimIdx;
                  int	 lcl_TDIMpres;		/* TDIM should be written out */
                  char	 lcl_TDIM[(shFitsTDIMsize) + 1];  /* TDIMn value      */
                  char	 lcl_keyAval[(shFitsHdrStrSize) + 1];
   SHALIGN_TYPE		 lcl_alignType;
                  int	 lcl_dataTypeIdx;
                  int	 lcl_heapTypeIdx;
                  int	 lcl_typeIdx;		/* Either data/heapTypeIdx    */

                  int	 lcl_rowIdx;		/* RSA record/row index       */
                  int	 lcl_fldIdx;		/* Field index (0-indexed)    */
                  int	 lcl_elemIdx;
                  int	 lcl_datumSize;		/* Size of datum in bytes     */
                  int	 lcl_datumIncr;		/* Space between datums: bytes*/
   SHFITSSWAP		 lcl_datumSwap;
                  int	 lcl_datumIdx;		/* Byte index into datum      */
                  int	 lcl_datumIdxIni;	/* Initial value              */
                  int	 lcl_datumCtr;		/* Counter for copying over   */
   unsigned       char	*lcl_datum;
   unsigned       char	*lcl_buf = ((unsigned char *)0);  /* One Table row    */
            long  int	 lcl_bufSize;		/* Maximum size of buffer     */
   size_t		 lcl_bufLen;
   unsigned long  int	 lcl_bufIdx;		/* Index into lcl_buf         */
            long  int	 lcl_FITSfpWritten = 0;	/* Amount of bytes written    */
   HDR			 lcl_hdr;		/* Header to be written out   */
                  int	 lcl_hdrLineCnt;
                  char	*lcl_hdrLine;
                  int	 lcl_hdrLineIdx;
   HDR			 lcl_tblHdr;		/* Copy of TBLCOL header      */
   TBLHEAPDSC		*lcl_heapDsc;
            long  int	 lcl_heapNonZero;
            long  int	 lcl_heapCntAll;
            long  int	 lcl_heapCnt;
            long  int	 lcl_heapSize      = 0;
            long  int	 lcl_heapDatumSizeMax=0;/* Maximum heap datum size    */
            long  int	 lcl_heapEnd;		/* Heap offset in FITS file   */
                  char	 lcl_keyword[11+1];	/* FITS keyword(only need 8+1)*/
                  int	 lcl_err;		/* Localized error?           */

   struct		 lclFldInfoData		/* Per-field information      */
      {
      unsigned long  int	 size;		/* Size of the field          */
      unsigned long  int	 incr;		/* Increment between in-memory*/
      SHFITSSWAP		 swap;		/* Swap datum bytes?          */
                     int	 swapIni;	/* Used to control swapping   */
						/* ... data.                  */
                     char	 dataType;	/* Binary Table data type     */
      unsigned       char	*dataPtr;	/* Pointer to in-memory data  */
      };

   struct		 lclFldInfo		/* Per-field information      */
      {
      struct lclFldInfoData	 data;		/* Primary ARRAY data         */
      struct lclFldInfoData	 heap;		/* Heap data (if present)     */
               long  int	 heapCntMax;	/* Maximum heap elem count    */
                     int	 heapCntAdj;	/* TBLHEAPDSC->cnt adjustment */
               long  int	 elemCnt;	/* TFORMn element count       */
               long  int	 schemaCnt;	/* schemaType object count    */
               long  int	 primCnt;	/* Primitive data type count  */
      unsigned       char	 signFlip;	/* Flip sign bit?             */
                     int	 fldTNULLok;	/* TNULLn allowed?            */
                     int	 fldTSCALok;	/* TSCALn allowed?            */
                     int	 fldTZEROok;	/* TZEROn allowed?            */
                     int	 fldTSCALset;	/* TSCALn set in this struct  */
                     int	 fldTZEROset;	/* TZEROn set in this struct  */
      double			 fldTSCAL;	/* TSCALn value               */
      double			 fldTZERO;	/* TZEROn value               */
      }			*lcl_fldInfo = (struct lclFldInfo *) 0;
   TYPE			 lcl_flipSchema;
   double		 lcl_flipTSCAL;
   double		 lcl_flipTZERO;
   TYPE			 lcl_schemaType;

   TYPE			 lcl_schemaTypeCHAR       = shTypeGetFromName ("CHAR"); 	/* For performance */
   TYPE			 lcl_schemaTypeSTR        = shTypeGetFromName ("STR");		/* For performance */
   TYPE			 lcl_schemaTypeTBLHEAPDSC = shTypeGetFromName ("TBLHEAPDSC");	/* For performance */
   TYPE			 lcl_schemaTypeLOGICAL    = shTypeGetFromName ("LOGICAL");	/* For performance */
   TYPE			 lcl_schemaTypeTBLFLD     = shTypeGetFromName ("TBLFLD");	/* For performance */

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   memset (((void *)&lcl_hdr),    0, sizeof (HDR));  /* Clear for rtn_return  */
   memset (((void *)&lcl_tblHdr), 0, sizeof (HDR));  /* ... test.             */

   s_shFitsPlatformCheck ();

   /*
    * Compute storage needs for writing out the Binary Table.
    *
    *   o   An array is allocated with data size and increments for each field.
    *       This is much more efficient than translating lcl_array->data.
    *       schemaType for each for each row.
    */

   lcl_fldIdx = shChainSize(&(tblCol->fld));
   lcl_TFIELDS = lcl_fldIdx;

   /* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */
   /*
    * Allocate an array of per-field information.
    */

   if ((lcl_fldInfo = ((struct lclFldInfo *)calloc (lcl_TFIELDS, sizeof (struct lclFldInfo)))) == ((struct lclFldInfo *)0))
      {
      lcl_status = SH_MALLOC_ERR;
      shErrStackPush ("Unable to allocate space temporarily for FITS file writing");
      goto rtn_return;
      }

   for (lcl_fldIdx = 0; lcl_fldIdx < shChainSize(&(tblCol->fld)); lcl_fldIdx++)
      {

      lcl_array  = (ARRAY *) shChainElementGetByPos(&(tblCol->fld), lcl_fldIdx);
      lcl_tblFld = (lcl_array->infoType == lcl_schemaTypeTBLFLD) ? ((TBLFLD *)lcl_array->info) : ((TBLFLD *)0);
      lcl_typeIdx = -1;

      /*
       * Check some legalities and support issues.
       */

      if (lcl_array->nStar != 0)
         {
         lcl_status = SH_INVOBJ;
         shErrStackPush ("Indirect access (pointers) to data not supported (field %d)", lcl_fldIdx + 1);
         continue;				/* Onto next field            */
         }

      /* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .  */
      /*
       * Compute the element count (as required by TFORMn).  The first (slowest
       * varying) dimension is ignore, as that's the Table row count.
       *
       *   o   Character strings have their last (fastest varying) index reduced
       *       by 1.  This counters the FITS Table reader's increment by 1 used
       *       used to guarantee space for a null terminator.
       *
       *   o   Some degenerate cases require special handling.
       *
       *          o   When no data is allocated to the ARRAY and the row count
       *              is greater than zero, the element count is set to zero.
       *              But, if no data is allocated, but the row count is zero,
       *              the element count is retained, as there is no danger of
       *              writing an invalid FITS file (as in the case of a non-zero
       *              NAXIS2 (row count) and non-zero element count causing the
       *              reading of non-existant data in the RSA (record storage
       *              area)).
       */

      lcl_fldInfo[lcl_fldIdx].fldTNULLok   = FALSE;	/* Default            */
      lcl_fldInfo[lcl_fldIdx].fldTSCALok   = FALSE; lcl_fldInfo[lcl_fldIdx].fldTSCALset = FALSE;
                                                    lcl_fldInfo[lcl_fldIdx].fldTSCAL    = 1.0;	/* Default value */
      lcl_fldInfo[lcl_fldIdx].fldTZEROok   = FALSE; lcl_fldInfo[lcl_fldIdx].fldTZEROset = FALSE;
                                                    lcl_fldInfo[lcl_fldIdx].fldTZERO    = 0.0;	/* Default value */
      lcl_fldInfo[lcl_fldIdx].signFlip     = TBLFLD_SIGNNOFLIP;	/* Default    */
      lcl_fldInfo[lcl_fldIdx].data.dataPtr = lcl_array->data.dataPtr;
      lcl_schemaType = lcl_array->data.schemaType;

      if (((lcl_array->data.dataPtr == ((unsigned char *)0)) && (tblCol->rowCnt > 0)) || (lcl_array->dimCnt <= 0))
         {
              lcl_fldInfo[lcl_fldIdx].primCnt   =
              lcl_fldInfo[lcl_fldIdx].schemaCnt =
              lcl_fldInfo[lcl_fldIdx].elemCnt   = 0;
         }
      else
         {
         for (lcl_fldInfo[lcl_fldIdx].elemCnt   = 1, lcl_dimIdx = (lcl_array->dimCnt - 2); lcl_dimIdx > 0; lcl_dimIdx--)
            {
              lcl_fldInfo[lcl_fldIdx].elemCnt  *=  lcl_array->dim[lcl_dimIdx];
            }
              lcl_fldInfo[lcl_fldIdx].primCnt   =
              lcl_fldInfo[lcl_fldIdx].schemaCnt = lcl_fldInfo[lcl_fldIdx].elemCnt;
         if ( lcl_array->dimCnt > 1)		/* Don't use Table row count  */
            {
            if (lcl_array->data.schemaType == lcl_schemaTypeSTR)
               {
               lcl_fldInfo[lcl_fldIdx].elemCnt  *= (lcl_array->dim[lcl_array->dimCnt - 1] - 1);
               }
            else
               {
               lcl_fldInfo[lcl_fldIdx].primCnt   =
               lcl_fldInfo[lcl_fldIdx].schemaCnt =
               lcl_fldInfo[lcl_fldIdx].elemCnt  *= (lcl_array->dim[lcl_array->dimCnt - 1]);
               }
            }
         }

      /* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .  */
      /*
       * Compute NAXIS1 and per-field information.
       *
       *   o   For character strings, the last (fastest varying) dimension is
       *       reduced by 1.  This counters the FITS reader's increment of this
       *       dimension, which was used to guarantee space for a null termina-
       *       tor.
       *
       *   o   Adjust previously computed information based on data type.
       *
       *          o   'P' heap descriptor fields are assumed to be 2 4-byte
       *              integers in size.
       *
       *       NOTE:  TBLHEAPDSC is composed of 2 members, a count and a heap
       *              pointer.  The count is assumed to be 4 bytes.  The pointer
       *              may be a different size then 4 bytes.
       */

      if      (lcl_array->data.schemaType == lcl_schemaTypeSTR)
         {
         if   (lcl_array->dimCnt < 2)
            {
            lcl_status = SH_NOT_SUPPORTED;	/*                        ### */
            shErrStackPush ("STR object schema and ARRAY dimension count inconsistent (field %d)", lcl_fldIdx);
            continue;
            }
         lcl_fldInfo[lcl_fldIdx].data.size     = lcl_array->dim[lcl_array->dimCnt - 1] - 1;
         lcl_fldInfo[lcl_fldIdx].data.incr     = lcl_array->dim[lcl_array->dimCnt - 1];
         lcl_fldInfo[lcl_fldIdx].data.swap     = SH_FITS_NOSWAP;	/* Always     */
         lcl_fldInfo[lcl_fldIdx].data.dataType = 'A';
         }
      else if (lcl_array->data.schemaType == lcl_schemaTypeLOGICAL)	/* 'L'*/
         {
         lcl_fldInfo[lcl_fldIdx].data.size     = lcl_array->data.size;
         lcl_fldInfo[lcl_fldIdx].data.incr     = lcl_array->data.incr;
         lcl_fldInfo[lcl_fldIdx].data.swap     = datumSwap;
         lcl_fldInfo[lcl_fldIdx].data.dataType = 'L';
         }
      else if (lcl_array->data.schemaType == lcl_schemaTypeTBLHEAPDSC)	/* 'P'*/
         {
         lcl_fldInfo[lcl_fldIdx].data.size     = shAlignment.type[SHALIGNTYPE_S32].size;
         lcl_fldInfo[lcl_fldIdx].data.incr     = shAlignment.type[SHALIGNTYPE_S32].incr;
         lcl_fldInfo[lcl_fldIdx].primCnt      *= 2;	/* Hardwired      ### */
         lcl_fldInfo[lcl_fldIdx].data.swap     = datumSwap;
         lcl_fldInfo[lcl_fldIdx].data.dataType = 'P';

         /*
          * Check for any heap descriptor abnormalities.
          */

         for (lcl_rowIdx = 0; lcl_rowIdx < tblCol->rowCnt; lcl_rowIdx++)
            {
            lcl_heapDsc = &((TBLHEAPDSC *)lcl_fldInfo[lcl_fldIdx].data.dataPtr)[lcl_rowIdx];
            if       (lcl_heapDsc->cnt < 0)
               {
               lcl_status = SH_INVOBJ;
               shErrStackPush ("Heap descriptor (field %d, row %d) is invalid (negative count)", lcl_fldIdx, lcl_rowIdx);
               }
            else if ((lcl_heapDsc->cnt > 0) && (lcl_heapDsc->ptr == ((unsigned char *)0)))
               {
               lcl_status = SH_INVOBJ;
               shErrStackPush ("Heap descriptor (field %d, row %d) is invalid (NULL pointer to heap data)", lcl_fldIdx, lcl_rowIdx);
               }
            }

         /*
          * Determine the heap data type.
          *
          *   o   A minimal of other work involving TBLFLDs is done here also.
          */

         lcl_fldInfo[lcl_fldIdx].heapCntAdj    = 0;	/* Default no adjust  */
         lcl_fldInfo[lcl_fldIdx].heap.size     = 1;	/* Default to unknown */	/*### */
         lcl_fldInfo[lcl_fldIdx].heap.incr     = 1;	/* Default to unknown */	/*### */
         lcl_fldInfo[lcl_fldIdx].heap.swap     = SH_FITS_NOSWAP;
         lcl_fldInfo[lcl_fldIdx].heap.dataType = '\000';	/* None       */

         if (lcl_tblFld != ((TBLFLD *)0))
            {
            /*
             * Handle TSCALn and TZEROn from TBLFLD.
             */

            if (shInSet (lcl_tblFld->Tpres, SH_TBL_TSCAL))
               {
               lcl_fldInfo[lcl_fldIdx].fldTSCALset = TRUE;
               lcl_fldInfo[lcl_fldIdx].fldTSCAL    = lcl_tblFld->TSCAL;
               }
            if (shInSet (lcl_tblFld->Tpres, SH_TBL_TZERO))
               {
               lcl_fldInfo[lcl_fldIdx].fldTZEROset = TRUE;
               lcl_fldInfo[lcl_fldIdx].fldTZERO    = lcl_tblFld->TZERO;
               }

            /*
             * Determine the heap data type.
             *
             *   o   STRs reflect the need to decrement the heap count by one to
             *       ignore the guaranteed null-terminator character.
             */

            lcl_schemaType = lcl_tblFld->heap.schemaType;
            if      (lcl_tblFld->heap.schemaType == lcl_schemaTypeSTR)
               {
               lcl_fldInfo[lcl_fldIdx].heapCntAdj    = -1; /* Lose null term  */
               lcl_fldInfo[lcl_fldIdx].heap.size     = lcl_tblFld->heap.size;
               lcl_fldInfo[lcl_fldIdx].heap.incr     = lcl_tblFld->heap.incr;
               lcl_fldInfo[lcl_fldIdx].heap.swap     = SH_FITS_NOSWAP; /* Always*/
               lcl_fldInfo[lcl_fldIdx].heap.dataType = 'A';
               }
            else if (lcl_tblFld->heap.schemaType == lcl_schemaTypeLOGICAL) /* 'L' */
               {
               lcl_fldInfo[lcl_fldIdx].heap.size     = lcl_tblFld->heap.size;
               lcl_fldInfo[lcl_fldIdx].heap.incr     = lcl_tblFld->heap.incr;
               lcl_fldInfo[lcl_fldIdx].heap.swap     = datumSwap;
               lcl_fldInfo[lcl_fldIdx].heap.dataType = 'L';
               }
            else
               {
               if ((lcl_alignType = shAlignSchemaToAlignMap (lcl_tblFld->heap.schemaType)) == SHALIGNTYPE_unknown)
                  {
                  lcl_status = SH_NOT_SUPPORTED;
                  shErrStackPush ("Heap object schema %s not supported (field %d)",shNameGetFromType (lcl_tblFld->heap.schemaType),
                                   lcl_fldIdx);
                  continue;			/* Onto next field            */
                  }
               lcl_fldInfo[lcl_fldIdx].heap.size     = lcl_tblFld->heap.size;
               lcl_fldInfo[lcl_fldIdx].heap.incr     = lcl_tblFld->heap.incr;
               lcl_fldInfo[lcl_fldIdx].heap.swap     = datumSwap;

               /*
                * Match the alignment type with the table of supported Binary
                * Table heap data types.
                *
                *   o   For character strings, the last (fastest varying) dimen-
                *       sion is reduced by 1.  This counters the FITS reader's
                *       increment of this dimension, which was used to guarantee
                *       space for a null terminator.
                *
                *   o   Adjust previously computed information based on data
                *       type.
                */

               for (lcl_heapTypeIdx = 0, lcl_forBreak = 0; ; lcl_heapTypeIdx++)
                  {
                  switch (p_shTblDataTypeBINTABLEmap[lcl_heapTypeIdx].dataType)
                     {
                     case '\000' : lcl_forBreak = 1;	/* Ran out of data types      */
                                   lcl_status = SH_INVOBJ;
                                   shErrStackPush ("Heap object schema %s does not map to Binary Table data type (field %d)",
                                                    shNameGetFromType (lcl_tblFld->heap.schemaType), lcl_fldIdx);
                                   break;
                     case 'P'    :		/* Ignore cases that were     */
                     case 'L'    :		/* tested specially above.    */
                                   break;

                     default     : if (p_shTblDataTypeBINTABLEmap[lcl_heapTypeIdx].fldType == lcl_alignType)
                                      {
                                      lcl_fldInfo[lcl_fldIdx].heap.dataType= p_shTblDataTypeBINTABLEmap[lcl_heapTypeIdx].dataType;
                                      lcl_typeIdx  = lcl_heapTypeIdx;
                                      lcl_forBreak = 1;	/* Found a match      */
                                      }
                                   break;
                     }
                  if (lcl_forBreak != 0)
                     {
                     break;
                     }
                  }   /* switch (p_shTblDataTypeBINTABLEmap[lcl_heapTypeIdx].dataType) */
               }   /* for (lcl_heapTypeIdx = 0, lcl_forBreak = 0; ; lcl_heapTypeIdx++) */
            }

         lcl_fldInfo[lcl_fldIdx].heap.swapIni = (lcl_fldInfo[lcl_fldIdx].heap.swap == SH_FITS_NOSWAP)
                                              ?  0
                                              :  lcl_fldInfo[lcl_fldIdx].heap.size - 1;

         /*
          * Compute heap space requirements.
          *
          *   o   Character strings have the last character dropped, to counteract the
          *       reader's addition of a character (to guarantee null-termination).
          *
          *   o   The largest heap datum size is used when figuring out the buffer (lcl_
          *       buf) size.
          */

         p_shTblFldHeapCnt (lcl_array, &lcl_heapNonZero, &lcl_heapCntAll, &lcl_fldInfo[lcl_fldIdx].heapCntMax);

         if (lcl_fldInfo[lcl_fldIdx].heap.dataType == 'A')
            {
            lcl_heapCntAll -= lcl_heapNonZero;		/* Fix as last char in*/
            lcl_fldInfo[lcl_fldIdx].heapCntMax--;	/* str is NOT written.*/
            }

         if (lcl_heapDatumSizeMax < lcl_fldInfo[lcl_fldIdx].heap.size)
            {
             lcl_heapDatumSizeMax = lcl_fldInfo[lcl_fldIdx].heap.size;
            }

         lcl_heapSize += (lcl_heapCntAll * lcl_fldInfo[lcl_fldIdx].heap.incr);
         }
      else
         {
         /*
          * All that remains is a primitive data type (for the ARRAY data area).
          */

         if ((lcl_alignType = shAlignSchemaToAlignMap (lcl_array->data.schemaType)) == SHALIGNTYPE_unknown)
            {
            lcl_status = SH_NOT_SUPPORTED;
            shErrStackPush ("Object schema %s not supported (field %d)",shNameGetFromType (lcl_array->data.schemaType), lcl_fldIdx);
            continue;				/* Onto next field            */
            }
         lcl_fldInfo[lcl_fldIdx].data.size     = lcl_array->data.size;
         lcl_fldInfo[lcl_fldIdx].data.incr     = lcl_array->data.incr;
         lcl_fldInfo[lcl_fldIdx].data.swap     = datumSwap;

         /*
          * Match the alignment type with the table of supported Binary Table
          * data types.
          */

         for (lcl_dataTypeIdx = 0, lcl_forBreak = 0; ; lcl_dataTypeIdx++)
            {
            switch (p_shTblDataTypeBINTABLEmap[lcl_dataTypeIdx].dataType)
               {
               case '\000' : lcl_forBreak = 1;	/* Ran out of data types      */
                             lcl_status = SH_INVOBJ;
                             shErrStackPush ("Object schema %s does not map to Binary Table data type (field %d)",
                                              shNameGetFromType (lcl_array->data.schemaType), lcl_fldIdx);
                             break;
               case 'P'    :			/* Ignore cases that were     */
               case 'L'    :			/* tested specially above.    */
                             break;

               default     : if (p_shTblDataTypeBINTABLEmap[lcl_dataTypeIdx].fldType == lcl_alignType)
                                {
				  if ( (lcl_array->data.schemaType != lcl_schemaTypeCHAR) || 
				       (p_shTblDataTypeBINTABLEmap[lcl_dataTypeIdx].dataType == 'A') )
				    {
				      lcl_fldInfo[lcl_fldIdx].data.dataType = p_shTblDataTypeBINTABLEmap[lcl_dataTypeIdx].dataType;
				      lcl_typeIdx  = lcl_dataTypeIdx;
				      lcl_forBreak = 1;	/* Found a match      */
				    }
				}
                             break;
               }
            if (lcl_forBreak != 0)
               {
               break;
               }
            }
         }

      /* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .  */
      /*
       * Determine whether any flipping of "signedness" is needed for FITS
       * compliance.
       */

      if (lcl_typeIdx != -1)
         {
         if (p_shTblDataTypeBINTABLEmap[lcl_typeIdx].dataType != '\000')
            {
            lcl_fldInfo[lcl_fldIdx].fldTNULLok = p_shTblDataTypeBINTABLEmap[lcl_typeIdx].fldTNULLok;
            lcl_fldInfo[lcl_fldIdx].fldTSCALok = p_shTblDataTypeBINTABLEmap[lcl_typeIdx].fldTSCALok;
            lcl_fldInfo[lcl_fldIdx].fldTZEROok = p_shTblDataTypeBINTABLEmap[lcl_typeIdx].fldTZEROok;

            if (FITSstd == STANDARD)
               {
               if (!p_shTblDataTypeBINTABLEmap[lcl_typeIdx].dataTypeComply)
                  {
                  if (p_shTblFldSignFlip (lcl_schemaType, &lcl_flipSchema, &lcl_flipTSCAL, &lcl_flipTZERO) == SH_SUCCESS)
                     {
                     if (   !lcl_fldInfo[lcl_fldIdx].fldTSCALok  || !lcl_fldInfo[lcl_fldIdx].fldTZEROok
                         || (lcl_fldInfo[lcl_fldIdx].fldTSCALset && (lcl_fldInfo[lcl_fldIdx].fldTSCAL != 1.0))
                         || (lcl_fldInfo[lcl_fldIdx].fldTZEROset && (lcl_fldInfo[lcl_fldIdx].fldTZERO != 0.0)) )
                        {
                        lcl_status = SH_NOT_SUPPORTED;
                        shErrStackPush ("Flipping sign bit to get object schema %s impossible: TSCALn/TZEROn in use",
                                         shNameGetFromType (lcl_flipSchema));
                        shErrStackPush ("Object schema %s cannot be made FITS-compliant (field %d)",
                                         shNameGetFromType (lcl_schemaType), lcl_fldIdx);
                        continue;		/* Onto next field            */
                        }
                     lcl_fldInfo[lcl_fldIdx].fldTSCALset = TRUE; lcl_fldInfo[lcl_fldIdx].fldTSCAL = lcl_flipTSCAL;
                     lcl_fldInfo[lcl_fldIdx].fldTZEROset = TRUE; lcl_fldInfo[lcl_fldIdx].fldTZERO = lcl_flipTZERO;

                     /*
                      * Change the TFORMn data type.
                      */

                     if ((lcl_alignType = shAlignSchemaToAlignMap (lcl_flipSchema)) == SHALIGNTYPE_unknown)
                        {
                        lcl_status = SH_NOT_SUPPORTED;
                        shErrStackPush ("Flipped object schema %s not supported",
                                        shNameGetFromType (lcl_flipSchema));
                        shErrStackPush ("Object schema %s cannot be made FITS-compliant (field %d)",
                                         shNameGetFromType (lcl_schemaType), lcl_fldIdx);
                        continue;		/* Onto next field            */
                        }

                     for (lcl_typeIdx = 0, lcl_forBreak = 0; ; lcl_typeIdx++)
                        {
                        switch (p_shTblDataTypeBINTABLEmap[lcl_typeIdx].dataType)
                          {
                           case '\000' : lcl_forBreak = 1; /* Out of data type*/
                                         lcl_status = SH_INVOBJ;
                                         shErrStackPush ("Flipped object schema %s does not map to Binary Table data type",
                                                          shNameGetFromType (lcl_flipSchema));
                                         shErrStackPush ("Object schema %s cannot be made FITS-compliant (field %d)",
                                                          shNameGetFromType (lcl_schemaType), lcl_fldIdx);
                                         break;
                           case 'P'    :	/* Ignore cases that cannot   */
                           case 'L'    :	/* ... be sign flipped.       */
                                         break;

                           default     : if (p_shTblDataTypeBINTABLEmap[lcl_typeIdx].fldType == lcl_alignType)
                                            {
                                            lcl_forBreak = 1;  /* Found a match  */
                                            lcl_fldInfo[lcl_fldIdx].signFlip = TBLFLD_SIGNFLIP;
                                            if (lcl_fldInfo[lcl_fldIdx].data.dataType != 'P')
                                               {
                                                lcl_fldInfo[lcl_fldIdx].data.dataType  =
                                                                      p_shTblDataTypeBINTABLEmap[lcl_typeIdx].dataType;
                                               }
                                            else
                                               {
                                                lcl_fldInfo[lcl_fldIdx].heap.dataType  =
                                                                      p_shTblDataTypeBINTABLEmap[lcl_typeIdx].dataType;
                                               }
                                            }
                                         break;
                           }
                        if (lcl_forBreak != 0)
                           {
                           break;
                           }
                        }
                     }
                  else
                     {
                     lcl_status = SH_NOT_SUPPORTED;
                     shErrStackPush ("Object schema %s cannot be made FITS-compliant (field %d)",
                                      shNameGetFromType (lcl_schemaType), lcl_fldIdx);
                     continue;			/* Onto next field            */
                     }
                  }
               }
            }
         }

      /* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .  */
      /*
       * Adjust for swapping.
       */

      lcl_fldInfo[lcl_fldIdx].data.swapIni = (lcl_fldInfo[lcl_fldIdx].data.swap == SH_FITS_NOSWAP)
                                           ?  0
                                           :  lcl_fldInfo[lcl_fldIdx].data.size - 1;

      lcl_NAXIS1 += (lcl_fldInfo[lcl_fldIdx].data.size * lcl_fldInfo[lcl_fldIdx].primCnt);

      if (lcl_fldInfo[lcl_fldIdx].primCnt < 0)
         {
         lcl_status = SH_INVOBJ;
         shErrStackPush ("Invalid Binary field %d element count (must be non-negative)", lcl_fldIdx);
         }
      }

   if (lcl_status != SH_SUCCESS)
      {
      shErrStackPush ("Unable to write FITS HDU");
      goto rtn_return;
      }

   /* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */
   /*
    * Allocate space for the output buffer.  This is done here, so that if it
    * fails, the FITS header is not written out to the file.
    *
    *   o   A minimum size, shFitsRecSize, is allocated so that I/O is efficient
    *       when zero padding the last FITS record.
    *
    *       Since each heap datum is fwriten out individually, the buffer must
    *       be large enough to handle the largest heap datum.
    */

   lcl_bufSize = (lcl_NAXIS1 >= (shFitsRecSize)) ? lcl_NAXIS1 : ((shFitsRecSize) > lcl_heapDatumSizeMax)
                                                              ?  (shFitsRecSize) : lcl_heapDatumSizeMax;

   if ((lcl_buf = ((unsigned char *)shMalloc (lcl_bufSize))) == ((unsigned char *)0))
      {
      lcl_status = SH_MALLOC_ERR;
      shErrStackPush ("Unable to allocate space for writing Binary Table row");
      goto rtn_return;
      }

   /* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */

   /* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */
   /*
    * Generate the FITS header.
    */
                                                                          lcl_hdrLineCnt = 0;
   shHdrInsertAscii   (&lcl_hdr, "XTENSION",    "BINTABLE", ((char *)0)); lcl_hdrLineCnt++;
   shHdrInsertInt     (&lcl_hdr, "BITPIX",              8,  ((char *)0)); lcl_hdrLineCnt++;
   shHdrInsertInt     (&lcl_hdr, "NAXIS",               2,  ((char *)0)); lcl_hdrLineCnt++;
   shHdrInsertInt     (&lcl_hdr, "NAXIS1",     lcl_NAXIS1,  ((char *)0)); lcl_hdrLineCnt++;
   shHdrInsertInt     (&lcl_hdr, "NAXIS2", tblCol->rowCnt,  ((char *)0)); lcl_hdrLineCnt++;
   shHdrInsertInt     (&lcl_hdr, "PCOUNT",   lcl_heapSize,  ((char *)0)); lcl_hdrLineCnt++;
   shHdrInsertInt     (&lcl_hdr, "GCOUNT",              1,  ((char *)0)); lcl_hdrLineCnt++;
   shHdrInsertInt     (&lcl_hdr, "TFIELDS",   lcl_TFIELDS,  ((char *)0)); lcl_hdrLineCnt++;

   /*
    * Copy Dervish header lines into FITS header.
    *
    *          o   A local copy of the TBLCOL header is made so that certain
    *              keywords can be cleaned out.
    */

   if ((lcl_status = shHdrCopy (&tblCol->hdr, &lcl_tblHdr)) != SH_SUCCESS)
      {
      shErrStackPush ("Error temporarily duplicating TBLCOL header");
      goto rtn_return;
      }

   p_shFitsHdrClean (&lcl_tblHdr);	/* Clean out things we generate       */

   for (lcl_hdrLineIdx = 0; (lcl_hdrLine = lcl_tblHdr.hdrVec[lcl_hdrLineIdx]) != ((char *)0); lcl_hdrLineIdx++)
      {
      shHdrInsertLine (&lcl_hdr, lcl_hdrLineCnt, lcl_hdrLine); lcl_hdrLineCnt++;
      }

   /*
    * Generate per-field keywords.
    */

   for (lcl_fldIdx = 0; lcl_fldIdx < shChainSize(&(tblCol->fld)); lcl_fldIdx++)
      {
      lcl_array = (ARRAY *) shChainElementGetByPos(&(tblCol->fld), lcl_fldIdx);
      lcl_tblFld = (lcl_array->infoType == lcl_schemaTypeTBLFLD) ? ((TBLFLD *)lcl_array->info) : ((TBLFLD *)0);

      /*   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .  */
      /*
       * TFORMn.
       */

      sprintf (lcl_keyword, "TFORM%d", lcl_fldIdx + 1);
      sprintf (lcl_keyAval, "%ld%c", lcl_fldInfo[lcl_fldIdx].elemCnt, lcl_fldInfo[lcl_fldIdx].data.dataType);

      /*
       *   o   Handle heap if it's present.
       *
       *       Some of this code (and routines that it calls) assume that heap
       *       data is 1-dimensional.
       */

      if (lcl_fldInfo[lcl_fldIdx].data.dataType == 'P')
         {
         lcl_err = 0;				/* No localized error yet     */
         if (lcl_array->dimCnt > 1)
            {
            lcl_status = SH_INV_FITS_FILE;  lcl_err = 1;   /* Localized error */
            shErrStackPush ("Multidimensional arrays not supported with TFORM%d 'P' data type", lcl_fldIdx + 1);
            }
         if (lcl_tblFld != ((TBLFLD *)0))
            {
            if (shInSet (lcl_tblFld->Tpres, SH_TBL_TDIM))
               {
               lcl_status = SH_INV_FITS_FILE;  lcl_err = 1;   /* Localized error */
               shErrStackPush ("TDIM%d keyword (multidimensional arrays) not supported with TFORM%d 'P' data type",
                                lcl_fldIdx + 1, lcl_fldIdx + 1);
               }
            if (!shInSet (lcl_tblFld->Tpres, SH_TBL_HEAP))
               {
               lcl_status = SH_INV_FITS_FILE;  lcl_err = 1;   /* Localized error */
               shErrStackPush ("TFORM%d heap information unavailable", lcl_fldIdx + 1);
               }
            }

         if (lcl_err != 0)
            {
            continue;				/* Onto next field upon error */
            }

         if (lcl_fldInfo[lcl_fldIdx].heap.dataType != '\000')
            {
            sprintf (&lcl_keyAval[strlen (lcl_keyAval)], "%c(%ld)", lcl_fldInfo[lcl_fldIdx].heap.dataType,
                                                                   lcl_fldInfo[lcl_fldIdx].heapCntMax);
            }
         }

      shHdrInsertAscii (&lcl_hdr, lcl_keyword, lcl_keyAval, ((char *)0)); lcl_hdrLineCnt++;

      /*   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .  */
      /*
       * TDIMn.
       */

      lcl_TDIMpres = (lcl_array->dimCnt > 2);	/* Ignore row dim.  For 1-D   */
						/* array (not counting row the*/
						/* row index), let TFORM elem */
						/* count handle dimension.    */

      if (lcl_tblFld != ((TBLFLD *)0))	/* Determine if TDIMn was present in  */
         {				/* the FITS file Table was read from. */
         lcl_TDIMpres = (shInSet (lcl_tblFld->Tpres, SH_TBL_TDIM)) ? 1 : lcl_TDIMpres;
         }

      if (lcl_TDIMpres)
         {
         s_shFitsTBLCOLtdimGen (lcl_array, lcl_TDIM);
         sprintf (lcl_keyword, "TDIM%d", lcl_fldIdx + 1);
         shHdrInsertAscii (&lcl_hdr, lcl_keyword, lcl_TDIM, ((char *)0)); lcl_hdrLineCnt++;
         }

      /*   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .  */
      /*
       * Other keywords based on TBLFLD.
       */

#define	lcl_shHdrKeywordInsertWithCheck(keywordType, keyword, keywordFld)\
   if (shInSet (lcl_tblFld->Tpres, SH_TBL_##keywordFld))\
      {\
      lcl_shHdrKeywordInsert(keywordType, keyword, lcl_tblFld->keywordFld);\
      }
#define	lcl_shHdrKeywordInsert(keywordType, keyword, value)\
      sprintf (lcl_keyword, #keyword "%d", lcl_fldIdx + 1);\
      shHdrInsert##keywordType (&lcl_hdr, lcl_keyword, value, ((char *)0)); lcl_hdrLineCnt++

      if (lcl_tblFld != ((TBLFLD *)0))
         {
                                                  lcl_shHdrKeywordInsertWithCheck (Ascii, TTYPE, TTYPE);
                                                  lcl_shHdrKeywordInsertWithCheck (Ascii, TUNIT, TUNIT);
        if (lcl_fldInfo[lcl_fldIdx].fldTSCALset
         && lcl_fldInfo[lcl_fldIdx].fldTSCALok) { lcl_shHdrKeywordInsert          (Dbl,   TSCAL, lcl_fldInfo[lcl_fldIdx].fldTSCAL);}
        if (lcl_fldInfo[lcl_fldIdx].fldTZEROset
         && lcl_fldInfo[lcl_fldIdx].fldTZEROok) { lcl_shHdrKeywordInsert          (Dbl,   TZERO, lcl_fldInfo[lcl_fldIdx].fldTZERO);}
        if (lcl_fldInfo[lcl_fldIdx].fldTNULLok) { lcl_shHdrKeywordInsertWithCheck (Int,   TNULL, TNULLINT); }
                                                  lcl_shHdrKeywordInsertWithCheck (Ascii, TDISP, TDISP);
         }
      }

#undef	lcl_shHdrKeywordInsertWithCheck
#undef	lcl_shHdrKeywordInsert

   /*  .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .  */

   shHdrInsertLine    (&lcl_hdr, lcl_hdrLineCnt, "END");    lcl_hdrLineCnt++;

   /*
    * Write out the FITS header.
    */

   if (lcl_status != SH_SUCCESS)
      {
      goto rtn_return;				/* Abort writing FITS HDU     */
      }

   if (!f_wrhead (lcl_hdr.hdrVec, FITSfp))	/* Write out FITS header      */
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("Error writing BINTABLE header to FITS file");
      goto rtn_return;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Write out the Binary Table RSA (Record Storage Area).
    *
    *   o   Adjust the data, if necessary.
    *
    *          o   LOGICALs are converted from a numeric value to a letter.
    *
    *                    0  -->  F
    *                 != 0  -->  T
    *									  ###
    *              NOTE:  LOGICALs are assumed to be one byte in size.	  ###
    *                     This assumption is "protected" by calling	  ###
    *                     s_shFitsPlatformCheck () by shFitsWrite ().	  ###
    */

   lcl_heapEnd = 0;		/* Put heap data at beginning of heap area    */

   for (lcl_rowIdx = 0; lcl_rowIdx < tblCol->rowCnt; lcl_rowIdx++)
      {
      lcl_bufIdx = 0;				/* Start at beginning of row  */

      for (lcl_fldIdx = 0; ((lcl_fldIdx < shChainSize(&(tblCol->fld))) && (lcl_fldIdx < lcl_TFIELDS)); lcl_fldIdx++)
         {
         lcl_array = (ARRAY *) shChainElementGetByPos(&(tblCol->fld), lcl_fldIdx);

         lcl_datum       = lcl_fldInfo[lcl_fldIdx].data.dataPtr; /* Copied for*/
         lcl_datumSize   = lcl_fldInfo[lcl_fldIdx].data.size;  /* performance */
         lcl_datumIncr   = lcl_fldInfo[lcl_fldIdx].data.incr;
         lcl_datumSwap   = lcl_fldInfo[lcl_fldIdx].data.swap;
         lcl_datumIdxIni = lcl_fldInfo[lcl_fldIdx].data.swapIni;

         if      (lcl_fldInfo[lcl_fldIdx].data.dataType == 'L')
            {
            for (lcl_elemIdx = lcl_fldInfo[lcl_fldIdx].primCnt; lcl_elemIdx > 0; lcl_elemIdx--)
               {
               lcl_buf[lcl_bufIdx++] = (*((unsigned char *)lcl_datum) == 0) ? 'F' : 'T';
               lcl_datum            += lcl_datumIncr;
               }
            }
         else if (lcl_fldInfo[lcl_fldIdx].data.dataType == 'P')
            {
            for (lcl_elemIdx = lcl_fldInfo[lcl_fldIdx].primCnt; lcl_elemIdx > 0; lcl_elemIdx -= 2) /* Hardwired -= 2 ### */
               {
               lcl_heapCnt = ((TBLHEAPDSC *)lcl_datum)->cnt + lcl_fldInfo[lcl_fldIdx].heapCntAdj; /* ... + (adjust for STR) */

               for (lcl_datumIdx  = lcl_datumIdxIni, lcl_datumCtr = lcl_datumSize; lcl_datumCtr > 0;
                    lcl_datumIdx += lcl_datumSwap,   lcl_datumCtr--)
                  {
                  lcl_buf[lcl_bufIdx++] = ((unsigned char *)&lcl_heapCnt)[lcl_datumIdx];	/* Fill datum byte by byte */
                  }

               /*
                * Write out heap offset.  This is adjusted for positioning in
                * the FITS file.
                */

               for (lcl_datumIdx  = lcl_datumIdxIni, lcl_datumCtr = lcl_datumSize; lcl_datumCtr > 0;
                    lcl_datumIdx += lcl_datumSwap,   lcl_datumCtr--)
                  {
                  lcl_buf[lcl_bufIdx++] = ((unsigned char *)&lcl_heapEnd)[lcl_datumIdx];	/* Fill datum byte by byte */
                  }

               lcl_heapEnd += (lcl_heapCnt * lcl_fldInfo[lcl_fldIdx].heap.size);
               lcl_datum   +=  lcl_array->data.incr;
               }
            }
         else
            {
            /*
             * A data type not requiring any adjustments.
             */

            for (lcl_elemIdx = lcl_fldInfo[lcl_fldIdx].primCnt; lcl_elemIdx > 0; lcl_elemIdx--)
               {
               for (lcl_datumIdx  = lcl_datumIdxIni, lcl_datumCtr = lcl_datumSize; lcl_datumCtr > 0;
                    lcl_datumIdx += lcl_datumSwap,   lcl_datumCtr--)
                  {
                  lcl_buf[lcl_bufIdx++] = lcl_datum[lcl_datumIdx];	/* Fill datum byte by byte */
                  }
               lcl_buf[lcl_bufIdx - lcl_datumSize] ^= lcl_fldInfo[lcl_fldIdx].signFlip; /* Flip sign bit? */
               lcl_datum += lcl_datumIncr;
               }
            }
         lcl_fldInfo[lcl_fldIdx].data.dataPtr = lcl_datum; /* Save 4 next row */
         }

      /*
       * Write the data out.
       */

      lcl_FITSfpWritten += (lcl_bufLen  = fwrite (lcl_buf, 1, lcl_NAXIS1, FITSfp));
      if (                  lcl_bufLen !=                     lcl_NAXIS1)
         {
         lcl_status = SH_INV_FITS_FILE;
         shErrStackPush ("Write error");
         goto rtn_return;
         }
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Write out the Binary Table heap area.
    *
    *   o   Adjust the data, if necessary.
    *
    *          o   LOGICALs are converted from a numeric value to a letter.
    *
    *                    0  -->  F
    *                 != 0  -->  T
    *									  ###
    *              NOTE:  LOGICALs are assumed to be one byte in size.	  ###
    *                     This assumption is "protected" by calling	  ###
    *                     s_shFitsPlatformCheck () by shFitsWrite ().	  ###
    *
    * NOTE:  Performance can be poor, as each primitive data type is written
    *        to the FITS file, rather than allocating a larger chunk of space
    *        to store multiple datums.
    */

   if (lcl_heapSize <= 0)
      {
      goto rtn_return;				/* No heap to write out       */
      }

    for (lcl_fldIdx = 0; ((lcl_fldIdx < shChainSize(&(tblCol->fld))) && (lcl_fldIdx < lcl_TFIELDS)); lcl_fldIdx++)
      {
      lcl_array = (ARRAY *) shChainElementGetByPos(&(tblCol->fld), lcl_fldIdx);
      lcl_fldInfo[lcl_fldIdx].data.dataPtr = (lcl_array->data.schemaType == lcl_schemaTypeTBLHEAPDSC) ? lcl_array->data.dataPtr
                                                                                                      : ((unsigned char *)0);
      }

   /*
    *   o   Write out row by row to match how the RSA was written out (facili-
    *       tates the computation of heap offsets in the FITS file).
    */

   for (lcl_rowIdx = 0; lcl_rowIdx < tblCol->rowCnt; lcl_rowIdx++)
      {
      for (lcl_fldIdx = 0; ((lcl_fldIdx < shChainSize(&(tblCol->fld))) && (lcl_fldIdx < lcl_TFIELDS)); lcl_fldIdx++)
         {
         lcl_array = (ARRAY *) shChainElementGetByPos(&(tblCol->fld), lcl_fldIdx);
         if ((lcl_heapDsc = ((TBLHEAPDSC *)lcl_fldInfo[lcl_fldIdx].data.dataPtr))== ((TBLHEAPDSC *)0))
            {
            continue;				/* Not a heap holding field   */
            }
         lcl_fldInfo[lcl_fldIdx].data.dataPtr += lcl_array->data.incr; /* For next row */

         if (((lcl_heapCnt = lcl_heapDsc->cnt) + lcl_fldInfo[lcl_fldIdx].heapCntAdj) <= 0)
            {
            continue;				/* No heap data for this row  */
            }
         if ((lcl_datum   = lcl_heapDsc->ptr) == ((unsigned char *)0))
            {
            continue;				/* No heap data for this row  */
            }
         lcl_datumSize    = lcl_fldInfo[lcl_fldIdx].heap.size; /* Copied for  */
         lcl_datumIncr    = lcl_fldInfo[lcl_fldIdx].heap.incr; /* performance */
         lcl_datumSwap    = lcl_fldInfo[lcl_fldIdx].heap.swap;
         lcl_datumIdxIni  = lcl_fldInfo[lcl_fldIdx].heap.swapIni;

         switch (lcl_fldInfo[lcl_fldIdx].heap.dataType)
            {
            case 'L' :
                       for (lcl_elemIdx = lcl_heapCnt, lcl_bufIdx = 0; lcl_elemIdx > 0; lcl_elemIdx--)
                          {
                          lcl_buf[lcl_bufIdx++] = (*((unsigned char *)lcl_datum) == 0) ? 'F' : 'T';
                          lcl_datum            +=  lcl_datumIncr;
                          lcl_FITSfpWritten    += (lcl_bufLen  = fwrite (lcl_buf, 1, 1, FITSfp));
                          if (                     lcl_bufLen !=                     1)
                             {
                             lcl_status = SH_INV_FITS_FILE;
                             shErrStackPush ("Write error");
                             goto rtn_return;
                             }
                          }
                       break;
            case 'A' :
                       lcl_FITSfpWritten += (lcl_bufLen  = fwrite (lcl_datum, 1, lcl_heapCnt - 1, FITSfp));
                       if (                  lcl_bufLen !=                       lcl_heapCnt - 1)
                          {
                          lcl_status = SH_INV_FITS_FILE;
                          shErrStackPush ("Write error");
                          goto rtn_return;
                          }
                       lcl_datum         += (lcl_heapCnt * lcl_datumIncr);
                       break;
            default  :
                       for (lcl_elemIdx = lcl_heapCnt; lcl_elemIdx > 0; lcl_elemIdx--)
                          {
                          for (lcl_datumIdx  = lcl_datumIdxIni, lcl_datumCtr = lcl_datumSize, lcl_bufIdx = 0; lcl_datumCtr > 0;
                               lcl_datumIdx += lcl_datumSwap,   lcl_datumCtr--)
                             {
                             lcl_buf[lcl_bufIdx++] = lcl_datum[lcl_datumIdx];	/* Fill datum byte by byte */
                             }
                          lcl_buf[lcl_bufIdx - lcl_datumSize] ^= lcl_fldInfo[lcl_fldIdx].signFlip; /* Flip sign bit? */
                          lcl_datum         +=  lcl_datumIncr;
                          lcl_FITSfpWritten += (lcl_bufLen  = fwrite (lcl_buf, 1, lcl_datumSize, FITSfp));
                          if (                  lcl_bufLen !=                     lcl_datumSize)
                             {
                             lcl_status = SH_INV_FITS_FILE;
                             shErrStackPush ("Write error");
                             goto rtn_return;
                             }
                          }
                       break;
            }
         }   /* for (lcl_array = ...) */
      }   /* for (lcl_rowIdx = ...) */

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    */

rtn_return : ;

   /*
    * Clean up the header.  Remove anything added to generate a legitimate FITS
    * header.
    */

   p_shFitsHdrClean (&tblCol->hdr);

   /*
    * Push the file position to the start of the next FITS record by padding
    * with zeros.
    *
    *   o   lcl_bufLen is used for a different purpose.
    */

   if ((lcl_bufLen = lcl_FITSfpWritten % (shFitsRecSize)) != 0)
      {
        lcl_bufLen = (shFitsRecSize) - lcl_bufLen;
        memset (((void *)lcl_buf), 0, lcl_bufLen);	/* Padding constant   */
        if (fwrite (lcl_buf, 1, lcl_bufLen, FITSfp) != lcl_bufLen)
           {
           lcl_status = SH_INV_FITS_FILE;
           shErrStackPush ("Write error");
           shErrStackPush ("FITS file not a multiple of FITS record size (%d bytes)", (shFitsRecSize));
           }
      }

   if (lcl_fldInfo       != ((struct lclFldInfo *)0)) { free (lcl_fldInfo);          }
   if (lcl_buf           != ((unsigned char *)0)) { shFree (lcl_buf);                }
   if (lcl_hdr.hdrVec    != ((HDR_VEC       *)0)) { p_shHdrFreeForVec (&lcl_hdr);    }
   if (lcl_tblHdr.hdrVec != ((HDR_VEC       *)0)) { p_shHdrFreeForVec (&lcl_tblHdr); }

   return (lcl_status);

   }	/* s_shFitsTBLCOLwriteBin */

/******************************************************************************/

static RET_CODE	 s_shFitsTBLCOLtdimGen
   (
   ARRAY		*array,			/* IN:    Array w/ dimensions */
                  char	 TDIM[(shFitsTDIMsize) + 1]  /* OUT:   Result         */
   )

/*++
 * ROUTINE:	 s_shFitsTBLCOLtdimGen
 *
 *	Generate the character string value for a FITS TDIMn keyword.  The
 *	information is taken from the passed ARRAY.  The ARRAY dimensions
 *	are treated as follows:
 *
 *	   o	The first dimension is ignored as it's the Table row count.
 *
 *	   o	If the ARRAY data type is a character string, the last dimension
 *		(fastest varying) is reduced by 1.  This counters the FITS Table
 *		reader's increment by 1 used to guarantee space for a null ter-
 *		minator.
 *
 *	   o	Dimensions are reversed from ARRAY's row-major order to FITS'
 *		column-major order.
 *
 *	The format of the TDIMn value is:
 *
 *	   (dim        , ..., dim )
 *	       dimCnt-1          1
 *
 *	where dim  is the dimension from the ARRAY.
 *	         i
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_TRUNC		Failure.
 *			The number of dimensions (and the character representa-
 *			tion of those dimensions) are too long for the output
 *			string (TDIM).  The output string is truncated (a null
 *			terminator is present), resulting in an invalid TDIMn
 *			value.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* s_shFitsTBLCOLtdimGen */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
                  int	 lcl_TDIMlen;		/* Length of lcl_TDIM string  */
                  int	 lcl_TDIMidx;
                  char	 lcl_number[22 + 1];	/* strlen ("(-9223372036854775808")    */

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    *   o   The first dimension is ignored, as it's the number of rows.
    *
    *   o   Dimensions are reversed from row-major order to FITS' column-major
    *       order.
    *
    *   o   The last dimension, fastest varying, is handled separately.  This is
    *       necessary to properly handle character strings losing one character.
    *       The dropping of the last character mirrors the addition of one char-
    *       acter as a null-terminator when a FITS Table is read in.
    */

   if (array->dimCnt > 1)			/* Ignore row dimension       */
      {
      sprintf (lcl_number, "(%d", (array->data.schemaType == shTypeGetFromName ("STR")) ? array->dim[array->dimCnt - 1] - 1
                                                                                        : array->dim[array->dimCnt - 1]);
      strncpy (TDIM, lcl_number, (shFitsTDIMsize));
      }

   for (lcl_TDIMidx = (array->dimCnt - 2); lcl_TDIMidx > 0; lcl_TDIMidx--)
      {
      sprintf (lcl_number, ",%d", array->dim[lcl_TDIMidx]);
      lcl_TDIMlen = strlen (TDIM);
      strncpy (&TDIM[lcl_TDIMlen], lcl_number, (shFitsTDIMsize) - lcl_TDIMlen);
      }

   if (strlen (TDIM) < (shFitsTDIMsize))
      {
      strcat  (TDIM, ")");			/* Closing parenthesis        */
      }
   else
      {
      lcl_status = SH_TRUNC;
      shErrStackPush ("TDIMn value truncated");
      goto rtn_return;
      }

   /*
    * All done.
    */

rtn_return : ;

   return (lcl_status);

   }	/* s_shFitsTBLCOLtdimGen */

/******************************************************************************/
