/* FACILITY:	Dervish
 *
 * ABSTRACT:	FITS ASCII and Binary Table reader and handling under Tcl.
 *
 *   Public
 *   --------------------------	------------------------------------------------
 *
 *   Private
 *   --------------------------	------------------------------------------------
 *   s_shTclTblInfoGet		Return information about a Table.
 *   s_shTclTblFldAdd		Add a field to a TBLCOL Table (to end of fields)
 *   s_shTclTblFldLoc		Locate a field using -field or -col option.
 *   s_shTclTblFldInfoGet	Return information about the field.
 *   s_shTclTblFldInfoClr	Clear TBLFLD member.
 *   s_shTclTblFldInfoSet	Set   TBLFLD member value.
 *   s_shTclTblFldGet		Get a field from a Table.
 *   s_shTclTblFldSet		Set field values in a Table.
 *   s_shTclTblCompliesWithFits	Table complies with FITS Standard?
 *
 *   p_shTclTblDeclare		Declare Tcl commands for handling Tables.
 *
 * ENVIRONMENT:	ANSI C.
 *		tclTbl.c
 *
 * AUTHOR: 	Tom Nicinski, Creation date: 29-Nov-1993
 *--
 ******************************************************************************/

/******************************************************************************/

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#include	"ftcl.h"

#include	"dervish_msg_c.h"
#include	"shTclHandle.h"
#include	"shCSchema.h"
#include	"shCAlign.h"
#include	"shTclAlign.h"
#include	"shCArray.h"
#include	"shTclArray.h"
#include	"shCUtils.h"
#include        "shCGarbage.h"
#include        "shChain.h"
#include	"shCFits.h"		/* For shFitsTBLCOLcomply ()          */

#include        "shCErrStack.h"
#include	"shTclUtils.h"  	/* For shTclDeclare ()                */
#include	"shTclVerbs.h"		/* Control C++ name mangling of ...   */
					/* ... shTclTblDeclare ()             */
#include	"shCTbl.h"
#include        "shTclErrStack.h"

/******************************************************************************/

/******************************************************************************/
/*
 * Tcl command declarations.
 */

#define	tblCLI_Def(symbol, usage, helpText)\
   static char	*shTclTbl##symbol##Name   =        "tbl" #symbol,\
		*shTclTbl##symbol##Use    = "USAGE: tbl" #symbol " " usage,\
		*shTclTbl##symbol##Help   = helpText

tblCLI_Def (InfoGet,	"<handle>",
			"Get information about a TBLCOL Table");
tblCLI_Def (FldAdd,	"<handle> <-type datType | -heaptype heapDataType> [-heapcnt lenList] [-dim dimList]",
			"Add a field to a TBLCOL (at the end of existing fields)");
tblCLI_Def (FldInfoGet,	"<handle> <-field name [-fieldpos pos] [-case | -nocase] | -col n>",
			"Get information about a field in a TBLCOL Table");
tblCLI_Def (FldInfoClr,	"<handle> <-field name [-fieldpos pos] [-case | -nocase] | -col n>  <member>",
			"Clear information about a field in a TBLCOL Table");
tblCLI_Def (FldInfoSet,	"<handle> <-field name [-fieldpos pos] [-case | -nocase] | -col n>  <member>  <value>",
			"Set information about a field in a TBLCOL Table");
tblCLI_Def (FldGet,	"<handle> <-field name [-fieldpos pos] [-case | -nocase] | -col n> [indices]",
			"Get a field value(s) from a TBLCOL Table");
tblCLI_Def (FldSet,	"<handle> <-field name [-fieldpos pos] [-case | -nocase] | -col n> [-trunc | -notrunc] <indices> <values>",
			"Set field value(s) in a TBLCOL Table");
tblCLI_Def (CompliesWithFits,	"<handle> <-ascii | -binary>",
				"Indicate whether Table is FITS compliant");

/******************************************************************************/
/*
 * Globals.
 *
 *   s_shTblKeys	Used to convert a character string to a TBLFLD_KEYWORDS.
 */

static SHKEYENT s_shTblKeys[]={	{ "TTYPE",	SH_TBL_TTYPE    },
				{ "TUNIT",	SH_TBL_TUNIT    },
				{ "TNULLSTR",	SH_TBL_TNULLSTR },
				{ "TDISP",	SH_TBL_TDISP    },
				{ "TSCAL",	SH_TBL_TSCAL    },
				{ "TZERO",	SH_TBL_TZERO    },
				{ "TNULLINT",	SH_TBL_TNULLINT },
				{ ((char *)0),	0             }};

/******************************************************************************/

static ftclArgvInfo shTclTblInfoGetArgTable[] = {
   {"<handle>",  FTCL_ARGV_STRING, NULL, NULL,"Get information about a TBLCOL Table"},
   {NULL,        FTCL_ARGV_END,    NULL, NULL, NULL}
   };

static int	 s_shTclTblInfoGet
   (
   ClientData	 clientData,		/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	*interp,		/* INOUT: Tcl interpreter structure   */
   int		 argc,			/* IN:    Tcl argument count          */
   char		*argv[]			/* IN:    Tcl arguments               */
   )

/*++
 * ROUTINE:	 s_shTclTblInfoGet
 *
 *	Return information about the TBLCOL Table. This information is returned
 *	in a keyed list.  The following keys are always returned:
 *
 *	    ROWCNT	Number of rows in the Table.
 *
 *	    FLDCNT	Number of fields in the Table.
 *
 * RETURN VALUES:
 *
 *   TCL_OK		Success.
 *			The interp result string contains a Tcl keyed list of
 *			information about the Table.
 *
 *   TCL_ERROR		Failure.
 *			The interp result string contains an error message.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* s_shTclTblInfoGet */

   /*
    * Declarations.
    */

                  int	  lcl_status     = TCL_OK;
		  int     lcl_flags      = FTCL_ARGV_NO_LEFTOVERS;
                  char	 *lcl_handleExpr = ((char  *)0);
   HANDLE		  lcl_handle;
                  void	 *lcl_handlePtr;
   TBLCOL		 *lcl_tblCol;
                  int	  lcl_fldCnt;		/* Number of fields           */
                  int	  lcl_fldIdx;		/* Field index (0-indexed)    */
                  char	  lcl_result[255+1];	/* Make this larger than any  */
						/* number and some characters */
						/* for generating one element */
						/* in the output keyed list.  */

   shTclTblInfoGetArgTable[0].dst = &lcl_handleExpr;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Parse the command line.
    */

   Tcl_SetResult (interp, "", TCL_VOLATILE);	/* Set TCL_VOLATILE           */
 
   if (ftcl_ParseArgv(interp, &argc, argv,shTclTblInfoGetArgTable, lcl_flags)!=
           FTCL_ARGV_SUCCESS)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, "Parsing error\n",  ((char *)0));
      Tcl_AppendResult (interp, shTclTblInfoGetUse, ((char *)0));
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

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * The handling of particular object schemas is done by other Tcl commands.
    */

   if (lcl_handle.type != shTypeGetFromName ("TBLCOL"))
      {
      Tcl_AppendResult (interp, shNameGetFromType (lcl_handle.type), " object schemas are not handled by ", shTclTblInfoGetName,
                                ((char *)0));
      goto rtn_return;
      }
  
     
   lcl_tblCol = ((TBLCOL *)lcl_handle.ptr);

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    *   o   Only TBLCOL object schemas are handled here.
    *
    *   o   Floating point values are output with the %g format specifier.  The
    *       precision depends on the type:
    *
    *           float    8 decimal digits
    *           double  17 decimal digits
    *
    *       The %g format specifier cuts off trailing zeros, even the decimal
    *       point if appropriate;  but, this code will output, at a minimum, a
    *       trailing decimal point and zero (lcl_ForceRealTrail).
    */
       
    lcl_fldIdx = shChainSize(&(lcl_tblCol->fld));
       
    lcl_fldCnt = lcl_fldIdx;

    sprintf (lcl_result, "%d", lcl_tblCol->rowCnt);
    Tcl_AppendResult (interp,  "{ROWCNT ", lcl_result, "}", ((char *)0));
       
    sprintf (lcl_result, "%d", lcl_fldCnt);
    Tcl_AppendResult (interp, " {FLDCNT ", lcl_result, "}", ((char *)0));
    

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    */
    
rtn_return : ;
   return (lcl_status);

   }	/* s_shTclTblInfoGet */

/******************************************************************************/

static ftclArgvInfo shTclTblFldAddArgTable[] = {
      {"<handle>", FTCL_ARGV_STRING, NULL, NULL, NULL},
      {"-type",    FTCL_ARGV_STRING, NULL, NULL, NULL},
      {"-heaptype",FTCL_ARGV_STRING, NULL, NULL, NULL},
      {"-heapcnt", FTCL_ARGV_STRING, NULL, NULL, NULL},
      {"-dim",     FTCL_ARGV_STRING, NULL, NULL, NULL},
      {NULL,       FTCL_ARGV_END,    NULL, NULL, NULL}
     };


static int	 s_shTclTblFldAdd
   (
   ClientData	 clientData,		/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	*interp,		/* INOUT: Tcl interpreter structure   */
   int		 argc,			/* IN:    Tcl argument count          */
   char		*argv[]			/* IN:    Tcl arguments               */
   )

/*++
 * ROUTINE:	 s_shTclTblFldAdd
 *
 *	Add a field to a TBLCOL Table to the end of the field list.
 *
 * RETURN VALUES:
 *
 *   TCL_OK		Success.
 *			The interp result string contains the field position
 *			(0-indexed).
 *
 *   TCL_ERROR		Failure.
 *			The interp result string contains an error message.
 *
 * SIDE EFFECTS:
 *
 *   o	If the TBLCOL Table does not have any fields, the first dimension of the
 *	new field is made the row count for the complete Table.  In that case,
 *	the -dim option must be passed.
 *
 *	   o	If the Table already had a row count (without any fields), that
 *		row count is overridden.
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* s_shTclTblFldAdd */

   /*
    * Declarations.
    */

                  int	  lcl_status         = TCL_OK;
                  int     lcl_flags          = FTCL_ARGV_NO_LEFTOVERS;
                  char	 *lcl_handleExpr     = ((char *)0);
   HANDLE		  lcl_handle;
                  void	 *lcl_handlePtr;
                  int	  lcl_optTYPE;		/* Option present: -type      */
                  char	 *lcl_optTYPEval     = ((char *)0);
		  char   *lcl_optTYPEvalMalloc = ((char *)0);
                  int	  lcl_optHEAPTYPE;	/* Option present: -heaptype  */
                  char	 *lcl_optHEAPTYPEval = ((char *)0);
                  int	  lcl_optHEAPCNT;	/* Option present: -heapcnt   */
                  int	 *lcl_heapCnt = ((int *)0);  /* -heapcnt lengths      */
                  int	  lcl_heapCntCnt;	/* # elems in lcl_heapCnt[]   */
                  int	  lcl_optDIM;		/* Option present: -dim       */
                  int	  lcl_dimIdx;
                  int	  lcl_dimCnt;		/* Number of dimensions passed*/
                  int	  lcl_dim[(shBinTDIMlim)+1+1];	/* +1 for TBLHEAPDSC  */

#if ((shArrayDimLim)     <       ((shBinTDIMlim)+1))
#error	ARRAYs do not support all possible FITS Binary Table array dimensions
#endif


   TBLCOL		 *lcl_tblCol;
                  int	  lcl_tblColEmpty;	/* No fields in TBLCOL?       */
   ARRAY		 *lcl_array;
                  int	  lcl_fldIdx     = -1;	/* Field index (0-indexed)    */
                  int	  lcl_rowCntOrig = -1;	/* Original row count in Table*/
                  int	  lcl_rowIdx;
   TBLHEAPDSC		 *lcl_tblHeapDsc;	/* In ARRAY data area         */
                  int	  lcl_dataUNKNOWN = 0;	/* Explicit UNKNOWN data type */

                  char	  lcl_result[255+1];	/* Make this larger than any  */
						/* number and some characters */
						/* for generating one element */
						/* in the output keyed list.  */

                  char	 *lcl_nl = "";
   static         char	 *    nl = "\n";

                  char   *lcl_typeValPtr     = NULL;   /* for use with parseArgv */
		  char   *lcl_heaptypeValPtr = NULL;
		  char   *lcl_heapcntValPtr  = NULL;
		  char   *lcl_dimValPtr      = NULL;

   shTclTblFldAddArgTable[0].dst = &lcl_handleExpr;
   shTclTblFldAddArgTable[1].dst = &lcl_typeValPtr;
   shTclTblFldAddArgTable[2].dst = &lcl_heaptypeValPtr;
   shTclTblFldAddArgTable[3].dst = &lcl_heapcntValPtr;
   shTclTblFldAddArgTable[4].dst = &lcl_dimValPtr;
   
   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Parse the command line.
    */

   Tcl_SetResult (interp, "", TCL_VOLATILE);	/* Set TCL_VOLATILE           */

   if (ftcl_ParseArgv (interp, &argc, argv, shTclTblFldAddArgTable, lcl_flags)!=FTCL_ARGV_SUCCESS)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, "Parsing error\n", ((char *)0));
      Tcl_AppendResult (interp, shTclTblFldAddUse, ((char *)0));
      goto rtn_return;
      }

   lcl_optTYPE     = (lcl_typeValPtr     != NULL);
   lcl_optHEAPTYPE = (lcl_heaptypeValPtr != NULL);
   lcl_optHEAPCNT  = (lcl_heapcntValPtr  != NULL);
   lcl_optDIM      = (lcl_dimValPtr      != NULL);

   /*
    *   o   Get the list of dimensions, if any.
    */

   lcl_dimCnt = 0;				/* Default                    */

   if (lcl_optDIM)
      {
      if (p_shTclListValRet (interp, shTypeGetFromName ("INT"), lcl_dimValPtr, "*", 1, (shBinTDIMlim) + 1, SH_ARRAY_TRAIL_ZERO,
          &lcl_dimCnt, ((void *)lcl_dim), SH_ARRAY_VAL_NON_NEG, ((void *)0), 0, 0, "dimension", "dimensions", "", 0, 0, SH_ARRAY_STR_FULL,
          ((int *)0)) != TCL_OK)
         {
         lcl_status = TCL_ERROR;
         lcl_nl     = nl;
         }
      }

   /*
    * Check for conflicting options.
    *
    *   o   Try to find as many conflicts as possible, so keep cascading down
    *       the  tests, even if a conflict is found.
    */

   if (!lcl_optTYPE && !lcl_optHEAPTYPE)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, lcl_nl, "Missing option: -type or -heaptype required to specify data type", ((char *)0));
                                lcl_nl = nl;
      }

   if (lcl_optHEAPCNT && !lcl_optHEAPTYPE)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, lcl_nl, "Inappropriate option: use -heapcnt only with -heaptype", ((char *)0)); lcl_nl = nl;
      }

   /*
    * Perform some argument checking.
    *
    *   o   Only TBLCOLs are handled.
    */

   if ((shTclHandleExprEval (interp, lcl_handleExpr, &lcl_handle, &lcl_handlePtr)) != TCL_OK)
      {
      lcl_status = TCL_ERROR;	/* shTclHandleExprEval sets interp result     */
      goto rtn_return;
      }

   if (lcl_handle.type != shTypeGetFromName ("TBLCOL"))
      {
      Tcl_AppendResult (interp, lcl_nl, shNameGetFromType (lcl_handle.type), " object schemas are not handled by ",
                                        shTclTblFldAddName, ((char *)0));
      goto rtn_return;
      }

   lcl_tblCol     = ((TBLCOL *)lcl_handle.ptr);

   lcl_rowCntOrig = lcl_tblCol->rowCnt;	/* Save in case of failure    */

   /*
    *   o   Determine whether any fields already exist in the Table.  If not,
    *       then the -dim option must be passed.
    */

   lcl_tblColEmpty = (shChainSize(&(lcl_tblCol->fld)) == 0);

   if (lcl_tblColEmpty)
      {
      if (!lcl_optDIM)
         {
         lcl_status = TCL_ERROR;
         Tcl_AppendResult (interp, lcl_nl, "Missing option: -dim required when TBLCOL originally has no fields", ((char *)0));
                                   lcl_nl = nl;
         }

      }
   /*
    *   o   If no fields exist in the Table now, use the (now mandatory) dimen-
    *       sion information to adjust the Table row count.
    */

   if (lcl_tblColEmpty)
      {
      lcl_tblCol->rowCnt = lcl_dim[0];		/* Use -dim value             */
      }

   if (lcl_dimCnt == 0)				/* Default if -dim not passed */
      {
       lcl_dim[lcl_dimCnt++] = lcl_tblCol->rowCnt;
      }

   if (lcl_dim[0] != lcl_tblCol->rowCnt)	/* Guaranteed min of 1 dimen  */
      {
      lcl_status = TCL_ERROR;
      sprintf (lcl_result, "%d", lcl_tblCol->rowCnt);
      Tcl_AppendResult (interp, lcl_nl, "1st dimension does not match TBLCOL row count (", lcl_result, ")", ((char *)0));
                                lcl_nl = nl;
      }

   /*
    *   o   Get the data type.
    *
    *          o   -type and/or -heaptype are guaranteed to have been specified.
    */

   if (lcl_optTYPE)
      {
      lcl_optTYPEval = lcl_typeValPtr;

      if (strcmp (lcl_optTYPEval, "TBLHEAPDSC") == 0)
         {
         if (!lcl_optHEAPTYPE)
            {
            lcl_status = TCL_ERROR;
            Tcl_AppendResult (interp, lcl_nl, "Missing option: -heaptype required when -type is TBLHEAPDSC (heap data)",
                       ((char *)0));  lcl_nl = nl;
            goto rtn_return;
            }
         }
      }
   else
      {
      if (lcl_optHEAPTYPE)
         {
         lcl_optTYPEval = (char*) shMalloc(strlen ("TBLHEAPDSC") + 1);
	 lcl_optTYPEvalMalloc = lcl_optTYPEval;
         strcpy  (lcl_optTYPEval, "TBLHEAPDSC");
         }
      }
   if (lcl_optHEAPTYPE)
      {
        lcl_optHEAPTYPEval = lcl_heaptypeValPtr;

      if (!lcl_optHEAPCNT && (lcl_tblCol->rowCnt > 0))
         {
         lcl_status = TCL_ERROR;
         Tcl_AppendResult (interp, lcl_nl, "Missing option: -heapcnt required with -heaptype",     ((char *)0));  lcl_nl = nl;
         }
      if (strcmp (lcl_optTYPEval, "TBLHEAPDSC") != 0)
         {
         lcl_status = TCL_ERROR;
         Tcl_AppendResult (interp, lcl_nl, "Heap data requires ARRAY data -type to be TBLHEAPDSC", ((char *)0));  lcl_nl = nl;
         }
      }

   /*
    *   o      o   Check special data types.
    */

   if (shStrcmp (lcl_optTYPEval, "STR", SH_CASE_INSENSITIVE) == 0)
      {
      if (lcl_dimCnt < 2)
         {
         lcl_status = TCL_ERROR;
         Tcl_AppendResult (interp, lcl_nl, "Too few dimensions specified (minimum of 2 required for STR object schema)",
                     ((char *)0)); lcl_nl = nl;
         }
      }

   /*
    *   o   If heap is being used, check -heapcnt.
    */

   if (lcl_status != TCL_OK)	/* Avoid problems if Table has no fields and  */
      {				/* -dim was not specified (or invalid minimum */
      goto rtn_return;		/* and/or maximums will be passed to p_shTcl- */
      }				/* ListValRet ().                             */

   if (lcl_optHEAPCNT)
      {
      if (lcl_tblCol->rowCnt != 0)	/* 0 rows in Table is legitimate      */
         {
         lcl_heapCnt = (int *) shMalloc (lcl_tblCol->rowCnt * sizeof (int));
         }

      if (p_shTclListValRet (interp, shTypeGetFromName ("INT"), lcl_heapcntValPtr, "*", lcl_tblCol->rowCnt,
          lcl_tblCol->rowCnt, SH_ARRAY_TRAIL_ZERO, &lcl_heapCntCnt, ((void *)lcl_heapCnt), SH_ARRAY_VAL_NON_NEG, ((void *)0), 0, 0,
          "heap count", "heap counts", "", 0, 0, SH_ARRAY_STR_FULL, ((int *)0)) != TCL_OK)
         {
         lcl_status = TCL_ERROR;
         lcl_nl     = nl;
         }
      }

   if (lcl_status != TCL_OK)
      {
      goto rtn_return;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Add the field.
    */

   if ((lcl_status = shTblFldAdd (lcl_tblCol, 0, &lcl_array, ((TBLFLD **)0), &lcl_fldIdx)) != SH_SUCCESS)
      {
      lcl_status = TCL_ERROR;
      shTclInterpAppendWithErrStack (interp);
      goto rtn_return;
      }

   /*
    *   o   Allocate space for the data area.
    *
    *          o   The object schema UNKNOWN is not permitted.  If this changes,
    *              then, for the ARRAY data set
    *
    *             lcl_array->data.size  = shAlignment.type[SHALIGNTYPE_U8].size;
    *             lcl_array->data.incr  = shAlignment.type[SHALIGNTYPE_U8].incr;
    *             lcl_array->data.align = shAlignment.alignMax;
    *
    *              For heap data, shTblHeapAlloc treats UNKNOWN as an unsigned
    *              char. In that case, nothing additional would need to be done
    *              to permit its use here.
    */

   if (lcl_optHEAPTYPEval != ((char *)0)) { lcl_dataUNKNOWN = (shStrcmp (lcl_optHEAPTYPEval, "UNKNOWN", SH_CASE_INSENSITIVE) == 0); }

   if ((shStrcmp (lcl_optTYPEval, "UNKNOWN", SH_CASE_INSENSITIVE) == 0) || lcl_dataUNKNOWN) /* lcl_optTYPEval contains string */
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, lcl_nl, "Object schema UNKNOWN is not supported", ((char *)0)); lcl_nl = nl;
      goto rtn_return;
      }
   lcl_array->nStar      = 0;			/* Data indirection unsupport */
   lcl_array->dimCnt     = lcl_dimCnt;
   for (lcl_dimIdx = 0; lcl_dimIdx < lcl_dimCnt; lcl_dimIdx++)
      {
      lcl_array->dim[lcl_dimIdx] = lcl_dim[lcl_dimIdx];	/* Copy to ARRAY      */
      }

   if (shArrayDataAlloc (lcl_array, lcl_optTYPEval) != SH_SUCCESS)
      {
      lcl_status = TCL_ERROR;
      shTclInterpAppendWithErrStack (interp);
      goto rtn_return;
      }

   /*
    *   o   If heap is being used, fill in the heap descriptors (TBLHEAPDSC) in
    *       the ARRAY data area.  Allocated heap space.
    *
    *          o   nStar must be zero (ARRAY contains TBLHEAPDSCs directly).
    *
    *          o   Although shArrayDataAlloc zeros the data area, this is NOT
    *              depended upon as a zero address may be different than a zero
    *              value.
    */

   if (lcl_optHEAPTYPE)
      {
      for (lcl_tblHeapDsc = ((TBLHEAPDSC *)lcl_array->data.dataPtr), lcl_rowIdx = 0; lcl_rowIdx < lcl_heapCntCnt;     lcl_rowIdx++)
         {
         lcl_tblHeapDsc[lcl_rowIdx].cnt = lcl_heapCnt[lcl_rowIdx];
         lcl_tblHeapDsc[lcl_rowIdx].ptr = ((unsigned char *)0);
         }
      for (                                                                        ; lcl_rowIdx < lcl_tblCol->rowCnt; lcl_rowIdx++)
         {
         lcl_tblHeapDsc[lcl_rowIdx].cnt = 0;			/* No heap    */
         lcl_tblHeapDsc[lcl_rowIdx].ptr = ((unsigned char *)0);
         }

      if (shTblFldHeapAlloc (lcl_array, lcl_optHEAPTYPEval, 0, ((unsigned char **)0)) != SH_SUCCESS)
         {
         lcl_status = TCL_ERROR;
         shTclInterpAppendWithErrStack (interp);
         goto rtn_return;
         }
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    */

   lcl_status = TCL_OK;

rtn_return : ;


   if (lcl_status == TCL_OK)
      {
      sprintf (lcl_result, "%d", lcl_fldIdx);
      Tcl_SetResult (interp, lcl_result, TCL_VOLATILE);
      }
   else
      {
      if (lcl_fldIdx >= 0)
         {
         p_shTblFldDelByIdx (&(lcl_tblCol->fld), lcl_fldIdx); /*No use for field*/
         }
      if (lcl_rowCntOrig != -1) { lcl_tblCol->rowCnt = lcl_rowCntOrig; }
      }

   if (lcl_optTYPEvalMalloc != ((char *)0)) { shFree (lcl_optTYPEval);     }
   if (lcl_heapCnt          != ((int  *)0)) { shFree (lcl_heapCnt);        }

   return (lcl_status);

   }	/* s_shTclTblFldAdd */

/******************************************************************************/

/**  it is a kludge! because ftcl_ParseArgv doesn't save the context,
 *   and s_shTclTblFldLoc() wants to share the parsing info with
 *   its caller, there is no way of knowing which switch/param is passed in
 *   from s_shTclTblFldLoc() unless you parse again. But if you parse again,
 *   the dst field in the parsing table has to be reset. I come up with an awkard 
 *   scheme to deal with this difficult situation using argTable and offset.
 *   *argTable is assumed to have following format:

      {"-field",     FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"-fieldpos",  FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"-case",      FTCL_ARGV_CONSTANT,  (void*) TRUE, NULL, NULL},
      {"-nocase",    FTCL_ARGV_CONSTANT,  (void*) TRUE, NULL, NULL},
      {"-col",       FTCL_ARGV_STRING,  NULL, NULL, NULL},
      
 *   Note the relative positions of the entries have to be invariant. 
 *   argTblEntryOffset is number of positional parameters before these switches. 
 *
 *   Wei Peng. 11/09/94
 */
      
static int	 s_shTclTblFldLoc
   (
   ftclArgvInfo*  argTable,
   int            argTblEntryOffset,
   ClientData	  clientData,		/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	 *interp,		/* INOUT: Tcl interpreter structure   */
   int		  argc,			/* IN:    Tcl argument count          */
   char		 *argv[],		/* IN:    Tcl arguments               */
   TBLCOL	 *tblCol,		/* IN:    Table descriptor            */
   ARRAY	**array,		/* OUT:   ARRAY  field descriptor     */
   TBLFLD	**tblFld,		/* OUT:   TBLFLD field descriptor     */
   int		 *tblFldIdx		/* OUT:   Field index (0-indexed)     */
   )

/*++
 * ROUTINE:	 s_shTclTblFldLoc
 *
 *	Locate a field in the Table based on either the -col or -field options
 *	passed from the command line.
 *
 * RETURN VALUES:
 *
 *   TCL_OK		Success.
 *			The field was located.  The interp result string is
 *			empty.
 *
 *   TCL_ERROR		Failure.
 *			The interp result string contains an error message.
 *
 * SIDE EFFECTS:
 *
 *   o	The interp result string is only appended to.  It is not set initially.
 *	No initial newline is appended to the result string.
 *
 * SYSTEM CONCERNS:
 *
 *   o	The fTcl parsing context is NOT saved.  This context should be saved at
 *	an upper level.
 *--
 ******************************************************************************/

   {	/* s_shTclTblFldLoc */

   /*
    * Declarations.
    */

                  int	  lcl_status      = TCL_OK;
                  int     lcl_flags       = FTCL_ARGV_NO_LEFTOVERS;
                  char	  lcl_char;
                  int	  lcl_tokCnt;
                  int	  lcl_optCOL;		/* Option present: -col       */
                  int	  lcl_optCOLval;
                  int	  lcl_optFIELD;		/* Option present: -field     */
                  char	 *lcl_optFIELDval    = ((char *)0);
                  int	  lcl_optFIELDPOS;	/* Option present: -fieldpos  */
                  int	  lcl_optFIELDPOSval = 0;
                  int	  lcl_optCASE;		/* Option present: -case      */
                  int	  lcl_optNOCASE;	/* Option present: -nocase    */
   SHCASESENSITIVITY	  lcl_optCASEval;	/* For both -case and -nocase */

                  char	 *lcl_nl = "",     *lcl_comma = "";
   static         char	 *    nl = "\n",   *    comma = ", ";

                  char   *lcl_dummy          = NULL;
                  char   *lcl_fieldValPtr    = NULL;
		  char   *lcl_fieldPosValPtr = NULL;
		  int     lcl_caseVal        = FALSE;
                  int     lcl_nocaseVal      = FALSE;
                  char   *lcl_colValPtr      = NULL;
                  
   argTable[0+argTblEntryOffset].dst = &lcl_fieldValPtr;
   argTable[1+argTblEntryOffset].dst = &lcl_fieldPosValPtr;
   argTable[2+argTblEntryOffset].dst = &lcl_caseVal;
   argTable[3+argTblEntryOffset].dst = &lcl_nocaseVal;
   argTable[4+argTblEntryOffset].dst = &lcl_colValPtr;
   
  /* Parse argument again */

   if(ftcl_ParseArgv(interp, &argc, argv,argTable,lcl_flags)
        != FTCL_ARGV_SUCCESS )
   {
          lcl_status = TCL_ERROR;  /* this is reached unless multiple ftclParseArgv call is forbidden */
          goto rtn_return;
   }
           
   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   lcl_optCOL      = (lcl_colValPtr      != NULL);
   lcl_optFIELD    = (lcl_fieldValPtr    != NULL);
   lcl_optFIELDPOS = (lcl_fieldPosValPtr != NULL);
   lcl_optCASE     = (lcl_caseVal        == TRUE);
   lcl_optNOCASE   = (lcl_nocaseVal      == TRUE);

   /*
    * Check for conflicting options.
    *
    *   o   Try to find as many conflicts as possible, so keep cascading down
    *       the  tests, even if a conflict is found.
    */

   if ( lcl_optCOL  && (lcl_optFIELD || lcl_optFIELDPOS || lcl_optCASE || lcl_optNOCASE))
      {
      lcl_status = TCL_ERROR;                       lcl_comma = "";
      Tcl_AppendResult (interp, "Conflicting options: -col and ",   (lcl_optFIELD)    ? "-field"    : "", ((char *)0));
                                                    lcl_comma =     (lcl_optFIELD)    ?   comma     : lcl_comma;
      Tcl_AppendResult (interp, (lcl_optFIELDPOS) ? lcl_comma : "", (lcl_optFIELDPOS) ? "-fieldpos" : "", ((char *)0));
                                                    lcl_comma =     (lcl_optFIELDPOS) ?   comma     : lcl_comma;
      Tcl_AppendResult (interp, (lcl_optCASE)     ? lcl_comma : "", (lcl_optCASE)     ? "-case"     : "", ((char *)0));
                                                    lcl_comma =     (lcl_optCASE)     ?   comma     : lcl_comma;
      Tcl_AppendResult (interp, (lcl_optNOCASE)   ? lcl_comma : "", (lcl_optNOCASE)   ? "-nocase"   : "", ((char *)0));
                                                    lcl_nl    = nl;
      }

   if ( lcl_optCASE &&  lcl_optNOCASE)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, lcl_nl, "Conflicting options: -case and -nocase", ((char *)0)); lcl_nl = nl;
      }

   if (!lcl_optFIELD && (lcl_optCASE || lcl_optNOCASE || lcl_optFIELDPOS))
      {
      lcl_status = TCL_ERROR;                       lcl_comma = "";
      Tcl_AppendResult (interp, lcl_nl, "Missing option: -field required with ",
                                                                    (lcl_optCASE)     ? "-case"     : "", ((char *)0));
                                                    lcl_comma =     (lcl_optCASE)     ?   comma     : lcl_comma;
      Tcl_AppendResult (interp, (lcl_optNOCASE)   ? lcl_comma : "", (lcl_optNOCASE)   ? "-nocase"   : "", ((char *)0));
                                                    lcl_comma =     (lcl_optNOCASE)   ?   comma     : lcl_comma;
      Tcl_AppendResult (interp, (lcl_optFIELDPOS) ? lcl_comma : "", (lcl_optFIELDPOS) ? "-fieldpos" : "", ((char *)0));
                                                    lcl_nl    = nl;
      }

   if (!lcl_optCOL && !lcl_optFIELD)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, "Missing option: either -col or -field must be specified", ((char *)0));
      goto rtn_return;
      }

   /*
    * Get option values, defaulting them as appropriate.
    *
    * Values are checked where appropriate.
    */

   lcl_optCASEval = (lcl_optNOCASE) ? SH_CASE_INSENSITIVE : SH_CASE_SENSITIVE;

   if (!lcl_optCOL)
      {
        lcl_optCOLval = -1;
      }
   else
      {
      lcl_tokCnt = sscanf (lcl_colValPtr, " %d %c", &lcl_optCOLval, &lcl_char);
      if ((lcl_tokCnt != 1) || (lcl_optCOLval < 0))
         {
         lcl_status = TCL_ERROR;
         Tcl_AppendResult (interp, lcl_nl, "Bad option value: -col requires a positive integer", ((char *)0)); lcl_nl = nl;
         }
      }

   if (!lcl_optFIELD)
      {
        lcl_optFIELDval = ((char *)0);
      }
   else
      {
      lcl_optFIELDval = lcl_fieldValPtr;

      if (strcmp (lcl_optFIELDval, "") == 0)
         {
         lcl_status = TCL_ERROR;
         Tcl_AppendResult (interp, lcl_nl, "Bad option value: -field requires a character string", ((char *)0)); lcl_nl = nl;
         }
      if (!lcl_optFIELDPOS)
         {
           lcl_optFIELDPOSval = 0;		/* Default to first match     */
         }
      else
         {
         lcl_tokCnt = sscanf (lcl_fieldPosValPtr, " %d %c", &lcl_optFIELDPOSval, &lcl_char);
         if ((lcl_tokCnt != 1) || (lcl_optFIELDPOSval < 0))
            {
            lcl_status = TCL_ERROR;
            Tcl_AppendResult (interp, lcl_nl, "Bad option value: -fieldpos requires a positive integer", ((char *)0)); lcl_nl = nl;
            }
         }
      }

   if (lcl_status != TCL_OK)
      {
      goto rtn_return;
      }

   /*
    * Locate the field.
    */

   if (shTblFldLoc (tblCol, lcl_optCOLval, lcl_optFIELDval, lcl_optFIELDPOSval, lcl_optCASEval, array, tblFld, tblFldIdx)
       != SH_SUCCESS)
      {
      lcl_status = TCL_ERROR;
      shTclInterpAppendWithErrStack (interp);
      goto rtn_return;
      }

   /*
    * All done.
    */

   lcl_status = TCL_OK;

rtn_return : ;


   return (lcl_status);

   }	/* s_shTclTblFldLoc */

/******************************************************************************/

static ftclArgvInfo shTclTblFldInfoGetArgTable[] = {
      {"<handle>",   FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"-field",     FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"-fieldpos",  FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"-case",      FTCL_ARGV_CONSTANT,  (void*) TRUE, NULL, NULL},
      {"-nocase",    FTCL_ARGV_CONSTANT,  (void*) TRUE, NULL, NULL},
      {"-col",       FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {NULL,         FTCL_ARGV_END,     NULL, NULL, NULL},
     };

static int	 s_shTclTblFldInfoGet
   (
   ClientData	 clientData,		/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	*interp,		/* INOUT: Tcl interpreter structure   */
   int		 argc,			/* IN:    Tcl argument count          */
   char		*argv[]			/* IN:    Tcl arguments               */
   )

/*++
 * ROUTINE:	 s_shTclTblFldInfoGet
 *
 *	Return information about the Table field.  This information is returned
 *	in a keyed list.  Many of the keys correspond to FITS header keywords
 *	(without any indexing suffix), but may act differently. The following
 *	keys are always returned:
 *
 *	    COL		Field within the Table (0-indexed).
 *
 *	    TYPE	Object schema type.
 *
 *	    DIM		Dimension sizes are returned in row-major order (as
 *			opposed to FITS' column-major order).  The number of
 *			Table rows (equivalent to NAXIS2) is the first, that
 *			is, slowest varying dimension.
 *
 *	    NSTAR	The amount of indirection of ARRAY data.
 *
 *	    EMPTY	Indicates whether the ARRAY data area is empty.
 *
 * RETURN VALUES:
 *
 *   TCL_OK		Success.
 *			The interp result string contains a Tcl keyed list of
 *			information about the Table field.
 *
 *   TCL_ERROR		Failure.
 *			The interp result string contains an error message.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* s_shTclTblFldInfoGet */

   /*
    * Declarations.
    */

                  int	  lcl_status     = TCL_OK;
		  int     lcl_flags      = FTCL_ARGV_NO_LEFTOVERS;
                  char	 *lcl_str;
                  char	 *lcl_handleExpr = ((char  *)0);
   HANDLE		  lcl_handle;
                  void	 *lcl_handlePtr;
   TBLCOL		 *lcl_tblCol;
   ARRAY		 *lcl_array;
   TBLFLD		 *lcl_tblFld;		/* TBLFLD structure           */
                  int	  lcl_fldIdx;		/* Field index (0-indexed)    */
                  int	  lcl_dimIdx;
                  char	 *lcl_prefixElem;	/* Prefix array element       */
                  char	  lcl_result[255+1];	/* Make this larger than any  */
                  char	 *lcl_resultPos;	/* number and some characters */
						/* for generating one element */
						/* in the output keyed list.  */

                  char   *lcl_fieldValPtr    = NULL;
		  char   *lcl_fieldPosValPtr = NULL;
		  int     lcl_caseVal        = FALSE;
                  int     lcl_nocaseVal      = FALSE;
                  char   *lcl_colValPtr      = NULL;

                  int     lcl_argc = argc;
                  char*   lcl_argv[10];

#define	lcl_ForceRealTrail\
	if ((lcl_resultPos = strchr (lcl_result, '.')) != ((char *)0))\
	   {\
	   if (*(++lcl_resultPos) == '\000') { *lcl_resultPos = '0'; *(++lcl_resultPos) = '\000'; }\
	   }\
	else\
	   {\
		if ( strchr(lcl_result,'e')  == ((char*)0) ) \
	   		strcat (lcl_result, ".0");\
	   }

  /* yet another Kludge! ftcl_ParseArgv alters argc/argv, making it impossible
   * to parse argc/argv twice without saving them first.
   */

   lcl_fldIdx = 0;
   while(lcl_fldIdx < argc) { lcl_argv[lcl_fldIdx] = argv[lcl_fldIdx]; lcl_fldIdx++;}

   shTclTblFldInfoGetArgTable[0].dst = &lcl_handleExpr;
   shTclTblFldInfoGetArgTable[1].dst = &lcl_fieldValPtr;
   shTclTblFldInfoGetArgTable[2].dst = &lcl_fieldPosValPtr;
   shTclTblFldInfoGetArgTable[3].dst = &lcl_caseVal;
   shTclTblFldInfoGetArgTable[4].dst = &lcl_nocaseVal;
   shTclTblFldInfoGetArgTable[5].dst = &lcl_colValPtr;
   
   

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Parse the command line.
    */

   Tcl_SetResult (interp, "", TCL_VOLATILE);	/* Set TCL_VOLATILE           */


   if (ftcl_ParseArgv (interp, &argc, argv,shTclTblFldInfoGetArgTable,lcl_flags )
          != FTCL_ARGV_SUCCESS)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, "Parsing error\n",     ((char *)0));
      Tcl_AppendResult (interp, shTclTblFldInfoGetUse, ((char *)0));
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

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * The handling of particular object schemas is done by other Tcl commands.
    */

   if (lcl_handle.type != shTypeGetFromName ("TBLCOL"))
      {
      Tcl_AppendResult (interp, shNameGetFromType (lcl_handle.type), " object schemas are not handled by ", shTclTblFldInfoGetName,
                                ((char *)0));
      goto rtn_return;
      }

   lcl_tblCol = ((TBLCOL *)lcl_handle.ptr);

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Locate the field and return information about it.
    *
    *   o   Only TBLCOL object schemas are handled here.
    *
    *   o   Floating point values are output with the %g format specifier.  The
    *       precision depends on the type:
    *
    *           float    8 decimal digits
    *           double  17 decimal digits
    *
    *       The %g format specifier cuts off trailing zeros, even the decimal
    *       point if appropriate;  but, this code will output, at a minimum, a
    *       trailing decimal point and zero (lcl_ForceRealTrail).
    */

   if ((lcl_status = s_shTclTblFldLoc (shTclTblFldInfoGetArgTable, 1, clientData, interp, lcl_argc, lcl_argv, lcl_tblCol,
                                      &lcl_array, &lcl_tblFld, &lcl_fldIdx)) != TCL_OK)
      {
      goto rtn_return;
      }

   sprintf (lcl_result, "%d", lcl_fldIdx);
   Tcl_AppendResult (interp,  "{COL ", lcl_result, "}", ((char *)0));

   Tcl_AppendResult (interp, " {TYPE ", shNameGetFromType (lcl_array->data.schemaType), "}", ((char *)0));

   Tcl_AppendResult (interp, " {DIM {", ((char *)0));
   lcl_prefixElem = "";
   for (lcl_dimIdx = 0; lcl_dimIdx < lcl_array->dimCnt; lcl_dimIdx++)
      {
      sprintf (lcl_result, "%s%d", lcl_prefixElem, lcl_array->dim[lcl_dimIdx]); lcl_prefixElem = " ";
      Tcl_AppendResult (interp, lcl_result, ((char *)0));
      }
   Tcl_AppendResult (interp, "}}", ((char *)0));

   sprintf (lcl_result, "%d", lcl_array->nStar);
   Tcl_AppendResult (interp, " {NSTAR ", lcl_result, "}", ((char *)0));

   Tcl_AppendResult (interp, " {EMPTY ", (lcl_array->data.dataPtr == ((unsigned char *)0)) ? "1" : "0", "}", ((char *)0));

   /* . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */
   /*
    *   o   Handle TBLFLD-specific information.
    */

   if (lcl_array->data.schemaType == shTypeGetFromName ("TBLHEAPDSC"))
      {
      lcl_str = "UNKNOWN";			/* Default value              */
      if (lcl_tblFld != ((TBLFLD *)0))
         {
         if (shInSet (lcl_tblFld->Tpres, SH_TBL_HEAP))
            {
            lcl_str = shNameGetFromType (lcl_tblFld->heap.schemaType);
            }
         }
      Tcl_AppendResult (interp, " {HEAPTYPE ", lcl_str, "}", ((char *)0));
      }


   if (lcl_tblFld != ((TBLFLD *)0))
      {
      if (shInSet (lcl_tblFld->Tpres, SH_TBL_TTYPE))
         {
         Tcl_AppendResult (interp, " {TTYPE {", lcl_tblFld->TTYPE, "}}", ((char *)0));
         }

      if (shInSet (lcl_tblFld->Tpres, SH_TBL_TUNIT))
         {
         Tcl_AppendResult (interp, " {TUNIT {", lcl_tblFld->TUNIT, "}}", ((char *)0));
         }

      if (shInSet (lcl_tblFld->Tpres, SH_TBL_TSCAL))
         {
         sprintf (lcl_result, "TSCAL %1.17g", lcl_tblFld->TSCAL); lcl_ForceRealTrail;
         Tcl_AppendResult (interp, " {", lcl_result, "}", ((char *)0));
         }

      if (shInSet (lcl_tblFld->Tpres, SH_TBL_TZERO))
         {
         sprintf (lcl_result, "TZERO %1.17g", lcl_tblFld->TZERO); lcl_ForceRealTrail;
         Tcl_AppendResult (interp, " {", lcl_result, "}", ((char *)0));
         }

      if (shInSet (lcl_tblFld->Tpres, SH_TBL_TNULLSTR) || shInSet (lcl_tblFld->Tpres, SH_TBL_TNULLINT))
         {
         lcl_prefixElem = "";
         Tcl_AppendResult (interp, " {TNULL {", ((char *)0));
         if (shInSet (lcl_tblFld->Tpres, SH_TBL_TNULLSTR))
            {
            sprintf (lcl_result,   "{STR %s}",                 lcl_tblFld->TNULLSTR);
            Tcl_AppendResult (interp, lcl_result, ((char *)0));
            lcl_prefixElem = " ";
            }
         if (shInSet (lcl_tblFld->Tpres, SH_TBL_TNULLINT))
            {
            sprintf (lcl_result, "%s{INT %d}", lcl_prefixElem, lcl_tblFld->TNULLINT);
            Tcl_AppendResult (interp, lcl_result, ((char *)0));
            }
         Tcl_AppendResult (interp, "}}", ((char *)0));
         }

      if (shInSet (lcl_tblFld->Tpres, SH_TBL_TDISP))
         {
         Tcl_AppendResult (interp, " {TDISP {", lcl_tblFld->TDISP, "}}", ((char *)0));
         }
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    */

rtn_return : ;

   return (lcl_status);

#undef	lcl_ForceRealTrail

   }	/* s_shTclTblFldInfoGet */

/******************************************************************************/

static ftclArgvInfo   shTclTblFldInfoClrArgTable[] = {
      {"<handle>",   FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"<member>",   FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"-field",     FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"-fieldpos",  FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"-case",      FTCL_ARGV_CONSTANT,  (void*) TRUE, NULL, NULL},
      {"-nocase",    FTCL_ARGV_CONSTANT,  (void*) TRUE, NULL, NULL},
      {"-col",       FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {NULL,         FTCL_ARGV_END,  NULL, NULL, NULL}
    };
      
  
static int	 s_shTclTblFldInfoClr
   (
   ClientData	 clientData,		/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	*interp,		/* INOUT: Tcl interpreter structure   */
   int		 argc,			/* IN:    Tcl argument count          */
   char		*argv[]			/* IN:    Tcl arguments               */
   )

/*++
 * ROUTINE:	 s_shTclTblFldInfoClr
 *
 *	Clear particular information about the Table field.
 *
 * RETURN VALUES:
 *
 *   TCL_OK		Success.
 *			The interp result string is empty.
 *
 *   TCL_ERROR		Failure.
 *			The interp result string contains an error message.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* s_shTclTblFldInfoClr */

   /*
    * Declarations.
    */

                  int	  lcl_status     = TCL_OK;
		  int     lcl_flags      = FTCL_ARGV_NO_LEFTOVERS;
                  char	 *lcl_handleExpr = ((char  *)0);
   HANDLE		  lcl_handle;
                  void	 *lcl_handlePtr;
                  char	 *lcl_memberExpr = ((char  *)0);
   TBLFLD_KEYWORDS	  lcl_member;
   unsigned       int	  lcl_uInt;

   TBLCOL		 *lcl_tblCol;
   ARRAY		 *lcl_array;
   TBLFLD		 *lcl_tblFld;		/* TBLFLD structure           */
                  int	  lcl_fldIdx;		/* Field index (0-indexed)    */
                  char	  lcl_decNum[21 /* strlen  ("-9223372036854775808") */ + 1];

		  
                  char   *lcl_fieldValPtr    = NULL;
		  char   *lcl_fieldPosValPtr = NULL;
		  int     lcl_caseVal        = FALSE;
                  int     lcl_nocaseVal      = FALSE;
                  char   *lcl_colValPtr      = NULL;
                  int     lcl_argc           = argc;
                  char   *lcl_argv[10];

   
   /* yet another Kludge! ftcl_ParseArgv alters argc/argv, making it impossible
   * to parse argc/argv twice without saving them first.
   */

   lcl_fldIdx = 0;
   while(lcl_fldIdx < argc) { lcl_argv[lcl_fldIdx] = argv[lcl_fldIdx]; lcl_fldIdx++;}

   shTclTblFldInfoClrArgTable[0].dst = &lcl_handleExpr;
   shTclTblFldInfoClrArgTable[1].dst = &lcl_memberExpr;
   shTclTblFldInfoClrArgTable[2].dst = &lcl_fieldValPtr;
   shTclTblFldInfoClrArgTable[3].dst = &lcl_fieldPosValPtr;
   shTclTblFldInfoClrArgTable[4].dst = &lcl_caseVal;
   shTclTblFldInfoClrArgTable[5].dst = &lcl_nocaseVal;
   shTclTblFldInfoClrArgTable[6].dst = &lcl_colValPtr;
   
   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Parse the command line.
    */

   Tcl_SetResult (interp, "", TCL_VOLATILE);	/* Set TCL_VOLATILE           */


   if (ftcl_ParseArgv (interp, &argc, argv,shTclTblFldInfoClrArgTable,lcl_flags)
            != FTCL_ARGV_SUCCESS)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, "Parsing error\n",     ((char *)0));
      Tcl_AppendResult (interp, shTclTblFldInfoClrUse, ((char *)0));
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

   if ((lcl_status = shKeyLocate (lcl_memberExpr, s_shTblKeys, &lcl_uInt, SH_CASE_SENSITIVE)) != SH_SUCCESS)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, (strcmp (lcl_memberExpr, "*") == 0) ? "Missing" : "Invalid", " member selection", ((char *)0));
      goto rtn_return;
      }
   lcl_member = ((TBLFLD_KEYWORDS)lcl_uInt);	/* To satisfy type checking   */

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * The handling of particular object schemas is done by other Tcl commands.
    */

   if (lcl_handle.type != shTypeGetFromName ("TBLCOL"))
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, shNameGetFromType (lcl_handle.type), " object schemas are not handled by ", shTclTblFldInfoGetName,
                                ((char *)0));
      goto rtn_return;
      }

   lcl_tblCol = ((TBLCOL *)lcl_handle.ptr);

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Locate the field.
    *
    *   o   Only TBLCOL object schemas are handled here.
    */

   if ((lcl_status = s_shTclTblFldLoc (shTclTblFldInfoClrArgTable,2,clientData, interp, lcl_argc, lcl_argv, lcl_tblCol, &lcl_array, &lcl_tblFld, &lcl_fldIdx)) != TCL_OK)
      {
      goto rtn_return;
      }

   /*
    * Clear out the field member.
    */

   if (lcl_tblFld != ((TBLFLD *)0))
      {
      if (shTblTBLFLDclr (lcl_array, lcl_member, ((TBLFLD **)0)) != SH_SUCCESS)
         {
         lcl_status = TCL_ERROR;
         sprintf (lcl_decNum, "%d", lcl_fldIdx);
         Tcl_AppendResult (interp, "Unable to clear ", lcl_memberExpr, " member of field ", lcl_decNum, "\n", ((char *)0));
         shTclInterpAppendWithErrStack (interp);
         goto rtn_return;
         }
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    */

rtn_return : ;

   return (lcl_status);

   }	/* s_shTclTblFldInfoClr */

/******************************************************************************/

static ftclArgvInfo shTclTblFldInfoSetArgTable[] = {
      {"<handle>",   FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"<member>",   FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"<value>",    FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"-field",     FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"-fieldpos",  FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"-case",      FTCL_ARGV_CONSTANT,  (void*) TRUE, NULL, NULL},
      {"-nocase",    FTCL_ARGV_CONSTANT,  (void*) TRUE, NULL, NULL},
      {"-col",       FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {NULL,         FTCL_ARGV_END,  NULL, NULL, NULL}
    };

static int	 s_shTclTblFldInfoSet
   (
   ClientData	 clientData,		/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	*interp,		/* INOUT: Tcl interpreter structure   */
   int		 argc,			/* IN:    Tcl argument count          */
   char		*argv[]			/* IN:    Tcl arguments               */
   )

/*++
 * ROUTINE:	 s_shTclTblFldInfoSet
 *
 *	Set particular information about the Table field.
 *
 * RETURN VALUES:
 *
 *   TCL_OK		Success.
 *			The interp result string is empty.
 *
 *   TCL_ERROR		Failure.
 *			The interp result string contains an error message.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* s_shTclTblFldInfoSet */

   /*
    * Declarations.
    */

                  int	  lcl_status     = TCL_OK;
                  int     lcl_flags      = FTCL_ARGV_NO_LEFTOVERS;
                  char	 *lcl_handleExpr = ((char *)0);
   HANDLE		  lcl_handle;
                  void	 *lcl_handlePtr;
                  char	 *lcl_memberExpr = ((char *)0);
   TBLFLD_KEYWORDS	  lcl_member;
                  char	 *lcl_valueAscii = ((char *)0);
   double		  lcl_valueDouble;
                  int	  lcl_valueInt;
                  int	  lcl_tokCnt;
   unsigned       int	  lcl_uInt;

   TBLCOL		 *lcl_tblCol;
   ARRAY		 *lcl_array;
   TBLFLD		 *lcl_tblFld;		/* TBLFLD structure           */
                  int	  lcl_fldIdx;		/* Field index (0-indexed)    */
                  char	  lcl_char;
                  char	  lcl_result[255+1];	/* Make this larger than any  */
                  char	 *lcl_resultPos;	/* number and some characters */
						/* for generating one element */
						/* in the output keyed list.  */

                  char   *lcl_fieldValPtr    = NULL;
		  char   *lcl_fieldPosValPtr = NULL;
		  int     lcl_caseVal        = FALSE;
                  int     lcl_nocaseVal      = FALSE;
                  char   *lcl_colValPtr      = NULL;
                  int     lcl_argc           = argc;
                  char   *lcl_argv[10];

   
   /* yet another Kludge! ftcl_ParseArgv alters argc/argv, making it impossible
   * to parse argc/argv twice without saving them first.
   */

   lcl_fldIdx = 0;
   while(lcl_fldIdx < argc) { lcl_argv[lcl_fldIdx] = argv[lcl_fldIdx]; lcl_fldIdx++;}

#define	lcl_ForceRealTrail\
	if ((lcl_resultPos = strchr (lcl_result, '.')) != ((char *)0))\
	   {\
	   if (*(++lcl_resultPos) == '\000') { *lcl_resultPos = '0'; *(++lcl_resultPos) = '\000'; }\
	   }\
	else\
	   {\
		if ( strchr(lcl_result,'e')  == ((char*)0) ) \
	   		strcat (lcl_result, ".0");\
	   }

#define	lcl_caseSet(      keyword, keywordType)\
   case SH_TBL_##keyword :\
        if (shTblTBLFLDsetWith##keywordType (lcl_array, lcl_member, lcl_value##keywordType, &lcl_tblFld) != SH_SUCCESS)\
           {\
           lcl_status = TCL_ERROR;\
           sprintf (lcl_result, "%d", lcl_fldIdx);\
           Tcl_AppendResult (interp, "Unable to set ", lcl_memberExpr, " member of field ", lcl_result, "\n", ((char *)0));\
           shTclInterpAppendWithErrStack (interp);\
           goto rtn_return;\
           }

#define	lcl_resultAscii(  keyword)\
   Tcl_AppendResult (interp,      lcl_tblFld->keyword, ((char *)0))

#define	lcl_resultDouble( keyword)\
   sprintf (lcl_result, "%1.17g", lcl_tblFld->keyword); lcl_ForceRealTrail; Tcl_AppendResult (interp, lcl_result, ((char *)0))

#define	lcl_resultInt(    keyword)\
   sprintf (lcl_result, "%d",     lcl_tblFld->keyword);                     Tcl_AppendResult (interp, lcl_result, ((char *)0))


   shTclTblFldInfoSetArgTable[0].dst = &lcl_handleExpr;
   shTclTblFldInfoSetArgTable[1].dst = &lcl_memberExpr;
   shTclTblFldInfoSetArgTable[2].dst = &lcl_valueAscii;
   shTclTblFldInfoSetArgTable[3].dst = &lcl_fieldValPtr;
   shTclTblFldInfoSetArgTable[4].dst = &lcl_fieldPosValPtr;
   shTclTblFldInfoSetArgTable[5].dst = &lcl_caseVal;
   shTclTblFldInfoSetArgTable[6].dst = &lcl_nocaseVal;
   shTclTblFldInfoSetArgTable[7].dst = &lcl_colValPtr;
   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Parse the command line.
    */

   Tcl_SetResult (interp, "", TCL_VOLATILE);	/* Set TCL_VOLATILE           */

   
   if (ftcl_ParseArgv (interp, &argc, argv,shTclTblFldInfoSetArgTable, lcl_flags )
              != FTCL_ARGV_SUCCESS)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, "Parsing error\n",     ((char *)0));
      Tcl_AppendResult (interp, shTclTblFldInfoSetUse, ((char *)0));
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

   if ((lcl_status = shKeyLocate (lcl_memberExpr, s_shTblKeys, &lcl_uInt, SH_CASE_SENSITIVE)) != SH_SUCCESS)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, (strcmp (lcl_memberExpr, "*") == 0) ? "Missing" : "Invalid", " member selection", ((char *)0));
      goto rtn_return;
      }
   lcl_member = ((TBLFLD_KEYWORDS)lcl_uInt);	/* To satisfy type checking   */

   switch (lcl_member)
      {
      case SH_TBL_TTYPE    :
      case SH_TBL_TUNIT    :
      case SH_TBL_TNULLSTR :
      case SH_TBL_TDISP    :
                           break;

      case SH_TBL_TSCAL    :
      case SH_TBL_TZERO    :
                           lcl_tokCnt = sscanf (lcl_valueAscii, " %lf %c", &lcl_valueDouble, &lcl_char);
                           if (lcl_tokCnt != 1)
                              {
                              lcl_status = TCL_ERROR;
                              Tcl_AppendResult (interp, "Bad member value: requires a real number", ((char *)0));
                              goto rtn_return;
                              }
                           break;
      case SH_TBL_TNULLINT :
                           lcl_tokCnt = sscanf (lcl_valueAscii, " %d %c",  &lcl_valueInt,    &lcl_char);
                           if (lcl_tokCnt != 1)
                              {
                              lcl_status = TCL_ERROR;
                              Tcl_AppendResult (interp, "Bad member value: requires an integer", ((char *)0));
                              goto rtn_return;
                              }
                           break;

      default            :
           lcl_status = SH_INTERNAL_ERR;
           shFatal ("Internal error (%s line %d): members accepted above not reflected here", __FILE__, __LINE__);
           goto rtn_return;
           break;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * The handling of particular object schemas is done by other Tcl commands.
    */

   if (lcl_handle.type != shTypeGetFromName ("TBLCOL"))
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, shNameGetFromType (lcl_handle.type), " object schemas are not handled by ", shTclTblFldInfoGetName,
                                ((char *)0));
      goto rtn_return;
      }

   lcl_tblCol = ((TBLCOL *)lcl_handle.ptr);

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Locate the field.
    *
    *   o   Only TBLCOL object schemas are handled here.
    *
    *   o   Floating point values are output with the %g format specifier.  The
    *       precision depends on the type:
    *
    *           float    8 decimal digits
    *           double  17 decimal digits
    *
    *       The %g format specifier cuts off trailing zeros, even the decimal
    *       point if appropriate;  but, this code will output, at a minimum, a
    *       trailing decimal point and zero (lcl_ForceRealTrail).
    */

   if ((lcl_status = s_shTclTblFldLoc (shTclTblFldInfoSetArgTable, 3, clientData, interp, lcl_argc, lcl_argv, lcl_tblCol, &lcl_array, &lcl_tblFld, &lcl_fldIdx)) != TCL_OK)
      {
      goto rtn_return;
      }

   /*
    * Set the field member.
    */

   switch (lcl_member)
      {
      lcl_caseSet (TTYPE,    Ascii);  lcl_resultAscii  (TTYPE);    break;
      lcl_caseSet (TUNIT,    Ascii);  lcl_resultAscii  (TUNIT);    break;
      lcl_caseSet (TNULLSTR, Ascii);  lcl_resultAscii  (TNULLSTR); break;
      lcl_caseSet (TDISP,    Ascii);  lcl_resultAscii  (TDISP);    break;
      lcl_caseSet (TSCAL,    Double); lcl_resultDouble (TSCAL);    break;
      lcl_caseSet (TZERO,    Double); lcl_resultDouble (TZERO);    break;
      lcl_caseSet (TNULLINT, Int);    lcl_resultInt    (TNULLINT); break;

      default : lcl_status = SH_INTERNAL_ERR;
                shFatal ("Internal error (%s line %d): members accepted above not reflected here", __FILE__, __LINE__);
                goto rtn_return;
                break;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    */

rtn_return : ;

   return (lcl_status);

#undef	lcl_ForceRealTrail
#undef	lcl_caseSet
#undef	lcl_resultAscii
#undef	lcl_resultDouble
#undef	lcl_resultInt

   }	/* s_shTclTblFldInfoSet */

/******************************************************************************/

static ftclArgvInfo shTclTblFldGetArgTable[] = {
      {"<handle>",   FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"[indices]",  FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"-field",     FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"-fieldpos",  FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"-case",      FTCL_ARGV_CONSTANT,  (void*) TRUE, NULL, NULL},
      {"-nocase",    FTCL_ARGV_CONSTANT,  (void*) TRUE, NULL, NULL},
      {"-col",       FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {NULL,         FTCL_ARGV_END,  NULL, NULL, NULL}
    };

static int	 s_shTclTblFldGet
   (
   ClientData	 clientData,		/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	*interp,		/* INOUT: Tcl interpreter structure   */
   int		 argc,			/* IN:    Tcl argument count          */
   char		*argv[]			/* IN:    Tcl arguments               */
   )

/*++
 * ROUTINE:	 s_shTclTblFldGet
 *
 * DESCRIPTION:
 *
 *	Return a value from a Table field. If the field is an array, all or part
 *	of the array may be returned.  If all indices are specified, then only a
 *	single array element (a scalar) is returned. Otherwise, if fewer indices
 *	are passed, an array (or arrays) is returned as a Tcl list.  Note that
 *	the first index is the Table row index. If that is not passed (therefore
 *	no indices are passed), all values for the specified field for all Table
 *	Table rows will be returned.
 *
 *	Data contained in heap is handled differently (and in a much more hard-
 *	wired fashion). One additional index is permitted beyond ARRAY's dimCnt.
 *	It can specify the single element within the variable length array.
 * 
 * RETURN VALUES:
 *
 *   TCL_OK		Success.  Normal successful completion.
 *			The Table field was retrieved. The interp result string
 *			contains the value(s). N-dimensional arrays are returned
 *			as Tcl lists (lists within lists are where n is greater
 *			than 1).
 *
 *   TCL_ERROR		Failure.
 *			The interp result string contains an error message.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	If the data area exists, shArrayDataAlloc ()   M U S T   have been used
 *	to allocate it.  This is necessary because of a dependency on subArrCnt.
 *
 *	   o	Array elements   M U S T   be contiguous, with the fastest
 *		varying indexed elements being adjacent to each other.
 *
 *   o	Performance can be poor and memory fragmentation can occur.  This is
 *	possible since Tcl_AppendResult () is used by p_shTclArrayPrimRet ()
 *	for each array element returned.
 *--
 ******************************************************************************/

   {	/* s_shTclTblFldGet */

   /*
    * Declarations.
    */

                  int	  lcl_status     = TCL_OK;
                  int     lcl_flags      = FTCL_ARGV_NO_LEFTOVERS;
                  char	 *lcl_handleExpr = ((char  *)0);
   HANDLE		  lcl_handle;
                  void	 *lcl_handlePtr;
                  int	  lcl_indicesCnt;
                  int	  lcl_indices[(shBinTDIMlim)+1+1];/* +1 for TBLHEAPDSC*/

#if ((shArrayDimLim)     <           ((shBinTDIMlim)+1))
#error	ARRAYs do not support all possible FITS Binary Table array dimensions
#endif

   ARRAY		 *lcl_array;
   TBLFLD		 *lcl_tblFld;		/* TBLFLD structure           */
                  int	  lcl_fldIdx;		/* Field index (0-indexed)    */
                  int	  lcl_dimIdx;
            long  int	  lcl_elemCnt;		/* # array elems to return    */
   unsigned       char	 *lcl_elemPtr;
            long  int	  lcl_elemIdx;
            long  int	  lcl_elemOff;
                  int	  lcl_heap;		/* Bool: TBLHEAPDSC case?     */
                  int	  lcl_heap1elem = 0;	/* Bool: 1 heap element only? */
            long  int	  lcl_heapIdx;
   TBLHEAPDSC		 *lcl_heapDsc;
   SHALIGN_TYPE		  lcl_alignType;
                  char	 *lcl_prefixList;
                  char	 *lcl_prefixElem;
                  char	  lcl_decNum[21 /* strlen  ("-9223372036854775808") */ + 1];

   TYPE			  lcl_schemaTypeSTR = shTypeGetFromName ("STR");

                  char	 *lcl_nl = "";
   static         char	 *    nl = "\n";


                  char   *lcl_indicesValPtr  = NULL;
                  char   *lcl_fieldValPtr    = NULL;
		  char   *lcl_fieldPosValPtr = NULL;
		  int     lcl_caseVal        = FALSE;
                  int     lcl_nocaseVal      = FALSE;
                  char   *lcl_colValPtr      = NULL;
                  int     lcl_argc           = argc;
                  char   *lcl_argv[10];

   
   /* yet another Kludge! ftcl_ParseArgv alters argc/argv, making it impossible
   * to parse argc/argv twice without saving them first.
   */

   lcl_fldIdx = 0;
   while(lcl_fldIdx < argc) { lcl_argv[lcl_fldIdx] = argv[lcl_fldIdx]; lcl_fldIdx++;}
   shTclTblFldGetArgTable[0].dst = &lcl_handleExpr;
   shTclTblFldGetArgTable[1].dst = &lcl_indicesValPtr;
   shTclTblFldGetArgTable[2].dst = &lcl_fieldValPtr;
   shTclTblFldGetArgTable[3].dst = &lcl_fieldPosValPtr;
   shTclTblFldGetArgTable[4].dst = &lcl_caseVal;
   shTclTblFldGetArgTable[5].dst = &lcl_nocaseVal;
   shTclTblFldGetArgTable[6].dst = &lcl_colValPtr;

   /***************************************************************************/
   /*
    * Parse the command line.
    */

   Tcl_SetResult (interp, "", TCL_VOLATILE);	/* Set TCL_VOLATILE           */

   if (ftcl_ParseArgv (interp, &argc, argv,shTclTblFldGetArgTable, lcl_flags )
              != FTCL_ARGV_SUCCESS)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, "Parsing error\n", ((char *)0));
      Tcl_AppendResult (interp, shTclTblFldGetUse, ((char *)0));
      goto rtn_return;
      }

   /*
    * Perform some argument checking.
    */

   if (shTclHandleExprEval (interp, lcl_handleExpr, &lcl_handle, &lcl_handlePtr) != TCL_OK)
      {
      lcl_status = TCL_ERROR;	/* shTclHandleExprEval sets interp result     */
      goto rtn_return;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * The handling of particular object schemas is done by other Tcl commands.
    */

   if (lcl_handle.type != shTypeGetFromName ("TBLCOL"))
      {
      Tcl_AppendResult (interp, lcl_nl, shNameGetFromType (lcl_handle.type), " object schemas are not handled by ",
                                shTclTblFldGetName, ((char *)0)); lcl_nl = nl;
      goto rtn_return;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Locate the field.
    *
    *   o   Only TBLCOL object schemas are handled here.
    *
    * Some errors are allowed to drop through to locate more potential errors.
    */

   if (s_shTclTblFldLoc (shTclTblFldGetArgTable, 2, clientData, interp, lcl_argc, lcl_argv, ((TBLCOL *)lcl_handle.ptr),
       &lcl_array, &lcl_tblFld, &lcl_fldIdx) != TCL_OK)
      {
      lcl_status = TCL_ERROR;
      goto rtn_return;
      }

   /*
    *   o   Get indices.  If too few are passed, default the missing indices on
    *       the right (fastest varying) to 0 (1st element of the array at that
    *       level/dimension).
    *
    *          o   Check index validity.
    *
    *          o   TBLHEAPDSCs are handled specially.  One index beyond ARRAY's
    *              dimCnt is allowed to index into the variable length array in
    *              heap.  In that case, lcl_indicesCnt is reduced by 1, since
    *              lcl_indices will only be used to locate the TBLHEAPDSC, not
    *              the actual data in the heap. But, lcl_indices[lcl_indicesCnt]
    *              (after lcl_indicesCnt is decremented) will contain the index
    *              into the variable length array.
    */

   lcl_heap = (lcl_array->data.schemaType == shTypeGetFromName ("TBLHEAPDSC"));

   if (p_shTclListValRet (interp, shTypeGetFromName ("INT"), lcl_indicesValPtr, "*", 0,
       lcl_array->dimCnt + ((lcl_heap) ? 1 : 0), SH_ARRAY_TRAIL_ZERO, &lcl_indicesCnt, ((void *)lcl_indices), SH_ARRAY_VAL_NON_NEG,
       ((void *)lcl_array->dim), 0, (lcl_array->dimCnt - 1), "index", "indices", "Table row", 0, 0, SH_ARRAY_STR_FULL, ((int *)0))
       != TCL_OK)
      {
      lcl_status = TCL_ERROR;
      goto rtn_return;
      }

   if (lcl_heap && (lcl_indicesCnt == (lcl_array->dimCnt + 1)))
      {
       lcl_heap1elem = 1;
       lcl_indicesCnt--;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Get the field value.
    *
    *   o   Only TBLCOL object schemas are handled here.
    *
    *   o   If more than one element is asked for, then a list (of a list ...)
    *       of elements is returned.  The number of elements to return is
    *
    *                     ____array->dimCnt-1
    *                      ||
    *       lcl_elemCnt =  ||     array->dim[i]
    *                      ||
    *                        i=lcl_indicesCnt
    *
    *       This can be gotten from array->subArrCnt, which is faster.
    *
    *   o   Array elements  M U S T   be stored sequentially in row-major order.
    */

   if (lcl_array->data.dataPtr == ((unsigned char *)0))
      {
      lcl_status = TCL_ERROR;
      sprintf (lcl_decNum, "%d", lcl_fldIdx);
      Tcl_AppendResult (interp, lcl_nl, "No data exists for field ", lcl_decNum, ((char *)0));
      goto rtn_return;
      }

   lcl_elemCnt = (lcl_indicesCnt > 0) ? lcl_array->subArrCnt[lcl_indicesCnt-1]
                                      : lcl_array->subArrCnt[0] * lcl_array->dim[0];

   /*
    *   o   Find the offset of the first element to look at.
    */

   if (lcl_indicesCnt == lcl_array->dimCnt) { lcl_elemOff = lcl_indices[lcl_indicesCnt-1]; lcl_dimIdx = lcl_indicesCnt - 2; }
                                      else  { lcl_elemOff =                            0;  lcl_dimIdx = lcl_indicesCnt - 1; }

   for ( ; lcl_dimIdx >= 0; lcl_dimIdx--)
      {
      lcl_elemOff += (lcl_array->subArrCnt[lcl_dimIdx] * lcl_indices[lcl_dimIdx]);
      }

   lcl_elemPtr = ((unsigned char *)lcl_array->data.dataPtr) + (lcl_elemOff * lcl_array->data.incr);

   /*
    * Retrieve the value(s).
    *
    *   o   Heap (TBLHEAPDSC) is handled specially.
    *
    *          o   Data in heap is treated as 1-dimensional.
    *
    *          o   TBLHEAPDSCs are assumed to be in a 1-dimensional ARRAY.
    */

   if (!lcl_heap)
      {
      lcl_status = p_shTclArrayPrimRet (interp, lcl_array, (lcl_indicesCnt - 1), &lcl_elemPtr, &lcl_elemCnt, 0);
      }
   else
      {
      if (lcl_array->dimCnt > 1)
         {
         lcl_status = TCL_ERROR;
         Tcl_AppendResult (interp, "Getting heap data from multidimensional ARRAY not supported", ((char *)0));
         goto rtn_return;
         }
      if (lcl_tblFld == ((TBLFLD *)0))
         {
         lcl_status = TCL_ERROR;
         sprintf (lcl_decNum, "%d", lcl_fldIdx + 1);
         Tcl_AppendResult (interp, "Heap information (field", lcl_decNum, ") missing", ((char *)0));
         goto rtn_return;
         }
      if (!shInSet (lcl_tblFld->Tpres, SH_TBL_HEAP))
         {
         lcl_status = TCL_ERROR;
         sprintf (lcl_decNum, "%d", lcl_fldIdx + 1);
         Tcl_AppendResult (interp, "Heap information (field", lcl_decNum, ") missing", ((char *)0));
         goto rtn_return;
         }

      lcl_heapDsc   = ((TBLHEAPDSC *)lcl_elemPtr);
      lcl_alignType = shAlignSchemaToAlignMap (lcl_tblFld->heap.schemaType);

      if (lcl_heap1elem != 0)
         {
         /*
          * Display a single element from the variable length array.
          */

         if (lcl_indices[lcl_indicesCnt] >= lcl_heapDsc->cnt)
            {
            lcl_status = TCL_ERROR;
            sprintf (lcl_decNum, "%d", lcl_indicesCnt + 1);
            Tcl_AppendResult (interp, lcl_nl, lcl_decNum, shNth (lcl_indicesCnt + 1), " index out of bounds", ((char *)0));
            goto rtn_return;
            }
         if ((lcl_status = p_shTclAlignPrimElemRet (interp, lcl_alignType, lcl_tblFld->heap.schemaType, 1,
                               lcl_heapDsc->ptr + (lcl_indices[lcl_indicesCnt] * lcl_tblFld->heap.incr), "") ) != TCL_OK)
            {
            goto rtn_return;
            }
         }
      else
         {
         /*
          * Display one or more rows of variable length arrays.
          */

         lcl_prefixList = "";
         for (lcl_elemIdx = 0; lcl_elemIdx < lcl_elemCnt; lcl_elemIdx++)
            {
            if (lcl_elemCnt > 1)
               {
               Tcl_AppendResult (interp, lcl_prefixList, "{", ((char *)0)); lcl_prefixList = " ";
               }
            if (lcl_tblFld->heap.schemaType == lcl_schemaTypeSTR)
               {
               if ((lcl_status = p_shTclAlignPrimElemRet (interp, lcl_alignType, lcl_tblFld->heap.schemaType, lcl_heapDsc->cnt,
                                                          lcl_heapDsc->ptr, "") ) != TCL_OK)
                  {
                  goto rtn_return;
                  }
               }
            else
               {
               lcl_prefixElem = "";
               for (lcl_heapIdx = 0; lcl_heapIdx < lcl_heapDsc->cnt; lcl_heapIdx++)
                  {
                  if ((lcl_status = p_shTclAlignPrimElemRet (interp, lcl_alignType, lcl_tblFld->heap.schemaType, 0,
                                        lcl_heapDsc->ptr + (lcl_heapIdx * lcl_tblFld->heap.incr), lcl_prefixElem) ) != TCL_OK)
                     {
                     goto rtn_return;
                     }
                  lcl_prefixElem = " ";
                  }
               }
            if (lcl_elemCnt > 1)		/* Done with this row's array */
               {
               Tcl_AppendResult (interp, "}", ((char *)0));
               }
            lcl_heapDsc    = ((TBLHEAPDSC *)(((unsigned char *)lcl_heapDsc) + lcl_array->data.incr));  /* Onto next Table row */
            }
         }
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    */

rtn_return : ;

   return (lcl_status);

   }	/* s_shTclTblFldGet */

/******************************************************************************/


static ftclArgvInfo shTclTblFldSetArgTable[] = {
      {"<handle>",   FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"<indices>",  FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"<values>",   FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"-field",     FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"-fieldpos",  FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"-case",      FTCL_ARGV_CONSTANT,  (void*) TRUE, NULL, NULL},
      {"-nocase",    FTCL_ARGV_CONSTANT,  (void*) TRUE, NULL, NULL},
      {"-col",       FTCL_ARGV_STRING,  NULL, NULL, NULL},
      {"-trunc",      FTCL_ARGV_CONSTANT,  (void*) TRUE, NULL, NULL},
      {"-notrunc",    FTCL_ARGV_CONSTANT,  (void*) TRUE, NULL, NULL},
      {NULL,         FTCL_ARGV_END,  NULL, NULL, NULL}
    };

static int	 s_shTclTblFldSet
   (
   ClientData	 clientData,		/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	*interp,		/* INOUT: Tcl interpreter structure   */
   int		 argc,			/* IN:    Tcl argument count          */
   char		*argv[]			/* IN:    Tcl arguments               */
   )

/*++
 * ROUTINE:	 s_shTclTblFldSet
 *
 * DESCRIPTION:
 *
 * RETURN VALUES:
 *
 *   TCL_OK		Success.  Normal successful completion.
 *			The Table field value(s) was set.  The interp result
 *			string contains the starting array indices and the
 *			ending array indices (inclusive).
 *
 *   TCL_ERROR		Failure.
 *			The interp result string contains an error message.
 *
 * SIDE EFFECTS:
 *
 *   o	For a parameter, ftclFullParseArg () treats a list of lists as multiple
 *	parameters.  This is unacceptable for character string arguments (STR).
 *	The work-around solves this problem.  In addition, it now permits the
 *	first numeric value to be negative.  The drawbacks are:
 *
 *	   o	No options (argv[] elements which begin with a dash (-)) can
 *		begin with a decimal digit (e.g., "-132" would be treated as a
 *		numeric parameter, as would "-1option").
 *
 *	   o	String values which begin with a dash (-) followed by a charac-
 *		ter which is NOT decimal digits will NEVER be treated as the
 *		3rd parameter (named "values").
 *
 *	These limitations disappear if these "options" appear after the 3rd
 *	parameter (named "values").
 *
 * SYSTEM CONCERNS:
 *
 *   o	If the data area exists, shArrayDataAlloc ()   M U S T   have been used
 *	to allocate it.  This is necessary because of a dependency on subArrCnt.
 *
 *	   o	Array elements   M U S T   be contiguous, with the fastest
 *		varying indexed elements being adjacent to each other.
 *
 *   o	Performance is degraded slightly by reading values into a temporary
 *	area and then copying to the final destination in the ARRAY.  This
 *	protects against any bad values aborting the values reading, leaving
 *	a partially filled ARRAY.
 *--
 ******************************************************************************/

   {	/* s_shTclTblFldSet */

   /*
    * Declarations.
    */

                  int	  lcl_status     = TCL_OK;
                  int     lcl_flags      = FTCL_ARGV_NO_LEFTOVERS;
                  char	 *lcl_handleExpr = ((char  *)0);
   HANDLE		  lcl_handle;
                  void	 *lcl_handlePtr;
                  int	  lcl_optTRUNC;
                  int	  lcl_optNOTRUNC;
   ARRAY_STRTRUNC	  lcl_optTRUNCval;
                  int	  lcl_indicesCnt;
                  int	  lcl_indices[(shBinTDIMlim)+1+1];/* +1 for TBLHEAPDSC*/

#if ((shArrayDimLim)     <           ((shBinTDIMlim)+1))
#error	ARRAYs do not support all possible FITS Binary Table array dimensions
#endif
                  char	 *lcl_prmVALUES;	/* VALUES parameter       ### */
                  int	  lcl_prmVALUESidx = -1;/* VALUES came from argv[]### */
                  int	  lcl_prmVALUEScnt;	/* VALUES came from argv[]### */

   ARRAY		 *lcl_array;
   TBLFLD		 *lcl_tblFld;		/* TBLFLD structure           */
                  int	  lcl_fldIdx;		/* Field index (0-indexed)    */
                  int	  lcl_dimIdx;
   unsigned       char	 *lcl_values    = ((unsigned char *)0);
                  int	  lcl_valuesCnt;	/* #   values passed          */
                  int	  lcl_valuesMax;	/* Max values permitted       */
                  int	  lcl_strLen1stMax;	/* Max length 1st string      */
                  int	  lcl_strLenMax;	/* Max length rest of strings */
                  int	  lcl_strLenLast;	/* Length of last string      */
#if 0
   unsigned       char	 *lcl_elemPtr;
#endif
            long  int	  lcl_elemIdx;
                  int	  lcl_heap;		/* Bool: TBLHEAPDSC case?     */
                  int	  lcl_heap1elem = 0;	/* Bool: 1 heap element only? */
#if 0
   TBLHEAPDSC		 *lcl_heapDsc;
   SHALIGN_TYPE		  lcl_alignType;
#endif
                  char	 *lcl_prefixElem;
/* char	 *lcl_prefixList; */	/* unused */
                  char	  lcl_decNum[21 /* strlen  ("-9223372036854775808") */ + 1];
                  char	  lcl_result[255+1];	/* Make this larger than any  */
						/* number and some characters.*/

   TYPE			  lcl_schemaTypeSTR = shTypeGetFromName ("STR");

                  char	 *lcl_nl = "";
   static         char	 *    nl = "\n";

                
                  char   *lcl_indicesValPtr  = NULL;
                  char   *lcl_valueValPtr    = NULL;
                  char   *lcl_fieldValPtr    = NULL;
		  char   *lcl_fieldPosValPtr = NULL;
		  int     lcl_caseVal        = FALSE;
                  int     lcl_nocaseVal      = FALSE;
                  char   *lcl_colValPtr      = NULL;
                  int     lcl_truncVal        = FALSE;
                  int     lcl_notruncVal      = FALSE;
                  int     lcl_argc           = argc;
                  char   *lcl_argv[10];

   
   /* yet another Kludge! ftcl_ParseArgv alters argc/argv, making it impossible
   * to parse argc/argv twice without saving them first.
   */

   lcl_fldIdx = 0;
   while(lcl_fldIdx < argc) { lcl_argv[lcl_fldIdx] = argv[lcl_fldIdx]; lcl_fldIdx++;}
   shTclTblFldSetArgTable[0].dst = &lcl_handleExpr;
   shTclTblFldSetArgTable[1].dst = &lcl_indicesValPtr;
   shTclTblFldSetArgTable[2].dst = &lcl_valueValPtr;
   shTclTblFldSetArgTable[3].dst = &lcl_fieldValPtr;
   shTclTblFldSetArgTable[4].dst = &lcl_fieldPosValPtr;
   shTclTblFldSetArgTable[5].dst = &lcl_caseVal;
   shTclTblFldSetArgTable[6].dst = &lcl_nocaseVal;
   shTclTblFldSetArgTable[7].dst = &lcl_colValPtr;
   shTclTblFldSetArgTable[8].dst = &lcl_truncVal;
   shTclTblFldSetArgTable[9].dst = &lcl_notruncVal;
   

   /***************************************************************************/
   /*
    * Parse the command line.
    *									  ###
    *   o   There are two HUGE holes in ftclFullParseArg ():		  ###
    *									  ###
    *          o   If a parameter (not an option) is a Tcl list of lists, ###
    *              ftclFullParseArg () treats it as multiple parameters	  ###
    *              (even though it's contained in one argv[] element).	  ###
    *									  ###
    *          o   A negative numeric parameter is treated as an option.  ###
    *									  ###
    *       The work-around is to locate the parameter (2nd in our case)  ###
    *       and set it to something benign, all the while retaining the	  ###
    *       original argv[] value.					  ###
    *									  ###
    *          o   tblFldSet can (reasonably) safely assume that negative ###
    *              numbers are NOT an option.				  ###
    *									  ###
    *          o   argv[0], the command name, can be skipped.		  ###
    *									  ###
    *       lcl_prmVALUES will contain the original argv[] pointer, so be ###
    *       C A R E F U L   not to free lcl_prmVALUESspace.  Instead, the ###
    *       original argv[] must be restored (see the code at the bottom).###
    */
                    
   Tcl_SetResult (interp, "", TCL_VOLATILE);	/* Set TCL_VOLATILE           */


   for (lcl_prmVALUEScnt = 0, lcl_prmVALUESidx = 1; lcl_prmVALUESidx < argc; lcl_prmVALUESidx++)	/*### */
      {													/*### */
      if (sscanf (argv[lcl_prmVALUESidx], " -%2[^0123456789]", lcl_result) != 1)			/*### */
         {								/*### */
         if (++lcl_prmVALUEScnt == 3) { break; }/* Ignore handle & indices### */
         }								/*### */
      }									/*### */
   if  (lcl_prmVALUESidx < argc)		/* Avoid potential syntax ### */
      {						/* errors (ftclFullParse) ### */
      lcl_prmVALUES = argv[lcl_prmVALUESidx];		/* Save param     ### */
                      argv[lcl_prmVALUESidx] = "";	/* Benign value   ### */
      }									/*### */
									/*### */
   if (ftcl_ParseArgv (interp, &argc, argv, shTclTblFldSetArgTable, lcl_flags)
                != FTCL_ARGV_SUCCESS)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, "Parsing error\n", ((char *)0));
      Tcl_AppendResult (interp, shTclTblFldSetUse, ((char *)0));
      goto rtn_return;
      }

									/*### */
   if (lcl_prmVALUESidx > argc)			/* Somehow missed param.  ### */
      {						/* Try to get it through  ### */
      /* lcl_prmVALUES = ftclGetStr ("values");	*/ /* normal ftclFullParse   ### */
         lcl_prmVALUES = lcl_valueValPtr;
      }						/* channels.              ### */

#if 0
   lcl_optTRUNC   = ftclPresent (  "trunc");
   lcl_optNOTRUNC = ftclPresent ("notrunc");  
#endif
   lcl_optTRUNC   = (lcl_truncVal == TRUE);
   lcl_optNOTRUNC = (lcl_notruncVal == TRUE);


   

   /*
    * Perform some argument checking.
    */

   if ( lcl_optTRUNC &&  lcl_optNOTRUNC)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, lcl_nl, "Conflicting options: -trunc and -notrunc", ((char *)0)); lcl_nl = nl;
      }

   lcl_optTRUNCval = (lcl_optTRUNC) ? SH_ARRAY_STR_TRUNC : SH_ARRAY_STR_FULL;

   if (shTclHandleExprEval (interp, lcl_handleExpr, &lcl_handle, &lcl_handlePtr) != TCL_OK)
      {
      lcl_status = TCL_ERROR;	/* shTclHandleExprEval sets interp result     */
      goto rtn_return;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * The handling of particular object schemas is done by other Tcl commands.
    */

   if (lcl_handle.type != shTypeGetFromName ("TBLCOL"))
      {
      Tcl_AppendResult (interp, lcl_nl, shNameGetFromType (lcl_handle.type), " object schemas are not handled by ",
                                shTclTblFldGetName, ((char *)0)); lcl_nl = nl;
      goto rtn_return;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Locate the field.
    *
    *   o   Only TBLCOL object schemas are handled here.
    *
    * Some errors are allowed to drop through to locate more potential errors.
    */

   if (s_shTclTblFldLoc (shTclTblFldSetArgTable, 3, clientData, interp, lcl_argc, lcl_argv, ((TBLCOL *)lcl_handle.ptr), &lcl_array, &lcl_tblFld, &lcl_fldIdx)
       != TCL_OK)
      {
      lcl_status = TCL_ERROR;
      goto rtn_return;
      }

   /*
    *   o   Get indices.  If too few are passed, default the missing indices on
    *       the right (fastest varying) to 0 (1st element of the array at that
    *       level/dimension).
    *
    *          o   Check index validity.
    *
    *          o   TBLHEAPDSCs are handled specially.  One index beyond ARRAY's
    *              dimCnt is allowed to index into the variable length array in
    *              heap.  In that case, lcl_indicesCnt is reduced by 1, since
    *              lcl_indices will only be used to locate the TBLHEAPDSC, not
    *              the actual data in the heap. But, lcl_indices[lcl_indicesCnt]
    *              (after lcl_indicesCnt is decremented) will contain the index
    *              into the variable length array.
    */

   lcl_heap = (lcl_array->data.schemaType == shTypeGetFromName ("TBLHEAPDSC"));

   if (p_shTclListValRet (interp, shTypeGetFromName ("INT"), lcl_indicesValPtr, "*", 0,
       lcl_array->dimCnt + ((lcl_heap) ? 1 : 0), SH_ARRAY_TRAIL_ZERO, &lcl_indicesCnt, ((void *)lcl_indices), SH_ARRAY_VAL_NON_NEG,
       ((void *)lcl_array->dim), 0, (lcl_array->dimCnt - 1), "index", "indices", "Table row", 0, 0, SH_ARRAY_STR_FULL, ((int *)0))
       != TCL_OK)
      {
      lcl_status = TCL_ERROR;
      goto rtn_return;
      }

   if (lcl_heap && (lcl_indicesCnt == (lcl_array->dimCnt + 1)))
      {
       lcl_heap1elem = 1;
       lcl_indicesCnt--;
      }

   /*
    *   o   From the indices, determine the maximum number of array elements
    *       that can be set.
    */

   if (lcl_indicesCnt == lcl_array->dimCnt) { lcl_elemIdx = lcl_indices[lcl_indicesCnt-1]; lcl_dimIdx = lcl_indicesCnt - 2; }
                                      else  { lcl_elemIdx =                            0;  lcl_dimIdx = lcl_indicesCnt - 1; }
   for ( ; lcl_dimIdx >= 0; lcl_dimIdx--)
      {
      lcl_elemIdx += (lcl_array->subArrCnt[lcl_dimIdx] * lcl_indices[lcl_dimIdx]);
      }

   lcl_valuesMax = (lcl_array->subArrCnt[0] * lcl_array->dim[0]) - lcl_elemIdx;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Set the field value(s).
    *
    *   o   Only TBLCOL object schemas are handled here.
    *
    *   o   Array elements  M U S T   be stored sequentially in row-major order.
    *
    *   o   Data is first read into a local array to protect the ARRAY data in
    *       case there are any errors (for efficiency, p_shTclListValRet ()
    *       checks data as it's reading it in, thus possibly aborting after
    *       modifying previous values).
    *
    *   o   Strings (STR) are handled specially.
    *
    *          o   The limits are the number of strings rather than the number
    *              of characters.
    *
    *          o   p_shTclListValRet () does not fill the strings into the array
    *              unless they are _all_ legal.
    *
    *   o   Heap (TBLHEAPDSC) is handled specially.
    *
    *          o   Data in heap is treated as 1-dimensional.
    *
    *          o   TBLHEAPDSCs are assumed to be in a 1-dimensional ARRAY.
    */

   if (lcl_array->nStar != 0)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, lcl_nl, "Indirect data (nStar != 0) may not be set", ((char *)0)); lcl_nl = nl;
      goto rtn_return;
      }

   if (lcl_array->data.dataPtr == ((unsigned char *)0))
      {
      lcl_status = TCL_ERROR;
      sprintf (lcl_decNum, "%d", lcl_fldIdx);
      Tcl_AppendResult (interp, lcl_nl, "No data exists for field ", lcl_decNum, ((char *)0)); lcl_nl = nl;
      goto rtn_return;
      }

   if (!lcl_heap)
      {
      lcl_values = (unsigned char *) shMalloc (lcl_valuesMax * lcl_array->data.incr);

      if (lcl_array->data.schemaType != lcl_schemaTypeSTR)
         {
         if (p_shTclListValRet (interp, lcl_array->data.schemaType, lcl_prmVALUES, ((char *)0), 1, lcl_valuesMax,
             SH_ARRAY_TRAIL_IGNORE, &lcl_valuesCnt, lcl_values, SH_ARRAY_VAL_ALL, ((void *)0), 0, 0, "value", "values", "", 0, 0,
             lcl_optTRUNCval, ((int *)0)) != TCL_OK)
            {
            lcl_status = TCL_ERROR;
            goto rtn_return;
            }
         memcpy (((void *)(((unsigned char *)lcl_array->data.dataPtr) + (lcl_elemIdx * lcl_array->data.incr))),
                 ((void *)lcl_values), (lcl_valuesCnt * lcl_array->data.incr));
         shFree((void *) lcl_values);
         lcl_values = NULL;
         }
      else
         {
         lcl_strLen1stMax = lcl_array->dim[lcl_array->dimCnt-1] - lcl_indices[lcl_array->dimCnt-1];
         lcl_strLenMax    = lcl_array->dim[lcl_array->dimCnt-1];
         lcl_valuesMax    = (lcl_valuesMax + (lcl_strLenMax - 1)) / lcl_strLenMax;	/* Character count --> string count */
         if (p_shTclListValRet (interp, lcl_array->data.schemaType, lcl_prmVALUES, ((char *)0), 1, lcl_valuesMax,
            SH_ARRAY_TRAIL_IGNORE, &lcl_valuesCnt,
                 ((void *)(((unsigned char *)lcl_array->data.dataPtr) + (lcl_elemIdx * lcl_array->data.incr))),
            SH_ARRAY_VAL_ALL, ((void *)0), 0, 0, "string", "strings", "", lcl_strLen1stMax, lcl_strLenMax, lcl_optTRUNCval,
            &lcl_strLenLast) != TCL_OK)
            {
            lcl_status = TCL_ERROR;
            goto rtn_return;
            }
         /*
          * Adjust lcl_valuesCnt from a string count to a character count.
          *
          *   o   All intermediate strings are considered to have been touched
          *       completely.
          */

         lcl_valuesCnt = ((lcl_valuesCnt > 2) ? (lcl_strLenMax * (lcl_valuesCnt - 2))   : 0)
                       + ((lcl_valuesCnt > 1) ?  lcl_strLen1stMax                       : 0)
                       +                         lcl_strLenLast;
         }

      /*
       *   o   Return the range of indices covered.
       *
       *          o   The original indices are "redone," as some may not have
       *              been supplied (and thus, defaulted to zero).
       */

      if (lcl_status != TCL_OK)
         {
         goto rtn_return;
         }

      lcl_prefixElem = ""; if (lcl_array->dimCnt > 1) { Tcl_AppendResult (interp, "{",   ((char *)0)); }
      for (lcl_dimIdx = 0; lcl_dimIdx <  lcl_array->dimCnt;      lcl_dimIdx++)
         {
         sprintf (lcl_result, "%s%d", lcl_prefixElem, lcl_indices[lcl_dimIdx]);
         Tcl_AppendResult (interp, lcl_result, ((char *)0)); lcl_prefixElem = " ";
         }
      lcl_prefixElem = ""; if (lcl_array->dimCnt > 1) { Tcl_AppendResult (interp, "} {", ((char *)0)); }
                                                else  { Tcl_AppendResult (interp,  " ",  ((char *)0)); }
      lcl_elemIdx += (lcl_valuesCnt - 1);	/* Get ending array offset    */
      for (lcl_dimIdx = 0; lcl_dimIdx < (lcl_array->dimCnt - 1); lcl_dimIdx++)
         {
         sprintf (lcl_result, "%s%ld", lcl_prefixElem, (lcl_elemIdx /  lcl_array->subArrCnt[lcl_dimIdx]));
                                                       lcl_elemIdx %= lcl_array->subArrCnt[lcl_dimIdx];
         Tcl_AppendResult (interp, lcl_result, ((char *)0)); lcl_prefixElem = " ";
         }
         sprintf (lcl_result, "%s%ld", lcl_prefixElem,  lcl_elemIdx);
         Tcl_AppendResult (interp, lcl_result, (lcl_array->dimCnt > 1) ? "}" : "", ((char *)0));
      }
   else
      {
      /*
       * Handle heap.
       */

/*###*/ Tcl_AppendResult (interp, lcl_nl, "Unsupported operation: Heap values cannot be set yet", ((char *)0)); lcl_nl = nl;
      if (lcl_array->dimCnt > 1)
         {
         lcl_status = TCL_ERROR;
         Tcl_AppendResult (interp, "Getting heap data from multidimensional ARRAY not supported", ((char *)0));
         goto rtn_return;
         }
      if (lcl_tblFld == ((TBLFLD *)0))
         {
         lcl_status = TCL_ERROR;
         sprintf (lcl_decNum, "%d", lcl_fldIdx + 1);
         Tcl_AppendResult (interp, "Heap information (field", lcl_decNum, ") missing", ((char *)0));
         goto rtn_return;
         }
      if (!shInSet (lcl_tblFld->Tpres, SH_TBL_HEAP))
         {
         lcl_status = TCL_ERROR;
         sprintf (lcl_decNum, "%d", lcl_fldIdx + 1);
         Tcl_AppendResult (interp, "Heap information (field", lcl_decNum, ") missing", ((char *)0));
         goto rtn_return;
         }

#if 0
      lcl_heapDsc   = ((TBLHEAPDSC *)lcl_elemPtr);
      lcl_alignType = shAlignSchemaToAlignMap (lcl_tblFld->heap.schemaType);
#endif

      goto rtn_return;	/* ### From this point on, the value of this heap code is unknown */
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    */

rtn_return : ;

   /*									  ###
    * Restore the appropriate argv[] from the ftclFullParseArg () fix.	  ###
    */									/*### */
									/*### */
   if ((lcl_prmVALUESidx >= 0) && (lcl_prmVALUESidx < argc)) { argv[lcl_prmVALUESidx] = lcl_prmVALUES; }	/*### */

   if (lcl_values)
       shFree((void *) lcl_values);

   return (lcl_status);

   }	/* s_shTclTblFldSet */

/******************************************************************************/



static ftclArgvInfo   shTclTblCompliesWithFitsArgTable[] = {
        {"<handle>",   FTCL_ARGV_STRING,  NULL,  NULL, NULL},
        {"-binary",    FTCL_ARGV_CONSTANT,  (void*)TRUE,  NULL, NULL},
        {"-ascii",     FTCL_ARGV_CONSTANT,  (void*)TRUE,  NULL, NULL},
        {NULL,         FTCL_ARGV_END,  NULL,  NULL, NULL}
     };

        
static int	 s_shTclTblCompliesWithFits
   (
   ClientData	 clientData,		/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	*interp,		/* INOUT: Tcl interpreter structure   */
   int		 argc,			/* IN:    Tcl argument count          */
   char		*argv[]			/* IN:    Tcl arguments               */
   )

/*++
 * ROUTINE:	 s_shTclTblCompliesWithFits
 *
 * DESCRIPTION:
 *
 *	Return a true (1) or false (0) value indicating whether the Table, if
 *	written out to a FITS HDU, would be FITS compliant.
 * 
 * RETURN VALUES:
 *
 *   TCL_OK		Success.  Normal successful completion.
 *			The Table is valid.  The interp result string contains
 *			either 0 to indicate the Table is not FITS compliant or
 *			1 to indicate the Table is FITS compliant.
 *
 *   TCL_ERROR		Failure.
 *			The interp result string contains an error message.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* s_shTclTblCompliesWithFits */

   /*
    * Declarations.
    */

                  int	  lcl_status;
                  int     lcl_flags = FTCL_ARGV_NO_LEFTOVERS;
                  char	 *lcl_handleExpr = ((char  *)0);
   HANDLE		  lcl_handle;
                  void	 *lcl_handlePtr;
                  int	  lcl_optASCII  = FALSE;	/* Option present: -ascii     */
                  int	  lcl_optBINARY = FALSE;	/* Option present: -binary    */
   FITSHDUTYPE		  lcl_FITStype;
   FITSFILETYPE		  lcl_FITSstd;		/* FITS compliant?            */

                  char	 *lcl_nl = "";
   static         char	 *    nl = "\n";

                  
   shTclTblCompliesWithFitsArgTable[0].dst = &lcl_handleExpr;
   shTclTblCompliesWithFitsArgTable[1].dst = &lcl_optBINARY;
   shTclTblCompliesWithFitsArgTable[2].dst = &lcl_optASCII;
   
   /***************************************************************************/
   /*
    * Parse the command line.
    */

   Tcl_SetResult (interp, "", TCL_VOLATILE);	/* Set TCL_VOLATILE           */

   if (ftcl_ParseArgv (interp, &argc, argv,shTclTblCompliesWithFitsArgTable, lcl_flags)
              != FTCL_ARGV_SUCCESS)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, "Parsing error\n",           ((char *)0));
      Tcl_AppendResult (interp, shTclTblCompliesWithFitsUse, ((char *)0));
      goto rtn_return;
      }

   /*
    * Perform some argument checking.
    *
    *   o   Try to find as many conflicts as possible, so keep cascading down
    *       the  tests, even if a conflict is found.
    */

   if ((lcl_status = shTclHandleExprEval (interp, lcl_handleExpr, &lcl_handle, &lcl_handlePtr)) != TCL_OK)
      {
      lcl_status = TCL_ERROR;	/* shTclHandleExprEval sets interp result     */
      lcl_nl     = nl;
      }

   if ( lcl_optASCII &&  lcl_optBINARY)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, lcl_nl, "Conflicting options: -ascii and -binary", ((char *)0)); lcl_nl = nl;
      }
   if (!lcl_optASCII && !lcl_optBINARY)
      {
      lcl_status = TCL_ERROR;
      Tcl_AppendResult (interp, "TBLCOL object schemas must have -ascii or -binary specified", ((char *)0)); lcl_nl = nl;
      goto rtn_return;
      }

   if (lcl_status != TCL_OK)
      {
      goto rtn_return;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * The handling of particular object schemas is done by other Tcl commands.
    */

   if (lcl_handle.type != shTypeGetFromName ("TBLCOL"))
      {
      Tcl_AppendResult (interp, lcl_nl, shNameGetFromType (lcl_handle.type), " object schemas are not handled by ",
                                shTclTblCompliesWithFitsName, ((char *)0)); lcl_nl = nl;
      goto rtn_return;
      }

   lcl_FITStype = (lcl_optASCII)  ? f_hduTABLE
                : (lcl_optBINARY) ? f_hduBINTABLE
                                  : f_hduUnknown;

   if (shFitsTBLCOLcomply (((TBLCOL *)lcl_handle.ptr), lcl_FITStype, &lcl_FITSstd) == SH_SUCCESS)
      {
      Tcl_SetResult    (interp, (lcl_FITSstd == STANDARD) ? "1" : "0", TCL_VOLATILE);       lcl_nl = nl;
      }
   else
      {
      Tcl_AppendResult (interp, lcl_nl, "Table results in invalid FITS file", ((char *)0)); lcl_nl = nl;
      }

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * All done.
    */

rtn_return : ;

   return (lcl_status);

   }	/* s_shTclTblCompliesWithFits */

/******************************************************************************/

void		 p_shTclTblDeclare
   (
   Tcl_Interp	*interp			/* IN:    Tcl Interpreter structure   */
   )

/*++
 * ROUTINE:	 p_shTclTblDeclare
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

   {	/* p_shTclTblDeclare */

   /*
    * Declarations.
    */

#define	shTclTblCLI(symbol, rtn)\
   (void )shTclDeclare (interp, shTclTbl##symbol##Name, ((Tcl_CmdProc *)(rtn)), ((ClientData)0), ((Tcl_CmdDeleteProc *)NULL), "shTbl",\
                         shTclTbl##symbol##Help, shTclTbl##symbol##Use)

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   shTclTblCLI (InfoGet,          s_shTclTblInfoGet);
   shTclTblCLI (FldAdd,           s_shTclTblFldAdd);
   shTclTblCLI (FldInfoGet,       s_shTclTblFldInfoGet);
   shTclTblCLI (FldInfoClr,       s_shTclTblFldInfoClr);
   shTclTblCLI (FldInfoSet,       s_shTclTblFldInfoSet);
   shTclTblCLI (FldGet,           s_shTclTblFldGet);
   shTclTblCLI (FldSet,           s_shTclTblFldSet);
   shTclTblCLI (CompliesWithFits, s_shTclTblCompliesWithFits);

#undef	shTclTblCLI

   }	/* p_shTclTblDeclare */

/******************************************************************************/
