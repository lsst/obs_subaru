/*
 * This is the new measure objects code
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dervish.h"
#include "phMaskbittype.h"
#include "phMeasureObj.h"
#include "phCellFitobj.h"
#include "phUtils.h"
#include "phVariablePsf.h"

/*****************************************************************************/
/*
 * Here are the init and fini functions for measure objects
 */
int
phInitMeasureObj(FIELDPARAMS *fparams,	/* where to store the parameters */
		 int ncolor,		/* number of colours being processed */
		 float *sb_mag,		/* surf brightness of isophotes for
					   measuring object ellipticity,
					   isophotal quantities, mag/arcsec^2*/
		 RANDOM *rand)		/* random numbers */
{
   int i;

   p_phInitMeasureObj(ncolor, rand);

   fparams->ncolor = ncolor;

   for(i = 0;i < ncolor;i++) {
      fparams->frame[i].sb_mag = sb_mag[i];
   }

   {
      int ret = p_phSetParamsRun(fparams);
      shAssert(ret == 0);
   }
   
   return(SH_SUCCESS);
}

int
phFiniMeasureObj(void)
{
   p_phFiniMeasureObj();
   
   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * These functions must be called before starting to call phMeasureObjc for
 * a set of objects; almost no memory is allocated but the parameters and
 * data to be used are stored; in addition the NOT_CHECKED OBJMASK chain is
 * merged into an OBJMASK for each colour.
 *
 * Why do we do this? Because otherwise the TCL overhead of calling
 * phMeasureObjc would be high, and the OBJMASK merge would be disasterous
 *
 * More specifically, you need to call phMeasureObjColorSet for each colour
 * that you'll be measuring, then phMeasureSet with the all-colour parameters:
 *  for(i = 0;i < ncolor;i++) {
 *     phMeasureObjColorSet(fparams,i,calib[i],...);
 *  }
 *
 *  phMeasureObjc(objc,fparams);
 *  phMeasureObjc(objc,fparams);
 *  ...
 *  phMeasureObjc(objc,fparams);
 *
 *  phMeasureObjUnset(fparams);
 *
 * This function is in addition to the Init and Fini functions required
 * by the Measure Objects module; init/fini allocate space and set up tables
 * independent of the particular frames being analysed, while ColorSet
 * is frame specific
 */
RET_CODE
phMeasureObjColorSet(FIELDPARAMS *fiparams, /* describe field */
		     FIELDSTAT *fieldstat, /* stats for field's reduction */
		     int color,		/* index of colour being set up */
		     const CALIB1 *calib, /* calibration info */
		     const REGION *data, /* corrected frame */
		     const BINREGION *sky, /* typical sky */
		     const BINREGION *skyErr, /* sky noise */
		     const PSF_BASIS *basis) /* variable PSF */
{
   FRAMEPARAMS *fr_params;
   SPANMASK *sm = NULL;

   shAssert(fiparams != NULL);
   shAssert(color >= 0 && color < fiparams->ncolor);

   if(data != NULL) {
      shAssert(data->mask != NULL);
      sm = (SPANMASK *)(data->mask);
      shAssert(sm->cookie == SPAN_COOKIE);
   }

   fr_params = &fiparams->frame[color];
   shAssert(fr_params->cprof == NULL);
/*
 * Unpack the CALIB1
 *
 * Turn the surf_brightness magnitude into a flux (counts/pix^2)
 * Note that this is not a truly calibrated flux, but it's the best we can
 * do in the photometric pipeline.  we end up with sb_counts equalling
 * the flux from a "sb_mag" mag point source concentrated in a single pixel
 */
   {
      float flux20 = calib->flux20;	/* flux of 20'th mag point source */
      float sb_counts = flux20*pow(10.0, 0.4*(20.0 - fr_params->sb_mag));
      sb_counts *= fiparams->pixscale*fiparams->pixscale;

      fr_params->flux20 = flux20;
      fr_params->sb_counts = sb_counts;
   }
   fr_params->psf = calib->psf;
   shAssert(fabs(fr_params->psf->a - 1.0) < 1e-5);
/*
 * must cast away const
 */
   *(COMP_PROFILE **)&fr_params->cprof = calib->prof;
   *(COMP_PROFILE **)&calib->prof = NULL;
   (*(COMP_PROFILE **)&fr_params->cprof)->prof_is_set = 1;
   shAssert(fr_params->cprof->psfCounts > 0);

   fr_params->gc_node = calib->node;
   fr_params->gc_incl = calib->incl;
   fr_params->toGCC = calib->toGCC;
   *(COMP_PROFILE **)&calib->toGCC = NULL;
   shAssert(calib->psf_ap_correctionErr >= 0.0);
#if 0
   fr_params->psf_app_correctionErr =
				 1 - pow(10, -0.4*calib->psf_ap_correctionErr);
#else
   fr_params->psf_app_correctionErr = 0; /* no longer used; subsumed into
					    calib->ap_corr_runErr. PR 2744 */
#endif
   fr_params->nann_ap_frame = calib->nann_ap_frame;
   fr_params->nann_ap_run = calib->nann_ap_run;
   fr_params->ap_corr_run = pow(10, -0.4*calib->ap_corr_run);
   fr_params->ap_corr_runErr = 1 - pow(10, -0.4*calib->ap_corr_runErr);

   fr_params->psfBasis = basis;

#if 0					/* it's also set in TCL,
					   taking co-adds into account */
   fr_params->gain = calib->gain;	/* n.b. this won't compile -- gain is a BINREGION */
   fr_params->dark_variance = calib->dark_variance; /* this won't either, for a closely related reason */
#endif
     
   fr_params->data = data;
   if(sm == NULL) {
      fr_params->badmask = NULL;
   } else {
      fr_params->badmask = phMergeObjmaskChain(sm->masks[S_MASK_NOTCHECKED]);
   }
   fr_params->sky = sky;
   fr_params->skyErr = skyErr;
#if 0					/* No longer needed (I think; remove next time you remember) */
/*
 * phFitCellColorSet wants a DGPSF with its coeffs[] values set; if they aren't
 * do so here. We have to make a REGION containing a PSF with an empty mask,
 * and then measure it
 */
   {
      const PSF_COEFFS *coeffs;		/* describe the PSF */
      if(fr_params->psf->coeffs.L >= 0) {	/* ok */
	 coeffs = &calib->psf->coeffs;
      } else {				/* not set */
	 REGION *reg =
	   shRegNew("psf", SYNC_REG_SIZEI+10, SYNC_REG_SIZEI+10, TYPE_PIX);
	 
	 shRegIntSetVal(reg,SOFT_BIAS);
	 phDgpsfAdd(reg, fr_params->psf, 0, 0.5*reg->nrow, 0.5*reg->ncol, 32000);
	 reg->mask = (MASK *)phSpanmaskNew(reg->nrow, reg->ncol);
	 
	 coeffs = phFitCellPsfCoeffs(reg, fr_params->psf->sigma1_2G, 0);
	 
	 phSpanmaskDel((SPANMASK *)reg->mask); reg->mask = NULL;
	 shRegDel(reg);
      }
      phFitCellColorSet(color, coeffs);
   }
#endif
/*
 * calculate the PSF's first and second derivatives
 */
   shAssert(fr_params->psfBasis == NULL || fr_params->psfBasis->kern != NULL);
   
   if(fr_params->psfBasis != NULL && fr_params->psfBasis->kern->nsigma > 0) {
      static int lbell = 5;
      phPsfBasisSetDerivatives((PSF_BASIS *)fr_params->psfBasis, NULL, lbell);
   }

   (void)p_phSetParamsFrame(color, fiparams, fieldstat);
   
   return(SH_SUCCESS);
}

void
phMeasureObjUnset(FIELDPARAMS *fiparams)
{
   int i;
   
   for(i = 0; i < fiparams->ncolor; i++) {
      phFrameparamsEmpty(&fiparams->frame[i]);
      p_phUnsetParamsFrame(i, fiparams);
   }
}
