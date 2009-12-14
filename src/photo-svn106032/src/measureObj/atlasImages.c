/*
 * Create ATLAS_IMAGEs from REGIONs and OBJC lists
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <alloca.h>
#include "dervish.h"
#include "phRandom.h"
#include "phMeasureObj.h"
#include "phOffset.h"
#include "phUtils.h"
#include "phObjc_p.h"

/************************************************************************************************************/

AI_PIX *
phAiPixNew(const OBJMASK *om,		/* OBJMASK defining real pixels; or NULL */
	   int allocate_data,		/* Allocate data array? */
	   PIX val)			/* value of virtual pixels */
{
    AI_PIX *new = shMalloc(sizeof(AI_PIX));
    new->mask = phObjmaskCopy(om, 0, 0);
    new->val = val;

    if (allocate_data) {
	new->mask->ndata = new->mask->npix;
	new->mask->data = shMalloc(new->mask->ndata*sizeof(PIX));
    }

    return new;
}

void
phAiPixFree(AI_PIX *aip)
{
    if (aip != NULL) {
	phObjmaskDel(aip->mask);
	shFree(aip);
    }
}

/*
 * Expand an ATLAS_IMAGE's pixel data to a full array of length npix.  If in_place
 * is true, adjust the ATLAS_IMAGE and return NULL, otherwise return a (PIX *) that
 * should be freed by the caller
 */
PIX *
phAtlasImagePixelExpand(const ATLAS_IMAGE *ai, /* Atlas image */
			int c,		/* in this colour */
			int in_place)	/* perform conversion in place? */
{
   PIX *data = NULL;

   shAssert(ai != NULL);
   shAssert(c >= 0 && c < ai->ncolor);
   shAssert(ai->pix[c]->mask != NULL);

   if (ai->pix[c]->mask->npix == ai->npix) { /* all pixels are real */
       if (in_place) {
	   return NULL;
       } else {
	   data = ai->pix[c]->mask->data;
	   p_shMemRefCntrIncr(data);

	   return data;
       }
   } else {
       AI_PIX *aip = ai->pix[c];	/* for convenience */
       const OBJMASK *mmask = ai->master_mask; /* also for convenience */
       REGION *tmpReg;			/* region that we'll use to expand aip into; this is inefficient
					   in that it requires more temp space, but it's simple */
       OBJMASK *tmpMask;		/* temp mask to set data (or new aip->mask if in_place) */

       shAssert(aip->mask->npix < ai->npix);
       shAssert(mmask != NULL);
       
       tmpReg = shRegNew(__func__, mmask->rmax - mmask->rmin + 1,
			 mmask->cmax - mmask->cmin + 1, TYPE_PIX);
       tmpReg->row0 = mmask->rmin;
       tmpReg->col0 = mmask->cmin;

       phRegionSetValFromObjmask(tmpReg, mmask, aip->val);
       phRegionSetFromObjmask(tmpReg, aip->mask);
       tmpMask = phObjmaskCopy(aip->mask, 0, 0);
       phObjmaskSetFromRegion(tmpMask, tmpReg);
       shRegDel(tmpReg);

       if (in_place) {
	   shFree(aip->mask);
	   aip->mask = tmpMask;
	   
	   data = NULL;
       } else {
	   data = tmpMask->data;
	   tmpMask->data = NULL;
	   phObjmaskDel(tmpMask);
       }
   }

   return data;
}

void
phAtlasImagePixelCompress(ATLAS_IMAGE *ai,	/* atlas image */
			  int c)		/* compress this band */
{
   shAssert(ai != NULL);
   shAssert(c >= 0 && c < ai->ncolor);
}

/************************************************************************************************************/

static AI_PIX *ai_pix_set(AI_PIX *pix, const OBJMASK *om, const REGION *reg,
			  int drow, int dcol);
static void ai_reg_set(const OBJMASK *mm, const AI_PIX *pix, const OBJMASK *om,
		       int odrow, int odcol,
		       REGION *reg, int drow, int dcol, int operator, float sky);
static void ai_reg_set_val(const OBJMASK *om, REGION *reg,
			   int val, float sigma, RANDOM *rand,
			   int drow, int dcol);

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Create an ATLAS_IMAGE 
 */
ATLAS_IMAGE *
phAtlasImageNew(int ncolor)		/* desired number of colours */
{
   int c;
   ATLAS_IMAGE *new_ai = shMalloc(sizeof(ATLAS_IMAGE));

   shAssert(ncolor <= NCOLOR);
   *(int *)&new_ai->ncolor = ncolor;
   new_ai->id = new_ai->parent = -1;
   new_ai->run = new_ai->rerun = new_ai->camCol = new_ai->field = -1;
   new_ai->shallow_copy = 0;
   new_ai->master_mask = NULL;
   
   new_ai->npix = 0;
   for(c = 0;c < NCOLOR;c++) {
      new_ai->drow[c] = new_ai->dcol[c] = 0;
      new_ai->regmask[c] = NULL;
      new_ai->mask[c] = NULL;
      new_ai->pix[c] = NULL;
   }

   return(new_ai);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Make an atlas image from an OBJC
 */
ATLAS_IMAGE *
phAtlasImageNewFromObjc(const OBJC *objc)
{
   int c;
   ATLAS_IMAGE *new;
   OBJECT1 *obj1;

   shAssert(objc != NULL);

   new = phAtlasImageNew(objc->ncolor);
   new->shallow_copy = 1;

   new->id = objc->id;

   for(c = 0;c < objc->ncolor;c++) {
      obj1 = objc->color[c];
      shAssert(obj1 != NULL);

      if(obj1->region != NULL) {
	 new->regmask[c] = (SPANMASK *)obj1->region->mask;
	 if(new->regmask[c] != NULL) {
	    shAssert(new->regmask[c]->cookie == SPAN_COOKIE);
	 }
      }
      new->mask[c] = obj1->mask;
   }

   return(new);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Destroy an ATLAS_IMAGE. If deep is true, all structures allocated within the
 * ATLAS_IMAGE will be freed; if it's false they will not. The pix[] arrays
 * are deleted whether or not deep is true
 */
void
phAtlasImageDel(ATLAS_IMAGE *ai, int deep)
{
   int c;
   
   if(ai == NULL) return;

   if(p_shMemRefCntrGet(ai) > 0) {	/* still referenced somewhere */
      p_shMemRefCntrDecr(ai);
      return;
   }

   shAssert(!(deep && ai->shallow_copy));

   for(c = 0;c < ai->ncolor;c++) {
       phAiPixFree(ai->pix[c]);

      if(deep) {
	 phSpanmaskDel(ai->regmask[c]);
	 phObjmaskDel(ai->mask[c]);
      }
   }
   if(deep) {
      phObjmaskDel(ai->master_mask);
   }

   shFree(ai);
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * Replace the pixels in one ATLAS_IMAGE with those in another; the latter's
 * pixels become NULL
 */
void
phAtlasImagePixReplace(ATLAS_IMAGE *ai1, /* one atlas image */
		       int c1,		/* colour to use in ai1 */
		       ATLAS_IMAGE *ai2, /* other atlas image */
		       int c2)		/* which color to use in ai2 */
{
   shAssert(ai1 != NULL && ai2 != NULL);
   shAssert(ai1->npix == ai2->npix);
   shAssert(c1 >= 0 && c1 < ai1->ncolor);
   shAssert(c2 >= 0 && c2 < ai2->ncolor);    

   phAiPixFree(ai1->pix[c1]);
   ai1->pix[c1] = ai2->pix[c2];
   ai2->pix[c2] = NULL;
}    

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * delete the ATLAS_IMAGEs associated with the given OBJC and its family
 */
void
phAtlasImageDelFromObjc(OBJC *objc,	/* OBJC whose AIs are to be deleted */
			int deep)	/* should we destroy siblings
					   and children? */
{
   if(objc == NULL) return;
   
   if(deep) {
      phAtlasImageDelFromObjc(objc->children, 1); /* n.b. will recurse */
      phAtlasImageDelFromObjc(objc->sibbs, 1); /*      down lists */
   }

   phAtlasImageDel(objc->aimage, deep);
   objc->aimage = NULL;
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Return a copy of an ATLAS_IMAGE. If deep is true, all structures allocated
 * within the ATLAS_IMAGE will be copied; if it's false they will not (they'll
 * be set to point to the ones in the original).
 *
 * The dcol[], drow[], and pix[] arrays are copied whether or not deep is true
 */
ATLAS_IMAGE *
phAtlasImageCopy(const ATLAS_IMAGE *old, /* atlas image to copy */
		 int deep)		/* do a deep copy? */
{
   int c;
   ATLAS_IMAGE *ai;			/* atlas image to return */
   int ncolor;				/* number of colours in atlas images */

   shAssert(old != NULL);
   shAssert(old->master_mask != NULL && old->master_mask->npix >= 0);

   ncolor = old->ncolor;
   ai = phAtlasImageNew(ncolor);
   ai->shallow_copy = deep ? 0 : 1;

   ai->npix = old->npix;
   for(c = 0;c < ncolor;c++) {
      const AI_PIX *aip = old->pix[c];
      ai->drow[c] = old->drow[c];
      ai->dcol[c] = old->dcol[c];

      ai->pix[c] = phAiPixNew(aip->mask, 1, aip->val);
      memcpy(ai->pix[c]->mask->data, aip->mask->data, aip->mask->npix*sizeof(PIX));
      
      if(deep) {
	 ai->regmask[c] = phSpanmaskCopy(old->regmask[c], 0, 0);
	 ai->mask[c] = phObjmaskCopy(old->mask[c], 0, 0);
      } else {
	 ai->regmask[c] = old->regmask[c];
	 ai->mask[c] = old->mask[c];
      }
   }

   if(deep) {
      ai->master_mask = phObjmaskCopy(old->master_mask, 0, 0);
   } else {
      ai->master_mask = old->master_mask;
   }
   

   return(ai);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Given an OBJC with its canonical centre set and a FIELDPARAMS,
 * set the pixel values pix[] in the OBJC's ATLAS_IMAGE
 *
 * If val is >= 0, the data regions in fparams will have their values
 * replaced by val after the atlas images are extracted. If sigma is
 * greater than zero, the values will have N(0,sigma^2) noise added
 * (why bother? because the deblender assumes that the sky is noisy
 * when correcting for biases)
 */
#if !defined(STAND_ALONE)
void
phAtlasImageCut(OBJC *objc,		/* set the ATLAS_IMAGE in here */
		int color,		/* which colour? (-1 => all) */
		const FIELDPARAMS *fparams, /* all about the frame */
		int val,		/* value to set
					 (if < 0 && != -1, use SOFT_BIAS + FRAMEPARAMS.bkgd) */
		float sigma,		/* s.d. of val */
		RANDOM *rand)		/* random numbers; may be NULL if
					   sigma <= 0 */
{
   ATLAS_IMAGE *ai;			/* the atlas image to set */
   int c0, c1;				/* range of colours to cut */
   int c;
   const REGION *data;			/* == fparams->frame[c].data */
   float drow, dcol;			/* offsets from reference colour */

   shAssert(objc != NULL);
   shAssert(objc->aimage != NULL && objc->aimage->master_mask != NULL);
   shAssert(objc->ncolor == objc->aimage->ncolor);
   shAssert(color < 0 || color < objc->ncolor);
   shAssert(fparams != NULL);
   shAssert(objc->flags3 & OBJECT3_HAS_CENTER);
   shAssert(sigma <= 0 || rand != NULL);

   ai = objc->aimage;
   if(color >= 0) {
      c0 = c1 = color;
   } else {
      c0 = 0; c1 = objc->ncolor - 1;
   }

   ai->npix = ai->master_mask->npix;
   for(c = c0;c <= c1;c++) {
      data = fparams->frame[c].data;
      shAssert(data != NULL);
/*
 * calculate the offsets from the reference colour
 */
      phOffsetDo(fparams, objc->rowc, objc->colc,
		 fparams->ref_band_index, c,
		 0, NULL, NULL, &drow, NULL, &dcol, NULL);
      ai->drow[c] = (drow > 0) ? drow + 0.5 : -(-drow + 0.5);
      ai->dcol[c] = (dcol > 0) ? dcol + 0.5 : -(-dcol + 0.5);
/*
 * copy over mask fields from objc->color, if they exist
 */
      if(objc->color[c] != NULL) {
	 phObjmaskDel(ai->mask[c]);
	 ai->mask[c] = objc->color[c]->mask;
	 if(objc->color[c]->mask != NULL) {
	    objc->color[c]->mask->refcntr++;
	 }
      }
/*
 * and set the ATLAS_IMAGE->pix fields
 */
      phAiPixFree(ai->pix[c]);		/* it may be the wrong size */
      ai->pix[c] = ai_pix_set(NULL, ai->master_mask, data,
						     ai->drow[c], ai->dcol[c]);
      phAtlasImagePixelCompress(ai, c);
/*
 * if so desired, set the cut-out pixels to some value
 */
      if(val != -1) {
	  phRegionSetValFromAtlasImage(ai, c, 1, (REGION *)fparams->frame[c].data,
				      (val >= 0 ? val : SOFT_BIAS + fparams->frame[c].bkgd),
	 			      sigma, rand, 0, 0);
	 
      }
   }
}
#endif

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Set the pix[] fields in an atlas image from a region
 */
void
phAtlasImageSetFromRegion(ATLAS_IMAGE *ai, /* the atlas image with pix to set*/
			  int c,	/* which colour? */
			  const REGION *data)	/* region with desired data */
{
   shAssert(ai != NULL && ai->master_mask != NULL);
   shAssert(ai->pix[0] == NULL || ai->npix == ai->master_mask->npix);
   shAssert(c >= 0 && c < ai->ncolor);
   shAssert(data != NULL);

   ai->npix = ai->master_mask->npix;
   ai->pix[c] = ai_pix_set(ai->pix[c], ai->master_mask, data,
						     ai->drow[c], ai->dcol[c]);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Set the pix[] fields in an atlas image to a certain value within a
 * specified OBJMASK
 */
void
phAtlasImageSetInObjmask(ATLAS_IMAGE *ai, /* the atlas image with pix to set*/
			 int c,		/* which colour? */
			 const OBJMASK *om, /* pixels to set */
			 const PIX val)	/* desired value */
{
   int drow, dcol;			/* how much to offset om's coords */
   PIX *dptr;				/* pointer to pix->data */
   int dindex;				/* index into dptr */
   int i, j, k;
   const OBJMASK *mm;			/* == ai->master_mask */
   SPAN *mp;				/* == mm->s[i] */
   SPAN *sp;				/* == om->s[i] */
   int x1, x2, y;			/* unpacked from sp */

   shAssert(c >= 0 && c < ai->ncolor);
   shAssert(ai != NULL && ai->master_mask != NULL)
   shAssert(ai->pix[c] != NULL && ai->pix[c]->mask != NULL && ai->pix[c]->mask->data != NULL);
   shAssert(ai->master_mask->row0 == 0 && ai->master_mask->col0 == 0);
   shAssert(om != NULL && om->npix >= 0);
/*
 * Go through the objmask om looking for overlaps with the master_mask.
 *
 * Note that the master_mask's must be offset by (drow, dcol) to bring
 * into the c-band coordinate system, so we have to shift om the other way
 */
   drow = om->row0 - ai->drow[c]; dcol = om->col0 - ai->dcol[c];

   mm = ai->pix[c]->mask;
   dptr = mm->data; dindex = 0;
   j = 0;				/* counter in mm->s */
   mp = &mm->s[j]; j++;

   for(i = 0;i < om->nspan;i++) {
      sp = &om->s[i];
      y = sp->y; x1 = sp->x1; x2 = sp->x2;

      y += drow; x1 += dcol; x2 += dcol;
/*
 * advance mm's counter j until mm's current span is on same row as om's,
 * and om's span doesn't lie entirely to the left of mm's
 */
      if(y < mp->y) {			/* below mm's current span */
	 continue;
      } else {				/* above mm's current span */
	 while(mp->y < y || mp->x2 < x1) {
	    dindex += mp->x2 - mp->x1 + 1;
	    if(j == mm->nspan) {
	       j++;			/* signal that we ran out of spans */
	       break;
	    }
	    mp = &mm->s[j]; j++;
	 }

	 if(j > mm->nspan) {		/* we ran out of spans in mm */
	    break;
	 }
      }

      if(mp->y > y || mp->x1 > x2) {	/* no overlap in this row */
	 continue;
      }
/*
 * If we get here, there's an overlap. Set the requisite values in pix
 */
      shAssert(y == mp->y && x1 <= mp->x2 && x2 >= mp->x1);
      if(x1 < mp->x1) {
	 x1 = mp->x1;
      }
      if(x2 > mp->x2) {
	 x2 = mp->x2;
      }

      for(k = x1; k < x2; k++) {
	 dptr[dindex + k - mp->x1] = val;
      }
   }
#if !defined(NDEBUG)
   if(j <= mm->nspan) {			/* we haven't finished with mm */
      for(;;) {
	 dindex += mp->x2 - mp->x1 + 1;
	 if(j == mm->nspan) {
	    break;
	 }
	 mp = &mm->s[j]; j++;
      }
   }
   shAssert(dindex == mm->npix);
#endif
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Given an ATLAS_IMAGE containing pixel data, set the corresponding pixels
 * in a region. If clip is true, only set the pixels within ai->mask[c]
 *
 * Because we may want to restore the atlas image data into a smaller
 * region than the one that it was created from, the region's physical
 * origin is taken to be at [row0,col0] in the atlas image's coordinate
 * system.
 *
 * Note that the atlas image is also shifted by the appropriate drow, dcol
 * to take it to the proper coordinate system for its colour
 */
void
phRegionSetFromAtlasImage(const ATLAS_IMAGE *ai, /* here's the data */
			  int c,	/* colour to set */
			  REGION *reg,	/* the region in question */
			  int row0, int col0, /* offset ai's origin to here */
			  float sky,	/* add this to all ai's pixels */
			  int operator,	/* requested operation: \0: set +: add -: subtract */
			  int clip)	/* Clip atlas image to pixels within mask? */
{
   shAssert(ai != NULL);
   shAssert(c >= 0 && c < ai->ncolor);
   shAssert(ai->master_mask != NULL && ai->master_mask->npix >= 0);
   if(clip) {
      shAssert(ai->mask[c] != NULL);
   }

   if(ai->npix == 0) {			/* don't set region */
      return;
   }
   
#if !defined(NDEBUG)
   if(ai->master_mask->npix == 0) {
      shAssert(ai->pix[c] == NULL);
   } else {
      shAssert(ai->pix[c] != NULL && ai->npix == ai->master_mask->npix);
   }
#endif
   shAssert(reg != NULL && reg->type == TYPE_PIX);

   ai_reg_set(ai->master_mask, ai->pix[c], (clip ? ai->mask[c] : NULL),
	      -ai->drow[c], -ai->dcol[c],
	      reg, ai->drow[c] - row0, ai->dcol[c] - col0, operator, sky);
}

/*****************************************************************************/
/*
 * Set the value of a part of a REGION wherever an atlas image has been cut.
 *
 * If sigma is greater than zero, the values will have N(0,sigma^2) noise
 * added (why bother? because the deblender assumes that the sky is noisy
 * when correcting for biases)
 */
#if !defined(STAND_ALONE)
void
phRegionSetValFromAtlasImage(const ATLAS_IMAGE *ai, /* atlas image */
			     int c,	/* which band? */
			     int use_master_mask, /* use the master, not per-band, mask */
			     REGION *reg, /* region to set */
			     int val,	/* value to set */
			     float sigma, /* s.d. of val */
			     RANDOM *rand, /* random numbers; may be NULL if
					      sigma <= 0 */
			     int row0,	/* offset of ai in reg */
			     int col0)
{
   shAssert(ai != NULL);
   shAssert(c >= 0 && c < ai->ncolor);
   shAssert(ai->master_mask != NULL && ai->master_mask->npix >= 0);
   shAssert(reg != NULL && reg->type == TYPE_PIX);
   if(sigma > 0 && rand == NULL) {
      sigma = 0;
   }

   if(!use_master_mask) {
       row0 += ai->drow[c]; col0 += ai->dcol[c];
   }

   ai_reg_set_val(use_master_mask ? ai->master_mask : ai->mask[c],
		  reg, val, sigma, rand,
		  ai->drow[c] - row0, ai->dcol[c] - col0);
}
#endif

/*****************************************************************************/
/*
 * Return an array of all the pixel values corresponding to pixels present
 * in the OBJMASK, the values being those in the REGION.
 *
 * We allow the origin of the region to be different from that in the mask;
 * the convention is that pixel [drow, dcol] in the region is [0,0] in the mask
 */
static AI_PIX *
ai_pix_set(AI_PIX *pix,			/* space to hold the pixels, or NULL */
	   const OBJMASK *om,		/* the OBJMASK specifying the pixels */
	   const REGION *reg,		/* the region wherein they dwell */
	   int drow, int dcol)		/* how much to offset om's coords */
{
   int i;
   int npix;				/* number of pixels in OBJMASK */
   PIX **rows;				/* == reg->ROWS */
   PIX *rptr;				/* == &reg->ROWS[y][x] */
   PIX *ptr;				/* pointer to pix */
   SPAN *sp;				/* == om->s[i] */
   int nrow, ncol;			/* == reg->n{row,col} */
   int x1, x2, y;			/* unpacked from sp */

   shAssert(om != NULL && om->npix >= 0);
   shAssert(reg != NULL && reg->type == TYPE_PIX);
/*
 * allocate pix if needs be
 */
   if(pix == NULL) {
      pix = phAiPixNew(om, 1, 0);
   }
/*
 * Set pix.data from the region. We must be careful as the
 * offset OBJMASK may lie outside the REGION
 */
   drow -= reg->row0; dcol -= reg->col0;

   rows = reg->ROWS;
   nrow = reg->nrow; ncol = reg->ncol;
   ptr = pix->mask->data;

   for(i = 0;i < om->nspan;i++) {
      sp = &om->s[i];
      y = sp->y; x1 = sp->x1; x2 = sp->x2;
      npix = x2 - x1 + 1;

      y += drow; x1 += dcol; x2 += dcol;
      if(y >= 0 && y < nrow) {
	 int ncopy = npix;		/* number of pixels to copy */
	 int col0 = x1;			/* starting column in region */
	 int ptr0 = 0;			/* starting index in ptr */

	 if(x1 < 0) {			/* span starts off left of region */
	    col0 = 0;
	    ptr0 = -x1;
	    if(x2 < 0) {
	       ptr0 = npix;
	    }

	    memset(ptr,'\0',ptr0*sizeof(PIX));
	    ncopy -= ptr0;
	 }
	 if(x2 >= ncol) {		/* span extends to right of region */
	    x2 = x2 - ncol;
	    
	    ncopy -= x2;
	    if(ncopy > 0) {		/* x1 is < ncol */
	       memset(&ptr[ptr0 + ncopy],'\0',x2*sizeof(PIX));
	    }
	 }
	 
	 if(ncopy > 0) {
	    rptr = &rows[y][col0];
	    memcpy(&ptr[ptr0],rptr,ncopy*sizeof(PIX));
	 }
      } else {
	 memset(ptr,'\0',npix*sizeof(PIX));
      }
      ptr += npix;
   }
   shAssert(ptr == pix->mask->data + om->npix);

   return(pix);
}

/*****************************************************************************/
/*
 * Set a REGION from the pixels present in an OBJMASK
 *
 * We allow the origin of the region to be different from that in the mask;
 * the convention is that pixel [drow, dcol] in the region is [0,0] in the mask
 */
#define NEXT_SPAN 			/* advance to the next span, if it exists */\
    if (j == om->nspan) {		/* no more spans => no more overlaps */\
        op = NULL;\
	more_overlaps = 0;\
    } else {				/* advance to next span */\
	op = &om->s[j]; j++;\
	oy = op->y; ox1 = op->x1; ox2 = op->x2;\
	oy += odrow; ox1 += odcol; ox2 += odcol;\
	\
        y = -(1 + drow);		/* we'll reconsider this span */ \
	more_overlaps = 1;\
    }

static void
ai_reg_set(const OBJMASK *mm,		/* the OBJMASK specifying the pixels */
	   const AI_PIX *pix,		/* the values to use */
	   const OBJMASK *om,		/* OBJMASK to clip to, or NULL */
	   int odrow, int odcol,	/* how much to offset om's coords relative to mm's */
	   REGION *reg,			/* the region to set */
	   int drow, int dcol,		/* how much to offset mm's coords */
	   int operator,		/* requested operation: \0: set +: add -: subtract */
	   float sky)			/* add this value to all pixels */
{
   int i, j;
   int npix;				/* number of pixels in OBJMASK */
   PIX **rows;				/* == reg->ROWS */
   const PIX *ptr;			/* pointer to pix->mask->data */
   const SPAN *mp;			/* == mm->s[i] */
   int nrow, ncol;			/* == reg->n{row,col} */
   int mx1, mx2, my;			/* unpacked from mp */
   const SPAN *op;			/* == om->s[j] */
   int ox1, ox2, oy;			/* unpacked from op */
   int x1 = 0, x2 = 0, y = 0;		/* a span to set in the image */

   shAssert(operator == '\0' || operator == '+' || operator == '-');
/*
 * If the AI_PIX contains all the pixels in the master_mask it's simple;
 * otherwise we'll reduce it the the simple case
 */
   if(pix->mask->npix == mm->npix) {
       ptr = pix->mask->data;		/* the easy one */
   } else {
       ai_reg_set_val(mm, reg, sky + pix->val, 0, NULL, drow + odrow, dcol + odcol); /* set the entire footprint */
       ai_reg_set(pix->mask, pix, om, odrow, odcol, reg, drow, dcol, operator, sky); /* set the "real" pixels */

       return;
   }
/*
 * Set the pixels in the region.
 *
 * We must be careful as the offset OBJMASK may lie outside the REGION
 */
   ptr = pix->mask->data;
   rows = reg->ROWS;
   nrow = reg->nrow; ncol = reg->ncol;

   j = 0;				/* prepare to walk through om */
   if(om == NULL) {
      op = NULL;
      oy = ox1 = ox2 = 0;
   } else {
      op = &om->s[j]; j++;
      oy = op->y; ox1 = op->x1; ox2 = op->x2;
      oy += odrow; ox1 += odcol; ox2 += odcol;
   }

   for(i = 0;i < mm->nspan;i++) {
      int more_overlaps;		/* do we have more overlaps or mp with op? */

      mp = &mm->s[i];
      my = mp->y; mx1 = mp->x1; mx2 = mp->x2;
      npix = mx2 - mx1 + 1;
      /*
       * Process all overlaps of mp with op
       */
      do {
	  if(op == NULL) {		/* that's easy */
	     if(om == NULL) {		/* no clip mask */
		y = my; x1 = mx1; x2 = mx2;
		more_overlaps = 0;
	     } else {			/* we ran out of spans in om */
		break;
	     }
	  } else {
	     if(my < oy) {
		y = -(1 + drow);	/* i.e. outside region */
		more_overlaps = 0;
	     } else if(my == oy) {
		if(mx2 < ox1) {		/* mp's to left of op */
		   y = -(1 + drow);	/* i.e. outside region */
		   more_overlaps = 0;
		} else if(mx1 > ox2) {	/* mp's to right of op */
		    NEXT_SPAN;
		} else {		/* an intersection */
		    x1 = (mx1 < ox1) ? ox1 : mx1;
		    x2 = (mx2 > ox2) ? ox2 : mx2;
		    NEXT_SPAN;
		    y = my;
		    more_overlaps = 1;	/* there may be more */
		}
	     } else {			/* oy < my */
		 NEXT_SPAN;
	     }
	  }
	  
	  y += drow; x1 += dcol; x2 += dcol;

	  if(y >= 0 && y < nrow) {
	      int col0 = x1;		/* starting column in image */
	      int ncopy = (x2 - x1 + 1);    /* number of pixels to copy */
	      int ptr0 = x1 - (mx1 + dcol); /* starting index in ptr[] */
	      
	      if(x1 < 0) {
		  col0 = 0;
		  ptr0 += (-x1);
		  ncopy -= (-x1);
	      }
	      if(x2 >= ncol) {
		  ncopy -= (x2 - (ncol - 1));
	      }
	      
	      if(ncopy > 0) {
		  if(sky == 0 && operator == '\0') {
		      memcpy(&rows[y][col0],&ptr[ptr0],ncopy*sizeof(PIX));
		  } else {
		      int j;
		      if(operator == '\0') {
			  for(j = 0; j < ncopy; j++) {
			      rows[y][col0 + j] = ptr[ptr0 + j] + sky;
			  }
		      } else if(operator == '+') {
			  for(j = 0; j < ncopy; j++) {
			      rows[y][col0 + j] += ptr[ptr0 + j] - SOFT_BIAS;
			  }
		      } else if(operator == '-') {
			  for(j = 0; j < ncopy; j++) {
			      rows[y][col0 + j] -= ptr[ptr0 + j] - SOFT_BIAS;
			  }
		      }
		  }
	      }
	  }
      } while(more_overlaps);

      ptr += npix;
   }
   shAssert(ptr == pix->mask->data + mm->npix);
}

/*****************************************************************************/
/*
 * Set all the pixels in the OBJMASK to val
 *
 * We allow the origin of the region to be different from that in the mask;
 * the convention is that pixel [drow, dcol] in the region is [0,0] in the mask
 */
static void
ai_reg_set_val(const OBJMASK *om,	/* the OBJMASK specifying the pixels */
	       REGION *reg,		/* the region to set */
	       int val,			/* the desired region */
	       float sigma,		/* s.d. of val */
	       RANDOM *rand,		/* random numbers; may be NULL if
					   sigma <= 0 */
	       int drow, int dcol)	/* how much to offset om's coords */
{
   float fac,r = 0,v1,v2;		/* for Gaussian noise; initialise r to
					   appease the IRIX 5.3 cc */
   int i, j;
   const float inorm = 1.0/(float)((1U<<(8*sizeof(int)-1)) - 1);
   int npix;				/* number of pixels in OBJMASK */
   PIX **rows;				/* == reg->ROWS */
   PIX *rptr;				/* == &reg->ROWS[y][x] */
   SPAN *sp;				/* == om->s[i] */
   int nrow, ncol;			/* == reg->n{row,col} */
   int x1, x2, y;			/* unpacked from sp */
   float var2 = 0;			/* twice the variance corrected
					   for dither noise */
   shAssert(om->npix >= 0);
   shAssert(reg != NULL && reg->type == TYPE_PIX);
/*
 * set up the random number generator
 */
   if(sigma > 0) {
      var2 = 2*(sigma*sigma - 1/12.0);	/* allow for dither noise */

      if(var2 <= 0) {
	 sigma = -1;			/* desired sigma's less than dither
					   noise */
      }
      shAssert(rand != NULL);
   }
/*
 * Set pixels to val; we must be careful as the offset OBJMASK may lie
 * outside the REGION
 */
   rows = reg->ROWS;
   nrow = reg->nrow; ncol = reg->ncol;

   DECLARE_PHRANDOM(rand);

   for(i = 0;i < om->nspan;i++) {
      sp = &om->s[i];
      y = sp->y; x1 = sp->x1; x2 = sp->x2;
      npix = x2 - x1 + 1;

      y += drow; x1 += dcol; x2 += dcol;
      if(y >= 0 && y < nrow) {
	 int ncopy = npix;		/* number of pixels to copy */
	 int col0 = x1;			/* starting column in region */

	 if(x1 < 0) {			/* span starts off left of region */
	    col0 = 0;
	    ncopy -= (-x1);
	 }
	 if(x2 >= ncol) {		/* span extends to right of region */
	    x2 = x2 - ncol;
	    
	    ncopy -= x2;
	 }
	 
	 if(ncopy > 0) {
	    rptr = &rows[y][col0];
	    
	    if(sigma > 0) {
	       j = 0;
	       if(ncopy & 01) {
		  rptr[j++] = val +
		    sqrt(var2/2)*phGaussdev() + (PHRANDOM & 0x1);
	       }
	       
	       while(j < ncopy) {
		  do {
		     v1 = PHRANDOM*inorm;
		     v2 = PHRANDOM*inorm;
		     r = v1*v1+v2*v2;
		  } while (r >= 1.0);
		  fac = sqrt(-var2*log(r)/r);
		  
		  rptr[j++] = val + fac*v1 + (PHRANDOM & 0x1);
		  rptr[j++] = val + fac*v2 + (PHRANDOM & 0x1);
	       }
	    } else {
	       for(j = 0;j < ncopy;j++) {
		  rptr[j] = val;
	       }
	    }
	 }
      }
   }

   END_PHRANDOM(rand);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Set all the pixels in an atlas image that are not in objmask to val
 *
 * The objmask is supposed to be in c's coordinate system
 */
#if !defined(STAND_ALONE)
void
phAtlasImageSetIfNotInObjmask(ATLAS_IMAGE *ai, /* the atlas image */
			      int c,	/* use this band */
			      const OBJMASK *omask, /* not these pixels */
			      const PIX val) /* value to use */
{
   shAssert(omask != NULL);
   shAssert(ai != NULL && ai->master_mask != NULL);
   shAssert(c >= 0 && c < ai->ncolor && ai->pix[c] != NULL);
   shAssert(ai->pix[c]->mask != NULL);

   {
       AI_PIX *aip = ai->pix[c];
       OBJMASK *aimask = aip->mask;
       OBJMASK *my_omask = NULL;	/* copy of omask in which I can set OBJMASK->data (in master_mask's frame) */
       const int drow = ai->drow[c];
       const int dcol = ai->dcol[c];
       const int rmin = (omask->rmin - drow < aimask->rmin) ? omask->rmin - drow : aimask->rmin;
       const int rmax = (omask->rmax - drow > aimask->rmax) ? omask->rmax - drow : aimask->rmax;
       const int cmin = (omask->cmin - dcol < aimask->cmin) ? omask->cmin - dcol : aimask->cmin;
       const int cmax = (omask->cmax - dcol > aimask->cmax) ? omask->cmax - dcol : aimask->cmax;
       REGION *tmpReg = shRegNew(__func__, rmax - rmin + 1, cmax - cmin + 1, TYPE_PIX);
       tmpReg->row0 = rmin; tmpReg->col0 = cmin;

       if (aimask->npix != ai->master_mask->npix) {
	   phRegionSetValFromObjmask(tmpReg, aimask, aip->val);
       }
       phRegionSetFromObjmask(tmpReg, aimask); /* aimask fully inserted */

       my_omask = phObjmaskCopy(omask, -drow, -dcol);
       phObjmaskSetFromRegion(my_omask, tmpReg); /* save part in omask */
       
       shRegIntSetVal(tmpReg, val);
       phRegionSetFromObjmask(tmpReg, my_omask); /* reinsert desired pixels */
       phObjmaskDel(my_omask);

       phObjmaskSetFromRegion(aimask, tmpReg);
       shRegDel(tmpReg);

       aip->val = val;
   }       
}
#endif

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Set an atlas image to a constant
 *    "ai->pix[c] = val;"
 *
 * See also phAtlasImages{Plus,Minus}Equals()
 */
#if !defined(STAND_ALONE)
void
phAtlasImageConstSet(ATLAS_IMAGE *ai,	/* atlas image to add to */
		     int c,		/* which color to use */
		     const float val)	/* the constant to set */
{
   PIX *end;				/* == ptr + npix */
   PIX *ptr;				/* == ai->pix[c] */

   shAssert(c >= 0 && c < ai->ncolor);
   shAssert(ai != NULL);
   shAssert(ai->pix[c] != NULL);
   shAssert(ai->pix[c]->mask != NULL);

   ptr = ai->pix[c]->mask->data;
   end = ptr + ai->pix[c]->mask->npix;

   while(ptr < end) {
       *ptr++ = val;
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Add a constant to an atlas image
 *    "ai->pix[c] += val;"
 *
 * See also phAtlasImages{Plus,Minus}Equals()
 */
void
phAtlasImageConstAdd(ATLAS_IMAGE *ai,	/* atlas image to add to */
		     int c,		/* which color to use */
		     const float val,	/* the constant to add */
		     const int dither)	/* dither, not round? */
{
   PIX *end;				/* == ptr + npix */
   PIX *ptr;				/* == ai->pix[c]->mask->data */

   if (val == 0.0) {
       return;
   }

   shAssert(c >= 0 && c < ai->ncolor);
   shAssert(ai != NULL);
   shAssert(ai->pix[c] != NULL);
   shAssert(ai->pix[c]->mask != NULL);
   ptr = ai->pix[c]->mask->data;
   end = ptr + ai->pix[c]->mask->npix;

   ai->pix[c]->val += val;
#if FLOATING_PHOTO
   while(ptr < end) {
       *ptr++ += val;
   }
#else
   {
      float tmp;
      if(dither) {
	 while(ptr < end) {
	    tmp = *ptr + val + phRandomUniformdev();
	    *ptr++ = (tmp >= 0) ? (tmp <= MAX_U16 ? tmp : MAX_U16) : 0;
	 }
      } else {
	 while(ptr < end) {
	    tmp = *ptr + val + 0.5;
	    *ptr++ = (tmp >= 0) ? (tmp <= MAX_U16 ? tmp : MAX_U16) : 0;
	 }
      }
   }
#endif
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Add a constant to atlas image pixels equal to SOFT_BIAS
 *    "ai->pix[c] += (val == SOFT_BIAS) ? val : 0;"
 */
void
phAtlasImageConstAddIfSoftBias(ATLAS_IMAGE *ai,	/* atlas image to add to */
			       int c,		/* which color to use */
			       const float val,	/* the constant to add */
			       const int dither) /* dither, not round? */
{
   PIX *end;				/* == ptr + npix */
   PIX *ptr;				/* == ai->pix[c]->mask->data */

   if (val == 0.0) {
       return;
   }

   shAssert(c >= 0 && c < ai->ncolor);
   shAssert(ai != NULL);
   shAssert(ai->pix[c] != NULL);
   shAssert(ai->pix[c]->mask != NULL);
   ptr = ai->pix[c]->mask->data;
   end = ptr + ai->pix[c]->mask->npix;

   ai->pix[c]->val += val;
#if FLOATING_PHOTO
   while(ptr < end) {
       if (*ptr == SOFT_BIAS) {
	   *ptr += val;
       }
       ptr++;
   }
#else
   {
      float tmp;
      if(dither) {
	 while(ptr < end) {
	    tmp = *ptr;
	    if (tmp == SOFT_BIAS) {
		tmp += val + phRandomUniformdev();
		*ptr = (tmp >= 0) ? (tmp <= MAX_U16 ? tmp : MAX_U16) : 0;
	    }
	    ptr++;
	 }
      } else {
	 while(ptr < end) {
	    tmp = *ptr;
	    if (tmp == SOFT_BIAS) {
		tmp += val + 0.5;
		*ptr = (tmp >= 0) ? (tmp <= MAX_U16 ? tmp : MAX_U16) : 0;
	    }
	    ptr++;
	 }
      }
   }
#endif
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Copy the pixel values from one atlas_image in one colour to another
 */
void phAtlasImagesPixelsCopy(ATLAS_IMAGE *ai, int ai_c, const ATLAS_IMAGE *cai, int cai_c)
{
    if (ai == cai && ai_c == cai_c) {
	return;				/* nothing to do */
    }

   shAssert(ai != NULL && cai != NULL);
   shAssert(ai->npix == cai->npix);
   shAssert(ai_c >= 0 && ai_c < ai->ncolor);
   shAssert(cai_c >= 0 && cai_c < cai->ncolor);

   ai->pix[ai_c]->val = cai->pix[cai_c]->val;

   if (!phObjmasksEqual(ai->pix[ai_c]->mask, cai->pix[cai_c]->mask)) {
       phAiPixFree(ai->pix[ai_c]);
       ai->pix[ai_c] = phAiPixNew(cai->pix[cai_c]->mask, 1, cai->pix[cai_c]->val);
   }
   
   memcpy(ai->pix[ai_c]->mask->data, cai->pix[cai_c]->mask->data, ai->pix[ai_c]->mask->npix*sizeof(PIX));
}

#endif

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Add the pixels in one atlas image to another:
 *    "ai->pix[ai_c] += cai->pix[cai_c] + bias;"
 *
 * Both atlas images must be the same size.
 * If {ai,cai}_c are in range, just add those bands; if they are -1, add all bands
 *
 * See also phAtlasImagesMinusEquals()
 */
#if !defined(STAND_ALONE)
void
phAtlasImagesPlusEquals(ATLAS_IMAGE *ai, /* atlas image to add to */
			int ai_c,	/* colour to use in ai, or -1 */
			const ATLAS_IMAGE *cai,	/* atlas image to add */
			int cai_c,	/* which color to use in cai, or -1 */
			int bias)	/* value to add to cai->pix[] */
{
   PIX *ai_pix;				/* == ai->pix[c] */
   const PIX *cai_pix;			/* == cai->pix[c] */
   int ai_c0, ai_c1;			/* range of colours in ai to add */
   int cai_c0, cai_c1;			/* range of colours in cai to add */
   int c;
   int i;
   int npix;				/* == ai->npix */

   shAssert(ai != NULL && cai != NULL);
   shAssert(ai->ncolor == cai->ncolor && ai->npix == cai->npix);
   shAssert(ai_c >= -1 && ai_c < ai->ncolor);
   shAssert(cai_c >= -1 && cai_c < cai->ncolor);
   npix = ai->npix;

   if(ai_c == -1) {
       shAssert(cai_c == -1);       
       ai_c0 = 0; ai_c1 = ai->ncolor;
       cai_c0 = 0; cai_c1 = cai->ncolor;
   } else {
       ai_c0 = ai_c; ai_c1 = ai_c + 1;
       cai_c0 = cai_c; cai_c1 = cai_c + 1;
   }

   for(c = ai_c0; c < ai_c1; c++) {
      phAtlasImagePixelExpand(ai, c, 1);
      phAtlasImagePixelExpand(cai, c, 1);

      ai_pix = ai->pix[c]->mask->data;
      cai_pix = cai->pix[c + (cai_c0 - ai_c0)]->mask->data;

      if(bias == 0) {
	 for(i = 0; i < npix; i++) {
	    ai_pix[i] += cai_pix[i];
	 }
      } else {
	 for(i = 0; i < npix; i++) {
	    ai_pix[i] += cai_pix[i] + bias;
	 }
      }
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Multiply an atlas image by a constant
 *    "ai->pix[ai_c] *= const"
 *
 * If ai_c is in range, just add process that band; if it's -1, handle all bands
 *
 * See also phAtlasImagesMinusEquals()
 */
void
phAtlasImagesTimesEquals(ATLAS_IMAGE *ai, /* atlas image to add to */
			 int ai_c,	/* colour to use in ai, or -1 */
			 float val)	/* value to multiply ai by */
{
   PIX *ai_pix;				/* == ai->pix[c] */
   int ai_c0, ai_c1;			/* range of colours in ai to add */
   int c;
   int i;
   int npix;				/* == ai->npix */

   shAssert(ai != NULL);
   shAssert(ai_c >= -1 && ai_c < ai->ncolor);
   npix = ai->npix;

   if(ai_c == -1) {
       ai_c0 = 0; ai_c1 = ai->ncolor;
   } else {
       ai_c0 = ai_c; ai_c1 = ai_c + 1;
   }

   for(c = ai_c0; c < ai_c1; c++) {
      phAtlasImagePixelExpand(ai, c, 1);

      ai_pix = ai->pix[c]->mask->data;
      for(i = 0; i < npix; i++) {
	  ai_pix[i] *= val;
      }
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Subtract the pixels in one atlas image from another:
 *    "ai->pix[ai_c] -= cai->pix[cai_c] + bias;"
 *
 * Both atlas images must be the same size.
 * If {ai,cai}_c are in range, just subtract those bands; if they are -1, subtract all bands
 *
 * See also phAtlasImagesPlusEquals()
 */
void
phAtlasImagesMinusEquals(ATLAS_IMAGE *ai, /* atlas image to subtract from */
			 int ai_c,	/* colour to use in ai, or -1 */
			 const ATLAS_IMAGE *cai,	/* atlas image to subtract */
			 int cai_c,	/* which color to use in cai, or -1 */
			 int bias)	/* value to add to cai->pix[] before subtracting */
{
   PIX *ai_pix;				/* == ai->pix[c] */
   const PIX *cai_pix;			/* == cai->pix[c] */
   int ai_c0, ai_c1;			/* range of colours in ai to add */
   int cai_c0, cai_c1;			/* range of colours in cai to add */
   int c;
   int i;
   int npix;				/* == ai->npix */

   shAssert(ai != NULL && cai != NULL);
   shAssert(ai->ncolor == cai->ncolor && ai->npix == cai->npix);
   shAssert(ai_c >= -1 && ai_c < ai->ncolor);
   shAssert(cai_c >= -1 && cai_c < cai->ncolor);
   npix = ai->npix;

   if(ai_c == -1) {
       shAssert(cai_c == -1);       
       ai_c0 = 0; ai_c1 = ai->ncolor;
       cai_c0 = 0; cai_c1 = cai->ncolor;
   } else {
       ai_c0 = ai_c; ai_c1 = ai_c + 1;
       cai_c0 = cai_c; cai_c1 = cai_c + 1;
   }

   for(c = ai_c0; c < ai_c1; c++) {
      phAtlasImagePixelExpand(ai, c, 1);
      phAtlasImagePixelExpand(cai, c, 1);

      ai_pix = ai->pix[c]->mask->data;
      cai_pix = cai->pix[c + (cai_c0 - ai_c0)]->mask->data;

      if(bias == 0) {
	 for(i = 0; i < npix; i++) {
	    ai_pix[i] -= cai_pix[i];
	 }
      } else {
	 for(i = 0; i < npix; i++) {
	    ai_pix[i] -= cai_pix[i] + bias;
	 }
      }
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Calculate the dot product of two atlas images
 *    "(ai1->pix[c1] - bkgd1).(ai2->pix[c2] + bkgd2);"
 *
 * Both atlas images must be the same size.
 */
double
phAtlasImageDotProduct(const ATLAS_IMAGE *ai1, /* one atlas image */
		       int c1,		/* colour to use in ai1 */
		       double bkgd1,	/* pedestal to subtract from ai1's values */
		       const ATLAS_IMAGE *ai2,	/* other atlas image */
		       int c2,		/* which color to use in ai2 */
		       double bkgd2)	/* pedestal to subtract from ai's values */
{
   shAssert(ai1 != NULL && ai2 != NULL);
   shAssert(ai1->npix == ai2->npix);
   shAssert(c1 >= 0 && c1 < ai1->ncolor);
   shAssert(c2 >= 0 && c2 < ai2->ncolor);    

   return phObjmaskDotProduct(ai1->pix[c1]->mask, bkgd1, ai2->pix[c2]->mask, bkgd2, 0.0, 0.0);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Return the largest pixel value in an atlas image
 */
double
phAtlasImageMaximum(const ATLAS_IMAGE *ai, /* the atlas image */
		    int c)		/* colour to use in ai1 */
{
   shAssert(ai != NULL);
   shAssert(c >= 0 && c < ai->ncolor);
   {
      int i;
      int max = -1e9;			/* maximum value of template */
      const PIX *pix = ai->pix[c]->mask->data; /* unalias */
      const int npix = ai->pix[c]->mask->npix; /* unalias */
      int val;				/* == pix[i] */
      
      for(i = 0; i < npix; i++) {
	 val = pix[i];
	 if(val != 0) {
	     if(val > max) {
	       max = val;
	    }
	 }
      }

      return (ai->pix[c]->val > max) ? ai->pix[c]->val : max;
   }
}
   
/*****************************************************************************/
/*
 * Trim an OBJC's atlas image to lie within the specified rectangle
 *
 * Return 0 if all is well; -1 if the trimmed object has no pixels
 */
int
phAtlasImageTrimToRect(OBJC *objc,	/* the object in question */
		       int rmin, int cmin, /* LLC of rectangle */
		       int rmax, int cmax) /* URC of rectangle */
{
   ATLAS_IMAGE *aimage;			/* == objc->aimage */
   int c;
   OBJMASK *tmp;

   shAssert(objc != NULL && objc->aimage != NULL);
   aimage = objc->aimage;
   shAssert(aimage->master_mask != NULL);

   tmp = phTrimObjmaskToRect(aimage->master_mask, cmin, rmin, cmax, rmax);
   if (tmp == NULL) {			/* nothing survived the trimming */
       return -1;
   }
   phObjmaskDel(aimage->master_mask); aimage->master_mask = tmp;
   aimage->npix = aimage->master_mask->npix;

   for (c = 0; c < aimage->ncolor; c++) {
       if (aimage->pix[c] != NULL) {
	   tmp = phTrimObjmaskToRect(aimage->pix[c]->mask, cmin, rmin, cmax, rmax);
	   shAssert (tmp != NULL);			/* something survived the trimming */
	   phObjmaskDel(aimage->pix[c]->mask); aimage->pix[c]->mask = tmp;
       }
   }

   return 0;
}

/************************************************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Return/Set the ith pixel of an atlas image in the specified band
 *
 * N.b. no checking of inputs -- this routine needs to be fast.  It
 * is used to hide the implementation of pixel storage in the atlas
 * image from the outside world
 */
PIX
phAtlasImagePixelGet(const ATLAS_IMAGE *ai, /* the atlas image */
		     int c,		/* requested colour */
		     int row, int col)	/* desired pixel (in reference coordinate system) */
{
    const AI_PIX *aip = ai->pix[c];
    const OBJMASK *aipmask = aip->mask;
    const int i = phObjmaskFindPixel(aipmask, row, col);

    return (i < 0) ? aip->val : aip->mask->data[i];
}

void
phAtlasImagePixelSet(ATLAS_IMAGE *ai,	/* the atlas image */
		     int c,		/* requested colour */
		     int row, int col,	/* desired pixel (in reference coordinate system) */
		     PIX val)		/* value to set pixel to */
{
    AI_PIX *aip = ai->pix[c];
    OBJMASK *aipmask = aip->mask;
    const int i = phObjmaskFindPixel(aipmask, row, col);
    
    if(i >= 0) {
	aip->mask->data[i] = val;
    } else {
	aip->val = val;
    }
}

/*****************************************************************************/
/*
 * Return the index in the ->pix arrays of the pixel (row, col)
 */
int
phFindAtlasImagePixel(const ATLAS_IMAGE *aimage, /* the atlas image */
			int row, int col) /* the desired pixel */
{
   if(aimage == NULL || aimage->master_mask == NULL) {
      shError("atlas image or master mask is NULL");
      return(-1);
   }

   return phObjmaskFindPixel(aimage->master_mask, row, col);
}
#endif
