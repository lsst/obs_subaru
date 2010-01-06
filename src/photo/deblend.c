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
//#include "phCellFitobj.h"
//#include "phMergeColors.h"
#include "phOffset.h"
#include "shCFitsIo.h"

#include "phFake.h"
#include "phObjc.h"

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
static double
columnDotColumn(const MAT *Q1, const MAT *Q2, /* the two matrices */
		const int c1, const int c2) /* the two columns */
	;
static void
swap_columns(MAT *Q, VEC *lambda,
	     const int c1, const int c2)
	;
static REGION *scr0 = NULL;
static REGION *scr1 = NULL;
static REGION *scr2 = NULL;
static MASK *mscr0 = NULL;

static OBJMASK *
find_maximal_objmask(const REGION *reg,	/* region that object exists in */
					 const OBJMASK *om,	/* mask of possible pixels */
					 int rin, int cin,	/* peak to be included */
					 int rout, int cout, /* peak to be excluded */
					 int low, int high,	/* range of thresholds to consider */
					 int ngrow,	 	/* number of pixels to grow mask */
					 int *critical)	/* critical threshold */
	;

static void
copy_region_within_mask(REGION *out,	/* region to copy to */
						const REGION *in, /* input region */
						const OBJMASK *mask, /* the mask to use */
						int drow, int dcol) /* offset to apply to mask */
	;

static OBJMASK *
improve_template(const OBJMASK *mask,	/* OBJC's master_mask */
				 int band,		/* which band are we processing? */
				 int rpeak, int cpeak,	/* position of peak of object */
				 REGION *data,		/* region specifying template */
				 int drow,		/* offset of mask to convert to */
				 int dcol,		/*     this band's coordinate system */
				 REGION *scra, REGION *smoothed, /* two buffers for smoothing*/
				 int rsize, int csize,	/* size of smoothing buffers */
				 float sigma,		/* s.d. of smoothing filter */
				 int filtsize,		/* size of smoothing filter */
				 int npeak_max,         /* max number of peaks/object */
				 ATLAS_IMAGE **smoothed_ai, /* ATLAS_IMAGE containing smoothed
											 templates */
				 int threshold,		/* object finding threshold */
				 int ngrow)		/* how far to grow objects */
	;

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

	{
		int m;
		for (m=0; m<objc->ncolor; m++) {
			OBJECT1* o1 = objc->color[m];
			trace("color %i: rowc,colc %g,%g\n",
				  m, o1->rowc, o1->colc);
			trace("pixel at center: %i\n", (int)fiparams->frame[m].data->rows_u16[(int)o1->rowc][(int)o1->colc]);
		}
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
			trace("Not deblending: too close to edge\n");
			return(-1);
		}
	}

	{
		int m;
		for (m=0; m<objc->ncolor; m++) {
			OBJECT1* o1 = objc->color[m];
			trace("color %i: rowc,colc %g,%g\n",
				  m, o1->rowc, o1->colc);
			trace("pixel at center: %i\n", (int)fiparams->frame[m].data->rows_u16[(int)o1->rowc][(int)o1->colc]);
		}
	}


	trace("objc:\n");
	printObjc(objc);

	trace("objc.nchild %i\n", objc->nchild);
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

	{
		int k,m;
		for (k=0; k<nchild; k++) {
			for (m=0; m<objc->ncolor; m++) {
				OBJECT1* o1 = children[k]->color[m];
				trace("child %i, color %i: rowc,colc %g,%g\n",
					  k, m, o1->rowc, o1->colc);
				trace("pixel at center: %i\n", (int)fiparams->frame[m].data->rows_u16[(int)o1->rowc][(int)o1->colc]);
			}
		}
	}


	/*
	 * decide whether any possible moving objects are really moving; if they
	 * are, deblend them as such
	 */
	if(objc->flags & OBJECT1_MOVED) {	/* there may be a moving child */
		trace("Checking moving child (flags=0x%x)...\n", objc->flags);
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

	trace("nchild %i\n", nchild);
	{
		int k,m;
		for (k=0; k<nchild; k++) {
			for (m=0; m<objc->ncolor; m++) {
				OBJECT1* o1 = children[k]->color[m];
				trace("child %i, color %i: rowc,colc %g,%g\n",
					  k, m, o1->rowc, o1->colc);
				trace("pixel at center: %i\n", (int)fiparams->frame[m].data->rows_u16[(int)o1->rowc][(int)o1->colc]);
			}
		}
	}

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
		trace("minval=%g, maxval=%g\n", minval, maxval);

		/*
		 * phMedianSmooth requires bin size < region size
		 */
		{
			//static char *dump_filename = NULL; /* write data to this file?
			//For use from gdb */
			static char *dump_filename = "median-filtered.fits";
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
		trace("Peak r,c (%i,%i)\n", peak->rpeak, peak->cpeak);
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
		trace("Highest nearby peak r,c (%i,%i)\n", peak->rpeak, peak->cpeak);

		phPeakCenterFit(peak, fiparams->frame[c].data,
						NULL, &fiparams->frame[c], bin/2, ALWAYS_SMOOTH);

		trace("Peak center (%g,%g)\n", peak->rowc, peak->colc);

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
				/*
				 (void)phFitCellAsPsf(child, c, NULL, fiparams, psf_nann[c],
				 sky_noise_only, &I0, &pedestal);
				 */
				phFitCellAsPsfFake(child, c, NULL, fiparams, psf_nann[c],
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
			trace("child %i: Not treating brightest peak as PSF\n", i);
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
			trace("child %i is PSF\n", i);
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
			trace("un-cleanable\n");
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
			trace("Discarding child %i: undetected.\n", i);
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
				trace("child %i, color %i: not detected.\n", i, c);
				continue;
			}
			trace("child %i, color %i: detected.\n", i, c);
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
			trace("re-extracting templates due to clearing OBJECT1_DEBLENDED_AS_PSF\n");
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
			trace("Dropping child %i: not OBJECT1_DETECTED\n", i);
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
		trace("<= one child left, so no need to deblend.\n");
		return(-1);
	}

	/*
	 * Average the per-band templates.
	 */
	trace("Averaging templates...\n");
	if (objc->ncolor > 1) {
		for(i = 0; i < nchild; i++) {       
			average_templates(children[i]);
		}
	}
	/*
	 * and now set up the normal equations for the least squares problem
	 */
	trace("Setting up normal equations...\n");
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
			trace("reject %i\n", reject);
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
			trace("reject %i\n", reject);
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
						trace("reject %i\n", reject);
						break;
					}
				}
			}
			trace("reject %i\n", reject);
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
						trace("reject %i\n", reject);
						break;
					}
				}
	    
				phMatDel(inner); inner = NULL;
				/*
				 * If one of a pair of templates has to be deleted, and if only one is
				 * DEBLENDED_AS_PSF, choose that one, otherwise choose the one with
				 * the _smaller_ peak value in the associated child.
				 */
				trace("reject %i\n", reject);
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
				trace("reject %i\n", reject);
				nchild = reject_template(objc, nchild, reject, children,
										 smoothed_ai, A, b, norm, lambda, Q, w);
				nparam--;

				trace("nchild %i\n", nchild);
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

	trace("Handling unassigned flux...\n");

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

	trace("Estimating sky due siblings...\n");

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

	trace("Set CR, SATUR, and INTERP flags...\n");

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
				trace("Two detections in the same band\n");
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
		trace("deblend.c: changing ndetect_min from 2 to 1\n");
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
// dstn
void phMergePeaks(PEAK* p1, PEAK* p2) {
	merge_peaks(p1, p2);
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



/*****************************************************************************/
/*
 * Find an object's deblend template.
 *
 * It's stored in the OBJC's ATLAS_IMAGE, as there exists convenient code
 * to do so. While this routine is running, however, the template is
 * accumulated in a scratch region (sym), and then within the parent
 * data region. The original pixels are preserved in the parent's
 * atlas image structure, so this is OK.
 *
 * N.b. The amplitude of the PSF fit is in objc->color[c]->profMean[0],
 * the value of the template at the original peak is in profMean[1],
 * the maximum value of the template is in profMean[2], and the amplitude
 * of the pedestal in profMean[3]
 */
#define DONT_USE_SUBTRACTED 0		/* don't use PSF-subtracted pixels */
#define MISSING_TEMPLATES_ARE_PSF 0	/* objects with no template are
									 taken to be PSFs */

static int
deblend_template_find(OBJC *objc,	/* the OBJC in question */
					  OBJMASK **psfmasks, /* masks for pixels in PSFs */
					  const FIELDPARAMS *fiparams, /* gain etc. */
					  ATLAS_IMAGE **smoothed_ai, /* ATLAS_IMAGE containing
												  smoothed templates */
					  int filtsize,	/* size of smoothing filter */
					  int ngrow)	/* how many pixels to grow objects */
{
	int aimage_drow, aimage_dcol;	/* ==objc->parent->aimage->d{row,col}*/
	int bias;				/* bias in sky level */
	int bkgd;				/* == bkgd from fiparams + SOFT_BIAS */
	int c;				/* counter in colour */
	int cmin, rmin;			/* origin of master_mask, as corrected
							 by aimage->d{row,col}[] */
	int csize, rsize;			/* size of master_mask's BBox */
	REGION *data;			/* fiparams->frame[].data */
	int i;
	OBJMASK *mmask;			/* objc->parent->aimage->master_mask */
	int my, mx1, mx2;			/* mirrored values of y, x2, x1 */
	int nrow = 0, ncol = 0;		/* == data->n{row,col} */
	int npeak_max = 10;			/* max number of peaks per object */
	OBJECT1 *obj1;			/* == objc->color[c] */
	MASK *psfmask;			/* MASK equiv to psfmasks[c] */
#if DONT_USE_SUBTRACTED
	unsigned char *psfrow, *mpsfrow;	/* psfmask->rows[] and its mirror */
#endif
	PIX *row, *mrow;			/* data[]->ROWS[] and its mirror */
	int rowc, colc;			/* centre of object */
	int ncolor;				/* == objc->ncolor */
	REGION *scra, *scrb;			/* two buffers for smoothing */
	PIX *srow;				/* sym->ROWS[] */
	REGION *sym;				/* symmetrised object */
	int template_max;			/* maximum allowed value of template */
	PIX val, mval;			/* data[]->ROWS[][] and its mirror */
	PIX tval;				/* value of template */
	int x, mx;				/* column counters */
	int y, x1, x2;			/* unpacked from mmask->s[] */

	shAssert(scr0 != NULL && scr1 != NULL && mscr0 != NULL);
	shAssert(fiparams != NULL);
	shAssert(objc->parent != NULL && objc->parent->aimage != NULL);
	shAssert(smoothed_ai != NULL);
	ncolor = objc->ncolor;
	mmask = objc->parent->aimage->master_mask;
	shAssert(mmask != NULL);
	/*
	 * set up the buffers for smoothing the initial estimates of the deblended
	 * children
	 */
	rsize = (mmask->rmax - mmask->rmin + 1);
	csize = (mmask->cmax - mmask->cmin + 1);
   
	if(rsize + filtsize > scr0->nrow || csize + filtsize > scr0->ncol ||
	   (fiparams->deblend_npix_max > 0 && mmask->npix > fiparams->deblend_npix_max)) {
		objc->parent->flags |= OBJECT1_TOO_LARGE;	/* too large to process */
		return(-1);
	} 
	/*
	 * if the object hangs over the edge of the data region in any band,
	 * give up now
	 */
	for(c = 0;c < ncolor;c++) {
		if(c == 0) {
			nrow = fiparams->frame[c].data->nrow;
			ncol = fiparams->frame[c].data->ncol;
		} else {
			shAssert(nrow == fiparams->frame[c].data->nrow);
			shAssert(ncol == fiparams->frame[c].data->ncol);
		}

		cmin = mmask->cmin + objc->parent->aimage->dcol[c];
		rmin = mmask->rmin + objc->parent->aimage->drow[c];
		if(rmin < filtsize/2 + 1 || rmin + rsize - 1 + filtsize/2 + 1 >= nrow ||
		   cmin < filtsize/2 + 1 || cmin + csize - 1 + filtsize/2 + 1 >= ncol) {
			objc->parent->flags |= OBJECT1_EDGE;
			if(objc->parent->flags2 & OBJECT2_DEBLENDED_AT_EDGE) {
				objc->parent->flags2 &= ~OBJECT2_DEBLENDED_AT_EDGE; /* it wasn't */
			}
			return(-1);
		}
	}
	/*
	 * create scratch buffers, and a region into which we symmetrise the
	 * original data.
	 *
	 * We also need an atlas image in which to save the smoothed template,
	 * so make that now
	 */
	scra = shSubRegNew("scra", scr0, rsize + filtsize, csize + filtsize,
					   0, 0, NO_FLAGS);
	scrb = shSubRegNew("scrb", scr1, rsize + filtsize, csize + filtsize,
					   0, 0, NO_FLAGS);
	sym = shSubRegNew("symmetrised", scra, rsize, csize, 0, 0, NO_FLAGS);
	psfmask = shSubMaskNew("psfmask", mscr0, rsize, csize, 0, 0, NO_FLAGS);
   
	if(*smoothed_ai != NULL) {
		phAtlasImageDel(*smoothed_ai, 0);
		for(c = 0; c < ncolor; c++) {
			phObjmaskDel(objc->aimage->mask[c]);
			phObjmaskDel(objc->color[c]->mask);
			objc->aimage->mask[c] = objc->color[c]->mask = NULL;
		}
	}

	*smoothed_ai = phAtlasImageCopy(objc->parent->aimage, 0);
	/*
	 * now go through each colour and construct the first estimate of the
	 * deblended image, based on the minimum value of pairs of pixels (unless
	 * we think that it is a PSF, in which case use that)
	 *
	 * If it wasn't detected in this band (i.e. there was no peak), don't generate
	 * a template here. We'll deal with missing templates later.
	 */
	for(c = 0;c < ncolor;c++) {
		if(!(objc->color[c]->flags & OBJECT1_DETECTED)) {
			continue;
		}
      
		data = (REGION *)fiparams->frame[c].data;

		bkgd = fiparams->frame[c].bkgd + SOFT_BIAS;
		{
			shAssert(objc->color[c] != NULL);
			const float gain = phGain(&fiparams->frame[c], objc->color[c]->rowc, objc->color[c]->colc);
			const float dark_variance = phDarkVariance(&fiparams->frame[c], objc->color[c]->rowc, objc->color[c]->colc);
			bias =				/* n.b. bias is -ve, hence "- 0.5" */
				MIN_2N_BIAS*sqrt(objc->parent->color[c]->sky/gain + dark_variance) - 0.5;
		}
		shRegIntSetVal(sym, bkgd);

		aimage_drow = objc->parent->aimage->drow[c];
		aimage_dcol = objc->parent->aimage->dcol[c];
		cmin = mmask->cmin + aimage_dcol;
		rmin = mmask->rmin + aimage_drow;
		/*
		 * unpack OBJMASK psfmasks[c] bits into MASK psfmask, as it's simpler
		 * than using the OBJMASK directly
		 */
		psfmask->row0 = rmin; psfmask->col0 = cmin;
		shMaskClear(psfmask);
		phMaskSetFromObjmask(psfmasks[c], psfmask, 1);
	   
		/*
		 * Find which pixel that peak lies in, so (40.6, 40.5) --> (40, 40)
		 * (adding 0.5 and truncating is the wrong thing to do)
		 */
		rowc = objc->color[c]->rowc;
		colc = objc->color[c]->colc;
		/*
		 * done with checks and unpacking. To work.
		 *
		 * If the object is consistent with being a PSF use a PSF as the template,
		 * otherwise use the minimum-of-pixel-pair method.
		 *
		 * Set the amplitude of the PSF to a bit more than the expected deblended
		 * amplitude, so that the best-fit coeffs will be of order unity, but
		 * truncation errors in the template won't be too bad
		 */
		if(objc->color[c]->flags & OBJECT1_DEBLENDED_AS_PSF) {
			int rad;			/* radius to mask */

			i = rowc - rmin;
			if(rowc < rmin) {		/* something's rotten in the state
									 of the astrometry */
				i = 0;
			} else if(rowc >= rmin + rsize) { /* something's wrong here too */
				i = rsize - 1;
			}
			row = sym->ROWS[i];

			template_max = objc->color[c]->profMean[0] + 100;
			objc->color[c]->profMean[2] = template_max;

			rad = 0.5*((rsize < csize) ? rsize : csize);
			if(rowc - rmin <= 0 || rowc - rmin >= sym->nrow - 1 ||
			   colc - cmin <= 0 || colc - cmin >= sym->ncol - 1) {
				objc->parent->flags2 |= OBJECT2_CENTER_OFF_AIMAGE;
				objc->parent->color[c]->flags2 |= OBJECT2_CENTER_OFF_AIMAGE;
				objc->flags &= ~OBJECT1_DETECTED; /* it'll be deleted later */

				shMaskDel(psfmask);
				phAtlasImageDel(*smoothed_ai, 0); *smoothed_ai = NULL;
				shRegDel(scra);
				shRegDel(scrb);
				shRegDel(sym);
	    
				return(0);
			}

			phDgpsfAdd(sym, fiparams->frame[c].psf, rad,
					   rowc - rmin, colc - cmin, template_max);
	 
			if(objc->color[c]->mask != NULL) {
				phObjmaskDel(objc->color[c]->mask);
			}

			rad = phDgpsfSize(fiparams->frame[c].psf, template_max, 1);
			objc->color[c]->mask = phObjmaskFromCircle(rowc, colc, rad);
			template_max += SOFT_BIAS;
		} else {				/* not a PSF; symmetrise image */
			template_max = objc->color[c]->profMean[2]; /* max. val. of template*/
			template_max += SOFT_BIAS;

			for(i = 0;i < mmask->nspan;i++) {
				y = mmask->s[i].y + aimage_drow;
				x1 = mmask->s[i].x1 + aimage_dcol;
				x2 = mmask->s[i].x2 + aimage_dcol;
				if(y < 0 || y >= nrow) {
					continue;
				}
				if(x1 < 0) x1 = 0;
				if(x2 >= ncol) x2 = ncol - 1;
	    
				my = rowc + (rowc - y);
				mx1 = colc + (colc - x2); mx2 = colc + (colc - x1); /* mx1 <= mx2*/
				/*
				 * check if the mirrored span lies outside mmask's bounding box; if so, leave
				 * sym at the background level
				 */
				if(my < rmin || my >= rmin + rsize ||
				   mx2 < cmin || mx1 >= cmin + csize) {
					continue;
				}
	    
				if(mx1 < cmin) {		/* partially outside psfmask */
					mx1 = cmin;
					x2 = colc - (mx1 - colc);
				}
				if(mx2 >= cmin + csize) {	/* partially outside psfmask */
					mx2 = cmin + csize - 1;
					x1 = colc - (mx2 - colc);
				}
				/*
				 * OK, everything's in range. Time to take the minimum of the pixel and
				 * its mirror. The minimum of two N(0,1) variates is biased by MIN_2N_BIAS,
				 * so correct for this effect in the sky level.
				 */
				shAssert(y - rmin >= 0 && y - rmin < rsize);
				shAssert(y >= 0 && y < nrow && my >= 0 && my < nrow);
				row = data->ROWS[y];
				mrow = data->ROWS[my];
#if DONT_USE_SUBTRACTED			/* don't use PSF-subtracted pixels */
				psfrow = psfmask->rows[y - rmin];
				mpsfrow = psfmask->rows[my - rmin];
#endif
				srow = sym->ROWS[y - rmin];
				for(x = x1, mx = mx2;x <= x2;x++, mx--) {
					shAssert(x - cmin >= 0 && x - cmin < csize);
					shAssert(x >= 0 && x < ncol && mx >= cmin && mx < cmin + csize);
					val = row[x];
					mval = mrow[mx];
#if DONT_USE_SUBTRACTED			/* don't use PSF-subtracted pixels */
					if(psfrow[x - cmin] != 0) { /* pixel is part of a PSF object */
						if(mpsfrow[mx - cmin] != 0) {	/* so is mirror pixel */
							tval = 0.5*(val + mval);
						} else {
							tval = mval;
						}
					} else if(mpsfrow[mx - cmin] != 0) {
						tval = val;
					} else {
						tval = (val < mval ? val : mval) - bias;
					}
#else
					tval = (val < mval ? val : mval) - bias;
#endif
#if 0
					srow[x - cmin] = (tval < template_max) ? tval : template_max;
#else
					srow[x - cmin] = tval;
#endif
				}
			}
		}
		/*
		 * copy the symmetrised image back to the original data region, where
		 * it will become the deblending template (the original pixel values
		 * are preserved in the parent's atlas image).
		 *
		 * We must of course only do this within the master_mask
		 */
		copy_region_within_mask((REGION *)data, sym, mmask,
								aimage_drow, aimage_dcol);
		/*
		 * Possibly dump image for debugging
		 */
		{
			static char *dump_filename = NULL; /* write data to this file?
												For use from gdb */
			if(dump_filename != NULL) {
				shRegWriteAsFits(data,
								 dump_filename, STANDARD, 2,DEF_NONE,NULL,0);
				dump_filename = NULL;
			}
		}
		/*
		 * we next want to run the object finder on that symmetrised image; the image
		 * is smoothed, and extra peaks rejected --- see improve_template() for details
		 */
		obj1 = objc->color[c];
		if(obj1->flags & OBJECT1_DEBLENDED_AS_PSF) {
			/*
			 * no need to check template, as we created it as a multiple of PSF
			 */
		} else {
			float sigma = 4*fiparams->frame[c].smooth_sigma; /* smoothing sigma XXX multiplier */
			float threshold = fiparams->frame[c].ffo_threshold; /* XXX Reconsider this threshold */
			if (sigma != 0.0) {
				threshold /= sigma;
			}

			shAssert(obj1->mask == objc->aimage->mask[c]);
			phObjmaskDel(obj1->mask); objc->aimage->mask[c] = NULL;
			obj1->mask = 
				improve_template(mmask, c, rowc,colc, data, aimage_drow,aimage_dcol,
								 scra, scrb, rsize + filtsize, csize + filtsize,
								 sigma, filtsize, npeak_max, smoothed_ai, threshold, ngrow);

			if(obj1->mask == NULL) {
				objc->flags &= ~OBJECT1_DETECTED;
				obj1->flags &= ~OBJECT1_DETECTED;
			}
		}
	}
	/*
	 * Possibly dump image for debugging
	 */
	for(c = 0; c < ncolor; c++) {
		static char *dump_filename = NULL; /* write data to this file?
											For use from gdb */
		if(dump_filename != NULL) {
			shRegWriteAsFits((REGION *)fiparams->frame[c].data,
							 dump_filename, STANDARD, 2,DEF_NONE,NULL,0);
			dump_filename = NULL;
		}
	}
	/*
	 * we've found the templates in all colours. They are represented by the
	 * pixels in the original data region, within the OBJECT1->mask
	 *
	 * Now go through them looking for objects which we didn't detect
	 * in any band; in this case, the object wouldn't have been found at all
	 * if it wasn't part of a blend, so dump it.
	 *
	 * Actually we cannot just dump it here as we've got an array with all the
	 * children in it, and we'd have to move the others down. Instead, mark
	 * the entire OBJC as not DETECTED, and we'll dump it when we get a chance.
	 */
	for(c = 0;c < ncolor;c++) {
		objc->flags |= (objc->color[c]->flags & OBJECT1_DETECTED);
	}
	if(phStrategicMemoryReserveIsEmpty() ||
	   !(objc->flags & OBJECT1_DETECTED)) { /* not detected in any band */
		phAtlasImageDel(*smoothed_ai, 0); *smoothed_ai = NULL;
		shRegDel(scra);
		shRegDel(scrb);
		shRegDel(sym);
		shMaskDel(psfmask);

		return(0);
	}
#if MISSING_TEMPLATES_ARE_PSF == 1
	/*
	 * create minimal templates in the bands where we failed to detect
	 * the object; we need these to define the deblended objects, and
	 * to cut atlas images. See note above call to copy_region_within_mask()
	 * as to why it's OK to cast away the const for fiparams->frame[c].data
	 */
	for(c = 0;c < ncolor;c++) {
		obj1 = objc->color[c];

		if(!phStrategicMemoryReserveIsEmpty() && obj1->mask == NULL) {
			obj1->mask = phObjmaskFromCircle(obj1->rowc, obj1->colc, 2 + ngrow);
			obj1->flags |= OBJECT1_DEBLENDED_AS_PSF;
			phRegIntSetValInObjmask((REGION *)fiparams->frame[c].data,
									obj1->mask, bkgd);
			phDgpsfAdd((REGION *)fiparams->frame[c].data, fiparams->frame[c].psf,
					   0.5*((rsize < csize) ? rsize : csize),
					   obj1->rowc, obj1->colc, 100);
		}
	}
#endif
	/*
	 * Time to cut an "atlas image"; it's actually the templates in each band
	 */
	phAtlasImageCut(objc, -1, fiparams, -1, -1, NULL);

#if MISSING_TEMPLATES_ARE_PSF == 0
	/*
	 * We may not have templates in some bands; go ahead and make them from the
	 * average of the bands where we _do_ have them
	 */
	for(c = 0;c < ncolor;c++) {
		if(objc->color[c]->mask == NULL) {
			break;
		}
	}

	if(!phStrategicMemoryReserveIsEmpty() && c != ncolor) { /* at least one missing template */
		const int missing0 = c;		/* first missing template */
		int ngood = 0;			/* number of good templates */
		OBJMASK *uni = NULL;		/* union of all non-NULL obj1->masks,
									 in canonical coordinates */
      
		phAtlasImageConstSet(objc->aimage, missing0, SOFT_BIAS);
      
		for(c = 0;c < ncolor;c++) {
			if(objc->color[c]->mask != NULL) {
				ngood++;

				uni = phObjmaskMerge(uni, objc->color[c]->mask, 
									 -objc->parent->aimage->drow[c],
									 -objc->parent->aimage->dcol[c]);
	    

				phAtlasImagesPlusEquals(objc->aimage, missing0, objc->aimage, c, 0);
			}
		}
		/*
		 * subtract bias from templates. They are scaled up by a factor of ngood,
		 * but that doesn't really matter (their fitted amplitudes will be smaller)
		 */
		phAtlasImageConstAdd(objc->aimage, missing0, -SOFT_BIAS*ngood, 0);
		/*
		 * Generate the correct masks in each missing band and copy over the
		 * average template
		 */
		shAssert(uni != NULL);
		(void)phObjmaskAndObjmask(uni, objc->parent->aimage->master_mask);

		for(c = missing0; c < ncolor; c++) {
			if(objc->color[c]->mask == NULL) {
				objc->color[c]->mask =
					phObjmaskCopy(uni, objc->parent->aimage->drow[c],
								  objc->parent->aimage->dcol[c]);

				phAtlasImagesPixelsCopy(objc->aimage, c, objc->aimage, missing0);
			}
		}

		phObjmaskDel(uni);
	}
#endif
	/*
	 * Did we give up?
	 */
	if(phStrategicMemoryReserveIsEmpty()) {
		shRegDel(scra);
		shRegDel(scrb);
		shRegDel(sym);
		shMaskDel(psfmask);
       
		return(-1);
	}
	/*
	 * We need smoothed templates too, even for DEBLENDED_AS_PSF objects.
	 * The unsmoothed one is noise-free, so we don't really need to smooth,
	 * and it's simpler not to bother
	 */
	for(c = 0;c < ncolor;c++) {
		if(objc->color[c]->flags & OBJECT1_DEBLENDED_AS_PSF) {
			phAtlasImagesPixelsCopy(*smoothed_ai, c, objc->aimage, c);
		}
	}
	/*
	 * Remove the background from the templates (we leave in the SOFT_BIAS)
	 */
	for(c = 0;c < ncolor;c++) {
		phAtlasImageConstAdd(objc->aimage, c, -fiparams->frame[c].bkgd, 0);
		phAtlasImageConstAdd(*smoothed_ai, c, -fiparams->frame[c].bkgd, 0);
	}   
	/*
	 * Set the parts of the template that aren't in obj1->mask to 0
	 */      
	for(c = 0;c < ncolor;c++) {
		obj1 = objc->color[c];

		phAtlasImageSetIfNotInObjmask(objc->aimage, c, obj1->mask, 0);

		phAtlasImageSetIfNotInObjmask(*smoothed_ai, c, obj1->mask, 0);
	}
	/*
	 * and clean up
	 */
	shRegDel(scra);
	shRegDel(scrb);
	shRegDel(sym);
	shMaskDel(psfmask);

	return(0);				/* OK */
}



/*****************************************************************************/
/*
 * return a better estimate of a deblending template. The template is
 * specified as the values of the data region, within the master_mask
 *
 * We pass in the scratch buffers purely for efficiency; that way we
 * can allocate them once per object rather than once per colour
 *
 * If no template is detected after smoothing, return NULL. Otherwise
 * return the mask of pixels in the template
 */
static OBJMASK *
improve_template(const OBJMASK *mask,	/* OBJC's master_mask */
				 int band,		/* which band are we processing? */
				 int rpeak, int cpeak,	/* position of peak of object */
				 REGION *data,		/* region specifying template */
				 int drow,		/* offset of mask to convert to */
				 int dcol,		/*     this band's coordinate system */
				 REGION *scra, REGION *smoothed, /* two buffers for smoothing*/
				 int rsize, int csize,	/* size of smoothing buffers */
				 float sigma,		/* s.d. of smoothing filter */
				 int filtsize,		/* size of smoothing filter */
				 int npeak_max,         /* max number of peaks/object */
				 ATLAS_IMAGE **smoothed_ai, /* ATLAS_IMAGE containing smoothed
											 templates */
				 int threshold,		/* object finding threshold */
				 int ngrow)		/* how far to grow objects */
{
	int i;
	unsigned short levels[1];		/* thresholds for object finder */
	int nobj;				/* number of objects detected */
	CHAIN *objs;				/* all objects in mask above levels */
	OBJECT *obj = NULL;			/* an object on the objs list */
	OBJMASK *objmask;			/* OBJMASK of desired object */
	int r0, c0;				/* origin of subdata in data */
	REGION *subdata;			/* subregion around peak */
   
	c0 = mask->cmin + dcol - filtsize/2;
	r0 = mask->rmin + drow - filtsize/2;
	shAssert(r0 >= 0 && r0 + rsize - 1 < data->nrow);
	shAssert(c0 >= 0 && c0 + csize - 1 < data->ncol);
	/*
	 * smooth the input data. Note that we don't own smoothed, so no memory
	 * leak is occuring when we set it to a subregion of itself
	 */
	subdata = shSubRegNew("", data, rsize, csize, r0, c0, NO_FLAGS);
	if(sigma <= 0) {
		smoothed = (REGION *)subdata;
	} else {
		phConvolveWithGaussian(smoothed, subdata, scra, filtsize, sigma,
							   0, CONVOLVE_ANY);
	}
	smoothed = shSubRegNew("", smoothed, rsize - filtsize, csize - filtsize,
						   filtsize/2, filtsize/2, NO_FLAGS);
	smoothed->row0 = mask->rmin; smoothed->col0 = mask->cmin;
	/*
	 * save the smoothed image in an atlas image of its own. We'll use this
	 * for assigning the light in pixels with very low S/N per pixel
	 */
	{
		const int s_drow = (*smoothed_ai)->drow[band];
		const int s_dcol = (*smoothed_ai)->dcol[band];

		(*smoothed_ai)->drow[band] = (*smoothed_ai)->dcol[band] = 0;
		phAtlasImageSetFromRegion(*smoothed_ai, band, smoothed);
		(*smoothed_ai)->drow[band] = s_drow; (*smoothed_ai)->dcol[band] = s_dcol;
	}
	/*
	 * set a suitable set of thresholds
	 */
	levels[0] = SOFT_BIAS + threshold;
	/*
	 * now look for objects. Note that the mask is the object's master_mask,
	 * and is therefore in canonical coordinates. We faked the origin of
	 * smoothed to allow for this, but as a consequence the detected objects
	 * are also in the canonical coordinate system. For simplicity, we'll
	 * convert cpeak and rpeak too
	 */
	rpeak -= drow; cpeak -= dcol;

	objs = phObjectsFindInObjmask(smoothed, mask, 1, levels, -1, 0, 100*npeak_max); /* XXX 100 */
	shAssert(objs != NULL);
	nobj = objs->nElements;
	/*
	 * Dump a region for debugging?
	 */
	{
		static char *dump_filename = NULL; /* write data to this file?
											For use from gdb */
		if(dump_filename != NULL) {
			shRegWriteAsFits(smoothed,
							 dump_filename, STANDARD, 2,DEF_NONE,NULL,0);
			dump_filename = NULL;
		}
	}
	/*
	 * Look for the peak that led to this template
	 */
	for(i = 0;i < nobj;i++) {
		obj = shChainElementRemByPos(objs,HEAD);
		/*
		 * is this our object, that is, does it contain our peak?
		 */
		if(phPixIntersectObjmask(obj->sv[0], cpeak, rpeak)) {
			i++;
			break;
		}
		phObjectDel(obj); obj = NULL;
	}
	for(;i < nobj;i++) {			/* destroy all unwanted objects */
		phObjectDel(shChainElementRemByPos(objs, HEAD));
	}
	shChainDel(objs);
	/*
	 * Correct the peaks for smoothed's origin ; this should really be done by
	 * phObjectsFindInObjmask()
	 */
	if(obj != NULL) {
		for(i = 0; i < obj->peaks->npeak; i++) {
			obj->peaks->peaks[i]->rpeak += smoothed->row0;
			obj->peaks->peaks[i]->rowc += smoothed->row0;
			obj->peaks->peaks[i]->cpeak += smoothed->col0;
			obj->peaks->peaks[i]->colc += smoothed->col0;
		}
	}
	/*
	 * process our object. We found it with a variety of different thresholds,
	 * and in general the higher detection thresholds will lead to non-simply-
	 * -connected OBJMASKs. If this is the case (i.e. if it has more than one
	 * peak) we now remove the non-connected parts from the template.
	 */
	if(obj == NULL) {
		objmask = NULL;
	} else {
		OBJMASK *clipped = phObjmaskNew(0); /* which pixels have been clipped */
		int critical = 0;			/* critical threshold to split peaks */
		OBJMASK *max_mask;		/* maximal mask including a peak,
								 but excluding main peak */
		PEAK *peak;			/* a peak in the object */

		if(obj->peaks->npeak > 1) {
			for(i = 0; i < obj->peaks->npeak; i++) {
				peak = obj->peaks->peaks[i];
				/*
				 * find the lowest threshold that includes peak->[cr]peak but not [cr]peak.
				 */
				max_mask = find_maximal_objmask(smoothed, obj->sv[0],
												peak->rpeak, peak->cpeak,
												rpeak, cpeak,
												SOFT_BIAS, SOFT_BIAS + peak->peak,
												ngrow, &critical);
#if 0
				if(max_mask != NULL && max_mask->npix > 2) { /* XXX */
					phObjmaskAndNotObjmask(max_mask, clipped);
					phRegIntClipValInObjmask(data, max_mask, 0, 0, critical);
					phRegIntClipValInObjmask(data, max_mask, 0, 0, SOFT_BIAS);
					phObjmaskOrObjmask(clipped, max_mask);
				}
#else
				if(max_mask != NULL && max_mask->nspan > 0) {
					OBJMASK *grown = phObjmaskGrow(max_mask, smoothed, ngrow);
					phObjmaskAndNotObjmask(grown, clipped);
					phRegIntClipValInObjmask(data, grown, 0, 0, critical);
					phObjmaskOrObjmask(clipped, grown);
					phObjmaskDel(grown);
				}
#endif
				phObjmaskDel(max_mask);	/* may be NULL */
			}
		}
		phObjmaskDel(clipped);
		/*
		 * grow the original mask, and convert to object's coordinate system
		 */
		{
			OBJMASK *grown = phObjmaskGrow(obj->sv[0], smoothed, ngrow);
			objmask = phObjmaskCopy(grown, drow, dcol);
			phObjmaskDel(grown);
		}

		phObjectDel(obj);
	}
	/*
	 * clean up
	 */
	shRegDel(subdata);
	if(smoothed != data) {
		shRegDel(smoothed);
	}
   
	return(objmask);
}

/*****************************************************************************/
/*
 * Find the largest objmask that _excludes_ the point (rout, cout)
 * but _includes_ (rin, cin) in the mask om
 */
static OBJMASK *
find_maximal_objmask(const REGION *reg,	/* region that object exists in */
					 const OBJMASK *om,	/* mask of possible pixels */
					 int rin, int cin,	/* peak to be included */
					 int rout, int cout, /* peak to be excluded */
					 int low, int high,	/* range of thresholds to consider */
					 int ngrow,	 	/* number of pixels to grow mask */
					 int *critical)	/* critical threshold */
{
	OBJMASK *highmask = NULL;		/* mask corresponding to level high */
	int mid;				/* intermediate threshold */
	const OBJMASK *mask = om;		/* desired mask */
	OBJMASK *midmask = NULL;		/* mask corresponding to level mid */
	/*
	 * find critical threshold by binary search
	 */
	for(;;) {
		mid = (low + high)/2;
		if(mid == low) {			/* got it */
			break;
		}

		midmask = phObjmaskFindInObjmask(reg, mask, rin, cin, mid);
		if(midmask == NULL) {
			high = mid;
		} else {
			if(phPixIntersectObjmask(midmask, cout, rout)) {
				low = mid;
				if(mask != om) {
					phObjmaskDel((OBJMASK *)mask);
				}
				mask = midmask; midmask = NULL;
			} else {
				high = mid;
				phObjmaskDel(highmask);
				highmask = midmask; midmask = NULL;
			}
		}
	}
	phObjmaskDel(midmask);
	/*
	 * find the mask that didn't _quite_ contain both peaks. If highmask is
	 * NULL it must because the original high threshold was never used, so
	 * use it now and see if it includes (rout, cout)
	 */
	if(highmask == NULL) {
		if(high == low) high++;
      
		highmask = phObjmaskFindInObjmask(reg, mask, rin, cin, high);
		if(highmask != NULL && phPixIntersectObjmask(highmask, cout, rout)) {
			phObjmaskDel(highmask);
			highmask = NULL;
		}
	}
	shAssert(highmask == NULL || !phPixIntersectObjmask(highmask,cout,rout));
#if 1
	/*
	 * grow highmask, but not into object containing (rout, cout)
	 */
	if(ngrow > 0 && highmask != NULL && highmask->npix > 2) { /* npix XXX */
		OBJMASK *grown = phObjmaskGrow(highmask, reg, ngrow);
		OBJMASK *out_mask;	 /* pixels above critical containing (rout, cout) */
		phObjmaskDel(highmask);
		out_mask = phObjmaskFindInObjmask(reg, mask, rout, cout,
										  SOFT_BIAS + 4*(*critical - SOFT_BIAS));
		if(out_mask != NULL) {	 /* grow object containing desired peak, (rout,cout) */
			OBJMASK *gout_mask = phObjmaskGrow(out_mask, reg, ngrow);
			phObjmaskDel(out_mask);
			out_mask = gout_mask;
		}
		highmask = phObjmaskAndNotObjmask(grown, out_mask);
		phObjmaskDel(out_mask);
	}
#endif
	/*
	 * clean up
	 */
	if(mask != om) {
		phObjmaskDel((OBJMASK *)mask);
	}
	/*
	 * return desired mask and threshold
	 */
	if(critical != NULL) {
		*critical = mid;
	}

	return(highmask);
}

/*****************************************************************************/
/*
 * copy the pixels in <in> to <out> within the <mask>
 *
 * The region <in> is supposed to have its origin at the point (rmin,cmin)
 */
static void
copy_region_within_mask(REGION *out,	/* region to copy to */
						const REGION *in, /* input region */
						const OBJMASK *mask, /* the mask to use */
						int drow, int dcol) /* offset to apply to mask */
{
	int i;
	const int nrow = out->nrow;
	const int ncol = out->ncol;
	const int cmin = mask->cmin + dcol;
	const int rmin = mask->rmin + drow;
	int x1, x2, y;			/* unpacked from a span */
   
	for(i = 0;i < mask->nspan;i++) {
		y = mask->s[i].y + drow;
		x1 = mask->s[i].x1 + dcol; x2 = mask->s[i].x2 + dcol;
		if(y < 0 || y >= nrow) {
			continue;
		}
		if(x1 < 0) x1 = 0;
		if(x2 >= ncol) x2 = ncol - 1;
      
		memcpy(&out->ROWS[y][x1], &in->ROWS[y - rmin][x1 - cmin],
			   (x2 - x1 + 1)*sizeof(PIX));
	}
}

/*****************************************************************************/
/*
 * If we are reduced to only a single child, make sure that the parent
 * has the child's positions in all bands; as the parent was a deblend
 * candidate it may have a mixture of the candidate peaks
 */
static void
set_parent_position_from_child(OBJC *objc,
							   const OBJC *child)
{
	int c;

	objc->flags &= ~OBJECT1_CANONICAL_CENTER;
	objc->flags |= (child->flags & OBJECT1_CANONICAL_CENTER);
	objc->rowc = child->rowc;
	objc->rowcErr = child->rowcErr;
	objc->colc = child->colc;
	objc->colcErr = child->colcErr;
   
	for(c = 0; c < objc->ncolor; c++) {
		objc->color[c]->flags &= ~OBJECT1_CANONICAL_CENTER;
		objc->color[c]->flags |=
			(child->color[c]->flags & OBJECT1_CANONICAL_CENTER);
		objc->color[c]->rowc = child->color[c]->rowc;
		objc->color[c]->rowcErr = child->color[c]->rowcErr;
		objc->color[c]->colc = child->color[c]->colc;
		objc->color[c]->colcErr = child->color[c]->colcErr;
	}
}


/*****************************************************************************/
/*
 * Average the per-band templates.
 *
 * This is a bit of a pain... First set the pixels outside the detection footprint to SOFT_BIAS (not 0),
 * then add a fraction 1/ncolor of each template to the 0th (also scaled down by 1/ncolor); this scaling
 * avoids overflows.  Then clip the resulting mean template to the union of the input masks, and copy
 * it back into the per-band templates.
 */
static void
average_templates(OBJC *child)
{
    int c;
    OBJMASK *mask = phObjmaskCopy(child->color[0]->mask, 0, 0);
    
    for(c = 0; c < child->ncolor;c++) {
		OBJECT1 *obj1 = child->color[c];
		phAtlasImageSetIfNotInObjmask(child->aimage, c, obj1->mask, SOFT_BIAS);

		phAtlasImagesTimesEquals(child->aimage, c, 1/(float)child->ncolor);
		if (c > 0) {
			phObjmaskOrObjmask(mask, child->color[c]->mask);
			phAtlasImagesPlusEquals(child->aimage, 0, child->aimage, c, 0);
		}
    }
	   
    for(c = 0; c < child->ncolor;c++) {
		phAtlasImageSetIfNotInObjmask(child->aimage, c, mask, 0);
		if (c > 0) {
			phAtlasImagesPixelsCopy(child->aimage, c, child->aimage, 0);
		}
    }

    phObjmaskDel(mask);
}

/************************************************************************************************************/
/*
 * Given a set of Q and lambda (the eigenvectors and values of a matrix),
 * one for each of a set of colours, sort the eigenobjects so that they
 * are in the same order as the canonical colour.  This is done by looking
 * at each eigenvector in turn, and seeing with which eigenvector in
 * Q[canonical_color] it has the largest |inner product|
 */
static void
sort_Q(const int ncolor,		/* dimen of lambda and Q arrays */
       const int canonical_color,	/* sort to match this colour */
       VEC **lambda,			/* eigenvalues */
       MAT **Q)				/* eigenvectors */
{
    int c;
    int i, j;

    for (c = 0; c < ncolor; c++) {
	if (c == canonical_color) continue; /* already sorted */

	for (i = 0; i < Q[c]->m; i++) {
	    int best_column = i;
	    double best_dot = columnDotColumn(Q[canonical_color], Q[c], i, best_column);
	    for (j = i + 1; j < Q[c]->m; j++) {
		double dot = columnDotColumn(Q[canonical_color], Q[c], i, j);
		if (fabs(dot) > fabs(best_dot)) {
		    best_column = j;
		    best_dot = dot;
		    
		}
	    }
	    swap_columns(Q[c], lambda[c], i, best_column);
	}
    }
}

/*****************************************************************************/
/*
 * Copy peaks in reject into keep, if the object isn't detected in that
 * band in object keep.  Move the centroid over too, as it came from the peaks
 */
static void
transfer_centers_and_peaks(OBJC *keep, OBJC *reject)
{
   int c;
   
   for(c = 0; c < keep->ncolor; c++) {
      if((keep->color[c]->flags & OBJECT1_DETECTED) != 0 ||
	 reject->color[c]->peaks == NULL) {
	 continue;			/* nothing to do */
      }

      phPeaksDel(keep->color[c]->peaks);
      keep->color[c]->peaks = reject->color[c]->peaks;
      reject->color[c]->peaks = NULL;
/*
 * We should copy over the centers too, but it seems rather scary to do
 * so at this late stage (v5_4_24).
 */
#if 1
      keep->color[c]->rowc = reject->color[c]->rowc;
      keep->color[c]->rowcErr = reject->color[c]->rowcErr;
      keep->color[c]->colc = reject->color[c]->colc;
      keep->color[c]->colcErr = reject->color[c]->colcErr;
#endif
      
      keep->flags &= ~OBJECT1_CANONICAL_CENTER; /* we need reject's value of */
      keep->color[c]->flags &= ~OBJECT1_CANONICAL_CENTER; /* CANONICAL_CENTER*/
      
      keep->flags |= reject->color[c]->flags & OBJECT1_DETECTED;
      keep->color[c]->flags |= reject->color[c]->flags & OBJECT1_DETECTED;
   }
}

/*
 * Set up the normal equations, Aw = b
 */
static void
setup_normal(const OBJC *parent,	/* parent object */
	     const OBJC **children,	/* array of children */
	     int nchild,		/* dimension of children[] */
	     int c,			/* the colour in question */
	     int bkgd,			/* == (bkgd from fparams) + SOFT_BIAS*/
	     MAT *A,			/* The LSQ problem is */
	     VEC *b,			/*    A w = b */
	     VEC *norm)			/* normalisation of templates */
{
   int i, j;
   float *maxpix = alloca(nchild*sizeof(float)); /* max. value for templates */
   int npix = parent->aimage->master_mask->npix;

   for(i = 0;i < nchild;i++) {
      shAssert(children[i]->aimage->master_mask->npix == npix);
   }
/*
 * First A.
 */
   for(i = 0;i < nchild;i++) {
      for(j = 0;j <= i;j++) {
	  A->me[i][j] = A->me[j][i] = phAtlasImageDotProduct(children[i]->aimage, c, bkgd,
							     children[j]->aimage, c, bkgd);
      }
   }
/*
 * and now b
 */
   for(i = 0;i < nchild; i++) {
      b->ve[i] = phAtlasImageDotProduct(children[i]->aimage, c, bkgd,
					parent->aimage,      c, bkgd);

      maxpix[i] = phAtlasImageMaximum(parent->aimage, c) - bkgd;
      if(maxpix[i] <= 0) {
	  maxpix[i] = 1;			/* don't divide by zero */
      }
   }
/*
 * Now renormalise to try to control overflows etc., and to make the values
 * of the eigenvalues more comprehensible. We renormalise each template
 * to have a maximum value of unity rather than an rms of unity to avoid
 * problems with poorly known sky levels
 */
   for(i = 0;i < nchild;i++) {
      norm->ve[i] = maxpix[i];
      b->ve[i] /= maxpix[i];
      for(j = 0;j <= i;j++) {
	 A->me[i][j] /= maxpix[i]*maxpix[j];
	 A->me[j][i] = A->me[i][j];
      }
   }
/*
 * write out template and data vectors, for debugging
 */
#if 0
   {
      FILE *fil = fopen("template.dat","w");
      if(fil != NULL) {
	  const OBJMASK *mmask = parent->aimage->master_mask;
	  for(k = 0; k < mmask->nspan; k++) {
	      int x1 = mmask->s[k].x1, x2 = mmask->s[k].x2;
	      int x;
	      int y = mmask->s[k].y;

	      fprintf(fil,"(%d, %d): ", y, x);
	      for(x = x1; x <= x2; x++) {
		  fprintf(fil," %d", phAtlasImagePixelGet(parent->aimage, c, y, x));
		  for(j = 0; j < nchild;j++) {
		      fprintf(fil," %d", phAtlasImagePixelGet(children[j]->aimage, c, y, x));
		  }
	      }
	      fprintf(fil,"\n");
	  }
	  fclose(fil);
      }
   }
#endif
}

/*****************************************************************************/
/*
 * Reject a template
 */
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
{
   int c;
   int i;

   objc->flags |= OBJECT1_DEBLEND_PRUNED;
   for(c = 0; c < objc->ncolor; c++) {
      objc->color[c]->flags |= OBJECT1_DEBLEND_PRUNED;
   }
/*
 * and reject it
 */
   phObjcChildDel(children[reject]);
   phAtlasImageDel(smoothed_ai[reject], 0);
   for(i = reject;i < nchild - 1;i++) { /* move the others down */
      children[i] = children[i + 1];
      smoothed_ai[i] = smoothed_ai[i + 1];
   }
   nchild--;
   
   for(c = 0; c < objc->ncolor; c++) { /* reject from Normal eqs.*/
      A[c] = phMatDelRowCol(A[c], reject, reject);
      b[c] = phVecDelElement(b[c], reject);
      norm[c] = phVecDelElement(norm[c], reject);
      phVecDel(w[c]); w[c] = NULL;
   }

   if(nchild <= 1) {
      for(c = 0; c < objc->ncolor; c++) {
	 phMatDel(A[c]);
	 phVecDel(b[c]);
	 phVecDel(norm[c]);
	 phVecDel(lambda[c]);
	 phMatDel(Q[c]);
      }
      
      if(nchild == 1) {
	 set_parent_position_from_child(objc, children[0]);
      }
      
      objc->flags |= OBJECT1_NODEBLEND;
      for(c = 0; c < objc->ncolor; c++) {
	 objc->color[c]->flags |= OBJECT1_NODEBLEND;
      }
      shFree(children);
      for(i = 0;i < nchild;i++) {
	 phAtlasImageDel(smoothed_ai[i], 0);
      }
      shFree(smoothed_ai);
      return(0);
   }

   return(nchild);
}

/*****************************************************************************/
/*
 * look at the potential children and see if there are any `local'
 * modifications to the list that would improve the deblend.
 *
 * Initially, the only such case is re-assembling unrecognised moving objects
 */

static int
peephole_optimizer(OBJC *objc,		/* parent */
		   const FIELDPARAMS *fiparams, /* astrometry etc. */
		   int nchild,		/* number of children */
		   OBJC *children[],	/* list of children */
		   ATLAS_IMAGE **smoothed_ai)/* list of atlas images containing
						smoothed templates. Not used,
						but may need to be freed */
		   
{
   int c;				/* counter in colour */
   float col[NCOLOR], colErr[NCOLOR];	/* estimated col centre in each band */
   OBJC *child;				/* == children[] */
   int *detected;			/* Which bands are present
						   in each of the children[] */
   float drow[NCOLOR], dcol[NCOLOR];	/* _Add_ to convert to canon. band */
   float drowErr[NCOLOR], dcolErr[NCOLOR]; /* errors in drow/dcol */
   int i, j;
   OBJC *merged = NULL;			/* candidate merged object */
   const float min_peak_spacing = fiparams->deblend_min_peak_spacing;
   int did_merge = 0;			/* did I merge any bands together? */
   float row[NCOLOR], rowErr[NCOLOR];	/* estimated row centre in each band */
/*
 * Find astrometric offsets
 */
   for(c = 0; c < objc->ncolor; c++) {
      phOffsetDo(fiparams, objc->rowc, objc->colc, 
		 c, fiparams->ref_band_index,
		 0, NULL, NULL, &drow[c], &drowErr[c], &dcol[c], &dcolErr[c]);
   }
/*
 * Go through list of children setting bits for which bands are present;
 * not totally by coincidence these are the same as PEAK_BAND0 etc. although
 * we don't use this fact.
 */
   detected = shMalloc(nchild*sizeof(int));

   for(i = 0; i < nchild; i++) {
      child = children[i];
      detected[i] = 0;
      for(c = 0; c < objc->ncolor; c++) {
	 if(child->color[c]->flags & OBJECT1_DETECTED) {
	    detected[i] |= (1 << c);
	 }
      }
   }
/*
 * Go through all pairs of children that don't have any peaks in
 * common, and see if it makes sense to merge them
 */
   for(i = 0; i < nchild; i++) {
      for(j = i + 1; j < nchild; j++) {
	 if((detected[i] & detected[j]) != 0) { /* they have bands in common */
	    continue;
	 }
	 /*
	  * Copy OBJECT1s from i and j into merged
	  */
	 if(merged == NULL) {
	    merged = phObjcNew(objc->ncolor);
	 }
	 for(c = 0; c < objc->ncolor; c++) {
	    if((detected[i] & (1 << c))) {
	       merged->color[c] = children[i]->color[c];
	    } else if((detected[j] & (1 << c))) {
	       merged->color[c] = children[j]->color[c];
	    }
	 }
	 /*
	  * estimate the velocity
	  */
	 if(phVelocityFind(merged, fiparams,
			   row, rowErr, col, colErr, NULL) != 0) {
	    for(c = 0; c < objc->ncolor; c++) {	/* Not a good candidate */
	       merged->color[c] = NULL;
	    }
	    
	    continue;
	 }
	 /*
	  * Hmm, a good candidate for a merger.  Replace i with merged
	  * (well, actually overlay i with merged) and prepare to discard j
	  */
	 {
	    long mask = ~(OBJECT2_NODEBLEND_MOVING |
			  OBJECT2_BAD_MOVING_FIT |
			  OBJECT2_BAD_MOVING_FIT_CHILD |
			  OBJECT2_TOO_FEW_DETECTIONS);

	    objc->flags2 &= mask;
	    children[i]->flags2 &= mask;
	 }

	 objc->flags2 |=
	    OBJECT2_DEBLENDED_AS_MOVING | OBJECT2_DEBLEND_PEEPHOLE;
	 children[i]->flags2 |=
	    OBJECT2_DEBLENDED_AS_MOVING | OBJECT2_DEBLEND_PEEPHOLE;
	 did_merge = 1;

	 detected[i] |= detected[j];

	 for(c = 0; c < objc->ncolor; c++) {
	    if(merged->color[c] == NULL) {
	       OBJECT1 *obj1 = children[i]->color[c];
	       shAssert(obj1 != NULL);
	       if(obj1->flags & OBJECT1_DETECTED) {
		  shAssert(obj1->flags & OBJECT1_CANONICAL_CENTER);
	       }
	       
	       obj1->colc = col[c] - dcol[c];
	       obj1->colcErr = sqrt(pow(colErr[c],2) + pow(dcolErr[c],2));
	       obj1->rowc = row[c] - drow[c];
	       obj1->rowcErr = sqrt(pow(rowErr[c],2) + pow(drowErr[c],2));
	       
	       obj1->flags |= OBJECT1_CANONICAL_CENTER;
	    } else {
	       if(children[i]->color[c] == merged->color[c]) {
		  ;			/* nothing to do */
	       } else if(children[j]->color[c] == merged->color[c]) {
		  phObject1Del(children[i]->color[c]);
		  children[i]->color[c] = merged->color[c];
		  children[j]->color[c] = NULL;
		  
		  shAssert(children[i]->aimage->mask[c] == NULL);
		  children[i]->aimage->mask[c] = children[j]->aimage->mask[c];
		  children[j]->aimage->mask[c] = NULL;
		  
		  phAtlasImagePixReplace(children[i]->aimage, c, children[j]->aimage, c);
		  phAtlasImagePixReplace(smoothed_ai[i],      c, smoothed_ai[j],      c);
	       } else {
		  shFatal("You cannot get here");
	       }
	       
	       merged->color[c] = NULL;
	    }
	 }
	 /*
	  * and discard j, filling its slot in children[]
	  */
	 phObjcChildDel(children[j]); children[j] = NULL;
	 phAtlasImageDel(smoothed_ai[j], 0);

	 for(; j < nchild - 1; j++) {
	    detected[j] = detected[j + 1];
	    children[j] = children[j + 1];
	    smoothed_ai[j] = smoothed_ai[j + 1];
	 }

	 nchild--;

	 i--;				/* try a further merge */
	 break;				/*       by continuing with i loop */
      }
   }
/*
 * If we merged any children to create a new moving object, see if
 * there are other detections of the moving object that weren't merged
 * into the new object.  In other words, look for detections in other
 * children at the correct calculated position.
 *
 * We need to do this to avoid shredding objects that now appear in
 * two objects 
 */
   if(did_merge) {
      for(i = 0; i < nchild; i++) {
	 if(!(children[i]->flags2 & OBJECT2_DEBLEND_PEEPHOLE)) {
	    continue;
	 }

	 for(j = 0; j < nchild; j++) {
	    int nmatch_j = 0;		/* number of matched objects in j */

	    if(i == j) {
	       continue;
	    }
	    if((detected[i] & detected[j]) != 0) { /* bands in common */
	       continue;
	    }

	    for(c = 0; c < objc->ncolor; c++) {
	       OBJECT1 *obj1_i = children[i]->color[c];
	       OBJECT1 *obj1_j = children[j]->color[c];
	       float rowc_i, rowcErr_i, colc_i, colcErr_i;
	       float rowc_j, rowcErr_j, colc_j, colcErr_j;

	       if((obj1_i->flags & OBJECT1_DETECTED) ||
		  !(obj1_j->flags & OBJECT1_DETECTED)) {
		  continue;
	       }

	       rowc_i = obj1_i->rowc;
	       colc_i = obj1_i->colc;
	       rowcErr_i = obj1_i->rowcErr;
	       colcErr_i = obj1_i->colcErr;

	       rowc_j = obj1_j->rowc;
	       colc_j = obj1_j->colc;
	       rowcErr_j = obj1_j->rowcErr;
	       colcErr_j = obj1_j->colcErr;

	       if(pow(fabs(rowc_i - rowc_j) - rowcErr_i - rowcErr_j, 2) +
		  pow(fabs(colc_i - colc_j) - colcErr_i - colcErr_j, 2) <
					   min_peak_spacing*min_peak_spacing) {
		  obj1_i->flags |= (obj1_j->flags & OBJECT1_DETECTED);
		  nmatch_j++;
	       } else {
		  break;
	       }
	    }

	    if(c == objc->ncolor && nmatch_j > 0) { /* all peaks match */
	       /*
		* and discard j, filling its slot in children[]
		*/
	       phObjcChildDel(children[j]); children[j] = NULL;
	       phAtlasImageDel(smoothed_ai[j], 0);
	       
	       for(; j < nchild - 1; j++) {
		  detected[j] = detected[j + 1];
		  children[j] = children[j + 1];
		  smoothed_ai[j] = smoothed_ai[j + 1];
	       }
	       
	       nchild--;
	       
	       i--;			/* try a further merge */
	       break;			/*       by continuing with i loop */
	    }
	 }
      }
   }
/*
 * Clean up
 */
   if(merged != NULL) {
      for(c = 0; c < objc->ncolor; c++) {
	 merged->color[c] = NULL;	/* merged never owned them */
      }
      phObjcDel(merged, 1);
   }
   shFree(detected);

   return(nchild);
}


/************************************************************************************************************/
/*
 * Calculate the inner product of a pair of columns of two matrices
 */
static double
columnDotColumn(const MAT *Q1, const MAT *Q2, /* the two matrices */
		const int c1, const int c2) /* the two columns */
{
    int i;
    double sum = 0;

    for (i = 0; i < Q1->n; i++) {
	sum += Q1->me[i][c1]*Q2->me[i][c2];
    }

    return sum;
}

/*
 * Swap columns c1 and c2 of matrix Q and the corresponding
 * elements of vector lambda (if non-NULL)
 */
static void
swap_columns(MAT *Q, VEC *lambda,
	     const int c1, const int c2)
{
    int i;

    if (c1 == c2) return;

    for (i = 0; i < Q->n; i++) {
	const double tmp = Q->me[i][c1];
	Q->me[i][c1] = Q->me[i][c2];
	Q->me[i][c2] = tmp;
    }

    if (lambda != NULL) {
	const double tmp = lambda->ve[c1];
	lambda->ve[c1] = lambda->ve[c2];
	lambda->ve[c2] = tmp;
    }
}

/*****************************************************************************/
/*
 * go through a set of children's atlas images, and for pixels where the
 * sum of the children's fluxes doesn't equal the parent, assign the balance
 * based on Is2/R^2
 *
 * Return the total number of DN that this routine was forced to share out
 * among the children
 */
static float
assign_missing_flux(const OBJC **children, /* the children in question */
		    int nchild,		/* number of children */
		    int c,		/* in this band */
		    const float *Is2)	/* children's I*sigma^2 values */
{
   int best_k;				/* best choice of child to get flux */
   const OBJC *child;			/* == children[k] */
   int drow, dcol;			/* distance to an object's centre */
   int i, j, k;				/* span and pixel counter resp. */
   float max;				/* maximum weight of any object */
   const OBJMASK *mmask = children[0]->parent->aimage->master_mask;
   const int nspan = mmask->nspan;
   int r2;				/* == (distance from object centre)^2*/
   float unassigned = 0;		/* flux unassigned to any object */
   int val;				/* value of a pixel to be assigned */
   float weight;			/* an object's weight */
#define USE_WEIGHTS 0
#if USE_WEIGHTS
   float *weights = alloca(nchild*sizeof(float)); /* weights for all children */
#endif
   int x;				/* column counter */
   int y, x1, x2;			/* SPAN.{y,x[12]} */
/*
 * assign fluxes
 */
   for(i = j = 0; i < nspan; i++) {
      y = mmask->s[i].y;
      x1 = mmask->s[i].x1; x2 = mmask->s[i].x2;
      for(x = x1; x <= x2; x++, j++) {
	 for(k = 0; k < nchild; k++) {
	    if(phAtlasImagePixelGet(children[k]->aimage, c, y, x) != SOFT_BIAS) {
	       break;			/* we assigned flux to someone */
	    }
	 }
	 if(k == nchild) {		/* unassigned pixel */
#if USE_WEIGHTS
	    double sum = 0;
	    val = phAtlasImagePixelGet(children[0]->parent->aimage, c, y, x) - SOFT_BIAS;
	    max = -1e9; best_k = 0;
	    for(k = 0; k < nchild; k++) {
	       child = children[k];
	       drow = y - child->rowc + 0.5;
	       dcol = x - child->colc + 0.5;
	       r2 = drow*drow + dcol*dcol;
	       if(r2 == 0) {
		  best_k = k;
		  break;
	       }

	       if(weight > max) {
		  max = weight; best_k = k;
	       }

	       weight = weights[k] = Is2[k]/r2;
	       sum += weight;
	    }
	    if(k < nchild) {
		phAtlasImagePixelSet(children[k]->aimage, c, y, x, val + SOFT_BIAS);
	    } else {
		for(k = 0; k < nchild; k++) {
		    int dval = val*weights[k]/sum + 0.5;
		    if(dval > 0) {
			phAtlasImagePixelSet(children[k]->aimage, c, y, x, dval + SOFT_BIAS);
			val -= dval;
		    }
		}
		phAtlasImagePixelSet(children[best_k]->aimage, c, y, x,
				     phAtlasImagePixelGet(children[best_k]->aimage, c, y, x) + val); /* left over flux*/
	    }
#else
	    val = phAtlasImagePixelGet(children[0]->parent->aimage, c, y, x) - SOFT_BIAS;
	    max = -1e9; best_k = 0;
	    for(k = 0; k < nchild; k++) {
	       child = children[k];
	       drow = y - child->rowc + 0.5;
	       dcol = x - child->colc + 0.5;
	       r2 = drow*drow + dcol*dcol;
	       if(r2 == 0) {
		  best_k = k; break;
	       }

	       weight = Is2[k]/r2;

	       if(weight > max) {
		  max = weight; best_k = k;
	       }
	    }
	    phAtlasImagePixelSet(children[best_k]->aimage, c, y, x, val + SOFT_BIAS);
#endif
	    unassigned += val;
	 }
      }
   }

   return(unassigned);
}

/*****************************************************************************/
/*
 * Find the sum of I_i * r_i^2 for an object in a given band
 *
 * Note that, because the master mask's in the canonical band, that's
 * the coordinate system we use in this routine
 */
static float
find_Isigma2(const OBJC *objc,		/* the object in question */
	     int c,			/* in this band */
	     int bkgd)			/* background level */
{
   const int colc = objc->color[c]->colc - objc->aimage->dcol[c] + 0.5;
   int i;				/* span counter */
   const OBJMASK *mmask = objc->aimage->master_mask;
   const int nspan = mmask->nspan;
   int peakval;				/* peak value in objc */
   int r2;				/* == (distance from object centre)^2*/
   const int rowc = objc->color[c]->rowc - objc->aimage->drow[c] + 0.5;
   int row2, col2;			/* == ({row,col} - {row,col}c)^2 */
   float sum = 0;			/* the desired sum */
   int val;				/* == pix[j] */
   int x, y;				/* column/row counters */
   int x1, x2;				/* ends of this span */

   if(objc->color[c]->peaks == NULL || objc->color[c]->peaks->npeak == 0) {
      return(0.0);
   }
   
   peakval = 1.1*objc->color[c]->peaks->peaks[0]->peak + bkgd;

   for(i = 0; i < nspan; i++) {
      x1 = mmask->s[i].x1; x2 = mmask->s[i].x2;
      y = mmask->s[i].y;

      row2 = y - rowc; row2 *= row2;
      col2 = x1 - colc; col2 *= col2;
      for(x = x1; x <= x2; x++) {
	 r2 = row2 + col2;
	 val = phAtlasImagePixelGet(objc->aimage, c, y, x);
	 if(val > bkgd && val < peakval) {
	    sum += (val - bkgd)*r2;
	 }

	 col2 += 2*(x - colc) + 1;	/* ((x-xc)+1)^2 == (x-xc)^2+2(x-xc)+1*/
      }
   }

   return(sum);
}

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
 * Setup scratch space for the deblender
 */
void
phDeblendSet(REGION *i_scr0,
	     REGION *i_scr1,
	     REGION *i_scr2,
	     REGION *i_scr3)
{
   int i;
   
   scr0 = i_scr0; scr1 = i_scr1; scr2 = i_scr2;

   if(scr0 == NULL) {
      shAssert(scr1 == NULL && scr2 == NULL && i_scr3 == NULL);
      return;
   }

   shAssert(scr0->type == TYPE_PIX);
   shAssert(scr1 != NULL && scr1->type == TYPE_PIX);
   shAssert(scr1->nrow == scr0->nrow && scr1->ncol == scr0->ncol);
   shAssert(scr2 != NULL && scr2->type == TYPE_PIX);
   shAssert(scr2->nrow == scr0->nrow && scr2->ncol == scr0->ncol);
   shAssert(i_scr3 != NULL && i_scr3->type == TYPE_PIX);
   shAssert(i_scr3->nrow == scr0->nrow && i_scr3->ncol == scr0->ncol);
/*
 * we want to make a MASK out of the i_scr3
 */
   shAssert(sizeof(mscr0->rows[0][0]) <= sizeof(PIX));

   mscr0 = shMaskNew("deblender", i_scr3->nrow, 0);
   for(i = 0; i < mscr0->nrow; i++) {
      mscr0->rows[i] = (unsigned char *)i_scr3->ROWS[i];
   }
   mscr0->ncol = i_scr3->ncol;
}

/*****************************************************************************/
/*
 * Note that we don't actually own this scratch space, we merely borrowed
 * it in phDeblendSet
 */
void
phDeblendUnset(void)
{
   scr0 = scr1 = scr2 = NULL;
   mscr0->rows[0] = NULL;		/* memory belongs to i_scr3 */
   shMaskDel(mscr0); mscr0 = NULL;
}
