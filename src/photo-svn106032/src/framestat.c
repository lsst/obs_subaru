   /*
    * <AUTO>
    * FILE: fieldstat.c
    *
    * Support for the FIELDSTAT structure
    *
    * </AUTO>
    */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <alloca.h>
#include "dervish.h"
#include "atTrans.h"			/* for atFilternames */
#include "phFramestat.h"

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * create a new FIELDSTAT
 *
 * returns:
 *   pointer to new FIELDSTAT
 */
FIELDSTAT * 
phFieldstatNew(void)			/* number of bands being processed */
{
   FIELDSTAT *new = shMalloc(sizeof(FIELDSTAT));

   phFieldstatClear(new);
   
   return(new);
}


/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * clear a FIELDSTAT
 */
void
phFieldstatClear(FIELDSTAT *fstat)
{
    int i;
    
    fstat->status = FIELD_UNKNOWN;
    fstat->nobjects = fstat->nchild = fstat->nstars = fstat->ngals = 0;

    for (i = 0; i < NCOLOR; i++) {
        fstat->rowOffset[i] = fstat->colOffset[i] = 0;

	fstat->neff_psf[i] = 0;
	fstat->saturation_level[i] = 0;
	fstat->sky_psp[i] = -1;
	fstat->sky_frames_sub[i] = -1;
	fstat->sky_frames[i] = -1;
	fstat->sigpix[i] = -1;
	fstat->deV_ap_correction[i] = -1;
	fstat->deV_ap_correctionErr[i] = -1;
	fstat->exp_ap_correction[i] = -1;
	fstat->exp_ap_correctionErr[i] = -1;
	fstat->deV_model_ap_correction[i] = -1;
	fstat->deV_model_ap_correctionErr[i] = -1;
	fstat->exp_model_ap_correction[i] = -1;
	fstat->exp_model_ap_correctionErr[i] = -1;
	fstat->nCR[i] = fstat->nbrightobj[i] = fstat->nfaintobj[i] = 0;
	fstat->median_fiberColor[i] = fstat->median_psfColor[i] = 0;
	fstat->Q[i] = fstat->U[i] = 0;
	fstat->adopted_cond3_fac2[i] = 0.0;

	fstat->objc_flags[i] = 0;
	fstat->objc_flags2[i] = 0;
    }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * delete a FIELDSTAT
 */
void
phFieldstatDel(FIELDSTAT *fieldstat) 
{

    if (fieldstat == NULL) return;
    shFree(fieldstat);
}

/*****************************************************************************/
/*
 * constructors etc. for FRAMEPARAMS
 */
static void
init_frameparams(FRAMEPARAMS *fparams,
		 char filter)		/* filter, e.g. g or r */
{
   *(int *)&fparams->filter = filter;
   fparams->dframe = 0;
   fparams->sb_mag = fparams->sb_counts = -1;
   fparams->psf = NULL;
   fparams->cprof = NULL;
   fparams->data = NULL;
   fparams->data_is_smoothed = 0;
   fparams->badmask = NULL;
   fparams->global_sky = VALUE_IS_BAD;
   fparams->sky = fparams->skyErr = NULL;
   fparams->gc_node = fparams->gc_incl = VALUE_IS_BAD;
   fparams->toGCC = NULL;
   fparams->psf_app_correction = 1;
   fparams->psf_app_correctionErr = 0;
   fparams->deV_ap_correction = 1;
   fparams->deV_ap_correctionErr = 0;
   fparams->exp_ap_correction = 1;
   fparams->exp_ap_correctionErr = 0;
   fparams->deV_model_ap_correction = 1;
   fparams->deV_model_ap_correctionErr = 0;
   fparams->exp_model_ap_correction = 1;
   fparams->exp_model_ap_correctionErr = 0;
   fparams->psfBasis = NULL;
   fparams->gain0 = fparams->gain1 = 1;
   fparams->gain = phBinregionNewFromConst(0.5*(fparams->gain0 + fparams->gain1), 1, 1, 1, 1, MAX_U16);
   fparams->dark_variance = phBinregionNewFromConst(0, 1, 1, 1, 1, MAX_U16);
   fparams->bkgd = 0;
   fparams->fullWell[0] = fparams->fullWell[1] = 0;
   fparams->extra_bad_satur_columns = 0;
   fparams->electronic_ghost_amplitude = 0;
   fparams->smooth_sigma = 0;
   fparams->ffo_threshold = 0;
   fparams->peak_min_sep = -1;
   fparams->peak_smooth_thresh = MAX_S32;
   fparams->min_peak_statistical_errors = 0;
   fparams->npeak_per_obj_max = -1;		/* i.e. unlimited */
   fparams->astrom_floor = 0;
   fparams->nann_ap_frame = fparams->nann_ap_run = -1;
   fparams->ap_corr_run = 1;
   fparams->ap_corr_runErr = 0;
   fparams->fiberCounts_seeing = 0.0;
   fparams->namp = 0;
   fparams->rowBinning = fparams->colBinning = 0;
   fparams->astrom_dcol_left = fparams->astrom_dcol_right = 0.0;
}

/*
 * <AUTO EXTRACT>
 *
 * contructor for FRAMEPARAMS
 */
FRAMEPARAMS *
phFrameparamsNew(char filter)		/* filter, e.g. g or r; or '0' */
{
   FRAMEPARAMS *new = shMalloc(sizeof(FRAMEPARAMS));

   init_frameparams(new, filter);

   return(new);
}
/*
 * <AUTO EXTRACT>
 *
 * destructor for FRAMEPARAMS
 */
void
phFrameparamsDel(FRAMEPARAMS *fparams)
{
   
   if(fparams == NULL) return;

   phFrameparamsEmpty(fparams);

   shFree(fparams);
}

/*
 * <AUTO EXTRACT>
 *
 * Free all structs associated with a FRAMEPARAMS, but not the FRAMEPARAMS
 * itself
 */
void
phFrameparamsEmpty(FRAMEPARAMS *fparams)
{
   
   shAssert(fparams != NULL);

   atTransDel((TRANS *)fparams->toGCC); fparams->toGCC = NULL;
   phCompositeProfileDel((COMP_PROFILE *)fparams->cprof);fparams->cprof = NULL;
   phObjmaskDel(fparams->badmask); fparams->badmask = NULL;
   phBinregionDel((BINREGION *)fparams->sky); fparams->sky = NULL;
   phBinregionDel((BINREGION *)fparams->skyErr); fparams->skyErr = NULL;
   phPsfBasisDel((PSF_BASIS *)fparams->psfBasis); fparams->psfBasis = NULL;
}

/*****************************************************************************/
/*
 * Methods to calculate gain and variance at point (row, col)
 */
float
phGain(const FRAMEPARAMS *fparams,	/* the source of information for this frame */
       const int row,			/* row and */
       const int col)			/*          column position within the chip; coordinates exclude overscan */
{
    shAssert(fparams != NULL && fparams->gain != NULL);

    return phBinregionInterpolate(fparams->gain, row, col);
}

float
phDarkVariance(const FRAMEPARAMS *fparams, /* the source of information for this frame */
	       const int row,		   /* row and */
	       const int col)		   /*          column position within the chip; coordinates exclude overscan */
{
    shAssert(fparams != NULL && fparams->dark_variance != NULL);

    return phBinregionInterpolate(fparams->dark_variance, row, col);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * contructor for FIELDPARAMS
 */
FIELDPARAMS *
phFieldparamsNew(const char *cfilters)	/* names of filters */
{
   int i;
   int ncolor;				/* number of colours being processed */
   FIELDPARAMS *new = shMalloc(sizeof(FIELDPARAMS));
   char *filters;

   if(cfilters == NULL) {
      cfilters = "0";
   }
   
   filters = alloca(strlen(cfilters) + 1);
   for(i = ncolor = 0; cfilters[i] != '\0'; i++) { /* remove spaces */
      if(!isspace(cfilters[i])) {
	 filters[ncolor++] = cfilters[i];
      }
   }
   filters[ncolor] = '\0';
/*
 * We usually require that all filternames be from the SDSS set, but
 * when only one filter is involved we can relax this condition
 */
   if(ncolor > 1) {
      for(i = 0; i < ncolor; i++) {
	 shAssert(strchr(atFilternames, filters[i]) != NULL);
      }
   }
   
   new->ncolor = ncolor;
   shAssert(ncolor <= sizeof(new->filters));
   strcpy((char *)new->filters, filters);
   
   new->frame = shMalloc(ncolor*sizeof(new->frame[0]));

   new->run = new->camCol = new->fieldnum = -1;

   new->scan_overlap = new->run_overlap = -1;
   new->ncol = -1;
   new->pixscale = -1;
   new->smooth_profs = 1;
   new->median_profs = 0;
   new->petro_f1 = -1;
   new->petro_f2 = -1000;
   new->petro_f4 = 0;
   new->petro_f5 = 0;
   new->fiber_rad = -1;
   new->ref_band_index = new->canonical_band_index = -1;
   new->nchild_max = 0;
   new->deblend_min_detect = 1;
   new->deblend_min_peak_spacing = 1000;
   new->deblend_psf_Lmin = -1;
   new->deblend_psf_nann = 0;
   new->deblend_psf_rad_max = 0;
   new->deblend_npix_max = 0;
   new->deblend_allowed_unassigned = 0;
   new->smooth_petro_radii = 1;
   new->petro_gcv_errors = 0;
   new->use_galaxy_ap_correction = 0;
   new->deblend_inner_max = 1.0;
   new->psf_constraint_nmodel = -1;
   new->deblend_bright_objects = 0;

   for(i = 0; i < NCLASSIFIER_FIDDLE; i++) {
      new->sg_classifier[i] = 0.0;
   }

   for(i = 0;i < ncolor;i++) {
      init_frameparams(&new->frame[i], filters[i]);
      new->classify_bands[i] = -1;
   }

   return(new);
}

/*
 * <AUTO EXTRACT>
 *
 * destructor for FIELDPARAMS
 */
void
phFieldparamsDel(FIELDPARAMS *fparams)
{
   int i;
   
   if(fparams == NULL) return;
   
   for(i = 0;i < fparams->ncolor;i++) {
      phFrameparamsEmpty(&fparams->frame[i]);
   }
   shFree(fparams->frame);

   shFree(fparams);
}
