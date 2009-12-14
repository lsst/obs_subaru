/*
 *   Find and remove cosmic-rays
 *
 * Algorithms designed by Kazu Shimasaku
 *
 * A high-brightness pixel is regarded as a cosmic ray if it
 * satisfies the following conditions:
 *
 * 1. p > sky + min_e/e_per_dn for some pixel in cosmic ray; not applied
 * to every pixel individually 
 *
 * 2. p > (p[2] + p[8])/2 + min_sigma*sigma_sky ||
 *    p > (p[4] + p[6])/2 + min_sigma*sigma_sky ||
 *    p > (p[1] + p[9])/2 + min_sigma*sigma_sky ||
 *    p > (p[3] + p[7])/2 + min_sigma*sigma_sky
 * (If min_sigma is -ve, it is taken to be the actual threshold, so this
 * is replaced by p > |min_sigma*sigma_sky|)
 *
 * 3. The data is outside the band limit. More specifically, no pixel can
 * be higher than its neighbours than the if it's at the highest value of
 * a (centred) PSF. Allowing for noise, the condition is
 *   (p - cond3_fac*N(p) - sky) > PSF(d)*(mean + cond3_fac*N(mean) - sky)
 * where PSF(d) is the value of the PSF at a distance d, mean is the average
 * of two pixels a distance d away, and N(p) is p's standard deviation. In
 * practice, we multiple PSF(d) by some fiddle factor, cond3_fac2
 *
 * This is calculated for the pairs p[2]--p[8] (d==1), p[4]--p[6] (d==1),
 * p[1]--p[9] (d = sqrt(2)), and p[3]--p[7] (d = sqrt(2)), and if any
 * satisfy it, we've got a CR.
 *
 * 4. No neighbouring pixels are saturated
 *
 * Here, the pixel identification is
 *                     [1] [2] [3]
 *                     [4] [5] [6]
 *                     [7] [8] [9],
 * and pixel #5 is the pixel to be judged.
 * p[n] is ADU counts of pixel #n after subtracting the sky value.
 * p \equiv p[5].
 *
 * II. Cosmic-ray remover
 *
 * The original value (DN) of a cosmic-ray contaminated pixel
 * is replaced with the 1-dim weighted mean of the sequential four pixels.
 *
 *                     [1]  [2]  [3]  [4]  [5]
 *                     [6]  [7]  [8]  [9] [10]
 *                    [11] [12] [13] [14] [15]
 *                    [16] [17] [18] [19] [20]
 *                    [21] [22] [23] [24] [25]
 *
 *  Pixel #13 is the pixel to be replaced. The four weighted means are
 *	pred_1 = (3*(p[12]+p[14]) - (p[11]+p[15])/4	E-W
 *	pred_2 = (3*(p[8]+p[18])  - (p[3]+p[23])/4	N-S
 *      pred_3 = (3*(p[7]+p[19])  - (p[1]+p[25])/4	NW-SE
 *      pred_4 = (3*(p[9]+p[17])  - (p[5]+p[21])/4	SW-NE
 * The predicted value for p[13] is the minimum of pred_{1-4}.
 * Pred_* is not used if at least one of the four pixel used for the weighted
 * mean has a mask value which is either mask_flag or mask_type.
 *   If there is no pred_* available, or if the predicted value is less than
 * the sky , p[13] is replaced with the sky value, which is an input parameter.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "dervish.h"
#include "phDgpsf.h"
#include "phMaskbittype.h"
#include "phCosmic.h"
#include "phSpanUtil.h"
#include "phCorrectFrames.h"
#include "phConsts.h"

#define MAXNUM 10000			/* max number of CR pixels permitted */

typedef struct {
   short row, col;			/* position of CR pixel */
   int val;				/* pixel's value */
   int id;				/* ID number for this CR */
} COSMIC_RAY;

static int
condition_3(int peak, int mean_ns, int mean_we, int mean_swne, int mean_nwse,
	    float thres_h, float thres_v, float thres_d, float sky,
	    float e_per_dn, float cond3_fac, int *c);
static float eval_psf(const DGPSF *psf, float dx, float dy);
static void removeCR(REGION *reg, CHAIN *crChain, float bkgd, float skysig,
		     int mask_type, int debias, int grow);

/*****************************************************************************/
/*
 * This is the code to see if a given pixel is bad; it's inlined in
 * phFindCR() for performance reasons. pt_m and/or pt_p can be NULL
 *
 * Note that the pixel in question is at index 0, so its value is pt_0[0]
 */
static int
is_cr_pixel(const PIX *pt_m, const PIX *pt_0, const PIX *pt_p, /* rows */
	    int thres_sky_sigma,	/* min_sigma*sky_sigma, or -threshold
					   if negative */
	    float thres_h, float thres_v, float thres_d, /* for condition #3 */
	    float sky,			/* sky value for region */
	    float bkgd,			/* unsubtracted background level */
	    float e_per_dn,		/* gain of amplifier, e^-/DN */
	    float cond3_fac,		/* fiddle factor for condition #3 */
	    int *corr)			/* corrected value */
{
   int itemp1;
   int mean_ns, mean_we, mean_swne, mean_nwse; /* averages of surrounding
						  eight pixels */
   int val = pt_0[0] - SOFT_BIAS;
/*
 * condition #1 is not applied on a pixel-by-pixel basis
 */
   ;
/*
 * condition #2
 */
   mean_we = pt_0[-1] + pt_0[1] - 2*SOFT_BIAS;
   if(pt_m == NULL || pt_p == NULL) {
      mean_swne = mean_nwse = mean_ns = MAX_U16;
   } else {
      mean_ns = pt_m[0] + pt_p[0] - 2*SOFT_BIAS;
      mean_swne = pt_m[-1] + pt_p[1] - 2*SOFT_BIAS;
      mean_nwse = pt_m[1] + pt_p[-1] - 2*SOFT_BIAS;
   }

   if(thres_sky_sigma < 0) {		/* |thres_sky_sigma| is threshold */
      if(val < -thres_sky_sigma) {
	 return(0);
      }
   } else {
      itemp1 = 2*(val - thres_sky_sigma);
      
      if(itemp1 < mean_ns && itemp1 < mean_we &&
				    itemp1 < mean_swne && itemp1 < mean_nwse) {
	 return(0);
      }
   }
/*
 * condition #3
 *
 * Note that this uses mean_ns etc. even if thres_sky_sigma is negative
 */
   if(!condition_3(val - bkgd,
		   mean_ns/2 - bkgd, mean_we/2 - bkgd,
		   mean_swne/2 - bkgd, mean_nwse/2 - bkgd,
		   thres_h, thres_v, thres_d, sky, e_per_dn, cond3_fac, corr)){
      return(0);
   }
/*
 * OK, it's a contaminated pixel
 */
   *corr += SOFT_BIAS + bkgd;
   
   return(1);
}

/*****************************************************************************/
/*
 * Does the region around (rowc, colc) look like a CR?
 *
 * Used to adjust the thresholds not to trigger on the PSF
 */
static int
is_it_a_cr(const PIX *const *rrows,
	   int rowc, int colc,
	   int hsize,
	   float thres_h,
	   float thres_v,
	   float thres_d,
	   float thresh)		/* threshold for CR pixels */
{
   float bkgd = 0;			/* unsubtracted background */
   float cond3_fac = 0;			/* fiddle factor for condition #3 */
   int corr = 0;			/* corrected pixel value; NOTUSED */
   float e_per_dn = 1;			/* gain of amplifier, e^-/DN */
   int i, j;
   float sky = 0;			/* sky value for region */
   
   for(j = rowc - hsize; j <= rowc + hsize; j++) {
      for(i = colc - hsize; i <= colc + hsize; i++) {
	 if(is_cr_pixel(&rrows[j-1][i], &rrows[j][i], &rrows[j+1][i],
			-thresh, thres_h, thres_v, thres_d,
			sky, bkgd, e_per_dn, cond3_fac, &corr)) {
	    return(1);
	 }
      }
   }

   return(0);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Find cosmic rays in a region, and mask and remove them
 *
 * returns:
 *   number of CRs identified, or -1 if error
 *
 * </AUTO>
 */
int
phFindCR(REGION *reg,			/* region to search */
	 float sky,			/* sky value for region */
	 float bkgd,			/* unsubtracted background of frame,
					   DN (in excess of SOFT_BIAS) */
	 float sky_sigma,		/* rms of sky background */
	 const DGPSF *psf,		/* the PSF */
	 float e_per_dn,		/* gain of amplifier, e^-/DN */
	 float min_sigma,		/* min sigma above sky in pixel
					   for CR candidates */
	 int min_e,			/* min number of e^- in an CRs */
	 float cond3_fac,		/* fiddle factor for condition #3 */
	 float cond3_fac2,		/* 2nd fiddle factor for condition #3*/
	 int keep,			/* if true, don't remove the CRs */
	 const PSF_BASIS *psfBasis,	/* describe real PSF, or NULL */
	 float *adopted_cond3_fac2)	/* the value of cond3_fac2 that doesn't
					   call the PSF a CR; or NULL */

{
   int canonized;			/* did we make any CRs canonical? */
   int corr = 0;			/* corrected value of a pixel */
   CURSOR_T crsr;			/* cursor for crChain */
   int debias_values = 0;		/* debias fixed pixel values? */
   long n;				/* number of cosmic ray pixels found */
   int i, j, k;
   PIX *pt_m, *pt_0, *pt_p;		/* pointers to nine pixels */
   int val;				/* the pixel judged  */
   int mean_ns, mean_we, mean_swne, mean_nwse; /* averages of surrounding
						  eight pixels */
   int thres_sky_sigma;			/* min_sigma*sky_sigma */
   int nextra;				/* extra CR pixels from second pass */
   int nrow, ncol;			/* == reg->nrow, reg->ncol */
   int itemp1;
   float thres_h, thres_v, thres_d;	/* for condition #3 */
   SPANMASK *sm;			/* The region's masks */
   OBJMASK *mask_sv;			/* Spanvecs of merged mask */
   SPAN	*s = NULL;			/* spans of merged mask */
   int mspan;				/* span bookkeeping */
   int npix;				/* == cr->npix */
   int nspan;				/* == cr->nspan */
   int ncr;				/* number of cosmic rays */
   int moffx = 0, moffy = 0;
   int inmasky;
   PIX **rrows;				/* == reg->ROWS */
   CHAIN *crChain;			/* CR mask */
   OBJMASK *cr;
   char *object_type;
   COSMIC_RAY crpixels[MAXNUM + 1];	/* CR contaminated pixels */

/*
 * check input parameters
 */
   if (sky < 0.0) {
      shErrStackPush("phFindCR: sky < 0.0");
      return (-1);
   }
   shAssert(sky_sigma > 0.0);
   shAssert(psf != NULL);
   shAssert(e_per_dn > 0.0);
   shAssert(reg != NULL && reg->type == TYPE_PIX);
   shAssert(reg->mask != NULL);
   sm = (SPANMASK *)(reg->mask);
   shAssert(sm->cookie == SPAN_COOKIE);
   rrows = reg->ROWS;
/*
 * mask values for pixels to ignore, if so desired
 */
#if 0
   {
      CHAIN *mask_chain = phObjmaskChainMerge(phObjmaskChainMerge(
				    shChainCopy(sm->masks[S_MASK_INTERP]),
				    shChainCopy(sm->masks[S_MASK_SATUR])),
				    shChainCopy(sm->masks[S_MASK_NOTCHECKED]));
   
      mask_sv = phMergeObjmaskChain(mask_chain);
      shChainDel(mask_chain);
   }
#else
   mask_sv = NULL;
#endif

   /* threshold for 1st condition */
   thres_sky_sigma = min_sigma*sky_sigma;
/*
 * thresholds for 3rd condition
 */
   thres_h = cond3_fac2*eval_psf(psf,0,1); /* horizontal */
   thres_v = cond3_fac2*eval_psf(psf,1,0); /* vertical */
   thres_d = cond3_fac2*eval_psf(psf,1,1); /* diagonal */
/*
 * Check if the PSF looks like a CR
 */
   if(psfBasis != NULL) {
      const float fiddle_c32 = 0.05;	/* Subtract this from best cond3_fac2*/
      float c32 = cond3_fac2 + fiddle_c32; /* The adopted value of cond3_fac2*/
      float dc32 = 0.01;		/* step in cond3_fac2 */
      const int hsize = 5;		/* half-size of region to search */
      const float rc[] = {0.5, 0.2, 0.2, 0.8, 0.8}; /* places to eval PSF */
      const float cc[] = {0.5, 0.2, 0.8, 0.2, 0.8};

      for(i = 0; i < sizeof(rc)/sizeof(rc[0]); i++) {
	 PSF_REG *psfReg = phPsfKLReconstruct(psfBasis, rc[i]*reg->nrow/2,
					      cc[i]*reg->ncol/2, TYPE_PIX);
	 const REGION *psfRegion = psfReg->reg;
	 const int rowc = psfRegion->nrow/2;
	 const int colc = psfRegion->ncol/2;
	 const PIX *const *rrows = (const PIX *const *)psfRegion->ROWS;
	 const float thresh_DN = 5000;	/* threshold for CR pixels */

	 shAssert(rowc - hsize >= 0 && rowc + hsize < psfRegion->nrow);
	 shAssert(colc - hsize >= 0 && colc + hsize < psfRegion->ncol);

	 if(is_it_a_cr(rrows, rowc, colc, hsize, thres_h, thres_v, thres_d,
		       thresh_DN)) {
	    while(c32 > 0 &&
		  is_it_a_cr(rrows, rowc, colc, hsize, thres_h,thres_v,thres_d,
			     thresh_DN)) {
#if 0
	       fprintf(stderr,"PSF is a CR: %.2f\n", c32);
#endif
	       
	       c32 -= dc32;
	       thres_h = c32*eval_psf(psf,0,1);
	       thres_v = c32*eval_psf(psf,1,0);
	       thres_d = c32*eval_psf(psf,1,1);
	    }
	 }
	 
	 phPsfRegDel(psfReg);
      }
/*
 * Back off a little further for safety; seems to help with e.g. PR 3992
 */
      c32 -= fiddle_c32;
      thres_h = c32*eval_psf(psf,0,1);
      thres_v = c32*eval_psf(psf,1,0);
      thres_d = c32*eval_psf(psf,1,1);
      
      cond3_fac2 = c32;			/* The adopted value of cond3_fac2 */
   }

   if(adopted_cond3_fac2 != NULL) {
      *adopted_cond3_fac2 = cond3_fac2;
   }
/*
 * Time to actually find the CRs lurking in the frame
 */
   nrow = reg->nrow;
   ncol = reg->ncol;
   n = 0;
   mspan = 0;
   if(mask_sv == NULL) {
      nspan = 0;
   } else {
      moffx = mask_sv->col0;
      moffy = mask_sv->row0;
      s = mask_sv->s;
      nspan = mask_sv->nspan;
   }
/*
 * Go through the frame looking at each pixel
 */
   for (j = 0; j < nrow; j++) {
				/* See if there is a mask span we have */
				/* to deal with */
      while(mspan < nspan && s[mspan].y + moffy < j)
	  mspan++;
      if(mspan < nspan && s[mspan].y + moffy == j)
	  inmasky = 1;
      else
	  inmasky = 0;

      pt_m = rrows[j - 1];
      pt_0 = rrows[j];
      pt_p = rrows[j + 1];
/*
 * treat first and last rows specially
 */
      if(j == 0 || j == nrow - 1) {
	 for(i = 1; i < ncol - 1; i++) {
	    if(is_cr_pixel(NULL, &rrows[j][i], NULL,
			   thres_sky_sigma, thres_h, thres_v, thres_d,
			   sky, bkgd, e_per_dn, cond3_fac, &corr)) {
	       if (n < MAXNUM) {
		  crpixels[n].col = i;
		  crpixels[n].row = j;
		  crpixels[n].val = rrows[j][i] - SOFT_BIAS;
		  crpixels[n].id = -1;
		  n++;

		  rrows[j][i] = corr;
	       }
	    }
	 }
	 continue;
      }
/*
 * now the first and last columns. 
 */
      for(i = 0; i < ncol; i += ncol - 1) {
	 PIX scr[3];			/* used to pass pixels to is_cr_pixel*/
	 if(ncol == 1) break;		/* avoid infinite loop */
	 
	 scr[0] = pt_m[i]; scr[1] = pt_0[i]; scr[2] = pt_p[i];
	 if(is_cr_pixel(NULL, &scr[1], NULL,
			thres_sky_sigma, thres_h, thres_v, thres_d,
			sky, bkgd, e_per_dn, cond3_fac, &corr)) {
	    if (n < MAXNUM) {
	       crpixels[n].col = i;
	       crpixels[n].row = j;
	       crpixels[n].val = rrows[j][i] - SOFT_BIAS;
	       crpixels[n].id = -1;
	       n++;
	       
	       rrows[j][i] = corr;
	    }
	 }
      }
/*
 * we've finished with the edge pixels, time to do the interior of the frame
 */
      for(i = 1; i < ncol - 1; i++) {
	 val = pt_0[i] - SOFT_BIAS;
/*
 * condition #1 is applied below, when the CR as a whole has been identified
 */
	 ;
/*
 * This isn't exactly a condition, more a pre-condition, but as it seldom
 * gets triggered it's more efficient to put it after condition #1
 */
	 if(inmasky) {
	     while(s[mspan].y + moffy == j && s[mspan].x2 + moffx < i)
		 mspan++;
	     if(s[mspan].y + moffy == j && s[mspan].x1 + moffx <= i)
		 continue;
	 }
/*
 * condition #2
 */
	 mean_ns = pt_m[i] + pt_p[i] - 2*SOFT_BIAS;
	 mean_we = pt_0[i - 1] + pt_0[i + 1] - 2*SOFT_BIAS;
	 mean_swne = pt_m[i - 1] + pt_p[i + 1] - 2*SOFT_BIAS;
	 mean_nwse = pt_m[i + 1] + pt_p[i - 1] - 2*SOFT_BIAS;
	 itemp1 = 2*(val - thres_sky_sigma);
	 
	 if(itemp1 < mean_ns && itemp1 < mean_we &&
				  itemp1 < mean_swne && itemp1 < mean_nwse) {
	    continue;
	 }
/*
 * condition #3
 */
	 if(!condition_3(val - bkgd, mean_ns/2 - bkgd, mean_we/2 - bkgd,
			 mean_swne/2 - bkgd, mean_nwse/2 - bkgd,
		  	 thres_h, thres_v, thres_d, sky, e_per_dn, cond3_fac,
								      &corr)) {
	    continue;
	 }
	 corr += SOFT_BIAS + bkgd;
/*
 * condition #4
 */
	 if(phPixIntersectMask(sm->masks[S_MASK_SATUR], i, j)) {
	    continue;
	 }
/*
 * OK, it's a CR
 *
 * replace CR-contaminated pixels with reasonable values as we go through
 * image, which increases the detection rate
 */
	 if (n < MAXNUM) {
	    pt_0[i] = corr;		/* just a preliminary estimate */
	    
	    crpixels[n].col = i;
	    crpixels[n].row = j;
	    crpixels[n].val = val;
	    crpixels[n].id = -1;
	    n++;
	 }
      }
   }
   phObjmaskDel(mask_sv);
/*
 * We've found them on a pixel-by-pixel basis, now merge those pixels
 * into cosmic rays
 */
   ncr = 0;
   if(n > 0) {
      int id;				/* id number for a CR */
      
      crpixels[n].row = -1;			/* impossible value */
      for(i = 0;i < n;i++) {
	 if(crpixels[i].id < 0) {		/* not already assigned */
	    crpixels[i].id = ++ncr;		/* a new CR */
	 }
	 id = crpixels[i].id;

	 if(crpixels[i + 1].row == crpixels[i].row &&
	    crpixels[i + 1].col == crpixels[i].col + 1) {
	    crpixels[i + 1].id = id;
	 }
/*
 * look for connected pixels in the next row; we have to skip those in
 * this row first
 */
	 for(j = i + 1;crpixels[j].row == crpixels[i].row;j++) continue;

	 for(;crpixels[j].row == crpixels[i].row + 1; j++) {
	    if(crpixels[j].col >= crpixels[i].col - 1) {
	       if(crpixels[j].col > crpixels[i].col + 1) {
		  break;
	       }
	       crpixels[j].id = id;	 
	    }
	 }
      }
   }
/*
 * assemble those pixels, now identified with ID numbers, into OBJMASKs
 */
   object_type = (char *)shTypeGetFromName("OBJMASK");
   crChain = sm->masks[S_MASK_CR];
   for(i = 1;i <= ncr;i++) {
      int max = -1;			/* maximum pixel in CR */
      int nspan = 0;
      int r;
      float sum;			/* total number of electrons in CR */

      cr = NULL;
      for(j = 0;j < n;j++) {
	 if(crpixels[j].val < 0) {
	    crpixels[j].val = 0;
	 }
	 
	 if(crpixels[j].id == i) {
	    if(cr == NULL) {
	       cr = phObjmaskNew(10);
	       max = crpixels[j].val;
	       sum = 0;
	    } else if(nspan >= cr->size) {
	       phObjmaskRealloc(cr, 2*cr->size);
	    }
	    r = cr->s[nspan].y = crpixels[j].row;
	    cr->s[nspan].x1 = crpixels[j].col;

	    do {
	       sum += crpixels[j].val - bkgd;
	       if(crpixels[j].val > max) {
		  max = crpixels[j].val;
	       }

	       j++;
	    } while (crpixels[j].id == i && crpixels[j].row == r &&
		     crpixels[j].col == crpixels[j - 1].col + 1);

	    j--;
	    cr->s[nspan].x2 = crpixels[j].col;
	    nspan++;
	 }
      }
      shAssert(max >= 0);
      cr->nspan = nspan;
      max -= bkgd;
/*
 * apply condition #1
 */
      if(max < min_e/e_per_dn) {	/* not bright enough */
	 phObjmaskDel(cr);
	 for(j = 0;j < n;j++) {
	    if(crpixels[j].id == i) {
	       rrows[crpixels[j].row][crpixels[j].col] =
						   crpixels[j].val + SOFT_BIAS;
	    }
	 }
      } else {
	 phObjmaskBBSet(cr);
	 shChainElementAddByPos(crChain, cr, object_type, TAIL, AFTER);
      }
   }
   ncr = crChain->nElements;		/* some may have been too faint */
/*
 * mark those pixels as CRs
 */
   phCanonizeObjmaskChain(crChain, 1, 1);
/*
 * We've found them all, time to kill them all
 */
   removeCR(reg, crChain, bkgd, sky_sigma, S_MASK_CR, debias_values, 0);
/*
 * Now that we've removed them, go through image again, examining area around
 * each CR for extra bad pixels. Note that we set cond3_fac = 0 for this pass
 */
   canonized = 0;			/* did we make any CRs canonical? */
   nextra = 0;				/* extra CR pixels */
   crsr = shChainCursorNew(crChain);
   while((cr = shChainWalk(crChain, crsr, NEXT)) != NULL) {
/*
 * Are all those `CR' pixels interpolated?  If so, don't grow it
 */
      OBJMASK *om = phObjmaskIntersection(cr, sm->masks[S_MASK_INTERP]);
      npix = (om == NULL) ? 0 : om->npix;
      phObjmaskDel(om);

      if(npix == cr->npix) {
	 continue;
      }
/*
 * No; some of the suspect pixels aren't interpolated
 */
      npix = cr->npix; nspan = cr->nspan;
      for(k = 0;k < nspan;k++) {
	 int col1 = cr->s[k].x1, col2 = cr->s[k].x2;
/*
 * first the previous line, adjacent to span
 */
	 if(cr->s[k].y > 1) {
	    j = cr->s[k].y - 1;
	    for(i = col1 - 1; i <= col2 + 1; i++) {
	       if(i < 0 || i >= ncol - 1) {
		  continue;		/* too close to edge */
	       }
	       
	       if(is_cr_pixel(&rrows[j-1][i], &rrows[j][i], &rrows[j+1][i],
			      -(sky + thres_sky_sigma/2), bkgd,
			      thres_h, thres_v, thres_d,
			      sky, e_per_dn, 0, &corr)) {
		  nextra++;
		  if(keep) {
		     if (n < MAXNUM) {
			crpixels[n].col = i;
			crpixels[n].row = j;
			crpixels[n].val = rrows[j][i] - SOFT_BIAS;
			crpixels[n].id = -1;
			n++;
		     }
		  }

		  rrows[j][i] = corr;
		  
		  if(cr->nspan >= cr->size) {
		     phObjmaskRealloc(cr, 2*cr->size);
		  }
		  cr->s[cr->nspan].y = j;
		  cr->s[cr->nspan].x1 = cr->s[cr->nspan].x2 = i;
		  cr->nspan++; cr->npix++;
	       }
	    }
	 }
/*
 * then the adjacent pixels in the line
 */
	 j = cr->s[k].y;
	 for(i = col1 - 1; i <= col2 + 1; i++) {
	    if(i < 0 || i >= ncol - 1) {
	       continue;		/* too close to edge */
	    }

	    pt_m = (j == 0) ? NULL : &rrows[j-1][i];
	    pt_p = (j == nrow - 1) ? NULL : &rrows[j+1][i];
	    if(is_cr_pixel(pt_m, &rrows[j][i], pt_p,
			   -(sky + thres_sky_sigma/2), bkgd,
			   thres_h, thres_v, thres_d,
			   sky, e_per_dn, 0, &corr)) {
	       nextra++;
	       if(keep) {
		  if (n < MAXNUM) {
		     crpixels[n].col = i;
		     crpixels[n].row = j;
		     crpixels[n].val = rrows[j][i] - SOFT_BIAS;
		     crpixels[n].id = -1;
		     n++;
		  }
	       }

	       rrows[j][i] = corr;

	       if(i == col1 - 1) {
		  cr->s[k].x1--;
		  cr->npix++;
	       } else if(i == col2 + 1) {
		  cr->s[k].x2++;
		  cr->npix++;
	       }
	    }
	 }
/*
 * and then the next line, adjacent to span
 */
	 if(cr->s[k].y < nrow - 2) {
	    j = cr->s[k].y + 1;
	    for(i = col1 - 1; i <= col2 + 1; i++) {
	       if(i < 0 || i >= ncol - 1) {
		  continue;		/* too close to edge */
	       }
	       
	       if(is_cr_pixel(&rrows[j-1][i], &rrows[j][i], &rrows[j+1][i],
			      -(sky + thres_sky_sigma/2), bkgd,
			      thres_h, thres_v, thres_d,
			      sky, e_per_dn, 0, &corr)) {
		  nextra++;
		  if(keep) {
		     if (n < MAXNUM) {
			crpixels[n].col = i;
			crpixels[n].row = j;
			crpixels[n].val = rrows[j][i] - SOFT_BIAS;
			crpixels[n].id = -1;
			n++;
		     }
		  }

		  rrows[j][i] = corr;

		  if(cr->nspan >= cr->size) {
		     phObjmaskRealloc(cr, 2*cr->size);
		  }
		  cr->s[cr->nspan].y = j;
		  cr->s[cr->nspan].x1 = cr->s[cr->nspan].x2 = i;
		  cr->nspan++; cr->npix++;
	       }
	    }
	 }
      }
      if(cr->npix != npix) {		/* added some more pixels */
	 canonized = 1;
	 phCanonizeObjmask(cr, 1);
      } else {
	 phObjmaskBBSet(cr);
      }
   }
   shChainCursorDel(crChain, crsr);

   if(canonized) {
      phCanonizeObjmaskChain(crChain, 1, 1);
   }
/*
 * Reinstate initial values; n.b. the same pixel may appear twice
 *
 * We have to do this as we may decide _not_ to remove certain CRs,
 * for example those which lie next to saturated pixels
 */
   {
      int r,c,val;
      for(i = n - 1;i >= 0;i--) {	/* initial values are at beginning */
	 r = crpixels[i].row; c = crpixels[i].col; val = crpixels[i].val;
	 rrows[r][c] = val + SOFT_BIAS;
      }
   }

   if(!keep) {
      if(nextra > 0) {
	 removeCR(reg, crChain, bkgd, sky_sigma, S_MASK_CR, debias_values, 1);
      }
/*
 * we interpolated over all CR pixels, so set the interp bits too
 */
      sm->masks[S_MASK_INTERP] = phObjmaskChainMerge(sm->masks[S_MASK_INTERP],
					    phObjmaskChainCopy(crChain, 0, 0));
   }

   if (n >= MAXNUM) {
      shError("phFindCR: "
	      "Too many contaminated pixels: at least some have been missed");
   }
	 
   return(ncr);
}

/*****************************************************************************/
/*
 * Find the value of the psf a distance delta from the central pixel,
 * in units of the central amplitude
 */
static float
eval_psf(const DGPSF *psf,
	 float dx,			/* the ROW */
	 float dy)			/* the COLUMN */
{
   const float r2 = dx*dx + dy*dy;
   float psf1 = exp(-0.5*r2/pow(psf->sigma1_2G,2)); 
   float psf2 = exp(-0.5*r2/pow(psf->sigma2_2G,2)); 

   return((psf1 + psf->b_2G*psf2)/(1 + psf->b_2G));
}

/*****************************************************************************/
/*
 * Is condition 3 true?
 */
static int
condition_3(int peak,			/* counts in central pixel (no sky) */
	    int mean_ns,		/* mean in NS direction (no sky) */
	    int mean_we,		/*  "   "  WE    "  "     "   "  */
	    int mean_swne,		/*  "   "  SW-NE "  "     "   "  */
	    int mean_nwse,		/*  "   "  NW-SE "  "     "   "  */
	    float thres_h,		/* horizontal threshold */
	    float thres_v,		/* vertical threshold */
	    float thres_d,		/* diagonal threshold */
	    float sky,			/* sky (which has been subtracted) */
	    float e_per_dn,		/* gain, e^_/DN */
	    float cond3_fac,		/* fiddle factor for noise */
	    int *estimate)		/* estimate of true value of pixel */
{
   float noise_peak;			/* noise in the peak */
   float noise_mean;			/* noise in the mean */

   noise_peak = sqrt((peak + sky)/e_per_dn);
   
   noise_mean = sqrt(((mean_ns + sky)/e_per_dn)/2);
   if(thres_v*(peak - cond3_fac*noise_peak) > mean_ns + cond3_fac*noise_mean) {
      *estimate = mean_ns;
      return(1);
   }

   noise_mean = sqrt(((mean_we + sky)/e_per_dn)/2);
   if(thres_h*(peak - cond3_fac*noise_peak) > mean_we + cond3_fac*noise_mean) {
      *estimate = mean_we;
      return(1);
   }

   noise_mean = sqrt(((mean_swne + sky)/e_per_dn)/2);
   if(thres_d*(peak - cond3_fac*noise_peak) >
					    mean_swne + cond3_fac*noise_mean) {
      *estimate = mean_swne;
      return(1);
   }

   noise_mean = sqrt(((mean_nwse + sky)/e_per_dn)/2);
   if(thres_d*(peak - cond3_fac*noise_peak) >
					    mean_nwse + cond3_fac*noise_mean) {
      *estimate = mean_nwse;
      return(1);
   }

   return (0);
}

/*****************************************************************************/
/*
 * actually remove CRs from the frame
 */
static void
removeCR(REGION *reg,			/* region to search */
	 CHAIN *crChain,		/* chain of CRs */
	 float bkgd,			/* non-subtracted background */
	 float skysig,			/* s.d. of the sky */
	 int mask_type,			/* mask type */
	 int debias,			/* statistically debias values? */
	 int grow)			/* Grow CRs? */
{
   OBJMASK *cr;				/* a CR's mask */
   CHAIN *satur_crChain = NULL;		/* CRs that touch saturated pixels */
   int row, col;			/* row and column counters */
   int col1, col2;			/* range of columns in a span */
   int i, j;
   CHAIN *badmask;			/* masks of pixels to ignore */
   int min;				/* minimum of estimates for replacement
					   value of a pixel under a CR */
   float minval = bkgd - 2*skysig + SOFT_BIAS; /* minimum acceptable pixel
						     value after interp. XXX */
   int nrow, ncol;			/* reg->nrow, reg->ncol */
   int ngood;				/* number of good values on min */
   PIX **rrows;				/* == reg->ROWS */
   int tmp;
   SPANMASK *sm;			/* The region's masks */

   shAssert(reg != NULL);
   shAssert(reg->type == TYPE_PIX);
   shAssert(reg->mask != NULL);
   shAssert(mask_type < NMASK_TYPES);

   min = ngood = 0;			/* placate gcc */

   sm = (SPANMASK *)(reg->mask);	/* XXX Change this with new REGION */
   shAssert(sm->cookie == SPAN_COOKIE);
   /*
    * Mask values to ignore
    */
   badmask = phObjmaskChainMerge(phObjmaskChainMerge(
				 shChainCopy(sm->masks[S_MASK_INTERP]),
				 shChainCopy(sm->masks[S_MASK_NOTCHECKED])),
				 shChainCopy(sm->masks[mask_type]));
   badmask = phObjmaskChainMerge(badmask, shChainCopy(crChain));

   nrow = reg->nrow;
   ncol = reg->ncol;
   rrows = reg->ROWS;
   /*
    * replace the values of cosmic-ray contaminated pixels with 1-dim
    * 2nd-order weighted means Cosmic-ray contaminated pixels have been
    * given a mask value, mask_type, by phFindCR().
    *
    * If there are no good options (i.e. all estimates are contaminated),
    * try using just pixels that are not CRs; failing that, interpolate
    * in the row- or column direction over as large a distance as is
    * required
    */
   for(j = shChainSize(crChain) - 1; j >= 0; j--) {
      cr = shChainElementGetByPos(crChain, j);
/*
 * If I grow this CR does it touch saturated pixels?  If so, don't
 * interpolate and add CR pixels to saturated chain
 */
      if(grow) {
	 OBJMASK *cr0 = phObjmaskResetCorner(phObjmaskCopy(cr, 0, 0),
					     reg->row0, reg->col0);
	 OBJMASK *gcr = phObjmaskGrow(cr0, reg, 1);
	 phObjmaskDel(cr0);
	 if(phObjmaskIntersectMask(sm->masks[S_MASK_SATUR], gcr)) {
	    if(satur_crChain == NULL) {
	       satur_crChain = shChainNew("OBJMASK");
	    }
  
	    shChainElementAddByPos(satur_crChain, gcr, "OBJMASK", TAIL, AFTER);

	    continue;
	 }

	 phObjmaskDel(gcr);
      }
/*
 * OK, fix it
 */
      for(i = 0;i < cr->nspan;i++) {
	 row = cr->s[i].y;
	 col1 = cr->s[i].x1;
	 col2 = cr->s[i].x2;
	 for(col = col1; col <= col2; col++) {
	    min = 123456;			/* some impossible value */
	    ngood = 0;
/*
 * W-E row
 */
	    if(col - 2 >= 0 && col + 2 < ncol) {
	       if(phRectIntersectMask(badmask, col-2, row, col-1, row) ||
		  phRectIntersectMask(badmask, col+1, row, col+2, row)) {
		  ;			/* estimate is contaminated */
	       } else {
		  tmp = FLT2PIX(INTERP_1_C1*(rrows[row][col - 1] +
					     rrows[row][col + 1]) +
				INTERP_1_C2*(rrows[row][col - 2] +
					     rrows[row][col + 2]));
		  if(tmp > minval && tmp < min) {
		     min = tmp;
		     ngood++;
		  }
	       }
	    }
/*
 * N-S column
 */
	    if(row - 2 >= 0 && row + 2 < nrow) {
	       if(phRectIntersectMask(badmask, col, row-2, col, row-1) ||
		  phRectIntersectMask(badmask, col, row+1, col, row+2)) {
		  ;			/* estimate is contaminated */
	       } else {
		  tmp = FLT2PIX(INTERP_1_C1*(rrows[row - 1][col] +
					     rrows[row + 1][col]) +
				INTERP_1_C2*(rrows[row - 2][col] +
					     rrows[row + 2][col]));
		  if(tmp > minval && tmp < min) {
		     min = tmp;
		     ngood++;
		  }
	       }
	    }
/*
 * SW--NE diagonal
 */
	    if(col - 2 >= 0 && col + 2 < ncol &&
	       row - 2 >= 0 && row + 2 < nrow) {
	       if(phPixIntersectMask(badmask, col-2, row-2) ||
		  phPixIntersectMask(badmask, col-1, row-1) ||
		  phPixIntersectMask(badmask, col+1, row+1) ||
		  phPixIntersectMask(badmask, col+2, row+2)) {
		  ;			/* estimate is contaminated */
	       } else {
		  tmp = FLT2PIX(INTERP_IS2_C1*(rrows[row - 1][col - 1] +
					       rrows[row + 1][col + 1]) +
				INTERP_IS2_C2*(rrows[row - 2][col - 2] +
					       rrows[row + 2][col + 2]));
		  if(tmp > minval && tmp < min) {
		     min = tmp;
		     ngood++;
		  }
	       }
	    }
/*
 * SE--NW diagonal
 */
	    if(col - 2 >= 0 && col + 2 < ncol &&
	       row - 2 >= 0 && row + 2 < nrow) {
	       if(phPixIntersectMask(badmask, col+2, row-2) ||
		  phPixIntersectMask(badmask, col+1, row-1) ||
		  phPixIntersectMask(badmask, col-1, row+1) ||
		  phPixIntersectMask(badmask, col-2, row+2)) {
		  ;			/* estimate is contaminated */
	       } else {
		  tmp = FLT2PIX(INTERP_IS2_C1*(rrows[row - 1][col + 1] +
					       rrows[row + 1][col - 1]) +
				INTERP_IS2_C2*(rrows[row - 2][col + 2] +
					       rrows[row + 2][col - 2]));
		  if(tmp > minval && tmp < min) {
		     min = tmp;
		     ngood++;
		  }
	       }
	    }
/*
 * Have we altogether failed to find an acceptable value? If so interpolate
 * using the full-up interpolation code both vertically and horizontally
 * and take the average. This can fail for large enough defects (e.g. CRs
 * lying in bad columns), in which case the interpolator returns -1. If
 * both directions fail, use the background value.
 */
	    if(ngood == 0) {
	       int val_h = phPixelInterpolate(row, col, reg, badmask,
								    1, minval);
	       int val_v = phPixelInterpolate(row, col, reg, badmask,
								    0, minval);
	       
	       if(val_h < 0) {
		  if(val_v < 0) {	/* Still no good value. Guess wildly */
		     min = SOFT_BIAS + bkgd + skysig*phGaussdev();
		  } else {
		     min = val_v;
		  }
	       } else {
		  if(val_v < 0) {
		     min = val_h;
		  } else {
		     min = (val_v + val_h + 1)/2;
		  }
	       }
	    }
/*
 * debias the minimum; If more than one uncontaminated estimate was
 * available, estimate the bias to be simply that due to choosing the
 * minimum of two Gaussians. In fact, even some of the "good" pixels
 * may have some extra charge, so even if ngood > 2, still use this
 * estimate
 */
	    if(debias && ngood > 1) {
	       min -= MIN_2N_BIAS*skysig;
	    }
	    rrows[row][col] = min;
	 }
      }
   }
/*
 * Add those CRs touching saturated pixels to the SATUR mask. We didn't
 * simply merge the pixels as I didn't want to deal with the case that
 * a CR merges two saturated OBJMASKs together.
 */
   if(satur_crChain != NULL) {
      phCanonizeObjmaskChain(satur_crChain, 0, 0);
      
      sm->masks[S_MASK_SATUR] =
	phObjmaskChainMerge(satur_crChain, sm->masks[S_MASK_SATUR]);
   }
/*
 * clean up
 */
   shChainDel(badmask);
}
