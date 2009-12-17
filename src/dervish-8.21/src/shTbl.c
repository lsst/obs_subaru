/*++
 * FACILITY:	Dervish
 *
 * ABSTRACT:	Routines to manipulate Tables.
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
 *     shTblColCreate		Create a skeleton TBLCOL Table.
 *     shTblFldAdd		Add a field to a TBLCOL Table (to end of fields)
 *     shTblFldLoc		Locate a field based on position or name.
 *     shTblFldHeapAlloc	Allocate space for  heap storage.
 *     shTblFldHeapFree		Free     space from heap storage.
 *     shTblTBLFLDlink		Allocate and link a TBLFLD to a field (ARRAY).
 *     shTblTBLFLDclr		Clear TBLFLD member (make it not present).
 *     shTblTBLFLDsetWithAscii	Set a TBLFLD member with an ASCII value.
 *     shTblTBLFLDsetWithDouble	Set a TBLFLD member with a double value.
 *     shTblTBLFLDsetWithInt	Set a TBLFLD member with a signed integer value.
 *
 *   Private
 *   --------------------------	------------------------------------------------
 *   p_shTblFldgen		Read a Table header and fill an ARRAY list.
 *   p_shTblFldDelByIdx		Delete a field from a TBLCOL (ARRAY list).
 *   p_shTblFldParseTFORMasc	Parse TFORM keyword in FITS ASCII  Table header.
 *   p_shTblFldParseTFORMbin	Parse TFORM keyword in FITS Binary Table header.
 *   p_shTblFldParseTDIM	Parse TDIM  keyword in FITS        Table header.
 *   p_shTblFldHeapCnt		Count number of heap elements in a field.
 *   p_shTblFldSignFlip		Return info about flipping "signedness" of data.
 *     shTblcolNew		Object schema constructor for TBLCOL.
 *     shTblcolDel		Object schema destructor  for TBLCOL.
 *     shTblfldNew		Object schema constructor for TBLFLD.
 *     shTblfldDel		Object schema destructor  for TBLFLD.
 *
 *   s_shTblTBLFLDreadAkey	Read a character string value from a keyword.
 *   s_shTblTBLFLDreadIkey	Read an integer         value from a keyword.
 *   s_shTblTBLFLDreadDkey	Read a  double real     value from a keyword.
 *   s_shTblTBLFLDreadLkey	Read a  logical         value from a keyword.
 *   s_shTblFldParseTFORMbinDataType Convert TFORMn data type to object schema.
 *
 * ENVIRONMENT:	ANSI C.
 *		shTbl.c
 *
 * AUTHOR: 	Tom Nicinski, Creation date: 18-Nov-1993
 *--
 ******************************************************************************/

/******************************************************************************/
/*						.----------------------------.
 * Outstanding Issues / Problems / Etc.		|  Last Update:  1-Dec-1993  |
 *						`----------------------------'
 *
 * This section lists out all the current problems with Table processing and any
 * suggested fixes and enhancements.  Please keep this section   C U R R E N T.
 *
 *- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 ******************************************************************************/

/******************************************************************************/
/*
 * Global declarations.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#include	"libfits.h"

#include	"shCSchema.h"
#include	"shChain.h"
#include	"shCArray.h"
#include	"shCUtils.h"
#include        "shCGarbage.h"
#include	"shCAlign.h"
#include	"shCErrStack.h"
#include	"shCAssert.h"
#include	"shCHdr.h"

#define		 SHTBLIMP		/* Need global initialized structures */
#include	"shCTbl.h"
#undef		 SHTBLIMP

/******************************************************************************/
/*
 * Very private routines.
 */

static RET_CODE	 s_shTblTBLFLDreadAkey	(HDR_VEC *hdrVec, TBLFLD *tblFld, int fldIdx, TBLFLD_KEYWORDS, char *keyName,
					 char   *keyVal, int dataTypeOk);
static RET_CODE	 s_shTblTBLFLDreadIkey	(HDR_VEC *hdrVec, TBLFLD *tblFld, int fldIdx, TBLFLD_KEYWORDS, char *keyName,
					 int    *keyVal, int dataTypeOk);
static RET_CODE	 s_shTblTBLFLDreadDkey	(HDR_VEC *hdrVec, TBLFLD *tblFld, int fldIdx, TBLFLD_KEYWORDS, char *keyName,
					 double *keyVal, int dataTypeOk);
#if 0					/* not called */
static RET_CODE	 s_shTblTBLFLDreadLkey	(HDR_VEC *hdrVec, TBLFLD *tblFld, int fldIdx, TBLFLD_KEYWORDS, char *keyName,
					 int    *keyVal, int dataTypeOk);
#endif
static RET_CODE	 s_shTblFldParseTFORMbinDataType	(char dataType, struct shTbl_SuppData *dataTypeMap, int *dataTypeIdx,
							 ARRAY_DATA *dataInfo);

/*
 * The value is set in shTblcolNew() as I (RHL) assume that no-one can want
 * to use tblcol code until after creating one. See notes at top of
 * shTblcolNew().
 */
static TYPE g_tblfld_type = UNKNOWN;	/* == shTypeGetFromName("TBLFLD") */

/******************************************************************************/

RET_CODE	   shTblColCreate
   (
                  int	  rowCnt,	/* IN:    Number of rows   in Table   */
                  int	  fldCnt,	/* IN:    Number of fields in Table   */
                  int	  tblFldPres,	/* IN:    Boolean: TBLFLD too?        */
   TBLCOL		**tblCol	/* OUT:   Pointer to Table descriptor */
   )

/*++
 * ROUTINE:	   shTblColCreate
 *
 *	Create a skeleton TBLCOL Table.  The ARRAY structures (and optional
 *	TBLFLD structures) are allocated and linked together properly.  Some
 *	initial filling of information is also performed:
 *
 *	   o	tblCol->rowCnt = rowCnt;	Number of rows
 *	   o	array->dimCnt  = 1;		First dimension: number of rows
 *	   o	array->dim[0]  = rowCnt;	First dimension: number of rows
 *	   o	array->nStar   = 0;		No data indirection.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_INVARG		Failure.
 *			The row count or field count was less than zero.
 *
 *   SH_MALLOC_ERR	Failure.
 *			Space for the TBLCOL or its associated structures could
 *			not be allocated.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	Because the first dimension in an ARRAY is the number of rows, this
 *	information is filled in each ARRAY.  When users are adding additional
 *	dimensions, they should be added with:
 *
 *	   array->dim[array->dimCnt++] = nnn;
 *--
 ******************************************************************************/

   {	/*   shTblColCreate */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
   TBLCOL		*lcl_tblCol = ((TBLCOL *)0);
                  int	 lcl_tblFldIdx;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Check arguments.
    */

   if (rowCnt < 0)
      {
      lcl_status = SH_INVARG;
      shErrStackPush ("Invalid row count: must not be negative");
      goto rtn_return;
      }

   if (fldCnt < 0)
      {
      lcl_status = SH_INVARG;
      shErrStackPush ("Invalid field count: must be greater than 0");
      goto rtn_return;
      }

   if (tblCol == ((TBLCOL **)0))
      {
      lcl_status = SH_INVARG;
      shErrStackPush ("tblCol argument not passed");
      goto rtn_return;
      }

   /*
    * Allocate space for everything and link it up.
    */

   if ((lcl_tblCol = shTblcolNew ()) == ((TBLCOL *)0))
      {
      lcl_status = SH_MALLOC_ERR;
      shErrStackPush ("Unable to allocate space for Table TBLCOL descriptor");
      goto rtn_return;
      }

   lcl_tblCol->rowCnt = rowCnt;

   for (lcl_tblFldIdx = 0; lcl_tblFldIdx < fldCnt; lcl_tblFldIdx++)
      {
      if ((lcl_status = shTblFldAdd (lcl_tblCol, tblFldPres, ((ARRAY **)0), ((TBLFLD **)0), ((int *)0))) != SH_SUCCESS)
         {
         goto rtn_return;
         }
      }

   /*
    * All done.
    */

rtn_return : ;

   if ((lcl_status != SH_SUCCESS) && (lcl_tblCol != ((TBLCOL *)0)))
      {
      shTblcolDel(lcl_tblCol);
      }
   else
      {
      *tblCol = lcl_tblCol;
      }

   return (lcl_status);

   }	/*   shTblColCreate */

/******************************************************************************/

RET_CODE	   shTblFldLoc
   (
   TBLCOL		 *tblCol,	/* IN:    Table descriptor            */
                  int	  tblFldWanted,	/* IN:    Field index (0-indexed)     */
                  char	 *tblFldName,	/* IN:    Field name                  */
                  int	  tblFldNamePos,/* IN:    Field name index (0-indexed)*/
   SHCASESENSITIVITY	  tblFldNameCase, /* IN:  Case sensitive 4 tblFldName?*/
   ARRAY		**array,	/* OUT:   ARRAY  field descriptor     */
   TBLFLD		**tblFld,	/* OUT:   TBLFLD field descriptor     */
                  int	 *tblFldIdx	/* OUT:   Field index (0-indexed)     */
   )

/*++
 * ROUTINE:	   shTblFldLoc
 *
 *	Locate a field within a Table.  Either the field position (0-indexed) or
 *	the field name (corresponding to the TTYPEn FITS keyword) can be speci-
 *	fied:
 *
 *	   tblFldWanted	<  0 indicates that tblFldName should be used.  The
 *	                     tblFldNamePos'th match is returned (0-indexed).
 *	                     Case sensitivity is controlled by tblFldNameCase.
 *
 *	   tblFldWanted >= 0 indicates the field should be located by position.
 *	                     In this case, tblFldName must be zero (0).
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *			The field was located.
 *
 *   SH_MISARG		Failure.
 *			Neither the field position, tblFldWanted, or field name,
 *			tblFldName, were passed.
 *
 *   SH_CONFLICT_ARG	Failure.
 *			Both tblFldWanted and tblFldName were passed.  No field
 *			was searched for.
 *
 *   SH_NOEXIST		Failure.
 *			The specified field does not exist.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/*   shTblFldLoc */

   /*
    * Declarations.
    */

   RET_CODE		  lcl_status = SH_SUCCESS;
   ARRAY		 *lcl_array;
   TBLFLD		 *lcl_tblFld = ((TBLFLD *)0);	/* Init to satisfy GCC*/
                  int	  lcl_tblFldIdx;		/* (erroneous) warning*/
                  int	  lcl_tblFldNamePos;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   if ((tblFldWanted <  0) && (tblFldName == ((char *)0)))
      {
      lcl_status = SH_MISARG;
      shErrStackPush ("Missing option: either field position or field name must be passed");
      goto rtn_return;
      }

   if ((tblFldWanted >= 0) && (tblFldName != ((char *)0)))
      {
      lcl_status = SH_CONFLICT_ARG;
      shErrStackPush ("Conflicting options: both field position and field name were passed");
      goto rtn_return;
      }

   if ( tblFldWanted >= 0)
      {
      /*
       * Search by position.
       */

      for (lcl_tblFldIdx = 0; lcl_tblFldIdx < shChainSize(&(tblCol->fld)); lcl_tblFldIdx++) {
         if (lcl_tblFldIdx == tblFldWanted) {
            lcl_array = shChainElementGetByPos(&(tblCol->fld), lcl_tblFldIdx);
            break;
         }
      }

      if (lcl_tblFldIdx >= shChainSize(&(tblCol->fld)))
         {
         lcl_status = SH_NOEXIST;
         shErrStackPush ("Field %d does not exist", tblFldWanted);
         goto rtn_return;
         }
      lcl_tblFld = ((TBLFLD *)((lcl_array->infoType == g_tblfld_type) ? lcl_array->info : 0));
      }
   else
      {
      /*
       * Search by name.
       */

      lcl_tblFldNamePos = (tblFldNamePos = (tblFldNamePos < 0) ? 0 : tblFldNamePos);

      for (lcl_tblFldIdx = 0; lcl_tblFldIdx < shChainSize(&(tblCol->fld)); lcl_tblFldIdx++)
         {
             lcl_array = shChainElementGetByPos(&(tblCol->fld), lcl_tblFldIdx);

             lcl_tblFld  = ((TBLFLD *)((lcl_array->infoType == g_tblfld_type) ? lcl_array->info : 0));
         if (lcl_tblFld != ((TBLFLD *)0))
            {
            if (shInSet (lcl_tblFld->Tpres, SH_TBL_TTYPE))
               {
               if (shStrcmp (tblFldName, lcl_tblFld->TTYPE, tblFldNameCase) == 0)
                  {
                  if (--lcl_tblFldNamePos < 0) { break; }	/* Got it     */
                  }
               }
            }
         }
      if (lcl_tblFldIdx >= shChainSize(&(tblCol->fld)))
         {
         lcl_status = SH_NOEXIST;
         shErrStackPush ("Field %s (%d%s occurrence) does not exist", tblFldName, tblFldNamePos + 1, shNth (tblFldNamePos + 1));
         goto rtn_return;
         }
      }

   /*
    * All done.
    */

   if (lcl_status == SH_SUCCESS)
      {
      if (array     != ((ARRAY  **)0)) { *array     = lcl_array;     }
      if (tblFld    != ((TBLFLD **)0)) { *tblFld    = lcl_tblFld;    }
      if (tblFldIdx != ((int     *)0)) { *tblFldIdx = lcl_tblFldIdx; }
      }

rtn_return : ;

   return (lcl_status);

   }	/*   shTblFldLoc */

/******************************************************************************/

RET_CODE	   shTblFldHeapAlloc
   (
   ARRAY		 *array,	/* INOUT: ARRAY to allocate heap for  */
                  char	 *schemaName,	/* IN:    Object schema type name     */
            long  int	  extraCnt,	/* IN:    Additional objects to alloc */
   unsigned       char	**extra		/* OUT:   Where extra space starts    */
   )

/*++
 * ROUTINE:	   shTblFldHeapAlloc
 *
 *	Allocate storage space for TBLFLD owned heap.  Heap space is allocated
 *	only for heap descriptors (TBLHEAPDSC) with null (zero) addresses. The
 *	heap descriptor counts area used to determine the space requirements
 *	(thus, they must be filled in prior to this call).  The heap descriptors
 *	are then linked to their portion of the heap area.  Additional space can
 *	also be allocated (through extraCnt).
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_NOT_SUPPORTED	Failure.
 *			The ARRAY is not 1-dimensional or the data is indirect
 *			(nStar is non-zero).
 *
 *   SH_INVOBJ		Failure.
 *			The ARRAY info member does not point to a TBLFLD and/
 *			or the ARRAY does not contain heap descriptors (TBLHEAP-
 *			DSC object schema).
 *
 *   SH_ALREXIST	Failure.
 *			Heap space is already allocated for the TBLFLD.
 *
 *   SH_MALLOC_ERR	Failure.
 *			Sufficient space for heap storage could not be allo-
 *			cated.
 *
 * SIDE EFFECTS:
 *
 *   o	If the ARRAY has no optional information associated with it (info
 *	member is a null address), then a TBLFLD is allocated and associated
 *	with the ARRAY.
 *
 *   o	An UNKNOWN object schema is treated as unsigned char.
 *
 * SYSTEM CONCERNS:
 *
 *   o	The heap descriptors must be initialized prior to this call:
 *
 *	   o	non-zero addresses in heap descriptors cause them to be
 *		skipped for space consideration.
 *
 *	   o	zero addresses cause the heap descriptor cound to be used
 *		to determine space requirements.
 *
 *   o	The ARRAY is considered to be 1-dimensional with no data indirection
 *	(nStar must be zero (0)).
 *
 *   o	The heap space allocated by this routine will be considered to be
 *	owned by the TBLFLD associated with the ARRAY.
 *--
 ******************************************************************************/

   {	/*   shTblFldHeapAlloc */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
   SHALIGN_TYPE		 lcl_alignType;
   TYPE			 lcl_schemaType;
   SCHEMA		*lcl_schema;
   TBLFLD		*lcl_tblFld;
                  int	 lcl_rowIdx;
   TBLHEAPDSC		*lcl_heapDsc;
            long  int	 lcl_heapSize;
   unsigned char	*lcl_heapPtr = NULL;
            long  int	 lcl_heapIncr;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Check legailities.
    */

   if (extraCnt < 0) { extraCnt = 0; }			/* Fix up a bit       */

   if ( array->data.schemaType != shTypeGetFromName ("TBLHEAPDSC"))
      {
      lcl_status = SH_INVOBJ;
      shErrStackPush ("ARRAY data type (%s) does not support Table heap storage", shNameGetFromType (array->data.schemaType));
      }
   if ((array->infoType        != g_tblfld_type) && (array->info != ((unsigned char *)0)))
      {
      lcl_status = SH_INVOBJ;
      shErrStackPush ("ARRAY optional information (%s) is not a TBLFLD object schema", shNameGetFromType (array->infoType));
      }
   lcl_schemaType  = shTypeGetFromName      (schemaName);
   if ((lcl_schema = ((SCHEMA *)shSchemaGet (schemaName))) == ((SCHEMA *)0))
      {
      if (strcmp (schemaName, "UNKNOWN") != 0)	/* Explicit UNKNOWN allowed   */
         {
         lcl_status = SH_NOT_SUPPORTED;
         shErrStackPush ("Heap object schema %s is not supported", schemaName);
         goto rtn_return;
         }
      }
   if ( array->nStar           != 0)
      {
      lcl_status = SH_NOT_SUPPORTED;
      shErrStackPush ("Indirect heap descriptors not supported");
      }
   if ( array->dimCnt          != 1)
      {
      lcl_status = SH_NOT_SUPPORTED;
      shErrStackPush ("ARRAY of heap descriptors must be 1-dimensional");
      }

   if (lcl_status != SH_SUCCESS)
      {
      goto rtn_return;
      }

   if ((lcl_tblFld = ((TBLFLD *)array->info)) != ((TBLFLD *)0))
      {
      if (lcl_tblFld->heap.dataPtr != ((unsigned char *)0))
         {
         lcl_status = SH_ALREXIST;
         shErrStackPush ("ARRAY already has heap area allocated");
         goto rtn_return;
         }
      }
   else
      {
      if ((lcl_status = shTblTBLFLDlink (array, &lcl_tblFld)) != SH_SUCCESS)
         {
         shErrStackPush ("Unable to associate TBLFLD with Table's ARRAY");
         goto rtn_return;
         }
      }

   /*
    * Count up the space needed.
    */

   if ((lcl_alignType = shAlignSchemaToAlignMap (lcl_schemaType)) == SHALIGNTYPE_unknown)
      {
      if (lcl_schemaType == shTypeGetFromName ("STR"))
         {
         lcl_alignType = SHALIGNTYPE_S8;	/* Fudge string to character  */
         }
      else
         {
         lcl_alignType = SHALIGNTYPE_U8;	/* Pick something             */
         }
      }
   lcl_heapIncr = (lcl_alignType != SHALIGNTYPE_unknown) ? shAlignment.type[lcl_alignType].incr : lcl_schema->size;

   for (lcl_heapSize = 0, lcl_rowIdx = array->dim[0] - 1; lcl_rowIdx >= 0; lcl_rowIdx --)
      {
      lcl_heapDsc = &((TBLHEAPDSC *)array->data.dataPtr)[lcl_rowIdx];
      if ((lcl_heapDsc->ptr == ((unsigned char *)0)) && (lcl_heapDsc->cnt > 0))
         {
         lcl_heapSize += lcl_heapDsc->cnt;
         }
      }

   lcl_heapSize += extraCnt;		/* More heap space requested?         */
   lcl_heapSize *= lcl_heapIncr;	/* Account for size and alignment     */

   /*
    * Allocate the space and link it up.
    *
    *   o   It's assumed that shMalloc () will allocate space such that the
    *       first byte is on a boundary that will satisfy any alignment needs.
    *
    *   o   Heap data space is zeroed out (this is a performance hit).
    */

   if (lcl_heapSize > 0)
      {
      if ((lcl_heapPtr = lcl_tblFld->heap.dataPtr = ((unsigned char *)shMalloc (lcl_heapSize))) == ((unsigned char *)0))
         {
         lcl_status = SH_MALLOC_ERR;
         shErrStackPush ("Unable to allocate space for heap storage");
         goto rtn_return;
         }

      memset (((void *)lcl_heapPtr), 0, lcl_heapSize);
      }

   lcl_tblFld->heap.schemaType = lcl_schemaType;
   if (lcl_alignType != SHALIGNTYPE_unknown)
      {
      lcl_tblFld->heap.size  = shAlignment.type[lcl_alignType].size;
      lcl_tblFld->heap.incr  = shAlignment.type[lcl_alignType].incr;
      lcl_tblFld->heap.align = shAlignment.type[lcl_alignType].align;
      }
   else
      {
      lcl_tblFld->heap.size  =
      lcl_tblFld->heap.incr  = lcl_schema->size;
      lcl_tblFld->heap.align = shAlignment.alignMax; /* Get largest alignment */
      }

   for (lcl_rowIdx = 0; lcl_rowIdx < array->dim[0]; lcl_rowIdx++)
      {
      lcl_heapDsc = &((TBLHEAPDSC *)array->data.dataPtr)[lcl_rowIdx];
      if ((lcl_heapDsc->ptr == ((unsigned char *)0)) && (lcl_heapDsc->cnt > 0))
         {
           lcl_heapDsc->ptr  = lcl_heapPtr;
                               lcl_heapPtr += (lcl_heapDsc->cnt * lcl_heapIncr); /* WP 06/30/94 */
         }
      }

   /*
    * All done.
    */

rtn_return : ;

   if (lcl_status == SH_SUCCESS)
      {
      shAddSet (lcl_tblFld->Tpres, SH_TBL_HEAP);
      if (extra != ((unsigned char **)0)) { *extra = (extraCnt > 0) ? lcl_heapPtr : ((unsigned char *)0); }
      }
   else
      {
      if (extra != ((unsigned char **)0)) { *extra =                                ((unsigned char *)0); }
      }

   return (lcl_status);

   }	/*   shTblFldHeapAlloc */

/******************************************************************************/

RET_CODE	   shTblFldHeapFree
   (
   ARRAY		 *array		/* INOUT: ARRAY to free heap from     */
   )

/*++
 * ROUTINE:	   shTblFldHeapFree
 *
 *	Free up any heap space owned by the ARRAY's TBLFLD.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_INVOBJ		Failure.
 *			The ARRAY does not have an associated TBLFLD.
 *
 * SIDE EFFECTS:
 *
 *   o	The TBLFLD heap information is NOT invalidated, as the ARRAY heap
 *	descriptors may still point to heap data outside the area owned by
 *	the TBLFLD.
 *
 *   o	Heap descriptors are   N O T   updated to reflect the freeing of the
 *	heap area owned by the TBLFLD.  This may leave dangling pointers.
 *
 * SYSTEM CONCERNS:
 *--
 ******************************************************************************/

   {	/*   shTblFldHeapFree */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   if ((array->info != ((void *)0)) && (array->infoType != g_tblfld_type))
      {
      lcl_status = SH_INVOBJ;
      goto rtn_return;
      }

   shFree (((void *)((TBLFLD *)array->info)->heap.dataPtr));
                    ((TBLFLD *)array->info)->heap.dataPtr = ((unsigned char *)0);

rtn_return : ;

   return (lcl_status);

   }	/*   shTblFldHeapFree */

/******************************************************************************/

RET_CODE	   shTblTBLFLDlink
   (
   ARRAY		 *array,	/* INOUT: ARRAY to link TBLFLD to     */
   TBLFLD		**tblFld	/* OUT:   Address of ARRAY's TBLFLD   */
   )

/*++
 * ROUTINE:	   shTblTBLFLDlink
 *
 *	Allocate and link a TBLFLD to the ARRAY, if the ARRAY already does not
 *	have one.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_ALREXIST	Failure.
 *			The ARRAY has a different informative structure (not a
 *			TBLFLD) hung off of itself.
 *
 *   SH_MALLOC_ERR	Failure.
 *			Space for the TBLFLD could not be allocated.
 *
 *   SH_INVOBJ		Failure.
 *			The ARRAY object is not valid.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/*   shTblTBLFLDlink */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
   TBLFLD		*lcl_tblFld = ((TBLFLD *)0);

   TYPE			 lcl_schemaTypeTBLFLD = g_tblfld_type;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   if (array == ((ARRAY *)0))
      {
      lcl_status = SH_INVOBJ;
      goto rtn_return;
      }

   if ((lcl_tblFld = ((TBLFLD *)array->info)) != ((TBLFLD *)0))
      {
      if (array->infoType != lcl_schemaTypeTBLFLD)
         {
         lcl_tblFld = ((TBLFLD *)0);		/* Don't return anything      */
         lcl_status = SH_ALREXIST;
         goto rtn_return;
         }
      }
   else
      {
      if ((lcl_tblFld = shTblfldNew ()) == ((TBLFLD *)0))
         {
         lcl_status = SH_MALLOC_ERR;
         shErrStackPush ("Unable to allocate space for TBLFLD structure");
         goto rtn_return;
         }
      array->info       = ((void *)lcl_tblFld);
      array->infoType   = lcl_schemaTypeTBLFLD;
      lcl_tblFld->array = array;
      }

   /*
    * All done.
    */

rtn_return : ;

   if (tblFld != ((TBLFLD **)0)) { *tblFld = lcl_tblFld; }

   return (lcl_status);

   }	/*   shTblTBLFLDlink */

/******************************************************************************/

RET_CODE	   shTblTBLFLDclr
   (
   ARRAY		 *array,	/* INOUT: ARRAY (field) to clear      */
   TBLFLD_KEYWORDS	  tblKeyFld,	/* IN:    TBLFLD member to clear      */
   TBLFLD		**tblFld	/* OUT:   Address of ARRAY's TBLFLD   */
   )

/*++
 * ROUTINE:	   shTblTBLFLDclr
 *
 *	Clear the specified TBLFLD member, making it not present.  Any previous
 *	value is destroyed.  If the ARRAY (field) does not have a TBLFLD asso-
 *	ciated with it, the TBLFLD is first allocated.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_INVARG		Failure.
 *			The TBLFLD member indicator is invalid (does not exist
 *			or its value cannot be cleared).
 *
 *   SH_ALREXIST	Failure.
 *			The ARRAY has a different informative structure (not a
 *			TBLFLD) hung off of itself.
 *
 *   SH_MALLOC_ERR	Failure.
 *			Space for the TBLFLD could not be allocated.
 *
 *   SH_INVOBJ		Failure.
 *			The ARRAY object is not valid.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/*   shTblTBLFLDclr */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
   TBLFLD		*lcl_tblFld = ((TBLFLD *)0);

#define	lcl_caseAscii(keyword)\
   case SH_TBL_##keyword :           lcl_tblFld->keyword[0] = '\000';\
                         shClrSet (lcl_tblFld->Tpres, SH_TBL_##keyword)

#define	lcl_caseDouble(keyword)\
   case SH_TBL_##keyword :           lcl_tblFld->keyword = ((double)0.0);\
                         shClrSet (lcl_tblFld->Tpres, SH_TBL_##keyword)

#define	lcl_caseInt(keyword)\
   case SH_TBL_##keyword :           lcl_tblFld->keyword = 0;\
                         shClrSet (lcl_tblFld->Tpres, SH_TBL_##keyword)

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * The checking of legal members is done prior to possibly allocating TBLFLD
    * space.
    */

   switch (tblKeyFld)
      {
      case SH_TBL_TTYPE    :
      case SH_TBL_TUNIT    :
      case SH_TBL_TNULLSTR :
      case SH_TBL_TDISP    :
      case SH_TBL_TSCAL    :
      case SH_TBL_TZERO    :
      case SH_TBL_TNULLINT :
                           break;

      default       : lcl_status = SH_INVARG;
                      shErrStackPush ("Invalid or unknown member of TBLFLD");
                      goto rtn_return;
                      break;
      }

   if ((lcl_status = shTblTBLFLDlink (array, &lcl_tblFld)) != SH_SUCCESS)
      {
      goto rtn_return;
      }

   switch (tblKeyFld)
      {
      lcl_caseAscii  (TTYPE);    break;
      lcl_caseAscii  (TUNIT);    break;
      lcl_caseAscii  (TNULLSTR); break;
      lcl_caseAscii  (TDISP);    break;
      lcl_caseDouble (TSCAL);    break;
      lcl_caseDouble (TZERO);    break;
      lcl_caseInt    (TNULLINT); break;

      default : lcl_status = SH_INTERNAL_ERR;
                shErrStackPush ("Internal error (%s line %d): members accepted above not reflected here", __FILE__, __LINE__);
                shFatal        ("Internal error (%s line %d): members accepted above not reflected here", __FILE__, __LINE__ - 1);
                goto rtn_return;
                break;
      }

   /*
    * All done.
    */

   if (tblFld != ((TBLFLD **)0)) { *tblFld = lcl_tblFld; }

rtn_return : ;

   return (lcl_status);

#undef	lcl_caseAscii
#undef	lcl_caseDouble
#undef	lcl_caseInt

   }	/*   shTblTBLFLDclr */

/******************************************************************************/

RET_CODE	   shTblTBLFLDsetWithAscii
   (
   ARRAY		 *array,	/* INOUT: ARRAY (field) to set value  */
   TBLFLD_KEYWORDS	  tblKeyFld,	/* IN:    TBLFLD member to set        */
                  char	 *tblKeyVal,	/* IN:    TBLFLD member value         */
   TBLFLD		**tblFld	/* OUT:   Address of ARRAY's TBLFLD   */
   )

/*++
 * ROUTINE:	   shTblTBLFLDsetWithAscii
 *
 *	Set the specified TBLFLD member with an ASCII value.  Any previous value
 *	is destroyed.  If the ARRAY (field) does not have a TBLFLD associated
 *	with it, the TBLFLD is first allocated.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_TRUNC		Success.
 *			The data was too long and therefore truncated.
 *
 *   SH_INVARG		Failure.
 *			The TBLFLD member indicator is invalid, (does not exist
 *			or member is does not accept an ASCII value).
 *
 *   SH_ALREXIST	Failure.
 *			The ARRAY has a different informative structure (not a
 *			TBLFLD) hung off of itself.
 *
 *   SH_MALLOC_ERR	Failure.
 *			Space for the TBLFLD could not be allocated.
 *
 *   SH_INVOBJ		Failure.
 *			The ARRAY object is not valid.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/*   shTblTBLFLDsetWithAscii */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
   TBLFLD		*lcl_tblFld = ((TBLFLD *)0);

#define	lcl_case(keyword)\
   case SH_TBL_##keyword : strncpy  (lcl_tblFld->keyword, tblKeyVal, (shFitsHdrStrSize));\
                                   lcl_tblFld->keyword[shFitsHdrStrSize] = '\000';\
                         shAddSet (lcl_tblFld->Tpres, SH_TBL_##keyword)

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * The checking of legal members is done prior to possibly allocating TBLFLD
    * space.
    */

   switch (tblKeyFld)
      {
      case SH_TBL_TTYPE    :
      case SH_TBL_TUNIT    :
      case SH_TBL_TNULLSTR :
      case SH_TBL_TDISP    :
                           break;

      case SH_TBL_TSCAL    :
      case SH_TBL_TZERO    :
      case SH_TBL_TNULLINT :
                           lcl_status = SH_INVARG;
                           shErrStackPush ("TBLFLD member does not accept ASCII value");
                           goto rtn_return;
                           break;

      default            : lcl_status = SH_INVARG;
                           shErrStackPush ("Invalid or unknown member of TBLFLD");
                           goto rtn_return;
                           break;
      }

   if ((lcl_status = shTblTBLFLDlink (array, &lcl_tblFld)) != SH_SUCCESS)
      {
      goto rtn_return;
      }

   if (shStrnlen (tblKeyVal, (shFitsHdrStrSize)) > (shFitsHdrStrSize))
      {
      lcl_status = SH_TRUNC;			/* Warn the caller            */
      }

   switch (tblKeyFld)
      {
      lcl_case (TTYPE);    break;
      lcl_case (TUNIT);    break;
      lcl_case (TNULLSTR); break;
      lcl_case (TDISP);    break;

      default : lcl_status = SH_INTERNAL_ERR;
                shErrStackPush ("Internal error (%s line %d): members accepted above not reflected here", __FILE__, __LINE__);
                shFatal        ("Internal error (%s line %d): members accepted above not reflected here", __FILE__, __LINE__ - 1);
                goto rtn_return;
                break;
      }

   /*
    * All done.
    */

   if (tblFld != ((TBLFLD **)0)) { *tblFld = lcl_tblFld; }

rtn_return : ;

   return (lcl_status);

#undef	lcl_case

   }	/*   shTblTBLFLDsetWithAscii */

/******************************************************************************/

RET_CODE	   shTblTBLFLDsetWithDouble
   (
   ARRAY		 *array,	/* INOUT: ARRAY (field) to set value  */
   TBLFLD_KEYWORDS	  tblKeyFld,	/* IN:    TBLFLD member to set        */
   double		  tblKeyVal,	/* IN:    TBLFLD member value         */
   TBLFLD		**tblFld	/* OUT:   Address of ARRAY's TBLFLD   */
   )

/*++
 * ROUTINE:	   shTblTBLFLDsetWithDouble
 *
 *	Set the specified TBLFLD member with an double value. Any previous value
 *	is destroyed.  If the ARRAY (field) does not have a TBLFLD associated
 *	with it, the TBLFLD is first allocated.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_INVARG		Failure.
 *			The TBLFLD member indicator is invalid, (does not exist
 *			or member is does not accept a double value).
 *
 *   SH_ALREXIST	Failure.
 *			The ARRAY has a different informative structure (not a
 *			TBLFLD) hung off of itself.
 *
 *   SH_MALLOC_ERR	Failure.
 *			Space for the TBLFLD could not be allocated.
 *
 *   SH_INVOBJ		Failure.
 *			The ARRAY object is not valid.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/*   shTblTBLFLDsetWithDouble */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
   TBLFLD		*lcl_tblFld = ((TBLFLD *)0);

#define	lcl_case(keyword)\
   case SH_TBL_##keyword :           lcl_tblFld->keyword = tblKeyVal;\
                         shAddSet (lcl_tblFld->Tpres, SH_TBL_##keyword)

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * The checking of legal members is done prior to possibly allocating TBLFLD
    * space.
    */

   switch (tblKeyFld)
      {
      case SH_TBL_TSCAL    :
      case SH_TBL_TZERO    :
                           break;

      case SH_TBL_TTYPE    :
      case SH_TBL_TUNIT    :
      case SH_TBL_TNULLSTR :
      case SH_TBL_TDISP    :
      case SH_TBL_TNULLINT :
                           lcl_status = SH_INVARG;
                           shErrStackPush ("TBLFLD member does not accept double value");
                           goto rtn_return;
                           break;

      default            : lcl_status = SH_INVARG;
                           shErrStackPush ("Invalid or unknown member of TBLFLD");
                           goto rtn_return;
                           break;
      }

   if ((lcl_status = shTblTBLFLDlink (array, &lcl_tblFld)) != SH_SUCCESS)
      {
      goto rtn_return;
      }

   switch (tblKeyFld)
      {
      lcl_case (TSCAL);    break;
      lcl_case (TZERO);    break;

      default : lcl_status = SH_INTERNAL_ERR;
                shErrStackPush ("Internal error (%s line %d): members accepted above not reflected here", __FILE__, __LINE__);
                shFatal        ("Internal error (%s line %d): members accepted above not reflected here", __FILE__, __LINE__ - 1);
                goto rtn_return;
                break;
      }

   /*
    * All done.
    */

   if (tblFld != ((TBLFLD **)0)) { *tblFld = lcl_tblFld; }

rtn_return : ;

   return (lcl_status);

#undef	lcl_case

   }	/*   shTblTBLFLDsetWithDouble */

/******************************************************************************/

RET_CODE	   shTblTBLFLDsetWithInt
   (
   ARRAY		 *array,	/* INOUT: ARRAY (field) to set value  */
   TBLFLD_KEYWORDS	  tblKeyFld,	/* IN:    TBLFLD member to set        */
                  int	  tblKeyVal,	/* IN:    TBLFLD member value         */
   TBLFLD		**tblFld	/* OUT:   Address of ARRAY's TBLFLD   */
   )

/*++
 * ROUTINE:	   shTblTBLFLDsetWithInt
 *
 *	Set the specified TBLFLD member with a signed integer value.  Any
 *	previous value is destroyed.  If the ARRAY (field) does not have a
 *	TBLFLD associated with it, the TBLFLD is first allocated.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_INVARG		Failure.
 *			The TBLFLD member indicator is invalid, (does not exist
 *			or member is does not accept a signed integer).
 *
 *   SH_ALREXIST	Failure.
 *			The ARRAY has a different informative structure (not a
 *			TBLFLD) hung off of itself.
 *
 *   SH_MALLOC_ERR	Failure.
 *			Space for the TBLFLD could not be allocated.
 *
 *   SH_INVOBJ		Failure.
 *			The ARRAY object is not valid.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/*   shTblTBLFLDsetWithInt */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
   TBLFLD		*lcl_tblFld = ((TBLFLD *)0);

#define	lcl_case(keyword)\
   case SH_TBL_##keyword :           lcl_tblFld->keyword = tblKeyVal;\
                         shAddSet (lcl_tblFld->Tpres, SH_TBL_##keyword)

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * The checking of legal members is done prior to possibly allocating TBLFLD
    * space.
    */

   switch (tblKeyFld)
      {
      case SH_TBL_TNULLINT :
                           break;

      case SH_TBL_TTYPE    :
      case SH_TBL_TUNIT    :
      case SH_TBL_TNULLSTR :
      case SH_TBL_TDISP    :
      case SH_TBL_TSCAL    :
      case SH_TBL_TZERO    :
                           lcl_status = SH_INVARG;
                           shErrStackPush ("TBLFLD member does not accept signed integer value");
                           goto rtn_return;
                           break;

      default            : lcl_status = SH_INVARG;
                           shErrStackPush ("Invalid or unknown member of TBLFLD");
                           goto rtn_return;
                           break;
      }

   if ((lcl_status = shTblTBLFLDlink (array, &lcl_tblFld)) != SH_SUCCESS)
      {
      goto rtn_return;
      }

   switch (tblKeyFld)
      {
      lcl_case (TNULLINT); break;

      default : lcl_status = SH_INTERNAL_ERR;
                shErrStackPush ("Internal error (%s line %d): members accepted above not reflected here", __FILE__, __LINE__);
                shFatal        ("Internal error (%s line %d): members accepted above not reflected here", __FILE__, __LINE__ - 1);
                goto rtn_return;
                break;
      }

   /*
    * All done.
    */

   if (tblFld != ((TBLFLD **)0)) { *tblFld = lcl_tblFld; }

rtn_return : ;

   return (lcl_status);

#undef	lcl_case

   }	/*   shTblTBLFLDsetWithInt */

/******************************************************************************/

RET_CODE	   shTblFldAdd
   (
   TBLCOL		 *tblCol,	/* INOUT: Pointer to Table descriptor */
                  int	  tblFldPres,	/* IN:    Boolean: TBLFLD too?        */
   ARRAY		**array,	/* OUT:   ARRAY  field descriptor     */
   TBLFLD		**tblFld,	/* OUT:   TBLFLD field descriptor     */
                  int	 *tblFldIdx	/* OUT:   Field index (0-indexed)     */
   )

/*++
 * ROUTINE:	   shTblFldAdd
 *
 *	Add an empty field to the end of fields in a TBLCOL Table.  The ARRAY
 *	structures (and optional TBLFLD structures) are allocated and linked
 *	together properly.  Some initial filling of information is also per-
 *	formed:
 *
 *	   o	array->dimCnt = 1;		First dimension: number of rows
 *	   o	array->dim[0] = tblCol->rowCnt;	First dimension: number of rows
 *	   o	array->nStar  = 0;		No data indirection.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_INVARG		Failure.
 *			The TBLCOL Table is not valid.
 *
 *   SH_MALLOC_ERR	Failure.
 *			Space for the field or its associated structures could
 *			not be allocated.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	Because the first dimension in an ARRAY is the number of rows in the
 *	Table, this information is filled in for the ARRAY.  When users are
 *	adding additional dimensions, they should be added with:
 *
 *	   array->dim[array->dimCnt++] = nnn;
 *--
 ******************************************************************************/

   {	/* shTblFldAdd */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status    = SH_SUCCESS;
   ARRAY		*lcl_arrayTmp;
   ARRAY		*lcl_array     = NULL;
   int			 lcl_tblFldIdx = 0;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Check arguments.
    */

   if (tblCol == ((TBLCOL *)0))
      {
      lcl_status = SH_INVARG;
      shErrStackPush ("Invalid TBLCOL");
      goto rtn_return;
      }

   if ((lcl_array = shArrayNew ()) == ((ARRAY *)0))
      {
      lcl_status = SH_MALLOC_ERR;
      shErrStackPush ("Unable to allocate space for Table field ARRAY descriptor");
      goto rtn_return;
      }

   if ((lcl_status = shChainElementAddByPos(&(tblCol->fld), (void *)lcl_array, "ARRAY", TAIL, AFTER))
        != SH_SUCCESS)  
      {
      shArrayDel(lcl_array);
      shErrStackPush ("Unable to append ARRAY field descriptor into Table (TBLCOL) list");
      return lcl_status;   /* Bail out */

      }
   if (tblFldPres)
      {
      if ((lcl_status = shTblTBLFLDlink (lcl_array, ((TBLFLD **)0))) != SH_SUCCESS)
         {
         shErrStackPush ("Unable to associate TBLFLD with Table's ARRAY");
         goto rtn_return;
         }
      }

   lcl_array->dimCnt = 1;		/* Table row count must be consistent */
   lcl_array->dim[0] = tblCol->rowCnt;	/* ... from field to field.           */

   /*
    * Determine position of the new field (only if requested).
    */

   if (tblFldIdx != ((int *)0))
      {
      lcl_arrayTmp = NULL;
      for (lcl_tblFldIdx = 0; lcl_tblFldIdx < shChainSize(&(tblCol->fld)); lcl_tblFldIdx++)  {
           lcl_arrayTmp = shChainElementGetByPos(&(tblCol->fld), lcl_tblFldIdx);
           if (lcl_arrayTmp == lcl_array)
               break;
      }

      if (lcl_tblFldIdx >= shChainSize(&(tblCol->fld)))
         {
         lcl_status = SH_INTERNAL_ERR;
         shErrStackPush ("Internal error (%s line %d): inserted ARRAY could not be located", __FILE__, __LINE__);
         shFatal        ("Internal error (%s line %d): inserted ARRAY could not be located", __FILE__, __LINE__ - 1);
         goto rtn_return;
         }
      }

   /*
    * All done.
    */

rtn_return : ;

   if (lcl_status != SH_SUCCESS)
      {
      if (tblCol  != ((TBLCOL *)0))
         {
         p_shTblFldDelByIdx(&(tblCol->fld), TAIL);  /* Clean up */
         }
      }
   else
      {
      if (array     != ((ARRAY  **)0)) { *array     = lcl_array;     }
      if (tblFld    != ((TBLFLD **)0)) { *tblFld    = ((TBLFLD *)lcl_array->infoType); }
      if (tblFldIdx != ((int     *)0)) { *tblFldIdx = lcl_tblFldIdx; }
      }

   return (lcl_status);

   }	/* shTblFldAdd */

/******************************************************************************/

RET_CODE	 p_shTblFldGen
   (
   CHAIN		 *fldList,	/* INOUT: ARRAY list head             */
   HDR_VEC		 *hdrVec,	/* IN:    Header vector for HDU       */
   FITSHDUTYPE		  hduType,	/* IN:    HDU type                    */
                  int	 *TFIELDS,	/* OUT:   TFIELDS value               */
                  int	 *NAXIS1,	/* OUT:   NAXIS1  value               */
                  int	 *NAXIS2	/* OUT:   NAXIS2  value               */
   )

/*++
 * ROUTINE:	 p_shTblFldGen
 *
 *	Parse the FITS ASCII or Binary Table header and generate a list of
 *	ARRAY field descriptors.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_NOT_SUPPORTED	Failure.
 *			The HDU type is not supported.
 *
 *   SH_INV_FITS_FILE	Failure.
 *			The FITS header is invalid.
 *
 *   SH_MALLOC_ERR	Failure.
 *			Space could not be allocated for either the ARRAY and/or
 *			TBLFLD field descriptors.
 *
 * SIDE EFFECTS:
 *
 *   o	For Binary Tables, TDIMn specifies array dimension in column-major order
 *	(first subscript varies most rapidly).  This is reversed for the Dervish
 *	environment to a row-major order (last subscript varies most rapidly):
 *
 *	                              th
 *	   d   is  TDIMn's (c - i + 1)   subscript
 *	    i
 *
 *	where c is the number of dimensions.  i and TDIMn's subscripts are
 *	1-indexed.
 *
 *	   TDIMn = '(s ,s , ...,s )'
 *	              1  2       c
 *
 *	d  is the Table row index and is set to NAXIS2's value.
 *	 0
 *
 *   o	For Binary Tables, if the TSCALn and TZEROn are proper (as deemed by the
 *	FITS community convention) for an integer, the field "signedness" is
 *	flipped.
 *
 *	   o	This is also done for heap integer data, even though the FITS
 *		Standard indicates that TSCALn and TZEROn are undefined for the
 *		'P' data type (in the RSA).
 *
 * SYSTEM CONCERNS:
 *
 *   o	TFIELDS, NAXIS1, and NAXIS2 are returned here to provide a small perfor-
 *	mance improvement.  The caller will more than likely be interested in
 *	these values, and thus, the FITS header only needs to be searched once
 *	for these keywords.
 *
 *   o	Field dimensions may be modified based on the data type.  For an ASCII
 *	character string field ('A' data type and object schema type "STR"), an
 *	additional character is allocated for each string (that is, the fastest
 *	varying dimension is made one greater) to allow all strings to be null
 *	terminated.  Table readers   M U S T   take this dimension change into
 *	consideration!
 *
 *	TBLFLD's prvt.TFORM.ascFldWidth and prvt.elemCnt do reflect this change
 *	in dimension also.
 *--
 ******************************************************************************/

   {	/* p_shTblFldGen */

   /*
    * Declarations.
    */

   RET_CODE		  lcl_status = SH_SUCCESS;
   RET_CODE		  lcl_statusTmp;
   ARRAY		 *lcl_array;
   TBLFLD		 *lcl_tblFld;		/* TBLFLD structure           */
                  int	  lcl_fldIdx;		/* Field index (0-indexed)    */
                  int	  lcl_fldWidth;		/* Field width (bytes) in file*/
   unsigned long  int	  lcl_rowWidth;		/* # bytes used per RSA row   */
   unsigned long  int	  lcl_rowWidthCand;	/* Candidate val for row width*/
                  int	  lcl_dataTypeIdx;
   struct shTbl_SuppData *lcl_dataTypeMap;	/* Which data types table?    */
   TYPE			  lcl_flipType;		/* Sign flipped data type     */
   double		  lcl_flipTSCAL;	/* TSCAL needed to flip sign  */
   double		  lcl_flipTZERO;	/* TZERO needed to flip sign  */

   TYPE			  lcl_schemaTypeSTR        = shTypeGetFromName ("STR");
   TYPE			  lcl_schemaTypeTBLHEAPDSC = shTypeGetFromName ("TBLHEAPDSC");

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   switch (hduType)
      {
      case f_hduTABLE    : lcl_dataTypeMap = p_shTblDataTypeTABLEmap;    break;
      case f_hduBINTABLE : lcl_dataTypeMap = p_shTblDataTypeBINTABLEmap; break;
      default            : lcl_status      = SH_NOT_SUPPORTED; goto rtn_return; break;
      }

   /*
    * Most legalities of the Table header have been checked (ultimately by
    * f_hinfo () called by p_shFitsHdrLocate ()).
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

   f_ikey (TFIELDS, hdrVec, "TFIELDS");	/* Number of fields in table          */
   f_ikey (NAXIS1,  hdrVec, "NAXIS1");	/* Number of bytes per row            */
   f_ikey (NAXIS2,  hdrVec, "NAXIS2");	/* Number of bytes per row            */

   /*
    * Allocate space for each TBLFLD field descriptor.
    *
    *   o   If any fields already exist, they are deleted first.  Although this
    *       may fragment memory, it does properly delete pointer/data areas for
    *       the existing TBLFLDs.
    *
    *   o   Fields are linked together through the ARRAY structure.
    */

   while (p_shTblFldDelByIdx (fldList, 0) == SH_SUCCESS) {} /*Free existing*/

   for (lcl_fldIdx = 0; lcl_fldIdx < *TFIELDS; lcl_fldIdx++)
      {
      if ((lcl_array  = shArrayNew  ()) == ((ARRAY  *)0))
         {
         lcl_status = SH_MALLOC_ERR;
         shErrStackPush ("Unable to allocate space for %s Table field ARRAY descriptors",
                        (hduType == f_hduTABLE)    ? "ASCII"
                      : (hduType == f_hduBINTABLE) ? "Binary"
                                                   : "(unknown)");
         goto rtn_return;
         }
      if ((lcl_status = shTblTBLFLDlink (lcl_array, ((TBLFLD **)0))) != SH_SUCCESS)
         {
         shErrStackPush ("Unable to associate TBLFLD with %s Table's ARRAY",
                        (hduType == f_hduTABLE)    ? "ASCII"
                      : (hduType == f_hduBINTABLE) ? "Binary"
                                                   : "(unknown)");
         goto rtn_return;
         }
      if ((lcl_status = shChainElementAddByPos(fldList, (void *)lcl_array, "ARRAY", TAIL, AFTER))
           != SH_SUCCESS)
         {
         shErrStackPush ("Unable to chain ARRAY field descriptor into % Table (TBLCOL) list",
                        (hduType == f_hduTABLE)    ? "ASCII"
                      : (hduType == f_hduBINTABLE) ? "Binary"
                                                   : "(unknown)");
         goto rtn_return;
         }
      }

   /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/
   /*
    * Compute the size of the area we'll need to store the RSA (record storage
    * area).
    *
    *   o   The size needed for the heap area will also be computed.  This will
    *       only be an approximation, since the 'P' data type is not required to
    *       specify the variable length array data type.  In that case, the
    *       variable array size is the same as in the FITS file HDU.
    *
    *   o   TFORMn syntax checking is performed.  TBCOLn is handled also if this
    *       is an ASCII Table.
    */

   lcl_rowWidth = 0;			/* Haven't used any part of row yet   */

   for (lcl_fldIdx = 0; (lcl_fldIdx < shChainSize(fldList)) && (lcl_fldIdx < *TFIELDS); lcl_fldIdx++)
     {
      lcl_array = (ARRAY *) shChainElementGetByPos(fldList, lcl_fldIdx);

      shAssert ((lcl_tblFld = ((TBLFLD *)lcl_array->info)) != ((TBLFLD *)0)); /* OK cast as we filled in */

      lcl_statusTmp = (hduType == f_hduBINTABLE)
                        ? p_shTblFldParseTFORMbin (hdrVec, lcl_tblFld, lcl_fldIdx, &lcl_fldWidth, lcl_dataTypeMap, &lcl_dataTypeIdx)
                    : (hduType == f_hduTABLE)
                        ? p_shTblFldParseTFORMasc (hdrVec, lcl_tblFld, lcl_fldIdx, &lcl_fldWidth, lcl_dataTypeMap, &lcl_dataTypeIdx)
                    : SH_INTERNAL_ERR;

      if (lcl_statusTmp != SH_SUCCESS)
         {
          if ((lcl_status = lcl_statusTmp) == SH_INTERNAL_ERR)
             {
             shErrStackPush ("FITS HDU type not caught earlier (%s line %d)", __FILE__, __LINE__);
             shFatal        ("FITS HDU type not caught earlier (%s line %d)", __FILE__, __LINE__);
             }
          continue;				/* Onto next field            */
         }

      /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */
      /*
       * Check for other FITS keywords.
       *
       *   o   TSCAL and TZERO are defaulted (without making them present) to
       *       simplify code below.
       */

      if ((lcl_statusTmp = s_shTblTBLFLDreadDkey    (hdrVec, lcl_tblFld, lcl_fldIdx, SH_TBL_TSCAL,    "TSCAL",
                                            &lcl_tblFld->TSCAL,    lcl_dataTypeMap[lcl_dataTypeIdx].fldTSCALok)) != SH_SUCCESS)
         {
           lcl_status    = lcl_statusTmp;
         }
      if (!shInSet (lcl_tblFld->Tpres, SH_TBL_TSCAL)) { lcl_tblFld->TSCAL = 1.0; }

      if ((lcl_statusTmp = s_shTblTBLFLDreadDkey    (hdrVec, lcl_tblFld, lcl_fldIdx, SH_TBL_TZERO,    "TZERO",
                                            &lcl_tblFld->TZERO,    lcl_dataTypeMap[lcl_dataTypeIdx].fldTZEROok)) != SH_SUCCESS)
         {
           lcl_status    = lcl_statusTmp;
         }
      if (!shInSet (lcl_tblFld->Tpres, SH_TBL_TZERO)) { lcl_tblFld->TZERO = 0.0; }

      if (hduType == f_hduBINTABLE)
         {
         if ((lcl_statusTmp = s_shTblTBLFLDreadIkey (hdrVec, lcl_tblFld, lcl_fldIdx, SH_TBL_TNULLINT, "TNULL",
                                            &lcl_tblFld->TNULLINT, lcl_dataTypeMap[lcl_dataTypeIdx].fldTNULLok)) != SH_SUCCESS)
            {
              lcl_status    = lcl_statusTmp;
            }
         }
      else					/* ASCII Table                */
         {
         if ((lcl_statusTmp = s_shTblTBLFLDreadAkey (hdrVec, lcl_tblFld, lcl_fldIdx, SH_TBL_TNULLSTR, "TNULL",
                                             lcl_tblFld->TNULLSTR, lcl_dataTypeMap[lcl_dataTypeIdx].fldTNULLok)) != SH_SUCCESS)
            {
              lcl_status    = lcl_statusTmp;
            }
         }

      if ((lcl_statusTmp = s_shTblTBLFLDreadAkey    (hdrVec, lcl_tblFld, lcl_fldIdx, SH_TBL_TTYPE,    "TTYPE",
                                             lcl_tblFld->TTYPE,    1)) != SH_SUCCESS)
         {
           lcl_status    = lcl_statusTmp;
         }
      if ((lcl_statusTmp = s_shTblTBLFLDreadAkey    (hdrVec, lcl_tblFld, lcl_fldIdx, SH_TBL_TUNIT,    "TUNIT",
                                             lcl_tblFld->TUNIT,    1)) != SH_SUCCESS)
         {
           lcl_status    = lcl_statusTmp;
         }

      /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */
      /*
       * Determine whether the "signedness" of the data should be flipped.
       *                                                                  ###
       *   o   The FITS Standard is "violated" here because the 'P' data  ###
       *       is allowed to specify TSCALn and TZEROn.                   ###
       */

      lcl_tblFld->prvt.signFlip = TBLFLD_SIGNNOFLIP;  /* No sign bit flipping */

      if (hduType == f_hduBINTABLE)
         {
         if (lcl_array->data.schemaType == lcl_schemaTypeTBLHEAPDSC)
            {
            if   (p_shTblFldSignFlip (lcl_tblFld->heap.schemaType, &lcl_flipType, &lcl_flipTSCAL, &lcl_flipTZERO) == SH_SUCCESS)
               {
               if ((lcl_flipTSCAL == lcl_tblFld->TSCAL) && (lcl_flipTZERO == lcl_tblFld->TZERO))
                  {
                  lcl_tblFld->prvt.signFlip = TBLFLD_SIGNFLIP;	/* Flip sign  */
                  lcl_tblFld->heap.schemaType = lcl_flipType;	/* Change type*/
                  shClrSet (lcl_tblFld->Tpres, SH_TBL_TSCAL); lcl_tblFld->TSCAL = 1.0;
                  shClrSet (lcl_tblFld->Tpres, SH_TBL_TZERO); lcl_tblFld->TZERO = 0.0;
                  }
               }
            }
         else if (p_shTblFldSignFlip (lcl_array->data.schemaType, &lcl_flipType, &lcl_flipTSCAL, &lcl_flipTZERO) == SH_SUCCESS)
            {
            if ((lcl_flipTSCAL == lcl_tblFld->TSCAL) && (lcl_flipTZERO == lcl_tblFld->TZERO))
               {
               lcl_tblFld->prvt.signFlip  = TBLFLD_SIGNFLIP;	/* Flip sign  */
               lcl_array->data.schemaType = lcl_flipType;	/* Change type*/
               shClrSet (lcl_tblFld->Tpres, SH_TBL_TSCAL); lcl_tblFld->TSCAL = 1.0;
               shClrSet (lcl_tblFld->Tpres, SH_TBL_TZERO); lcl_tblFld->TZERO = 0.0;
               }
            }
         }

      /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */
      /*
       * Get dimension information.
       *
       *   array->dimCnt is the number of dimensions from the TDIMn keyword plus
       *		1 for the row index.
       *
       *   array->dim	the array dimensions.  array->dim[0] is the number of
       *                rows, namely, NAXIS2.
       *
       *                NOTE:  These array->dim's are in row-major order,
       *                       reversed from FITS' TDIM of column-major order.
       *
       * For an ASCII character string field ('A' data type and object schema
       * type "STR"), an additional character is allocated for each string (that
       * is, the fastest varying dimension is made one greater) to allow all
       * strings to be null terminated.  Table readers   M U S T   take this
       * dimension change into consideration!
       */

      lcl_array->dimCnt = 1;			/* Assume it's scalar         */
      lcl_array->dim[0] = *NAXIS2;

      if (hduType == f_hduBINTABLE)		/* TDIM for BINTABLE only     */
         {
         if ((lcl_statusTmp = p_shTblFldParseTDIM (hdrVec, lcl_tblFld, lcl_fldIdx)) != SH_SUCCESS)
            {
              lcl_status    = lcl_statusTmp;
              continue;				/* Onto next field            */
            }
         if (lcl_array->data.schemaType == lcl_schemaTypeSTR)	/*Adjust char */
            {				/* string field width f/ ASCII Table  */
             lcl_tblFld->prvt.TFORM.ascFldWidth = lcl_array->dim[lcl_array->dimCnt - 1];
            }
         if ((lcl_tblFld->prvt.dataType == 'P') && shInSet (lcl_tblFld->Tpres, SH_TBL_TDIM))
            {
              lcl_status = SH_NOT_SUPPORTED;
              shErrStackPush ("TDIM%d keyword not supported with TFORM%d 'P' data type", lcl_fldIdx + 1, lcl_fldIdx + 1);
              continue;				/* Onto next field            */
            }
         }
      else					/* ASCII Table                */
         {
         /*
          * Fudge/adjust array dimensions.
          *
          *   o   If the element count is greater than 1, an array is implied.
          *       Force it to be 1-D (beyond the row index).
          *
          *   o   If the data type is a character string, an array is implied.
          *       Force it to be 1-D (beyond the row index).
          *
          * NOTE:  This is done here since ASCII Tables do not permit the TDIMn
          *        keyword.  As p_shTblFldParseTDIM () will not be called, the
          *        field element count will not be reflected as an array dimen-
          *        sion.  This is done explicitly here.
          */

         if ((lcl_tblFld->prvt.elemCnt > 1) || (lcl_array->data.schemaType == lcl_schemaTypeSTR))
            {
            lcl_array->dim[lcl_array->dimCnt] =  lcl_tblFld->prvt.elemCnt;
                           lcl_array->dimCnt++;
            }
         }

      /*
       *   o   Compute space usage by data in the FITS file row.
       *
       *       NOTE:  This needs to be done prior to the fudging of the arrays
       *              sizes for character strings.
       */

      if (hduType == f_hduBINTABLE)		/* Binary Table               */
         {
         lcl_rowWidth += lcl_fldWidth;
         }
      else					/* ASCII Table                */
         {
         if (lcl_tblFld->prvt.TFORM.ascFldWidth > 0)
            {
            if (lcl_rowWidth < (lcl_rowWidthCand = lcl_tblFld->prvt.TFORM.ascFldOff + lcl_tblFld->prvt.TFORM.ascFldWidth))
               {
                lcl_rowWidth =  lcl_rowWidthCand;
               }
            }
         }

      /*
       *   o   For character string fields, add an extra character for each
       *       string to allow null termination.
       *
       *       NOTE:  For 'A' (object schema type "STR") fields, the code above
       *              guarantees that there will be at least two dimensions, the
       *              row index and the number of characters in a string.
       */

      lcl_tblFld->prvt.schemaCnt = lcl_tblFld->prvt.elemCnt;	/* For most   */
								/* data types */
      if (lcl_array->data.schemaType == lcl_schemaTypeSTR)
         {
	   if (lcl_array->dim[lcl_array->dimCnt - 1] != 0) {
	     lcl_tblFld->prvt.schemaCnt /= lcl_array->dim[lcl_array->dimCnt - 1]; /* Adjust */
	   }
	   lcl_array->dim[lcl_array->dimCnt - 1]++;	/* For null term      */

          /*
           * Reflect dimension change for character strings elsewhere.
           */

          lcl_tblFld->prvt.elemCnt += lcl_tblFld->prvt.schemaCnt;
          lcl_tblFld->prvt.TFORM.ascFldWidth++;
         }
      }

   /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

   if (*NAXIS1 < lcl_rowWidth)
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("TFORMn data specifications extend beyond row (NAXIS1 of %d vs. %d bytes)", *NAXIS1, lcl_rowWidth);
      }

   /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/
   /*
    * All done.
    *
    *   o   If there were any errors, clean up after ourselves.
    */

rtn_return : ;

   if (lcl_status != SH_SUCCESS)
      {
      while (p_shTblFldDelByIdx (fldList, 0) == SH_SUCCESS) {}
      }

   return (lcl_status);

   }	/* p_shTblFldGen */

RET_CODE	 p_shTblFldDelByIdx
   (
   CHAIN	*fldList,	/* INOUT: ARRAY list head             */
   unsigned int	 fldIdx		/* IN:    Field position (0-indexed)  */
   )

/*++
 * ROUTINE:	 p_shTblFldDelByIdx
 *
 *	Delete a field from a TBLCOL (ARRAY list).
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *			The specified field was deleted.
 *
 *   SH_CHAIN_EMPTY	Failure.
 *			The specified field could not be located from the list
 *			of fields.  No field was deleted.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	The pointer/data space used by ARRAY (array->arrayPtr)   M U S T   have
 *	been allocated with shMalloc ().  shFree () is used to release it.
 *--
 ******************************************************************************/

   {	/* p_shTblFldDelByIdx */

   ARRAY *pArray;

   pArray = (ARRAY *)shChainElementRemByPos(fldList, fldIdx);

   if (g_shChainErrno != SH_SUCCESS)
       return g_shChainErrno;

   if (pArray != NULL)  {
       shTblfldDel((TBLFLD *)pArray->info);
       shArrayDel(pArray);
   }

   return SH_SUCCESS;

   }	/* p_shTblFldDelByIdx */

/******************************************************************************/

RET_CODE	 p_shTblFldParseTFORMasc
   (
   HDR_VEC		 *hdrVec,	/* IN:    Header vector for HDU       */
   TBLFLD		 *tblFld,	/* INOUT: Table field descriptor      */
                 int	  fldIdx,	/* IN:    Field index (0-indexed)     */
                 int	 *fldWidth,	/* OUT:   Field width (bytes) in file */
   struct shTbl_SuppData *dataTypeMap,	/* IN:    Supported data types table  */
                 int	 *dataTypeIdx	/* OUT:   Index to data type map      */
   )

/*++
 * ROUTINE:	 p_shTblFldParseTFORMasc
 *
 *	Parse a ASCII  Table TFORM keyword from a FITS header.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_INV_FITS_FILE	Failure.
 *			The TFORM keyword in the FITS ASCII Table header is
 *			invalid.
 *
 * SIDE EFFECTS:
 *
 *   o	TBCOLn is checked for and parsed.
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* p_shTblFldParseTFORMasc */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
                  char	 lcl_keyword[11+1];	/* FITS keyword(only need 8+1)*/
                  char	 lcl_keyAval[shFitsHdrStrSize+1]; /* Keyword str value*/
                  char	*lcl_keyAptr;
                  int	 lcl_dataTypeIdx = -1;
   SHALIGN_TYPE		 lcl_dataType;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Parse TBCOLn.
    *
    *   o   prvt.TFORM.ascFldOff is 0-indexed while TBCOLn is 1-indexed.
    */

   sprintf (lcl_keyword, "TBCOL%d", fldIdx + 1);	/* Gen indexed keyword*/
   if (!f_ikey (&tblFld->prvt.TFORM.ascFldOff, hdrVec, lcl_keyword))
      {
       lcl_status  = SH_INV_FITS_FILE;
      }
   else if (tblFld->prvt.TFORM.ascFldOff <= 0)
      {
       lcl_status  = SH_INV_FITS_FILE;
      }
   else
      {
       tblFld->prvt.TFORM.ascFldOff--;
      }
   if (lcl_status != SH_SUCCESS)
      {
      shErrStackPush ("TBCOL%d keyword missing or invalid", fldIdx + 1);
      }

   /*
    * Parse TFORMn.
    */

   sprintf (lcl_keyword, "TFORM%d", fldIdx + 1);	/* Gen indexed keyword*/
   if (!f_akey (lcl_keyAval, hdrVec, lcl_keyword))
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("TFORM%d keyword missing or invalid", fldIdx + 1);
      }

   if (lcl_status != SH_SUCCESS)
      {
      goto rtn_return;
      }

   shAddSet (tblFld->Tpres, SH_TBL_TFORM);	/* Show TFORM present         */

   /*
    * Check the data type.
    */

   lcl_keyAptr = lcl_keyAval;

   tblFld->prvt.TFORM.ascFldFmt =
   tblFld->prvt.dataType        = *lcl_keyAptr++;	/* Get field format   */

   /*
    *   o   Check if the data type (from the TFORMn) is supported.
    */

   for (lcl_dataTypeIdx = 0; (   (dataTypeMap[lcl_dataTypeIdx].dataType != '\000')
                              && (dataTypeMap[lcl_dataTypeIdx].dataType != tblFld->prvt.dataType));
        lcl_dataTypeIdx++) {}

   if (dataTypeMap[lcl_dataTypeIdx].dataType == '\000')
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("TFORM%d keyword has invalid/unsupported data type '%c'", fldIdx + 1, tblFld->prvt.dataType);
      goto rtn_return;
      }

   lcl_dataType = dataTypeMap[lcl_dataTypeIdx].fldType;

   /*
    *   o   Parse out the field width.
    *
    *       For non-numeric data types, the field width is fudged to be the
    *       element count (akin to Binary Tables).  This helps the ASCII Table
    *       data reader.
    *
    *       NOTE:  strtol () provides more information than sscanf (). But, it
    *              does allow the FITS standard to be violated because strtol ()
    *              will accept leading white space.
    */

   if ((tblFld->prvt.TFORM.ascFldWidth = strtol (lcl_keyAptr, &lcl_keyAptr, 10)) == 0)
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("TFORM%d keyword must specify field width", fldIdx + 1);
      }

   switch (tblFld->prvt.dataType)
      {
      case 'A' : tblFld->prvt.elemCnt = tblFld->prvt.TFORM.ascFldWidth; break;
      default  : tblFld->prvt.elemCnt = 1;                              break;
      }

   /*
    *   o   Check for a decimal field width (decimal point position).
    */

   switch (tblFld->prvt.dataType)
      {
      case 'A' :
      case 'I' :
                 tblFld->prvt.TFORM.ascFldDecPt = 0;
                 break;
      case 'F' :
      case 'E' :
      case 'D' :
                 if (*lcl_keyAptr++ != '.')
                    {
                    lcl_status = SH_INV_FITS_FILE;
                    shErrStackPush ("TFORM%d keyword expected decimal point position for '%c' data type", fldIdx + 1,
                                     tblFld->prvt.dataType);
                    goto rtn_return;
                    }
                 if ((tblFld->prvt.TFORM.ascFldDecPt = strtol (lcl_keyAptr, &lcl_keyAptr, 10)) == 0)
                    {
                    lcl_status = SH_INV_FITS_FILE;
                    shErrStackPush ("TFORM%d keyword expected decimal point position for '%c' data type", fldIdx + 1,
                                     tblFld->prvt.dataType);
                    goto rtn_return;
                    }
                 break;
      default  :
                 lcl_status = SH_INTERNAL_ERR;
                 shErrStackPush ("Not all supported data types handled (%s line %d)", __FILE__, __LINE__);
                 shFatal        ("Not all supported data types handled (%s line %d)", __FILE__, __LINE__);
                 goto rtn_return;
                 break;
      }

   /*
    *   o   Make sure no junk is after the width and decimal point position.
    */

   if (*(lcl_keyAptr + strspn (lcl_keyAptr, " ")) != ((char)0))
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("TFORM%d keyword invalid", fldIdx + 1);
      goto rtn_return;
      }

   /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/
   /*
    * All done.
    */

   switch (tblFld->prvt.dataType)
      {
      case 'A' : tblFld->array->data.schemaType = shTypeGetFromName ("STR");
                 tblFld->array->data.size       = shAlignment.type[lcl_dataType].size;
                 tblFld->array->data.align      = shAlignment.type[lcl_dataType].align;
                 tblFld->array->data.incr       = shAlignment.type[lcl_dataType].incr;
                 break;
      default  : tblFld->array->data.schemaType = shAlignAlignToSchemaMap (lcl_dataType);
                 tblFld->array->data.size       = shAlignment.type[lcl_dataType].size;
                 tblFld->array->data.align      = shAlignment.type[lcl_dataType].align;
                 tblFld->array->data.incr       = shAlignment.type[lcl_dataType].incr;
                 break;
      }

rtn_return : ;

   if (fldWidth    != ((int *)0)) { *fldWidth    = tblFld->prvt.TFORM.ascFldWidth; }
   if (dataTypeIdx != ((int *)0)) { *dataTypeIdx = lcl_dataTypeIdx; }

   return (lcl_status);

   }	/* p_shTblFldParseTFORMasc */

/******************************************************************************/

RET_CODE	 p_shTblFldParseTFORMbin
   (
   HDR_VEC		 *hdrVec,	/* IN:    Header vector for HDU       */
   TBLFLD		 *tblFld,	/* INOUT: Table field descriptor      */
                 int	  fldIdx,	/* IN:    Field index (0-indexed)     */
                 int	 *fldWidth,	/* OUT:   Field width (bytes) in file */
   struct shTbl_SuppData *dataTypeMap,	/* IN:    Supported data types table  */
                 int	 *dataTypeIdx	/* OUT:   Index to data type map      */
   )

/*++
 * ROUTINE:	 p_shTblFldParseTFORMbin
 *
 *	Parse a Binary Table TFORM keyword from a FITS header.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_INV_FITS_FILE	Failure.
 *			The TFORM keyword in the FITS Binary Table header is
 *			invalid.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* p_shTblFldParseTFORMbin */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
                  char	 lcl_keyword[11+1];	/* FITS keyword(only need 8+1)*/
                  char	 lcl_keyAval[shFitsHdrStrSize+1]; /* Keyword str value*/
                  char	*lcl_keyAptr;
                  int	 lcl_dataTypeIdx = -1;
                  int	 lcl_fldWidth;
                  char	 lcl_heapType;
                  int	 lcl_heapTypeIdx;
   TYPE			 lcl_schemaTypeUNKNOWN = shTypeGetFromName ("UNKNOWN");

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   sprintf (lcl_keyword, "TFORM%d", fldIdx + 1);	/* Gen indexed keyword*/
   if (!f_akey (lcl_keyAval, hdrVec, lcl_keyword))
      {
      lcl_status = SH_INV_FITS_FILE;
      shErrStackPush ("TFORM%d keyword missing or invalid", fldIdx + 1);
      goto rtn_return;
      }

   shAddSet (tblFld->Tpres, SH_TBL_TFORM);		/* Show TFORM present         */

   /*
    * Parse out the field element count.
    *
    * NOTE:  strtol () provides more information than sscanf (). But, it does
    *        allow the FITS standard to be violated because strtol () will
    *        accept leading white space and a sign (+/-).
    */

   if ((tblFld->prvt.elemCnt = strtol (lcl_keyAval, &lcl_keyAptr, 10)) == 0)
      {
      if (lcl_keyAptr == lcl_keyAval)
         {
         tblFld->prvt.elemCnt = 1;		/* Default element count      */
         }
      }

   /*
    * Check the data type.
    *
    *   o   Some data types restrict the element count.
    */

   tblFld->prvt.dataType = *lcl_keyAptr;	/* Get data type              */

   switch (tblFld->prvt.dataType)
      {
      case 'P' :  if ((tblFld->prvt.elemCnt < 0) || (tblFld->prvt.elemCnt > 1))
                     {
                     lcl_status = SH_INV_FITS_FILE;
                     shErrStackPush ("TFORM%d keyword 'P' data type must have an element count of 0 or 1", fldIdx + 1);
                     goto rtn_return;
                     }
      }

   /*
    *   o   Check if the data type (from the TFORMn) is supported.
    */

   if ((lcl_status = s_shTblFldParseTFORMbinDataType (tblFld->prvt.dataType, dataTypeMap, &lcl_dataTypeIdx, &tblFld->array->data))
                  != SH_SUCCESS)
      {
      shErrStackPush ("TFORM%d keyword has invalid/unsupported data type '%c'", fldIdx + 1, tblFld->prvt.dataType);
      goto rtn_return;
      }

   /*
    *   o   Determine the width of the data (bytes) in the row in the RSA.
    *									  ###
    *          o   'P' data type is special: it's 8 bytes in size.	  ###
    *									  ###
    *              All other data in the RSA is assumed to be the same	  ###
    *              size as the equivalent data in memory.		  ###
    */

   lcl_fldWidth = tblFld->prvt.elemCnt * ((tblFld->prvt.dataType != 'P') ? tblFld->array->data.size : 8);

   /*
    *   o   Anything after the data type that we should check?
    */

   if (tblFld->prvt.dataType == 'P')
      {
      if ((lcl_heapType = *(++lcl_keyAptr)) == ((char)0))   /* Get heap type  */
         {
         tblFld->heap.schemaType = lcl_schemaTypeUNKNOWN;
         }
      else
         {
         /* ### check for maxelem count ### */
         }

      /*
       * Check if the heap data type is supported.
       *
       *   o   An unknown data type is supported also.  But, the datum size is
       *       one byte and the alignment is conservative, that is, it's the
       *       largest alignment type of any of the primitive data types.
       */

      if (    (lcl_heapType == 'P')
          || ((lcl_status = s_shTblFldParseTFORMbinDataType (lcl_heapType, dataTypeMap, &lcl_heapTypeIdx, &tblFld->heap))
                         != SH_SUCCESS) )
         {
         shErrStackPush ("TFORM%d 'P' array descriptor has invalid/unsupported data type '%c'", fldIdx + 1, lcl_heapType);
         goto rtn_return;
         }

      if (tblFld->heap.schemaType == lcl_schemaTypeUNKNOWN)
         {
          tblFld->heap.size  = shAlignment.type[SHALIGNTYPE_U8].size;
          tblFld->heap.incr  = shAlignment.type[SHALIGNTYPE_U8].incr;
          tblFld->heap.align = shAlignment.alignMax;
         }

      shAddSet (tblFld->Tpres, SH_TBL_HEAP);	/* Show heap data is used     */
      }

   /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/
   /*
    * All done.
    */

   tblFld->prvt.TFORM.ascFldWidth = dataTypeMap[lcl_dataTypeIdx].ascFldWidthDef;
   tblFld->prvt.TFORM.ascFldDecPt = dataTypeMap[lcl_dataTypeIdx].ascFldDecPtDef;
   tblFld->prvt.TFORM.ascFldFmt   = dataTypeMap[lcl_dataTypeIdx].ascFldFmtDef;
   tblFld->prvt.TFORM.ascFldOff   = -1;		/* No specific offset         */

rtn_return : ;

   if (fldWidth    != ((int *)0)) { *fldWidth    = lcl_fldWidth; }
   if (dataTypeIdx != ((int *)0)) { *dataTypeIdx = lcl_dataTypeIdx; }

   return (lcl_status);

   }	/* p_shTblFldParseTFORMbin */

/******************************************************************************/

RET_CODE	 p_shTblFldParseTDIM
   (
   HDR_VEC		 *hdrVec,	/* IN:    Header vector for HDU       */
   TBLFLD		 *tblFld,	/* INOUT: Table field descriptor      */
                 int	  fldIdx	/* IN:    Field index (0-indexed)     */
   )

/*++
 * ROUTINE:	 p_shTblFldParseTDIM
 *
 *	Parse the TDIM keyword from a FITS header.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_NOT_SUPPORTED	Failure.
 *			Too many dimensions are specified by the TDIM keyword.
 *			This probably indicates that the underlying LIBFITS
 *			routine, f_akey (), has a problem.
 *
 *   SH_INV_FITS_FILE	Failure.
 *			The TDIM keyword in the FITS header is invalid.
 *
 * SIDE EFFECTS:
 *
 *   o	For Binary Tables, TDIMn specifies array dimension in column-major order
 *	(first subscript varies most rapidly).  This is reversed for the Dervish
 *	environment to a row-major order (last subscript varies most rapidly):
 *
 *	                              th
 *	   d   is  TDIMn's (c - i + 1)   subscript
 *	    i
 *
 *	where c is the number of dimensions.  i and TDIMn's subscripts are
 *	1-indexed.
 *
 *	   TDIMn = '(s ,s , ...,s )'
 *	              1  2       c
 *
 *	d  is the Table row index and is set to NAXIS2's value.
 *	 0
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* p_shTblFldParseTDIM */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
                  char	 lcl_keyword[11+1];	/* FITS keyword(only need 8+1)*/
                  char	 lcl_keyAval[shFitsHdrStrSize+1]; /* Keyword str value*/
                  char	*lcl_keyAptr;
                  char	*lcl_keyAptrSav;
   unsigned       int	 lcl_TDIMerr;		/* Error in current TDIMn?    */
            long  int	 lcl_TDIMelems;		/* Total element count        */
                  int	 lcl_TDIMidx;
   unsigned       int	 lcl_uint;		/* Temporary                  */

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Get dimension information.
    *
    *   array->dimCnt	is the number of dimensions from the TDIMn keyword plus
    *			1 for the row index.
    *
    *   array->dim	the array dimensions.  array->dim[0] is the number of
    *			rows, namely, NAXIS2.  NOTE:  These array->dim\s are in
    *			row-major order, reversed from FITS' TDIM of column-
    *                   major order.
    *
    * NOTE: strtol () provides more information than sscanf (). But, it does
    *       allow the FITS standard to be violated because strtol () will
    *       accept leading white space and a sign (+/-).
    */

   sprintf (lcl_keyword, "TDIM%d", fldIdx + 1);	/* Generate indexed keyword   */
   if (!f_akey (lcl_keyAval, hdrVec, lcl_keyword))
      {
      if ((tblFld->prvt.elemCnt > 1) || (tblFld->prvt.dataType == 'A'))
         {					/* Array implied if STRing or */
         tblFld->array->dim[tblFld->array->dimCnt] = /* elemCnt > 1. Force it */
         lcl_TDIMelems = tblFld->prvt.elemCnt;	/* to be 1-D beyond row index.*/
         tblFld->array->dimCnt++;
         }
      }
   else
      {
      shAddSet (tblFld->Tpres, SH_TBL_TDIM);	/* Show TDIM  present         */
      lcl_keyAptr = lcl_keyAval;
      if (*lcl_keyAptr++ != '(')		/* Skip past open "("         */
         {
         lcl_status = SH_INV_FITS_FILE;
         shErrStackPush ("TDIM%d keyword invalid", fldIdx + 1);
         goto rtn_return;
         }

      /*
       *   o   Check each dimension in TDIMn.
       */

      lcl_TDIMerr   = 0;			/* No error in this TDIMn     */
      lcl_TDIMelems = 1;
      for (tblFld->array->dimCnt = 1; tblFld->array->dimCnt <= (shBinTDIMlim); tblFld->array->dimCnt++)
         {
         lcl_keyAptrSav = lcl_keyAptr;		/* Save for strtol            */
         if ((tblFld->array->dim[tblFld->array->dimCnt] = strtol (lcl_keyAptr, &lcl_keyAptr, 10)) == 0)
            {
            lcl_TDIMerr = 1;			/* No # or a zero is an error */
            break;				/* Onto next field            */
            }
         lcl_TDIMelems *= tblFld->array->dim[tblFld->array->dimCnt];
         if (*lcl_keyAptr != ',')
            {
            tblFld->array->dimCnt++;		/* Count this dimension       */
            break;				/* No more dimensions         */
            }
         lcl_keyAptr++;				/* Skip comma delimiter       */
         }

      /*
       *   o   Check that the TDIMn list was terminated properly with a closing
       *       parenthesis and no junk after it (blanks are allowed).
       */

      if ((*lcl_keyAptr == ',') && (tblFld->array->dimCnt == (shArrayDimLim)))
         {
         lcl_status = SH_NOT_SUPPORTED;		/* Strong enough msg?     ### */
         shErrStackPush ("Possible internal error (LIBFITS f_akey ())");
         shErrStackPush ("TDIM%d keyword exceeded Dervish limit of %d dimensions", fldIdx + 1, (shArrayDimLim));
         goto rtn_return;
         }

      lcl_TDIMerr = (* lcl_keyAptr++                            != ')'   ) ? 1
                  : (*(lcl_keyAptr + strspn (lcl_keyAptr, " ")) != '\000') ? 1 : lcl_TDIMerr;

      if (lcl_TDIMerr != 0)
         {
         lcl_status = SH_INV_FITS_FILE;
         shErrStackPush ("TDIM%d keyword invalid", fldIdx + 1);
         goto rtn_return;
         }

      if (lcl_TDIMelems != tblFld->prvt.elemCnt)
         {
         lcl_status = SH_INV_FITS_FILE;
         shErrStackPush ("TDIM%d keyword element count does not match TFORM%d element count", fldIdx + 1, fldIdx + 1);
         goto rtn_return;
         }

      /*
       * Reverse the TDIMn subscripts from FITS' column-major to Dervish's
       * row-major order.
       *
       * NOTE: lcl_TDIMelems is reused here to control number of loops.
       */

      for (lcl_TDIMelems = (tblFld->array->dimCnt / 2), lcl_TDIMidx  = 1 /* Skip row subscript */;
                                                        lcl_TDIMidx <= lcl_TDIMelems; lcl_TDIMidx++)
         {
         lcl_uint = tblFld->array->dim[lcl_TDIMidx];
                    tblFld->array->dim[lcl_TDIMidx] = tblFld->array->dim[tblFld->array->dimCnt - lcl_TDIMidx];
                                                      tblFld->array->dim[tblFld->array->dimCnt - lcl_TDIMidx] = lcl_uint;
         }
      }

   /*
    * All done.
    */

rtn_return : ;

   return (lcl_status);

   }	/* p_shTblFldParseTDIM */

/******************************************************************************/

RET_CODE	 p_shTblFldHeapCnt
   (
   ARRAY		 *array,	/* IN:    ARRAY field descriptor      */
            long  int	 *heapNonZero,	/* OUT:   Number of non-zero counts   */
            long  int	 *heapCntAll,	/* OUT:   Total heap element count    */
            long  int	 *heapCntMax	/* OUT:   Maximum heap count          */
   )

/*++
 * ROUTINE:	 p_shTblFldHeapCnt
 *
 *	Count the number of elements in the heap and also return the largest
 *	single (per row) heap element count.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_INVOBJ		Failure.
 *			The ARRAY does not contain heap array descriptors (TBL-
 *			HEAPPTRs), the ARRAY is not a 1-dimensional array (or
 *			nStar is non-zero), or a heap element count is negative.
 *
 * SIDE EFFECTS:
 *
 *   o	No check is made to see that the ARRAY points of to a TBLFLD (the only
 *	structure that can own heap).
 *
 *   o	TBLHEAPDSCs with null pointers   A R E   counted towards returned
 *	values.
 *
 *   o	For character strings (STR object schema type), the additional character
 *	used for guaranteed null-termination   I S   counted towards returned
 *	values.
 *
 * SYSTEM CONCERNS:
 *
 *   o	There may be some controversy associated with a multidimensional array
 *	of TBLHEAPDSCs (as the FITS Standard only addresses multidimensional
 *	arrays as a convention, without explaining how that convention applies
 *	to variable length arrays).  Thus, only 1-dimensional arrays of TBLHEAP-
 *	PTRs are accepted.
 *--
 ******************************************************************************/

   {	/* p_shTblFldHeapCnt */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status      = SH_SUCCESS;
                  int	 lcl_rowIdx;
            long  int	 lcl_heapCnt;
            long  int	 lcl_heapNonZero = 0;
            long  int	 lcl_heapCntAll  = 0;
            long  int	 lcl_heapCntMax  = 0;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   if (array == ((ARRAY *)0))
      {
      goto rtn_return;
      }
   if ((array->dimCnt != 1) || (array->nStar != 0) || (array->data.schemaType != shTypeGetFromName ("TBLHEAPDSC")))
      {
      lcl_status = SH_INVOBJ;
      goto rtn_return;
      }

   for (lcl_rowIdx = (array->dim[0] - 1), lcl_heapCntAll = 0, lcl_heapCntMax = 0; lcl_rowIdx >= 0; lcl_rowIdx--)
      {
      if ((lcl_heapCnt = ((TBLHEAPDSC *)array->data.dataPtr)[lcl_rowIdx].cnt) < 0)
         {
           lcl_heapCnt = 0;
           lcl_status  = SH_INVOBJ;
           shErrStackPush ("Invalid heap array descriptor in row %d", lcl_rowIdx);
         }
      if (lcl_heapCnt    > 0)
         {
          lcl_heapNonZero++;
         }
      if (lcl_heapCntMax < lcl_heapCnt)
         {
          lcl_heapCntMax = lcl_heapCnt;
         }
      lcl_heapCntAll    += lcl_heapCnt;
      }

   /*
    * All done.
    */

rtn_return : ;

   if (heapNonZero != ((long int *)0)) { *heapNonZero = lcl_heapNonZero; }
   if (heapCntAll  != ((long int *)0)) { *heapCntAll  = lcl_heapCntAll;  }
   if (heapCntMax  != ((long int *)0)) { *heapCntMax  = lcl_heapCntMax;  }

   return (lcl_status);

   }	/* p_shTblFldHeapCnt */

/******************************************************************************/

RET_CODE	 p_shTblFldSignFlip
   (
   TYPE		 schemaType,		/* IN:    Input   schema (data) type  */
   TYPE		*schemaTypeFlip,	/* OUT:   Flipped schema (data) type  */
   double	*TSCAL,			/* OUT:   Resultant TSCALn value      */
   double	*TZERO			/* OUT:   Resultant TZEROn value      */
   )

/*++
 * ROUTINE:	 p_shTblFldSignFlip
 *
 *	Return information about changing the "signedness" of a data type.  This
 *	is done only for particular object schemas.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *			The input data type can have its signedness changed.
 *
 *   SH_INVOBJ		Failure.
 *			The input data type cannot have its signedness changed.
 *			The output arguments are not set.
 *
 * SIDE EFFECTS:
 *
 *   o	LONG/ULONG integers are permitted to flip only if they are 4 bytes (as
 *	that's the maximum integer size supported by FITS).
 *
 * SYSTEM CONCERNS:
 *
 *   o	p_shFitsPlatformCheck should have been called to guarantee data size
 *	assumptions.
 *--
 ******************************************************************************/

   {	/* p_shTblFldSignFlip */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
   SCHEMA		*lcl_schemaLONG = ((SCHEMA *)shSchemaGet ("LONG"));

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Only integers can have their signedness changed.
    *
    *   o   Only integers supported by the FITS Standard are handled.
    */

#define	s_FlipType(flipType, zero) \
	  *schemaTypeFlip = shTypeGetFromName (flipType); *TZERO = zero; *TSCAL = 1.0

        if                                 (schemaType == shTypeGetFromName ( "CHAR"))  { s_FlipType ("UCHAR",         128.0); }
   else if                                 (schemaType == shTypeGetFromName ("SCHAR"))  { s_FlipType ("UCHAR",         128.0); }
   else if                                 (schemaType == shTypeGetFromName ("UCHAR"))  { s_FlipType ("SCHAR",         128.0); }
   else if                                 (schemaType == shTypeGetFromName ( "SHORT")) { s_FlipType ("USHORT",      32768.0); }
   else if                                 (schemaType == shTypeGetFromName ("USHORT")) { s_FlipType ( "SHORT",      32768.0); }
   else if                                 (schemaType == shTypeGetFromName ( "INT"))   { s_FlipType ("UINT",   2147483648.0); }
   else if                                 (schemaType == shTypeGetFromName ("UINT"))   { s_FlipType ( "INT",   2147483648.0); }
   else if ((lcl_schemaLONG->size == 4) && (schemaType == shTypeGetFromName ( "LONG"))) { s_FlipType ("ULONG",  2147483648.0); }
   else if ((lcl_schemaLONG->size == 4) && (schemaType == shTypeGetFromName ("ULONG"))) { s_FlipType ( "LONG",  2147483648.0); }
   else
      {
      lcl_status = SH_INVOBJ;
      }

   return (lcl_status);

#undef	s_FlipType

   }	/* p_shTblFldSignFlip */

/******************************************************************************/

TBLCOL		*  shTblcolNew
   (
   void
   )

/*++
 * ROUTINE:	   shTblcolNew
 *
 *	Constructor function used by object schema (for TBLCOL typedef).
 *
 * RETURN VALUES:
 *
 *   TBLCOL *	The address of the TBLCOL object just constructed.
 *
 *   0		The constructor function failed.
 *
 * SIDE EFFECTS:	Sets the file-static g_tblfld_type, used to avoid
 *                      many many calls to shTypeGetFromName. This isn't really
 *                      the right place for it, but putting it here avoids
 *                      creating a new init function
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/*   shTblcolNew */

   /*
    * Declarations.
    */

   TBLCOL		*lcl_tblCol;


   if(g_tblfld_type == UNKNOWN) {
      g_tblfld_type = shTypeGetFromName("TBLFLD");
   }
   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Malloc space for the TBLCOL structure along with space for the FITS header
    * vector used by LIBFITS.
    */

   if ((lcl_tblCol = ((TBLCOL *)shMalloc (sizeof (TBLCOL)))) == ((TBLCOL *)0))
      {
      goto rtn_return;
      }

   memset (((void *)lcl_tblCol), 0, sizeof (TBLCOL));	/* Clear struct out   */

   p_shHdrMallocForVec (&lcl_tblCol->hdr);

   if (lcl_tblCol->hdr.hdrVec == ((HDR_VEC *)0))
      {
      shFree (lcl_tblCol);
              lcl_tblCol = ((TBLCOL *)0);
      goto rtn_return;
      }

   lcl_tblCol->hdr.modCnt = 0;			/* Not modified yet           */

   /* Just initialize the chain as it's already part of the tblCol structure */
   p_shChainInit(&(lcl_tblCol->fld), "ARRAY");

   /*
    * All done.
    */

rtn_return : ;

   return (lcl_tblCol);

   }	/*   shTblcolNew */

/******************************************************************************/

void		   shTblcolDel
   (
   TBLCOL	*tblCol			/* IN:    Table to destroy            */
   )

/*++
 * ROUTINE:	   shTblcolDel
 *
 *	Destructor function for object schema (for TBLCOL typedef).
 *
 * RETURN VALUES:	None
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/*   shTblcolDel */

   if (tblCol != ((TBLCOL *)0))
      {
      p_shHdrFreeForVec (&tblCol->hdr);
      while (p_shTblFldDelByIdx (&(tblCol->fld), 0) == SH_SUCCESS) {}
      p_chainElementDel(&(tblCol->fld));                /* Free only the elems*/
      memset (((void *)tblCol), 0, sizeof (TBLCOL));	/* For safety         */
      shFree (tblCol);
      }

   }	/*   shTblcolDel */

/******************************************************************************/

TBLFLD		*  shTblfldNew
   (
   void
   )

/*++
 * ROUTINE:	   shTblfldNew
 *
 *	Constructor function used by object schema (for TBLFLD typedef).
 *
 * RETURN VALUES:
 *
 *   TBLFLD *	The address of the TBLFLD object just constructed.
 *
 *   0		The constructor function failed.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/*   shTblfldNew */

   /*
    * Declarations.
    */

   TBLFLD		*lcl_tblFld = ((TBLFLD *)0);

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Malloc space for the TBLFLD structure.
    */

   if ((lcl_tblFld = ((TBLFLD *)shMalloc (sizeof (TBLFLD)))) == ((TBLFLD *)0))
      {
      goto rtn_return;
      }

   memset (((void *)lcl_tblFld), 0, sizeof (TBLFLD));	/* Clear struct out   */

   /*
    * All done.
    */

rtn_return : ;

   return (lcl_tblFld);

   }	/*   shTblfldNew */

/******************************************************************************/

void		   shTblfldDel
   (
   TBLFLD	*tblFld			/* IN:    Table field to destroy      */
   )

/*++
 * ROUTINE:	   shTblfldDel
 *
 *	Destructor function for object schema (for TBLFLD typedef).  The ARRAY
 *	structure and the data associated with it are   N O T   deleted.
 *
 * RETURN VALUES:	None
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	Heap storage is assumed to be owned by the TBLFLD.  It   M U S T   have
 *	been allocated with shMalloc () as shFree () is used to release it.
 *--
 ******************************************************************************/

   {	/*   shTblfldDel */

   if (tblFld != ((TBLFLD *)0))
      {
      if (tblFld->heap.dataPtr != ((unsigned char *)0))
         {
         shFree (tblFld->heap.dataPtr);			/* Free heap storage  */
         }

      memset (((void *)tblFld), 0, sizeof (TBLFLD));	/* For safety         */
      shFree (tblFld);
      }

   }	/*   shTblfldDel */

/******************************************************************************/

static RET_CODE	 s_shTblTBLFLDreadAkey
   (
   HDR_VEC		 *hdrVec,	/* IN:    Header vector for HDU       */
   TBLFLD		 *tblFld,	/* INOUT: Table field descriptor      */
                 int	  fldIdx,	/* IN:    Field index (0-indexed)     */
   TBLFLD_KEYWORDS	  tblKeyFld,	/* IN:    Mask indicating which field */
                 char	 *keyName,	/* IN:    Unindexed keyword name      */
                 char	 *keyVal,	/* OUT:   Keyword value               */
                 int	  dataTypeOk	/* IN:    Keyword & data type compat. */
   )

/*++
 * ROUTINE:	 s_shTblTBLFLDreadAkey
 *
 *	If the indexed keyword (keyName + (fldIdx + 1)) is present, read the
 *	character string value into the TBLFLD structure and indicate its
 *	presence (tblKeyFld).  Trailing blanks are removed.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_NOT_SUPPORTED	Failure.
 *			The unindexed keyword is too long to permit the proper
 *			generation of a legal indexed keyword.
 *
 *   SH_INV_FITS_FILE	Failure.
 *			The keyword had an invalid value.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	The TFORMn keyword   M U S T   have already been parsed and be legal.
 *	The tblFld->TFORM structure must have been filled out properly.
 *--
 ******************************************************************************/

   {	/* s_shTblTBLFLDreadAkey */

   /*
    * Declarations.
    */

   RET_CODE		  lcl_status = SH_SUCCESS;
                  char	  lcl_keyword[11+1];	/* FITS keyword(only need 8+1)*/
                  int	  lcl_keyValIdx;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   if (strlen (keyName) > 5)
      {
      lcl_status = SH_NOT_SUPPORTED;
      shErrStackPush ("%s is too long to be formed into an indexed FITS keyword", keyName);
      goto rtn_return;
      }

   sprintf (lcl_keyword, "%s%d", keyName, fldIdx + 1);	/* Gen indexed keyword*/

   if (f_lnum (hdrVec, lcl_keyword) == -1)
      {
      *keyVal = ((char)0);
      }
   else
      {
      if (!dataTypeOk)			/* Check if keyword can be used with  */
         {				/* ... the particular data type.      */
         lcl_status = SH_INV_FITS_FILE;
         shErrStackPush ("%s keyword cannot be used with data type '%c'", lcl_keyword, tblFld->prvt.dataType);
         goto rtn_return;
         }
      if (!f_akey (keyVal, hdrVec, lcl_keyword))
         {
         lcl_status = SH_INV_FITS_FILE;
         shErrStackPush ("%s keyword invalid", lcl_keyword);
         goto rtn_return;
         }
      shAddSet (tblFld->Tpres, tblKeyFld);	/* Show keyword is present    */

      /*
       * Remove trailing blanks.
       */

      for (lcl_keyValIdx = strlen (keyVal) - 1; (lcl_keyValIdx >= 0); lcl_keyValIdx--)
         {
         if (keyVal[lcl_keyValIdx] != ' ')
            {
            break;				/* All   trailing blanks gone */
            }
             keyVal[lcl_keyValIdx]  = ((char)0); /* Drop trailing blank       */
         }

      }

   /*
    * All done.
    */

rtn_return : ;

   return (lcl_status);

   }	/* s_shTblTBLFLDreadAkey */

/******************************************************************************/

static RET_CODE	 s_shTblTBLFLDreadIkey
   (
   HDR_VEC		 *hdrVec,	/* IN:    Header vector for HDU       */
   TBLFLD		 *tblFld,	/* INOUT: Table field descriptor      */
                 int	  fldIdx,	/* IN:    Field index (0-indexed)     */
   TBLFLD_KEYWORDS	  tblKeyFld,	/* IN:    Mask indicating which field */
                 char	 *keyName,	/* IN:    Unindexed keyword name      */
                 int	 *keyVal,	/* OUT:   Keyword value               */
                 int	  dataTypeOk	/* IN:    Keyword & data type compat. */
   )

/*++
 * ROUTINE:	 s_shTblTBLFLDreadIkey
 *
 *	If the indexed keyword (keyName + (fldIdx + 1)) is present, read the
 *	integer into the TBLFLD structure and indicate its presence (tblKeyFld).
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_NOT_SUPPORTED	Failure.
 *			The unindexed keyword is too long to permit the proper
 *			generation of a legal indexed keyword.
 *
 *   SH_INV_FITS_FILE	Failure.
 *			The keyword had an invalid value.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* s_shTblTBLFLDreadIkey */

   /*
    * Declarations.
    */

   RET_CODE		  lcl_status = SH_SUCCESS;
                  char	  lcl_keyword[11+1];	/* FITS keyword(only need 8+1)*/

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   if (strlen (keyName) > 5)
      {
      lcl_status = SH_NOT_SUPPORTED;
      shErrStackPush ("%s is too long to be formed into an indexed FITS keyword", keyName);
      goto rtn_return;
      }

   sprintf (lcl_keyword, "%s%d", keyName, fldIdx + 1);	/* Gen indexed keyword*/

   if (f_lnum (hdrVec, lcl_keyword) == -1)
      {
      *keyVal = 0;
      }
   else
      {
      if (!dataTypeOk)			/* Check if keyword can be used with  */
         {				/* ... the particular data type.      */
         lcl_status = SH_INV_FITS_FILE;
         shErrStackPush ("%s keyword cannot be used with data type '%c'", lcl_keyword, tblFld->prvt.dataType);
         goto rtn_return;
         }
      if (!f_ikey (keyVal, hdrVec, lcl_keyword))
         {
         lcl_status = SH_INV_FITS_FILE;
         shErrStackPush ("%s keyword invalid", lcl_keyword);
         goto rtn_return;
         }
      shAddSet (tblFld->Tpres, tblKeyFld);	/* Show keyword is present    */
      }

   /*
    * All done.
    */

rtn_return : ;

   return (lcl_status);

   }	/* s_shTblTBLFLDreadIkey */

/******************************************************************************/

static RET_CODE	 s_shTblTBLFLDreadDkey
   (
   HDR_VEC		 *hdrVec,	/* IN:    Header vector for HDU       */
   TBLFLD		 *tblFld,	/* INOUT: Table field descriptor      */
                 int	  fldIdx,	/* IN:    Field index (0-indexed)     */
   TBLFLD_KEYWORDS	  tblKeyFld,	/* IN:    Mask indicating which field */
                 char	 *keyName,	/* IN:    Unindexed keyword name      */
   double		 *keyVal,	/* OUT:   Keyword value               */
                 int	  dataTypeOk	/* IN:    Keyword & data type compat. */
   )

/*++
 * ROUTINE:	 s_shTblTBLFLDreadDkey
 *
 *	If the indexed keyword (keyName + (fldIdx + 1)) is present, read the
 *	double floating point number into the TBLFLD structure and indicate
 *	its presence (tblKeyFld).
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_NOT_SUPPORTED	Failure.
 *			The unindexed keyword is too long to permit the proper
 *			generation of a legal indexed keyword.
 *
 *   SH_INV_FITS_FILE	Failure.
 *			The keyword had an invalid value.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* s_shTblTBLFLDreadDkey */

   /*
    * Declarations.
    */

   RET_CODE		  lcl_status = SH_SUCCESS;
                  char	  lcl_keyword[11+1];	/* FITS keyword(only need 8+1)*/

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   if (strlen (keyName) > 5)
      {
      lcl_status = SH_NOT_SUPPORTED;
      shErrStackPush ("%s is too long to be formed into an indexed FITS keyword", keyName);
      goto rtn_return;
      }

   sprintf (lcl_keyword, "%s%d", keyName, fldIdx + 1);	/* Gen indexed keyword*/

   if (f_lnum (hdrVec, lcl_keyword) == -1)
      {
      *keyVal = 0.0;
      }
   else
      {
      if (!dataTypeOk)			/* Check if keyword can be used with  */
         {				/* ... the particular data type.      */
         lcl_status = SH_INV_FITS_FILE;
         shErrStackPush ("%s keyword cannot be used with data type '%c'", lcl_keyword, tblFld->prvt.dataType);
         goto rtn_return;
         }
      if (!f_dkey (keyVal, hdrVec, lcl_keyword))
         {
         lcl_status = SH_INV_FITS_FILE;
         shErrStackPush ("%s keyword invalid", lcl_keyword);
         goto rtn_return;
         }
      shAddSet (tblFld->Tpres, tblKeyFld);	/* Show keyword is present    */
      }

   /*
    * All done.
    */

rtn_return : ;

   return (lcl_status);

   }	/* s_shTblTBLFLDreadDkey */

/******************************************************************************/

#if 0
static RET_CODE	 s_shTblTBLFLDreadLkey
   (
   HDR_VEC		 *hdrVec,	/* IN:    Header vector for HDU       */
   TBLFLD		 *tblFld,	/* INOUT: Table field descriptor      */
                 int	  fldIdx,	/* IN:    Field index (0-indexed)     */
   TBLFLD_KEYWORDS	  tblKeyFld,	/* IN:    Mask indicating which field */
                 char	 *keyName,	/* IN:    Unindexed keyword name      */
                 int	 *keyVal,	/* OUT:   Keyword value               */
                 int	  dataTypeOk	/* IN:    Keyword & data type compat. */
   )

/*++
 * ROUTINE:	 s_shTblTBLFLDreadLkey
 *
 *	If the indexed keyword (keyName + (fldIdx + 1)) is present, read the
 *	logical value into the TBLFLD structure and indicate its presence
 *	(tblKeyFld).
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_NOT_SUPPORTED	Failure.
 *			The unindexed keyword is too long to permit the proper
 *			generation of a legal indexed keyword.
 *
 *   SH_INV_FITS_FILE	Failure.
 *			The keyword had an invalid value.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* s_shTblTBLFLDreadLkey */

   /*
    * Declarations.
    */

   RET_CODE		  lcl_status = SH_SUCCESS;
                  char	  lcl_keyword[11+1];	/* FITS keyword(only need 8+1)*/

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   if (strlen (keyName) > 5)
      {
      lcl_status = SH_NOT_SUPPORTED;
      shErrStackPush ("%s is too long to be formed into an indexed FITS keyword", keyName);
      goto rtn_return;
      }

   sprintf (lcl_keyword, "%s%d", keyName, fldIdx + 1);	/* Gen indexed keyword*/

   if (f_lnum (hdrVec, lcl_keyword) == -1)
      {
      *keyVal = 0;
      }
   else
      {
      if (!dataTypeOk)			/* Check if keyword can be used with  */
         {				/* ... the particular data type.      */
         lcl_status = SH_INV_FITS_FILE;
         shErrStackPush ("%s keyword cannot be used with data type '%c'", lcl_keyword, tblFld->prvt.dataType);
         goto rtn_return;
         }
      if (!f_lkey (keyVal, hdrVec, lcl_keyword))
         {
         lcl_status = SH_INV_FITS_FILE;
         shErrStackPush ("%s keyword invalid", lcl_keyword);
         goto rtn_return;
         }
      shAddSet (tblFld->Tpres, tblKeyFld);	/* Show keyword is present    */
      }

   /*
    * All done.
    */

rtn_return : ;

   return (lcl_status);

   }	/* s_shTblTBLFLDreadLkey */
#endif

/******************************************************************************/

static RET_CODE	 s_shTblFldParseTFORMbinDataType
   (
                  char	  dataType,	/* IN:    TFORMn  data type           */
   struct shTbl_SuppData *dataTypeMap,	/* IN:    Supported data types table  */
                  int	 *dataTypeIdx,	/* OUT:   Index to data type map      */
   ARRAY_DATA		 *dataInfo	/* OUT:   Data type information       */
   )

/*++
 * ROUTINE:	 s_shTblFldParseTFORMbinDataType
 *
 *	Convert a TFORMn data type from a Binary Table into its corresponding
 *	object schema type.
 *
 * RETURNS:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_INV_FITS_FILE	Failure.
 *			The TFORMn data type is unknown or not supported.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* s_shTblFldParseTFORMbinDataType */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
                  int	 lcl_dataTypeIdx = -1;
   SHALIGN_TYPE		 lcl_dataType;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   for (lcl_dataTypeIdx = 0; (   (dataTypeMap[lcl_dataTypeIdx].dataType != '\000')
                              && (dataTypeMap[lcl_dataTypeIdx].dataType != dataType));
        lcl_dataTypeIdx++) {}

   if (dataTypeMap[lcl_dataTypeIdx].dataType == '\000')
      {
      lcl_status = SH_INV_FITS_FILE;
      goto rtn_return;
      }

   lcl_dataType = dataTypeMap[lcl_dataTypeIdx].fldType;

   switch (dataType)
      {
      case 'P' : dataInfo->schemaType = shTypeGetFromName ("TBLHEAPDSC");
                 dataInfo->size       = sizeof (TBLHEAPDSC);
                 dataInfo->incr       = sizeof (TBLHEAPDSC);		/*### */
                 dataInfo->align      = shAlignment.type[lcl_dataType].align;
                 break;
      case 'L' : dataInfo->schemaType = shTypeGetFromName ("LOGICAL");
                 dataInfo->size       = shAlignment.type[lcl_dataType].size;
                 dataInfo->incr       = shAlignment.type[lcl_dataType].incr;
                 dataInfo->align      = shAlignment.type[lcl_dataType].align;
                 break;
      case 'A' : dataInfo->schemaType = shTypeGetFromName ("STR");
                 dataInfo->size       = shAlignment.type[lcl_dataType].size;
                 dataInfo->incr       = shAlignment.type[lcl_dataType].incr;
                 dataInfo->align      = shAlignment.type[lcl_dataType].align;
                 break;
      default  : dataInfo->schemaType = shAlignAlignToSchemaMap (lcl_dataType);
                 dataInfo->size       = shAlignment.type[lcl_dataType].size;
                 dataInfo->incr       = shAlignment.type[lcl_dataType].incr;
                 dataInfo->align      = shAlignment.type[lcl_dataType].align;
                 break;
      }

   /*
    * All done.
    */

rtn_return : ;

   if (dataTypeIdx != ((int *)0)) { *dataTypeIdx = lcl_dataTypeIdx; }

   return (lcl_status);

   }	/* s_shTblFldParseTFORMbinDataType */

/******************************************************************************/
