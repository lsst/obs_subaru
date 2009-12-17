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
