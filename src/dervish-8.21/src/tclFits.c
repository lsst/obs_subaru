/*++
 * FACILITY:	Dervish
 *
 * ABSTRACT:	FITS reading, writing,  and handling under Tcl.
 *
 *   Public
 *   --------------------------	------------------------------------------------
 *
 *   Private
 *   --------------------------	------------------------------------------------
 *   p_shTclDIRSETexpand	Expand a file spec based on -dirset option.
 *
 *   s_shTclFitsDirGet		Return  a FITS file directory of HDUs.
 *   s_shTclFitsRead		Read in a FITS header and data unit (HDU).
 *   s_shTclFitsReadNext	Read in the next FITS header and data unit (HDU).
 *   s_shTclFitsBinOpen		Open    a FITS file for future reading.
 *   s_shTclFitsBinClose	Close   a FITS file.
 *   s_shTclFitsWrite		Write   a FITS header and data unit (HDU).
 *
 *   p_shTclFitsDeclare		Declare Tcl commands for handling FITS files.
 *
 * ENVIRONMENT:	ANSI C.
 *		tclFits.c
 *
 * AUTHOR: 	Tom Nicinski, Creation date: 29-Nov-1993
 *              Bruce Greenawalt, added shTclFitsReadNext, shTclFitsBinOpen,
 *                                and shTclFitsBinClose: 13-May-1999
 *--
 ******************************************************************************/

/******************************************************************************/

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#include	"ftcl.h"
#include	"libfits.h"

#include	"dervish_msg_c.h"
#include	"shTclHandle.h"
#include	"shCUtils.h"
#include        "shCGarbage.h"
#include	"shCErrStack.h"    		/* For shTclInterpAppendWithErrStack () */
#include        "shTclErrStack.h"

#include	"shTclUtils.h"  		/* For shTclDeclare ()        */
#include	"shTclVerbs.h"			/* Control C++ name mangling  */
						/* ... of shTclFitsDeclare () */
#include	"shCEnvscan.h"			/* For shEnvfree ()           */
#include	"shCSchema.h"			/* For LOGICAL                */
#include	"shCFitsIo.h"			/* For DEFDIRENUM,FITSFILETYPE*/
#include	"shCHdr.h"
#include	"shCTbl.h"
#include	"shCFits.h"

#include	"shTclFits.h"
					/* Remove for ObjectCenter v2_0 - THN */
#ifdef	__cplusplus			/* Remove for ObjectCenter v2_0 - THN */
#ifndef	sgi				/* Remove for ObjectCenter v2_0 - THN */
#define	SEEK_CUR  1	/* <stdio.h> */	/* Remove for ObjectCenter v2_0 - THN */
#endif					/* Remove for ObjectCenter v2_0 - THN */
#endif					/* Remove for ObjectCenter v2_0 - THN */

/******************************************************************************/
/*
 * Globals.
 */

extern SHKEYENT		 TCLDIRTYPES[];		/* Valid -dirset values       */

/******************************************************************************/

/******************************************************************************/
/*
 * Tcl command declarations.
 */

#define	fitsCLI_Def(symbol, usage, helpText)\
   static char	*shTclFits##symbol##Name   =        "fits" #symbol,\
		*shTclFits##symbol##Use    = "USAGE: fits" #symbol " " usage,\
		*shTclFits##symbol##Help   = helpText

fitsCLI_Def (DirGet,
	     "<file> [-dirset defaultType]", "Return FITS file HDU directory (Header and Data Units)");
fitsCLI_Def (Read,
	     "<handle> <file> [-dirset defaultType] [-ascii | -binary] [-hdu n] [-xtension name] [-checktype] [-keeptype]",
	     "Read FITS file HDU (Header and Data Unit)");
fitsCLI_Def (ReadNext,
	     "<handle> <file handle>",
	     "Read FITS file HDU (Header and Data Unit)");
fitsCLI_Def (BinOpen,
	     "<file> [-hdu n] [-dirset defaultType]",
	     "Opens FITS file and position file marker before an HDU (Header and Data Unit)");
fitsCLI_Def (BinClose,
	     "<handle>",
	     "Closes FITS file");
fitsCLI_Def (Write,
	     "<handle> <file> [-dirset defaultType] [-ascii | -binary] [-pdu {NONE | MINIMAL}] [-standard {T|F}] [-extend] [-append]",
	     "Write FITS file HDU (Header and Data Unit)");

/******************************************************************************/

int		 p_shTclDIRSETexpand
   (
   char		 *dirset,		/* IN:    Entry in ftcl_ParseArgv tbl */
   Tcl_Interp	 *interp,		/* INOUT: Tcl interpreter structure   */
   DEFDIRENUM	  defType,		/* IN:    Default    file spec?       */
   DEFDIRENUM	 *expType,		/* OUT:   Default type used           */
   char		 *inSpec,		/* IN:    Unexpanded file spec        */
   char		**expSpec,		/* OUT:   Expanded file spec          */
   char		**fullSpec		/* OUT:   Expanded/substituted spec   */
   )

/*++
 * ROUTINE:	 p_shTclDIRSETexpand
 *
 *	Expand a file specification based on a default and the -dirset option.
 *	If inSpec is a pointer to 0, then only the parsing of -dirset is done;
 *	no file expansions are preformed.
 *
 * RETURN VALUES:
 *
 *   TCL_OK		Success.
 *			The file specification was expanded successfully.
 *
 *   TCL_ERROR		Failure.
 *			The interp result string contains an error message.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	The ftclFullParseArg () context is needed.  Therefore, ftclParseSave ()
 *	is   N O T   called to save the FTCL parsing context.
 *
 *   o	Space for the expanded file specifications is allocated here.  The
 *	caller   M U S T   release that space with shFree ().
 *
 *   o	The Tcl interp return string is appended to.
 *--
 ******************************************************************************/

   {	/* p_shTclDIRSETexpand */

   /*
    * Declarations.
    */

                  int	 lcl_status = TCL_OK;
   unsigned       int	 lcl_uInt;		/* Temporary                  */

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   if (dirset != NULL)
      {
      if (shKeyLocate (dirset, TCLDIRTYPES, &lcl_uInt, SH_CASE_INSENSITIVE) != SH_SUCCESS)
         {
         lcl_status = TCL_ERROR;
         Tcl_AppendResult (interp, "Invalid -dirset value", ((char *)0));
         goto rtn_return;
         }
      *expType = ((DEFDIRENUM)lcl_uInt);	/* To satisfy type checking   */
      }
   else
      {
      *expType = defType;
      }

   if (inSpec != ((char *)0))
      {
      if (shFileSpecExpand (inSpec, expSpec, fullSpec, *expType) != SH_SUCCESS)
         {
         lcl_status = TCL_ERROR;
         Tcl_AppendResult (interp, "Unable to default/translate FITS file specification \"", inSpec, "\"", ((char *)0));
         shTclInterpAppendWithErrStack (interp);
         goto rtn_return;
         }
      }
   else
      {
      if (expSpec  != ((char **)0)) { *expSpec  = ((char *)0); }
      if (fullSpec != ((char **)0)) { *fullSpec = ((char *)0); }
      }

   /*
    * All done.
    */

rtn_return : ;

   return (lcl_status);

   }	/* p_shTclDIRSETexpand */

/******************************************************************************/

static ftclArgvInfo  shTclFitsDirGetArgTable[] = {
	{"<file>",	FTCL_ARGV_STRING,	NULL,      NULL, NULL},
	{"-dirset",	FTCL_ARGV_STRING,	NULL,      NULL, NULL},
	{NULL,		FTCL_ARGV_END,		NULL,      NULL, NULL}
  };

static int	 s_shTclFitsDirGet
   (
   ClientData	 clientData,		/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	*interp,		/* INOUT: Tcl interpreter structure   */
   int		 argc,			/* IN:    Tcl argument count          */
   char		*argv[]			/* IN:    Tcl arguments               */
   )

/*++
 * ROUTINE:	 s_shTclFitsDirGet
 *
 * DESCRIPTION:
 *
 *	Return a directory of header and data units (HDUs) in the FITS file.
 * 
 * RETURN VALUES:
 *
 *   TCL_OK		Success.
 *			The FITS file was processed successfully.  The interp
 *			result string contains the directory of HDUs.
 *
 *   TCL_ERROR		Failure.
 *			The interp result string contains an error message.
 *
 * SIDE EFFECTS:
 *
 *   o	If the FITS file is on tape, the tape position will be changed, even if
 *	the HDU could not be read (or found, etc.).
 *
 *   o  The FITS file is closed using fclose instead of f_fclose because the
 *      routines f_fopen and f_fclose are not meant to be used as a pair.
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* s_shTclFitsDirGet */

   /*
    * Declarations.
    */

                  int	 lcl_status       = TCL_OK;
                  int    lcl_flags        = FTCL_ARGV_NO_LEFTOVERS;

   DEFDIRENUM		 lcl_FITSdef;		/* Default file spec?         */
                  char	*lcl_FITSname     = ((char *)0);
                  char	*lcl_FITSspec     = ((char *)0);
                  char	*lcl_FITSspecFull = ((char *)0);
                  char	*lcl_optDIRSETstr = NULL;

   FILE			*lcl_FITSfp       = ((FILE    *)0);
   HDR_VEC		*lcl_hdrVec       = ((HDR_VEC *)0);
                  int	 lcl_hdrIdx;
                  int	 lcl_hduIdx;		/* (0-indexed)                */
                  char	 lcl_hduIdxText[21 + 1];/* strlen ("-9223372036854775808")     */
   FITSHDUTYPE		 lcl_hduType;
   unsigned long  int	 lcl_dataAreaSize;	/* HDU data area size (w/ pad)*/
   unsigned long  int	 lcl_rsaSize;		/* Record storage area size   */
   unsigned long  int	 lcl_heapOff;		/* Heap offset from data start*/
   unsigned long  int	 lcl_heapSize;		/* Heap area size             */
                  int	 lcl_EXTEND = 0;	/* EXTENDsions present?       */
                  int	 lcl_EOF;
                  int	 lcl_keyIval;		/* FITS keyword integer value */
                  char	 lcl_keyedField[255+1];	/* Make larger than any need  */
						/* ... (anything a bit larger */
						/* ... than a FITS line (80   */
						/* ... bytes) is fine.        */
                  char	*lcl_prefixKey    = "";	/* Prefix Tcl key (keyed list)*/
                  char	*lcl_prefixSubkey = "";

   /***************************************************************************/
   /*
    * Parse the command line.
    */

   Tcl_SetResult (interp, "", TCL_VOLATILE);	/* Set TCL_VOLATILE           */

   shTclFitsDirGetArgTable[0].dst = &lcl_FITSname;
   shTclFitsDirGetArgTable[1].dst = &lcl_optDIRSETstr;

   if (ftcl_ParseArgv (interp, &argc, argv, shTclFitsDirGetArgTable, lcl_flags) != FTCL_ARGV_SUCCESS)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, "Parsing error\n",  ((char *)0));
      Tcl_AppendResult (interp, shTclFitsDirGetUse, ((char *)0));
      goto rtn_return;
      }

   if ((lcl_status = p_shTclDIRSETexpand (lcl_optDIRSETstr, interp, DEF_REGION_FILE, &lcl_FITSdef, lcl_FITSname,
                                          &lcl_FITSspec, &lcl_FITSspecFull)) != TCL_OK)
      {
      goto rtn_return;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Allocate space for the header vector (used by LIBFITS).
    */

   if ((lcl_hdrVec = ((HDR_VEC *)shMalloc (((MAXHDRLINE) + 1) * sizeof (HDR_VEC)))) == ((HDR_VEC *)0))
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, "Unable to allocate space for FITS header vector", ((char *)0));
      goto rtn_return;
      }
   for (lcl_hdrIdx = MAXHDRLINE; lcl_hdrIdx >= 0; lcl_hdrIdx--)
      {
      lcl_hdrVec[lcl_hdrIdx] = NULL;
      }

   /*
    * Open the FITS file and go through each HDU.
    */

   if ((lcl_FITSfp = f_fopen (lcl_FITSspecFull, "r")) == ((FILE *)0))
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, "Error opening FITS file ", lcl_FITSspecFull, ((char *)0));
      goto rtn_return;
      }

   lcl_hduIdx       = -1;
   lcl_dataAreaSize =  0;
   do {
      f_hdel (lcl_hdrVec);			/* Clean up previous header   */

      /*
       * Onto the next HDU.
       */

      lcl_hduIdx++;				/* 0-indexed                  */
      sprintf (lcl_hduIdxText, "%d", lcl_hduIdx);

      if (p_shFitsSeek (lcl_FITSfp, 0, lcl_dataAreaSize) != SH_SUCCESS)
         {
         lcl_status = TCL_ERROR;
         Tcl_SetResult    (interp, "", TCL_VOLATILE); /* Clear out keyed list */
         Tcl_AppendResult (interp, "Invalid FITS file",  lcl_FITSspec, "\n", ((char *)0));
         p_shFitsLRecMultiple (lcl_FITSfp);
         shTclInterpAppendWithErrStack (interp);
         Tcl_AppendResult (interp, "Data area for HDU ", lcl_hduIdxText, " too short", ((char *)0));
         goto rtn_return;
         }

      /*
       * Any HDU there?
       */

      if ((lcl_EOF = fgetc (lcl_FITSfp)) == EOF)
         {
         break;					/* No more HDUs               */
         }
      ungetc (lcl_EOF, lcl_FITSfp);		/* Restore the stream         */

      if (!f_rdheadi (lcl_hdrVec, (MAXHDRLINE), lcl_FITSfp))
         {
         lcl_status = TCL_ERROR;
         Tcl_SetResult    (interp, "", TCL_VOLATILE); /* Clear out keyed list */
         Tcl_AppendResult (interp, "Error processing FITS file in s_shTclFitsDirGet ", lcl_FITSspec, "\n", ((char *)0));
         Tcl_AppendResult (interp, "Error reading FITS header ", lcl_hduIdxText, ((char *)0));
         goto rtn_return;
         }

      if (!f_hinfo (lcl_hdrVec, 1, &lcl_hduType, &lcl_dataAreaSize, &lcl_rsaSize, &lcl_heapOff, &lcl_heapSize))
         {
         lcl_status = TCL_ERROR;
         Tcl_SetResult    (interp, "", TCL_VOLATILE); /* Clear out keyed list */
         Tcl_AppendResult (interp, "Invalid FITS file ", lcl_FITSspec, "\n", ((char *)0));
         Tcl_AppendResult (interp, "Invalid FITS ", shFitsTypeName (lcl_hduType), " header ", lcl_hduIdxText, ((char *)0));
         goto rtn_return;
         }

      if (lcl_hduIdx == 0)		/* Primary header is a special case   */
         {
         f_lkey (&lcl_EXTEND, lcl_hdrVec, "EXTEND");
         }

      /*
       * Generate the output result.  This is an Extended Tcl keyed list.
       */

      Tcl_AppendResult (interp, lcl_prefixKey, "{HDU", lcl_hduIdxText, " {", ((char *)0)); lcl_prefixKey = " ";
      lcl_prefixSubkey = "";		/* Reset for this keyed list element  */

      switch (lcl_hduType)
         {
         case f_hduPrimary  : Tcl_AppendResult (interp, lcl_prefixSubkey, "{TYPE {Primary}}",       0); break;
         case f_hduGroups   : Tcl_AppendResult (interp, lcl_prefixSubkey, "{TYPE {Random Groups}}", 0); break;
         case f_hduTABLE    : Tcl_AppendResult (interp, lcl_prefixSubkey, "{TYPE {TABLE}}",         0); break;
         case f_hduBINTABLE : Tcl_AppendResult (interp, lcl_prefixSubkey, "{TYPE {BINTABLE}}",      0); break;
         default            : Tcl_AppendResult (interp, lcl_prefixSubkey, "{TYPE {Unknown}}",       0); break;
         }
      lcl_prefixSubkey = " ";

      if (!f_ikey (&lcl_keyIval, lcl_hdrVec, "NAXIS")) { lcl_keyIval = 0; }
      sprintf (lcl_keyedField, "{NAXIS %d}", lcl_keyIval);
      Tcl_AppendResult (interp, lcl_prefixSubkey, lcl_keyedField, 0); lcl_prefixSubkey = " ";

      /*
       *   o   Close off keyed list for current HDU.
       */

      Tcl_AppendResult  (interp, "}}", ((char *)0));	/* End   keyed list   */

      } while (lcl_EXTEND);

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    */

rtn_return : ;

   if (lcl_FITSfp       != ((FILE    *)0)) { fclose    (lcl_FITSfp);       }
   if (lcl_FITSspec     != ((char    *)0)) {    shFree (lcl_FITSspec);     }
   if (lcl_FITSspecFull != ((char    *)0)) { shEnvfree (lcl_FITSspecFull); }
   if (lcl_hdrVec       != ((HDR_VEC *)0)) {    shFree (lcl_hdrVec);       }

   return (lcl_status);

   }	/* s_shTclFitsDirGet */

/******************************************************************************/

static ftclArgvInfo  shTclFitsReadArgTable[] = {
	{"<handle>",	FTCL_ARGV_STRING,	NULL,      NULL, NULL},
	{"<file>",	FTCL_ARGV_STRING,	NULL,      NULL, NULL},
	{"-dirset",	FTCL_ARGV_STRING,	NULL,      NULL, NULL},
	{"-ascii",	FTCL_ARGV_CONSTANT,	(void *)1, NULL, NULL},
	{"-binary",	FTCL_ARGV_CONSTANT,	(void *)1, NULL, NULL},
	{"-hdu",	FTCL_ARGV_STRING,	NULL,      NULL, NULL},
	{"-hdr",	FTCL_ARGV_CONSTANT,	(void *)1, NULL, NULL},
	{"-xtension",	FTCL_ARGV_STRING,	NULL,      NULL, NULL},
	{"-checktype",	FTCL_ARGV_CONSTANT,	(void *)1, NULL, NULL},
	{"-keeptype",	FTCL_ARGV_CONSTANT,	(void *)1, NULL, NULL},
	{NULL,		FTCL_ARGV_END,		NULL,      NULL, NULL}
      };
        
static int	 s_shTclFitsRead
   (
   ClientData	 clientData,		/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	*interp,		/* INOUT: Tcl interpreter structure   */
   int		 argc,			/* IN:    Tcl argument count          */
   char		*argv[]			/* IN:    Tcl arguments               */
   )

/*++
 * ROUTINE:	 s_shTclFitsRead
 *
 * DESCRIPTION:
 *
 *	Read a header and data unit (HDU) from the FITS file; with the
 *      -hdr flag, only read the header
 * 
 * RETURN VALUES:
 *
 *   TCL_OK		Success.
 *			The FITS file HDU was read successfully.  The interp
 *			result string contains the handle expression.
 *
 *   TCL_ERROR		Failure.
 *			The interp result string contains an error message.
 *
 * SIDE EFFECTS:
 *
 *   o	If the FITS file is on tape, the tape position will be changed, even if
 *	the HDU could not be read (or found, etc.).
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* s_shTclFitsRead */

   /*
    * Declarations.
    */

                  int	 lcl_status       = TCL_OK;
                  int	 lcl_statusTmp;
                  char	 lcl_char;
                  int	 lcl_tokCnt;
                  char	*lcl_FITSname     = ((char *)0);
   DEFDIRENUM		 lcl_FITSdef;		/* Default file spec?         */
   FITSHDUTYPE		 lcl_FITStype;
                  char	*lcl_handleExpr   = ((char *)0);
   HANDLE		 lcl_handle;
                  void	*lcl_handlePtr;
                  int	 lcl_optASCII     = 0;	/* Option present: -ascii     */
                  int	 lcl_optBINARY    = 0;	/* Option present: -binary    */
                  int	 lcl_optHDU       = 0;	/* Option present: -hdu       */
                  char	*lcl_optHDUstr    = ((char *)0);
                  int	 lcl_optHDUval    = 0;
                  int	 lcl_optHDR  = 0;	/* Option present: -hdr  */
                  char	*lcl_optHDRstr = ((char *)0);
                  int	 lcl_optXTENSION  = 0;	/* Option present: -xtension  */
                  char	*lcl_optXTENSIONstr = ((char *)0);
                  int	 lcl_optCHECKTYPE = 0;	/* Option present: -checktype */
                  int	 lcl_optKEEPTYPE  = 0;	/* Option present: -keeptype  */
                  char	*lcl_optDIRSETstr = ((char *)0);
   HDR			*lcl_hdr = NULL;

                  char	*lcl_nl = "";
   static         char	*    nl = "\n";

#define	s_shTclFitsRead_ConfOpt(opt1, opt1Name, opt2, opt2Name)\
   if (lcl_opt ## opt1 && lcl_opt ## opt2)\
      {\
      lcl_status = TCL_ERROR;\
      Tcl_AppendResult (interp, lcl_nl, "Conflicting options: -", opt1Name, " and -", opt2Name, ((char *)0)); lcl_nl = nl;\
      }

   /***************************************************************************/
   /*
    * Parse the command line.
    */

   Tcl_SetResult (interp, "", TCL_VOLATILE);	/* Set TCL_VOLATILE           */

   shTclFitsReadArgTable[0].dst = &lcl_handleExpr;
   shTclFitsReadArgTable[1].dst = &lcl_FITSname;
   shTclFitsReadArgTable[2].dst = &lcl_optDIRSETstr;
   shTclFitsReadArgTable[3].dst = &lcl_optASCII;
   shTclFitsReadArgTable[4].dst = &lcl_optBINARY;
   shTclFitsReadArgTable[5].dst = &lcl_optHDUstr;
   shTclFitsReadArgTable[6].dst = &lcl_optHDRstr;
   shTclFitsReadArgTable[7].dst = &lcl_optXTENSIONstr;
   shTclFitsReadArgTable[8].dst = &lcl_optCHECKTYPE;
   shTclFitsReadArgTable[9].dst = &lcl_optKEEPTYPE;

   if (ftcl_ParseArgv (interp, &argc, argv,shTclFitsReadArgTable, FTCL_ARGV_NO_LEFTOVERS) != FTCL_ARGV_SUCCESS)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, "Parsing error\n", ((char *)0));
      Tcl_AppendResult (interp, shTclFitsReadUse,  ((char *)0));
      goto rtn_return;
      }

   lcl_optHDU       = (lcl_optHDUstr      != NULL);
   lcl_optHDR       = (lcl_optHDRstr      != NULL);
   lcl_optXTENSION  = (lcl_optXTENSIONstr != NULL);
   
   /*
    * Check for conflicting options.
    *
    *   o   Try to find as many conflicts as possible, so keep cascading down
    *       the  tests, even if a conflict is found.
    */

   s_shTclFitsRead_ConfOpt (HDU,   "hdu",   XTENSION, "xtension");
   s_shTclFitsRead_ConfOpt (ASCII, "ascii", BINARY,   "binary");

   /*
    * Get option values, defaulting them as appropriate.
    *
    *   o   ftclFullParseArg () will default -hdu to 0 if it's not present.
    *
    *   o   ftclFullParseArg () will default -xtension to BINCOL if it's not
    *       present.  This value is ignore if -hdu is present.
    *
    * Values are checked where appropriate.
    */

   if (lcl_optHDU)
      {
      lcl_tokCnt = sscanf (lcl_optHDUstr, " %d %c", &lcl_optHDUval, &lcl_char);
      }

   if ((lcl_optHDUval < 0))
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, lcl_nl, "Bad option value: -hdu requires a positive integer", ((char *)0)); lcl_nl = nl;
      }

   if (!lcl_optHDU && lcl_optXTENSION)	/* Will -xtension be used (over -hdu)?*/
      {
      lcl_optHDUval = -1;		/* Indicate -xtension is to be used   */
      }

   if ((lcl_statusTmp = p_shTclDIRSETexpand (lcl_optDIRSETstr, interp, (DEFDIRENUM)DEF_DEFAULT,
                                             &lcl_FITSdef, ((char *)0), ((char **)0), ((char **)0))) != TCL_OK)
      {
        lcl_status    = lcl_statusTmp;
      }

   /*
    * If there were any parsing problems, time to quit.
    */

   if (lcl_status != TCL_OK)
      {
      goto rtn_return;
      }

   /*
    * Perform some argument checking.
    */

   if ((lcl_status = shTclHandleExprEval (interp, lcl_handleExpr, &lcl_handle, &lcl_handlePtr)) != TCL_OK)
      {
      lcl_status = TCL_ERROR;	/* shTclHandleExprEval sets interp result     */
      goto rtn_return;
      }

   lcl_FITStype = (lcl_optASCII)  ? f_hduTABLE
                : (lcl_optBINARY) ? f_hduBINTABLE
                                  : f_hduUnknown;

   if (lcl_handle.type != shTypeGetFromName ("REGION"))
      {
      if (lcl_optCHECKTYPE)
         {
         lcl_status = TCL_ERROR;
         Tcl_AppendResult (interp, lcl_nl, "-checktype can only be used with REGION object schemas", ((char *)0)); lcl_nl = nl;
         }
      if (lcl_optKEEPTYPE)
         {
         lcl_status = TCL_ERROR;
         Tcl_AppendResult (interp, lcl_nl, "-keeptype can only be used with REGION object schemas", ((char *)0)); lcl_nl = nl;
         }
      }

   if (lcl_status != TCL_OK)
      {
      goto rtn_return;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * If lcl_HDR is true, we want to read the header and NOT the data
    */
   if(lcl_optHDR) {
      if(lcl_handle.type != shTypeGetFromName ("TBLCOL")) {
	 Tcl_AppendResult (interp, lcl_nl, "Handle must be a TBLCOL if you specify -hdr", "\n", ((char *)0)); lcl_nl = nl;
	 shTclInterpAppendWithErrStack (interp);
	 lcl_status = TCL_ERROR;
	 goto rtn_return;
	 
      }
      lcl_hdr = &((TBLCOL *)lcl_handle.ptr)->hdr;
      lcl_handlePtr = NULL;
   } else {
      lcl_handlePtr = &lcl_handle;
   }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Read in the FITS HDU.
    */

   if ((lcl_status = shFitsRead (lcl_handlePtr, lcl_FITSname, lcl_FITSdef, lcl_FITStype, lcl_optHDUval, lcl_optXTENSIONstr, &lcl_hdr,
                                  lcl_optCHECKTYPE, lcl_optKEEPTYPE)) != SH_SUCCESS)
      {
      if (lcl_status == SH_NOT_SUPPORTED)
         {
         Tcl_AppendResult (interp, lcl_nl, "Error processing options for FITS file ", lcl_FITSname, "\n", ((char *)0)); lcl_nl = nl;
         }
      else
         {
         Tcl_AppendResult (interp, lcl_nl, "Error processing FITS file ", lcl_FITSname, "\n", ((char *)0)); lcl_nl = nl;
         }
      shTclInterpAppendWithErrStack (interp);
      lcl_status = TCL_ERROR;
      goto rtn_return;
      }

   lcl_status = TCL_OK;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    */


rtn_return : ;

   if (lcl_status == TCL_OK)
      {
      Tcl_SetResult (interp, lcl_handleExpr, TCL_VOLATILE);
      }

   return (lcl_status);

#undef	s_shTclFitsRead_ConfOpt

   }	/* s_shTclFitsRead */

/******************************************************************************/

static ftclArgvInfo  shTclFitsBinOpenArgTable[] = {
	{"<file>",	FTCL_ARGV_STRING,	NULL,      NULL, NULL},
	{"-hdu",	FTCL_ARGV_STRING,	NULL,      NULL, NULL},
	{"-dirset",	FTCL_ARGV_STRING,	NULL,      NULL, NULL},
	{NULL,		FTCL_ARGV_END,		NULL,      NULL, NULL}
      };
        
static int	 s_shTclFitsBinOpen
   (
   ClientData	 clientData,		/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	*interp,		/* INOUT: Tcl interpreter structure   */
   int		 argc,			/* IN:    Tcl argument count          */
   char		*argv[]			/* IN:    Tcl arguments               */
   )

/*++
 * ROUTINE:	 s_shTclFitsBinOpen
 *
 * DESCRIPTION:
 *
 *	Open a FITS file at the beginning of the hdu'th HDU.  Return the
 *      file position marker as a handle.
 * 
 * RETURN VALUES:
 *
 *   TCL_OK		Success.
 *			The handle was positioned successfully.  The interp
 *			result string contains the handle expression.
 *
 *   TCL_ERROR		Failure.
 *			The interp result string contains an error message.
 *
 * SIDE EFFECTS:
 *
 *   o	If the FITS file is on tape, the tape position will be changed, even if
 *	the HDU could not be read (or found, etc.).
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* s_shTclFitsBinOpen */

   /*
    * Declarations.
    */

                  int	 lcl_status       = TCL_OK;
                  int	 lcl_statusTmp;
                  char	 lcl_char;
                  int	 lcl_tokCnt;
   FILE                 *lcl_FITSfp       = ((FILE *)0);
                  char	*lcl_FITSname     = ((char *)0);
   DEFDIRENUM		 lcl_FITSdef;		/* Default file spec?         */
   FITSHDUTYPE		 lcl_FITStype;
                  char   lcl_handleName[HANDLE_NAMELEN];
     HANDLE		 lcl_handle;
                    int	 lcl_optHDU       = 0;	/* Option present: -hdu       */
                  char	*lcl_optHDUstr    = ((char *)0);
                  int	 lcl_optHDUval    = 0;
                  char	*lcl_optDIRSETstr = ((char *)0);

   /***************************************************************************/
   /*
    * Parse the command line.
    */

   Tcl_SetResult (interp, "", TCL_VOLATILE);	/* Set TCL_VOLATILE           */

   shTclFitsBinOpenArgTable[0].dst = &lcl_FITSname;
   shTclFitsBinOpenArgTable[1].dst = &lcl_optHDUstr;
   shTclFitsBinOpenArgTable[2].dst = &lcl_optDIRSETstr;

   if (ftcl_ParseArgv (interp, &argc, argv,shTclFitsBinOpenArgTable, FTCL_ARGV_NO_LEFTOVERS) != FTCL_ARGV_SUCCESS)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, "Parsing error\n", ((char *)0));
      Tcl_AppendResult (interp, shTclFitsBinOpenUse,  ((char *)0));
      goto rtn_return;
      }

   lcl_optHDU       = (lcl_optHDUstr      != NULL);
   
   /*
    * Get option values, defaulting them as appropriate.
    *
    *   o   ftclFullParseArg () will default -hdu to 0 if it's not present.
    *
    *   o   ftclFullParseArg () will default -xtension to BINCOL if it's not
    *       present.  This value is ignore if -hdu is present.
    *
    * Values are checked where appropriate.
    */

   if (lcl_optHDU) {
     lcl_tokCnt = sscanf (lcl_optHDUstr, " %d %c", &lcl_optHDUval, &lcl_char);
     lcl_optHDUval--;          /* Position file marker at end of previous HDU */
   }   

   if ( (!lcl_optHDU) || (lcl_optHDUval < 0) ) {
     lcl_optHDUval = 0;		/* Position file marker at end of primary HDU */
   }

   if ((lcl_statusTmp = p_shTclDIRSETexpand (lcl_optDIRSETstr, interp, (DEFDIRENUM)DEF_DEFAULT,
                                             &lcl_FITSdef, ((char *)0), ((char **)0), ((char **)0))) != TCL_OK)
      {
        lcl_status    = lcl_statusTmp;
      }

   /*
    * If there were any parsing problems, time to quit.
    */

   if (lcl_status != TCL_OK)
      {
      goto rtn_return;
      }


   /*
    * Perform some argument checking.
    */
   
   lcl_FITStype = f_hduBINTABLE;

   if (lcl_status != TCL_OK) {
     goto rtn_return;
   }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Open the FITS file and position at end of previous HDU.
    */

   if ((lcl_status = shFitsBinOpen (&lcl_FITSfp, lcl_FITSname, lcl_FITSdef, lcl_optHDUval)) != SH_SUCCESS)
     {
       shTclInterpAppendWithErrStack (interp);
       lcl_status = TCL_ERROR;
       goto rtn_return;
     }

   lcl_status = TCL_OK;

   lcl_handle.ptr = lcl_FITSfp;
   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Need to create the handle to store the file pointer.
    */

   if(p_shTclHandleNew(interp,lcl_handleName) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   lcl_handle.type = shTypeGetFromName("PTR");
   if(p_shTclHandleAddrBind(interp,lcl_handle,lcl_handleName) != TCL_OK) {
     shTclInterpAppendWithErrStack(interp);
     return(TCL_ERROR);

   }
   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    */

rtn_return : ;

   if (lcl_status == TCL_OK)
      {
      Tcl_SetResult (interp, lcl_handleName, TCL_VOLATILE);
      }

   return (lcl_status);

   }	/* s_shTclFitsBinOpen */

/******************************************************************************/

static ftclArgvInfo  shTclFitsReadNextArgTable[] = {
  {"<handle>",	FTCL_ARGV_STRING,	NULL,      NULL, NULL},
  {"<handle>",	FTCL_ARGV_STRING,	NULL,      NULL, NULL},
  {NULL,		FTCL_ARGV_END,		NULL,      NULL, NULL}
};
        
static int	 s_shTclFitsReadNext
(
 ClientData	 clientData,		/* IN:    Tcl parameter (not used)    */
 Tcl_Interp	*interp,		/* INOUT: Tcl interpreter structure   */
 int		 argc,			/* IN:    Tcl argument count          */
 char		*argv[]			/* IN:    Tcl arguments               */
 )

/*++
 * ROUTINE:	 s_shTclFitsReadNext
 *
 * DESCRIPTION:
 *
 *	Read a header and data unit (HDU) from the FITS file
 * 
 * RETURN VALUES:
 *
 *   TCL_OK		Success.
 *			The FITS file HDU was read successfully.  The interp
 *			result string contains the handle expression.
 *
 *   TCL_ERROR		Failure.
 *			The interp result string contains an error message.
 *
 * SIDE EFFECTS:
 *
 *   o	If the FITS file is on tape, the tape position will be changed, even if
 *	the HDU could not be read (or found, etc.).
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* s_shTclFitsReadNext */

   /*
    * Declarations.
    */

                  int	 lcl_status       = TCL_OK;
                  char	*lcl_handleExpr   = ((char *)0);
   HANDLE		 lcl_handle;
                  void	*lcl_handlePtr;
                  char	*lcl_handle2Expr   = ((char *)0);
   HANDLE		 lcl_handle2;
                  void	*lcl_handle2Ptr;
   FILE                 *lcl_FITSfp;

                  char	*lcl_nl = "";
   static         char	*    nl = "\n";


   /***************************************************************************/
   /*
    * Parse the command line.
    */

   Tcl_SetResult (interp, "", TCL_VOLATILE);	/* Set TCL_VOLATILE           */

   shTclFitsReadNextArgTable[0].dst = &lcl_handleExpr;
   shTclFitsReadNextArgTable[1].dst = &lcl_handle2Expr;

   if (ftcl_ParseArgv (interp, &argc, argv,shTclFitsReadNextArgTable, FTCL_ARGV_NO_LEFTOVERS) != FTCL_ARGV_SUCCESS)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, "Parsing error\n", ((char *)0));
      Tcl_AppendResult (interp, shTclFitsReadNextUse,  ((char *)0));
      goto rtn_return;
      }

   /*
    * Perform some argument checking.
    */

   if ((lcl_status = shTclHandleExprEval (interp, lcl_handleExpr, &lcl_handle, &lcl_handlePtr)) != TCL_OK)
      {
      lcl_status = TCL_ERROR;	/* shTclHandleExprEval sets interp result     */
      goto rtn_return;
      }

   if ((lcl_status = shTclHandleExprEval (interp, lcl_handle2Expr, &lcl_handle2, &lcl_handle2Ptr)) != TCL_OK)
      {
      lcl_status = TCL_ERROR;	/* shTclHandleExprEval sets interp result     */
      goto rtn_return;
      }

   lcl_handlePtr = &lcl_handle;
   lcl_FITSfp = (FILE*)lcl_handle2.ptr;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Read in the FITS HDU.
    */
   
   if ((lcl_status = shFitsReadNext (lcl_handlePtr, &lcl_FITSfp)) != SH_SUCCESS) {
     Tcl_AppendResult (interp, lcl_nl, "Error processing FITS file \n", ((char *)0)); lcl_nl = nl;
     shTclInterpAppendWithErrStack (interp);
     lcl_status = TCL_ERROR;
     goto rtn_return;
   }
   
   lcl_status = TCL_OK;
   
   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    */


rtn_return : ;

   if (lcl_status == TCL_OK)
      {
      Tcl_SetResult (interp, lcl_handleExpr, TCL_VOLATILE);
      }

   return (lcl_status);

   }	/* s_shTclFitsReadNext */

/******************************************************************************/

static ftclArgvInfo  shTclFitsBinCloseArgTable[] = {
	{"<handle>",	FTCL_ARGV_STRING,	NULL,      NULL, NULL},
	{NULL,		FTCL_ARGV_END,		NULL,      NULL, NULL}
      };
        
static int	 s_shTclFitsBinClose
   (
   ClientData	 clientData,		/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	*interp,		/* INOUT: Tcl interpreter structure   */
   int		 argc,			/* IN:    Tcl argument count          */
   char		*argv[]			/* IN:    Tcl arguments               */
   )

/*++
 * ROUTINE:	 s_shTclFitsBinClose
 *
 * DESCRIPTION:
 *
 *	Close a FITS file.  
 * 
 * RETURN VALUES:
 *
 *   TCL_OK		Success.
 *			The handle was positioned successfully.  The interp
 *			result string contains the handle expression.
 *
 *   TCL_ERROR		Failure.
 *			The interp result string contains an error message.
 *
 * SIDE EFFECTS:        None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* s_shTclFitsBinClose */

   /*
    * Declarations.
    */

                  int	 lcl_status       = TCL_OK;

   FILE                 *lcl_FITSfp       = ((FILE *)0);
                  char	*lcl_handleExpr   = ((char *)0);
   HANDLE		 lcl_handle;
                  void	*lcl_handlePtr;

   /***************************************************************************/
   /*
    * Parse the command line.
    */

   Tcl_SetResult (interp, "", TCL_VOLATILE);	/* Set TCL_VOLATILE           */

   shTclFitsBinCloseArgTable[0].dst = &lcl_handleExpr;

   if (ftcl_ParseArgv (interp, &argc, argv,shTclFitsBinCloseArgTable, FTCL_ARGV_NO_LEFTOVERS) != FTCL_ARGV_SUCCESS)
     {
       lcl_status = TCL_ERROR;
       Tcl_AppendResult (interp, "Parsing error\n", ((char *)0));
       Tcl_AppendResult (interp, shTclFitsBinCloseUse,  ((char *)0));
       goto rtn_return;
     }

   /*
    * If there were any parsing problems, time to quit.
    */

   if (lcl_status != TCL_OK) {
     goto rtn_return;
   }

   /*
    * Perform some argument checking.
    */

   if ((lcl_status = shTclHandleExprEval (interp, lcl_handleExpr, &lcl_handle, &lcl_handlePtr)) != TCL_OK)
     {
       lcl_status = TCL_ERROR;	/* shTclHandleExprEval sets interp result     */
       goto rtn_return;
     }

   lcl_FITSfp = lcl_handlePtr;

   if (lcl_status != TCL_OK) {
     goto rtn_return;
   }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Close the FITS file.
    */

   if ((lcl_status = shFitsBinClose (lcl_FITSfp)) != SH_SUCCESS) {
     shTclInterpAppendWithErrStack (interp);
     lcl_status = TCL_ERROR;
     goto rtn_return;
   }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Delete the Handle.
    */

   p_shTclHandleDel (interp, lcl_handleExpr);



   lcl_status = TCL_OK;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    */

rtn_return : ;

   return (lcl_status);
   
   }	/* s_shTclFitsBinClose */

/******************************************************************************/

static ftclArgvInfo shTclFitsWriteArgTable[] = {
	{"<handle>",	FTCL_ARGV_STRING,	NULL,      NULL, NULL},
	{"<file>",	FTCL_ARGV_STRING,	NULL,      NULL, NULL},
	{"-dirset",	FTCL_ARGV_STRING,	NULL,      NULL, NULL},
	{"-ascii",	FTCL_ARGV_CONSTANT,	(void *)1, NULL, NULL},
	{"-binary",	FTCL_ARGV_CONSTANT,	(void *)1, NULL, NULL},
	{"-pdu",	FTCL_ARGV_STRING,	NULL,      NULL, NULL},
	{"-standard",	FTCL_ARGV_STRING,	NULL,      NULL, NULL},
	{"-extend",	FTCL_ARGV_CONSTANT,	(void *)1, NULL, NULL},
	{"-append",	FTCL_ARGV_CONSTANT,	(void *)1, NULL, NULL},
	{NULL,		FTCL_ARGV_END,		NULL,      NULL, NULL}
   };
     
static int	 s_shTclFitsWrite
   (
   ClientData	 clientData,		/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	*interp,		/* INOUT: Tcl interpreter structure   */
   int		 argc,			/* IN:    Tcl argument count          */
   char		*argv[]			/* IN:    Tcl arguments               */
   )

/*++
 * ROUTINE:	 s_shTclFitsWrite
 *
 *	Write an object schema out to a FITS file header and data unit (HDU).
 *
 * RETURN VALUES:
 *
 *   TCL_OK		Success.
 *			The FITS file HDU was written successfully.  The interp
 *			result string contains the handle expression.
 *
 *   TCL_ERROR		Failure.
 *			The interp result string contains an error message.
 *
 * SIDE EFFECTS:
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* s_shTclFitsWrite */

   /*
    * Declarations.
    */

                  int	 lcl_status      = TCL_OK;
                  int	 lcl_statusTmp;
   RET_CODE		 lcl_statusRtn;
                  char	*lcl_FITSname    = ((char *)0);
   DEFDIRENUM		 lcl_FITSdef;		/* Default file spec?         */
   FITSHDUTYPE		 lcl_FITStype;
                  char	*lcl_handleExpr  = ((char *)0);
   HANDLE		 lcl_handle;
                  void	*lcl_handlePtr;
                  char	*lcl_handleTypeName;
                  int	 lcl_optDIRSET   = 0;	/* Option present: -dirset    */
                  int	 lcl_optASCII    = 0;	/* Option present: -ascii     */
                  int	 lcl_optBINARY   = 0;	/* Option present: -binary    */
                  int	 lcl_optPDU      = 0;	/* Option present: -pdu       */
                  char	*lcl_optPDUstr   = ((char *)0);
   SHFITSPDU		 lcl_optPDUval   = SH_FITS_PDU_NONE;
                  int	 lcl_optSTANDARD = 0;	/* Option present: -standard  */
                  char	*lcl_optSTANDARDstr = ((char *)0);
   FITSFILETYPE		 lcl_optSTANDARDval = STANDARD;
                  int	 lcl_optEXTEND   = 0;	/* Option present: -extend    */
                  int	 lcl_optAPPEND   = 0;	/* Option present: -append    */
                  char	*lcl_optDIRSETstr= ((char *)0); /* Option: -dirset    */
   HDR			*lcl_hdr         = ((HDR  *)0);/*Init to sate compiler*/
                  char	 lcl_hdrLine[2][(HDRLINECHAR)+1];
                  int	 lcl_hdrLineNo   = -1;	/* Init to sate compiler warn */
   unsigned       int	 lcl_uInt;		/* Used for type casting      */
   LOGICAL		 lcl_bool;

   TYPE			 lcl_schemaTypeREGION = shTypeGetFromName ("REGION");
   TYPE			 lcl_schemaTypeMASK   = shTypeGetFromName ("MASK");
   TYPE			 lcl_schemaTypeTBLCOL = shTypeGetFromName ("TBLCOL");
   TYPE			 lcl_schemaTypeHDR    = shTypeGetFromName ("HDR");

                  char	*lcl_nl = "";
   static         char	*    nl = "\n";

static SHKEYENT		 lcl_PDUopts[]      ={	{ "NONE",	SH_FITS_PDU_NONE		},
						{ "MINIMAL",	SH_FITS_PDU_MINIMAL	},
						{ ((char *)0),	0			}};

#define	s_shTclFitsWrite_ConfOpt(opt1, opt1Name, opt2, opt2Name)\
   if (lcl_opt ## opt1 && lcl_opt ## opt2)\
      {\
      lcl_status = TCL_ERROR;\
      Tcl_AppendResult (interp, lcl_nl, "Conflicting options: -", opt1Name, " and -", opt2Name, ((char *)0)); lcl_nl = nl;\
      }

#define	s_shTclFitsWrite_ConfOptSchema(opt, optName, schemaName, schemaExtra)\
   if (lcl_opt ## opt)\
      {\
      lcl_status = TCL_ERROR;\
      Tcl_AppendResult (interp, lcl_nl, schemaName, " object schemas", schemaExtra, "conflict with -", optName,\
                  ((char *)0)); lcl_nl = nl;\
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Parse the command line.
    */

   Tcl_SetResult (interp, "", TCL_VOLATILE);	/* Set TCL_VOLATILE           */

   shTclFitsWriteArgTable[0].dst = &lcl_handleExpr;
   shTclFitsWriteArgTable[1].dst = &lcl_FITSname;
   shTclFitsWriteArgTable[2].dst = &lcl_optDIRSETstr;
   shTclFitsWriteArgTable[3].dst = &lcl_optASCII;
   shTclFitsWriteArgTable[4].dst = &lcl_optBINARY;
   shTclFitsWriteArgTable[5].dst = &lcl_optPDUstr;
   shTclFitsWriteArgTable[6].dst = &lcl_optSTANDARDstr;
   shTclFitsWriteArgTable[7].dst = &lcl_optEXTEND;
   shTclFitsWriteArgTable[8].dst = &lcl_optAPPEND;
   
   if (ftcl_ParseArgv (interp, &argc, argv, shTclFitsWriteArgTable, FTCL_ARGV_NO_LEFTOVERS) != FTCL_ARGV_SUCCESS)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, "Parsing error\n", ((char *)0));
      Tcl_AppendResult (interp, shTclFitsWriteUse, ((char *)0));
      goto rtn_return;
      }

   lcl_optDIRSET   = (lcl_optDIRSETstr   !=NULL);
   lcl_optPDU      = (lcl_optPDUstr      !=NULL);
   lcl_optSTANDARD = (lcl_optSTANDARDstr !=NULL);
   
   /*
    * Check for conflicting options.
    *
    *   o   Try to find as many conflicts as possible, so keep cascading down
    *       the  tests, even if a conflict is found.
    */

   s_shTclFitsWrite_ConfOpt (APPEND, "append", PDU,    "pdu");
   s_shTclFitsWrite_ConfOpt (APPEND, "append", EXTEND, "extend");
   s_shTclFitsWrite_ConfOpt (ASCII,  "ascii",  BINARY, "binary");

   /*
    * Get option values, defaulting them as appropriate.
    *
    *   o   ftclFullParseArg () will default -pdu      to * if it's not present.
    *   o   ftclFullParseArg () will default -standard to * if it's not present.
    *
    * Values are checked where appropriate.
    */

   if (lcl_optPDU)
      {
      if (shKeyLocate (lcl_optPDUstr, lcl_PDUopts, &lcl_uInt, SH_CASE_INSENSITIVE) != SH_SUCCESS)
         {
         lcl_status = TCL_ERROR;
         Tcl_AppendResult (interp, lcl_nl, "Invalid -pdu value", ((char *)0)); lcl_nl = nl;
         }
      else
         {
         lcl_optPDUval = ((SHFITSPDU)lcl_uInt);
         }
      }

   if (lcl_optSTANDARD)
      {
      if (shBooleanGet (lcl_optSTANDARDstr, &lcl_bool) != SH_SUCCESS)
         {
         lcl_status = TCL_ERROR;
         Tcl_AppendResult (interp, lcl_nl, "Invalid -standard value", ((char *)0)); lcl_nl = nl;
         }
      else
         {
         lcl_optSTANDARDval = (lcl_bool) ? STANDARD : NONSTANDARD;
         }
      }


   if ((lcl_statusTmp = p_shTclDIRSETexpand (lcl_optDIRSETstr, interp, (DEFDIRENUM)DEF_DEFAULT, 
                           &lcl_FITSdef, ((char *)0), ((char **)0), ((char **)0))) != TCL_OK)
      {
        lcl_status    = lcl_statusTmp;
      }

   /*
    * If there were any parsing problems, time to quit.
    */

   if (lcl_status != TCL_OK)
      {
      goto rtn_return;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Perform some argument checking.
    */

   if ((lcl_status = shTclHandleExprEval (interp, lcl_handleExpr, &lcl_handle, &lcl_handlePtr)) != TCL_OK)
      {
      lcl_status = TCL_ERROR;	/* shTclHandleExprEval sets interp result     */
      goto rtn_return;
      }

   lcl_handleTypeName = shNameGetFromType (lcl_handle.type);	/* For msgs   */

   lcl_FITStype = (lcl_optASCII)                            ? f_hduTABLE
                : (lcl_optBINARY)                           ? f_hduBINTABLE
                : (lcl_handle.type == lcl_schemaTypeREGION) ? f_hduPrimary
                : (lcl_handle.type == lcl_schemaTypeMASK)   ? f_hduPrimary
                                                            : f_hduUnknown;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * The handling of particular object schemas is done by other Tcl commands.
    */

   if (lcl_handle.type == lcl_schemaTypeMASK)
      {
      s_shTclFitsWrite_ConfOptSchema (STANDARD, "standard", lcl_handleTypeName, "");
      }

   if (   (lcl_handle.type == lcl_schemaTypeREGION)
       || (lcl_handle.type == lcl_schemaTypeMASK)
      )
      {
      switch (lcl_FITStype)
         {
         case f_hduUnknown :
         case f_hduPrimary :
                             break;
         default : lcl_status = TCL_ERROR;
                   Tcl_AppendResult (interp, lcl_nl, lcl_handleTypeName, " object schemas can be written only to the Primary HDU",
                               ((char *)0)); lcl_nl = nl;
                   break;
         }
      s_shTclFitsWrite_ConfOptSchema (APPEND,   "append",   lcl_handleTypeName, " (Primary HDUs) ");
      if (lcl_optPDU && (lcl_optPDUval != SH_FITS_PDU_NONE))
         {
         s_shTclFitsWrite_ConfOptSchema (PDU,   "pdu",      lcl_handleTypeName, " (Primary HDUs) ");
         }

      if (lcl_status != TCL_OK)
         {
         goto rtn_return;
         }
      }

   if (lcl_handle.type == lcl_schemaTypeHDR)
      {
      s_shTclFitsWrite_ConfOptSchema (ASCII,    "ascii",    lcl_handleTypeName, "");
      s_shTclFitsWrite_ConfOptSchema (BINARY,   "binary",   lcl_handleTypeName, "");
      s_shTclFitsWrite_ConfOptSchema (APPEND,   "append",   lcl_handleTypeName, "");
      if (lcl_optPDU && (lcl_optPDUval != SH_FITS_PDU_NONE))
         {
         s_shTclFitsWrite_ConfOptSchema (PDU,   "pdu",      lcl_handleTypeName, "");
         }
      }

   /*
    * If -extend is specified, make sure that the PDU header has EXTEND=T.
    *
    *   o   The addition of -extend is only temporary. The header is restored
    *       to its original condition.
    */

   if (lcl_optEXTEND)
      {
      if (   (lcl_handle.type != lcl_schemaTypeREGION)
          && (lcl_handle.type != lcl_schemaTypeMASK)
          && (lcl_handle.type != lcl_schemaTypeHDR)
         )
         {
         lcl_status = TCL_ERROR;
         Tcl_AppendResult (interp, lcl_nl, "-extend may be used only with Primary HDUs", ((char *)0)); lcl_nl = nl;
         goto rtn_return;
         }

      lcl_hdr = (lcl_handle.type == lcl_schemaTypeREGION) ? &(((REGION *)lcl_handle.ptr)->hdr)
              : (lcl_handle.type == lcl_schemaTypeHDR)    ?   ((HDR    *)lcl_handle.ptr)
                                                          :   ((HDR    *)0);
      if (lcl_hdr == ((HDR *)0))
         {
         lcl_status = TCL_ERROR;
         Tcl_AppendResult (interp, lcl_nl, "Cannot force EXTEND=T in PDU (-extend)", ((char *)0)); lcl_nl = nl;
         Tcl_AppendResult (interp, lcl_nl, "Don't know how to get header from ", lcl_handleTypeName, " object schema", ((char *)0));
         goto rtn_return;
         }
      f_mklkey (lcl_hdrLine[0], "EXTEND", 1, "");
      if ((lcl_hdr->hdrVec != ((HDR_VEC *)0)) && ((lcl_hdrLineNo = f_lnum (lcl_hdr->hdrVec, "EXTEND")) >= 0))
         {
         strncpy (lcl_hdrLine[1], lcl_hdr->hdrVec[lcl_hdrLineNo], sizeof (lcl_hdrLine[1]) - 1);
                  lcl_hdrLine[1][sizeof (lcl_hdrLine[1]) - 1] = ((char)0);
         f_hlrep (lcl_hdr->hdrVec, lcl_hdrLineNo, lcl_hdrLine[0]);
         }
      else
         {
         if ((lcl_hdr->hdrVec == ((HDR_VEC *)0)) || !f_hlins (lcl_hdr->hdrVec, MAXHDRLINE, 0, lcl_hdrLine[0]))
            {
            lcl_status = TCL_ERROR;
            Tcl_AppendResult (interp, lcl_nl, lcl_handleTypeName, " not written to ", lcl_FITSname, ((char *)0)); lcl_nl = nl;
            Tcl_AppendResult (interp, lcl_nl, "Problem encountered forcing EXTEND=T in ", lcl_handleTypeName, " header",
                        ((char *)0)); lcl_nl = nl;
            }
         }
      }

   /***************************************************************************/
   /*
    * Write out the FITS HDU.
    */

   if ((lcl_handle.type == lcl_schemaTypeTBLCOL) && (!lcl_optASCII && !lcl_optBINARY))
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, "TBLCOL object schemas must have -ascii or -binary specified", ((char *)0)); lcl_nl = nl;
      goto rtn_return;
      }

   if (lcl_status != TCL_OK)
      {
      goto rtn_return;
      }

   if ((lcl_statusRtn = shFitsWrite (&lcl_handle, lcl_FITSname, lcl_FITSdef, lcl_FITStype, lcl_optPDUval, lcl_optAPPEND,
                                      lcl_optSTANDARDval)) != SH_SUCCESS)
      {
      lcl_status = TCL_ERROR;
      if (lcl_statusRtn == SH_NOT_SUPPORTED)
         {
         Tcl_AppendResult (interp, lcl_nl, "Error processing options for FITS file ", lcl_FITSname, "\n", ((char *)0)); lcl_nl = nl;
         }
      else
         {
         Tcl_AppendResult (interp, lcl_nl, "Error processing FITS file ", lcl_FITSname, "\n", ((char *)0)); lcl_nl = nl;
         }
      shTclInterpAppendWithErrStack (interp);
      }

   /*
    * Undo EXTEND=T if we put it in the header.
    */

   if (lcl_optEXTEND)
      {
      if (lcl_hdrLineNo >= 0)
         {
         f_hlrep (lcl_hdr->hdrVec, lcl_hdrLineNo, lcl_hdrLine[1]);
         }
      else
         {
         if ((lcl_hdr->hdrVec != ((HDR_VEC *)0)) && ((lcl_hdrLineNo = f_lnum (lcl_hdr->hdrVec, "EXTEND")) >= 0))
            {
            f_hldel (lcl_hdr->hdrVec, lcl_hdrLineNo);
            }
         }
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    */

rtn_return : ;

   if (lcl_status == TCL_OK)
      {
      Tcl_SetResult (interp, lcl_handleExpr, TCL_VOLATILE);
      }
   return (lcl_status);

#undef	s_shTclFitsWrite_ConfOpt
#undef	s_shTclFitsWrite_ConfOptSchema

   }	/* s_shTclFitsWrite */

/******************************************************************************/

void		 p_shTclFitsDeclare
   (
   Tcl_Interp	*interp			/* IN:    Tcl Interpreter structure   */
   )

/*++
 * ROUTINE:	 p_shTclFitsDeclare
 *
 * DESCRIPTION:
 *
 *	Declare Tcl commands for this module.
 *
 * RETURN VALUES:	None
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	This routine should only be called once per task.
 *--
 ******************************************************************************/

   {	/* p_shTclFitsDeclare */

   /*
    * Declarations.
    */

#define	shTclFitsCLI(symbol, rtn)\
   shTclDeclare (interp, shTclFits##symbol##Name, ((Tcl_CmdProc *)(rtn)), ((ClientData)0), ((Tcl_CmdDeleteProc *)NULL), "shFitsIo",\
                         shTclFits##symbol##Help, shTclFits##symbol##Use)

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   shTclFitsCLI (DirGet,   s_shTclFitsDirGet);
   shTclFitsCLI (Read,     s_shTclFitsRead);
   shTclFitsCLI (ReadNext, s_shTclFitsReadNext);
   shTclFitsCLI (BinOpen,  s_shTclFitsBinOpen);
   shTclFitsCLI (BinClose, s_shTclFitsBinClose);
   shTclFitsCLI (Write,    s_shTclFitsWrite);

#undef	shTclFitsCLI

   }	/* p_shTclFitsDeclare */

/******************************************************************************/
