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
#include "shTclTree.h"

static REGION *makeRegion(const char *name, int ncol, PIXDATATYPE type);

/*
 * These are needed until real crc stuff gets in
 */
int p_shGetRegionCheck(REGION *region) { return(0); }
int p_shGetMaskCheck(MASK *mask) { return(0); }


/*****************************************************************************/
/*
 * a static function to initialise most of a MASK
 */
static MASK *
makeMask(
	 const char *name,		/* name of MASK; non-null (but "" OK)*/
	 int nrow			/* number of rows */
	 )
{
   MASK *masknew;
   
   if((masknew = (MASK *)shMalloc(sizeof(MASK))) == NULL) {
      shFatal("makeMask: can't alloc for new mask");
   }

   p_shMaskVectorGet(masknew, nrow);

   if(name == NULL) name = "";		/* be nice */
   if((masknew->name = (char *)shMalloc(strlen(name) + 1)) == NULL) {
      shFatal("makeMask: can't alloc for name");
   }
   strcpy(masknew->name,name);

   if((masknew->prvt = (struct mask_p *)shMalloc(sizeof(struct mask_p)))==NULL){
      shFatal("shSubMaskNew: Can't create prvt struct");
   }
   masknew->prvt->flags = 0;
   masknew->prvt->crc = 0;
   masknew->prvt->nchild = 0;
   masknew->prvt->children = NULL;  /* vkg */
   masknew->prvt->parent = NULL;
   masknew->prvt->col0 = masknew->prvt->row0 = 0;

   return(masknew);
}


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

/*
 * ROUTINE: p_shRegVectorFree
 *
 * Given a region structure, free the vector array that pointed to the rows.
 * The rows themselves had better have been freed first.
 *
 * RETURN VALUES:
 *	NONE 
 */
void
p_shRegVectorFree(
		  REGION *reg
		  )
{
   void **rows;				/* a generic pointer to rows */
   int data_size;			/* sizeof data type (unused) */

   if((rows = p_shRegPtrGet(reg,&data_size)) != NULL) {
      shFree((void *)rows);

      /* Zero out the pointer in the region structure */
      p_shRegPtrZero(reg);
   }
}

/*
 * ROUTINE: p_shRegPtrZero
 *
 * Given a region structure, set the vector pointer to be NULL.
 * The vector itself had better have been freed first.
 *
 * RETURN VALUES:
 *	NONE 
 */
void
p_shRegPtrZero(
	       REGION *reg
	       )
{
   switch (reg->type) {
    case TYPE_U8:
      reg->rows_u8 = NULL;
      break;
    case TYPE_S8:
      reg->rows_s8 = NULL;
      break;
    case TYPE_U16:
      reg->rows_u16 = NULL;
      reg->rows = NULL;
      break;
    case TYPE_S16:
      reg->rows_s16 = NULL;
      break;
    case TYPE_U32:
      reg->rows_u32 = NULL;
      break;
    case TYPE_S32:
      reg->rows_s32 = NULL;
      break;
    case TYPE_FL32:
      reg->rows_fl32 = NULL;
      break;
    default:
      /* We need a default statement since certain pixel types are
         not allowed by this function */
      shFatal("p_shRegPtrZero: pixel type %d is not handled", (int)reg->type);
      break;
   }
}

/*****************************************************************************/
/*
 * These would be static if we didn't need them in other files
 */
void **
p_shRegPtrGet(const REGION *reg, int *size)
{
   void **rows;				/* a generic pointer to rows */

   rows = (void **)reg;			/* use as a marker */
   switch (reg->type) {
    case TYPE_U8:
      rows = (void **)reg->rows_u8; *size = sizeof(U8); break;
    case TYPE_S8:
      rows = (void **)reg->rows_s8; *size = sizeof(S8); break;
    case TYPE_U16:
      rows = (void **)reg->rows_u16; *size = sizeof(U16); break;
    case TYPE_S16:
      rows = (void **)reg->rows_s16; *size = sizeof(S16); break;
    case TYPE_U32:
      rows = (void **)reg->rows_u32; *size = sizeof(U32); break;
    case TYPE_S32:
      rows = (void **)reg->rows_s32; *size = sizeof(S32); break;
    case TYPE_FL32:
      rows = (void **)reg->rows_fl32; *size = sizeof(FL32); break;
    default:
      /* We need a default statement since certain pixel types are
         not allowed by this function */
      shFatal("p_shRegPtrGet: pixel type %d is not handled", (int)reg->type);
      break;
   }

   if(rows == (void **)reg) {
      shFatal("p_shRegPtrGet: Unknown PIXDATATYPE: %d",(int)reg->type);
   }
   return(rows);
}

/*****************************************************************************/
/*
 * free the space allocated to the given mask. check for NULL pointers
 * before calling free(), and print a warning message if the mask
 * pointer itself is NULL
 *
 * RETURN VALUE:
 *	SH_SUCCESS		OK
 *      SH_GENERIC_ERROR	region has children
 */
RET_CODE
shMaskDel(MASK *mask)
{
   if(mask == NULL) {
      return(SH_SUCCESS);
   }
   if(mask->prvt == NULL) {		/* can happen if the mask was freed
					   more than once. This can happen
					   while recovering from aborts */
      shError("shMaskDel called with already-freed region");
      while(p_shMemRefCntrGet(mask) > 0) {
	 p_shMemRefCntrDecr(mask);
      }

      shFree((char *)mask);		/* will cause dervish memory fault,
					   which will usually cause an abort,
					   but which can be trapped */
      return(SH_SUCCESS);      
   }
   if(mask->prvt->nchild != 0) {
      shError("shMaskDel: Attempt to free a mask with children");
      return(SH_GENERIC_ERROR);
   }

   if(p_shMemRefCntrGet(mask) > 0) {	/* mask is bound to a handle */
      p_shMemRefCntrDecr(mask);
      return(SH_SUCCESS);
   }
      
   shFree(mask->name);
   p_shMaskRowsFree(mask);
   shFree((char *)mask->prvt);
   p_shMaskVectorFree(mask);

   shFree((char *)mask);
   return(SH_SUCCESS);
}

/*
 * ROUTINE: p_shMaskRowsFree
 *
 * Given a mask structure, free the mask data storage itself.
 *
 * RETURN VALUES:
 *	NONE 
 */
void
p_shMaskRowsFree(
		 MASK *mask
		 )
{
   if(mask->prvt->parent == NULL) {
      if(mask->rows != NULL) shFree((char *)mask->rows[0]);
   } else {
      mask->prvt->parent->prvt->nchild--;
      mask->prvt->parent->prvt->children = 
            shTreeKeyDel(mask->prvt->parent->prvt->children, (IPTR) mask);
      shAssert(mask->prvt->parent->prvt->nchild >= 0);
   }

}

/*
 * ROUTINE: p_shMaskVectorFree
 *
 * Given a mask structure, free the vector array that pointed to the rows.
 * The rows themselves had better have been freed first.
 *
 * RETURN VALUES:
 *	NONE 
 */
void
p_shMaskVectorFree(
		   MASK *mask
		   )
{
   shFree((char *)mask->rows);
   mask->rows = NULL;
}

/*****************************************************************************/
/*
 * ROUTINE: shRegNew
 *
 * Create an image with the given number of rows and columns -
 * allocate all the memory necessary and return a pointer to the image
 * structure.  If we can't get enough memory, call shFatal()
 *
 * If the product nrow*ncol is 0, don't try to allocate pixel data or row
 * pointers
 *
 * RETURN VALUES:
 *	pointer to new region		all went well
 *	NULL				otherwise
 */
REGION *
shRegNew(const char *name, int nrow, int ncol, PIXDATATYPE type)
{
   REGION *regnew;
   
   if((regnew = makeRegion(name,nrow,type)) == NULL) {
      shFatal("shRegNew: Can't make subregion");
   }

   if (p_shRegRowsGet(regnew, nrow, ncol, type) == (int )NULL) {
      return(NULL);
   }

   regnew->row0 = 0;
   regnew->col0 = 0;
   
   return(regnew);
}

/*****************************************************************************/
/*
 * A static function to make and partially initialise a REGION
 */
static REGION *
makeRegion(const char *name, int nrow, PIXDATATYPE type)
{
   REGION *regnew;
   
   if((regnew = (REGION *)shMalloc(sizeof(REGION))) == NULL) {
      shFatal("makeRegion: can't alloc for new region");
   }
   
   if (p_shRegVectorGet(regnew, nrow, type) == (int )NULL) {
      return(NULL);
   }

   if(name == NULL) name = "";		/* be nice */
   if((regnew->name = (char *)shMalloc(strlen(name) + 1)) == NULL) {
      shFatal("makeRegion: can't alloc for name");
   }
   strcpy(regnew->name,name);

   regnew->mask = NULL;
   /*
    * Finally set the prvt struct
    */
   if((regnew->prvt = (struct region_p *)shMalloc(sizeof(struct region_p)))
      == NULL) {
      shFatal("shSubRegNew: can't alloc REGION's prvt struct");
   }
   regnew->hdr.hdrVec = NULL;
   regnew->hdr.modCnt = 0;
   regnew->prvt->type = SHREGINVALID;
   regnew->prvt->flags = 0;
   regnew->prvt->crc = 0;
   regnew->prvt->parent = NULL;
   regnew->prvt->col0 = regnew->prvt->row0 = 0;
   regnew->prvt->pixels = NULL;
   regnew->prvt->nchild = 0;
   regnew->prvt->children = NULL;
   regnew->prvt->ModCntr = 0;
   
   regnew->prvt->phys = NULL;
   
   return(regnew);
}

/*
 * ROUTINE: p_shRegVectorGet
 *
 * Given a region structure and the number of rows in the region, malloc the
 * region vector array that will point to each of the rows.
 *
 * RETURN VALUES:
 *	NULL       There was an error getting the pixel type.
 *      1          Everything went ok.
 *	(shFatal is called if there is a mallocing error.)
 */
int
p_shRegVectorGet(
		 REGION *reg,
		 int         nrow,
		 PIXDATATYPE type
		 )
{
   int rstatus = 1;
   int i;
   void **rows = NULL;			/* generic pointer to pointer to rows*/

   reg->rows_u8 = NULL;		/* all but one of these will be NULL */
   reg->rows_s8 = NULL;
   reg->rows_u16 = NULL;
   reg->rows_s16 = NULL;
   reg->rows_u32 = NULL;
   reg->rows_s32 = NULL;
   reg->rows_fl32 = NULL;
   
   if(nrow == 0) {
      reg->rows = NULL;
   } else {
      rows = (void **)reg;		/* just a marker for a missing case */
      switch (type) {
       case TYPE_U8:
	 reg->rows_u8 = (U8 **)shMalloc(nrow*sizeof(U8 *));
	 rows = (void **)reg->rows_u8;
	 break;
       case TYPE_S8:
	 reg->rows_s8 = (S8 **)shMalloc(nrow*sizeof(S8 *));
	 rows = (void **)reg->rows_s8;
	 break;
       case TYPE_U16:
	 reg->rows_u16 = (U16 **)shMalloc(nrow*sizeof(U16 *));
	 rows = (void **)reg->rows_u16;
	 break;
       case TYPE_S16:
	 reg->rows_s16 = (S16 **)shMalloc(nrow*sizeof(S16 *));
	 rows = (void **)reg->rows_s16;
	 break;
       case TYPE_U32:
	 reg->rows_u32 = (U32 **)shMalloc(nrow*sizeof(U32 *));
	 rows = (void **)reg->rows_u32;
	 break;
       case TYPE_S32:

	 reg->rows_s32 = (S32 **)shMalloc(nrow*sizeof(S32 *));
	 rows = (void **)reg->rows_s32;
	 break;
       case TYPE_FL32:
	 reg->rows_fl32 = (FL32 **)shMalloc(nrow*sizeof(FL32 *));
	 rows = (void **)reg->rows_fl32;
	 break;
       default:
         /* We need a default statement since certain pixel types are
            not allowed by this function */
         shFatal("makeRegion: pixel type %d is not handled", (int)type);
         break;
      }

      if(rows == NULL) {
         shFatal("makeRegion: can't alloc for rows");
      } else if(rows == (void **)reg) {
	 shError("makeRegion: Unknown PIXDATATYPE: %d",(int)type);
	 shFree((void *)reg);
	 return((int )NULL);
      }
      reg->rows = (type == TYPE_U16) ? (U16 **)rows : NULL;
   }

   *(PIXDATATYPE *)&reg->type = type;	/* cast away const */
   reg->nrow = nrow;
   reg->ncol = 0;
   reg->row0 = reg->col0 = 0;
   if(rows != NULL) {
      for(i = 0; i < nrow; i++) {
	 rows[i] = NULL;
      }
   }

   return(rstatus);
}

/*
 * ROUTINE: p_shRegRowsGet
 *
 * Given a region structure and the number of rows and columns in the region,
 * malloc the space for the region pixels themselves.
 *
 * RETURN VALUES:
 *	NULL       There was an error getting the pixel type.
 *      1          Everything went ok.
 */
int
p_shRegRowsGet(
	       REGION      *reg,
	       int         nrow,
	       int         ncol,
	       PIXDATATYPE type
	       )
{
   int rstatus = 1;
   int i;

   if(ncol*nrow != 0) {			/* allocate pixel memory */
      void **rows = NULL;		/* a generic pointer to rows */
      int data_size;			/* sizeof for requested data type */

      *(PIXDATATYPE *)&reg->type = type;	/* cast away const */
      if((rows = p_shRegPtrGet(reg,&data_size)) == NULL) {
	 shError("p_shRegRowsGet: Unknown PIXDATATYPE: %d",(int)type);
	 shRegDel(reg);
	 return((int )NULL);
      }      

      if((rows[0] = shMalloc(nrow*ncol*data_size)) == NULL) {
         shFatal("p_shRegRowsGet: can't alloc for rows");
      }
      reg->prvt->pixels = rows[0];
      
      for(i = 0;i < nrow;i++) {
         rows[i] = (char *)rows[0] + i*ncol*data_size;
      }
      reg->ncol = ncol;
      reg->prvt->type = SHREGVIRTUAL;
   }

   return(rstatus);
}

/*
 * ROUTINE: shSubRegNew
 *
 * Given a region, and a set of parameters specifying some subset of
 * that region (row, col and number of rows and cols), return
 * a pointer to a new region which consists of the subset alone.  This
 * routine allocates no new pixel memory itself.
 *
 * If the subset parameters are invalid (row < 0, number of cols larger
 * than in image, etc.), it examines the region's parent to see if the
 * pixels requested actually exist. If so it'll grow the region, otherwise
 * it returns NULL.
 *
 * As always, row and col numbering of the pixels in a region begins
 * at 0, not 1.  Also, note that the new, "child" region will have the
 * passed region as its "parent" (or, more precisely, the passed region's
 * ultimate ancestor as its parent).
 *
 * RETURN VALUES:
 *	pointer to new region		all went well
 *	NULL				otherwise
 */
REGION *
shSubRegNew(
	     const char *name,		/* non-NULL name of REGION ("" is OK)*/
	     const REGION *region,	/* parent REGION */
	     int nrow,			/* number of rows */
	     int ncol,			/* number of columns */
	     int row0,			/* starting row number in parent */
	     int col0,			/* and starting column number */
	     REGION_FLAGS flags		/* flags as defined above */
	     )
{
   int data_size;
   int i;
   REGION *regnew;
   void **rows = NULL;			/* rows pointer in parent, */
   void **nrows;			/* and in child */

   if(region == NULL) {
      shErrStackPush("shSubRegNew: given a NULL region");
      return(NULL);
   }
/* 
 * The following is in two parts:
 *
 *     a) initialization involving the 'ultimate parent'
 *     b) initialization involving the immediate parent or
 *        no parent at all.
 *
 *     a) is done in a recursive way.
 *     b) should be done in a recursive way.
 */

/*
 * Try to get subregion from parent if it exists; this allows shSubRegNew
 * to be used to grow subregions
 */
   if(region->prvt->parent != NULL) {	/* get it from parent */
      regnew = shSubRegNew(name,region->prvt->parent,nrow,ncol,
			   region->prvt->row0 + row0,region->prvt->col0 + col0,
			   flags);
      if(regnew == NULL) {
	 return(NULL);
      }

      regnew->col0 = col0; regnew->row0 = row0;
      if(regnew->mask != NULL) {
	 regnew->mask->col0 = col0; regnew->mask->row0 = row0;
      }
   }
   else {
     /*
      * check to make sure that this makes sense
      */
     if ((row0 < 0) || (row0 >= region->nrow) ||
	 (col0 < 0) || (col0 >= region->ncol)) {
       shErrStackPush("shSubRegNew: subregion row/col out of bounds");
       return(NULL);
     }
     if ((row0 + nrow > region->nrow) || (col0 + ncol > region->ncol)) {
       shErrStackPush("shSubRegNew: ending row/col out of bounds");
       return(NULL);
     }
     /*
      * subregion fits in new parent; build the new region. If the new parent
      * has a living parent, try to get the subregion from there so as to
      * keep subregions as close to the top of the region pile as possible
      */
   
     if((regnew = makeRegion(name,nrow,region->type)) == NULL) {
       shFatal("shSubRegNew: Can't make region");
     }
     regnew->prvt->type = SHREGVIRTUAL;	/* this is true even if the parent
					   is a physical region */
     /*
      * now set the region pointers
      */
     if((rows = p_shRegPtrGet(region,&data_size)) == NULL) {
       shError("shSubRegNew: Pixel data is NULL");
       return(NULL);
     }
     nrows = p_shRegPtrGet(regnew,&data_size); /* this can't fail if previous
					     call succeeded */

     for(i = 0;i < nrow;i++) {
       nrows[i] = (char *)rows[0] + ((i + row0)*region->ncol + col0)*data_size;
     }
     regnew->ncol = ncol;
     regnew->prvt->parent = (REGION *)region;
     
     /* Build record of child in parent*/
     region->prvt->nchild++;
     region->prvt->children =  shTreeKeyInsert (region->prvt->children, (IPTR)regnew, (IPTR)0);
     regnew->row0 = row0; regnew->col0 = col0;
     regnew->prvt->row0 = row0; regnew->prvt->col0 = col0;

   }

/*
 * Now the initialization involving the 'direct' parent
 */

   if((flags & COPY_MASK) && region->mask != NULL) {
      if((regnew->mask =
	  shSubMaskNew(name,region->mask,nrow,ncol,row0,col0,flags)) == NULL) {
	 shErrStackPush("shSubRegNew: Can't copy mask");
      }
   } else {
      regnew->mask = NULL;
   }
/*
**	Copy the header if necessary
*/
   if ((flags & COPY_HEADER_DEEP) && region->hdr.hdrVec != NULL) {
      if (shHdrCopy (&region->hdr, &regnew->hdr) != SH_SUCCESS) {
	 shErrStackPush("shSubRegNew: Can't copy region header");
      }
   } else {
      regnew->hdr.hdrVec = NULL;
   }

   regnew->prvt->crc = p_shGetRegionCheck(regnew);
   if(flags & READ_ONLY) regnew->prvt->flags |= READ_ONLY;
   
   return(regnew);
}

/*****************************************************************************/
/*
 * Return a sub-mask to the mask provided.
 *
 * RETURN VALUE:
 *	the sub-mask, or NULL on error
 */
MASK *
shSubMaskNew(
	   const char *name,		/* name of MASK; non-null (but "" OK)*/
	   const MASK *mask,		/* parent MASK */
	   int nrow,			/* number of rows */
	   int ncol,			/* number of columns */
	   int row0,			/* starting row number in parent */
	   int col0,			/* and starting column number */
	   REGION_FLAGS flags		/* flags as defined above */
	   )
{
   int i;
   MASK *masknew;
   
   /* check to make sure that this makes sense */
   if (mask == NULL) {
      shError("shSubMaskNew: given a NULL mask");
      return(NULL);
   }
   if ((row0 < 0) || (row0 >= mask->nrow) ||
       (col0 < 0) || (col0 >= mask->ncol)) {
      shError("shSubMaskNew: submask row/col out of bounds");
      return(NULL);
   }
   if ((row0 + nrow > mask->nrow) || (col0 + ncol > mask->ncol)) {
      shError("shSubMaskNew: ending row/col out of bounds");
      return(NULL);
   }
/*
 * submask fits in new parent; build the new mask. If the new parent
 * has a living parent, try to get the submask from there so as to
 * keep submasks as close to the top of the mask pile as possible
 */
   if(mask->prvt->parent != NULL) {		/* get it from parent */
      masknew = shSubMaskNew(name,mask->prvt->parent,nrow,ncol,
		       	mask->prvt->row0 + row0,mask->prvt->col0 + col0,flags);
      masknew->col0 = col0; masknew->row0 = row0;
      return(masknew);
   }
   masknew = makeMask(name,nrow);
   masknew->prvt->parent = (MASK *)mask;
   mask->prvt->nchild++;
   mask->prvt->children = shTreeKeyInsert(mask->prvt->children, (IPTR) masknew,
                                          (IPTR) 0); /* vkg */
   masknew->ncol = ncol;
   masknew->row0 = row0; masknew->col0 = col0;
   masknew->prvt->row0 = row0; masknew->prvt->col0 = col0;
   for (i = 0; i < nrow; i++) {
      masknew->rows[i] = &mask->rows[row0 + i][col0];
   }

   masknew->prvt->crc = p_shGetMaskCheck(masknew);
   if(flags & READ_ONLY) masknew->prvt->flags |= READ_ONLY;

   return(masknew);
}

/*
 * ROUTINE: p_shMaskVectorGet
 *
 * Given a mask structure and the number of rows in the mask, malloc the mask
 * vector array that will point to each of the rows.
 *
 * RETURN VALUES:
 *	NONE (shFatal is called if there is a mallocing error.)
 */
void
p_shMaskVectorGet(
		  MASK *mask,
		  int  nrow
		  )
{
   int i;

   if(nrow == 0) {
      mask->rows = NULL;
   } else {
      if((mask->rows = (unsigned char **)shMalloc(nrow*sizeof(unsigned char *))) == NULL) {
         shFatal("p_shMaskVectorGet: can't alloc for rows");
      }
   }

   mask->nrow = nrow;
   mask->ncol = 0;
   for(i = 0;i < nrow;i++) {
      mask->rows[i] = NULL;
   }
   mask->row0 = mask->col0 = 0;
}

/*****************************************************************************/
/*
 * Clear a region, setting all pixels to 0
 *
 * RETURN VALUE:
 *	none
 */
void
shRegClear(REGION *region)
{
   int data_size;			/* sizeof for requested data type */
   int i;
   void **rows = NULL;
   
   if(region == NULL || region->nrow <= 0 || region->ncol <= 0) return;
   
   if((rows = p_shRegPtrGet(region,&data_size)) == NULL) {
      shError("shSubClear: Unknown PIXDATATYPE: %d",(int)region->type);
      return;
   }
   memset(rows[0],'\0',region->ncol*data_size);
   for(i = 1;i < region->nrow;i++) {
      memcpy(rows[i], rows[0], region->ncol*data_size);
   }
}

/*****************************************************************************/
/*
 * create a MASK structure with the given number of rows and columns.
 * fill the array with 0s, and place the appropriate values into
 * the 'nrow', 'ncol', fields.  set the 'parent' and 'prvt' fields
 * to NULL.
 *
 * RETURN VALUE:
 *	a pointer to the mask, or NULL on error
 */
MASK *
shMaskNew(const char *name, int nrow, int ncol)
{
   MASK *masknew;
   
   masknew = makeMask(name,nrow);

   p_shMaskRowsGet(masknew, nrow, ncol);

   return(masknew);
}

/*
 * ROUTINE: p_shMaskRowsGet
 *
 * Given a mask structure and the number of rows and columns in the mask,
 * malloc the space for the mask itself.
 *
 * RETURN VALUES:
 *	NONE (shFatal is called if there is a mallocing error.)
 */
void
p_shMaskRowsGet(
		MASK *mask,
		int  nrow,
		int  ncol
		)
{
   int i;

   if(nrow*ncol != 0) {
      if((mask->rows[0] = (unsigned char *)shMalloc(nrow*ncol)) == NULL) {
         shFatal("p_shMaskRowsGet: Can't allocate storage for rows");
      }
      for(i = 0;i < nrow;i++) {
	 mask->rows[i] = mask->rows[0] + i*ncol;
      }
      mask->ncol = ncol;
   }

   shMaskClear(mask);
}

