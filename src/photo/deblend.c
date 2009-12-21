/*
 * Do the work of deblending OBJCs
 *
 * The algorithm is described in $PHOTO_DIR/doc/overview/deblend.html
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <alloca.h>
#include "dervish.h"
#include "phMeasureObj.h"
#include "phObjects.h"
#include "phObjectCenter.h"
#include "phPeaks.h"
#include "phUtils.h"
#include "phMathUtils.h"
#include "phCellFitobj.h"
//#include "phMergeColors.h"
#include "phOffset.h"

static REGION *scr0 = NULL;
static REGION *scr1 = NULL;
static REGION *scr2 = NULL;
static MASK *mscr0 = NULL;

void
phObjcCenterCalc(OBJC *objc,		/* the object in question */
		 const FIELDPARAMS *fparams, /* properties of field */
		 int use_color)		/* use colour information? */
	;

OBJC *
phObjcChildNew(OBJC *objc,		/* the parent */
	       const PEAK *peak,	/* which peak? */
	       const FIELDPARAMS *fiparams, /* gain etc. */
	       int is_child)		/* make new OBJC a child, not a sibb */
	;

static void
average_peak_centers(const PEAK *peak1,	/* the peaks */
		     const PEAK *peak2,	/*         in question */
		     float *rowc, float *colc)	/* mean position */
	;

static void
merge_peaks(PEAK *peak_i,
	    PEAK *peak_j)
	;

static float
assign_missing_flux(const OBJC **children, /* the children in question */
		    int nchild,		/* number of children */
		    int c,		/* in this band */
		    const float *Is2)	/* children's I*sigma^2 values */
	;

static float
find_Isigma2(const OBJC *objc,		/* the object in question */
	     int c,			/* in this band */
	     int bkgd)			/* background level */
	;

static int
peephole_optimizer(OBJC *objc,		/* parent */
		   const FIELDPARAMS *fiparams, /* astrometry etc. */
		   int nchild,		/* number of children */
		   OBJC *children[],	/* list of children */
		   ATLAS_IMAGE **smoothed_ai)/* list of atlas images containing
						smoothed templates. Not used,
						but may need to be freed */
	;


static int
reject_template(OBJC *objc,
		int nchild,
		int reject,
		OBJC *children[],
		ATLAS_IMAGE *smoothed_ai[],
		MAT *A[],
		VEC *b[],
		VEC *norm[],
		VEC *lambda[],
		MAT *Q[],
		VEC *w[])
	;

static void
transfer_centers_and_peaks(OBJC *keep, OBJC *reject)
	;

static void
sort_Q(const int ncolor,		/* dimen of lambda and Q arrays */
       const int canonical_color,	/* sort to match this colour */
       VEC **lambda,			/* eigenvalues */
       MAT **Q)				/* eigenvectors */
	;

static void
setup_normal(const OBJC *parent,	/* parent object */
	     const OBJC **children,	/* array of children */
	     int nchild,		/* dimension of children[] */
	     int c,			/* the colour in question */
	     int bkgd,			/* == (bkgd from fparams) + SOFT_BIAS*/
	     MAT *A,			/* The LSQ problem is */
	     VEC *b,			/*    A w = b */
	     VEC *norm)			/* normalisation of templates */
	;

static int
maybe_deblend_at_edge(OBJC *objc,		/* object to deblend */
		      const FIELDPARAMS *fiparams, /* gain etc. */
		      int filtsize)		   /* smoothing filter size */
	;
int
phObjcDeblendMovingChild(OBJC *objc,	/* OBJC to deblend */
			 const FIELDPARAMS *fiparams) /* info about frame */
	;
void
phObjcChildDel(OBJC *child)		/* the child to destroy */
	;

static int
deblend_template_find(OBJC *objc,	/* the OBJC in question */
		      OBJMASK **psfmasks, /* masks for pixels in PSFs */
		      const FIELDPARAMS *fiparams, /* gain etc. */
		      ATLAS_IMAGE **smoothed_ai, /* ATLAS_IMAGE containing
						   smoothed templates */
		      int filtsize,	/* size of smoothing filter */
		      int ngrow)	/* how many pixels to grow objects */
	;

static void
set_parent_position_from_child(OBJC *objc,
			       const OBJC *child)
	;
static void
average_templates(OBJC *child)
	;


/*****************************************************************************/
/*
 * Look at an object with the EDGE flag set and determine if it'll be
 * EDGE objects on the neighbouring frame too; if so, take heroic measures
 * to split it enough to be deblended
 *
 * Return 0 if the deblender can proceed, or -1 if it's hopeless
 */
static int
maybe_deblend_at_edge(OBJC *objc,		/* object to deblend */
		      const FIELDPARAMS *fiparams, /* gain etc. */
		      int filtsize)		   /* smoothing filter size */
{
   int aimage_drow, aimage_dcol;	/* == objc->aimage->d{row,col}*/
   int c;				/* colour counter */
   int cmin, cmax, rmin, rmax;		/* corner of master_mask, as corrected
					   by objc->aimage->d{row,col}[] */
   int rmin_max, cmin_max, rmax_min, cmax_min; /* {min,max} allowable values
						  of mmask's bounding box */
   int edge_in_all;			/* will object always touch edge? */
   int fuzz;				/* fuzz in whether objects touch edge*/
   OBJMASK *mmask;			/* == objc->aimage->master_mask */
   int ncol;
   int nrow;
   int run_overlap;
   int scan_overlap;

   shAssert(fiparams);
   shAssert(fiparams->frame[0].data);
   ncol = fiparams->frame[0].data->ncol;
   nrow = fiparams->frame[0].data->nrow;
   run_overlap = fiparams->run_overlap; /* side-to-side overlap */
   scan_overlap = fiparams->scan_overlap; /* top-to-bottom overlap */
/*
 * does the object touch both sides of an overlap region in some band?
 *
 * Due to smoothing, an object needn't actually touch the edge of
 * the frame to be EDGE, so merely require that one corner of its
 * mask be within the edge region and one without
 */
   mmask = objc->aimage->master_mask;
   shAssert(mmask != NULL);

   cmin = cmax = rmin = rmax = 0;	/* make gcc happy */

   rmin_max = mmask->rmin;
   cmin_max = mmask->cmin;
   rmax_min = mmask->rmax;
   cmax_min = mmask->cmax;
   
   fuzz = 0;
   for(c = 0; c < objc->ncolor; c++) {
      if(fabs(objc->aimage->drow[c]) > fuzz) {
	 fuzz = fabs(objc->aimage->drow[c]);
      }
      if(fabs(objc->aimage->dcol[c]) > fuzz) {
	 fuzz = fabs(objc->aimage->dcol[c]);
      }
   }
/*
 * If we're binned, and some of the chips are 1-amp and some are 2-amp
 * there are "missing pixels" between the left and the right half of
 * the data in 2-amp CCDs. If some of the bands use 1-amp and some 2-amp
 * chips, this leads to an extra offset of (I think) 2*(bin - 1)/bin
 * binned pixels; certainly the value is 1.5 for bin == 4.
 *
 * Actually, bin == 2 is OK as we can recover the missing 1 column from
 * the overclock/serial data, but let's not assume that this has been done
 *
 * Rather than deal with this carefully, simply increase the fuzz by
 * a suitable amount
 */
   if (fiparams->frame[0].colBinning > 1) {
     fuzz += 2*(fiparams->frame[0].colBinning - 1)/
				 (float)fiparams->frame[0].colBinning + 0.9999;
   }

   edge_in_all = 0;
   for(c = 0; c < objc->ncolor; c++) {
      aimage_drow = objc->aimage->drow[c];
      aimage_dcol = objc->aimage->dcol[c];
      cmin = mmask->cmin + aimage_dcol;
      rmin = mmask->rmin + aimage_drow;
      cmax = mmask->cmax + aimage_dcol;
      rmax = mmask->rmax + aimage_drow;

      if(rmin < fuzz && rmax > scan_overlap - fuzz) {
	 edge_in_all++;
	 if(fuzz - aimage_drow > rmin_max) {
	    rmin_max = fuzz - aimage_drow;
	 }
      }
      if(rmin < nrow - scan_overlap + fuzz && rmax >= nrow - 1 - fuzz) {
	 edge_in_all++;
	 if((nrow - 1 - fuzz) - aimage_drow < rmax_min) {
	    rmax_min = (nrow - 1 - fuzz) - aimage_drow;
	 }
      }
      if(cmin < fuzz && cmax > run_overlap - fuzz) {
	 edge_in_all++;
	 if(fuzz - aimage_dcol > cmin_max) {
	    cmin_max = fuzz - aimage_dcol;
	 }
      }
      if(cmin < ncol - run_overlap + fuzz && cmax >= ncol - 1 - fuzz) {
	 edge_in_all++;
	 if((ncol - 1 - fuzz) - aimage_dcol < cmax_min) {
	    cmax_min = (ncol - 1 - fuzz) - aimage_dcol;
	 }
      }
   }
/*
 * Will this object be deblended in some other fields/strips?
 * If so, give up now
 */
#define ALWAYS_DEBLEND 1
#if !ALWAYS_DEBLEND
   if(edge_in_all == 0) {		/* not an edge object in all fields */
      return(-1);
   }
#endif
/*
 * The object will be rejected wherever it's found. Do the work required to
 * make it deblendable in this frame
 *
 * We've just pasted the original object into the frame, so if we trim
 * the atlas image's master_mask some pixels will be left behind when
 * we proceed to the next object.  To fix this, remove it now and
 * reinstate it after trimming
 */
#if ALWAYS_DEBLEND
   {
      int margin = filtsize + fuzz;
      
      for(c = 0; c < objc->ncolor; c++) {
	aimage_drow = objc->aimage->drow[c];
	aimage_dcol = objc->aimage->dcol[c];
	if(c == 0 || mmask->cmin + aimage_dcol < cmin) {
	   cmin = mmask->cmin + aimage_dcol;
	}
	if(c == 0 || mmask->rmin + aimage_drow < rmin) {
	   rmin = mmask->rmin + aimage_drow;
	}
	if(c == 0 || mmask->cmax + aimage_dcol > cmax) {
	  cmax = mmask->cmax + aimage_dcol;
	}
	if(c == 0 || mmask->rmax + aimage_drow > rmax) {
	  rmax = mmask->rmax + aimage_drow;
	}
      }

      rmin_max = (rmin <= margin) ? margin + 1 : rmin;
      cmin_max = (cmin <= margin) ? margin + 1 : cmin;
      rmax_min = (rmax >= nrow - margin) ? nrow - margin - 1 : rmax;
      cmax_min = (cmax >= ncol - margin) ? ncol - margin - 1 : cmax;

      if(rmin_max == mmask->rmin && cmin_max == mmask->cmin &&
	 rmax_min == mmask->rmax && cmax_min == mmask->cmax) {
	 return 0;			/* no need to trim object */
      }

      if (rmin_max > rmax_min || cmin_max > cmax_min) {	/* no pixels left */
#if 0
	  shError("all pixels in object at edge were trimmed while attempting to deblend");
#endif
	  return -1;
      }
   }
#endif
/*
 * Set DEBLENDED_AT_EDGE iff we touch the edge
 */
   for(c = 0; c < objc->ncolor; c++) {
       if (objc->color[c]->flags & OBJECT1_EDGE) {
	   objc->flags2 |= OBJECT2_DEBLENDED_AT_EDGE;
	   break;
       }
   }

#if 1
   phRemoveAtlasImage(objc, fiparams);
   if (phAtlasImageTrimToRect(objc, rmin_max, cmin_max, rmax_min, cmax_min) < 0) {
      objc->flags |= OBJECT1_NODEBLEND;
      return(-1);
   }

   phInsertAtlasImage(objc, fiparams);

   {
      OBJC *child;			/* trim children too */
      (void)phObjcDescendentNext(objc);	/* returns objc */
      while((child = phObjcDescendentNext(NULL)) != NULL) {
	 const int i = phAtlasImageTrimToRect(child, rmin_max, cmin_max, rmax_min, cmax_min);
	 shAssert(i == 0);
	 child->flags2 |= OBJECT2_DEBLENDED_AT_EDGE;
      }
   }
#endif
   
   return(0);
}


/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Actually deblend an object. At the end of this routine, each child knows
 * the values of all its pixels that lie within the OBJMASK defining the
 * parent, and is ready to be passed to Measure Objects
 *
 * The children were created in an earlier call to phObjcMakeChildren()
 *
 * Return 0 if OK, -1 in case of trouble
 */
int
phObjcDeblend(OBJC *objc,		/* object to deblend */
	      const FIELDPARAMS *fiparams) /* gain, smoothing sigma, etc. */
{
   MAT *A[NCOLOR];			/* normal equations for LSQ fit are */
   VEC *b[NCOLOR];			/*      A*w = b */
   ATLAS_IMAGE **smoothed_ai;		/* list of atlas images containing
					   smoothed templates */
   int bin = 16;			/* how to bin when finding centre */
   int bkgd;				/* == bkgd from fiparams + SOFT_BIAS */
   int c;
   int central_child;			/* indx of child closest to centre */
   OBJC *child;
   OBJC **children;			/* list of all children */
   int filtsize;			/* size of smoothing filter */
   static int skip_deblend = 0;		/* skip the actual deblending? */
   int i, j;
   float i_nchild;			/* == 1/(float)nchild */
   float I0;				/* amplitude of a PSF */
   float I0_min[NCOLOR];		/* min. acceptable amplitude of PSF */
   float *Is2;				/* I*sigma^2 for each child */
   VEC *lambda[NCOLOR];			/* eigen values */
   VEC *norm[NCOLOR];			/* template's normalisation */
   static float min_lambda = 1e-5;	/* minimum acceptable eigenvalue */
   int nchild;				/* number of children */
   int ngrow = 5;			/* how far to grow profiles */
   int nparam;				/* number of parameters in the fit */
   int npix;				/* number of pixels in master_mask */
   OBJECT1 *obj1;			/* == child->color[c] */
   float pedestal;			/* pedestal that a peak is sitting on*/
   float psf_Lmin;			/* minimum L to be considered a star */
   int psf_nann[NCOLOR];		/* number of annuli for PSF fit */
   int psf_nann0;			/* number of annuli for PSF fit,
					   in width = 1 pixel seeing */
   float psf_rad[NCOLOR];		/* radius to subtract to */
   float psf_rad0;			/* radius to subtract PSF to
					   in width = 1 pixel seeing */
   DGPSF *psfs[NCOLOR];			/* PSFs in each band; parameters
					   from double Gaussian fit */
   MAT *Q[NCOLOR];			/* eigen vectors */
   OBJMASK *psfmasks[NCOLOR];		/* masks for pixels that are in PSFs */
   ATLAS_IMAGE *subtracted;		/* (original data) - PSFs */
   float val;				/* value of a pixel */
   VEC *w[NCOLOR];			/* desired weights in each colour */
   int x, y;				/* column/row coordinates of a pixel */

   shAssert(objc != NULL);
   shAssert(objc->children != NULL || objc->sibbs != NULL);
   shAssert(objc->aimage != NULL);
   shAssert(fiparams != NULL && fiparams->deblend_psf_Lmin >= 0);
   psf_Lmin = fiparams->deblend_psf_Lmin;
   psf_nann0 = fiparams->deblend_psf_nann;
   psf_rad0 = fiparams->deblend_psf_rad_max;
   {
      float sigma_max = 0;		/* largest sigma1 in any band */
      for(i = 0;i < objc->ncolor;i++) {
	 if(fiparams->frame[i].smooth_sigma > sigma_max) {
	    sigma_max = fiparams->frame[i].smooth_sigma;
	 }
      }
      filtsize = 2*(int)(3*sigma_max) + 1;
   }
/*
 * give up on edge objects; they don't satisfy the the deblender's symmetry
 * assumptions and are too close to the edge to be smoothed. Some objects
 * may not (yet) be identified as edge, so call maybe_deblend_at_edge() on
 * all objects.
 *
 * In the case of large enough objects we mustn't just give up as they
 * wouldn't be deblended in any field
 */
   if(maybe_deblend_at_edge(objc, fiparams, filtsize) < 0) { /* no need for heroics */
      int flags = objc->flags;
      for(i = 0; i < objc->ncolor; i++) {
	 flags |= objc->color[i]->flags;
      }

      if(flags & (OBJECT1_EDGE | OBJECT1_NODEBLEND)) {
	 objc->flags |= OBJECT1_EDGE | OBJECT1_NODEBLEND;
	 return(-1);
      }
   }

/*
 * Put pointers to the descendents into an array, children[], for the
 * convenience of this routine
 */
   children = shMalloc(objc->nchild*sizeof(OBJC *));
   
   nchild = 0;
   (void)phObjcDescendentNext(objc);	/* returns objc */
   while((child = phObjcDescendentNext(NULL)) != NULL) {
      children[nchild++] = child;
   }
   shAssert(nchild == objc->nchild);

/*
 * decide whether any possible moving objects are really moving; if they
 * are, deblend them as such
 */
   if(objc->flags & OBJECT1_MOVED) {	/* there may be a moving child */
      for(i = 0;i < nchild;i++) {
	 child = children[i];
	 
	 if(child->flags2 & OBJECT2_DEBLENDED_AS_MOVING) {
	    if(phObjcDeblendMovingChild(child, fiparams) == 0) {
/*
 * Yes; deblend it as moving. Delete the other children that share the same
 * peaks
 */
#if !defined(NDEBUG)			/* only one moving child */
	       for(i++;i < nchild;i++) {
		  child = children[i];
		  shAssert(!(child->flags2 & OBJECT2_DEBLENDED_AS_MOVING));
	       }
#endif

	       for(i = 0;i < nchild;i++) {
		  child = children[i];
		  if((child->flags & OBJECT1_MOVED) &&
		     !(child->flags2 & OBJECT2_DEBLENDED_AS_MOVING)) {
		     phObjcChildDel(children[i]); children[i] = NULL;
		  }
	       }
	    } else {
/*
 * No; don't deblend it as a moving object.
 * It was deleted by phObjcDeblendMovingChild
 */
	       children[i] = NULL;
	    }
	    break;
	 }
      }
/*
 * remove deleted children
 */
      for(i = j = 0;i < nchild;i++) {
	 if(children[i] != NULL) {
	    children[j++] = children[i];
	 }
      }
      nchild = j;
   }

   printf("Bailing out of deblending...\n");

#if defined(NOPE)

/*
 * find the centre of the object after median smoothing. We use this to try
 * to ensure that the centres of big bright galaxies aren't deblended as PSFs
 */
   npix = objc->aimage->master_mask->npix;
   c = fiparams->ref_band_index;
   if(!(objc->color[c]->flags & OBJECT1_DETECTED) || npix < 4*bin*bin) {
      central_child = 0;
   } else {
      const int aimage_drow = objc->aimage->drow[c];
      const int aimage_dcol = objc->aimage->dcol[c];
      int bin_r = bin, bin_c = bin;	/* actual adopted binning */
      BINREGION *binreg;		/* the desired BINREGION */
      const REGION *data = fiparams->frame[c].data;
      int minval, maxval;		/* range of pixel values to histogram*/
      int cmax = objc->aimage->master_mask->cmax + aimage_dcol;
      int cmin = objc->aimage->master_mask->cmin + aimage_dcol;
      int rmax = objc->aimage->master_mask->rmax + aimage_drow;
      int rmin = objc->aimage->master_mask->rmin + aimage_drow;
      PEAKS *peaks = phPeaksNew(1);	/* peak of object */
      PEAK *peak;			/* == peaks->peaks[0] */
      int r0, c0;			/* a child's peak pixel */
      float r2;				/* distance of peak from centre */
      float r2_min;			/* minimum value of r2 */
      int size = 5;			/* size of area to search for SATUR */
      const SPANMASK *sm = (SPANMASK *)data->mask;
      REGION *sub;			/* subregion including master_mask */

      if(cmin < 0) cmin = 0;
      if(cmax >= data->ncol) cmax = data->ncol - 1;
      if(rmin < 0) rmin = 0;
      if(rmax >= data->nrow) rmax = data->nrow - 1;

      sub = shSubRegNew("phObjcDeblend", fiparams->frame[c].data,
			rmax - rmin + 1, cmax - cmin + 1, rmin, cmin,NO_FLAGS);
      shAssert(sub != NULL);
/*
 * find the highest non-saturated peak in the object; if we simply median
 * a large saturated star we can easily need 40Mby of histograms. We let
 * phMedianSmooth choose the minval
 */
      minval = SOFT_BIAS - 100;		/* i.e. don't let it autoscale */
      maxval = 0;
      for(i = 0;i < nchild;i++) {
	 obj1 = children[i]->color[c];

	 r0 = (int)obj1->rowc; c0 = (int)obj1->colc;
	 if(r0 < 0 || r0 >= data->nrow || c0 < 0 || c0 >= data->ncol) {
	    continue;
	 }
	    
	 if(phRectIntersectMask(sm->masks[S_MASK_SATUR],
				   c0, r0, c0 + size + 0.5, r0 + size + 0.5)) {
	    continue;
	 }

	 val = data->ROWS[r0][c0];
	 if(val > maxval) {
	    maxval = val;
	 }
      }
/*
 * phMedianSmooth requires bin size < region size
 */
      {
	 static char *dump_filename = NULL; /* write data to this file?
					       For use from gdb */
	 shAssert(sub->nrow >= 1 && sub->ncol >= 1);
	 while(bin_c < sub->nrow/8) { bin_c *= 2; }
	 while(bin_r < sub->ncol/8) { bin_r *= 2; }
	 while(bin_c >= sub->nrow) { bin_c /= 2; }
	 while(bin_r >= sub->ncol) { bin_r /= 2; }
	 binreg = phMedianSmooth(sub, bin_r, bin_c, 1, minval, maxval,
				 0, 0, NULL, 0, (PIXDATATYPE)0);
	 if(dump_filename != NULL) {
	    shRegWriteAsFits(binreg->reg,
			     dump_filename, STANDARD, 2,DEF_NONE,NULL,0);
	    dump_filename = NULL;
	 }
      }
/*
 * find centre and undo effects of binning.
 *
 * Note that the binning is bin*bin, centred every (bin/2, bin/2);
 * so the unbinned data coresponding to the (1,1) pixel in the binimage
 * runs from (0,0) to (bin-1, bin-1)
 *
 * The BINREGION records the actual sizes of the binned pixels (i.e. bin/2)
 * rather than the filter size.
 *
 * The actual binning may have been less than bin*bin to fit in sub
 */
      phObjectPeakFind(binreg->reg, NULL, peaks);
      shAssert(peaks->npeak == 1);
      peak = peaks->peaks[0];

      phPeakCenterFit(peak, binreg->reg,
		      NULL, &fiparams->frame[c], 1, ALWAYS_SMOOTH);
      
      peak->rpeak = sub->row0 + (peak->rowc - 0.5)*binreg->bin_row;
      peak->cpeak = sub->col0 + (peak->colc - 0.5)*binreg->bin_col;
/*
 * Find the highest peak within +- bin_{col,row}/3 of that point, and
 * centroid it
 */
      {
	 const int nrow = fiparams->frame[c].data->nrow;
	 const int ncol = fiparams->frame[c].data->ncol;
	 int rmin = peak->rpeak - bin_c/3;
	 int cmin = peak->cpeak - bin_r/3;
	 int rmax = peak->rpeak + bin_c/3 - 1;
	 int cmax = peak->cpeak + bin_r/3 - 1;
	 if(rmin < 0) rmin = 0;
	 if(cmin < 0) cmin = 0;
	 if(rmax >= nrow) rmax = nrow - 1;
	 if(cmax >= ncol) cmax = ncol - 1;

	 phRegIntMaxPixelFind(fiparams->frame[c].data,0, rmin, cmin, rmax, cmax,
			      &peak->rpeak, &peak->cpeak, NULL, NULL);
      }

      phPeakCenterFit(peak, fiparams->frame[c].data,
		      NULL, &fiparams->frame[c], bin/2, ALWAYS_SMOOTH);

      shRegDel(sub);
      phBinregionDel(binreg);
/*
 * find the peak closest to that centre
 */
      r2_min = 1e10;
      central_child = -1;
      for(i = 0;i < nchild;i++) {
	 child = children[i];
	 r2 = pow(child->rowc-peak->rowc, 2) + pow(child->colc-peak->colc, 2);
	 if(r2 < r2_min) {
	    r2_min = r2;
	    central_child = i;
	 }
      }
      shAssert(central_child >= 0);

      phPeaksDel(peaks);
   }
/*
 * Find all the peaks that are consistent with being PSFs, and do a crude
 * job of subtracting a PSF from each. Note that the resulting values are
 * only used in estimating templates, and only then if there is no alternative
 *
 * Only stars that would have been detected are included; the detection
 * threshold in the smoothed image is ffo_threshold, and a star of peak
 * intensity I0 would have a peak of I0/2 in the smoothed image, hence the
 * value of I0_min
 */
   for(c = 0;c < objc->ncolor;c++) {
      psfmasks[c] = phObjmaskNew(0);
      I0_min[c] = fiparams->frame[c].ffo_threshold*2;

      psfs[c] = phDgpsfNew();
      psfs[c]->sigmax1 = psfs[c]->sigmay1 = fiparams->frame[c].psf->sigma1_2G;
      psfs[c]->sigmax2 = psfs[c]->sigmay2 = fiparams->frame[c].psf->sigma2_2G;
      psfs[c]->b = fiparams->frame[c].psf->b_2G;

      psf_rad[c] = psf_rad0*fiparams->frame[c].psf->width;
      psf_nann[c] = psf_nann0*fiparams->frame[c].psf->width;
      if(psf_nann[c] < psf_nann0) {
	 psf_nann[c] = psf_nann0;
      }
   }

   for(i = 0;i < nchild;i++) {
      int is_psf;		/* is the object in question a PSF? */
      struct {
	 int satur;		/* is peak saturated? */
	 int detected;		/* is this object be detected in this band? */
	 int is_good_psf;	/* object is a clearly a PSF */
	 float I0;		/* central value */
	 float pedestal;	/* background level */
	 float star_L;		/* likelihood of stellar fit */
      } psf_props[NCOLOR];

      child = children[i];
/*
 * First look at the object in all bands, gathering information
 */
      for(c = 0;c < objc->ncolor;c++) {
/*
 * Note that the return value from phFitCellAsPsf() is chi^2/nu, and that
 * this information is used in the form of child->color[c]->star_L; hence
 * the cast to void
 *
 * If the object is saturated, assume that it's a star, otherwise try
 * fitting a PSF
 *
 * The returned value I0 is the flux within the innermost cell, which has
 * to be corrected to the central value of the DGPSF; we also save the
 * central value of the object in objc->color[c]->profMean[2] and the
 * value of the pedestal in profMean[3]
 */
	 if(child->color[c]->flags & OBJECT1_SATUR) {
	    I0 = 0.5*(fiparams->frame[c].fullWell[0] +
		      fiparams->frame[c].fullWell[1]);
	    child->color[c]->profMean[2] = child->color[c]->profMean[0] = I0;
	    child->color[c]->profMean[3] = 0; /* pedestal */
	    child->color[c]->star_L = 1.0;

	    psf_props[c].satur = 1;
	    psf_props[c].I0 = I0;
	    psf_props[c].pedestal = 0;
	 } else {
	    int sky_noise_only = 0;	/* only include sky noise? */
	    (void)phFitCellAsPsf(child, c, NULL, fiparams, psf_nann[c],
				 sky_noise_only, &I0, &pedestal);

	    child->color[c]->nprof = 0;	/* we haven't really measured profile*/
	    child->color[c]->profMean[2] = child->color[c]->profMean[0];
	    child->color[c]->profMean[3] = pedestal;
	    shAssert(fiparams->frame[c].psf->a == 1.0);
	    I0 /= phDgpsfCumulativeFlux(fiparams->frame[c].psf, 1/sqrt(M_PI));

	    psf_props[c].satur = 0;
	    psf_props[c].I0 = I0;
	    psf_props[c].pedestal = pedestal;
	 }
	 psf_props[c].star_L = child->color[c]->star_L;

	 psf_props[c].detected = (I0 > 2*I0_min[c]) ? 1 : 0;
	 psf_props[c].is_good_psf = (psf_props[c].I0 > 10*I0_min[c] &&
				     psf_props[c].star_L > psf_Lmin) ? 1 : 0;
      }
/*
 * Now use that information to decide if the object is indeed a star; note
 * that we don't deblend saturated objects as PSFs
 */
      if(i == central_child) {
	is_psf = 0;			/* don't treat brightest peak as PSF*/
      } else {
	 is_psf = 0;			/* we don't yet know what it is */
	 for(c = 0; c < objc->ncolor; c++) {
	    if(psf_props[c].satur) {
	       is_psf = 1;
	       break;
	    }
	  
	    if(psf_props[c].is_good_psf) {
	       is_psf = 1;
	       break;
	    }
	 }
/*	 
 * If it isn't obviously a PSF, but there aren't many detections,
 * accept a lesser standard of proof.
 */
	 if(!is_psf) {
	    int ndetect = 0;
	    for(c = 0; c < objc->ncolor; c++) {
	       ndetect += psf_props[c].detected;
	    }
	    if(ndetect <= 2) {
	       for(c = 0; c < objc->ncolor; c++) {
		  if(psf_props[c].star_L > psf_Lmin) {
		     is_psf = 1;
		     break;
		  }
	       }
	    }
	 }
      }
/*
 * If it's a PSF, subtract it, and prepare to deblend it AS_PSF
 */
      if(is_psf) {
	 for(c = 0; c < objc->ncolor; c++) {
	    if(!psf_props[c].satur && psf_props[c].I0 > 0) {
	       const float colc = child->color[c]->colc;
	       const float rowc = child->color[c]->rowc;
	       const int ncol = fiparams->frame[c].data->ncol;
	       const int nrow = fiparams->frame[c].data->nrow;
	       const int col0 = fiparams->frame[c].data->col0;
	       const int row0 = fiparams->frame[c].data->row0;

	       child->color[c]->flags |= OBJECT1_DEBLENDED_AS_PSF;
	       I0 = psf_props[c].I0;
/*
 * Only subtract the PSF if the object's on the chip; if it's thought to
 * be moving it may not be.  Only modify pixels within the atlas image
 */
	       if(colc -psf_rad[c] >= col0 && colc +psf_rad[c] < col0 + ncol &&
		  rowc -psf_rad[c] >= row0 && rowc +psf_rad[c] < row0 + nrow) {
		  phDgpsfAddInObjmask((REGION *)fiparams->frame[c].data,
				      objc->aimage->master_mask,
				      objc->aimage->drow[c],
				      objc->aimage->dcol[c],
				      psfs[c], psf_rad[c], rowc, colc, -I0);
	       }
	       child->color[c]->profMean[0] = I0; /* amplitude of template */

	       phObjmaskMergeWithCircle(psfmasks[c], rowc, colc,
				   phDgpsfSize(fiparams->frame[c].psf, I0, 1));
	    }
	 }
      }
   }
/*
 * Dump a region for debugging?
 */
   for(c = 0;c < objc->ncolor;c++) {
       static char *dump_filename = NULL; /* write data to this file?
					     For use from gdb */
       if(dump_filename != NULL) {
	   shRegWriteAsFits((REGION *)fiparams->frame[c].data,
			    dump_filename, STANDARD, 2,DEF_NONE,NULL,0);
	   dump_filename = NULL;
       }
   }
/*
 * we need a copy of the pixel values in the object after we subtracted
 * all objects deemed to be PSFs. The resulting subtracted image is used
 * in defining the templates
 */
   subtracted = phAtlasImageCopy(objc->aimage, 0);
   
   for(c = 0;c < objc->ncolor;c++) {
      phAtlasImageSetFromRegion(subtracted, c, fiparams->frame[c].data);
   }
/*
 * now go through each object extracting its templates
 */
   smoothed_ai = shMalloc(nchild*sizeof(ATLAS_IMAGE *));
   for(i = 0;i < nchild;i++) {
      child = children[i];

      for(c = 0;c < objc->ncolor;c++) {
	 phRegionSetFromAtlasImage(subtracted, c,
				   (REGION *)fiparams->frame[c].data, 0, 0, 0, '\0', 0);
      }
      
      smoothed_ai[i] = NULL;
      if(phStrategicMemoryReserveIsEmpty() ||
	 deblend_template_find(child, psfmasks, fiparams, &smoothed_ai[i], filtsize, ngrow) < 0) { /* un-cleanable */
	 objc->flags |= OBJECT1_NODEBLEND;
	 shFree(children);
	 for(c = 0;c < objc->ncolor;c++) {
	    phObjmaskDel(psfmasks[c]);
	    phDgpsfDel(psfs[c]);
	 }
	 while(--i >= 0) {
	    phAtlasImageDel(smoothed_ai[i], 0);
	 }
	 shFree(smoothed_ai);
	 phAtlasImageDel(subtracted, 0);
	 return(-1);
      }
   }
/*
 * We've got all the templates, so it's time to reconsider some of those
 * choices of DEBLENDED_AS_PSF.  In particular, if we created a PSF
 * template from a peak superimposed on a significant pedestal, but there's
 * no other template to provide that pedestal, clear DEBLENDED_AS_PSF
 *
 * DEBLENDED_AS_PSF is set in all bands (unless they're saturated)
 */
   for(i = 0;i < nchild;i++) {
      int not_psf = 0;			/* clear DEBLENDED_AS_PSF? */
      int ndetect = 0;			/* in how many bands was it detected?*/
      child = children[i];

      if(!(child->flags & OBJECT1_DETECTED)) {
	 continue;			/* We're going to discard this child */
      }

      for(c = 0;c < objc->ncolor;c++) {
	 float amp_i;			/* amplitude for i'th PSF fit */
	 float other_templates = 0;	/* sum of values of other templates */
	 float ped_i;			/* value of i'th template */

	 if(!(child->color[c]->flags & OBJECT1_DEBLENDED_AS_PSF)) {
	    continue;
	 }
#if 0
	 if(child->color[c]->flags & OBJECT1_DETECTED) {
	    ndetect++;
	 }
#endif
	 
#if 0			/* k is no longer used.  The DEBLENDED_AT_EDGE is a useful thought */
	 k = phFindAtlasImagePixel(smoothed_ai[i], (int)(child->rowc); (int)(child->colc));
	 if(k < 0) {			/* pixel's not in atlas image */
	    shAssert(objc->flags2 & OBJECT2_DEBLENDED_AT_EDGE);
	    continue;
	 } else {
	    shAssert(k >= 0);
	 }
#endif
	 
	 amp_i = child->color[c]->profMean[0]; /* Amplitude of PSF fit */
	 ped_i = child->color[c]->profMean[3]; /* Pedestal for PSF fit */

	 if(amp_i < I0_min[c]) {
	    continue;
	 }
	 ndetect++;
	 
	 y = child->rowc; x = child->colc;
	 for(j = 0; j < nchild; j++) {
	    if(i == j || smoothed_ai[j] == NULL) {
	       continue;
	    }
	    
	    val = phAtlasImagePixelGet(smoothed_ai[j], c, y, x);
	    if(val != 0) {
	       other_templates += val - SOFT_BIAS;
	    }
	 }
	 
	 if(other_templates < 0.5*ped_i) {
	    const float colc = child->color[c]->colc;
	    const float rowc = child->color[c]->rowc;
	    const int ncol = fiparams->frame[c].data->ncol;
	    const int nrow = fiparams->frame[c].data->nrow;
	    const int col0 = fiparams->frame[c].data->col0;
	    const int row0 = fiparams->frame[c].data->row0;
/*
 * Only reinstate the PSF if the object's on the chip; if it's thought to
 * be moving it may not be.  Only modify pixels within the atlas image
 */
	    if(colc -psf_rad[c] >= col0 && colc +psf_rad[c] < col0 + ncol &&
 	       rowc -psf_rad[c] >= row0 && rowc +psf_rad[c] < row0 + nrow) {
	       not_psf++;
	    }
	 }
      }
/*
 * Re-extract templates if we cleared the OBJECT1_DEBLENDED_AS_PSF bit
 */
      if(not_psf > 0 && not_psf >= ndetect - 1) {
	 for(c = 0;c < objc->ncolor;c++) {
	    static char *dump_filename = NULL; /* write data to this file?
						     For use from gdb */
	    const float colc = child->color[c]->colc;
	    const float rowc = child->color[c]->rowc;

	    child->color[c]->flags  &= ~OBJECT1_DEBLENDED_AS_PSF;
	    child->color[c]->flags2 |= OBJECT2_NOT_DEBLENDED_AS_PSF;

	    phRegionSetFromAtlasImage(subtracted, c,
				      (REGION *)fiparams->frame[c].data,0,0,0,'\0',0);
	    
	    I0 = child->color[c]->profMean[0];
	    phDgpsfAddInObjmask((REGION *)fiparams->frame[c].data,
				objc->aimage->master_mask,
				objc->aimage->drow[c],
				objc->aimage->dcol[c],
				psfs[c], psf_rad[c], rowc, colc, I0);
	    phObjmaskClearWithCircle(psfmasks[c], rowc, colc,
				     phDgpsfSize(fiparams->frame[c].psf,I0,1));
	    
	    phAtlasImageSetFromRegion(subtracted,c,fiparams->frame[c].data);
	    
	    
	    if(dump_filename != NULL) {
	       shRegWriteAsFits((REGION *)fiparams->frame[c].data,
				dump_filename, STANDARD, 2,DEF_NONE,NULL,0);
	       dump_filename = NULL;
	    }
#if 0
	    phRegionSetFromAtlasImage(subtracted, c,
				      (REGION *)fiparams->frame[c].data, 0, 0, 0, '\0', 0);
#endif
	 }
	 
	 if(deblend_template_find(child, psfmasks, fiparams, &smoothed_ai[i],
				  filtsize, ngrow) < 0) { /* un-cleanable */
	    objc->flags |= OBJECT1_NODEBLEND;
	    shFree(children);
	    for(c = 0;c < objc->ncolor;c++) {
	       phObjmaskDel(psfmasks[c]);
	       phDgpsfDel(psfs[c]);
	    }
	    while(--i >= 0) {
	       phAtlasImageDel(smoothed_ai[i], 0);
	    }
	    shFree(smoothed_ai);
	    phAtlasImageDel(subtracted, 0);
	    return(-1);
	 }
      }
   }

   for(i = 0; i < nchild; i++) {
      for(c = 0;c < objc->ncolor;c++) {
	 children[i]->flags |=
	   (children[i]->color[c]->flags & OBJECT1_DEBLENDED_AS_PSF);
      }
   }
/*
 * when debugging, set the SUBTRACTED bits in the data region where
 * we ignored pixels as PSF contaminated. If skip_deblend is 1,
 * copy the subtracted image into the parent atlas image, and leave
 * the templates in the children's atlas images; if it's 2, we'll
 * copy the smoothed templates into the children atlas images below.
 */
   if(skip_deblend) {
      for(c = 0;c < objc->ncolor;c++) {
	 phObjmaskAddToSpanmask(psfmasks[c],
		(SPANMASK *)fiparams->frame[c].data->mask, S_MASK_SUBTRACTED);
	 psfmasks[c] = NULL;		/* don't free it */

	 if(skip_deblend == 1) {	/* copy subtracted image */
	    phAtlasImagesPixelsCopy(objc->aimage, c, subtracted, c);
	 }
      }
   }
/*
 * done with psfmasks and the PSF-subtracted data
 */
   for(c = 0;c < objc->ncolor;c++) {
      phObjmaskDel(psfmasks[c]);
      phDgpsfDel(psfs[c]);
   }

   phAtlasImageDel(subtracted, 0);
/*
 * now go through templates looking for objects which we didn't detect
 * in any band; in this case, the object wouldn't have been found at all
 * if it wasn't part of a blend, so dump it.
 */
   for(i = 0;i < nchild;i++) {
      child = children[i];

      if(!(child->flags & OBJECT1_DETECTED)) {
	 phObjcChildDel(child);
	 phAtlasImageDel(smoothed_ai[i], 0);

	 for(j = i;j < nchild - 1;j++) { /* move the others down */
	    children[j] = children[j + 1];
	    smoothed_ai[j] = smoothed_ai[j + 1];
	 }
	 nchild--;
	 i--;				/* look at the new children[i] */
	 continue;
      }
   }
/*
 * If there's only one child left (or none) there's no need to deblend,
 * unless the object's moving
 *
 * If there was originally more than one object, but we decided not to
 * deblend, make sure that the parent has the centres from the surviving
 * child
 */
   if(nchild == 0 || (nchild == 1 &&
		      (objc->sibbs == NULL ||
		       !(objc->sibbs->flags2 & OBJECT2_DEBLENDED_AS_MOVING)))){
      objc->flags &= ~OBJECT1_BLENDED;
      for(c = 0;c < objc->ncolor;c++) {
	objc->color[c]->flags &= ~OBJECT1_BLENDED;
      }

      if(nchild == 1) {
	 set_parent_position_from_child(objc, children[0]);
      }

      shFree(children);
      for(i = 0;i < nchild;i++) {
	 phAtlasImageDel(smoothed_ai[i], 0);
      }
      shFree(smoothed_ai);
      return(-1);
   }
/*
 * Average the per-band templates.
 */
   if (objc->ncolor > 1) {
       for(i = 0; i < nchild; i++) {       
	   average_templates(children[i]);
       }
   }
/*
 * and now set up the normal equations for the least squares problem
 */
   if(skip_deblend) {
      if(skip_deblend == 2) {		/* save the smoothed templates */
	 for(i = 0;i < nchild;i++) {
	    for(c = 0;c < objc->ncolor;c++) {
	       phAtlasImagesPixelsCopy(children[i]->aimage, c, smoothed_ai[i], c);
	    }
	 }
      }
   } else {
      int ilambda_min[NCOLOR];		/* index of minimum |eigenvalue| */
      float lambda_min[NCOLOR];		/* minimum |eigenvalue| */
      float lambda_reject;		/* eigenvalue corresponding to reject*/
      int reject;			/* which template to reject */
      int reject_c;			/* which band we rejected in */
      float *weights;			/* weights to use in each pixel
					   for each template */
	    
      nparam = nchild;
      for(c = 0; c < objc->ncolor; c++) {
	 A[c] = phMatNew(nparam, nparam);
	 b[c] = phVecNew(nparam);
	 norm[c] = phVecNew(nparam);
	 lambda[c] = w[c] = NULL;
	 Q[c] = phMatNew(nparam, nparam);
      }
      
      for(c = 0; c < objc->ncolor; c++) {
	 bkgd = fiparams->frame[c].bkgd + SOFT_BIAS;
	 
	 setup_normal(objc, (const OBJC **)children, nchild, c, bkgd,
							  A[c], b[c], norm[c]);
      }

      for(;;) {
/*
 * If any rows of A are all zeros don't even bother with eigenvalue decomp;
 * phEigen() has problems with very singular matrices on e.g. some Linux boxes
 */
	 reject = -1;
	 for(c = 0; c < objc->ncolor; c++) {
	    for(i = 0; i < nchild; i++) {
	       for(j = i; j < nchild; j++) {
		  if(A[c]->me[i][j] != 0) {
		     break;
		  }
	       }
	       if(j == nchild) {	/* no non-zero values */
		  reject = i;
		  break;
	       }
	    }
	    if(reject >= 0) {		/* we found a bad child */
	       break;
	    }
	 }
/*
 * Eigenvalue decompose A into Q and lambda
 */
	 if(reject < 0) {
	    for(c = 0; c < objc->ncolor; c++) {
	       lambda[c] = phEigen(A[c], Q[c], lambda[c]);
	    }
	    sort_Q(objc->ncolor, fiparams->ref_band_index, lambda, Q);
/*
 * Note that the _columns_ of Q are the eigenvectors, e.g. Q[c]->me[][0]
 * is the eigenvector corresponding to lambda[c]->ve[0]
 *
 * Check if are any of those eigenvalues too close to zero; first find
 * the smallest in each band
 */
	    for(c = 0; c < objc->ncolor; c++) {
	       ilambda_min[c] = 0;
	       lambda_min[c] = fabs(lambda[c]->ve[0]);
	       for(i = 1;i < nparam; i++) {
		  if(fabs(lambda[c]->ve[i]) < lambda_min[c]) {
		     ilambda_min[c] = i;
		     lambda_min[c] = fabs(lambda[c]->ve[i]);
		  }
	       }
	    }
/*
 * then the smallest in any band, if it's less than min_lambda
 */
	    reject = reject_c = -1; lambda_reject = 1e10;
	    for(c = 0; c < objc->ncolor; c++) {
	       if(lambda_min[c] < min_lambda) {	/* too small */
		  if(reject == -1) {
		     reject_c = c;
		     reject = ilambda_min[c];
		     lambda_reject = lambda_min[c];
		  } else {
		     if(lambda_reject > lambda_min[c]) {
			reject_c = c;
			reject = ilambda_min[c];
			lambda_reject = lambda_min[c];
		     }
		  }
	       }
	    }
/*
 * reject the template with the smallest eigenvalue, if it's unacceptably small
 *
 * We have to find which template corresponds to the rejected eigenvalue
 */
	    if(reject >= 0) {
	       int imax;		/* index of max. elem. eigenvector */
	       float max;		/* maximum elem. of eigenvector */
	       
	       imax = 0; max = 0;
	       for(j = 0; j < nparam; j++) {
#if 0					/* traditional code */
		  if(fabs(Q[reject_c]->me[reject][j]) > max) {
		     imax = j; max = Q[reject_c]->me[reject][j];
		  }
#else  /* isn't this right?  Check the columns? */
		  if(fabs(Q[reject_c]->me[j][reject]) > max) {
		     imax = j; max = Q[reject_c]->me[j][reject];
		  }
#endif
	       }
	       reject = imax;
	    }
	 }
/*
 * If there were no rejectable eigenvalues, solve for the weights and see if
 * any of the resulting children are too faint to have been detected; if
 * one is reject _it_ and try again.
 *
 * If we really can find no children to reject, break out of the loop
 * and prepare to allocate flux to each child
 */	 
	 if(reject < 0) {		/* none to reject */
	    for(c = 0; c < objc->ncolor; c++) {
	       w[c] = phEigenBackSub(Q[c], lambda[c], b[c]);
/*
 * undo effects of normalising templates when calculating normal equations
 */
	       for(i = 0;i < w[c]->dim;i++) {
		  w[c]->ve[i] /= norm[c]->ve[i];
	       }
#if 1
/*
 * set negative weights to zero.
 */
	       for(i = 0;i < w[c]->dim;i++) {
		  if(w[c]->ve[i] < 0) {
		     w[c]->ve[i] = 0;
		  }
	       }
#endif
	    }
/*
 * see if any of those children are undetectable. Note that we store
 * the maximum value of the template in child->color[c]->profMean[2]
 *
 * For objects superimposed on other objects, we make the detectibility
 * condition somewhat tighter: if the per-pixel flux in the parent is 
 * greater than 10*ffo_threshold, we require that the child be detected
 * at at least the 3*ffo_threshold level (as opposed to the usual criterion
 * of simple exceeding ffo_threshold) XXX
 */
	    for(i = 0; i < nchild; i++) {
	       for(c = 0; c < objc->ncolor; c++) {
		  bkgd = fiparams->frame[c].bkgd + SOFT_BIAS;

		  val = w[c]->ve[i]*children[i]->color[c]->profMean[2];
		  if(val >= fiparams->frame[c].ffo_threshold) {
#if 0
		     float flux = 0;
 		     for(j = 0; j < nchild; j++) {
			if(i != j) {
			   flux +=
				w[c]->ve[j]*children[j]->color[c]->profMean[2];
			}
 		     }
#else
		     const REGION *data = fiparams->frame[c].data;
		     const int rpeak = children[i]->color[c]->rowc;
		     const int cpeak = children[i]->color[c]->colc;
		     float flux;	/* flux in other objects */

		     if(rpeak < 0 || rpeak >= data->nrow ||
			cpeak < 0 || cpeak >= data->ncol) {
			c = objc->ncolor; /* off chip; not detected! */
			break;
		     }
		     flux = data->ROWS[rpeak][cpeak] - bkgd - val;
#endif

		     if(flux < 10*fiparams->frame[c].ffo_threshold ||
			val > 3*fiparams->frame[c].ffo_threshold) {
			break;
		     }
		  }
	       }
	       
	       if(c == objc->ncolor) {
		  reject = i;
		  break;
	       }
	    }
	 }
/*
 * Find angles between templates (by summing the inner products over all
 * bands, which isn't the same as summing the templates but which is fast)
 *
 * If any are too nearly parallel delete the one which generates the
 * fainter child
 */
	 if(reject < 0) {
	    MAT *inner;			/* dot products of templates */
	    
	    inner = phMatNew(nparam, nparam);
	    phMatClear(inner);
/*
 * sum the inner products
 */
	    for(c = 0; c < objc->ncolor; c++) {
	       for(i = 0; i < nparam; i++) {
		  for(j = 0; j <= i; j++) {
		     inner->me[i][j] += A[c]->me[i][j];
		  }
	       }
	    }
/*
 * convert them to cosines
 */
	    for(i = 0; i < nparam; i++) {
	       for(j = 0; j < i; j++) {
		  inner->me[i][j] = inner->me[j][i] = 
		    inner->me[i][j]/sqrt(inner->me[i][i]*inner->me[j][j]);
	       }
	    }
	    for(i = 0; i < nparam; i++) {
	       inner->me[i][i] = 1;
	    }
/*
 * find the maximum non-diagonal cosine
 */
	    for(i = 0; i < nparam; i++) {
	       float max_inner = 0.0;
	       for(j = 0; j < nparam; j++) {
		  if(j != i && inner->me[i][j] > max_inner) {
		     max_inner = inner->me[i][j];
		     if(max_inner > fiparams->deblend_inner_max) {
			reject = j;
		     }
		  }
	       }
	       
	       if(reject >= 0) {
		  break;
	       }
	    }
	    
	    phMatDel(inner); inner = NULL;
/*
 * If one of a pair of templates has to be deleted, and if only one is
 * DEBLENDED_AS_PSF, choose that one, otherwise choose the one with
 * the _smaller_ peak value in the associated child.
 */
	    if(reject >= 0) {
	       int keep = i;		/* which child did we keep? */
	       j = reject;		/* it was called j originally */

	       if((children[i]->flags ^ children[j]->flags) &
						   OBJECT1_DEBLENDED_AS_PSF) {
		  if(children[i]->flags & OBJECT1_DEBLENDED_AS_PSF) {
		     keep = j; reject = i;
		  }
	       } else {
		  float child_peak_i, child_peak_j;
		  
		  child_peak_i = child_peak_j = 0;
		  for(c = 0; c < objc->ncolor; c++) {
		     child_peak_i += norm[c]->ve[i]*w[c]->ve[i];
		     child_peak_j += norm[c]->ve[j]*w[c]->ve[j];
		  }
		  
		  if(child_peak_i < child_peak_j) {
		     reject = i; keep = j;
		  }
	       }

	       shAssert(keep >= 0 && keep < nchild);
	       objc->flags2 |= OBJECT2_DEBLEND_DEGENERATE;
	       children[keep]->flags2 |= OBJECT2_DEBLEND_DEGENERATE;
/*
 * If the rejected child was detected in a band but the kept one wasn't,
 * we need to inherit the rejected child's PEAKs and DETECTED flags as
 * it was presumably only missed because the object chose the other band
 * at random -- remember that the two detections are degenerate. The
 * centroid was derived from the peak, so copy it over too. Copying
 * the centroid probably only matters if some of the peaks actually
 * belong to moving objects
 */
	       transfer_centers_and_peaks(children[keep], children[reject]);
	    }
	 }

	 if(reject < 0) {		/* there are none to delete */
	    break;
	 } else {
	    nchild = reject_template(objc, nchild, reject, children,
				     smoothed_ai, A, b, norm, lambda, Q, w);
	    nparam--;

	    if(nchild <= 0) {
	       return(-1);
	    }
	 }
      }
/*
 * We have the final list of templates, so now's the time to run the
 * peephole optimiser;  in other words, look at the potential children
 * and see if there are any `local' modifications to that list that would
 * improve the deblend.
 */
      nchild = peephole_optimizer(objc, fiparams, nchild,
				  children, smoothed_ai);
/*
 * now multiply the templates by those coefficients, and use the ratios
 * of the products for the different children to decide how to share
 * the flux between them
 */
      i_nchild = 1/(float)nchild;
      weights = shMalloc(nchild*sizeof(float));
      for(c = 0; c < objc->ncolor; c++) {
	 bkgd = fiparams->frame[c].bkgd + SOFT_BIAS;

	 for(i = 0;i < objc->aimage->master_mask->nspan;i++) {
	     y = objc->aimage->master_mask->s[i].y;
	     int x1 = objc->aimage->master_mask->s[i].x1;
	     int x2 = objc->aimage->master_mask->s[i].x2;
	     for (x = x1; x <= x2; x++) {
		 int candidate = -1;		/* last (maybe only) candidate */
		 int ncandidate = 0;		/* number of candidates */
		 static int pix_thresh = 40000; /* XXX */
		 float sum = 0;
		 float ww;			/* == w[c]->ve[j] */

		 for(j = 0;j < nchild;j++) {
		     if((val = phAtlasImagePixelGet(children[j]->aimage, c, y, x)) == 0) {
			 weights[j] = 0;
		     } else {
			 candidate = j; ncandidate++;

			 ww = w[c]->ve[j]*(val - bkgd);
			 if(ww < 0) {
			     ww = i_nchild;
			 }
			 weights[j] = ww;
			 sum += ww;
		     }
		 }
/*
 * If there's only one candidate, so give it all the flux unless that
 * flux level is higher than its peak (in which case assign_missing_flux()
 * will deal with it)
 */
		 if(ncandidate == 1) {
		     for(j = 0;j < nchild;j++) {
			 phAtlasImagePixelSet(children[j]->aimage, c, y, x, SOFT_BIAS);
		     }
	       
		     phAtlasImagePixelSet(children[candidate]->aimage, c, y, x,
					  phAtlasImagePixelGet(objc->aimage, c, y, x));
	       
		     continue;
		 }

		 if(sum > 10.0) {		/* use the unsmoothed templates */
		     for(j = 0;j < nchild;j++) {
			 weights[j] /= sum;
		     }
		 } else {			/* use smoothed templates */
		     sum = 0;
		     for(j = 0;j < nchild;j++) {
			 if((val = phAtlasImagePixelGet(children[j]->aimage, c, y, x)) == 0) {
			     weights[j] = 0;
			 } else {
			     ww = w[c]->ve[j]*(val - bkgd);
			     if(ww < 0) {
				 ww = i_nchild;
			     }
			     weights[j] = ww;
			     sum += ww;
			 }
		     }

		     if(sum > 1.0) {
			 for(j = 0;j < nchild;j++) {
			     weights[j] /= sum;
			 }
		     } else {
/*
 * Force assign_missing_flux() to do the work for us.
 */
			 for(j = 0;j < nchild;j++) {
			     phAtlasImagePixelSet(children[j]->aimage, c, y, x, SOFT_BIAS);
			 }

			 continue;
		     }
		 }
/*
 * OK, we've got the weights for each template in this pixel; time to
 * share the flux among the children
 */
		 for(j = 0;j < nchild;j++) {
		     ww = weights[j];
		     if(ww == 0.0) {
			 val = SOFT_BIAS;
		     } else {
			 val = bkgd + ww*(phAtlasImagePixelGet(objc->aimage, c, y, x) - bkgd);
			 if(val < 0) {
			     val = 0;
			 }
		     }
		     phAtlasImagePixelSet(children[j]->aimage, c, y, x, val);

		     if(val < 0 || val > pix_thresh) {
			 if(pix_thresh > 0 && sum < 100 &&
			    phAtlasImagePixelGet(objc->aimage, c, y, x) < pix_thresh) {
			     shError("phObjcDeblend: "
				     "children[%d]->aimage->pix[%d][%d] = " PIXFMT " "
				     "(sum = %g) pos = (%.1f,%.1f)",
				     j,c,i,val,sum,
				     children[j]->rowc,children[j]->colc);
			 }
		     }
		 }
	     }
	 }
      }

      for(c = 0; c < objc->ncolor; c++) {
	 phMatDel(A[c]);
	 phVecDel(w[c]);
	 phVecDel(b[c]);
	 phVecDel(norm[c]);
	 phVecDel(lambda[c]);
	 phMatDel(Q[c]);
      }
      shFree(weights);
   }
/*
 * There may still be unassigned flux, from low S/N areas of the parent,
 * or from high points of templates that didn't make it into the final
 * child list. Go through the list of children, and calculate for each
 * child Is2 == \sum (I*r_i^2) where r_i is the distance from the pixel to the
 * objects center. This is an estimate of (total flux)*(scale size)^2
 *
 * Then assign missing flux to the child with the largest value of Is2/R^2
 * where R is now the distance from the pixel to the child.
 */
   Is2 = alloca(nchild*sizeof(float));
   for(c = 0; c < objc->ncolor; c++) {
      float nsigma = 2;			/* no. of s.d. to subtract from count*/
      float petroCounts;		/* estimate of parent's petro counts */
      float unassigned;			/* previously unassigned flux */
      float bkgd = fiparams->frame[c].bkgd + SOFT_BIAS;
      for(i = 0; i < nchild; i++) {
	 Is2[i] = find_Isigma2(children[i], c, bkgd);
      }

      unassigned =
	assign_missing_flux((const OBJC **)children, nchild, c, Is2);

      petroCounts = objc->color[c]->petroCounts;
      petroCounts -= nsigma*objc->color[c]->petroCountsErr; /*allow for error*/
      if(petroCounts > 0 &&
	 unassigned > fiparams->deblend_allowed_unassigned*petroCounts) {
	 objc->color[c]->flags2 |= OBJECT2_DEBLEND_UNASSIGNED_FLUX;
      }
   }
/*
 * Estimate the background level ("sky") due to siblings. As we expect that
 * the sky level will be close, or closeish, to zero, the numerical
 * consequences of accumulating both sums simultaneously should be bearable
 *
 * If we cannot find any pixels in some band we can't do this, despite the
 * fact that the errors are needed (and important) in this case. We therefore
 * choose a small area around the nominal centre to estimate sky etc; this
 * is done by fiddling with the atlas image, and then resetting it. The
 * possibility of doing this is why the sky-estimation code is in a loop
 *
 * This is allowed to for OBJECT2_DEBLENDED_AT_EDGE objects which may have
 * peaks in the region that we trimmed from their atlas images
 */
   for(i = 0; i < nchild; i++) {
      for(c = 0; c < objc->ncolor; c++) {
	 OBJMASK *disk = NULL;		/* small disk around centre of object*/
	 int n = 0;			/* number of pixels in object */
	 float sky;			/* estimate of background level */
	 double sum = 0.0, sum2 = 0.0;	/* Sum(sky), Sum(sky^2) in object */
	 
	 do {
	    for(j = 0;j < objc->aimage->master_mask->nspan;j++) {
	       y = objc->aimage->master_mask->s[j].y;
	       int x1 = objc->aimage->master_mask->s[j].x1;
	       int x2 = objc->aimage->master_mask->s[j].x2;
	       for (x = x1; x <= x2; x++) {
		  if((val = phAtlasImagePixelGet(children[i]->aimage, c, y, x)) != SOFT_BIAS) {
		     sky = phAtlasImagePixelGet(objc->aimage, c, y, x) - val;	/* SOFT_BIAS subtracts out */
		     sum += sky;
		     sum2 += sky*sky;
		     n++;
		  }
	       }
	    }
	       
	    if(n == 0) {		/* use a small circle around centre */
	       int r = 5;		/* XXX */
		
	       if(disk == NULL) {
		  disk = phObjmaskFromCircle(children[i]->color[c]->rowc,
					     children[i]->color[c]->colc, r);
		  phAtlasImageSetInObjmask(children[i]->aimage, c, disk, SOFT_BIAS + 1);
	       } else {			/* beware an infinite loop */
		  shAssert(children[i]->flags2 & \
			   (OBJECT2_DEBLENDED_AT_EDGE|OBJECT2_DEBLENDED_AS_MOVING));
		  sum = children[i]->color[c]->sky; sum2 = ERROR_IS_BAD;
		  n = 1;
	       }
	    }
	 } while(n == 0);
	 
	 if(disk != NULL) {		/* reset empty atlas image */
	    phAtlasImageSetInObjmask(children[i]->aimage, c, disk, SOFT_BIAS);
	    phObjmaskDel(disk);
	 }

	 if(sum < 0) {			/* this causes trouble when estimating
					   sky variance.  It's also nonsense */
	    sum = 0;
	 }
	 children[i]->color[c]->sky = sum/(float)n;
	 children[i]->color[c]->skyErr = (sum2 < 0) ? sum2 : sqrt(sum2/(float)n - pow(sum/(float)n, 2));
      }
   }
/*
 * set CR, SATUR, and INTERP flags as appropriate
 */
   for(i = 0;i < nchild;i++) {
      SPANMASK *sm;			/* == obj1->region->mask */
      child = children[i];

      for(c = 0; c < objc->ncolor; c++) {
	 obj1 = child->color[c];
	 sm = (SPANMASK *)obj1->region->mask;
	 shAssert(sm != NULL && sm->cookie == SPAN_COOKIE);

	 if(phObjmaskIntersectMask(sm->masks[S_MASK_CR], obj1->mask)) {
	    obj1->flags |= OBJECT1_CR;
	 }
	 if(phObjmaskIntersectMask(sm->masks[S_MASK_INTERP], obj1->mask)) {
	    obj1->flags |= OBJECT1_INTERP;
	 }
	 if(phObjmaskIntersectMask(sm->masks[S_MASK_SATUR], obj1->mask)) {
	    obj1->flags |= OBJECT1_SATUR;
	 }
      }
   }
/*
 * If there's a background level, add it to all pixels == SOFT_BIAS
 */
   for(i = 0;i < nchild;i++) {
       child = children[i];
       for(c = 0; c < child->ncolor; c++) {
	   phAtlasImageConstAddIfSoftBias(child->aimage, c, fiparams->frame[c].bkgd, 1);
       }
   }
/*
 * and clean up
 */
   shFree(children);
   for(i = 0;i < nchild;i++) {
      phAtlasImageDel(smoothed_ai[i], 0);
   }
   shFree(smoothed_ai);


#endif // NOPE

   
   return(0);
}


/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Make an OBJC's children, returning the number created (may be 0)
 *
 * Once an OBJC has been through phObjcMakeChildren() it has a non-NULL
 * children field which points at one of its children; in addition its
 * OBJECT1_BLENDED bit is set, and nchild is one or more.
 *
 * Each child has a non-NULL parent field which points to its parent, and may
 * have a sibbs field too. It has its OBJECT1_CHILD bit set.
 */
int
phObjcMakeChildren(OBJC *objc,		/* give this OBJC a family */
		   const FIELDPARAMS *fiparams) /* misc. parameters */
{
   const PEAK *cpeak;			/* == objc->peaks->peaks[] */
   float errMin = 0;			/* minimum possible positional error */
   int i, j;
   OBJC *moving;			/* objc's moving sibbling */
   PEAK *mpeaks[NCOLOR];		/* list of possibly-moving peaks */
   int nchild;				/* number of children created */
   PEAK *peak;				/* a PEAK in the OBJC */
   float min_peak_spacing;

   shAssert(objc != NULL && objc->peaks != NULL && fiparams != NULL);
   min_peak_spacing = fiparams->deblend_min_peak_spacing;

   nchild = objc->peaks->npeak;
/*
 * See if this blend might simply be a moving object.
 *
 * Look for objects detected in a different place in each band, i.e. that
 * show up as a single detection in a given band. We currently
 * only allow a single unmatched detection in each band, and
 * require that there are at least ndetect_min detections in toto.
 *
 * We do not require the peaks to appear in the order of the filters
 * on the sky, although we shall use this as a criterion when
 * deciding whether to deblend as a moving object
 *
 * We can do a little better for isolated moving objects; if we have no
 * more than one detection in each band we can ask if they are consistent
 * with a moving object; to do this it is sufficient to set the OBJECT1_MOVED
 * bit and see if the fit is acceptable
 */
   for(i = 0; i < objc->ncolor; i++) {
      mpeaks[i] = NULL;
   }

   for(i = 0; i < objc->peaks->npeak; i++) {
      for(peak = objc->peaks->peaks[i]; peak != NULL;
						   peak = (PEAK *)peak->next) {
	 j = phPeakBand(peak);

	 if(mpeaks[j] == NULL) {
	    mpeaks[j] = peak;
	 } else {
	    break;			/* two detections in the same band */
	 }
      }
      if(peak != NULL) {		/* we broke out */
	 break;
      }
   }
   
   if(i == objc->peaks->npeak) {	/* we didn't break out */
      objc->flags |= OBJECT1_MOVED;	/* we'll check that the velocity is
					   consistent with zero and that the
					   fit's acceptable in a moment, and
					   maybe clear this bit again */
   } else if(nchild > 1) {
      int ndetect = 0;			/* number of single-band detections */
      int ndetect_min = 2;		/* must be seen at least this many
								 times in different bands */
	  printf("deblend.c: changing ndetect_min from 2 to 1\n");
	  ndetect_min = 1;

      for(i = 0; i < objc->ncolor; i++) {
	 mpeaks[i] = NULL;
      }

      for(i = 0; i < objc->peaks->npeak; i++) {
	 if(objc->peaks->peaks[i] == NULL) {
	    continue;
	 }
/*
 * If there's more than one detection with a good position, this cannot
 * be a moving peak. More precisely, if there's more than one peak that
 * is clearly at the same position it cannot be moving, but if the second
 * (third, ...) peak is merely consistent but has large errors, don't
 * hold that against the primary.  This is a concern because the u and z
 * detections can confuse the gri ones of real moving objects
 *
 * So look at the peaks.
 */
	 if(objc->peaks->peaks[i]->next != NULL) { /* more than one detection*/
	    int n = 0;			/* number of "good" detections */
	    const PEAK *cpeak = objc->peaks->peaks[i];
	    static float err_min = 0.2;	/* minimum error to accept nth peak */
	    static float nsigma = 2;	/* minimum significance to treat
					   peaks as separate */
	    float const rowc = cpeak->rowc;
	    float const colc = cpeak->colc;
	    float const rowcErr = cpeak->rowcErr;
	    float const colcErr = cpeak->colcErr;

	    do {
	       if(n == 0) {
		  n++;			/* first detection's always good */
	       } else {
		  if(cpeak->rowcErr > err_min || cpeak->colcErr > err_min) {
		     ;			/* too bad to invalidate first peak */
		  } else if(pow(cpeak->rowc - rowc, 2) <
			    nsigma*nsigma*(cpeak->rowcErr*cpeak->rowcErr +
					    rowcErr*rowcErr + errMin*errMin) &&
			    pow(cpeak->colc - colc, 2) <
			    nsigma*nsigma*(cpeak->colcErr*cpeak->colcErr +
					    colcErr*colcErr + errMin*errMin)) {
		     n++;		/* they really are in the same place */
		  }
	       }
	       if(n > 1) {
		  break;
	       }
	       
	       cpeak = cpeak->next;
	    } while(cpeak != NULL);

	    if(n > 1) {
	       continue;		/* two detections of same peak */
	    }
	 }
/*
 * It's only detected in one band; remember the peak in the mpeaks array
 */
	 for(peak = objc->peaks->peaks[i]; peak != NULL;
						   peak = (PEAK *)peak->next) {
	    j = phPeakBand(peak);
	    
	    if(mpeaks[j] != NULL) {	/* slot was already taken */
	       ;			/* somehow decide which one to use;
					   by doing nothing we keep the
					   earlier (brighter) one. XXX */
	    } else {
	       mpeaks[j] = peak;
	       ndetect++;
	    }
	 }
      }
      if(ndetect >= ndetect_min) {
	 objc->flags |= OBJECT1_MOVED;
      }
   }
/*
 * If the object may have moved, create an extra sibling for it. The array
 * mpeaks points to PEAKs linked into their own lists, so we have to
 * copy them here. The original mpeaks are labelled PEAK_MOVED, so that
 * we can identify and ignore them if we decide to deblend them as a single
 * moving object
 */
   if(objc->flags & OBJECT1_MOVED) {
      PEAK *tmp;

      peak = NULL;			/* list of peaks for this relative */
      for(i = 0; i < objc->ncolor; i++) {
	 if(mpeaks[i] != NULL) {
	    tmp = peak;
	    peak = phPeakNew();
	    phPeakCopy(peak, mpeaks[i]);
	    peak->flags &= ~PEAK_DANGLING;
	    peak->next = NULL;
	    
	    mpeaks[i]->flags |= PEAK_MOVED; /* peak is copied into moving */

	    if(tmp != NULL) {
	       peak->next = tmp;
	       tmp->flags |= PEAK_DANGLING;
	    }
	 }
      }
      moving = phObjcChildNew(objc, peak, fiparams, 0);
      moving->flags |= OBJECT1_MOVED;
      moving->flags2 |= OBJECT2_DEBLENDED_AS_MOVING;
      objc->flags2 |= OBJECT2_DEBLENDED_AS_MOVING;
      
      phPeakDel(peak);
   }
/*
 * Done with moving objects. See if any of the surviving peaks are
 * too close
 */
   for(i = 0; i < objc->peaks->npeak; i++) {
      PEAK *const peak_i = objc->peaks->peaks[i];
      PEAK *peak_j;
      float rowc_i, colc_i;		/* == peak_i->{col,row}c */
      float rowcErr_i, colcErr_i;	/* == peak_i->{col,row}cErr */
      float rowc_j, colc_j;		/* == peak_j->{col,row}c */
      float rowcErr_j, colcErr_j;	/* == peak_j->{col,row}cErr */

      if(peak_i == NULL) {
	 continue;
      }
      shAssert(peak_i->flags & PEAK_CANONICAL);
      
      rowc_i = peak_i->rowc;
      colc_i = peak_i->colc;
      rowcErr_i = peak_i->rowcErr;
      colcErr_i = peak_i->colcErr;
      for(j = i + 1; j < objc->peaks->npeak; j++) {
	 if(objc->peaks->peaks[j] == NULL) {
	    continue;
	 }

	 peak_j = objc->peaks->peaks[j];
	 rowc_j = peak_j->rowc;
	 colc_j = peak_j->colc;
	 rowcErr_j = peak_j->rowcErr;
	 colcErr_j = peak_j->colcErr;
	 if(pow(fabs(rowc_i - rowc_j) - rowcErr_i - rowcErr_j, 2) +
	    pow(fabs(colc_i - colc_j) - colcErr_i - colcErr_j, 2) <
					   min_peak_spacing*min_peak_spacing) {
	    objc->flags2 |= OBJECT2_PEAKS_TOO_CLOSE;
/*
 * If the two peaks are in the same band, delete peak_j otherwise add
 * it to peak_i's ->next list.  If there's already a peak on the ->next
 * list in the same band, average their positions
 */
	    merge_peaks(peak_i, peak_j);

	    objc->peaks->peaks[j] = NULL;
	    nchild--;

	    i--;			/* reconsider the i'th peak */
	    break;
	 }
      }
   }
/*
 * We demand that the children are detected in at least deblend_min_detect
 * bands; reject peaks that fail this test
 */
   for(i = 0; i < objc->peaks->npeak; i++) {
      int n;				/* number of detections */

      if(objc->peaks->peaks[i] == NULL) {
	 continue;
      }

      for(n = 0, peak = objc->peaks->peaks[i]; peak != NULL;
						   peak = (PEAK *)peak->next) {
	 n++;
      }
      if(n < fiparams->deblend_min_detect) {
	 objc->flags2 |= OBJECT2_TOO_FEW_DETECTIONS;
	 
	 phPeakDel(objc->peaks->peaks[i]);
	 objc->peaks->peaks[i] = NULL;
	 nchild--;
      }
   }
/*
 * condense the peaks list
 */
   if(nchild != objc->peaks->npeak) {
      for(i = j = 0; i < objc->peaks->npeak; i++) {
	 if(objc->peaks->peaks[i] != NULL) {
	    objc->peaks->peaks[j++] = objc->peaks->peaks[i];
	 }
      }
      shAssert(j == nchild);
      for(i = nchild; i < objc->peaks->npeak; i++) {
	 objc->peaks->peaks[i] = NULL;	/* it's been moved down */
      }
      phPeaksRealloc(objc->peaks, nchild);
   }
/*
 * and create the desired children
 */
   if(objc->peaks->npeak > fiparams->nchild_max) { /* are there too many? */
      objc->flags |= OBJECT1_DEBLEND_TOO_MANY_PEAKS;
      phPeaksRealloc(objc->peaks, fiparams->nchild_max);
   }

   for(i = 0;i < objc->peaks->npeak;i++) { /* create children */
      cpeak = objc->peaks->peaks[i];
      (void)phObjcChildNew(objc, cpeak, fiparams, 1);
   }

   return(objc->peaks->npeak + ((objc->sibbs != NULL) ? 1 : 0));
}

/*****************************************************************************/
/*
 * Merge peak_j into peak_i, including processing their ->next chains
 */
static void
merge_peaks(PEAK *peak_i,
	    PEAK *peak_j)
{
   int flagi = 0, flagj = 0;		/* union of [ij] flags */
   int i, j;
   int npeak_i, npeak_j;
   PEAK *peaki, *peakj;			/* pointers to peak_[ij]->next chains*/
   PEAK **peaklist_i, **peaklist_j;		/* unpacked [ij] chains */
/*
 * Count the ->next peaks, and OR together their flags
 */
   for(npeak_i = 0, peaki = peak_i; peaki != NULL;
						 peaki = (PEAK *)peaki->next) {
      shAssert((flagi & peaki->flags) == 0); /* no duplicates within chain */
      flagi |= (peaki->flags & PEAK_BANDMASK);
      npeak_i++;
   }

   for(npeak_j = 0, peakj = peak_j; peakj != NULL;
						 peakj = (PEAK *)peakj->next) {
      shAssert((flagj & peakj->flags) == 0); /* no duplicates within chain */
      flagj |= (peakj->flags & PEAK_BANDMASK);
      npeak_j++;
   }
/*
 * look for peaks on the two chains that are in the same band;
 * we can easily detect the simple no-duplicates case
 */
   if((flagi & flagj) == 0) {		/* no duplicates */
      for(peaki = peak_i; peaki->next != NULL; peaki = (PEAK *)peaki->next) {
	 ;
      }
      peaki->next = peak_j;
      peak_j->flags |= PEAK_DANGLING;

      return;
   }
/*
 * unpack the ->next chains.  This makes removing PEAKs from the middle
 * of the (singly-linked) lists much easier
 */
   peaklist_i = alloca(npeak_i*sizeof(PEAK *));
   for(i = 0, peaki = peak_i; peaki != NULL; peaki = (PEAK *)peaki->next) {
      peaklist_i[i++] = peaki;
   }
   peaklist_j = alloca(npeak_j*sizeof(PEAK *));
   for(j = 0, peakj = peak_j; peakj != NULL; peakj = (PEAK *)peakj->next) {
      peaklist_j[j++] = peakj;
   }
/*
 * look for multiple detections in the same band
 */
   for(i = 0; i < npeak_i; i++) {
      peaki = peaklist_i[i];
      
      for(j = 0; j < npeak_j; j++) {
	 peakj = peaklist_j[j];

	 if(peakj == NULL) {		/* already merged */
	    continue;
	 }

	 if(phPeakBand(peaki) == phPeakBand(peakj)) {
	    average_peak_centers(peaki, peakj, &peaki->rowc, &peaki->colc);
	    peakj->flags &= ~PEAK_DANGLING; peakj->next = NULL;
	    phPeakDel(peakj);
	    peaklist_j[j] = NULL;
	    if(j > 0 && peaklist_j[j - 1] != NULL) {
	       peaklist_j[j - 1]->next = NULL;
	    }
	    break;
	 }
      }
   }
/*
 * Add the surviving peak_j peaks to peak_i
 */
   peaki = peaklist_i[npeak_i - 1];	/* end of ->next chain */
   for(j = 0; j < npeak_j; j++) {
      peakj = peaklist_j[j];
      
      if(peakj == NULL) {		/* already merged */
	 continue;
      }

      peaki->next = peakj;
      peakj->flags |= PEAK_DANGLING;
      
      peaki = (PEAK *)peaki->next;
   }
}

/*****************************************************************************/
/*
 * Average together the positions of two peaks
 */
static void
average_peak_centers(const PEAK *peak1,	/* the peaks */
		     const PEAK *peak2,	/*         in question */
		     float *rowc, float *colc)	/* mean position */
{
   const float val1 = peak1->peak; const float val2 = peak2->peak;
   const float rowc1 = peak1->rowc; const float rowc2 = peak2->rowc;
   const float colc1 = peak1->colc; const float colc2 = peak2->colc;

   shAssert(val1 + val2 != 0);
   shAssert(val1 > -9998.0 && val2 > -9998.0);

   *rowc = (val1*rowc1 + val2*rowc2)/(val1 + val2);
   *colc = (val1*colc1 + val2*colc2)/(val1 + val2);
}


/*****************************************************************************/
/*
 * Given an OBJC, create a child from OBJC->peaks->peaks[n], link it into
 * OBJC->children or OBJC->sibbs, and copy appropriate fields
 */
OBJC *
phObjcChildNew(OBJC *objc,		/* the parent */
	       const PEAK *peak,	/* which peak? */
	       const FIELDPARAMS *fiparams, /* gain etc. */
	       int is_child)		/* make new OBJC a child, not a sibb */
{
   OBJC *child;				/* the desired child */
   int c;				/* a color index */
   OBJECT1 *cobj1;			/* an OBJECT1 in child */
   PEAK *cpeak;				/* == cobj1->peaks->peaks[0] */
   float drow, dcol;			/* offsets from reference colour */
   float drowErr, dcolErr;		/* errors in drow, dcol */
   int i;
   const OBJECT1 *obj1;			/* an OBJECT1 in objc */
   SPANMASK *sm;			/* == obj1->region->mask */
   
   shAssert(objc != NULL && objc->peaks != NULL);
   shAssert(peak != NULL);

   objc->nchild++;
   child = phObjcNew(objc->ncolor);
   child->parent = objc; shMemRefCntrIncr(child->parent);
   child->flags = objc->flags & (OBJECT1_EDGE |
				 OBJECT1_INTERP |
				 OBJECT1_NOTCHECKED |
				 OBJECT1_SUBTRACTED);
   child->flags |= OBJECT1_CHILD;
   if(peak->flags & PEAK_MOVED) {
      child->flags |= OBJECT1_MOVED;
   }
   child->flags3 = objc->flags3 & OBJECT3_GROWN_MERGED;
/*
 * link new child as a sibling of objc->children or objc->sibbs according
 * to the value of is_child
 */
   if(is_child) {
      if(objc->children == NULL) {
	 objc->children = child;
      } else {
	 OBJC *tmp = objc->children;

	 while(tmp->sibbs != NULL) tmp = tmp->sibbs;
	 tmp->sibbs = child;
      }
   } else {
      if(objc->sibbs == NULL) {
	 objc->sibbs = child;
      } else {
	 OBJC *tmp = objc->sibbs;

	 while(tmp->sibbs != NULL) tmp = tmp->sibbs;
	 tmp->sibbs = child;
      }
   }
/*
 * and set fields from the parent
 */
   for(i = 0;i < objc->ncolor;i++) {
      obj1 = objc->color[i];
      cobj1 = child->color[i] = phObject1New();
      shAssert(obj1 != NULL && obj1->mask != NULL);
      shAssert(obj1->region != NULL && obj1->region->mask != NULL);
      sm = (SPANMASK *)obj1->region->mask;
      shAssert(sm != NULL && sm->cookie == SPAN_COOKIE);

      cobj1->flags = obj1->flags & (OBJECT1_CANONICAL_CENTER |
				    OBJECT1_EDGE |
				    OBJECT1_NOTCHECKED |
				    OBJECT1_SUBTRACTED);
      cobj1->flags |= OBJECT1_CHILD;
      cobj1->flags2 = obj1->flags2 & OBJECT2_LOCAL_EDGE;
      cobj1->flags3 = obj1->flags3 & OBJECT3_GROWN_MERGED;

      cobj1->region = shSubRegNew("",obj1->region,
				  obj1->region->nrow, obj1->region->ncol,
				  0, 0, NO_FLAGS);
      shAssert(cobj1->region != NULL);
      cobj1->region->row0 = obj1->region->row0;
      cobj1->region->col0 = obj1->region->col0;
      cobj1->region->mask = (MASK *)phSubSpanmaskNew(sm);

      cobj1->mask = NULL;

      cobj1->sky = obj1->sky; cobj1->skyErr = obj1->skyErr;
   }
/*
 * and copy the master_mask to the children
 */
   shAssert(objc->aimage->master_mask != NULL);
   child->aimage->master_mask =
     phObjmaskCopy(objc->aimage->master_mask, 0, 0);
/*
 * now set the centres from the peaks list. We label it BINNED1 for now,
 * although we'll reconsider this when deblending
 */
   do {
      c = phPeakBand(peak);
      shAssert(c < objc->ncolor);
      cobj1 = child->color[c];

      shAssert(cobj1->peaks == NULL);	/* only one peak in each band */
      cobj1->peaks = phPeaksNew(1);
      cobj1->peaks->npeak++;
      cpeak = cobj1->peaks->peaks[0];
      phPeakCopy(cpeak, peak);

      if(peak->flags & PEAK_CANONICAL) {
	 phOffsetDo(fiparams, peak->rowc, peak->colc, 
		    fiparams->ref_band_index, c,
		    1, NULL, NULL, &drow, &drowErr, &dcol, &dcolErr);
	 cpeak->rowc += drow;
	 cpeak->colc += dcol;
	 cpeak->flags &= ~PEAK_CANONICAL; /* it no longer is*/
      }
      if (peak->flags & PEAK_REAL) {	/* our position was derived in this band */
	  drowErr = dcolErr = 0;	/*   => no added error; PR 6747 */
      }

      cpeak->next = NULL;
      cpeak->flags &= ~PEAK_DANGLING;	/* it no longer is */

      if(cobj1->rowcErr < -900) {	/* not yet set */
	 cobj1->rowc = cpeak->rowc;
	 cobj1->rowcErr = sqrt(pow(cpeak->rowcErr,2) + pow(drowErr,2));
	 cobj1->colc = cpeak->colc;
	 cobj1->colcErr = sqrt(pow(cpeak->colcErr,2) + pow(dcolErr,2));

	 cobj1->flags |= OBJECT1_BINNED1;
      }

      if(cpeak->flags & PEAK_PEAKCENTER) { /* used pixel center as peak */
	 child->flags |= OBJECT1_PEAKCENTER;
	 cobj1->flags |= OBJECT1_PEAKCENTER;
      }
      if(cpeak->flags & PEAK_SATUR) {	/* a saturated peak */
	 cobj1->flags |= OBJECT1_SATUR;
      }

      cobj1->profMean[0] = cpeak->peak;
      /*
       * deal with known objects
       */
      if (cpeak->catID != 0 && child->catID != peak->catID) {
	  if (child->catID != 0) {
	      fprintf(stderr,"Replacing catID %d with %d\n", child->catID, peak->catID);
	  }
	  child->catID = peak->catID;
      }

      peak = (PEAK *)peak->next;
   } while(peak != NULL);
/*
 * if the OBJECT1 in some band still has no centre, generate one from the
 * canonical centre
 */
   phObjcCenterCalc(child, fiparams, 0); /* find canonical centre */
   for(c = 0;c < child->ncolor;c++) {
      cobj1 = child->color[c];
      
      if(!(cobj1->flags & OBJECT1_DETECTED)) { /* no peak, so no centre */
	 phOffsetDo(fiparams, child->rowc, child->colc, 
		    fiparams->ref_band_index, c,
		    0, NULL, NULL, &drow, &drowErr, &dcol, &dcolErr);
	 cobj1->rowc = child->rowc + drow;
	 cobj1->rowcErr = sqrt(pow(child->rowcErr,2) + pow(drowErr,2));
	 cobj1->colc = child->colc + dcol;
	 cobj1->colcErr = sqrt(pow(child->colcErr,2) + pow(dcolErr,2));
	 cobj1->flags |= OBJECT1_CANONICAL_CENTER;
      }
   }
   
   return(child);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * delete an OBJC which is a descendent of some other OBJC
 */
void
phObjcChildDel(OBJC *child)		/* the child to destroy */
{
   OBJC *objc;
   OBJC *parent;
   
   shAssert(child != NULL && child->children == NULL);
   parent = child->parent;
   shAssert(parent != NULL && parent->nchild > 0);
   shAssert(parent->children != NULL || parent->sibbs != NULL);
   parent->nchild--;
 
   if(parent->sibbs == child) {		/* at start of parent's sibbs chain*/
      parent->sibbs = child->sibbs;
      phObjcDel(child, 0);
      
      return;
   }

   if(parent->children == child) {	/* at start of childrens' sibbs chain*/
      parent->children = child->sibbs;
      phObjcDel(child, 0);
      
      return;
   }

   for(objc = parent->children;objc != NULL;objc = objc->sibbs) {
      if(objc->sibbs == child) {	/* a sibbling not at start of chain */
	 objc->sibbs = child->sibbs;
	 phObjcDel(child, 0);

	 return;
      }
   }
   shFatal("phObjcChildDel: Cannot find the child's family");
}


   
/*****************************************************************************/
/*
 * Given an OBJC that is thought to be moving, generate proper centres
 * and atlas images
 *
 * Return 0 if OK, -1 in case of trouble; in this case, the object is
 * deleted
 */
int
phObjcDeblendMovingChild(OBJC *objc,	/* OBJC to deblend */
			 const FIELDPARAMS *fiparams) /* info about frame */

{
   float col[NCOLOR], colErr[NCOLOR];	/* estimated col centre in each band */
   float drow, dcol;			/* offset from canonical band */
   float drowErr, dcolErr;		/* errors in drow, dcol */
   int flags2;				/* flags2 bits set in velocity fit */
   int i;
   OBJECT1 *obj1;			/* == objc->color[] */
   int ndetect;				/* how many times was object found? */
   float row[NCOLOR], rowErr[NCOLOR];	/* estimated row centre in each band */

   shAssert(objc != NULL && (objc->flags2 & OBJECT2_DEBLENDED_AS_MOVING));
   shAssert(objc->parent != NULL && objc->parent->aimage != NULL);
   shAssert(fiparams != NULL);
/*
 * In how many bands was the object detected? Was this enough?
 */
   ndetect = 0;
   for(i = 0; i < objc->ncolor; i++) {
      if(objc->color[i]->flags & OBJECT1_BINNED1) {
	 ndetect++;
      }
   }
   if(ndetect <= 2) {
      objc->parent->flags2 |= (OBJECT2_TOO_FEW_DETECTIONS |
			       OBJECT2_NODEBLEND_MOVING);
      objc->parent->flags2 &= ~OBJECT2_DEBLENDED_AS_MOVING;
      phObjcChildDel(objc);
      
      return(-1);
   }
/*
 * estimate the velocity
 */
   flags2 = phVelocityFind(objc, fiparams, row, rowErr, col, colErr, NULL);

   if(flags2 & OBJECT2_TOO_FEW_GOOD_DETECTIONS) { /* too few good detections */
      objc->parent->flags2 |= (OBJECT2_TOO_FEW_GOOD_DETECTIONS |
			       OBJECT2_NODEBLEND_MOVING);
      objc->parent->flags2 &= ~OBJECT2_DEBLENDED_AS_MOVING;
      phObjcChildDel(objc);
      
      return(-1);
   } else if(flags2 & OBJECT2_BAD_MOVING_FIT) {
      objc->parent->flags2 |= (OBJECT2_BAD_MOVING_FIT_CHILD |
			       OBJECT2_NODEBLEND_MOVING);
      objc->parent->flags2 &= ~OBJECT2_DEBLENDED_AS_MOVING;
      phObjcChildDel(objc);
      
      return(-1);
   }
/*
 * If the velocity is consistent with zero, don't deblend as moving
 */
   if(flags2 & OBJECT2_STATIONARY) {
      objc->parent->flags2 |= (OBJECT2_STATIONARY |
			       OBJECT2_NODEBLEND_MOVING);
      objc->parent->flags2 &= ~OBJECT2_DEBLENDED_AS_MOVING;
      phObjcChildDel(objc);
      
      return(-1);
   }
/*
 * The positions in bands that weren't detected should be those from
 * the velocity fit, suitably transformed, rather than a suitably
 * transformed canonical centre
 */
   for(i = 0; i < objc->ncolor; i++) {
      obj1 = objc->color[i];
      shAssert(obj1 != NULL);
      
      if(obj1->flags & OBJECT1_CANONICAL_CENTER) {
	 phOffsetDo(fiparams, objc->rowc, objc->colc, 
		    fiparams->ref_band_index, i,
		    0, NULL, NULL, &drow, &drowErr, &dcol, &dcolErr);
	 obj1->colc = col[i] + dcol;
	 obj1->colcErr = sqrt(pow(colErr[i],2) + pow(dcolErr,2));
	 obj1->rowc = row[i] + drow;
	 obj1->rowcErr = sqrt(pow(rowErr[i],2) + pow(drowErr,2));
      }
   }

   return(0);
}
