/*****************************************************************************
******************************************************************************
**
** FILE:
**	region.c
**
** ABSTRACT:
**	This file contains routines that manipulate masks and regions
**
** ENTRY POINT		SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
** shRegNew		public	Return a new region
** shSubRegNew		public	Return a new sub-region
** parentRegion		public	Return a pointer to a region's parent
** shRegDel		public	Delete a region
** shRegClear		public	Set all the pixels in a region to 0
** isRWRegion		public	Is a region Read/Write?
** shMaskNew		public	Return a new mask
** shSubMaskNew		public	return a new sub-mask
** shMaskClear		public	Set all the pixels in a mask to 0
** isRWMask		public	Is a mask Read/Write?
** shMaskDel		public	Delete a mask
** shRegInfoGet         public  Fill out a REGINFO structure
** shMaskInfoGet        public  Fill out a MASKINFO structure
** shRegNewFromReg	public	Create a new region based on an existing region
**
** ENVIRONMENT:
**	ANSI C.
**
** REQUIRED PRODUCTS:
**
** AUTHOR:
**	Robert Lupton (rhl@astro.princeton.edu)
**
** MODIFICATIONS:
** Vijay     05/19/93    Initialized headerModCntr in makeRegion()
** Vijay     06/21/93    Set to null pointers after a call to shFree()
** Eileen    02/16/94    Add the following routines (so that shRegReadAsFits
**                       code can call them) - p_shRegVectorFree,
**                       p_shRegRowsFree, p_shRegVectorGet, p_shRegRowsGet,
**                       p_shMaskVectorFree, p_shMaskRowsFree,
**                       p_shMaskVectorGet, p_shMaskRowsGet.
** Eileen    02/21/94    Delete header vector in shRegDel too.
******************************************************************************
******************************************************************************
*/
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#include "region.h"
#include "shCUtils.h"
#include "shCAssert.h"
#include "shCHdr.h"
#include "shCErrStack.h"
#include "shCGarbage.h"
#include "prvt/shGarbage_p.h"		/* For p_shMemRefCntrGet() */
#include "prvt/region_p.h"


/*****************************************************************************/
/*
 * free the space allocated to the given region (including its data
 * and its mask).
 *
 * RETURN VALUE:
 *	dervish_success		OK
 *	dervish_freeNonVirtualReg	Region is non-virtual (probably physical)
 *      dervish_genericError	region has children
 */
RET_CODE
shRegDel(REGION *reg)
{
   RET_CODE ret_code;			/* return value */
   
   if(reg == NULL) {
      return(SH_SUCCESS);
   }
   if(reg->prvt == NULL) {		/* can happen if the region was freed
					   more than once. This can happen
					   while recovering from aborts */
      shError("shRegDel called with already-freed region");
      while(p_shMemRefCntrGet(reg) > 0) {
	 p_shMemRefCntrDecr(reg);
      }
      
      shFree((char *)reg);		/* will cause dervish memory fault,
					   which will usually cause an abort,
					   but which can be trapped */
      return(SH_SUCCESS);      
   }
   if((reg->prvt->type != SHREGVIRTUAL) &&
      (reg->prvt->type != SHREGINVALID)) {
      shError("shRegDel: Attempt to free a non-virtual REGION");
      return(SH_FREE_NONVIRTUAL_REG);
   }
   if(reg->prvt->nchild != 0) {
      shError("shRegDel: Attempt to free a region with children");
      return(SH_GENERIC_ERROR);
   }

   if(p_shMemRefCntrGet(reg) > 0) {	/* region is bound to a handle */
      p_shMemRefCntrDecr(reg);
      return(SH_SUCCESS);
   }
   
   shFree((char *)reg->name);

   p_shRegRowsFree(reg);
   p_shRegVectorFree(reg);

/* Free the header and the mask if they exist */
   p_shHdrFreeForVec(&reg->hdr);      /* free the header lines and vector */
   ret_code = shMaskDel(reg->mask);

/* Now free the private area */
   shFree((char *)reg->prvt);

/* And finally the region structure itself */
   shFree((char *)reg);

   return(ret_code);
}

/*
 * ROUTINE: p_shRegRowsFree
 *
 * Given a region structure, free the pixel data storage itself.
 *
 * RETURN VALUES:
 *	NONE 
 */
void
p_shRegRowsFree(
		REGION *reg
		)
{
   if(reg->prvt->parent == NULL) {
      shFree((char *)reg->prvt->pixels);
      reg->prvt->pixels = NULL;
   } else {
      reg->prvt->parent->prvt->nchild--;
      reg->prvt->parent->prvt->children = shTreeKeyDel (reg->prvt->parent->prvt->children, (IPTR)reg);
      shAssert(reg->prvt->parent->prvt->nchild >= 0);
   }
}

