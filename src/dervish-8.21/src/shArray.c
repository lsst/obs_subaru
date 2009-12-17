/*++
 * FACILITY:	Dervish
 *
 * ABSTRACT:	Routines to manipulate Arrays.
 *
 *   Public
 *   --------------------------	------------------------------------------------
 *     shArrayDataAlloc		Allocate space for array data and pointers.
 *     shArrayDataFree		Free     space for array data and pointers.
 *
 *   Private
 *   --------------------------	------------------------------------------------
 *   p_shArrayLink		Link up a multidimension array of ptrs to data.
 *     shArrayNew		Object schema constructor for ARRAY.
 *     shArrayDel		Object schema destructor  for ARRAY.
 *
 * ENVIRONMENT:	ANSI C.
 *		shArray.c
 *
 * AUTHOR: 	Tom Nicinski, Creation date: 14-Feb-1994
 *--
 ******************************************************************************/

/******************************************************************************/
/*						.----------------------------.
 * Outstanding Issues / Problems / Etc.		|  Last Update: 14-Feb-1994  |
 *						`----------------------------'
 *
 * This section lists out all the current problems with FITS processing and any
 * suggested fixes and enhancements.  Please keep this section   C U R R E N T.
 *
 *- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * Routine:	shArrayDataAlloc
 *
 * Problem:	If data space already exists for the ARRAY, it is first freed.
 *		This can cause memory fragmentation and can be an inefficient
 *		use of time.
 *
 * Solution:	It may be useful to store the size of the data area (including
 *		the hierarchy of array pointers) in the ARRAY structure.  This
 *		would allow one to reuse existing space if adequate in size.
 *
 *- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 ******************************************************************************/

/******************************************************************************/
/*
 * Global declarations.
 */

#include	<string.h>

#include	"dervish_msg_c.h"
#include	"shCSchema.h"
#include	"shCUtils.h"
#include        "shCGarbage.h"
#include	"shCAlign.h"
#include	"shCErrStack.h"
#include	"shCAssert.h"

#include	"shCArray.h"

/******************************************************************************/

RET_CODE	   shArrayDataAlloc
   (
   ARRAY		*array,		/* INOUT: Array to alloc space for    */
                  char	*schemaName	/* IN:    Object schema name          */
   )

/*++
 * ROUTINE:	   shArrayDataAlloc
 *
 *	Allocate space for the data and the hierarchy of array pointers.  The
 *	amount of space needed is based on the object schema type.
 *
 *	The number of ARRAY dimensions and the dimensions themselves must have
 *	been filled in prior to this call.  Additionally, for an unpassed
 *	schemaName, or a schema name of "UNKNOWN", the size, alignment, and
 *	address increments for the data must have been filled in also.
 *
 *	If the data indirection (which must be filled in prior to this call),
 *	nStar, is non-zero, space is allocated for pointers to data, rather
 *	than the data type itself.  The hierarchy of array pointers will then
 *	reference the pointers in the actual ARRAY data area.
 *
 *	The sizes of array subarrays is also filled in subArrCnt.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 *   SH_NOT_SUPPORTED	Failure.
 *			The object schema name is unknown.  No data space was
 *			allocated.
 *
 *   SH_MALLOC_ERR	Failure.
 *			Space could not be allocated for the data and/or the
 *			hierarchy of array pointers.
 *
 * SIDE EFFECTS:
 *
 *   o	If the data area already has space allocated to it, it is freed first.
 *
 *   o	If the object schema is not a type recognized by shAlign, then
 *
 *	   o	the size of the object schema is used as the best guess for
 *		the address increment.  This should be a perfectly valid
 *		assumption, since sizeof () (according to the ANSI C standard)
 *		returns the size of the object plus any padding before and/or
 *		after the object.  It's assumed that the object schema (SCHEMA)
 *		size is set this way.
 *
 *	   o	the worst case (largest) alignment factor is used.
 *
 * SYSTEM CONCERNS:
 *
 *   o	The following ARRAY fields   M U S T   have been filled in:
 *
 *	   dimCnt		The number of dimensions
 *	   dim			The dimensions of the array.
 *
 *   o	If the object schema name is "UNKNOWN", the following ARRAY fields
 *	M U S T   have been filled in prior to this call:
 *
 *	   data.size		The size of a datum in bytes.
 *	   data.align		The alignment factor for the first byte of data.
 *	   data.incr		The address increment between datums.
 *
 *   o	shArrayNew () must have been used to guarantee that memset can be used
 *	to properly zero out most data types.
 *--
 ******************************************************************************/

   {	/*   shArrayDataAlloc */

   /*
    * Declarations.
    */

   RET_CODE		 lcl_status = SH_SUCCESS;
   unsigned long  int	 lcl_ptrSize;		/* Size (bytes) of ptr  area  */
   unsigned long  int	 lcl_dataSize;		/* Size (bytes) of data area  */
                  int	 lcl_dimIdx;
                  int	 lcl_dimIdxEnd;
                  int	 lcl_dimProd;		/* Product of dimensions      */
   const SCHEMA		*lcl_schema;
   SHALIGN_TYPE		 lcl_alignType;
static            char	*lcl_schemaUNKNOWN = "UNKNOWN";

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Free any existing space (since we can't determine its size, we can't
    * reuse it).
    *
    *   o   This step is done early so that this routine's behaviour is consis-
    *       tent.  By using this routine, the caller is prepared to have any
    *       existing data lost.
    */

   shArrayDataFree (array);

   /*
    * Fill in fields based on the object schema type.
    *
    *   o   If the object schema name is "UNKNOWN", then data.size, data.align,
    *       and data.incr   M U S T   have been filled in prior to this call.
    *
    *   o   If the object schema is not a type recognized by shAlign, then
    *
    *          o   the size of the object schema is used as the best guess for
    *              the address increment.  This should be a perfectly valid
    *              assumption, since sizeof () (according to the ANSI C stan-
    *              dard) returns the size of the object plus any padding before
    *              and/or after the object.  It's assumed that the object schema
    *              (SCHEMA) size is set this way.
    *
    *          o   the worst case (largest) alignment factor is used.
    */

   if (schemaName != ((char *)0))
      {
      if ((array->data.schemaType = shTypeGetFromName (schemaName)) == shTypeGetFromName (lcl_schemaUNKNOWN))
         {
         if (strcmp (schemaName, lcl_schemaUNKNOWN) != 0)
            {
            lcl_status = SH_NOT_SUPPORTED;
            shErrStackPush ("Object schema %s is not supported", schemaName);
            goto rtn_return;
            }
         }
      else
         {
         if ((lcl_alignType = shAlignSchemaToAlignMap (array->data.schemaType)) == SHALIGNTYPE_unknown)
            {
            if (array->data.schemaType == shTypeGetFromName ("STR"))
               {
               lcl_alignType = SHALIGNTYPE_S8;	/* Fudge string to character  */
               }
            }

         if (lcl_alignType != SHALIGNTYPE_unknown)
            {
            /*
             * Primitive data type.
             */

            array->data.size  = shAlignment.type[lcl_alignType].size;
            array->data.align = shAlignment.type[lcl_alignType].align;
            array->data.incr  = shAlignment.type[lcl_alignType].incr;
            }
         else
            {
            if ((lcl_schema = shSchemaGet (schemaName)) == ((SCHEMA *)0))
               {
               lcl_status = SH_NOT_SUPPORTED;
               shErrStackPush ("Object schema %s is not supported", schemaName);
               goto rtn_return;
               }
            array->data.size  =			/* Best assumption is the size*/
            array->data.incr  = lcl_schema->size;
            array->data.align = shAlignment.alignMax; /* Get largest alignment*/
            }
         }
      }

   /*
    * Space is allocated for the hierarchical set of pointers (to permit C array
    * referencing notation without knowing array bounds) and the data itself.
    * The number of pointers and data elements needed is:
    *
    *          array.dimCnt-2
    *              .---. ____j
    *    ptrArea:   \     ||
    *               /     ||   array.dim[i]
    *              '---'    i=0
    *               j=0
    *
    *              ____array.dimCnt-1
    *    dataArea:  ||
    *               ||   array.dim[i]
    *                 i=0
    *
    * If necessary, some space is allocated for alignment between the pointers
    * and data area.
    */

   lcl_dataSize = 1;
   lcl_ptrSize  = 0;

   for (lcl_dimIdx  = 0, lcl_dimIdxEnd = (array->dimCnt - 2), lcl_dimProd = 1;
        lcl_dimIdx <=    lcl_dimIdxEnd;   lcl_dimIdx++)
      {
      lcl_dataSize *= array->dim[lcl_dimIdx];
      lcl_dimProd  *= array->dim[lcl_dimIdx];
      lcl_ptrSize  += lcl_dimProd;
      }
   lcl_dataSize *= (array->nStar == 0) ? array->dim[lcl_dimIdx] * array->data.size
                                       : sizeof (char *);
   lcl_ptrSize  *= sizeof (char *);		/* Ptr count to byte count    */

   /*
    * At level l, there are dim[l] subarrays.  The size of each subarray is
    *
    *                  ____array.dimCnt-1
    *    subArrCnt[l]:  ||
    *                   ||     array.dim[j]
    *                     j=l+1
    *
    * By definition, subArrCnt[array.dimCnt-1] is 1.
    */

      array->subArrCnt[array->dimCnt - 1] = 1;	/* By definition              */
   for (lcl_dimIdx =   array->dimCnt - 1; lcl_dimIdx > 0; lcl_dimIdx--)
      {
      array->subArrCnt[lcl_dimIdx-1] = array->subArrCnt[lcl_dimIdx] * array->dim[lcl_dimIdx];
      }

   /*
    * Allocate the space for this field.
    *
    *   o   Any alignment space between the pointer and data areas is added
    *       to the pointer area size (keeps things simple).  But, it does
    *       assume that malloc (shMalloc) will allocate storage with proper
    *       aligment so that any data type on the machine can be stored at
    *       the start of the allocated space).
    *
    *   o   Data space is zeroed out (this is a performance hit).
    */

   lcl_ptrSize   = (array->nStar == 0) ? p_shAlignUp (lcl_ptrSize, array->data.align)
                                       : p_shAlignUp (lcl_ptrSize, shAlignment.type[SHALIGNTYPE_ptrData].align);

   if (lcl_dataSize == 0)
      {
      array->arrayPtr     = ((void          *)0);	/* For safety         */
      array->data.dataPtr = ((unsigned char *)0);	/* For safety         */
      }
   else
      {
      /*
       * NOTE: shMalloc () aborts with shFatal () if the allocation fails.
       */

      array->arrayPtr = shMalloc (lcl_ptrSize + lcl_dataSize);
      array->data.dataPtr = ((unsigned char *)array->arrayPtr) + lcl_ptrSize;
      memset (((void *)array->data.dataPtr), 0, lcl_dataSize);	/* Zero out data      */
      p_shArrayLink (array->dimCnt, array->dim, array->arrayPtr, array->data.dataPtr,
                    (array->nStar == 0) ? array->data.incr : shAlignment.type[SHALIGNTYPE_ptrData].incr);
      }

rtn_return : ;

   return (lcl_status);

   }	/*   shArrayDataAlloc */

/******************************************************************************/

RET_CODE	   shArrayDataFree
   (
   ARRAY		*array		/* INOUT: Array to deallocate from    */
   )

/*++
 * ROUTINE:	   shArrayDataFree
 *
 *	Free up the data and hierarchy of array pointers space associated with
 *	an ARRAY.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *			The data and array pointers were freed.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	The data and array pointers are assumed to be contiguous, thus only one
 *	free operation is performed.
 *
 *   o	The data and array pointers   M U S T   have been allocated with
 *	shMalloc () as they are released with shFree ().
 *--
 ******************************************************************************/

   {	/*   shArrayDataFree */

   if (array != ((ARRAY *)0))
      {
      if (       array->arrayPtr     != ((void *)0))
         {
         shFree (array->arrayPtr);			/* Free ptr/data area */
                 array->arrayPtr      = ((void *)0);	/* For safety         */
         }
      else if   (array->data.dataPtr != ((unsigned char *)0))
         {
         shFree (array->data.dataPtr);
                 array->data.dataPtr  = ((unsigned char *)0); /* For safety       */
         }
      }

   return (SH_SUCCESS);

   }	/*   shArrayDataFree */

/******************************************************************************/

RET_CODE	 p_shArrayLink
   (
                  int	 nDim,		/* IN:    Number of dimensions        */
                  int	 dim[],		/* IN:    Dimensions                  */
   void			*ptrArea,	/* OUT:   Ptrs to link together       */
   unsigned       char	*dataArea,	/* IN:    Data area to link to        */
   unsigned       int	 dataIncr	/* IN:    Distance between data elems */
   )

/*++
 * ROUTINE:	 p_shArrayLink
 *
 *	Link together a set of pointers and pointers to data to allow dynamic
 *	access to a multidimension array without knowing its bounds beforehand.
 *
 * RETURN VALUES:
 *
 *   SH_SUCCESS		Success.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	The ptrArea and dataArea must be large enough to contain all pointers
 *	and data elements.  Minimum sizes are:
 *
 *                    nDim-2
 *                    .---. ____j
 *          ptrArea:   \     ||
 *                     /     ||   dim[i]
 *                    '---'    i=0
 *                     j=0
 *
 *                    ____nDim-1
 *          dataArea:  ||
 *                     ||   dim[i]
 *                       i=0
 *
 *	These counts are then multiplied by the data pointer size and data size
 *	(dataIncr) respectively.
 *
 *   o	It's assumed that the pointer and data areas are properly aligned for
 *	their respective types (pointer to data and the data type itself).
 *
 *   o	It's assumed that all pointers to data have the same size.  Therefore,
 *	pointer to unsigned char is used to determine that size.
 *--
 ******************************************************************************/

   {	/* p_shArrayLink */

   /*
    * Declarations.
    */

   unsigned       char	**lcl_dstPtr;	/* Destination pointer index          */
   unsigned long  int	  lcl_curPtrIdx;/* Current ptr index (one gettin link)*/
   unsigned long  int	  lcl_curPtrCnt;/* Pointer count at current depth     */
   unsigned long  int	  lcl_curIdx;
                  int	  lcl_curDepth;	/* Current dimension (0-indexed)      */

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    *   Depth                         .-dim[0]-.
    *   -----                         |        |
    *                                 +-+-//-+-+
    *     0                           |o|    |o|
    *                                 +|+-//-+|+
    *                          .-------'      `-.
    *                          V                V
    *                         +-+-//-+-+       +-+-//-+-+
    *     1                   |o|    |o|  ...  |o|    |o|
    *                         +|+-//-+|+       +-+-//-+-+
    *         .----------------'      |         |      `---------.
    *         |                .------'         |                |
    *         V                V                V                V
    *        +-+-//-+-+       +-+-//-+-+       +-+-//-+-+       +-+-//-+-+
    *     2  |o|    |o|  ...  |o|    |o|  ...  |o|    |o|  ...  |o|    |o|
    *        +|+-//-+|+       +|+-//-+|+       +|+-//-+|+       +|+-//-+|+
    *         .              .        .              .         .
    *         :              :        :              :         :
    *         V              V        V              V         V
    *        +-+-+--//--+-+-+---//---+-+-+--//--+-+-+---//----+-+-+--//--+-+-+
    * nDim-1 | | |      | | |        | | |      | | |         | | |      | | |
    *        +-+-+--//--+-+-+---//---+-+-+--//--+-+-+---//----+-+-+--//--+-+-+
    *        |              |        |              |         |              |
    *        `-dim[nDim-1]--'        `-dim[nDim-1]--'         `-dim[nDim-1]--'
    *
    * All pointers are contiguous, with the top level at the lowest addresses
    * (indices) and the leaf pointers (at depth nDim-2) at the highest addresses
    * (indices).  At a particular depth, d (0-indexed), the offset (0-indexed)
    * to the first pointer and the number of pointers at the depth are:
    *
    *       d-1
    *      .--. ____i			____d
    *       \    ||			 ||
    *       /    ||   dim[j]		 ||    dim[k]
    *      '--'    j=0			   k=0
    *       i=0
    *
    * Link up pointers to each other.  The last set of pointers are handled
    * differently, since they link up to data.
    */

   lcl_dstPtr    = &((unsigned char **)ptrArea)[dim[0]];
   lcl_curPtrIdx = 0;
   lcl_curPtrCnt = 1;

   for (lcl_curDepth = 0; lcl_curDepth < (nDim - 2); lcl_curDepth++)
      {
      lcl_curPtrCnt *= dim[lcl_curDepth];	/* Number ptrs at this level  */
      for (lcl_curIdx = 0; lcl_curIdx < lcl_curPtrCnt; lcl_curIdx++)
         {
         ((unsigned char ***)ptrArea)[lcl_curPtrIdx++] = lcl_dstPtr;  /* Link to next level down */
                                                         lcl_dstPtr += dim[lcl_curDepth + 1];
         }
      }

   /*
    * Link up the last set of pointers to actual data.
    *
    *   o   With scalars and 1-dimensional arrays, there is no last set of
    *       pointers.
    */

   if (nDim > 1)
      {
      lcl_curPtrCnt *= dim[nDim-2];
      for (lcl_curIdx = 0; lcl_curIdx < lcl_curPtrCnt; lcl_curIdx++)
         {
         ((unsigned char **)ptrArea)[lcl_curPtrIdx++] = dataArea;
                                                        dataArea += (dim[nDim-1] * dataIncr);
         }
      }

   /*
    * All done.
    */

   return (SH_SUCCESS);

   }	/* p_shArrayLink */

/******************************************************************************/

ARRAY		*  shArrayNew
   (
   void
   )

/*++
 * ROUTINE:	   shArrayNew
 *
 *	Constructor function used by object schema (for ARRAY typedef).
 *
 * RETURN VALUES:
 *
 *   ARRAY *	The address of the ARRAY object just constructed.
 *
 *   0		The constructor function failed.
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/*   shArrayNew */

   /*
    * Declarations.
    */

   ARRAY		*lcl_array;
   union {
      unsigned int		 byte[sizeof (double) + sizeof (char *)];
      float			 fpFloat;
      double			 fpDouble;
      char			*ptr;
      }			 lcl_assert;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Malloc space for the ARRAY structure.
    *
    * NOTE: shMalloc () aborts with shFatal if the allocation fails.
    *
    * NOTE: A check is made to ensure that memset can be used to zero out most
    *       data types properly.
    */

   memset (((void *)lcl_assert.byte), 0, sizeof (lcl_assert.byte));
   shAssert (lcl_assert.fpFloat  == ((float)0.0));
   shAssert (lcl_assert.fpDouble == ((double)0.0));
   shAssert (lcl_assert.ptr      == ((char *)0));

   lcl_array = ((ARRAY *)shMalloc (sizeof (ARRAY)));

   memset (((void *)lcl_array), 0, sizeof (ARRAY));	/* Clear struct out   */

   lcl_array->data.schemaType =
   lcl_array->infoType        = shTypeGetFromName ("UNKNOWN");	/* For safety */

   /*
    * All done.
    */

   return (lcl_array);

   }	/*   shArrayNew */

/******************************************************************************/

void		   shArrayDel
   (
   ARRAY	*array			/* IN:    Table to destroy            */
   )

/*++
 * ROUTINE:	   shArrayDel
 *
 *	Destructor function for object schema (for ARRAY typedef).  Any data
 *	associated with the ARRAY structure is also deleted.
 *
 * RETURN VALUES:	None
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	Data associated with the ARRAY   M U S T   have been allocated with
 *	shMalloc (), as it's released with shFree ().
 *
 *   o	Any data maintained outside the ARRAY structure (when nStar is non-zero)
 *	is   N O T   freed.  Therefore, the caller   M U S T   handle that data
 *	prior to calling shArrayDel ();  otherwise, a memory leak is possible.
 *--
 ******************************************************************************/

   {	/*   shArrayDel */

   if (array != ((ARRAY *)0))
      {
      shArrayDataFree (array);

      array->data.schemaType =
      array->infoType        = shTypeGetFromName ("UNKNOWN"); /* Safety       */
      array->info            = ((void *)0);		/* For safety         */
      array->arrayPtr        = ((void *)0);		/* For safety         */
      array->data.dataPtr    = ((unsigned char *)0);	/* For safety         */

      shFree (array);
      }

   }	/*   shArrayDel */

/******************************************************************************/
