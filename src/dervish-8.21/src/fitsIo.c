/*****************************************************************************
******************************************************************************
**
** FILE:
**	fitsIo.c
**
** ABSTRACT:
**	This file contains routines that read/write FITS format files.
**
** ENTRY POINT		SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
** shDirGet             public  Gets the default directory
** shDirSet             public  Sets the default directory
** shFileSpecExpand	public	Expand file specification with defaults
** shRegReadAsFits      public  Reads a FITS file into a region
** shRegWriteAsFits     public  Writes a region in a FITS format file
** shHdrReadAsFits      public  Reads JUST a header from a FITS format file
** shHdrWriteAsFits     public  Writes a header as a FITS format file (no data)
**
** ENVIRONMENT:
**	ANSI C.
**
** REQUIRED PRODUCTS:
**	libfits
**
** AUTHORS:
**	Creation date:  May 7, 1993
**	George Jetson
**	Smokey Stover
**
******************************************************************************
******************************************************************************
*/
#include <string.h>
#include <alloca.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#if !defined(NOTCL)
#include "ftcl.h"
#endif
#include "libfits.h"
#include "region.h"
#include "shCUtils.h"
#include "prvt/region_p.h"
#include "shCErrStack.h"
#include "shCFitsIo.h"
#include "shCHdr.h"
#include "shCRegUtils.h"
#include "shCEnvscan.h"
#include "shCGarbage.h"
#include "shCAssert.h"

#ifdef __sgi    /* for kill prototype */
extern int   kill(pid_t pid, int sig);
extern FILE *fdopen(int, const char *);
#endif

/*============================================================================  
**============================================================================
**
** LOCAL MACROS, DEFINITIONS, ETC.
**
**============================================================================
*/
/* Use the following when transforming the pixels using bzero.  All data types
   are promoted to unsigneds for the transformation */
U8   **pix8VecPtr, **pix8EndVecPtr, *pix8Ptr, *pix8EndPtr;
U16  **pix16VecPtr, **pix16EndVecPtr, *pix16Ptr, *pix16EndPtr;
U32  **pix32VecPtr, **pix32EndVecPtr, *pix32Ptr, *pix32EndPtr;
S8   **pixS8VecPtr, **pixS8EndVecPtr, *pixS8Ptr, *pixS8EndPtr;
S16  **pixS16VecPtr, **pixS16EndVecPtr, *pixS16Ptr, *pixS16EndPtr;
S32  **pixS32VecPtr, **pixS32EndVecPtr, *pixS32Ptr, *pixS32EndPtr;
FL32 **pixFL32VecPtr, **pixFL32EndVecPtr, *pixFL32Ptr, *pixFL32EndPtr;

/*----------------------------------------------------------------------------
**
** LOCAL DEFINITIONS (where needed to provide forward definitions)
*/
static void     resetRegType(REGION *a_regPtr, PIXDATATYPE a_type);
static int      getTypeSize(PIXDATATYPE a_type);
static RET_CODE cmpRegSize(REGION *a_regPtr, PIXDATATYPE a_newType, int a_nr,
			   int a_nc);
static RET_CODE regTest(REGION *a_regPtr);
static RET_CODE subRegTest(REGION *a_regPtr);
static RET_CODE regGetCType(PIXDATATYPE a_filePixType, char **a_pixType);
static RET_CODE regCheckType(PIXDATATYPE a_filePixType,
                             PIXDATATYPE a_regPixType);
static RET_CODE regMatchType(PIXDATATYPE a_pixType, FITSFILETYPE a_fitsType);
static RET_CODE regCheckHdr(REGION *a_regPtr, int *a_hasHeader);
static RET_CODE fitsReadPix(PIXDATATYPE a_pixType, int a_nr, int a_nc,
                            FILE *a_f, char *a_fileName, REGION *a_regPtr,
			    double a_bzero, double a_bscale, int a_transform);
static RET_CODE fitsWritePix(PIXDATATYPE a_pixType, int a_nr, int a_nc,
			     FILE *a_f, char *a_fileName, REGION *a_regPtr);
static RET_CODE fitsXWritePix(PIXDATATYPE a_pixType, int a_nr, int a_nc,
			      FILE *a_f, char *a_fileName, REGION *a_regPtr,
			      double a_bzero);
static RET_CODE fitsGetPixType(int a_bitpix, int a_simple,
                               double a_bscale, double a_bzero,
                               int a_transform, PIXDATATYPE *a_fileType,
			       PIXDATATYPE *a_pixType);
static RET_CODE fitsWriteHdr(char *a_fname, const char *mode,
			     char *a_l_filePtr, FILE **a_f, char **a_hdrVec);
static RET_CODE fitsFreeLine(char *a_keyword);
static RET_CODE fitsEditHdr(FITSFILETYPE a_filetype, int a_transform,
                            PIXDATATYPE filetype);
static RET_CODE fitsCopyHdr(HDR_VEC *a_fromHdrVecPtr,
                            HDR_VEC *a_toHdrVecPtr, int a_hasHeader);
static RET_CODE fitsSubFromHdr(HDR_VEC *a_hdr, double *a_bzero,
			       double *a_bscale, int *a_transform, int *a_nr,
			       int *a_nc, PIXDATATYPE *a_filePixType,
			       PIXDATATYPE *a_pixPreXformType,int naxis3_is_ok);


/*----------------------------------------------------------------------------
**
** GLOBAL VARIABLES
*/
/* Static header vector array to copy the header file into for inspection. This
   is done so the existing header is not destroyed if there is an error. The
   last header line must be a null. */
char     *g_hdrVecTemp[MAXHDRLINE+1];

/* Global area to store the default directories and extensions for reading
   fits files and the pool. */
static struct global_defaultDir
   {
   char   defDir[MAXDDIRLEN];	/* Default directory */
   char	  defExt[MAXDEXTLEN];	/* Default extension */
   } g_defaultDir[DEF_TYPEMAX];

/* Static pixel array to use when the pixels are to be transformed before
   writing out. */
static int g_pixTemp[MAXPIXTEMPSIZE];

/* Pointers into the above array */
U8  *temp8Ptr, *temp8EndPtr;
U16 *temp16Ptr, *temp16EndPtr;
U32 *temp32Ptr, *temp32EndPtr;

/* Our values for bzero and bscale. */
double g_bz8 = -128.0, g_bz16 = 32768.0;
double g_bz32 = 2147483648.0;
double g_bscale = 1.0;
char  g_cbz32[] = "BZERO   =            2147483648.0               ";

/* Store the InfoPtr's here as we need to get at them in multiple places */
REGINFO     *g_regInfoPtr;
MASKINFO    *g_maskInfoPtr;

#ifdef SDSS_LITTLE_ENDIAN
/* The following variables and macros are used to swap bytes.  All data is
   stored on disk in big endian format (FITS standard).  If running on a little
   endian machine, it must be swapped before writin out or after reading in. */

/* Macros used to do the actual swapping. We do not want to write separate
   functions as we need to make the code as fast as possible */
#define SWAPBYTESDCLR(sign, wSize)\
    union {\
	     U8 byte[wSize/8];\
	     sign ## wSize word;\
	   } swap;\
    U8 swapByte;

#define SWAP16BYTES(iWord)\
    swap.word = iWord;\
    swapByte = swap.byte[0];\
    swap.byte[0] = swap.byte[1];\
    swap.byte[1] = swapByte;

#define SWAP32BYTES(iWord)\
    swap.word = iWord;\
    swapByte = swap.byte[0];\
    swap.byte[0] = swap.byte[3];\
    swap.byte[3] = swapByte;\
    swapByte = swap.byte[1];\
    swap.byte[1] = swap.byte[2];\
    swap.byte[2] = swapByte;
#endif



/*============================================================================
**============================================================================
**
** ROUTINE: shDirGet
**
** DESCRIPTION:
**	This file gets the default directory.
**
** RETURN VALUES:
**	0   -   Successful completion.
**	1   -   Failed miserably.
**
** CALLS TO:
**	routineName27
**	routineName36
**	routineName98
**
** GLOBALS REFERENCED:
**	g_myglobal
**
**============================================================================
*/
RET_CODE shDirGet
   (
   DEFDIRENUM   a_type,     /* IN  : Type of default to get */
   char         *a_dir,     /* OUT : Default directory */
   char         *a_ext      /* OUT : Default file extension */
   )   
{
(void )strcpy(a_dir, g_defaultDir[a_type].defDir);
(void )strcpy(a_ext, g_defaultDir[a_type].defExt);
return(SH_SUCCESS);
}

/*============================================================================
**============================================================================
**
** ROUTINE: shDirSet
**
** DESCRIPTION:
**	This file sets the default directory.
**
** RETURN VALUES:
**	0   -   Successful completion.
**	1   -   Failed miserably.
**
** CALLS TO:
**	routineName27
**	routineName36
**	routineName98
**
** GLOBALS REFERENCED:
**	g_myglobal
**
**============================================================================
*/
RET_CODE shDirSet
   (
   DEFDIRENUM   a_type,     /* IN  : Type of default to get */
   char         *a_dir,     /* IN  : Default directory */
   char         *a_ext,     /* IN  : Default file extension */
   int          a_slash     /* IN  : Append "/" after directory (if = 1) */
   )   
{
RET_CODE rstatus;
int l_len;

if ((l_len = strlen(a_dir)) < MAXDDIRLEN)
   {
   rstatus = SH_SUCCESS;

   /* Make sure we have a directory string first */
   if (l_len != 0)
      {
      (void )strcpy(g_defaultDir[a_type].defDir, a_dir);

      /* If there is no '/' on the end of the directory and there is supposed to
         be one, add it. */
      if ((g_defaultDir[a_type].defDir[0] != (char)0) &&
          (g_defaultDir[a_type].defDir[l_len - 1] != '/') && a_slash)
         {
         g_defaultDir[a_type].defDir[l_len++] = '/';
         g_defaultDir[a_type].defDir[l_len]   = (char)0;
         }
      }

   if (a_ext[0] != 0)
      {
      if (strlen(a_ext) < MAXDEXTLEN)
         {(void )strcpy(g_defaultDir[a_type].defExt, a_ext);}
      else
         {
         rstatus = SH_EXT_TOO_LONG;
         shErrStackPush("ERROR : Maximum default file extension length is %i",
                        MAXDEXTLEN);
         }
      }
   }
else
   {
   rstatus = SH_DIR_TOO_LONG;
   shErrStackPush("ERROR : Maximum default directory length is %i",
                  MAXDDIRLEN);
   }


return(rstatus);
}

RET_CODE	   shFileSpecExpand
   (
                  char	 *fileName,	/* IN:    Original file specification */
                  char	**fileSpec,	/* OUT:   File spec w/o translations  */
                  char	**fileTran,	/* OUT:   File spec w/  translations  */
   DEFDIRENUM		  defType	/* IN:    Which default to apply      */
   )

/*++
 * ROUTINE:	   shFileSpecExpand
 *
 *	Generate a full file specification by augmenting it with a default path
 *	and extension.  The tilde (~) metacharacter is translated to the home
 *	directory and environment variables are translated also.
 *
 *	If defType is DEF_NONE, then no defaults are appended, but the transla-
 *	tion of metacharacters and environment variables is still done.
 *
 *	fileSpec can be passed as zero, indicating that only fileTran will be
 *	returned.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_INVARG		Failure.
 *
 *   SH_ENVSCAN_ERR	Failure.
 *
 *   SH_MALLOC_ERR	Failure.
 *			Space could not be temporarily allocated for the file
 *			specification.
 *
 * SIDE EFFECTS:
 *
 *   o	The space for fileSpec is shMalloc ()'ed.  It should be released with
 *	free ().
 *
 *   o	The space for fileTran is allocated by shEnvscan ().  It should be
 *	released with shEnvfree ().
 *
 * SYSTEM CONCERNS:
 *
 *   o	There are no checks to make sure the output buffer, fileSpec, is large
 *	enough to contain the defaulted file specification.
 *--
 ******************************************************************************/

   {	/*   shFileSpecExpand */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
                  char	 lcl_defExt[MAXDEXTLEN+1];
                  char	*lcl_fileSpec = NULL;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   if (((defType < 0) || (defType >= DEF_NUM_OF_ELEMS)) && (defType != DEF_NONE))
      {
      lcl_status = SH_INVARG;
      goto rtn_return;
      }

   if ((lcl_fileSpec = (char*)shMalloc (strlen (fileName) + MAXDDIRLEN + 1 + MAXDEXTLEN + 1)) == ((char *)0))
      {
      lcl_status = SH_MALLOC_ERR;
      shErrStackPush ("Unable to%s shMalloc space for defaulted file specification",
                      (fileSpec == ((char **)0)) ? " temporarily" : "");
      goto rtn_return;
      }

   if (defType != DEF_NONE)
      {
      (void )shDirGet (defType, lcl_fileSpec, lcl_defExt);
      (void )strcat            (lcl_fileSpec, fileName);
      (void )strcat            (lcl_fileSpec, lcl_defExt);
      }
   else
      {
      (void )strcpy            (lcl_fileSpec, fileName);
      }

   /*
    * Perform any translations on metacharacters and environment variables.
    */

   if ((*fileTran = shEnvscan (lcl_fileSpec)) == ((char *)0))
      {
      lcl_status = SH_ENVSCAN_ERR;
      shErrStackPush ("Cannot translate file specification \"%s\"", lcl_fileSpec);
      goto rtn_return;
      }

   /*
    * All done.
    */

rtn_return : ;

   if (fileSpec != ((char **)0))
      {
      *fileSpec  = lcl_fileSpec;
      }
   else
      {
      if (lcl_fileSpec != ((char *)0)) { shFree (lcl_fileSpec); }
      }

   return (lcl_status);

   }	/*   shFileSpecExpand */

/******************************************************************************/

/*============================================================================
**============================================================================
**
** ROUTINE: fitsGetPixType
**
** DESCRIPTION:
**	Construct the pixel data type of the data in a FITS file.
**
** RETURN VALUES:
**	SH_SUCCESS
**	SH_UNK_PIX_TYPE
**
** CALLS TO:
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static RET_CODE fitsGetPixType
   (
   int         a_bitpix,    /* IN:  number of bits per pixel */
   int         a_simple,    /* IN:  =1 if signed, otherwise, unsigned */
   double      a_bscale,    /* IN: value to multiply pixels by */
   double      a_bzero,     /* IN: value to add to pixel */
   int         a_transform, /* IN:  =1 if signed but convert to 
			       unsigned */
   PIXDATATYPE  *a_fileType, /* OUT: resultant data type of file */
   PIXDATATYPE  *a_pixType   /* OUT: data type of pixels before xformation */
   )
{
RET_CODE rstatus = SH_SUCCESS;		/* Assume there is a match */

/* Ok, here are the rules.  If SIMPLE=F, the data is considered to be in the
   unsupported FITS format (either S8, U16 or U32).  If SIMPLE=T AND the values
   of bzero and bscale are equal to the global ones that we deal with, then the
   data are also in FITS unsupported format as mentioned above.  If SIMPLE=T
   and the values of bzero and bscale are different from ours, then we assume
   the data are as follows from the value of BITPIX (U8, S16, S32 or FL32). 
   All of this refers to the data as it will be in the new region, after any
   transformations have been applied to it.  The variable 'a_pixType' refers
   to the type of the data after they have been read in and before any
   transformation.  If there are no transformations, then this is the same as
   a_fileType. */

switch (a_simple)
   {
   /* SIMPLE = T */
   case 1 :
      switch (a_transform)
         {
         case 1 :
            switch (a_bitpix)
               {
               case 8:
                  if ((a_bzero == g_bz8) && (a_bscale == g_bscale))
                     {
		     *a_fileType = TYPE_S8;
		     *a_pixType = TYPE_U8;
		     }
                  else
                     {
		     *a_fileType = TYPE_U8;
		     *a_pixType = *a_fileType;
		     }
                  break;
               case 16:
                  if ((a_bzero == g_bz16) && (a_bscale == g_bscale))
                     {
		     *a_fileType = TYPE_U16;
		     *a_pixType = TYPE_S16;


		     }
                  else
                     {
		     *a_fileType = TYPE_S16;
		     *a_pixType = *a_fileType;
		     }
                  break;
               case 32:
                  if ((a_bzero == g_bz32) && (a_bscale == g_bscale))
                     {
		     *a_fileType = TYPE_U32;
		     *a_pixType = TYPE_S32;
		     }
                  else
                     {
		     *a_fileType = TYPE_S32;
		     *a_pixType = *a_fileType;
		     }
                  break;
               case -32:
                  *a_fileType = TYPE_FL32;
		  *a_pixType = *a_fileType;
                  break;
               default:			/* This is an unknown data type */
                  rstatus = SH_UNK_PIX_TYPE;
                  shErrStackPush("ERROR : FITS file contains unknown datatype, (SIMPLE = %d, BITPIX = %d).",
                                 a_simple, a_bitpix);
               }
            break;
         default :
            /* This is signed data */
            switch (a_bitpix)
               {
               case 8:
                  *a_fileType = TYPE_U8;
		  *a_pixType = *a_fileType;
                  break;
               case 16:
                  *a_fileType = TYPE_S16;
		  *a_pixType = *a_fileType;
                  break;
               case 32:
                  *a_fileType = TYPE_S32;
		  *a_pixType = *a_fileType;
                  break;
               case -32:
                  *a_fileType = TYPE_FL32;
		  *a_pixType = *a_fileType;
                  break;
               default:			/* This is an unknown data type */
                  rstatus = SH_UNK_PIX_TYPE;
                  shErrStackPush("ERROR : FITS file contains unknown datatype, (SIMPLE = %d, BITPIX = %d).",
                                 a_simple, a_bitpix);
            break;
               }
         }
      break;
   /* SIMPLE = F */
   default :
      switch (a_bitpix)
         {
         case 8:
            *a_fileType = TYPE_S8;
	    *a_pixType = *a_fileType;
            break;
         case 16:
            *a_fileType = TYPE_U16;
	    *a_pixType = *a_fileType;
            break;
         case 32:
            *a_fileType = TYPE_U32;
	    *a_pixType = *a_fileType;
            break;
         default:			/* This is an unknown data type */
            rstatus = SH_UNK_PIX_TYPE;
            shErrStackPush("ERROR : FITS file contains unknown datatype, (SIMPLE = %d, BITPIX = %d).",
                         a_simple, a_bitpix);
         }
   }

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: p_shFitsDelHdrLines
**
** DESCRIPTION:
**	Remove the lines with the following keywords from the header -
**	   SIMPLE, BITPIX, NAXIS, NAXIS1, NAXIS2, BZERO, BSCALE, UNSIGNED,
**	   SDSS, END
**
** RETURN VALUES:
**	SH_SUCCESS
**
** CALLS TO:
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
RET_CODE p_shFitsDelHdrLines
   (
   HDR_VEC *a_hdr,	/* MOD: header vector */
   int        a_simple	/* IN : specifies signed or unsigned data */
   )
{
RET_CODE rstatus = SH_SUCCESS;

/* Delete the keywords from the lowest one up, as the libfits code will then
   move up the rest of the lines to fill in the blank and starting with the
   last keyword will mean the least number of moves. */

(void )f_kdel(a_hdr, "END");

if (!a_simple)
   {
   /* This is an unsigned header and it has the UNSIGNED keyword in it */
   (void )f_kdel(a_hdr, "UNSIGNED");
   (void )f_kdel(a_hdr, "SIGNED");
   (void )f_kdel(a_hdr, "SDSS");
   }
else
   {
   /* This is a header for signed pixels and it may have the keywords BZERO
      and BSCALE.  It it does not have one then it does not have the other. */
   if (f_kdel(a_hdr, "BSCALE"))
      {(void )f_kdel(a_hdr, "BZERO");}
   }

(void )f_kdel(a_hdr, "NAXIS2");
(void )f_kdel(a_hdr, "NAXIS1");
(void )f_kdel(a_hdr, "NAXIS");
(void )f_kdel(a_hdr, "BITPIX");
(void )f_kdel(a_hdr, "SIMPLE");

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: p_shFitsAddToHdr
**
** DESCRIPTION:
**	Add the lines with the following keywords to the header -
**	   SIMPLE, BITPIX, NAXIS, NAXIS1, NAXIS2, BZERO, BSCALE, UNSIGNED,
**	   SDSS, END.  The NAXIS keyword must be added last, as LIBFITS on each
**	   addition, will check if the keyword exists and automatically place
**	   the new line 3 lines down from the NAXIS line.
**
** RETURN VALUES:
**	SH_SUCCESS
**      SH_MISMATCH_NAXIS
**      SH_HEADER_FULL
**
** CALLS TO:
**
** GLOBALS REFERENCED:
**	g_hdrVecTemp
**
**============================================================================
*/
RET_CODE p_shFitsAddToHdr
   (
   int          a_naxis,         /* IN : number of axes (NAXIS) */
   int          a_nr,            /* IN : number of rows */
   int          a_nc,            /* IN : number of columns */
   PIXDATATYPE  a_filePixType,   /* IN : data type of the pixels */
   FITSFILETYPE a_fileType,      /* IN : STANDARD, NONSTANDARD, or XTENSION */
   double       *a_bzero,        /* OUT: zero offset for pix xform */
   double       *a_bscale,       /* OUT: scale factor for pix xform */
   int          *a_transform     /* OUT: flag whether to xform pixels */
   )
{
RET_CODE rstatus = SH_SUCCESS;
char              fline[81];  /* empty FITS header line - must alwasy leave room for at least a NULL */
int               i;
int               lineno = 0; /* insert at first line - add lines backwards*/
int               bitpix = 0;		/* number of bits per pixel */
int               bp8 = 8, bp16 = 16, bp32 = 32, bpfl = -32;

/* Add the keywords from the highest one down, as the libfits code will then
   move down the rest of the lines to create a blank and starting with the
   first keyword will mean the least number of moves. */

/* Will this be a standard (SIMPLE=T) or a non-standard (SIMPLE=F) header?
   Another alternative is IMAGE for an IMAGE XTENSION */
switch (a_fileType)
   {
   case STANDARD:
      (void )f_mklkey(fline, "SIMPLE", TRUE, "");
      break;
   case NONSTANDARD:
      (void )f_mklkey(fline, "SIMPLE", FALSE, "");
      break;
   case IMAGE:
      (void )f_mkakey(fline, "XTENSION", "IMAGE", "");
      break;
   default:
      /* We should never reach here.  If we do, scream */
      shFatal("In p_shFitsAddToHdr: SIMPLE is neither TRUE or FALSE!");
      break;
   }
(void )f_hlins(g_hdrVecTemp, MAXHDRLINE, lineno, fline);	/* add line */
++lineno;

/* Set the value for bitpix depending on the file pixel type */
switch (a_filePixType)
   {
   case (TYPE_U8):
   case (TYPE_S8):
      bitpix = bp8;
      break;
   case (TYPE_U16):
   case (TYPE_S16):
      bitpix = bp16;
      break;
   case (TYPE_U32):
   case (TYPE_S32):
      bitpix = bp32;
      break;
   case (TYPE_FL32):
      bitpix = bpfl;
      break;
   default:
      /* We should never reach here.  If we do, scream */
      shFatal("In p_shFitsAddToHdr: The file pixel type is unknown!!");
      break;
   }

/* Insert the BITPIX line in the header */
(void )f_mkikey(fline, "BITPIX", bitpix, "");
(void )f_hlins(g_hdrVecTemp, MAXHDRLINE, lineno, fline);
++lineno;

/* Insert the NAXIS1 AND NAXIS2 keywords in the header.  Only do this if there
   will be data following the header. */
if (a_naxis != 0)
   {
   (void )f_mkikey(fline, "NAXIS1", a_nc, "");			/* NAXIS1 */
   (void )f_hlins(g_hdrVecTemp, MAXHDRLINE, lineno, fline);
   ++lineno;
   if (a_naxis > 1)
      {
      (void )f_mkikey(fline, "NAXIS2", a_nr, "");		/* NAXIS2 */
      (void )f_hlins(g_hdrVecTemp, MAXHDRLINE, lineno, fline);
      ++lineno;
      }
   }

if(a_fileType == IMAGE) {
   (void )f_mkikey(fline, "PCOUNT", 0, "");
   (void )f_hlins(g_hdrVecTemp, MAXHDRLINE, lineno, fline);
   ++lineno;
   (void )f_mkikey(fline, "GCOUNT", 1, "");
   (void )f_hlins(g_hdrVecTemp, MAXHDRLINE, lineno, fline);
   ++lineno;
}
/* Insert either the BZERO and BSCALE keywords or the UNSIGNED & SDSS keyword
   in the header */
if (a_naxis != 0)
   {
switch (a_fileType)
   {
   case IMAGE:
   case STANDARD:
      /* The values of BSCALE and BZERO depend on the pixel data type and are
         only used in a STANDARD file */
      switch (a_filePixType)
         {
         case (TYPE_S8):
            *a_bzero = g_bz8;
            *a_transform = 1;
            (void )f_mkdkey(fline, "BZERO", *a_bzero, "");
	    /* BZERO line */
            (void )f_hlins(g_hdrVecTemp, MAXHDRLINE, lineno, fline);
            break;
         case (TYPE_U16):
            *a_bzero = g_bz16;
            *a_transform = 1;
            (void )f_mkdkey(fline, "BZERO", *a_bzero, "");
	    /* BZERO line */
            (void )f_hlins(g_hdrVecTemp, MAXHDRLINE, lineno, fline);
            break;
         case (TYPE_U32):
            *a_bzero = g_bz32;
            *a_transform = 1;
            (void )f_mkdkey(fline, "BZERO", *a_bzero, "");
	    /* BZERO line */
            (void )f_hlins(g_hdrVecTemp, MAXHDRLINE, lineno, fline);
            break;
         default:			/* all the signed cases fall to here */
            *a_transform = 0;
         }
      if (*a_transform == 1)
         {
         *a_bscale = g_bscale;		/* return local default value now */

         /* Only add the keywords for unsigned data. */
         ++lineno;
         (void )f_mkdkey(fline, "BSCALE", g_bscale, "");
	 /* BSCALE line */
         (void )f_hlins(g_hdrVecTemp, MAXHDRLINE, lineno, fline);
         }
      break;
   case NONSTANDARD:
      (void )f_hlins(g_hdrVecTemp, MAXHDRLINE, lineno, "SDSS"); /* SDSS line */
      ++lineno;
      switch (a_filePixType)
         {
         case (TYPE_S8):
            (void )f_hlins(g_hdrVecTemp, MAXHDRLINE, lineno, "SIGNED");
            break;
         default :
            (void )f_hlins(g_hdrVecTemp, MAXHDRLINE, lineno, "UNSIGNED");
            break;
         }
      *a_transform = 0;			/* do not transform these either */
      break;
   default:
      /* We should never reach here.  If we do, scream */
      shFatal("In p_shFitsAddToHdr: SIMPLE is neither TRUE or FALSE again!");
      break;
   }
}

/* Now add the END keyword.  This keyword goes after the last line in the
   header. */

/* Find the end of the header */
for (i = 0 ; i <= MAXHDRLINE ; ++i)
   {
   if (g_hdrVecTemp[i] == (char *)0)
      {
      lineno = i;
      break;				/* Get out of for loop */
      }
   }
if (i > MAXHDRLINE)
   {
   /* There is no more room in the header */
   shErrStackPush("ERROR : No more room in header for standard FITS keywords.");
   rstatus = SH_HEADER_FULL;
   }
else
   {
   /* Insert the line in the header */
   (void )f_hlins(g_hdrVecTemp, MAXHDRLINE, lineno, "END");

   /* Insert the NAXIS, keyword in the header */
   lineno = 2;
   (void )f_mkikey(fline, "NAXIS", a_naxis, "");		  /* NAXIS */
   (void )f_hlins(g_hdrVecTemp, MAXHDRLINE, lineno, fline);
   if (   ((a_naxis == 1) && (a_nr != 1))
       || ((a_naxis < 0) || (a_naxis >= 3)) ) /* Only allow NAXIS=(0|1|2) */
      {
      if  ((a_naxis == 1) && (a_nr != 1))
         {
         shErrStackPush(
		      "ERROR : NAXIS=1 specified for multidimensional array.");
         }
      else
         {
         shErrStackPush("ERROR : Only NAXIS=0, 1 or 2 supported.");
         }
      rstatus = SH_MISMATCH_NAXIS;
      }
   }

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: fitsFreeLine
**
** DESCRIPTION:
**	Deallocate the memory allocated with the passed in keyword.
**
** RETURN VALUES:
**	SH_SUCCESS
**
** CALLS TO:
**
** GLOBALS REFERENCED:
**	g_hdrVecTemp
**
**============================================================================
*/
static RET_CODE fitsFreeLine
   (
   char *a_keyword		/* IN:  Pointer to keyword string */
   )
{
RET_CODE rstatus;
int      lineno;		/* start inserting at first line */

/* Find each keyword first, then deallocate the memory it holds. */
if ((lineno = f_lnum(g_hdrVecTemp, a_keyword)) == -1)
   {
   /* Could not find the keyword - something is really messed up */
   rstatus = SH_NO_FITS_KWRD;
   shErrStackPush("ERROR : Could not find the %s keyword in the FITS file header.",
                   a_keyword);
   }
else
   {
   (void )f_hldel(g_hdrVecTemp, lineno);
   rstatus = SH_SUCCESS;
   }

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: fitsEditHdr
**
** DESCRIPTION:
**	Deallocate the memory allocated when the following lines were added
**	to the header:
**	   SIMPLE, BITPIX, NAXIS, NAXIS1, NAXIS2, BZERO, BSCALE, UNSIGNED,
**	   SDSS, END
**
** RETURN VALUES:
**	SH_SUCCESS
**
** CALLS TO:
**
** GLOBALS REFERENCED:
**	g_hdrVecTemp
**
**============================================================================
*/
static RET_CODE fitsEditHdr
   (
   FITSFILETYPE a_fileType,	/* IN: either STANDARD or NONSTANDARD */
   int          a_transform,	/* IN: flags if BZERO and BSCALE were used */
   PIXDATATYPE  a_filepixtype	/* IN: flags if UNSIGNED or SIGNED used */
   )
{
RET_CODE rstatus;
int      naxis;
char     naxisKeyword [16]; /* strlen ("NAXISnnn") + safety if nnn --> nn..nn */

/* Find each keyword first, then deallocate the memory it holds.  DO this
   for the keywords that are common to both STANDARD AND NONSTANDARD headers. */

if((rstatus = fitsFreeLine("SIMPLE"))   != SH_SUCCESS &&
   (rstatus = fitsFreeLine("XTENSION")) != SH_SUCCESS) {
   goto rtn_return;
}
if ((rstatus = fitsFreeLine("BITPIX")) != SH_SUCCESS) { goto rtn_return; }
if (f_ikey (&naxis, g_hdrVecTemp, "NAXIS"))
   {
   for ( ; naxis > 0; naxis--)
      {
      (void )sprintf (naxisKeyword, "NAXIS%d", naxis);
      if ((rstatus = fitsFreeLine (naxisKeyword)) != SH_SUCCESS) { goto rtn_return; }
      }
   }
if ((rstatus = fitsFreeLine("NAXIS"))  != SH_SUCCESS) { goto rtn_return; }
if ((rstatus = fitsFreeLine("END"))    != SH_SUCCESS) { goto rtn_return; }

/* Now free the lines with the following keywords depending on fileType:
   BZERO, BSCALE, UNSIGNED, SDSS. */

switch (a_fileType)
   {
   case STANDARD:
        if (a_transform == 1) 
           {
           if ((rstatus = fitsFreeLine("BZERO")) == SH_SUCCESS)
               {rstatus = fitsFreeLine("BSCALE");}
           }
        break;
   case NONSTANDARD:
        switch (a_filepixtype)
           {
           case (TYPE_S8):
                rstatus = fitsFreeLine("SIGNED");
                break;
           default :
                rstatus = fitsFreeLine("UNSIGNED");
           }
           rstatus = fitsFreeLine("SDSS");
           break;
      case IMAGE:
	if ((rstatus = fitsFreeLine("PCOUNT"))  != SH_SUCCESS) {
	   goto rtn_return;
	}
	if ((rstatus = fitsFreeLine("GCOUNT"))  != SH_SUCCESS) {
	   goto rtn_return;
	}
	break;
   default:
        /* We should never reach here.  If we do, scream */
        shFatal("In fitsEditHdr: SIMPLE is neither TRUE or FALSE!");
        break;
   }

rtn_return: ;

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: regTest
**
** DESCRIPTION:
**      Make sure a region can be used for reading in a FITS file.
**
** RETURN VALUES:
**	SH_SUCCESS
**	SH_IS_PHYS_REG
**	SH_IS_SUB_REG
**	SH_HAS_SUB_REG
**	SH_REGINFO_ERR
**
** CALLS TO:
**	shErrStackPush
**	shRegInfoGet
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static RET_CODE regTest
   (
   REGION   *a_regPtr   /* IN:  Pointer to region */
   )
{
RET_CODE    rstatus;

/* Check if this region is ok to use.  To pass the test, the following must be
   true  -  region is not physical
            region is not a sub-region
            region does not have sub-regions */

/* Get the region information structure */
if ((rstatus = shRegInfoGet(a_regPtr, &g_regInfoPtr)) == SH_SUCCESS)
   {
   /* Make sure is not a physical region */
   if ((unsigned int )g_regInfoPtr->isPhysical != 0)
      {
      /* This is a physical region */
      rstatus = SH_IS_PHYS_REG;
      shErrStackPush("ERROR : Region - %s is a physical region, cannot read a FITS file into it.",
                   a_regPtr->name);
      }
/* we are lifting the NO sub region test for TESTING purpose 
   else if ((unsigned int )g_regInfoPtr->isSubReg != 0)
      {
*/      /* This region is really a subregion */
/*      rstatus = SH_IS_SUB_REG;
      shErrStackPush("ERROR : Region - %s is a subregion, cannot read a FITS file into it.",
                   a_regPtr->name);
      }
*/
   else if (g_regInfoPtr->nSubReg != 0)
      {
      /* this region has subRegions associated with it */
      rstatus = SH_HAS_SUB_REG;
      shErrStackPush("ERROR : Region - %s has subregion(s), cannot read a FITS file into it.",
                   a_regPtr->name);
      }
   }
else
   {
   /* Could not get the regInfoPtr structure for this region */
   rstatus = SH_REGINFO_ERR;
   shErrStackPush("ERROR : Could not get the REGINFO structure for Region - %s",
                   a_regPtr->name);
   }

return(rstatus);
} 

/*============================================================================
**============================================================================
**
** ROUTINE: subRegTest
**
** DESCRIPTION:
**      Determine if a region is NOT in fact a sub-region
**
** RETURN VALUES:
**	SH_SUCCESS
**	SH_IS_SUB_REG
**
** CALLS TO:
**	shErrStackPush
**	shRegInfoGet
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static RET_CODE subRegTest
   (
   REGION   *a_regPtr   /* IN:  Pointer to region */
   )
{
RET_CODE    rstatus;

/* Check if this region is ok to use.  To pass the test, the following must be
   true    - region is not a sub-region */
         

/* Get the region information structure */
if ((rstatus = shRegInfoGet(a_regPtr, &g_regInfoPtr)) == SH_SUCCESS)
   {
   if ((unsigned int )g_regInfoPtr->isSubReg != 0)
      {
      /* This region is really a subregion */
      rstatus = SH_IS_SUB_REG;
      shErrStackPush("ERROR : Region - %s is a subregion, cannot read a FITS file of different data size into it.",
                   a_regPtr->name);
      }
   }
else
   {
   /* Could not get the regInfoPtr structure for this region */
   rstatus = SH_REGINFO_ERR;
   shErrStackPush("ERROR : Could not get the REGINFO structure for Region - %s",
                   a_regPtr->name);
   }

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: p_shMaskTest
**
** DESCRIPTION:
**      If a region has a mask attached to it, it is possible that the mask
**      data and vector array will have to be deleted and remalloced if the
**      newly read in region is of a different size than the original one.
**      If the mask is to be deleted, it must first pass the tests specified
**      here.
**
** RETURN VALUES:
**	SH_SUCCESS
**	SH_IS_SUB_MASK
**	SH_HAS_SUB_MASK
**	SH_MASKINFO_ERR
**
** CALLS TO:
**	shErrStackPush
**	shMaskInfoGet
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
RET_CODE p_shMaskTest
   (
   MASK   *a_maskPtr   /* IN:  Pointer to mask */
   )
{
RET_CODE    rstatus;

/* Check if this mask is ok to use.  To pass the test, the following must be
   true  -  mask is not a sub-mask
            mask does not have sub-masks */

/* Get the mask information structure */
if ((rstatus = shMaskInfoGet(a_maskPtr, &g_maskInfoPtr)) == SH_SUCCESS)
   {
   if ((unsigned int )g_maskInfoPtr->isSubMask != 0)
      {
      /* This mask is really a submask */
      rstatus = SH_IS_SUB_MASK;
      shErrStackPush("ERROR : Mask - %s is a submask, cannot delete to resize",
		     a_maskPtr->name);
      }
   else if ((unsigned int )g_maskInfoPtr->nSubMask != 0)
      {
      /* this mask has subMasks associated with it */
      rstatus = SH_HAS_SUB_MASK;
      shErrStackPush("ERROR : Mask - %s has submask(s), cannot delete to resize",
		     a_maskPtr->name);
      }
   }
else
   {
   /* Could not get the maskInfoPtr structure for this mask */
   rstatus = SH_MASKINFO_ERR;
   shErrStackPush("ERROR : Could not get the MASKINFO structure for Mask - %s",
		  a_maskPtr->name);
   }

return(rstatus);
} 

/*============================================================================
**============================================================================
**
** ROUTINE: regGetCType
**
** DESCRIPTION:
**      Return a pointer to a character string representing the region's pixel
**	data type.
**
** RETURN VALUES:
**	SH_SUCCESS
**
** CALLS TO:
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static RET_CODE regGetCType
   (
   PIXDATATYPE a_filePixType,	/* IN: pixel type of pixels in file */
   char **pixType		/* OUT: points to correct char string */
   )
{
RET_CODE rstatus = SH_SUCCESS;
static char *uns8 = "UNSIGNED 8-BIT";
static char *sig8 = "SIGNED 8-BIT";
static char *uns16 = "UNSIGNED 16-BIT";
static char *sig16 = "SIGNED 16-BIT";
static char *uns32 = "UNSIGNED 32-BIT";
static char *sig32 = "SIGNED 32-BIT";
static char *flt = "FLOATING POINT";
static char *unkn = "UNKNOWN";

/* Point the type pointer to the correct character string */
switch (a_filePixType)
   {
   case (TYPE_U8):
      *pixType = uns8;
      break;
   case (TYPE_S8):
      *pixType = sig8;
      break;
   case (TYPE_U16):
      *pixType = uns16;
      break;
   case (TYPE_S16):
      *pixType = sig16;
      break;
   case (TYPE_U32):
      *pixType = uns32;
      break;
   case (TYPE_S32):
      *pixType = sig32;
      break;
   case (TYPE_FL32):
      *pixType = flt;
      break;
   default:
      *pixType = unkn;
   }
return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: regCheckType
**
** DESCRIPTION:
**      Check the data types of the file and the reg to make sure they are the
**	same.
**
** RETURN VALUES:
**	SH_SUCCESS
**	SH_PIX_TYP_NOMATCH
**
** CALLS TO:
**	regGetCType
**	shErrStackPush
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static RET_CODE regCheckType
   (
   PIXDATATYPE a_filePixType,	/* IN: pixel type of pixels in file */
   PIXDATATYPE a_regPixType	/* IN: pixel type of pixels in reg */
   )
{
RET_CODE rstatus;
char     *fpt;
char     *rpt;

if (a_filePixType != a_regPixType)
   {
   /* The types do not match - construct the error message */
   /* Point the file type pointer to the right character string */
   (void )regGetCType(a_filePixType, &fpt);

   /* Point the reg type pointer to the right character string */
   (void )regGetCType(a_regPixType, &rpt);

   shErrStackPush("ERROR : Data Types do not match. File type is %s, Region type is %s.",
                   fpt, rpt);
   rstatus = SH_PIX_TYP_NOMATCH;
   }
else
   {rstatus = SH_SUCCESS;}

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: readU8Transform,   readU8,  readS8Transform,  readS8
**          readU16Transform,  readU16, readS16Transform, readS16, readS16Quick
**          readU23Transform,  readU32, readS32Transform, readS32
**          readFL32Transform, readFL32
**
** DESCRIPTION:
**      Read the pixels in, transform according to bzero/bscale and put them
**      in the region data area.  The routines with the 'Transform' in them
**      are the only ones that do the transform.  The above routines are all
**      grouped together because they somewhat duplicate code.  I have made
**      them separate routines to speed up things so that we can avoid case
**      statments and if statements.
**
** RETURN VALUES:
**	SH_SUCCESS
**	SH_UNK_PIX_TYPE
**	SH_FITS_PIX_READ_ERR
**
** CALLS TO:
**	f_get8
**	f_get16
**	f_get32
**	f_getf
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static RET_CODE readU8Transform
   (
   REGION   *a_regPtr,     /* IN:  Pointer to region. Data is read to here */
   int      a_nr,          /* IN: number of rows of data */
   int      a_nc,          /* IN: number of columns of data */
   FILE     *a_f,          /* IN: file pointee */
   double   a_bzero,       /* IN: used to transform the pixels */
   double   a_bscale       /* IN: used to transform the pixels */
   )
{
RET_CODE fstatus = SH_SUCCESS;
int  i,j;
double value;
U8 *u8Row;              /* Pointer to an 8 bit array */

/* Malloc an array to read the row of pixels in to */
u8Row = (U8 *)alloca(sizeof(U8) * a_nc);

/* Include this extra test to determine if the value of a_bscale is equal to
   our magic value of 1.0.  If it is, then we need not use it in the
   computation as multiplying a numbber by 1.0 will return the original
   number. */
if (a_bscale == g_bscale)
   {
   for (i = 0 ; i < a_nr ; ++i)
      {
      if (!(fstatus = f_get8((int8 *)u8Row, a_nc, a_f)))
	 {break;}

      /* Transform the pixels using bzero and bscale */
      for (j = 0 ; j < a_nc ; ++j)
	 {
	 value = (double )u8Row[j] + a_bzero;
	 shRegPixSetWithDbl(a_regPtr, i, j, value);
	 }
      }
   }
else
   {
   for (i = 0 ; i < a_nr ; ++i)
      {
      if (!(fstatus = f_get8((int8 *)u8Row, a_nc, a_f)))
	 {break;}

      /* Transform the pixels using bzero and bscale */
      for (j = 0 ; j < a_nc ; ++j)
	 {
	 value = ((double )u8Row[j] * a_bscale) + a_bzero;
	 shRegPixSetWithDbl(a_regPtr, i, j, value);
	 }
      }
   }

return(fstatus);
}
/*===========================================================================*/
static RET_CODE readU8
   (
   REGION   *a_regPtr,     /* IN:  Pointer to region. Data is read to here */
   int      a_nr,          /* IN: number of rows of data */
   int      a_nc,          /* IN: number of columns of data */
   FILE     *a_f           /* IN: file pointee */
   )
{
RET_CODE fstatus = SH_SUCCESS;
int  i,j;
U8   *u8Row;              /* Pointer to an 8 bit array */

/* Malloc an array to read the row of pixels in to */
u8Row = (U8 *)alloca(sizeof(U8) * a_nc);

for (i = 0 ; i < a_nr ; ++i)
   {
   if (!(fstatus = f_get8((int8 *)u8Row, a_nc, a_f)))
      {break;}

   /* Move the pixels to the region */
   for (j = 0 ; j < a_nc ; ++j)
      {shRegPixSetWithDbl(a_regPtr, i, j, (double )u8Row[j]);}
   }

return(fstatus);
}
/*===========================================================================*/
static RET_CODE readS8Transform
   (
   REGION   *a_regPtr,     /* IN:  Pointer to region. Data is read to here */
   int      a_nr,          /* IN: number of rows of data */
   int      a_nc,          /* IN: number of columns of data */
   FILE     *a_f,          /* IN: file pointee */
   double   a_bzero,       /* IN: used to transform the pixels */
   double   a_bscale       /* IN: used to transform the pixels */
   )
{
RET_CODE fstatus = SH_SUCCESS;
int  i,j;
double value;
S8   *s8Row;              /* Pointer to an 8 bit signed array */

/* Malloc an array to read the row of pixels in to */
s8Row = (S8 *)alloca(sizeof(S8) * a_nc);

/* Include this extra test to determine if the value of a_bscale is equal to
   our magic value of 1.0.  If it is, then we need not use it in the
   computation as multiplying a numbber by 1.0 will return the original
   number. */
if (a_bscale == g_bscale)
   {
   for (i = 0 ; i < a_nr ; ++i)
      {
      if (!(fstatus = f_get8((int8 *)s8Row, a_nc, a_f)))
	 {break;}

      /* Transform the pixels using bzero and bscale. */
      for (j = 0 ; j < a_nc ; ++j)
	 {
	 value = (double )s8Row[j] + a_bzero;
	 shRegPixSetWithDbl(a_regPtr, i, j, value);
	 }
      }
   }
else
   {
   for (i = 0 ; i < a_nr ; ++i)
      {
      if (!(fstatus = f_get8((int8 *)s8Row, a_nc, a_f)))
	 {break;}

      /* Transform the pixels using bzero and bscale. */
      for (j = 0 ; j < a_nc ; ++j)
	 {
	 value = ((double )s8Row[j] * a_bscale) + a_bzero;
	 shRegPixSetWithDbl(a_regPtr, i, j, value);
	 }
      }
   }

return(fstatus);
}
/*===========================================================================*/
static RET_CODE readS8
   (
   REGION   *a_regPtr,     /* IN:  Pointer to region. Data is read to here */
   int      a_nr,          /* IN: number of rows of data */
   int      a_nc,          /* IN: number of columns of data */
   FILE     *a_f           /* IN: file pointee */
   )
{
RET_CODE fstatus = SH_SUCCESS;
int  i,j;
S8   *s8Row;              /* Pointer to an 8 bit signed array */

/* Malloc an array to read the row of pixels in to */
s8Row = (S8 *)alloca(sizeof(S8) * a_nc);

for (i = 0 ; i < a_nr ; ++i)
   {
   if (!(fstatus = f_get8((int8 *)s8Row, a_nc, a_f)))
      {break;}

   /* Move the pixels to the region */
   for (j = 0 ; j < a_nc ; ++j)
      {shRegPixSetWithDbl(a_regPtr, i, j, (double )s8Row[j]);}
   }

return(fstatus);
}
/*===========================================================================*/
static RET_CODE readU16Transform
   (
   REGION   *a_regPtr,     /* IN:  Pointer to region. Data is read to here */
   int      a_nr,          /* IN: number of rows of data */
   int      a_nc,          /* IN: number of columns of data */
   FILE     *a_f,          /* IN: file pointee */
   double   a_bzero,       /* IN: used to transform the pixels */
   double   a_bscale       /* IN: used to transform the pixels */
   )
{
RET_CODE fstatus = SH_SUCCESS;
int  i,j;
double value;
U16  *u16Row;            /* Pointer to a 16 bit array */

#ifdef SDSS_LITTLE_ENDIAN
/* Perform necessary declarations */
SWAPBYTESDCLR(U, 16);
#endif

/* Malloc an array to read the row of pixels in to */
u16Row = (U16 *)alloca(sizeof(U16) * a_nc);

/* Include this extra test to determine if the value of a_bscale is equal to
   our magic value of 1.0.  If it is, then we need not use it in the
   computation as multiplying a numbber by 1.0 will return the original
   number. */
if (a_bscale == g_bscale)
   {
   for (i = 0 ; i < a_nr ; ++i)
      {
      if (!(fstatus = f_get16((int16 *)u16Row, a_nc, a_f)))
	 {break;}

      /* Transform the pixels using bzero and bscale */
      for (j = 0 ; j < a_nc ; ++j)
	 {
#ifdef SDSS_LITTLE_ENDIAN
	 SWAP16BYTES(u16Row[j]);
	 value = (double )swap.word + a_bzero;
#else
	 value = (double )u16Row[j] + a_bzero;
#endif
	 shRegPixSetWithDbl(a_regPtr, i, j, value);
	 }
      }
   }
else
   {
   for (i = 0 ; i < a_nr ; ++i)
      {
      if (!(fstatus = f_get16((int16 *)u16Row, a_nc, a_f)))
	 {break;}

      /* Transform the pixels using bzero and bscale */
      for (j = 0 ; j < a_nc ; ++j)
	 {
#ifdef SDSS_LITTLE_ENDIAN
	 SWAP16BYTES(u16Row[j]);
	 value = ((double )swap.word * a_bscale) + a_bzero;
#else
	 value = ((double )u16Row[j] * a_bscale) + a_bzero;
#endif
	 shRegPixSetWithDbl(a_regPtr, i, j, value);
	 }
      }
   }

return(fstatus);
}
/*===========================================================================*/
static RET_CODE readU16
   (
   REGION   *a_regPtr,     /* IN:  Pointer to region. Data is read to here */
   int      a_nr,          /* IN: number of rows of data */
   int      a_nc,          /* IN: number of columns of data */
   FILE     *a_f           /* IN: file pointee */
   )
{
RET_CODE fstatus = SH_SUCCESS;
int  i,j;
U16  *u16Row;            /* Pointer to a 16 bit array */

#ifdef SDSS_LITTLE_ENDIAN
/* Perform necessary declarations */
SWAPBYTESDCLR(U, 16);
#endif

/* Malloc an array to read the row of pixels in to */
u16Row = (U16 *)alloca(sizeof(U16) * a_nc);

for (i = 0 ; i < a_nr ; ++i)
   {
   if (!(fstatus = f_get16((int16 *)u16Row, a_nc, a_f)))
      {break;}

   /* Move the pixels to the region */
   for (j = 0 ; j < a_nc ; ++j)
      {
#ifdef SDSS_LITTLE_ENDIAN
      SWAP16BYTES(u16Row[j]);
      shRegPixSetWithDbl(a_regPtr, i, j, (double )swap.word);
#else
      shRegPixSetWithDbl(a_regPtr, i, j, (double )u16Row[j]);
#endif
      }
   }

return(fstatus);
}
/*===========================================================================*/
static RET_CODE readS16Transform
   (
   REGION   *a_regPtr,     /* IN:  Pointer to region. Data is read to here */
   int      a_nr,          /* IN: number of rows of data */
   int      a_nc,          /* IN: number of columns of data */
   FILE     *a_f,          /* IN: file pointee */
   double   a_bzero,       /* IN: used to transform the pixels */
   double   a_bscale       /* IN: used to transform the pixels */
   )
{
RET_CODE fstatus = SH_SUCCESS;
int  i,j;
double value;
S16  *s16Row;            /* Pointer to a 16 bit signed array */

#ifdef SDSS_LITTLE_ENDIAN
/* Perform necessary declarations */
SWAPBYTESDCLR(S, 16);
#endif

/* Malloc an array to read the row of pixels in to */
s16Row = (S16 *)alloca(sizeof(S16) * a_nc);

/* Include this extra test to determine if the value of a_bscale is equal to
   our magic value of 1.0.  If it is, then we need not use it in the
   computation as multiplying a numbber by 1.0 will return the original
   number. */
if (a_bscale == g_bscale)
   {
   for (i = 0 ; i < a_nr ; ++i)
      {
      if (!(fstatus = f_get16((int16 *)s16Row, a_nc, a_f)))
	 {break;}
      
      /* Transform the pixels using bzero and bscale */
      for (j = 0 ; j < a_nc ; ++j)
	 {
#ifdef SDSS_LITTLE_ENDIAN
	 SWAP16BYTES(s16Row[j]);
	 value = (double )swap.word + a_bzero;
#else
	 value = (double )s16Row[j] + a_bzero;
#endif
	 shRegPixSetWithDbl(a_regPtr, i, j, value);
	 }
      }
   }
else
   {
   for (i = 0 ; i < a_nr ; ++i)
      {
      if (!(fstatus = f_get16((int16 *)s16Row, a_nc, a_f)))
	 {break;}
      
      /* Transform the pixels using bzero and bscale */
      for (j = 0 ; j < a_nc ; ++j)
	 {
#ifdef SDSS_LITTLE_ENDIAN
	 SWAP16BYTES(s16Row[j]);
	 value = ((double )swap.word * a_bscale) + a_bzero;
#else
	 value = ((double )s16Row[j] * a_bscale) + a_bzero;
#endif
	 shRegPixSetWithDbl(a_regPtr, i, j, value);
	 }
      }
   }

return(fstatus);
}
/*===========================================================================*/
static RET_CODE readS16Quick
   (
   REGION   *a_regPtr,     /* IN:  Pointer to region. Data is read to here */
   int      a_nr,          /* IN: number of rows of data */
   int      a_nc,          /* IN: number of columns of data */
   FILE     *a_f,          /* IN: file pointee */
   double   a_bzero,       /* NOTUSED */
   double   a_bscale       /* NOTUSED */
   )
{
   RET_CODE fstatus;
   int  i,j;
   unsigned int ubz16;
   U16 *ptr;				/* unalias a_regPtr->rows_u16 */
   
#ifdef SDSS_LITTLE_ENDIAN
   /* Perform necessary declarations */
   SWAPBYTESDCLR(U, 16);
#endif

   ubz16 = (unsigned int )g_bz16;  /* Need a local unsigned int version */
   
   for (i = 0 ; i < a_nr ; ++i) {
	ptr = a_regPtr->rows_u16[i];
	if (!(fstatus = f_get16((int16 *)ptr, a_nc, a_f))) {
	   return(fstatus);
	}
	
	/* Transform the pixels using bzero only */
	for (j = 0 ; j < a_nc ; ++j) {
#ifdef SDSS_LITTLE_ENDIAN
	   SWAP16BYTES(ptr[j]);
	   ptr[j] = swap.word;
#endif
	   ptr[j] ^= ubz16;
	}
     }
   return(SH_SUCCESS);
}
/*===========================================================================*/
static RET_CODE readS16
   (
   REGION   *a_regPtr,     /* IN:  Pointer to region. Data is read to here */
   int      a_nr,          /* IN: number of rows of data */
   int      a_nc,          /* IN: number of columns of data */
   FILE     *a_f           /* IN: file pointee */
   )
{
RET_CODE fstatus = SH_SUCCESS;
int  i,j;
S16  *s16Row;            /* Pointer to a 16 bit signed array */

#ifdef SDSS_LITTLE_ENDIAN
/* Perform necessary declarations */
SWAPBYTESDCLR(S, 16);
#endif

/* Malloc an array to read the row of pixels in to */
s16Row = (S16 *)alloca(sizeof(S16) * a_nc);

for (i = 0 ; i < a_nr ; ++i)
   {
   if (!(fstatus = f_get16((int16 *)s16Row, a_nc, a_f)))
      {break;}
   
   /* Move the pixels to the region */
   for (j = 0 ; j < a_nc ; ++j)
      {
#ifdef SDSS_LITTLE_ENDIAN
      SWAP16BYTES(s16Row[j]);
      shRegPixSetWithDbl(a_regPtr, i, j, (double )swap.word);
#else
      shRegPixSetWithDbl(a_regPtr, i, j, (double )s16Row[j]);
#endif
      }
   }

return(fstatus);
}
/*===========================================================================*/
static RET_CODE readU32Transform
   (
   REGION   *a_regPtr,     /* IN:  Pointer to region. Data is read to here */
   int      a_nr,          /* IN: number of rows of data */
   int      a_nc,          /* IN: number of columns of data */
   FILE     *a_f,          /* IN: file pointee */
   double   a_bzero,       /* IN: used to transform the pixels */
   double   a_bscale       /* IN: used to transform the pixels */
   )
{
RET_CODE fstatus = SH_SUCCESS;
int  i,j;
double value;
U32  *u32Row;            /* Pointer to a 32 bit array */

#ifdef SDSS_LITTLE_ENDIAN
/* Perform necessary declarations */
SWAPBYTESDCLR(U, 32);
#endif

/* Malloc an array to read the row of pixels in to */
u32Row = (U32 *)alloca(sizeof(U32) * a_nc);

/* Include this extra test to determine if the value of a_bscale is equal to
   our magic value of 1.0.  If it is, then we need not use it in the
   computation as multiplying a numbber by 1.0 will return the original
   number. */
if (a_bscale == g_bscale)
   {
   for (i = 0 ; i < a_nr ; ++i)
      {
      if (!(fstatus = f_get32((int32 *)u32Row, a_nc, a_f)))
	 {break;}
   
      /* Transform the pixels using bzero and bscale */
      for (j = 0 ; j < a_nc ; ++j)
	 {
#ifdef SDSS_LITTLE_ENDIAN
	 SWAP32BYTES(u32Row[j]);
	 value = (double )swap.word + a_bzero;
#else
	 value = (double )u32Row[j] + a_bzero;
#endif
	 shRegPixSetWithDbl(a_regPtr, i, j, value);
	 }
      }
   }
else
   {
   for (i = 0 ; i < a_nr ; ++i)
      {
      if (!(fstatus = f_get32((int32 *)u32Row, a_nc, a_f)))
	 {break;}
   
      /* Transform the pixels using bzero and bscale */
      for (j = 0 ; j < a_nc ; ++j)
	 {
#ifdef SDSS_LITTLE_ENDIAN
	 SWAP32BYTES(u32Row[j]);
	 value = ((double )swap.word * a_bscale) + a_bzero;
#else
	 value = ((double )u32Row[j] * a_bscale) + a_bzero;
#endif
	 shRegPixSetWithDbl(a_regPtr, i, j, value);
	 }
      }
   }

return(fstatus);
}
/*===========================================================================*/
static RET_CODE readU32
   (
   REGION   *a_regPtr,     /* IN:  Pointer to region. Data is read to here */
   int      a_nr,          /* IN: number of rows of data */
   int      a_nc,          /* IN: number of columns of data */
   FILE     *a_f           /* IN: file pointee */
   )
{
RET_CODE fstatus = SH_SUCCESS;
int  i,j;
U32  *u32Row;            /* Pointer to a 32 bit array */

#ifdef SDSS_LITTLE_ENDIAN
/* Perform necessary declarations */
SWAPBYTESDCLR(U, 32);
#endif

/* Malloc an array to read the row of pixels in to */
u32Row = (U32 *)alloca(sizeof(U32) * a_nc);

for (i = 0 ; i < a_nr ; ++i)
   {
   if (!(fstatus = f_get32((int32 *)u32Row, a_nc, a_f)))
      {break;}
   
   /* Move the pixels to the region */
   for (j = 0 ; j < a_nc ; ++j)
      {
#ifdef SDSS_LITTLE_ENDIAN
      SWAP32BYTES(u32Row[j]);
      shRegPixSetWithDbl(a_regPtr, i, j, (double)swap.word);
#else
      shRegPixSetWithDbl(a_regPtr, i, j, (double )u32Row[j]);
#endif
      }
   }

return(fstatus);
}
/*===========================================================================*/
static RET_CODE readS32Transform
   (
   REGION   *a_regPtr,     /* IN:  Pointer to region. Data is read to here */
   int      a_nr,          /* IN: number of rows of data */
   int      a_nc,          /* IN: number of columns of data */
   FILE     *a_f,          /* IN: file pointee */
   double   a_bzero,       /* IN: used to transform the pixels */
   double   a_bscale       /* IN: used to transform the pixels */
   )
{
RET_CODE fstatus = SH_SUCCESS;
int  i,j;
double value;
S32  *s32Row;            /* Pointer to a 32 bit signed array */

#ifdef SDSS_LITTLE_ENDIAN
/* Perform necessary declarations */
SWAPBYTESDCLR(S, 32);
#endif

/* Malloc an array to read the row of pixels in to */
s32Row = (S32 *)alloca(sizeof(S32) * a_nc);

/* Include this extra test to determine if the value of a_bscale is equal to
   our magic value of 1.0.  If it is, then we need not use it in the
   computation as multiplying a numbber by 1.0 will return the original
   number. */
if (a_bscale == g_bscale)
   {
   for (i = 0 ; i < a_nr ; ++i)
      {
      if (!(fstatus = f_get32((int32 *)s32Row, a_nc, a_f)))
	 {break;}
   
      /* Transform the pixels using bzero and bscale and move them to region */
      for (j = 0 ; j < a_nc ; ++j)
	 {
#ifdef SDSS_LITTLE_ENDIAN
	 SWAP32BYTES(s32Row[j]);
	 value = (double )swap.word + a_bzero;
#else
	 value = (double )s32Row[j] + a_bzero;
#endif
	 shRegPixSetWithDbl(a_regPtr, i, j, value);
	 }
      }
   }
else
   {
   for (i = 0 ; i < a_nr ; ++i)
      {
      if (!(fstatus = f_get32((int32 *)s32Row, a_nc, a_f)))
	 {break;}
   
      /* Transform the pixels using bzero and bscale and move them to region */
      for (j = 0 ; j < a_nc ; ++j)
	 {
#ifdef SDSS_LITTLE_ENDIAN
	 SWAP32BYTES(s32Row[j]);
	 value = ((double )swap.word * a_bscale) + a_bzero;
#else
	 value = ((double )s32Row[j] * a_bscale) + a_bzero;
#endif
	 shRegPixSetWithDbl(a_regPtr, i, j, value);
	 }
      }
   }

return(fstatus);
}
/*===========================================================================*/
static RET_CODE readS32
   (
   REGION   *a_regPtr,     /* IN:  Pointer to region. Data is read to here */
   int      a_nr,          /* IN: number of rows of data */
   int      a_nc,          /* IN: number of columns of data */
   FILE     *a_f           /* IN: file pointee */
   )
{
RET_CODE fstatus = SH_SUCCESS;
int  i,j;
S32  *s32Row;            /* Pointer to a 32 bit signed array */

#ifdef SDSS_LITTLE_ENDIAN
/* Perform necessary declarations */
SWAPBYTESDCLR(S, 32);
#endif

/* Malloc an array to read the row of pixels in to */
s32Row = (S32 *)alloca(sizeof(S32) * a_nc);

for (i = 0 ; i < a_nr ; ++i)
   {
   if (!(fstatus = f_get32((int32 *)s32Row, a_nc, a_f)))
      {break;}
   
   /* Move the pixels to the region */
   for (j = 0 ; j < a_nc ; ++j)
      {
#ifdef SDSS_LITTLE_ENDIAN
      SWAP32BYTES(s32Row[j]);
      shRegPixSetWithDbl(a_regPtr, i, j, (double )swap.word);
#else
      shRegPixSetWithDbl(a_regPtr, i, j, (double )s32Row[j]);
#endif
      }
   }

return(fstatus);
}
/*===========================================================================*/
static RET_CODE readFL32Transform
   (
   REGION   *a_regPtr,     /* IN:  Pointer to region. Data is read to here */
   int      a_nr,          /* IN: number of rows of data */
   int      a_nc,          /* IN: number of columns of data */
   FILE     *a_f,          /* IN: file pointee */
   double   a_bzero,       /* IN: used to transform the pixels */
   double   a_bscale       /* IN: used to transform the pixels */
   )
{
RET_CODE fstatus = SH_SUCCESS;
int  i,j;
double value;
FL32 *fl32Row;          /* Pointer to a 32 bit array */

#ifdef SDSS_LITTLE_ENDIAN
/* Perform necessary declarations */
SWAPBYTESDCLR(FL, 32);
#endif

/* Malloc an array to read the row of pixels in to */
fl32Row = (FL32 *)alloca(sizeof(FL32) * a_nc);

/* Include this extra test to determine if the value of a_bscale is equal to
   our magic value of 1.0.  If it is, then we need not use it in the
   computation as multiplying a numbber by 1.0 will return the original
   number. */
if (a_bscale == g_bscale)
   {
   for (i = 0 ; i < a_nr ; ++i)
      {
      if (!(fstatus = f_get32((int32 *)fl32Row, a_nc, a_f)))
	 {break;}
   
      /* Transform the pixels using bzero and bscale */
      for (j = 0 ; j < a_nc ; ++j)
	 {
#ifdef SDSS_LITTLE_ENDIAN
	 SWAP32BYTES(fl32Row[j]);
	 value = (double )swap.word + a_bzero;
#else
	 value = (double )fl32Row[j] + a_bzero;
#endif
	 shRegPixSetWithDbl(a_regPtr, i, j, value);
	 }
      }
   }
else
   {
   for (i = 0 ; i < a_nr ; ++i)
      {
      if (!(fstatus = f_get32((int32 *)fl32Row, a_nc, a_f)))
	 {break;}
   
      /* Transform the pixels using bzero and bscale */
      for (j = 0 ; j < a_nc ; ++j)
	 {
#ifdef SDSS_LITTLE_ENDIAN
	 SWAP32BYTES(fl32Row[j]);
	 value = ((double )swap.word * a_bscale) + a_bzero;
#else
	 value = ((double )fl32Row[j] * a_bscale) + a_bzero;
#endif
	 shRegPixSetWithDbl(a_regPtr, i, j, value);
	 }
      }
   }

return(fstatus);
}
/*===========================================================================*/
static RET_CODE readFL32
   (
   REGION   *a_regPtr,     /* IN:  Pointer to region. Data is read to here */
   int      a_nr,          /* IN: number of rows of data */
   int      a_nc,          /* IN: number of columns of data */
   FILE     *a_f           /* IN: file pointee */
   )
{
RET_CODE fstatus = SH_SUCCESS;
int  i,j;
FL32 *fl32Row;          /* Pointer to a 32 bit array */

#ifdef SDSS_LITTLE_ENDIAN
/* Perform necessary declarations */
SWAPBYTESDCLR(FL, 32);
#endif

/* Malloc an array to read the row of pixels in to */
fl32Row = (FL32 *)alloca(sizeof(FL32) * a_nc);

for (i = 0 ; i < a_nr ; ++i)
   {
   if (!(fstatus = f_get32((int32 *)fl32Row, a_nc, a_f)))
      {break;}
   
   /* Move the pixels to the region */
   for (j = 0 ; j < a_nc ; ++j)
      {
#ifdef SDSS_LITTLE_ENDIAN
      SWAP32BYTES(fl32Row[j]);
      shRegPixSetWithDbl(a_regPtr, i, j, (double )swap.word);
#else
      shRegPixSetWithDbl(a_regPtr, i, j, (double )fl32Row[j]);
#endif
      }
   }

return(fstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: fitsReadPix
**
** DESCRIPTION:
**      Read the FITS pixels into a region.
**
** RETURN VALUES:
**	SH_SUCCESS
**	SH_UNK_PIX_TYPE
**	SH_FITS_PIX_READ_ERR
**
** CALLS TO:
**	shErrStackPush
**	regGetCType
**	f_get8
**	f_get16
**	f_get32
**	f_getf
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static RET_CODE fitsReadPix
   (
   PIXDATATYPE a_pixType,  /* IN: pixel type of data */
   int      a_nr,          /* IN: number of rows of data */
   int      a_nc,          /* IN: number of columns of data */
   FILE     *a_f,          /* IN: file pointee */
   char     *a_fileName,   /* IN: name of file */
   REGION   *a_regPtr,     /* IN: Pointer to region. Data is read to here */
   double   a_bzero,       /* IN: Used when transforming the pixels. */
   double   a_bscale,      /* IN: Used when transforming the pixels. */
   int      a_transform    /* IN: If = 1, means data needs to be transformed */
   )
{
RET_CODE rstatus = SH_SUCCESS;
RET_CODE fstatus = 0;
int  i, j;
char *fpt;
U8   **u8Ptr;              /* Pointer to an 8 bit array */
U16  **u16Ptr;             /* Pointer to a 16 bit array */
U32  **u32Ptr;             /* Pointer to a 32 bit array */
S8   **s8Ptr;              /* Pointer to an 8 bit signed array */
S16  **s16Ptr;             /* Pointer to a 16 bit signed array */
S32  **s32Ptr;             /* Pointer to a 32 bit signed array */
FL32 **fl32Ptr;            /* Pointer to a 32 bit array */

/* Check to see that we have anything to read in first */
if ((a_nr * a_nc) == 0)
   {return(rstatus);}

/* Well. we now have to read in the pixels.  We are reading in from any of the
   following [U8, S8, U16, S16, U32, S32, FL32] to anyone of the following
   [U8, S8, U16, S16, U32, S32, Fl32].  In cases where the input pixel type is
   the same as the reg pixel type [i.e. U32 and U32], there is no problem.
   However, when they are not the same type, then we need to read into a
   buffer of the same size as the input pixels and then transfer to the reg
   buffer using shRegPixSetWithDbl which does the math correctly to convert
   between pixel types. We will try to make things as fast as possible by not
   combibning code into functions or putting in extra case statements. */
switch (a_pixType)               /* Size/type of pixels on disk */
   {
   case (TYPE_U8):
     switch (a_regPtr->type)     /* Size/type of pixels in memory */
	{
        case (TYPE_U8):                  /* Pixels are of type U8 */
	  /* Either do just the read or the read and then transform
	     the pixels */
	  u8Ptr = a_regPtr->rows_u8;
	  if (a_transform == 1)
	     {
	     for (i = 0 ; i < a_nr ; ++i)
		{
		if (!(fstatus = f_get8((int8 *)u8Ptr[i], a_nc, a_f)))
		   {break;}

		/* Transform the pixels using bzero and bscale */
		for (j = 0 ; j < a_nc ; ++j)
		   {u8Ptr[i][j] = (U8 )(((double )u8Ptr[i][j] * a_bscale) 
				     + a_bzero);}
		}
	     }
	  else
	     {
	     for (i = 0 ; i < a_nr ; ++i)
		{
		if (!(fstatus = f_get8((int8 *)u8Ptr[i], a_nc, a_f)))
		   {break;}
		}
	     }
	  break;
	case (TYPE_S8):                  /* Pixels are of type U8 */
	case (TYPE_U16):
	case (TYPE_S16):
	case (TYPE_U32):
	case (TYPE_S32):
	case (TYPE_FL32):
	  /* Call the routine that will do the read/transform if we need to
	     do a transform.  Else call the routine to just do the read. */
	  if (a_transform == 1)
	     {fstatus = readU8Transform(a_regPtr, a_nr, a_nc, a_f, a_bzero,
					a_bscale);}
	  else
	     {fstatus = readU8(a_regPtr, a_nr, a_nc, a_f);}
	  break;
        case (TYPE_FL64):
          shFatal("In fitsReadPix: TYPE_FL64 is unsupported!!");
	  break;
	}
     break;
   case (TYPE_S8):
     switch (a_regPtr->type)     /* Size/type of pixels in memory */
	{
	case (TYPE_S8):                  /* Pixels are of type S8 */
	  /* Either do just the read or the read and then transform
	     the pixels */
	  s8Ptr = a_regPtr->rows_s8;
	  if (a_transform == 1)
	     {
	     for (i = 0 ; i < a_nr ; ++i)
		{
		if (!(fstatus = f_get8((int8 *)s8Ptr[i], a_nc, a_f)))
		   {break;}

		/* Transform the pixels using bzero and bscale */
		for (j = 0 ; j < a_nc ; ++j)
		   {s8Ptr[i][j] = (S8 )(((double )s8Ptr[i][j] * a_bscale) 
				     + a_bzero);}
		}
	     }
	  else
	     {
	     for (i = 0 ; i < a_nr ; ++i)
		{
		if (!(fstatus = f_get8((int8 *)s8Ptr[i], a_nc, a_f)))
		   {break;}
		}
	     }
	  break;
        case (TYPE_U8):                  /* Pixels are of type S8 */
        case (TYPE_U16):
        case (TYPE_S16):
        case (TYPE_U32):
        case (TYPE_S32):
        case (TYPE_FL32):
	  /* Call the routine that will do the read/transform if we need to
	     do a transform.  Else call the routine to just do the read. */
	  if (a_transform == 1)
	     {fstatus = readS8Transform(a_regPtr, a_nr, a_nc, a_f, a_bzero,
					a_bscale);}
	  else
	     {fstatus = readS8(a_regPtr, a_nr, a_nc, a_f);}
	  break;
        case (TYPE_FL64):
          shFatal("In fitsReadPix: TYPE_FL64 is unsupported!!");
	  break;
	}
     break;
   case (TYPE_U16):
     switch (a_regPtr->type)     /* Size/type of pixels in memory */
	{
	case (TYPE_U16):                  /* Pixels are of type U16 */
	  {
#ifdef SDSS_LITTLE_ENDIAN
	  /* Perform necessary declarations */
	  SWAPBYTESDCLR(U, 16);
#endif

	  /* Either do just the read or the read and then transform
	     the pixels */
	  u16Ptr = a_regPtr->rows_u16;
	  if (a_transform == 1)
	     {
	     for (i = 0 ; i < a_nr ; ++i)
		{
		if (!(fstatus = f_get16((int16 *)u16Ptr[i], a_nc, a_f)))
		   {break;}

		/* Transform the pixels using bzero and bscale */
		for (j = 0 ; j < a_nc ; ++j)
		   {
#ifdef SDSS_LITTLE_ENDIAN
		   SWAP16BYTES(u16Ptr[i][j]);
		   u16Ptr[i][j] = (U16)(((double )swap.word * a_bscale)
					+ a_bzero);
#else
		   u16Ptr[i][j] = (U16 )(((double )u16Ptr[i][j] * a_bscale) 
					  + a_bzero);
#endif
		   }
		}
	     }
	  else
	     {
	     for (i = 0 ; i < a_nr ; ++i)
		{
		if (!(fstatus = f_get16((int16 *)u16Ptr[i], a_nc, a_f)))
		   {break;}

#ifdef SDSS_LITTLE_ENDIAN
		/* Swap the bytes */
		for (j = 0 ; j < a_nc ; ++j)
		   {
		   SWAP16BYTES(u16Ptr[i][j]);
		   u16Ptr[i][j] = swap.word;
		   }
#endif
		}
	     }
	  break;
	  }
        case (TYPE_U8):                  /* Pixels are of type U16 */
        case (TYPE_S8):
        case (TYPE_S16):
        case (TYPE_U32):
        case (TYPE_S32):
        case (TYPE_FL32):
	  /* Call the routine that will do the read/transform if we need to
	     do a transform.  Else call the routine to just do the read. */
	  if (a_transform == 1)
	     {fstatus = readU16Transform(a_regPtr, a_nr, a_nc, a_f, a_bzero,
					 a_bscale);}
	  else
	     {fstatus = readU16(a_regPtr, a_nr, a_nc, a_f);}
	  break;
        case (TYPE_FL64):
          shFatal("In fitsReadPix: TYPE_FL64 is unsupported!!");
	  break;
	}
     break;
   case (TYPE_S16):
     switch (a_regPtr->type)     /* Size/type of pixels in memory */
       {
       case (TYPE_S16):                  /* Pixels are of type S16 */
	 {
#ifdef SDSS_LITTLE_ENDIAN
	   /* Perform necessary declarations */
	   SWAPBYTESDCLR(S, 16);
#endif
	   
	   /* Either do just the read or the read and then transform
	      the pixels */
	   s16Ptr = a_regPtr->rows_s16;
	   if (a_transform == 1)
	     {
	       for (i = 0 ; i < a_nr ; ++i)
		 {
		   if (!(fstatus = f_get16((int16 *)s16Ptr[i], a_nc, a_f)))
		     {break;}
		   
		   /* Transform the pixels using bzero and bscale */
		   for (j = 0 ; j < a_nc ; ++j)
		     {
#ifdef SDSS_LITTLE_ENDIAN
		       SWAP16BYTES(s16Ptr[i][j]);
		       s16Ptr[i][j] = (S16)(((double )swap.word * a_bscale)
					    + a_bzero);
#else
		       s16Ptr[i][j] = (S16 )(((double )s16Ptr[i][j] * a_bscale) 
					     + a_bzero);
#endif
		     }
		 }
	     }
	   else
	     {
	       for (i = 0 ; i < a_nr ; ++i)
		 {
		   if (!(fstatus = f_get16((int16 *)s16Ptr[i], a_nc, a_f))) {
		       fprintf(stderr,"Failed to read row %d\n", i);
		       break;
		   }
		   
#ifdef SDSS_LITTLE_ENDIAN
		   /* Swap the bytes */
		   for (j = 0 ; j < a_nc ; ++j)
		     {
		       SWAP16BYTES(s16Ptr[i][j]);
		       s16Ptr[i][j] = swap.word;
		     }
#endif
		 }
	     }
	   break;
	 }


       case (TYPE_U8):                  /* Pixels are of type S16 */
       case (TYPE_S8):
       case (TYPE_U32):
       case (TYPE_S32):
       case (TYPE_FL32):
	 
       case (TYPE_U16):
	 if ((a_regPtr->type) == (TYPE_U16)) {
	   
	   /* We treat the case where the pixels on disk are type S16 and
	      the region is of type U16 and BZERO and  BSCALE are equal to
	      the magic values used by the SDSS software.  In this case, 
	      each pixel is read in straight and transformed using BZERO and
	      BSCALE.  No calls to shRegPixSetWithDbl are done.  This
	      should hopefully save time when doing this typical read.  This
	      transformation is typical because pixels come off of the DA in
	      U16 format and are supposed to be written as legal fits
	      files which means transforming them to S16 on write . So this
	      would be useful in the case where we are reading back in a fits
	      file created from data from the DA. */
	   
	   /* Check to see if we have the magic values for BZERO and BSCALE */
	   if ((a_transform == 1) && (a_bzero == g_bz16)
	       && (a_bscale == g_bscale))
	     {
	       /* We have the special case, do the reading here. */
	       fstatus = readS16Quick(a_regPtr, a_nr, a_nc, a_f, a_bzero,
				      a_bscale);
	       break;
	     }
	 }
	 /* Just drop through and do things as for the other cases */
	 
	 
	 
	 

	 
	 /* Call the routine that will do the read/transform if we need to
	    do a transform.  Else call the routine to just do the read. */
	 if (a_transform == 1)
	   {fstatus = readS16Transform(a_regPtr, a_nr, a_nc, a_f, a_bzero,
				       a_bscale);}
	 else
	   {fstatus = readS16(a_regPtr, a_nr, a_nc, a_f);}
	 break;
       case (TYPE_FL64):
	 shFatal("In fitsReadPix: TYPE_FL64 is unsupported!!");
	 break;
       }
     break;
   case (TYPE_U32):
     switch (a_regPtr->type)     /* Size/type of pixels in memory */
	{
	case (TYPE_U32):                  /* Pixels are of type U32 */
	  {
#ifdef SDSS_LITTLE_ENDIAN
	  /* Perform necessary declarations */
	  SWAPBYTESDCLR(U, 32);
#endif

	  /* Either do just the read or the read and then transform
	     the pixels */
	  u32Ptr = a_regPtr->rows_u32;
	  if (a_transform == 1)
	     {
	     for (i = 0 ; i < a_nr ; ++i)
		{
		if (!(fstatus = f_get32((int32 *)u32Ptr[i], a_nc, a_f)))
		   {break;}

		/* Transform the pixels using bzero and bscale */
		for (j = 0 ; j < a_nc ; ++j)
		   {
#ifdef SDSS_LITTLE_ENDIAN
		   SWAP32BYTES(u32Ptr[i][j]);
		   u32Ptr[i][j] = (U32)(((double )swap.word * a_bscale)
					+ a_bzero);
#else
		   u32Ptr[i][j] = (U32 )(((double )u32Ptr[i][j] * a_bscale) +
					   a_bzero);
#endif
		   }
		}
	     }
	  else
	     {
	     for (i = 0 ; i < a_nr ; ++i)
		{
		if (!(fstatus = f_get32((int32 *)u32Ptr[i], a_nc, a_f)))
		   {break;}

#ifdef SDSS_LITTLE_ENDIAN
		/* Swap the bytes */
		for (j = 0 ; j < a_nc ; ++j)
		   {
		   SWAP32BYTES(u32Ptr[i][j]);
		   u32Ptr[i][j] = swap.word;
		   }
#endif
	        }
	     }
	  break;
	  }
        case (TYPE_U8):                  /* Pixels are of type U32 */
        case (TYPE_S8):
        case (TYPE_U16):
        case (TYPE_S16):
        case (TYPE_S32):
        case (TYPE_FL32):
	  /* Call the routine that will do the read/transform if we need to
	     do a transform.  Else call the routine to just do the read. */
	  if (a_transform == 1)
	     {fstatus = readU32Transform(a_regPtr, a_nr, a_nc, a_f, a_bzero,
					 a_bscale);}
	  else
	     {fstatus = readU32(a_regPtr, a_nr, a_nc, a_f);}
	  break;
        case (TYPE_FL64):
          shFatal("In fitsReadPix: TYPE_FL64 is unsupported!!");
	  break;
	}
      break;
   case (TYPE_S32):
     switch (a_regPtr->type)     /* Size/type of pixels in memory */
	{
	case (TYPE_S32):                  /* Pixels are of type S32 */
	  {
#ifdef SDSS_LITTLE_ENDIAN
	  /* Perform necessary declarations */
	  SWAPBYTESDCLR(S, 32);
#endif

	  /* Either do just the read or the read and then transform
	     the pixels */
	  s32Ptr = a_regPtr->rows_s32;
	  if (a_transform == 1)
	     {
	     for (i = 0 ; i < a_nr ; ++i)
		{
		if (!(fstatus = f_get32((int32 *)s32Ptr[i], a_nc, a_f)))
		   {break;}

		/* Transform the pixels using bzero and bscale */
		for (j = 0 ; j < a_nc ; ++j)
		   {
#ifdef SDSS_LITTLE_ENDIAN
		   SWAP32BYTES(s32Ptr[i][j]);
		   s32Ptr[i][j] = (S32)(((double )swap.word * a_bscale)
					+ a_bzero);
#else
		   s32Ptr[i][j] = (S32 )(((double )s32Ptr[i][j] * a_bscale) +
					   a_bzero);
#endif
		   }
		}
	     }
	  else
	     {
	     for (i = 0 ; i < a_nr ; ++i)
		{
		if (!(fstatus = f_get32((int32 *)s32Ptr[i], a_nc, a_f)))
		   {break;}

#ifdef SDSS_LITTLE_ENDIAN
		/* Swap the bytes */
		for (j = 0 ; j < a_nc ; ++j)
		   {
		   SWAP32BYTES(s32Ptr[i][j]);
		   s32Ptr[i][j] = swap.word;
		   }
#endif
		}
	     }
	  break;
	  }
        case (TYPE_U8):                  /* Pixels are of type S32 */
        case (TYPE_S8):
        case (TYPE_U16):
        case (TYPE_S16):
        case (TYPE_U32):
        case (TYPE_FL32):
	  /* Call the routine that will do the read/transform if we need to
	     do a transform.  Else call the routine to just do the read. */
	  if (a_transform == 1)
	     {fstatus = readS32Transform(a_regPtr, a_nr, a_nc, a_f, a_bzero,
					 a_bscale);}
	  else
	     {fstatus = readS32(a_regPtr, a_nr, a_nc, a_f);}
	  break;
        case (TYPE_FL64):
          shFatal("In fitsReadPix: TYPE_FL64 is unsupported!!");
	  break;
	}
      break;
   case (TYPE_FL32):
     switch (a_regPtr->type)     /* Size/type of pixels in memory */
	{
	case (TYPE_FL32):                  /* Pixels are of type FL32 */
	  {
#ifdef SDSS_LITTLE_ENDIAN
	  /* Perform necessary declarations */
	  SWAPBYTESDCLR(FL, 32);
#endif

	  /* Either do just the read or the read and then transform
	     the pixels */
	  fl32Ptr = a_regPtr->rows_fl32;
	  if (a_transform == 1)
	     {
	     for (i = 0 ; i < a_nr ; ++i)
		{
		if (!(fstatus = f_get32((int32 *)fl32Ptr[i], a_nc, a_f)))
		   {break;}

		/* Transform the pixels using bzero and bscale */
		for (j = 0 ; j < a_nc ; ++j)
		   {
#ifdef SDSS_LITTLE_ENDIAN
		   SWAP32BYTES(fl32Ptr[i][j]);
		   fl32Ptr[i][j] = (FL32)(((double )swap.word * a_bscale)
					+ a_bzero);
#else
		   fl32Ptr[i][j] = (FL32 )(((double )fl32Ptr[i][j] *
					     a_bscale) + a_bzero);
#endif
		   }
		}
	     }
	  else
	     {
	     for (i = 0 ; i < a_nr ; ++i)
		{
		if (!(fstatus = f_get32((int32 *)fl32Ptr[i], a_nc, a_f)))
		   {break;}

#ifdef SDSS_LITTLE_ENDIAN
		/* Swap the bytes */
		for (j = 0 ; j < a_nc ; ++j)
		   {
		   SWAP32BYTES(fl32Ptr[i][j]);
		   fl32Ptr[i][j] = swap.word;
		   }
#endif
		}
	     }
	  break;
	  }
        case (TYPE_U8):                  /* Pixels are of type FL32 */
        case (TYPE_S8):
        case (TYPE_U16):
        case (TYPE_S16):
        case (TYPE_U32):
        case (TYPE_S32):
	  /* Call the routine that will do the read/transform if we need to
	     do a transform.  Else call the routine to just do the read. */
	  if (a_transform == 1)
	     {fstatus = readFL32Transform(a_regPtr, a_nr, a_nc, a_f, a_bzero,
					  a_bscale);}
	  else
	     {fstatus = readFL32(a_regPtr, a_nr, a_nc, a_f);}
	  break;
        case (TYPE_FL64):
          shFatal("In fitsReadPix: TYPE_FL64 is unsupported!!");
	  break;
	}
      break;
   case (TYPE_FL64):
      shFatal("In fitsReadPix: TYPE_FL64 is unsupported!!");
      break;
   default:
      /* We should never reach here.  If we do, scream */
      shFatal("In fitsReadPix: The file pixel type is unknown!!");
      break;
   }

/* Check to see if there was an error */
if (!fstatus)
   {
   /* Get the ascii equivalent of the pixel type - then construct the error
      message. */
   (void )regGetCType(a_pixType, &fpt);
   shErrStackPush("ERROR : Unable to read in %d x %d pixels of type %s, from file %s.",
                a_nr, a_nc, fpt, a_fileName);
   rstatus = SH_FITS_PIX_READ_ERR;
   }
else
   {rstatus = SH_SUCCESS;}

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: fitsReadHdr
**
** DESCRIPTION:
**      Read a FITS header.
**
** RETURN VALUES:
**	SH_SUCCESS
**	SH_FITS_OPEN_ERR
**
** CALLS TO:
**	shErrStackPush
**	f_rdfits
**
** GLOBALS REFERENCED:
**	g_hdrVecTemp
**
**============================================================================
*/
static RET_CODE fitsReadHdr
   (
   const char  *a_fname, /* IN:  translated file name */
   FILE  **a_f,		/* OUT: file descriptor pointer */
   char  **a_hdrVec,     /* MOD: vector for array of header line addresses */
   int save_fd				/* have libfits save file descriptor?*/
   )
{
RET_CODE rstatus;
int      i;
char     mode[] = "r";

/* Initialize the storage for the header vector */
for (i = 0 ; i < (MAXHDRLINE+1) ; ++i)
   {a_hdrVec[i] = 0;}

if ((*a_f != NULL) || (*a_f = f_fopen((char *)a_fname, mode)))
   {
   /* Now read the header */
   if (f_rdhead(a_hdrVec, MAXHDRLINE, *a_f))
      {
      /* Save the file information as f_get?? will check for it.
       * Note well that this will require us to call f_fclose or
       * the moral equivalent --- in particular, we cannot allow
       * TCL to simply close a_f without cleaning up first by calling
       * f_lose()
       */
      if (save_fd && !f_save(*a_f, a_hdrVec, mode))
         {
         shErrStackPush("ERROR : Could not save FITS file information.");
         rstatus = SH_FITS_HDR_SAVE_ERR;
         }
      else
         {rstatus = SH_SUCCESS;}
      }
   else
      {
      shErrStackPush("ERROR : Could not read FITS header.");
      rstatus = SH_FITS_HDR_READ_ERR;
      }
   }
else
   {
   shErrStackPush("ERROR : Could not open FITS file : %s", a_fname);
   rstatus = SH_FITS_OPEN_ERR;
   }

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: fitsSubFromHdr
**
** DESCRIPTION:
**      Get the values of and then remove several keywords from the FITS
**	header.  The user does not need to see these.
**
** RETURN VALUES:
**	SH_SUCCESS
**	SH_INV_FITS_FILE
**
** CALLS TO:
**	f_lkey
**	f_ikey
**	fitsGetPixType
**	p_shFitsDelHdrLines
**
** GLOBALS REFERENCED:
**	g_hdrVecTemp  
**
**============================================================================
*/
static RET_CODE fitsSubFromHdr
   (
   HDR_VEC     *a_hdr,           /* MOD: header vector */
   double      *a_bzero,         /* OUT: zero offset for pix xform */
   double      *a_bscale,        /* OUT: scale factor for pix xform */
   int         *a_transform,     /* OUT: flag whether to xform pixels */
   int         *a_nr,            /* OUT: number of rows */
   int         *a_nc,            /* OUT: number of columns */
   PIXDATATYPE *a_filePixType,   /* OUT: data type of the pixels */
   PIXDATATYPE *a_pixPreXformType, /* OUT: data type of pixels before xform */
   int naxis3_is_ok)			/* can NAXIS3 be > 1? */
{
RET_CODE          rstatus = SH_SUCCESS;
int               simple, bitpix, bscale = 0, naxis;
unsigned int      bzero = 0;
char              hline[HDRLINESIZE];		/* line from header */
int naxis3;			/* value of NAXIS3 keyword */

/* Get the number of bits per pixel from the header */ 
(void )f_ikey(&bitpix, a_hdr, "BITPIX");

/* Get the value of the SIMPLE keyword which will tell you if the data is
   signed or unsigned

   IMAGE XTENSION HDUs look like regular PDUs with SIMPLE = T
   replaced by XTENSION = IMAGE; handle this case by setting simple == 1
   */
simple = 0;				/* guilty until proven innocent */
(void )f_lkey(&simple, a_hdr, "SIMPLE");

if(f_akey(hline, a_hdr, "XTENSION") == TRUE) {
   char *blank = strchr(hline, ' ');
   if(blank != NULL) *blank = '\0'; /* trim blanks */
   
   if(strcmp(hline, "IMAGE") != 0) {
      shErrStackPush("Unsupported imaging xtension: %s", hline);
      return(SH_INV_FITS_FILE);
   }

   simple = 1;			/* it's a simple extension */
}


/* If the SIMPLE keyword = F, then the SDSS keyword had better be there as
   well */
if (!(simple) && (!f_getlin(hline, a_hdr, "SDSS")))
   {
   /* SIMPLE = F and Header line could not be found */
   shErrStackPush("ERROR : Invalid FITS File, SIMPLE = F but no SDSS keyword was found.");
   rstatus = SH_INV_FITS_FILE;
   }
else 
   {
   /* Get number of rows (nr) and columns (nc).  If the value of NAXIS is 0,
      then there will be no NAXIS1 and NAXIS2 keywords and a_nc and a_nr will
      both be 0. */
   if (!f_ikey(&naxis, a_hdr, "NAXIS"))
      {
      /* Error - the NAXIS keyword is mandatory. */
      rstatus = SH_NO_NAXIS_KWRD;
      shErrStackPush("ERROR : Invalid FITS File, no NAXIS keyword.\n");
      }
   else
      {
      /* Depending on the value of NAXIS, we should expect a NAXIS1 and
	 maybe an NAXIS2 keyword or neither. */
      switch (naxis)
	 {
         case 0:
	    /* There is no data in this file. */
	    *a_nr = 0;
	    *a_nc = 0;
	    break;
	 case 1:
            *a_nr = 1;
	    if (!f_ikey(a_nc, a_hdr, "NAXIS1"))
	       {
	       /* There should be an NAXIS1 keyword. */
	       rstatus = SH_NO_NAXIS1_KWRD;
	       shErrStackPush("ERROR : Invalid FITS File, no NAXIS1 keyword when NAXIS = %d.\n", naxis);
	       }
	    break;
         case 2:
	 case 3:
	    if(naxis == 3) {
	       if (!f_ikey(&naxis3, a_hdr, "NAXIS3")) {
		  rstatus = SH_BAD_NAXIS3_KWRD;
		  shErrStackPush("ERROR : Invalid FITS File, no NAXIS3 keyword when NAXIS = %d.\n", naxis);
		  break;
	       } else if(naxis3 <= 0) {
		  rstatus = SH_BAD_NAXIS3_KWRD;
		  shErrStackPush("ERROR : Invalid FITS File, NAXIS3 == %d <= 0.\n", naxis3);
	       } else if(naxis3 > 1) {
		  if(!naxis3_is_ok) {
		     shError("WARNING : dervish cannot handle FITS Files with NAXIS3 > 1");
		     shError("          setting NAXIS2 *= NAXIS3; NAXIS2 = 1;");
		  }
	       }
	    }
	    if (!f_ikey(a_nc, a_hdr, "NAXIS1"))
	       {
	       /* There should be an NAXIS1 keyword. */
	       rstatus = SH_NO_NAXIS1_KWRD;
	       shErrStackPush("ERROR : Invalid FITS File, no NAXIS1 keyword when NAXIS = %d.\n", naxis);
               break;
	       }
	    if (!f_ikey(a_nr, a_hdr, "NAXIS2"))
	       {
	       /* There should be an NAXIS2 keyword. */
	       rstatus = SH_NO_NAXIS2_KWRD;
	       shErrStackPush("ERROR : Invalid FITS File, no NAXIS2 keyword when NAXIS = %d.\n", naxis);
	       }	    
               break;
	 default:
	    rstatus = SH_UNSUP_NAXIS_KWRD;
	    shErrStackPush("ERROR : Unsupported NAXIS keyword value NAXIS = %d. (Supported values are 0, 1, 2)\n", naxis);
	    }
      }
      if(naxis > 2) {		/* as warned about above */
	*a_nr *= naxis3; naxis3 = 1;
      }

   /* Make sure we did not get any errors */
   if (rstatus == SH_SUCCESS)
      {
      /* Get the values of BZERO and BSCALE - may have to transform the pixels.
	 If both keywords are present, the data was unsigned but converted
	 before writing. BZERO may be a double, so first try and get it
	 as an integer, and if this does not work, try as a double. */
      if ((f_lnum(a_hdr, "BZERO")) != -1)
	 {
	 /* The keyword does exist */
	 if (f_ikey((int *)&bzero, a_hdr, "BZERO"))
	    {
	    *a_bzero = (double )bzero;	       /* Save as double */
	    
	    /* The two keywords are present together */
	    *a_transform = 1;	               /* Need to transform the data */
	    if ((f_lnum(a_hdr, "BSCALE")) != -1)
	       {
	       /* The keyword does exist */
	       if (!(f_ikey((int *)&bscale, a_hdr, "BSCALE")))
		  {
		  /* It was not an integer try as a double. */
		  if (!(f_dkey((double *)a_bscale, a_hdr, "BSCALE")))
		     {
		     /* The key wasn't there as an integer or double. ERROR! */
		     shErrStackPush("ERROR : Invalid FITS File, BSCALE is stored in an unsupported format.");
		     rstatus = SH_INV_BSCALE_KWRD;
		     }
		  }
	       else
		  {*a_bscale = (double )bscale;}       /* Save as double */
	       }
	    }
	 else 
	    {
	    /* It was not an integer, try as a double. */
	    if ((f_dkey((double *)a_bzero, a_hdr, "BZERO")))
	       {
	       /* BZERO exists, now look for BSCALE in the same way */
	       *a_transform = 1;	/* Need to transform the data */
	       if ((f_lnum(a_hdr, "BSCALE")) != -1)
		  {
		  /* The keyword does exist */
		  if (!(f_ikey((int *)&bscale, a_hdr, "BSCALE")))
		     {
		     /* It was not an integer try as a double */
		     if (!(f_dkey((double *)a_bscale, a_hdr, "BSCALE")))
			{
			/* The key was not there as an integer or a double.
			   ERROR! */
			shErrStackPush("ERROR : Invalid FITS File, BSCALE is stored in an unsupported format.");
			rstatus = SH_INV_BSCALE_KWRD;
			}
		     }
		  else
		     {*a_bscale = (double )bscale;}       /* Save as double */
		  }
	       else
		  {
		  /* The key was not there as an integer or double.  ERROR! */
		  shErrStackPush("ERROR : Invalid FITS File, BZERO exists in header, but BSCALE does not.");
		  rstatus = SH_NO_BSCALE_KWRD;
		  }
	       }
	    else
	       {
	       /* The key was not there as an integer or a double.  ERROR! */
	       shErrStackPush("ERROR : Invalid FITS File, BSCALE is stored in an unsupported format.");
	       rstatus = SH_NO_BZERO_KWRD;
	       }
	    }
	 }
      else
	 {*a_transform = 0;}		/* do not transform the data */

      if(*a_transform) {		/* do we really need to transform? */
	 if(*a_bzero == 0.0 && *a_bscale == 1.0) {
	    *a_transform = 0;
	 }
      }

      /* Get the data type of the pixels */
      if ((rstatus = fitsGetPixType(bitpix, simple, *a_bscale, *a_bzero,
				    *a_transform, a_filePixType,
				    a_pixPreXformType)) == SH_SUCCESS)
	 {
	 /* Remove the lines with the following keywords from the header -
	    SIMPLE, BITPIX, NAXIS, NAXIS1, NAXIS2, BZERO, BSCALE, UNSIGNED, SDSS,
	    END */
	 (void )p_shFitsDelHdrLines(g_hdrVecTemp, simple);
	 }
      }
   }
return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: fitsCopyHdr
**
** DESCRIPTION:
**      Copy the header vectors from one array to another. First the new vector
**	array must be malloced (prior to calling this routine).
**
** RETURN VALUES:
**	SH_SUCCESS
**
** CALLS TO:
**	shErrStackPush
**	regGetCType
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static RET_CODE fitsCopyHdr
   (
   HDR_VEC *a_fromHdrVecPtr, /* IN:  Ptr to header vector with valid ptrs */
   HDR_VEC *a_toHdrVecPtr,   /* IN:  Ptr to empty header vector */
   int        a_hasHeader      /* IN:  TRUE if this region has a header */
   )
{
RET_CODE rstatus = SH_SUCCESS;
int      i = 0;

if (a_hasHeader)
   {
   /* Transfer the vectors */
   while ((a_fromHdrVecPtr[i] != 0) && (i < MAXHDRLINE))
      {
      a_toHdrVecPtr[i] = a_fromHdrVecPtr[i];
      ++i;
      }
   }
else
   {
   /* No header */
   a_toHdrVecPtr[0] = '\0';
   }

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: getTypeSize
**
** DESCRIPTION:
**      Return the size of the entered PIXDATATYPE
**
** RETURN VALUES:
**	size of the pixel data type
**
** CALLS TO:
**      sizeof
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static int getTypeSize
   (
   PIXDATATYPE a_type   /* IN: Pixel type to get the size of. */
   )
{

switch (a_type)
   {
   case TYPE_U8:
     return(sizeof(U8));
   case TYPE_S8:
     return(sizeof(S8));
   case TYPE_U16:
     return(sizeof(U16));
   case TYPE_S16:
     return(sizeof(S16));
   case TYPE_U32:
     return(sizeof(U32));
   case TYPE_S32:
     return(sizeof(S32));
   case TYPE_FL32:
     return(sizeof(FL32));
   case (TYPE_FL64):
     shFatal("In getTypeSize: TYPE_FL64 is unsupported!!");
     break;
   default:
     /* We should never reach here.  If we do, scream */
     shFatal("In getTypeSize: The pixel type is unknown!!");
     break;
   }

return(0);                  /* This line is here to avoid compiler warnings.
			       We actually should never reach this statement
			       as shFatal will core dump. */
}

/*============================================================================
**============================================================================
**
** ROUTINE: resetRegType
**
** DESCRIPTION:
**      Reset the 'pointer to pointer to rows' in the region structure to
**      the new value.  Reset the type to match this new value.
**
** RETURN VALUES:
**
** CALLS TO:
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static void resetRegType
   (
   REGION      *a_regPtr,   /* IN: Pointer to region */
   PIXDATATYPE a_newType    /* IN: type to change region to. */
   )
{
void **rows;                /* Generic rows pointer */
int    size;

/* Get the pointer to the pointer to the rows currently in the region */
rows = p_shRegPtrGet(a_regPtr, &size);

/* Move the address to the new type vector pointer. */
switch (a_newType)
   {
   case TYPE_U8:
     return;            /* We match, there is nothing to do. */
   case TYPE_S8:
     a_regPtr->rows_s8 = (S8 **)rows;
     break;
   case TYPE_U16:
     a_regPtr->rows_u16 = (U16 **)rows;
     break;
   case TYPE_S16:
     a_regPtr->rows_s16 = (S16 **)rows;
     break;
   case TYPE_U32:
     a_regPtr->rows_u32 = (U32 **)rows;
     break;
   case TYPE_S32:
     a_regPtr->rows_s32 = (S32 **)rows;
     break;
   case TYPE_FL32:
     a_regPtr->rows_fl32 = (FL32 **)rows;
     break;
   case TYPE_FL64:
     shFatal("In resetRegType: file TYPE_FL64 is unsupported!!");
     break;
   default:
     /* We should never reach here.  If we do, scream */
     shFatal("In resetRegType: The file pixel type is unknown!!");
     }

/* If the new type was different from the old type, clear out the address
   stored in the old type vector pointer */
if (a_regPtr->type != a_newType)
   {p_shRegPtrZero(a_regPtr);}

/* Reset the region type */
*(PIXDATATYPE *)&a_regPtr->type = a_newType;	/* cast away const */
}

/*============================================================================
**============================================================================
**
** ROUTINE: cmpRegSize
**
** DESCRIPTION:
**      Compare the size of a region with the size of the data in a FITS file
**      to see if they are the same.  To be the same the following criteria
**      must be met -
**          o The region and the FITS data must have the same number of rows.
**          o The region and the FITS data must use the same amount of
**            storage for each row. (i.e. (number of columns) x (pixel size)
**            must be the same.
**
** RETURN VALUES:
**	SH_SUCCESS
**      SH_SIZE_MISMATCH
**
** CALLS TO:
**      getTypeSize
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static RET_CODE cmpRegSize
   (
   REGION      *a_regPtr,   /* IN: Pointer to region */
   PIXDATATYPE a_newType,   /* IN: Type of data after being read in. */
   int         a_nr,        /* IN: NUmber of rows in the FITS file. */
   int         a_nc         /* IN: Number of columns in the FITS file. */
   )
{
RET_CODE  rstatus = SH_SUCCESS;

if (a_regPtr->nrow != a_nr)
   {rstatus = SH_SIZE_MISMATCH;}
else if ((a_regPtr->ncol * getTypeSize(a_regPtr->type)) != 
	 (a_nc * getTypeSize(a_newType)))
   {rstatus = SH_SIZE_MISMATCH;}

return(rstatus); 
}


/*****************************************************************************/
/*
 * Skip a number of HDUs
 */
static int
skip_hdus(int hdu,			/* how many HDUs to skip */
	  const char *fname,		/* filename */
	  FILE **f,			/* file descriptor (or NULL) */
	  char **g_hdrVecTemp)		/* header from file */
{
   int i;
   RET_CODE rstatus = SH_SUCCESS;
   
   for(i = 0; i < hdu; i++) {
      if ((rstatus = fitsReadHdr(fname, f, g_hdrVecTemp, (i == 0 ? 1 : 0))) !=
								  SH_SUCCESS) {
	 break;
      } else {
	 int bitpix = 8;
	 int naxis1 = 0, naxis2 = 0;
	 int gcount = 1, pcount = 0;
	 int nbytes;			/* length of data in bytes */
	 int nseek;			/* number of FITS records to seek */
	 if(f_ikey(&bitpix, g_hdrVecTemp, "BITPIX") != TRUE) {
	    shErrStackPush("failed to read BITPIX");
	    return(SH_GENERIC_ERROR);
	 }
	 if(f_ikey(&naxis1, g_hdrVecTemp, "NAXIS1") != TRUE) {
	    shErrStackPush("failed to read NAXIS1");
	    return(SH_GENERIC_ERROR);
	 }
	 if(f_ikey(&naxis2, g_hdrVecTemp, "NAXIS2") != TRUE) {
	    shErrStackPush("failed to read NAXIS2");
	    return(SH_GENERIC_ERROR);
	 }
	 if(f_ikey(&gcount, g_hdrVecTemp, "GCOUNT") != TRUE) {
	    if(i == 0) {		/* PDU */
	       gcount = 1;
	    } else {
	       shErrStackPush("failed to read GCOUNT");
	       return(SH_GENERIC_ERROR);
	    }
	 }
	 if(f_ikey(&pcount, g_hdrVecTemp, "PCOUNT") != TRUE) {
	    if(i == 0) {		/* PDU */
	       pcount = 0;
	    } else {
	       shErrStackPush("failed to read PCOUNT");
	       return(SH_GENERIC_ERROR);
	    }
	 }
	 
	 nbytes = abs(bitpix)/8*gcount*(pcount + naxis1*naxis2);
	 nseek = nbytes/2880;
	 if(nseek*2880 != nbytes) nseek++;
	 if(fseek(*f, nseek*2880, SEEK_CUR) < 0) {
	    shErrStackPushPerror("skip_hdus: Failed to skip %dth HDU", i);
	    return(SH_GENERIC_ERROR);
	 }
      }
   }

   return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: shRegReadAsFits
**
** DESCRIPTION:
**      Read a FITS file into a region.  If the size of the region (specified
**      in the RegionNew command) is insufficient, the appropriate amount of
**      space is automatically allocated.  In all cases, the existing region
**      is overwritten.
**      If the region is in fact a sub-region, if the size of the region is
**      insufficient the operation is not allowed otherwise the parent region
**      is modified.
**
** RETURN VALUES:
**	SH_SUCCESS
**	SH_UNK_PIX_TYPE
**	SH_IS_PHYS_REG
**	SH_IS_SUB_REG
**	SH_HAS_SUB_REG
**	SH_REGINFO_ERR
**	SH_PIX_TYP_NOMATCH
**
** CALLS TO:
**      shEnvscan
**	shEnvfree
**      cmpRegSize
**	regTest
**	p_shMaskTest
**	shDirGet
**	isalpha
**	strcpy
**	strncat
**	shErrStackPush
**	f_rdfits
**	regCheckType
**	shRegDel
**	shRegNew
**	regGetCType
**
** GLOBALS REFERENCED:
**	g_hdrVecTemp
**
**============================================================================
*/
RET_CODE shRegReadAsFits
   (
   REGION   *a_regPtr,   /* IN:  Pointer to region */
   char     *a_file,     /* IN:  Name of FITS file */
   int      a_checktype, /* IN:  Flag to signify if should match file and
                                 region data type */
   int      a_keeptype,  /* IN:  Flag to signify if must make new region the
			         same type as the old region */
   DEFDIRENUM a_FITSdef, /* IN:  Which default file specs to apply */
   FILE    *a_inPipePtr, /* IN:  If != NULL then is a pipe to read from */
   int      a_readtape,  /* IN:  Flag to indicate that we must fork/exec an
                                  instance of "dd" to read a tape drive */
   int	    naxis3_is_ok, /* IN: don't complain even if NAXIS3 > 1 */
   int      hdu		  /* IN: which HDU to read (0: PDU) */
   )
{
RET_CODE    rstatus;
FILE        *f = NULL;
PIXDATATYPE filePixType, desiredType, pixPreXformType;
int         transform, nr, nc, ddSts;
double      bscale = 0, bzero = 0;
char        *fname = 0;
char        *l_filePtr = 0;
char        *lcl_ddCmd = NULL;
int         tapeFd[2], tapeStderr;
pid_t       tapePid, rPid;
char        tapeBuf;

/* Make sure this region is ok */
if (a_FITSdef == DEF_DEFAULT) {a_FITSdef = DEF_REGION_FILE;}

if ((rstatus = regTest(a_regPtr)) == SH_SUCCESS)
   {
   /* If a_readtape, we shouldn't have an open a_inPipePtr.  If so, complain
      and return.  If not, form a dd command and try a "dd" command. */
   if (a_readtape)
      {
      if (a_inPipePtr != NULL)
	 {
	 rstatus = SH_TAPE_BAD_FILE_PTR;
	 shErrStackPush("ERROR : a_inPipePtr not NULL with a_readtape.");
	 goto the_exit;
         }
      if ((lcl_ddCmd = (char*)shMalloc (strlen (a_file) + 4)) == ((char *)0))
	 {
	 rstatus = SH_MALLOC_ERR;
	 shErrStackPush ("Unable to shMalloc space for dd command");
	 goto the_exit;
         }
      sprintf(lcl_ddCmd, "if=%s", a_file);
      /* OK, here we go.  Open a pipe, fork a child, dup the child's stdout,
	 exec in the child dd, associate parent's pipe end with a stream */
      if (pipe(tapeFd) < 0)
	 {
	 shErrStackPush(strerror(errno));
	 rstatus = SH_TAPE_DD_FAIL;
	 shErrStackPush("ERROR : could not create pipe for dd process.");
	 shFree(lcl_ddCmd);
	 goto the_exit;
         }
      if ( (tapePid = fork()) < 0)
	 {
	 shErrStackPush(strerror(errno));
	 rstatus = SH_TAPE_DD_FAIL;
	 shErrStackPush("ERROR : dd process could not be forked.");
	 shFree(lcl_ddCmd);
	 goto the_exit;
         }
      else if (tapePid > 0)         /* tapePid > 0 - parent process */
	 {
	 close (tapeFd[1]);         /* close the write end of the pipe */
	 f = fdopen(tapeFd[0], "r");  /* attempt to get FILE* from fd */
	 if (f == NULL)
	    {
	    rstatus = SH_TAPE_DD_FAIL;
	    shErrStackPush("ERROR : could not open FILE * for dd process.");
	    shFree(lcl_ddCmd);
	    goto the_exit;
	    }
         }
      else                          /* tapePid == 0 - child process */
	 {
	 close (tapeFd[0]);         /* close the read end of the pipe */
         if (tapeFd[1] != STDOUT_FILENO)    /* dup the write end onto STDOUT */
	    {
	    if (dup2(tapeFd[1],STDOUT_FILENO) != STDOUT_FILENO)
	       {
	       fprintf(stderr,"ERROR : couldn't dup2 in dd child.");
	       _exit(EXIT_FAILURE);
	       }
	    close(tapeFd[1]);      /* don't need this fd anymore */
	    }
#if 1
	 if ((tapeStderr=open("/dev/null",O_WRONLY)) > 0)
	    {
	    dup2(tapeStderr, STDERR_FILENO);
	    close(tapeStderr);
	    }
#endif
	 execlp ("dd", "dd", lcl_ddCmd, "bs=144000", "files=1", (char *)0);  /* do the dd call */
	 _exit(EXIT_FAILURE);       /* if exec fails return an error */
         }
      }
   /* else if we were passed a pipe name then we do not have to do any file spec
      stuff. Also the pipe is already opened. */
   else if (a_inPipePtr == NULL)
      {
      /* Get the full path name to the FITS file */
      if ((rstatus = shFileSpecExpand(a_file, &l_filePtr, &fname, a_FITSdef))
	  != SH_SUCCESS)
         {goto the_exit;}
      }
   else
      {
      /* Use the passed in file descriptor as it is associated with a pipe
	 and is already open */
      f = a_inPipePtr;
      }
/*
 * Skip over HDUs if so requested
 */
      if((rstatus = skip_hdus(hdu, fname, &f, g_hdrVecTemp)) != SH_SUCCESS) {
	 goto the_exit;
      }
   
      {
      /* Read in the fits header to a temp area */
      if ((rstatus = fitsReadHdr(fname, &f, g_hdrVecTemp, (hdu == 0 ? 1 : 0)))
								 == SH_SUCCESS)
         {
         /* Adjust the header by removing some of the keywords */
         if ((rstatus = fitsSubFromHdr(g_hdrVecTemp, &bzero, &bscale,
				       &transform, &nr, &nc, &filePixType,
				       &pixPreXformType, naxis3_is_ok)) != SH_SUCCESS)
            {goto the_exit;}

	 {
	    char buf[81];
	    if(f_akey(buf, g_hdrVecTemp, "XTENSION") == TRUE) {
	       int i;
	       for(i = 0; i < sizeof(buf); i++) { /* trim trailing spaces */
		  if(isspace(buf[i])) {
		     buf[i] = '\0';
		     break;
		  }
	       }
	       
	       if(strcmp(buf, "IMAGE") != 0) {
		  shErrStackPush("Unsupported imaging xtension: %s", buf);
		  rstatus = SH_GENERIC_ERROR;
		  goto the_exit;
	       }
	    }
	 }

         /* If the option 'checktype' was entered on the command line, the FITS
            file data type had better be the same as that of the region */
         if (a_checktype == 1)
            {
            if ((rstatus = regCheckType(filePixType, a_regPtr->type)) !=
                SH_SUCCESS)
               {goto the_exit;}
            }

	 /* If the option 'keeptype' was entered on the command line, the FITS
	    data will be cast to the type of the region. Otherwise change the
	    region type to match that of the FITS file. */
	 if (a_keeptype == 1)
	    {desiredType = a_regPtr->type;}
	 else
	    {desiredType = filePixType;}

	 /* Check to see if the FITS file data is of the same exact size as
	    the current region. */
	 if ((rstatus = cmpRegSize(a_regPtr, desiredType, nr, nc)) ==
	     SH_SUCCESS)
	    {
	    /* Region and FITS file are of the same size (data wise).  Don't
	       delete anything, just read into the current region pixel
	       storage. */

	    /* Reset the pixel type of the region to be the desired type. The
	       pixel vector pointer must be changed first. Also reset the
	       number of columns as this may differ (it is only ncol * sizeof
	       a pixel that must have been the same) */
            a_regPtr->ncol = nc;
	    resetRegType(a_regPtr, desiredType);
	    }
	 else      
	    { 
	    /* We do not allow overwriting a subregion with a different size
	       (data wise). */ 
	    if ((rstatus = subRegTest(a_regPtr)) != SH_SUCCESS) {
	       int i;
	       for (i = 0 ; i <= MAXHDRLINE && g_hdrVecTemp[i] != NULL; i++) {
		  shFree(g_hdrVecTemp[i]);
		  g_hdrVecTemp[i] = NULL;
	       }

	       goto the_exit;
	    }

	    /* Since we have to delete the mask, make sure it is not a
	       sub mask or does not have sub masks. */
	    if ((unsigned int )g_regInfoPtr->hasMask == 1)
	       {
	       if ((rstatus = p_shMaskTest(a_regPtr->mask)) != SH_SUCCESS)
		  {goto the_exit;}
	       }

	    /* Region pixel storage is not the same size as that of the FITS
	       file. Delete all the current storage and malloc new storage.
	       The mask (if there is one) must also be deleted and re-gotten.
	       */
	    p_shRegRowsFree(a_regPtr);      /* Free the pixel storage */
	    p_shRegVectorFree(a_regPtr);    /* Free the pixel vector array */

	    /* Now get a new pixel vector array and pixel storage. */
	    (void )p_shRegVectorGet(a_regPtr, nr, desiredType);
	    (void )p_shRegRowsGet(a_regPtr, nr, nc, desiredType);

	    /* Do the same thing for the mask if there is one. */
	    if ((unsigned int )g_regInfoPtr->hasMask == 1)
	       {
	       p_shMaskRowsFree(a_regPtr->mask);    /* Free the mask storage */
	       p_shMaskVectorFree(a_regPtr->mask);  /* Free the vector array */
	       
	       /* Now get a new mask vector array and data storage */
	       p_shMaskVectorGet(a_regPtr->mask, nr);
	       p_shMaskRowsGet(a_regPtr->mask, nr, nc);
	       }
	    }

	 p_shRegCntrIncrement(a_regPtr);    /* update the reg's mod counter */
	 p_shHdrFree(&a_regPtr->hdr);       /* Free the header lines */

	 /* If there was no header in the region, we will have to malloc at
	    least the headr vector before transferring the read in header into
	    the proper place. */
	 if (a_regPtr->hdr.hdrVec == NULL)
	    {
	    /* No current header exists, malloc a header vector. */
	    p_shHdrMallocForVec(&a_regPtr->hdr);
	    }
	 
	 /* Transfer addresses of all the temp header vectors to region */
	 if ((rstatus = fitsCopyHdr(g_hdrVecTemp, a_regPtr->hdr.hdrVec,
				    TRUE)) == SH_SUCCESS)
	    {
	    /* Read in the pixels depending on their data type and transform
	       them */
	    rstatus = fitsReadPix(pixPreXformType, nr, nc, f, l_filePtr,
                                    a_regPtr, bzero, bscale, transform);
            }
         }
      }
   }

the_exit:
/* Clean up after shFileSpecExpand */
if (l_filePtr != (char *)0)
   {shFree(l_filePtr);}
if (lcl_ddCmd != (char *)0)
   {shFree(lcl_ddCmd);}
if (fname != (char *)0)
   {shEnvfree(fname);}

   if(f == a_inPipePtr) {		/* we don't own this file pointer */
      (void)f_lose(f);			/* but we registered it with libfits */
      f = NULL;
   }

if (f != (FILE *)0) {
   if (!a_readtape)
      {(void )f_fclose(f);}
   else 
      /* We have to handle the termination of our child process, dd.  If
         all went well, we've read to EOF from the pipe from dd's stdout.
         All we need to do is waitpid on dd.  If we're not at EOF, most
         likely the tape image has extra characters.  Report same as an
         error, issue a kill, and continue - waitpid will report a SIGKILL. */
      {
      fread(&tapeBuf,1,1,f);  /* attempt to read another byte */
      if (!feof(f))           /* should have EOF set if dd closed pipe */
	 {
	 shErrStackPush("ERROR : excess data available from dd.");
	 if (kill(tapePid, SIGKILL) != 0)   /* have to kill dd ourselves */
	    {
	    shErrStackPush(strerror(errno));
	    shErrStackPush("ERROR : attempt to kill dd failed.");
	    }
         }
      f_fclose(f);              /* close our end of the pipe */
      rPid = waitpid(tapePid, &ddSts, 0);
      if (rPid != tapePid)
	 {
	 if (!rPid)
	    {
	    rstatus = SH_TAPE_DD_ERR;
	    shErrStackPush("ERROR : dd did not exit, killing...");
	    if (kill(tapePid, SIGKILL) != 0)
	       {
	       shErrStackPush(strerror(errno));
	       shErrStackPush("ERROR : attempt to kill dd failed.");
	       }
	    else
	       {
	       shErrStackPush("ERROR : ... succeeded in killing dd.");
	       }
	    }
	 else
	    {
	    shErrStackPush(strerror(errno));
	    rstatus = SH_TAPE_DD_ERR;
	    shErrStackPush("ERROR : while waiting for dd to terminate.");
	    }
	 }   
      else if (WIFEXITED(ddSts))
	 {
	 if (WEXITSTATUS(ddSts))
	    {
	    rstatus = SH_TAPE_DD_ERR;
	    shErrStackPush("ERROR : dd return status %d.", WEXITSTATUS(ddSts));
	    }
         }
      else if (WIFSIGNALED(ddSts))
	 {
	 rstatus = SH_TAPE_DD_ERR;
	 shErrStackPush("ERROR : dd returned signal %d.", WTERMSIG(ddSts));
         }
      else if (WIFSTOPPED(ddSts))
	 {
	 rstatus = SH_TAPE_DD_ERR;
	 shErrStackPush("ERROR : dd stopped with signal %d, killing...",
			WSTOPSIG(ddSts));
	 if (kill(tapePid, SIGKILL) != 0)
	    {
	    shErrStackPush(strerror(errno));
	    shErrStackPush("ERROR : attempt to kill dd failed.");
	    }
	 else
	    {
	    shErrStackPush("ERROR : ... succeeded in killing dd.");
	    }
         }  
      }  /* end else wait on our child */
   }  /* end if f != NULL */

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: shHdrReadAsFits
**
** DESCRIPTION:
**      Read a FITS header into a HDR.  In all cases the existing header is
**      overwritten
**
** RETURN VALUES:
**	SH_SUCCESS
**
** CALLS TO:
**      shEnvscan
**	shEnvfree
**	shDirGet
**	isalpha
**	strcpy
**	strncat
**	shErrStackPush
**	f_rdfits
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
RET_CODE shHdrReadAsFits
   (
   HDR      *a_hdrPtr,   /* IN:  Pointer to header */
   char     *a_file,     /* IN:  Name of FITS file */
   DEFDIRENUM a_FITSdef, /* IN:  Which default file specs to apply */
   FILE     *a_inPipePtr, /* IN:  If != NULL then is a pipe to read from */
   int       a_readtape,  /* IN:  Flag which indicates we must fork/exec an
                                  instance of "dd" to read tape */
   int      hdu		  /* IN: which HDU to read (0: PDU) */
   )
{
RET_CODE    rstatus;
FILE        *f = NULL;
char        *fname = 0;
char        *l_filePtr = 0;
char        *lcl_ddCmd = NULL;
int         tapeFd[2], tapeStderr, ddSts;
pid_t       tapePid, rPid;
char        tapeBuf;
 
if (a_FITSdef == DEF_DEFAULT) {a_FITSdef = DEF_REGION_FILE;}

/* If a_readtape, we shouldn't have an open a_inPipePtr.  If so, complain
   and return.  If not, form a dd command and try a "dd" command. */
if (a_readtape)
   {
   if (a_inPipePtr != NULL)
      {
      rstatus = SH_TAPE_BAD_FILE_PTR;
      shErrStackPush("ERROR : a_inPipePtr not NULL with a_readtape.");
      goto rtn_return;
      }
   if ((lcl_ddCmd = (char*)shMalloc (strlen (a_file) + 4)) == ((char *)0))
      {
      rstatus = SH_MALLOC_ERR;
      shErrStackPush ("Unable to shMalloc space for dd command");
      goto rtn_return;
      }
   sprintf(lcl_ddCmd, "if=%s", a_file);
   /* OK, here we go.  Open a pipe, fork a child, dup the child's stdout,
      exec in the child dd, associate parent's pipe end with a stream */
   if (pipe(tapeFd) < 0)
      {
      shErrStackPush(strerror(errno));
      rstatus = SH_TAPE_DD_FAIL;
      shErrStackPush("ERROR : could not create pipe for dd process.");
      shFree(lcl_ddCmd);
      goto rtn_return;
      }
   if ( (tapePid = fork()) < 0)
      {
      shErrStackPush(strerror(errno));
      rstatus = SH_TAPE_DD_FAIL;
      shErrStackPush("ERROR : dd process could not be forked.");
      shFree(lcl_ddCmd);
      goto rtn_return;
      }
   else if (tapePid > 0)         /* tapePid > 0 - parent process */
      {
      close (tapeFd[1]);         /* close the write end of the pipe */
      f = fdopen(tapeFd[0], "r");  /* attempt to get FILE* from fd */
      if (f == NULL)
	 {
	 rstatus = SH_TAPE_DD_FAIL;
	 shErrStackPush("ERROR : could not open FILE * for dd process.");
	 shFree(lcl_ddCmd);
	 goto rtn_return;
         }
      }
   else                          /* tapePid == 0 - child process */
      {
      close (tapeFd[0]);         /* close the read end of the pipe */
      if (tapeFd[1] != STDOUT_FILENO)    /* dup the write end onto STDOUT */
	 {
	 if (dup2(tapeFd[1],STDOUT_FILENO) != STDOUT_FILENO)
	    {
	    fprintf(stderr,"ERROR : couldn't dup2 in dd child.");
	    _exit(EXIT_FAILURE);
	    }
	 close(tapeFd[1]);      /* don't need this fd anymore */
         }
#if 1
      if ((tapeStderr=open("/dev/null",O_WRONLY)) > 0)
	 {
	 dup2(tapeStderr, STDERR_FILENO);
	 close(tapeStderr);
         }
#endif
      execlp ("dd", "dd", lcl_ddCmd, "bs=144000", "files=1", (char *)0);  /* do the dd call */
      _exit(EXIT_FAILURE);       /* if exec fails return an error */
      }
  }
   /* else if we were passed a pipe name then we do not have to do any file spec
      stuff. Also the pipe is already opened. */
else if (a_inPipePtr == NULL)
   {
   /* Get the full path name to the FITS file */
   if ((rstatus = shFileSpecExpand(a_file, &l_filePtr, &fname, a_FITSdef))
       != SH_SUCCESS)
      {goto rtn_return;}
   }
else
   {
   /* Use the passed in file descriptor as it is associated with a pipe
      and is already open */
   f = a_inPipePtr;
   }

/* If there was no header, we will have to malloc
   the header vector before reading in the header. */
if (a_hdrPtr->hdrVec == NULL)
   {
   /* No current header exists, malloc a header vector. */
   p_shHdrMallocForVec(a_hdrPtr);
   }
else
   {
   /* There was a previous header vector, free any current header lines */
   p_shHdrFree(a_hdrPtr);       /* Free the header lines */
   }

if((rstatus = skip_hdus(hdu, fname, &f, a_hdrPtr->hdrVec)) != SH_SUCCESS) {
   goto rtn_return;
}

/* Read in the fits header */
rstatus = fitsReadHdr(fname, &f, (char **)a_hdrPtr->hdrVec,(hdu == 0 ? 1 : 0));


rtn_return:

/* Clean up after shFileSpecExpand */
if (l_filePtr != (char *)0)
   {shFree(l_filePtr);}
if (lcl_ddCmd != (char *)0)
   {shFree(lcl_ddCmd);}
if (fname != (char *)0)
   {shEnvfree(fname);}

   if(f == a_inPipePtr) {		/* we don't own this file pointer */
      (void)f_lose(f);			/* but we registered it with libfits */
      f = NULL;
   }

if (f != (FILE *)0)
   {
   if (!a_readtape)
      {(void )f_fclose(f);}
   else 
      /* We have to handle the termination of our child process, dd.  If
         all went well, we've read to EOF from the pipe from dd's stdout.
         All we need to do is waitpid on dd.  If we're not at EOF, most
         likely the tape image has extra characters.  Report same as an
         error, issue a kill, and continue - waitpid will report a SIGKILL. */
      {
      fread(&tapeBuf,1,1,f);  /* attempt to read another byte */
      if (!feof(f))           /* should have EOF set if dd closed pipe */
	 {
	 shErrStackPush("ERROR : excess data available from dd.");
	 if (kill(tapePid, SIGKILL) != 0)   /* have to kill dd ourselves */
	    {
	    shErrStackPush(strerror(errno));
	    shErrStackPush("ERROR : attempt to kill dd failed.");
	    }
         }
      f_fclose(f);              /* close our end of the pipe */
      rPid = waitpid(tapePid, &ddSts, 0);
      if (rPid != tapePid)
	 {
	 if (!rPid)
	    {
	    rstatus = SH_TAPE_DD_ERR;
	    shErrStackPush("ERROR : dd did not exit, killing...");
	    if (kill(tapePid, SIGKILL) != 0)
	       {
	       shErrStackPush(strerror(errno));
	       shErrStackPush("ERROR : attempt to kill dd failed.");
	       }
	    else
	       {
	       shErrStackPush("ERROR : ... succeeded in killing dd.");
	       }
	    }
	 else
	    {
	    shErrStackPush(strerror(errno));
	    rstatus = SH_TAPE_DD_ERR;
	    shErrStackPush("ERROR : while waiting for dd to terminate.");
	    }
	 }   
      else if (WIFEXITED(ddSts))
	 {
	 if (WEXITSTATUS(ddSts))
	    {
	    rstatus = SH_TAPE_DD_ERR;
	    shErrStackPush("ERROR : dd return status %d.", WEXITSTATUS(ddSts));
	    }
         }
      else if (WIFSIGNALED(ddSts))
	 {
	 rstatus = SH_TAPE_DD_ERR;
	 shErrStackPush("ERROR : dd returned signal %d.", WTERMSIG(ddSts));
         }
      else if (WIFSTOPPED(ddSts))
	 {
	 rstatus = SH_TAPE_DD_ERR;
	 shErrStackPush("ERROR : dd stopped with signal %d, killing...",
			WSTOPSIG(ddSts));
	 if (kill(tapePid, SIGKILL) != 0)
	    {
	    shErrStackPush(strerror(errno));
	    shErrStackPush("ERROR : attempt to kill dd failed.");
	    }
	 else
	    {
	    shErrStackPush("ERROR : ... succeeded in killing dd.");
	    }
         }  
      }  /* end else wait on our child */
   }  /* end if f != NULL */

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: fitsWriteHdr
**
** DESCRIPTION:
**      Write a FITS header.
**
** RETURN VALUES:
**	SH_SUCCESS
**	SH_FITS_OPEN_ERR
**
** CALLS TO:
**	shErrStackPush
**	f_wrfits
**
** GLOBALS REFERENCED:
**	g_hdrVecTemp
**
**============================================================================
*/
static RET_CODE fitsWriteHdr
   (
   char  *a_fname,      /* IN:  translated file name */
   const char *a_mode,	/* IN:  mode to open file */
   char  *a_l_filePtr,  /* IN:  full file name */
   FILE  **a_f,		/* MOD: file descriptor pointer */
   char  **a_hdrVec     /* IN:  header vector */
   )
{
RET_CODE rstatus;

if ((*a_f != NULL) || (*a_f = f_fopen(a_fname, (char *)a_mode)))
   {
   /* if appending, change SIMPLE to XTENSION='IMAGE' */
   if(strcmp(a_mode, "a") == 0) {
      a_mode = "w";			/* libfits doesn't understand "a" */
   }
   /* Now write the header */
   if (f_wrhead(a_hdrVec, *a_f))
      {
      /* Save the file information as f_put* will check for it.
       * Note well that this will require us to call f_fclose or
       * the moral equivalent --- in particular, we cannot allow
       * TCL to simply close a_f without cleaning up first by calling
       * f_lose()
       */
      if (! f_save(*a_f, a_hdrVec, (char *)a_mode))
         {
	 shErrStackPush("ERROR : Could not save FITS file information.");
         rstatus = SH_FITS_HDR_SAVE_ERR;
         }
      else
         {rstatus = SH_SUCCESS;}
      }
   else
      {
      shErrStackPush("ERROR : Could not write FITS header.");
      rstatus = SH_FITS_HDR_WRITE_ERR;
      }   
   }
else
   {
   shErrStackPush("ERROR : Could not open FITS file : %s", a_l_filePtr);
   rstatus = SH_FITS_OPEN_ERR;
   }

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: fitsXWritePix
**
** DESCRIPTION:
**      Transform the pixels according to 'bzero' and then write the FITS
**	pixels into a file.
**
** RETURN VALUES:
**	SH_SUCCESS
**	SH_FITS_PIX_WRITE_ERR
**
** CALLS TO:
**	shErrStackPush
**	regGetCType
**	f_put8
**	f_put16
**	f_put32
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static RET_CODE fitsXWritePix
   (
   PIXDATATYPE a_pixType,     /* IN: pixel type of data */
   int         a_nr,          /* IN: number of rows of data */
   int         a_nc,          /* IN: number of columns of data */
   FILE        *a_f,          /* IN: file pointee */
   char        *a_fileName,   /* IN: name of file */
   REGION      *a_regPtr,     /* IN: Pointer to reg. Data from here */
   double      a_bzero        /* IN: used in the transformation */
   )
{
RET_CODE          rstatus;
int               fstatus = 0;
int               numPix;
int               bzero;
char              *fpt;

/* Make sure we have something to write out first */
if ((a_nr * a_nc) == 0)
   {return(SH_SUCCESS);}

/* Store bzero locally as integer to be used in math (to help speed it up */
if (a_bzero == g_bz32)
  /* For g_bz32 (which represents the upper bit of a 32-bit word) we have to 
     cast it as an 'unsigned' int, since it would be set to the largest 
     'signed' int value if cast as an int (i.e., the top bit would not 
     be set). We need to ensure that the upper bit remains set. */  
  {bzero  = (unsigned int )a_bzero;}
else
  {bzero  = (int )a_bzero;}

/* According to the reference material, all the additions are first promoted
   to longs and then added.  This is the default behavior. */

/* Before doing all the arithmetic, the pointer indirection will be reduced
   by transferring pointer to pointers into just pointers whenever possible.
   WHAT WE ARE DOING: there is a local static buffer that will be filled with
   the transformed pixels, once this buffer is full, write it out, then go on
   to the next bunch of pixels. FOR THE TRANSFORMTION: get a pointer to the
   beginning and end of the vector table. Then to the beginning and end of
   each row of data and each column of data. Then do the transformation. 
   Loop around each pixel in a row (each column) and each row to transform all
   the pixels. */

switch (a_pixType)
   {
   case (TYPE_S8):
      temp8Ptr = (U8 *)g_pixTemp;			/* beginning of buf */
      temp8EndPtr = temp8Ptr + MAXPIXTEMPSIZE;		/* end of temp buf */
      pix8VecPtr = (U8 **)a_regPtr->rows_s8;
      pix8EndVecPtr = pix8VecPtr + a_nr;
      for ( ; pix8VecPtr < pix8EndVecPtr ; ++pix8VecPtr) /* do for each row */
         {
         pix8Ptr = pix8VecPtr[0];			/* beginning of row */
         pix8EndPtr = pix8Ptr + a_nc;			/* end of row */
         for ( ; pix8Ptr < pix8EndPtr ; ++pix8Ptr)	/* do for each col */
            {
            *temp8Ptr = *pix8Ptr - (S8 )bzero;	/* transformation */
            if ((++temp8Ptr) == temp8EndPtr)	/* have filled the buffer */
               {
               if (!(fstatus = f_put8((int8 *)g_pixTemp, MAXPIXTEMPSIZE, a_f)))
                  {goto err_check;}		/* error while writing */
               temp8Ptr = (U8 *)g_pixTemp;	/* point back to beginning */
               }
            }
         }

      /* Now check to see if the temp buffer was partly full.  If so, write out
         that part. */
      if ((U8 *)g_pixTemp != temp8Ptr)
         {
         numPix = temp8Ptr - (U8 *)g_pixTemp;	/* Get # of pixels in buffer */
         fstatus = f_put8((int8 *)g_pixTemp, numPix, a_f);
         }
      break;
   case (TYPE_U16):
     {
	int r,c;
	U16 *buf = (U16 *) alloca(a_nc*sizeof(U16));
	U16 *ptr;
#ifdef SDSS_LITTLE_ENDIAN
	SWAPBYTESDCLR(U, 16);
#endif

	pix16VecPtr = a_regPtr->rows_u16;
	for(r = 0;r < a_nr;r++) {
	   ptr = pix16VecPtr[r];
	   for(c = 0;c < a_nc;c++) {
#ifdef SDSS_LITTLE_ENDIAN
	      SWAP16BYTES(ptr[c] - (U16 )bzero);
	      buf[c] = swap.word;
#else
	      buf[c] = ptr[c] - (U16 )bzero;
#endif
	   }
	   if(!(fstatus = f_put16((int16 *)buf, a_nc, a_f))) break;
	}
     }
      break;
   case (TYPE_U32):
     {
#ifdef SDSS_LITTLE_ENDIAN
      SWAPBYTESDCLR(U, 32);
#endif
      temp32Ptr = (U32 *)g_pixTemp;			/* beginning of buf */
      temp32EndPtr = temp32Ptr + MAXPIXTEMPSIZE;	/* end of temp buf */
      pix32VecPtr = a_regPtr->rows_u32;
      pix32EndVecPtr = pix32VecPtr + a_nr;
      for ( ; pix32VecPtr < pix32EndVecPtr ; ++pix32VecPtr) /* for each row */
         {
         pix32Ptr = pix32VecPtr[0];			/* beginning of row */
         pix32EndPtr = pix32Ptr + a_nc;			/* end of row */
         for ( ; pix32Ptr < pix32EndPtr ; ++pix32Ptr)	/* do for each col */
            {
#ifdef SDSS_LITTLE_ENDIAN
	    SWAP32BYTES(*pix32Ptr - bzero);
	    *temp32Ptr = swap.word;
#else
            *temp32Ptr = *pix32Ptr - bzero;		/* transformation */
#endif
            if ((++temp32Ptr) == temp32EndPtr)	/* have filled the buffer */
               {
               if (!(fstatus = f_put32((int32 *)g_pixTemp, MAXPIXTEMPSIZE,
                                       a_f)))
                  {goto err_check;}		/* error while writing */
               temp32Ptr = (U32 *)g_pixTemp;	/* point back to beginning */
               }
            }
         }

      /* Now check to see if the temp buffer was partly full.  If so, write out
         that part. */
      if ((U32 *)g_pixTemp != temp32Ptr)
         {
         numPix = temp32Ptr - (U32 *)g_pixTemp;	/* Get # of pixels in buffer */
         fstatus = f_put32((int32 *)g_pixTemp, numPix, a_f);
         }
      }
      break;
   default:
      /* We should never reach here.  If we do, scream */
      shFatal("In fitsXWritePix: The file pixel type is unknown or is FL32!!");
      break;
   }

/* Check to see if there was an error */

err_check:
if (!fstatus)
   {
   (void )regGetCType(a_pixType, &fpt);
   shErrStackPush("ERROR : Unable to write %d x %d pixels of type %s, to file %s.",
                a_nr, a_nc, fpt, a_fileName);
   rstatus = SH_FITS_PIX_WRITE_ERR;
   }
else
   {rstatus = SH_SUCCESS;}

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: fitsWritePix
**
** DESCRIPTION:
**      Write the FITS pixels from a region into a file.
**
** RETURN VALUES:
**	SH_SUCCESS
**	SH_FITS_PIX_WRITE_ERR
**
** CALLS TO:
**	shErrStackPush
**	regGetCType
**	f_put8
**	f_put16
**	f_put32
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static RET_CODE fitsWritePix
   (
   PIXDATATYPE a_pixType,  /* IN: pixel type of data */
   int      a_nr,          /* IN: number of rows of data */
   int      a_nc,          /* IN: number of columns of data */
   FILE     *a_f,          /* IN: file pointee */
   char     *a_fileName,   /* IN: name of file */
   REGION   *a_regPtr      /* IN: Pointer to region. Data is read from here */
   )
{
RET_CODE rstatus;
U8   **u8Ptr = NULL;             /* Pointer to an 8 bit array */
U16  **u16Ptr = NULL;            /* Pointer to a 16 bit array */
U32  **u32Ptr = NULL;            /* Pointer to a 32 bit array */
int  fstatus = 0, i;
#ifdef SDSS_LITTLE_ENDIAN
int  j;
#endif
char *fpt;

/* Make sure we have something to write out first */
if ((a_nr * a_nc) == 0)
   {return(SH_SUCCESS);}

/* Write out nc bit words, once for each row (nr).  Since we arent interpreting
   the values but only transferring them from memory storage to disk storage,
   we only need one pointer for each size [8, 16, 32] of arrays that we have.
   It will all get cast to an int of the appropriate length anyway.  First
   get the right pointer, then do the writing. */
switch (a_pixType)
   {
   case (TYPE_U8):
      u8Ptr = a_regPtr->rows_u8;	/* Remove 1 level of indirection */
      break;
   case (TYPE_S8):
      u8Ptr = (U8 **)a_regPtr->rows_s8;	/* Remove 1 level of indirection */
      break;
   case (TYPE_U16):
      u16Ptr = a_regPtr->rows_u16;	/* Remove 1 level of indirection */
      break;
   case (TYPE_S16):
      u16Ptr = (U16 **)a_regPtr->rows_s16;  /* Remove 1 level of indirection */
      break;
   case (TYPE_U32):
      u32Ptr = a_regPtr->rows_u32;	/* Remove 1 level of indirection */
      break;
   case (TYPE_S32):
      u32Ptr = (U32 **)a_regPtr->rows_s32;  /* Remove 1 level of indirection */
      break;
   case (TYPE_FL32):
      u32Ptr = (U32 **)a_regPtr->rows_fl32; /* Remove 1 level of indirection */
      break;
   default:
      /* We should never reach here.  If we do, scream */
      shFatal("In fitsWritePix: The file pixel type is unknown!!");
      break;
   }

switch (a_pixType)
   {
   case (TYPE_U8):
   case (TYPE_S8):
      for (i = 0 ; i < a_nr ; ++i)
         {
         if (!(fstatus = f_put8((int8 *)u8Ptr[i], a_nc, a_f)))
            {break;}
         }
      break;
   case (TYPE_U16):
   case (TYPE_S16):
      {
#ifdef SDSS_LITTLE_ENDIAN
      /* Get space to swap the bytes into */
      U16   *lcl_dataPtr = (U16 *)alloca(sizeof(U16)*a_nc);
      U16   *lcl_dataStart;
      SWAPBYTESDCLR(U, 16);

      if (lcl_dataPtr == NULL)
	 {
	 rstatus = SH_MALLOC_ERR;
	 shErrStackPush("Unable to alloca space to swap bytes.\n");
	 break;
	 }

      lcl_dataStart = lcl_dataPtr;
#endif
      for (i = 0 ; i < a_nr ; ++i)
         {
#ifdef SDSS_LITTLE_ENDIAN
	 for (j = 0 ; j < a_nc ; ++j, ++lcl_dataPtr)
	   {
	   SWAP16BYTES(u16Ptr[i][j]);
	   *lcl_dataPtr = swap.word;
	   }
	 lcl_dataPtr = lcl_dataStart;

         if (!(fstatus = f_put16((int16 *)lcl_dataPtr, a_nc, a_f)))
            {break;}
#else
         if (!(fstatus = f_put16((int16 *)u16Ptr[i], a_nc, a_f)))
            {break;}
#endif
         }
      }
      break;
   case (TYPE_U32):
   case (TYPE_S32):
   case (TYPE_FL32):
      {
#ifdef SDSS_LITTLE_ENDIAN
      /* Get space to swap the bytes into */
      U32   *lcl_dataPtr = (U32 *)alloca(sizeof(U32)*a_nc);
      U32   *lcl_dataStart;
      SWAPBYTESDCLR(U, 32);

      if (lcl_dataPtr == NULL)
	 {
	 rstatus = SH_MALLOC_ERR;
	 shErrStackPush("Unable to malloc space to swap bytes.\n");
	 break;
	 }

      lcl_dataStart = lcl_dataPtr;
#endif
      for (i = 0 ; i < a_nr ; ++i)
         {
#ifdef SDSS_LITTLE_ENDIAN
	 for (j = 0 ; j < a_nc ; ++j, ++lcl_dataPtr)
	   {
	   SWAP32BYTES(u32Ptr[i][j]);
	   *lcl_dataPtr = swap.word;
	   }
	 lcl_dataPtr = lcl_dataStart;

         if (!(fstatus = f_put32((int32 *)lcl_dataPtr, a_nc, a_f)))
            {break;}
#else
         if (!(fstatus = f_put32((int32 *)u32Ptr[i], a_nc, a_f)))
            {break;}
#endif
         }
      }
      break;
   default:
      /* We should never reach here.  If we do, scream */
      shFatal("In fitsWritePix: The file pixel type is unknown!!");
      break;
   }

/* Check to see if there was an error */
if (!fstatus)
   {
   (void )regGetCType(a_pixType, &fpt);
   shErrStackPush("ERROR : Unable to write %d x %d pixels of type %s, to file %s.",
                a_nr, a_nc, fpt, a_fileName);
   rstatus = SH_FITS_PIX_WRITE_ERR;
   }
else
   {rstatus = SH_SUCCESS;}

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: regCheckHdr
**
** DESCRIPTION:
**	This file checks to see if the region has a header or not.  The value
**	of a_hasHeader is set to TRUE if there is a header.
**
** RETURN VALUES:
**
**
** CALLS TO:
**	shRegInfoGet
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static RET_CODE regCheckHdr
   (
   REGION  *a_regPtr,		/* IN:  Pointer to region */
   int     *a_hasHeader		/* OUT: = TRUE if a header exists, else FALSE */
   )
{
RET_CODE    rstatus;

/* Get the region information structure */
if ((rstatus = shRegInfoGet(a_regPtr, &g_regInfoPtr)) == SH_SUCCESS)
   {
   /* See if this region has a header or not */
   *a_hasHeader = ((unsigned int )g_regInfoPtr->hasHeader == (char )0 ? FALSE : TRUE);
   }
else
   {
   /* Could not get the regInfoPtr structure for this region */
   rstatus = SH_REGINFO_ERR;
   shErrStackPush("ERROR : Could not get the REGINFO structure for Region");
   }

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: regMatchType
**
** DESCRIPTION:
**	Check to see that the filetype and pixeltype are compatible.
**
** RETURN VALUES:
**	SH_SUCCESS
**	SH_TYPEMISMATCH
**
** CALLS TO:
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static RET_CODE regMatchType
   (
   PIXDATATYPE  a_pixType,	/* IN: Type of pixels */
   FITSFILETYPE a_fitsType	/* IN: Type of FITS file, either STANDARD or
				   non-standard */
   )
{
RET_CODE rstatus;

/* Check if region can be written out at requested.  The following
   combinations are illegal -
         pixel type = unsigned 8    , fitsType = NONSTANDARD
         pixel type = signed 16     , fitsType = NONSTANDARD
         pixel type = signed 32     , fitsType = NONSTANDARD
         pixel type = floating point, fitsType = NONSTANDARD */

switch (a_fitsType)
   {
   case NONSTANDARD:
      switch (a_pixType)
         {
         case TYPE_U8:
            rstatus = SH_TYPE_MISMATCH;
            shErrStackPush("ERROR : Incompatible pixel type (U8) with FITS file type (NONSTANDARD).");
            break;
         case TYPE_S16:
            rstatus = SH_TYPE_MISMATCH;
            shErrStackPush("ERROR : Incompatible pixel type (S16) with FITS file type (NONSTANDARD).");
            break;
         case TYPE_S32:
            rstatus = SH_TYPE_MISMATCH;
            shErrStackPush("ERROR : Incompatible pixel type (S32) with FITS file type (NONSTANDARD).");
            break;
         case TYPE_FL32:
            rstatus = SH_TYPE_MISMATCH;
            shErrStackPush("ERROR : Incompatible pixel type (FL32) with FITS file type (NONSTANDARD).");
            break;
         default:
            rstatus = SH_SUCCESS;
            break;
         }
      break;
   default:
      rstatus = SH_SUCCESS;
      break;
   }

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: shRegWriteAsFits
**
** DESCRIPTION:
**	Write the specified region as a FITS file
**
** RETURN VALUES:
**
**
** CALLS TO:
**	f_wrfits
**	f_fclose
**	f_put16
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
RET_CODE shRegWriteAsFits
   (
   REGION      *a_reg,		/* IN:  Name of region */
   char        *a_file,		/* IN:  Name of FITS file */
   FITSFILETYPE a_type,		/* IN:  Type of FITS file, either STANDARD or
				   non-standard */
   int		a_naxis,	/* IN:  Number of axes (FITS NAXIS keyword) */
   DEFDIRENUM   a_FITSdef,	/* IN:  Which default file specs to apply */
   FILE        *a_outPipePtr,   /* IN:  If != NULL then a pipe to write to */
   int          a_writetape     /* IN:  If != 0, fork/exec off a "dd" to pipe data to */
   )
{
RET_CODE    rstatus, fstatus;
FILE        *f = NULL;
PIXDATATYPE filePixType;
int         i, hasHeader, nr, nc, transform;
char        *l_filePtr = 0, *fname = 0;
double      bscale;
double      bzero;
char        *lcl_ddCmd = NULL;
int         tapeFd[2], tapeStderr, ddSts;
pid_t       tapePid, rPid;
char        *mode = "w";		/* mode to open file */

if(a_type == IMAGE) {
   mode = "a";
   if(a_outPipePtr != NULL) {
      (void)fseek(a_outPipePtr, 0, SEEK_END);
   }
/*
 * Read header and check that it says `EXTEND'. We can only do this if reading
 * a real file; trust them if not.
 */
   if(a_outPipePtr == NULL && !a_writetape) {
      HDR *hdr = shHdrNew();
      int val = FALSE;

      if((rstatus = shHdrReadAsFits(hdr, a_file, a_FITSdef, NULL, 0, 0))
							       != SH_SUCCESS) {
	 shHdrDel(hdr);
	 goto rtn_return;
      }

      if((rstatus = shHdrGetLogical(hdr, "EXTEND", &val)) != SH_SUCCESS) {
	 shHdrDel(hdr);
	 shErrStackPush("shRegWriteAsFits: "
			"File %s has no EXTEND card in its PDU", a_file);
	 goto rtn_return;
      }
	
      shHdrDel(hdr);

      if(val == FALSE) {
	 rstatus = SH_GENERIC_ERROR;
	 goto rtn_return;
      }
   }
}

nr = a_reg->nrow;
nc = a_reg->ncol;
filePixType = a_reg->type;

/* Check to see if pixel type and file type are compatible */
if (a_FITSdef == DEF_DEFAULT) {a_FITSdef = DEF_REGION_FILE;}

if ((rstatus = regMatchType(filePixType, a_type)) == SH_SUCCESS)
   {
   /* See if this region has a header or not */
   if ((rstatus = regCheckHdr(a_reg, &hasHeader)) == SH_SUCCESS)
       {
       /* If a_writetape, we shouldn't have an open a_outPipePtr.  If so, complain
	  and return.  If not, form a dd command and try a "dd" command. */
       if (a_writetape)
	  {
	  if (a_outPipePtr != NULL)
	     {
	     rstatus = SH_TAPE_BAD_FILE_PTR;
	     shErrStackPush("ERROR : a_outPipePtr not NULL with a_writetape.");
	     goto rtn_return;
	     }
	  if ((lcl_ddCmd = (char*)shMalloc (strlen (a_file) + 4)) == ((char *)0))
	     {
	     rstatus = SH_MALLOC_ERR;
	     shErrStackPush ("Unable to shMalloc space for dd command");
	     goto rtn_return;
	     }
	  sprintf(lcl_ddCmd, "of=%s", a_file);
	  /* OK, here we go.  Open a pipe, fork a child, dup the child's stdin,
	     exec in the child dd, associate parent's pipe end with a stream */
	  if (pipe(tapeFd) < 0)
	     {
	     shErrStackPush(strerror(errno));
	     rstatus = SH_TAPE_DD_FAIL;
	     shErrStackPush("ERROR : could not create pipe for dd process.");
	     shFree(lcl_ddCmd);
	     goto rtn_return;
	     }
	  if ( (tapePid = fork()) < 0)
	     {
	     shErrStackPush(strerror(errno));
	     rstatus = SH_TAPE_DD_FAIL;
	     shErrStackPush("ERROR : dd process could not be forked.");
	     shFree(lcl_ddCmd);
	     goto rtn_return;
	     }
	  else if (tapePid > 0)         /* tapePid > 0 - parent process */
	     {
	     close (tapeFd[0]);         /* close the read end of the pipe */
	     f = fdopen(tapeFd[1], "w");  /* attempt to get FILE* from fd */
	     if (f == NULL)
	        {
		rstatus = SH_TAPE_DD_FAIL;
		shErrStackPush("ERROR : could not open FILE * for dd process.");
		shFree(lcl_ddCmd);
		goto rtn_return;
	        }
	     }
	  else                          /* tapePid == 0 - child process */
	     {
	     close (tapeFd[1]);         /* close the write end of the pipe */
	     if (tapeFd[0] != STDIN_FILENO)    /* dup the read end onto STDIN */
	        {
		if (dup2(tapeFd[0],STDIN_FILENO) != STDIN_FILENO)
		   {
		   fprintf(stderr,"ERROR : couldn't dup2 in dd child.");
		   _exit(EXIT_FAILURE);
		   }
		close(tapeFd[0]);      /* don't need this fd anymore */
	        }
#if 1
	     if ((tapeStderr=open("/dev/null",O_WRONLY)) > 0)
	        {
		dup2(tapeStderr, STDERR_FILENO);
		close(tapeStderr);
	        }
#endif
	     execlp ("dd", "dd", lcl_ddCmd, "bs=144000", "files=1", (char *)0);  /* do the dd call */
	     _exit(EXIT_FAILURE);       /* if exec fails return an error */
	     }
	  }
      /* else if we were passed a pipe name then we do not have to do any file spec
	 stuff. Also the pipe is already opened. */
      else if (a_outPipePtr == NULL)
	 {
	 /* Get the full path name to the FITS file */
	   if ((rstatus = shFileSpecExpand(a_file, &l_filePtr, &fname,
					   a_FITSdef)) != SH_SUCCESS)
	      {goto rtn_return;}
	 }
      else
	 {
	 /* Use the passed in file descriptor as it is associated with a pipe
	    and is already open */
	 f = a_outPipePtr;
         }

      /* Initialize the temporary storage for the header vector */
      for (i = 0 ; i < (MAXHDRLINE+1) ; ++i)
	 {g_hdrVecTemp[i] = 0;}

      /* Copy the header vector to a private area */
      if ((rstatus = fitsCopyHdr(a_reg->hdr.hdrVec, g_hdrVecTemp, hasHeader))
	  == SH_SUCCESS)
	 {
	 /* Adjust the header by adding some keywords */
	 if ((rstatus = p_shFitsAddToHdr(a_naxis, nr, nc, filePixType,
					 a_type, &bzero, &bscale,
					 &transform)) == SH_SUCCESS)
	    {
	    /* Write the header to the file */
	    if ((rstatus = fitsWriteHdr(fname, mode, l_filePtr, &f, g_hdrVecTemp))
		== SH_SUCCESS)
	       {
	       /* Write the pixels to the file.  If the pixels need to be
		  transformed, they must be transformed to another array and 
		  not in place, then written out.  In order to minimize the 
		  possibility of memory allocation errors, a large static
		  buffer is used.  Pixels are transformed into the buffer
		  and then written out. If the pixels do not need to be
		  transformed, they are just written out. */
	       if (transform == 1)
		  {
		  /* Pixels are transformed using bzero & written out. */
		 rstatus = fitsXWritePix(filePixType, nr, nc, f, l_filePtr,
					 a_reg, bzero);
	          }
	       else
		  {
		  /* The pixels not transformed, just write them. */
		  rstatus = fitsWritePix(filePixType, nr, nc, f, l_filePtr,
					 a_reg);
		  }
	    }
	    /* Free up the space allocated for the extra lines in the
	       header.  This should be tried even if there was an error
	       on the write.  Vice versa, an error here should not stop
	       us from doing the write. */
	    fstatus = fitsEditHdr(a_type, transform, filePixType);
	    if (rstatus == SH_SUCCESS)
	      {
		 /* Only return any errors here if they will not hide the
		    more serious write errors. */
		 rstatus = fstatus;
	      }
            }
         }
      }
   }

rtn_return:

if(f == a_outPipePtr) {			/* we don't own this file pointer */
   (void)f_lose(f);			/* but we registered it with libfits */
   f = NULL;
}

/* Clean up after shFileSpecExpand */
if (l_filePtr != (char *)0)
   {shFree(l_filePtr);}
if (lcl_ddCmd != (char *)0)
   {shFree(lcl_ddCmd);}
if (fname != (char *)0)
   {shEnvfree(fname);}
if (f != (FILE *)0)
   {
   if (!a_writetape)
      {(void )f_fclose(f);}
   else
      /* We have to handle the termination of our child process, dd.  We
         close the write end of our pipe, which should send an EOF to dd.
	 All we need to do is waitpid on dd. */
      {
      f_fclose(f);                 /* close our end of the pipe */
      rPid = waitpid(tapePid, &ddSts, 0);
      if (rPid != tapePid)
	 {
	 if (!rPid)
	    {
	    rstatus = SH_TAPE_DD_ERR;
	    shErrStackPush("ERROR : dd did not exit, killing...");
	    if (kill(tapePid, SIGKILL) != 0)
	       {
	       shErrStackPush(strerror(errno));
	       shErrStackPush("ERROR : attempt to kill dd failed.");
	       }
	    else
	       {
	       shErrStackPush("ERROR : ... succeeded in killing dd.");
	       }
	    }
	 else
	    {
	    shErrStackPush(strerror(errno));
	    rstatus = SH_TAPE_DD_ERR;
	    shErrStackPush("ERROR : while waiting for dd to terminate.");
	    }
	 }   
      else if (WIFEXITED(ddSts))
	 {
	 if (WEXITSTATUS(ddSts))
	    {
	    rstatus = SH_TAPE_DD_ERR;
	    shErrStackPush("ERROR : dd return status %d.", WEXITSTATUS(ddSts));
	    }
         }
      else if (WIFSIGNALED(ddSts))
	 {
	 rstatus = SH_TAPE_DD_ERR;
	 shErrStackPush("ERROR : dd returned signal %d.", WTERMSIG(ddSts));
         }
      else if (WIFSTOPPED(ddSts))
	 {
	 rstatus = SH_TAPE_DD_ERR;
	 shErrStackPush("ERROR : dd stopped with signal %d, killing...",
			WSTOPSIG(ddSts));
	 if (kill(tapePid, SIGKILL) != 0)
	    {
	    shErrStackPush(strerror(errno));
	    shErrStackPush("ERROR : attempt to kill dd failed.");
	    }
	 else
	    {
	    shErrStackPush("ERROR : ... succeeded in killing dd.");
	    }
         }  
      }  /* end else wait on our child */
   }  /* end if f != NULL */
      
return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: fitsVerifyHdr
**
** DESCRIPTION:
**	Verify that the following keywords are in the header.
**	   SIMPLE, BITPIX, NAXIS, NAXISnn, END
**
** RETURN VALUES:
**      SH_NO_FITS_KWRD
**	SH_SUCCESS
**
** CALLS TO:
**
** GLOBALS REFERENCED:
**	g_hdrVecTemp
**
**============================================================================
*/
static RET_CODE fitsVerifyHdr
   (
   HDR *a_hdrPtr                /* IN: pointer to header vector */
   )
{
RET_CODE rstatus = SH_NO_FITS_KWRD;
char     hdrLine[HDRLINESIZE];
int      naxis;
char     naxisKeyword [16]; /* strlen ("NAXISnnn") + safety if nnn --> nn..nn */

/* Check to make sure each keyword exists in the header */
if (f_getlin(hdrLine, a_hdrPtr->hdrVec, "SIMPLE") == TRUE)
   { 
   if (f_getlin(hdrLine, a_hdrPtr->hdrVec, "BITPIX") == TRUE)
      {
      if (f_getlin(hdrLine, a_hdrPtr->hdrVec, "END") == TRUE)
	 {
	 if (f_ikey (&naxis, a_hdrPtr->hdrVec, "NAXIS") == TRUE)
	    {
	    for ( ; naxis > 0; naxis--)
	       {
	       (void )sprintf (naxisKeyword, "NAXIS%d", naxis);
	       if (f_getlin (hdrLine, a_hdrPtr->hdrVec, naxisKeyword) != TRUE)
		  {
		  /* No NAXISnn keyword */
		  shErrStackPush("ERROR : No %s keyword in header",
				 naxisKeyword);
		  goto rtn_return;
		  }
	       }
	    rstatus = SH_SUCCESS;
	    }
	 else
	    {
	    /* No NAXIS keyword */
	    shErrStackPush("ERROR : No NAXIS keyword in header");
	    }
	 }
      else
	 {
	 /* No END keyword */
	 shErrStackPush("ERROR : No END keyword in header");
	 }
      }
   else
      {
      /* No BITPIX keyword */
      shErrStackPush("ERROR : No BITPIX keyword in header");
      }
   }
else
   {
   /* No SIMPLE keyword */
   shErrStackPush("ERROR : No SIMPLE keyword in header");
   }

rtn_return: ;
return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: shHdrWriteAsFits
**
** DESCRIPTION:
**	Write the specified header as a FITS file
**
** RETURN VALUES:
**
**
** CALLS TO:
**	f_wrfits
**	f_fclose
**	f_put16
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
RET_CODE shHdrWriteAsFits
   (
   HDR         *a_hdrPtr,	/* IN:  Pointer to header */
   char        *a_file,		/* IN:  Name of FITS file */
   FITSFILETYPE a_type,		/* IN:  Type of FITS file, either STANDARD or
				   non-standard */
   int		a_naxis,	/* IN:  Number of axes (FITS NAXIS keyword) */
   DEFDIRENUM   a_FITSdef,	/* IN:  Which default file specs to apply */
   REGION      *a_regPtr,       /* IN:  pointer to region that header
				        corresponds to */
   FILE        *a_outPipePtr,   /* IN:  If != NULL then a pipe to write to */
   int          a_writetape     /* IN:  If != 0 then fork/exec "dd" and write to it */
   )
{
RET_CODE    rstatus, fstatus;
int         i;
char        *l_filePtr = 0, *fname = 0;
FILE        *f = NULL;
int         nr, nc, transform;
PIXDATATYPE filePixType;
double      bscale;
double      bzero;
char        *lcl_ddCmd = NULL;
int         tapeFd[2], tapeStderr, ddSts;
pid_t       tapePid, rPid;
char        *mode = "w";		/* mode to open file */

if(a_type == IMAGE) {
   mode = "a";
}

if (a_FITSdef == DEF_DEFAULT) {a_FITSdef = DEF_REGION_FILE;}

/* See if this header has any data or not */
if (a_hdrPtr->hdrVec != NULL)
   {
   /* If a_writetape, we shouldn't have an open a_outPipePtr.  If so, complain
      and return.  If not, form a dd command and try a "dd" command. */
   if (a_writetape)
      {
      if (a_outPipePtr != NULL)
	 {
	 rstatus = SH_TAPE_BAD_FILE_PTR;
	 shErrStackPush("ERROR : a_outPipePtr not NULL with a_writetape.");
	 goto rtn_return;
         }
      if ((lcl_ddCmd = (char*)shMalloc (strlen (a_file) + 4)) == ((char *)0))
	 {
	 rstatus = SH_MALLOC_ERR;
	 shErrStackPush ("Unable to shMalloc space for dd command");
	 goto rtn_return;
         }
      sprintf(lcl_ddCmd, "of=%s", a_file);
      /* OK, here we go.  Open a pipe, fork a child, dup the child's stdin,
	 exec in the child dd, associate parent's pipe end with a stream */
      if (pipe(tapeFd) < 0)
	 {
	 shErrStackPush(strerror(errno));
	 rstatus = SH_TAPE_DD_FAIL;
	 shErrStackPush("ERROR : could not create pipe for dd process.");
	 shFree(lcl_ddCmd);
	 goto rtn_return;
         }
      if ( (tapePid = fork()) < 0)
	 {
	 shErrStackPush(strerror(errno));
	 rstatus = SH_TAPE_DD_FAIL;
	 shErrStackPush("ERROR : dd process could not be forked.");
	 shFree(lcl_ddCmd);
	 goto rtn_return;
         }
      else if (tapePid > 0)         /* tapePid > 0 - parent process */
	 {
	 close (tapeFd[0]);         /* close the read end of the pipe */
	 f = fdopen(tapeFd[1], "w");  /* attempt to get FILE* from fd */
	 if (f == NULL)
	    {
	    rstatus = SH_TAPE_DD_FAIL;
	    shErrStackPush("ERROR : could not open FILE * for dd process.");
	    shFree(lcl_ddCmd);
	    goto rtn_return;
	    }
         }
      else                          /* tapePid == 0 - child process */
	 {
	 close (tapeFd[1]);         /* close the write end of the pipe */
	 if (tapeFd[0] != STDIN_FILENO)    /* dup the read end onto STDIN */
	    {
	    if (dup2(tapeFd[0],STDIN_FILENO) != STDIN_FILENO)
	       {
	       fprintf(stderr,"ERROR : couldn't dup2 in dd child.");
	       _exit(EXIT_FAILURE);
	       }
	    close(tapeFd[0]);      /* don't need this fd anymore */
	    }
#if 1
	 if ((tapeStderr=open("/dev/null",O_WRONLY)) > 0)
	    {
	    dup2(tapeStderr, STDERR_FILENO);
	    close(tapeStderr);
	    }
#endif
	 execlp ("dd", "dd", lcl_ddCmd, "bs=144000", "files=1", (char *)0);  /* do the dd call */
	 _exit(EXIT_FAILURE);       /* if exec fails return an error */
         }
      }
   /* else if we were passed a pipe name then we do not have to do any file spec
      stuff. Also the pipe is already opened. */
   else if (a_outPipePtr == NULL)
      {
      /* Get the full path name to the FITS file */
      if ((rstatus = shFileSpecExpand(a_file, &l_filePtr, &fname,
				      a_FITSdef)) != SH_SUCCESS)
         {goto rtn_return;}
      }
   else
      {
      /* Use the passed in file descriptor as it is associated with a pipe
	 and is already open */
      f = a_outPipePtr;
      }

   /* This header might be attached to a region.  If it is it will not have
      all of the required FITS keywords in it. These keywords need to be
      added to the header before it can be written out.  First check to see
      if a region pointer has been passed.  If so, overwrite/add to the
      current header with the information from the region.  In no region
      pointer was specified, the formal FITS header keywords (SIMPLE,
      BITPIX, and NAXIS) had better be in the header or it is an error. */
   if (a_regPtr != NULL)
      {
      nr = a_regPtr->nrow;
      nc = a_regPtr->ncol;
      filePixType = a_regPtr->type;
      
      /* We have a region, transfer the info from the region structure into
	 the header. Initialize the temporary storage for the header vector
	 first */
      for (i = 0 ; i < (MAXHDRLINE+1) ; ++i)
	 {g_hdrVecTemp[i] = 0;}
      
      /* Copy the passed header vector to the  private area */
      if ((rstatus = fitsCopyHdr(a_hdrPtr->hdrVec, g_hdrVecTemp, TRUE))
	  == SH_SUCCESS)
	 {
	 /* Adjust the header by adding some keywords */
	 if ((rstatus = p_shFitsAddToHdr(a_naxis, nr, nc, filePixType,
					 a_type, &bzero, &bscale,
					 &transform)) == SH_SUCCESS)
	    {
	    /* Write the header to the file */
	    rstatus = fitsWriteHdr(fname, mode, l_filePtr, &f, g_hdrVecTemp);
	    }

	 /* Free up the space allocated for the extra lines in the
	    header.  This should be tried even if there was an error
	    on the write.  Vice versa, an error here should not stop
	    us from doing the write. */
	 fstatus = fitsEditHdr(a_type, transform, filePixType);
	 if (rstatus == SH_SUCCESS)
	    {
	    /* Only return any errors here if they will not hide the
	       more serious write errors. */
	    rstatus = fstatus;
	    }
	 }
      }
   else
      {
      /* Make sure the passed header has the required FITS keywords in it */
      if ((rstatus = fitsVerifyHdr(a_hdrPtr)) == SH_SUCCESS)
	 {
	 /* Write the header to the file */
	 rstatus = fitsWriteHdr(fname, mode, l_filePtr, &f, a_hdrPtr->hdrVec);
	 }
      }
   }
else
   {
   shErrStackPush("ERROR : The HEADER is NULL!");
   rstatus = SH_HEADER_IS_NULL;
   }

rtn_return:

/* Clean up after shFileSpecExpand */
if (l_filePtr != (char *)0)
   {shFree(l_filePtr);}
if (lcl_ddCmd != (char *)0)
   {shFree(lcl_ddCmd);}
if (fname != (char *)0)
   {shEnvfree(fname);}

if (f != (FILE *)0)
   {
   if (!a_writetape)
      {(void )f_fclose(f);}
   else
      /* We have to handle the termination of our child process, dd.  We
         close the write end of our pipe, which should send an EOF to dd.
	 All we need to do is waitpid on dd. */
      {
      f_fclose(f);                 /* close our end of the pipe */
      rPid = waitpid(tapePid, &ddSts, 0);
      if (rPid != tapePid)
	 {
	 if (!rPid)
	    {
	    rstatus = SH_TAPE_DD_ERR;
	    shErrStackPush("ERROR : dd did not exit, killing...");
	    if (kill(tapePid, SIGKILL) != 0)
	       {
	       shErrStackPush(strerror(errno));
	       shErrStackPush("ERROR : attempt to kill dd failed.");
	       }
	    else
	       {
	       shErrStackPush("ERROR : ... succeeded in killing dd.");
	       }
	    }
	 else
	    {
	    shErrStackPush(strerror(errno));
	    rstatus = SH_TAPE_DD_ERR;
	    shErrStackPush("ERROR : while waiting for dd to terminate.");
	    }
	 }   
      else if (WIFEXITED(ddSts))
	 {
	 if (WEXITSTATUS(ddSts))
	    {
	    rstatus = SH_TAPE_DD_ERR;
	    shErrStackPush("ERROR : dd return status %d.", WEXITSTATUS(ddSts));
	    }
         }
      else if (WIFSIGNALED(ddSts))
	 {
	 rstatus = SH_TAPE_DD_ERR;
	 shErrStackPush("ERROR : dd returned signal %d.", WTERMSIG(ddSts));
         }
      else if (WIFSTOPPED(ddSts))
	 {
	 rstatus = SH_TAPE_DD_ERR;
	 shErrStackPush("ERROR : dd stopped with signal %d, killing...",
			WSTOPSIG(ddSts));
	 if (kill(tapePid, SIGKILL) != 0)
	    {
	    shErrStackPush(strerror(errno));
	    shErrStackPush("ERROR : attempt to kill dd failed.");
	    }
	 else
	    {
	    shErrStackPush("ERROR : ... succeeded in killing dd.");
	    }
         }  
      }  /* end else wait on our child */
   }  /* end if f != NULL */

return(rstatus);
}


