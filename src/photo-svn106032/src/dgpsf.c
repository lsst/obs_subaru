/*
  * <AUTO>
  * FILE: dgpsf.c
  *
  * creation/deletion for DGPSF (Double-Gaussian PSF) structure 
  *
  * </AUTO>
  *
  * <HTML>
  * A DGPSF holds information about a PSF which is represented as
  * a sum of two Gaussians, each separable in X and Y, plus a power
  * law part, like so:
  *
  *       func = a*[exp(-x^2/(2*sigmax1^2) - y^2/(2*sigmay1^2)) + 
  *	              b*exp(-x^2/(2*sigmax2^2) - y^2/(2*sigmay2^2)) +
  *		      p0*(1 + r^2/(beta*sigmap^2))^{-beta/2}]
  *
  * N.b. y is the ROW direction, and x the COLUMN
  *
  * N.b. the sigma values are _after_ convolution with the pixel response,
  * so sigma^2 is greater than the PSF by 1/12 (all measured in pixels);
  * that is, the functional form described above should NOT be integrated
  * over a pixel, but merely evaluated at its centre
  * </HTML>
  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dervish.h"
#include "phDgpsf.h"
#include "phUtils.h"
#include "phRandom.h"
#include "phConsts.h"

/*****************************************************************************/
/*
 * Clear all fields in a PSF_COEFFS
 */
static void
clear_psf_coeffs(PSF_COEFFS *coeffs)
{
   memset(coeffs, '\0', sizeof(PSF_COEFFS));
   
   coeffs->L = -1;
   
   coeffs->residuals.ncell = -1;
   coeffs->residuals.is_median = -1;
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * create a new DGPSF structure.
 * 
 * returns:
 *   pointer to new DGPSF
 */
DGPSF *
phDgpsfNew(void) 
{
   DGPSF *new;

   new = shMalloc(sizeof(DGPSF));
   new->a = 1.0;
   new->width = new->sigmax1 = new->sigmay1 = 0;
   new->sigmax2 = new->sigmay2 = 1e5;
   new->sigmap = 1.0;
   new->beta = 3.0;
   new->p0 = new->b = 0.0;
   new->sigma1_2G = new->sigma2_2G = new->b_2G = 0.0;
   new->chisq = -1.0;
   new->psfCounts = -1e10;

   clear_psf_coeffs(&new->coeffs);

   return(new);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * delete a DGPSF structure.
 *
 */
void
phDgpsfDel(DGPSF *dgpsf)
{
   if(dgpsf == NULL) return;

   shFree(dgpsf);
}


/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * copy the content of dgpsf2 to dgpsf1  DGPSF structure.
 *
 */
void
phDgpsfCopy(DGPSF *dgpsf1,   /* target dgpsf */  
            DGPSF *dgpsf2)   /* source dgpsf */
{
   shAssert(dgpsf1 != NULL);
   shAssert(dgpsf2 != NULL);
   /* copy the content */
   *dgpsf1 = *dgpsf2;
}



/****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Given an DGPSF structure with already-determined parameters, a
 * REGION, an outer radius defining the area of effect, and a centroid
 * and peak value, create an artificial star at the given centroid and
 * with the given amplitude and add it to the REGION.
 */
void
phDgpsfAdd(REGION *region,		/* add to this region */
	   const DGPSF *dgpsf,		/* using this PSF */
	   float outer,			/* maximum size of added psf */
           float rowc,			/* desired row centre */
	   float colc,			/* desired column centre */
	   float I0)			/* peak value of PSF */
{
   float amp;				/* amplitude of PSF */
   int i, j, wings;
   float aval;                          /* value of pixel */
   float b;				/* == dgpsf->b */
   float ivarx1, ivarx2, ivary1, ivary2; /* == 1/(dgpsf->sigmax1^2) etc. */
   float ivarp, r2, p0, beta, sigmap;    /* power law part */
   float xdist, xdist2, ydist, ydist2;	/* (x - colc), (x - colc)^2, etc. */
   float outer2;			/* outer^2 */
   REGION *reg;				/* the region to put the PSF in */
   PIX *row;				/* == reg->row[] */

   shAssert(dgpsf != NULL);
   shAssert(region != NULL && region->type == TYPE_PIX);

   if(outer > 0) {
      int r0, c0;			/* origin of subregion */
      int r1, c1;			/* upper right corner of subregion */
      int h;				/* rounded value of outer radius */
      /*
       * first, we get a SubRegion around the centroid which is large enough 
       * to hold the PSF
       */
      h = (int)(outer + 0.5);
      c0 = ((int)colc - region->col0) - h; c1 = c0 + 2*h + 1;
      r0 = ((int)rowc - region->row0) - h; r1 = r0 + 2*h + 1;

      if(c0 < 0) c0 = 0;
      if(c0 >= region->ncol) c0 = region->ncol - 1;
      if(c1 < 0) c1 = 0;
      if(c1 >= region->ncol) c1 = region->ncol - 1;
      if(r0 < 0) r0 = 0;
      if(r0 >= region->nrow) r0 = region->nrow - 1;
      if(r1 < 0) r1 = 0;
      if(r1 >= region->nrow) r1 = region->nrow - 1;

      shAssert(r1 - r0 >= 0 && c1 - c0 >= 0);
      reg = shSubRegNew("phDgpsfAdd", region,
			r1 - r0 + 1, c1 - c0 + 1, r0, c0, NO_FLAGS);
      shAssert(reg != NULL);
      rowc -= r0;
      colc -= c0;
   } else {
      reg = region;
      outer = reg->nrow + reg->ncol;	/* i.e. larger than region */
   }

   rowc -= region->row0;
   colc -= region->col0;
/*
 * unpack dgpsf
 */
   ivarx1 = 1/pow(dgpsf->sigmax1,2);
   ivarx2 = 1/pow(dgpsf->sigmax2,2);
   ivary1 = 1/pow(dgpsf->sigmay1,2);
   ivary2 = 1/pow(dgpsf->sigmay2,2);
   b = dgpsf->b;
   sigmap = dgpsf->sigmap;
   beta = dgpsf->beta;
   p0 = dgpsf->p0;
   if (sigmap != 0 && beta != 0 && p0 != 0) {
       ivarp = 1/(pow(sigmap,2)*beta);
       wings = 1;
   } else {
       ivarp = wings = 0;
   }
/*
 * Find the "amplitude" that will give the desired value of the central
 * intensity (at the centre of the profile, not the maximum pixel value)
 */
   amp = I0/(1 + b + (wings ? p0 : 0));
/*
 * and add the PSF to the region
 */
   rowc -= 0.5; colc -= 0.5;		/* centre of pixel's (0.5, 0.5) */
   outer2 = outer*outer;
   for (i = 0; i < reg->nrow; i++) {
      ydist = i - rowc;
      ydist2 = ydist*ydist;
      if (ydist2 > outer2) {
	 continue;
      }

      row = reg->ROWS[i];
      for (j = 0; j < reg->ncol; j++) {
	 xdist = j - colc;
	 xdist2 = xdist*xdist;
	 if(ydist2 + xdist2 > outer2) {
	    continue;
	 }

         if (wings) {
	     r2 = xdist2 + ydist2;
             aval = p0*pow((1 + r2*ivarp),-beta/2); /* power law */
         } else {
             aval = 0.0;
         }

	 aval = FLT2PIX(amp*(exp(-0.5*(xdist2*ivarx1 + ydist2*ivary1)) +
			     b*exp(-0.5*(xdist2*ivarx2 + ydist2*ivary2)) +
									aval));
	 aval += row[j];
#if !FLOATING_PHOTO			/* need to dither */
	 aval += phRandomUniformdev() - 0.5; /* 0.5 was already added by FLT2PIX */
#endif
	 row[j] = (aval >= MAX_U16) ? MAX_U16 : ((aval <= 0) ? 0 : aval);
      }
   }
/*
 * clean up
 */
   if(reg != region) {
      shRegDel(reg);
   }
}

/*****************************************************************************/
/*
 * Like phDgpsfAdd(), but only modify pixels within the OBJMASK.  If the
 * offset OBJMASK overlaps the edge of the frame, don't subtract anything
 *
 * This implementation is v. simple, and could be sped up if necessary
 */
void
phDgpsfAddInObjmask(REGION *region,	/* add to this region */
		    const OBJMASK *om,	/* within this mask */
		    int drow, int dcol,	/* offset mask by this much */
		    const DGPSF *dgpsf,	/* using this PSF */
		    float outer,	/* maximum size of added psf */
		    float rowc,		/* desired row centre */
		    float colc,		/* desired column centre */
		    float I0)		/* peak value of PSF */
{
   int nrow, ncol;			/* size of om's bounding box */
   int row0, col0;			/* origin of om after drow/col offset*/
   REGION *scr;				/* scratch region */
   const int subtract = (I0 < 0) ? 1 : 0; /* Subtract PSF? */

   shAssert(om != NULL);
/*
 * Make a scratch region as big as om's bounding box
 */
   row0 = om->rmin;
   col0 = om->cmin;
   nrow = om->rmax - om->rmin + 1;
   ncol = om->cmax - om->cmin + 1;
   scr = shRegNew("", nrow, ncol, TYPE_PIX);
/*
 * The mask should be offset by (drow, dcol);  we do that by offsetting
 * the region and centre by (-drow, -dcol)
 */
   scr->row0 = row0 - drow; scr->col0 = col0 - dcol;
   rowc -= drow; colc -= dcol;
/*
 * Insert the PSF into the scratch region
 */
   if(subtract) {
      I0 = -I0;
   }
   shRegClear(scr);
   phDgpsfAdd(scr, dgpsf, outer, rowc, colc, I0);
/*
 * Copy the PSF into a second region, and clear the pixels withing the objmask.
 * Then subtract the result from scr --- and Heh Presto we've set the PSF only
 * in the mask
 */
   {
      REGION *scr2 = shRegNew("", nrow, ncol, TYPE_PIX);
      shRegIntCopy(scr2, scr);
      scr2->row0 = scr->row0; scr2->col0 = scr->col0;
      
      phRegionSetValFromObjmask(scr2, om, 0);
      shRegIntLincom(scr, scr2, 0, 1, -1, LINCOM_EXACT);
      shRegDel(scr2);
   }
/*
 * Add/subtract the masked PSF into/from the real region
 */
   {
      REGION *sreg =
	shSubRegNew("", region, nrow, ncol,
		    row0 - region->row0, col0 - region->col0, (REGION_FLAGS)0);
      if(sreg != NULL) {
	 shRegIntLincom(sreg, scr, 0, 1, (subtract ? -1 : 1), 0);
	 shRegDel(sreg);
      }
   }
/*
 * and clear up
 */
   shRegDel(scr);
}

/****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Given an DGPSF structure with already-determined parameters, return the
 * radius at which it falls to the specified value
 */
float
phDgpsfSize(const DGPSF *dgpsf,		/* using this PSF */
	    float I0,			/* peak value of PSF */
	    int level)			/* desired level */
{
   float amp;				/* amplitude of PSF */
   int i;
   float aval;                          /* value of pixel */
   float b;				/* == dgpsf->b */
   float ivar1, ivar2;			/* == 2/(sigmaz^2 + sigmayz^2) */
   float ivarp, p0, beta, sigmap;	/* power law part */
   float r, r2;				/* (distance from centre)^[12] */
   const float rmax = 1e2;		/* maximum permitted value of r */
   int val;				/* value of pixel */
   int wings;				/* are we including the wings? */

   shAssert(dgpsf != NULL);
   shAssert(I0 == I0 && level >= 0);	/* i.e. !NaN */
/*
 * unpack dgpsf
 */
   ivar1 = 2/(pow(dgpsf->sigmax1,2) + pow(dgpsf->sigmay1,2));
   ivar2 = 2/(pow(dgpsf->sigmax2,2) + pow(dgpsf->sigmay2,2));
   b = dgpsf->b;
   sigmap = dgpsf->sigmap;
   beta = dgpsf->beta;
   p0 = dgpsf->p0;
   if (sigmap != 0 && beta != 0 && p0 != 0) {
       ivarp = 1/(pow(sigmap,2)*beta);
       wings = 1;
   } else {
       ivarp = wings = 0;
   }
/*
 * Find the "amplitude" that will give the desired value of the central
 * intensity (at the centre of the profile, not the maximum pixel value)
 */
   amp = I0/(1 + b + (wings ? p0 : 0));
/*
 * Work out from centre, waiting for psf to fall below level
 */
   for (i = 0;; i++) {
      r2 = (i + 0.5)*(i + 0.5);

      if (wings) {
	 aval = p0*pow((1 + r2*ivarp),-beta/2); /* power law */
      } else {
	 aval = 0.0;
      }
      
      val = FLT2PIX(amp*(exp(-0.5*r2*ivar1) + b*exp(-0.5*r2*ivar2) + aval));
      if(val < level) {
	 break;
      }
   }

   r = sqrt(r2);

   if(r > rmax) {
      shError("Too large radius for PSF in phDgpsfSize (%g > %g)", r, rmax);
      r = rmax;
   }

   return(r);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * evaluate the cumulative flux to radius r of a DGPSF
 *
 * currently ASSUMES circular symmetry
 */
float
phDgpsfCumulativeFlux(const DGPSF *psf,	/* the DGPSF in question */
		      float r)		/* the desired radius */
{
   float flux;
   float sigma1, sigma2;

   shAssert(psf != NULL);

   sigma1 = 0.5*(psf->sigmax1 + psf->sigmay1);
   sigma2 = 0.5*(psf->sigmax2 + psf->sigmay2);

   flux = 0;
   flux += 2*M_PI*sigma1*sigma1*(1 - exp(-r*r/(2*sigma1*sigma1)));
   flux += psf->b*2*M_PI*sigma2*sigma2*(1 - exp(-r*r/(2*sigma2*sigma2)));
   if(psf->sigmap != 0 && psf->beta != 0 && psf->p0 != 0) {
      float beta = psf->beta, sigmap = psf->sigmap;
#if 0
      shAssert(beta > 2);
#else
      if(beta > 2) {
#endif
      flux += psf->p0*2*M_PI*beta/(beta - 2)*
	sigmap*sigmap*(1 - pow(1 + r*r/(beta*sigmap*sigmap), 1 - beta/2));
#if !0
      }
#endif
   }

   return(psf->a*flux);
}
