/*****************************************************************************
******************************************************************************
**
** FILE:
**	hdr.c
**
** ABSTRACT:
**	This file contains routines that manipulate header information
**	for the "Pure C" layer.
**
** ENTRY POINT		       SCOPE   DESCRIPTION
** -------------------------------------------------------------------------
** p_shHdrFree      	       private Free the header buffers (not the vector)
** p_shHdrMallocForVec         private Allocate space for the header vector
** p_shHdrFreeForVec	       private Free the header vector
** p_shHdrPrint		       private Print header vector to stdout
** shHdrNew                    public  Malloc space for a new header
** shHdrDel                    public  Delete the space used by a header
** shHdrGetAscii	       public  Search header for ASCII value
** shHdrGetDbl                 public  Search header for DOUBLE value
** shHdrGetInt                 public  Search header for INTEGER value
** shHdrGetLogical             public  Search header for LOGICAL value
** shHdrGetLine                public  Search header for keyword and return 
**                                        whole line on which it was found
** shHdrGetLinno               public  Search header for keyword and return 
**                                        line number on which it was found
** shHdrGetLineTotal           public  Return the total number of lines in the
**                                        header.
** shHdrDelByLine              public  Delete indicated line number from header
** shHdrInsertLine             public  Insert a new line in the header
** shHdrReplaceLine            public  Replace an existing line in the header
** shHdrInsertLogical          public  Insert a LOGICAL value in the header
** shHdrInsertDbl              public  Insert a DOUBLE value in the header
** shHdrInsertInt              public  Insert an INTEGER value in the header
** shHdrInsertAscii            public  Insert an ASCII value in the header
** shHdrMakeLineWithAscii      public  Construct a line suitable for inclusion
**                                        in header containing an ASCII value
** shHdrMakeLineWithDbl        public  Construct a line suitable for inclusion
**                                        in header containing a DOUBLE value
** shHdrMakeLineWithInt        public  Construct a line suitable for inclusion
**                                        in header containing an INTEGER value
** shHdrMakeLineWithLogical    public  Construct a line suitable for inclusion
**                                        in header containing a LOGICAL value
** shHdrCopy                   public  Copy a header from one to another
** shHdrInit                   private Initializes the header
** shHdrDelByKeyword           public  Deletes header line w/ keyword specified
**
** p_shHdrCntrIncrement        private Increment header modification count
**
** ENVIRONMENT:
**	ANSI C.
**
** AUTHORS:
**	Creation date:  May 7, 1993
**	   Vijay K. Gurbani
**
** MODIFICATIONS:
** 05/14/1993   VKG   Modified function names according to convention
** 05/20/1993   VKG   Modified shRegHdrInit() to create a header if none exists
** 05/24/1993   VKG   Renamed functions shRegHdrFree, shRegHdrMallocForVec, and
**                       shRegHdrFreeForVec to p_shRegHdrFree, 
**                       p_shRegHdrMallocForVec, and p_shRegHdrFreeForVec. This
**                       implies that these are private functions now.
** 05/24/1993   VKG   Changed name of shRegHdr to shRegHdrAccess
** 05/26/1993   VKG   Modified shRegHdrInit so that it does not populate the
**                       header with default lines
** 05/26/1993   VKG   Made shRegHdrInit private
** 01/17/1994	THN   Converted from REGION-specific to generic header code.
** 03/07/1994   EFB   Changed SHHDR to HDR and SHHDR_VEC HDR_VEC
** 03/08/1994   EFB   Added shHdrNew and shHdrDel
******************************************************************************
******************************************************************************
*/
#include <stdio.h>
#include <string.h>

#include "shCUtils.h"
#include "shCGarbage.h"
#include "region.h"
#include "prvt/region_p.h"

// from ShCFitsIO.h
#define MAXHDRLINE      (5000) /* Maximum number of lines in a header */

/*
 * Functions local to this source file only.
 */
static void p_shHdrCntrIncrement(HDR *a_hdr);


/*============================================================================
 * ROUTINE: p_shHdrCntrIncrement
 *
 * DESCRIPTION:
 *   This routine increments a header modification counter each time the header
 *   is modified.
 *
 * CALL:
 *   (void) p_shHdrCntrIncrement(HDR *a_hdr);
 *          a_hdr - pointer to the header structure
 *============================================================================
 */
static void p_shHdrCntrIncrement(HDR *a_hdr)
{
   if (a_hdr != NULL)
       a_hdr->modCnt += 1;

   return;
}

/*============================================================================
 * ROUTINE: p_shHdrFreeForVec
 *
 * DESCRIPTION:
 *   Routine to de-allocate the base memory for a header vector
 *
 * CALL:
 *   (void) p_shHdrFreeForVec(HDR *a_hdr);
 *          a_hdr    - pointer to the header structure
 *
 * CALLS TO:
 *   shFree()
 *   p_shHdrCntrIncrement()
 *============================================================================
 */
void p_shHdrFreeForVec(HDR *a_hdr)
{
   if (a_hdr->hdrVec != NULL)  {
       p_shHdrFree(a_hdr);
       shFree(a_hdr->hdrVec);
       a_hdr->hdrVec = NULL;
       p_shHdrCntrIncrement(a_hdr);
   }
}

/*============================================================================
 * ROUTINE: p_shHdrFree
 *
 * DESCRIPTION:
 *    Free the header buffer and remove all traces of it from the header
 *    vector.
 *
 * CALL:
 *   (void) p_shHdrFree(HDR *a_hdr);
 *              a_hdr    - pointer to the header structure
 *
 * CALLS TO:
 *   p_shHdrCntrIncrement()
 *============================================================================
 */
void p_shHdrFree(HDR *a_hdr)
{
/*###*   struct region_p *rp;
 *###*
 *###*   rp = a_reg->prvt;
 *###*/

   if ((a_hdr->hdrVec != NULL) && (a_hdr->hdrVec[0] != NULL))  {
/*###*/       /*
 *###*        * If this is a physical header, and a free procedure for it is defined,
 *###*        * then call it
 *###*        */
/*###*       if ((rp->type == SHREGPHYSICAL) && (rp->phys) &&
 *###*           (rp->phys->physHdrFree))  
 *###*/           /*
 *###*            * The first element of the header structure contains the starting
 *###*            * address of the physical header
 *###*            */
/*###*           (*(rp->phys->physHdrFree))(rp->phys->physIndex, a_hdr->hdrVec[0]);
 *###*/
       /*
        * Now, for each physical and virtual region, free each of the header
        * lines. The routines responsible for populating these header lines
        * always malloc memory for them
        * Note: do not free the vector.
        */
#if defined(NOPE)
       f_hdel(a_hdr->hdrVec);
#endif // NOPE
       p_shHdrCntrIncrement(a_hdr);
   }

   return;
}

/*============================================================================
 * ROUTINE: shHdrCopy
 *
 * DESCRIPTION:
 *   This function is responsible for copying one header to another.  If a_inHdr
 *   is NULL, there's nothing much it can do except return an error code.
 *   If a_outHdr's header vector is NULL, memory is allocated as needed.
 *   If a_outHdr's header vector is NOT null, it is wiped out.  Thus it is
 *   the caller's responsibility to make sure that they have saved the contents
 *   of a_outHdr's header if needed.
 *
 * CALL:
 *   (RET_CODE) shHdrInsertLine(const HDR *a_inHdr, HDR *a_outHdr);
 *              a_inHdr  - header to copy from
 *              a_outHdr - header to copy to
 *   
 *
 * CALLS TO:
 *   f_hlrep()
 *   p_shHdrCntrIncrement()
 *
 * RETURN VALUES:
 *   SH_SUCCESS        - on success
 *   SH_HEADER_IS_NULL - if header has not been allocated
 *============================================================================
 */
RET_CODE shHdrCopy(const HDR *a_inHdr, HDR *a_outHdr)
{
   int i;

   if (a_inHdr->hdrVec == NULL)          /* Source header is null, nothing */
       return SH_HEADER_IS_NULL;         /* to copy from */

   if (a_outHdr->hdrVec == NULL)   /* Destination header is null, */
       p_shHdrMallocForVec(a_outHdr); /* allocate memory for it */
   else                                  /* If it's not null, free what it's */
       p_shHdrFree(a_outHdr);         /* currently pointing to */

   for (i = 0; i < MAXHDRLINE; i++)
        if (a_inHdr->hdrVec[i] != NULL)  {
            a_outHdr->hdrVec[i] = 
                 (char *)shMalloc(strlen(a_inHdr->hdrVec[i]) + 1);
            strcpy(a_outHdr->hdrVec[i], a_inHdr->hdrVec[i]);
        }

   p_shHdrCntrIncrement(a_outHdr);

   return SH_SUCCESS;
}

/*============================================================================
 * ROUTINE: p_shHdrMallocForVec
 *
 * DESCRIPTION:
 *   Routine to allocate the base memory for a header vector
 *
 * CALL:
 *   (void) p_shHdrMallocForVec(HDR *a_hdr);
 *          a_hdr    - pointer to the header structure
 *
 * CALLS TO:
 *   shMalloc()
 *   p_shHdrCntrIncrement()
 *
 * RETURN VALUES:
 *   Nothing
 *============================================================================
 */
void p_shHdrMallocForVec(HDR *a_hdr)
{
   int i;

   /*
    * shMalloc aborts if there is no memory on the heap. So, don't bother
    * with sending a return value if a_hdr->hdrVec is NULL after a call to
    * shMalloc()
    */
   a_hdr->hdrVec = ((HDR_VEC *)shMalloc((MAXHDRLINE + 1) * sizeof(HDR_VEC)));

   for (i = 0; i <= MAXHDRLINE; i++)
        a_hdr->hdrVec[i] = NULL;

   p_shHdrCntrIncrement(a_hdr);

   return;
}


