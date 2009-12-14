/*
 * <AUTO>
 *
 * FILE: findPsf.c
 * 
 * DESCRIPTION:
 * routine to make star1s with fitted PSF's for all good postage stamps
 * on input list.
 *
 * </AUTO>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <alloca.h>
#include <assert.h>
#include "atConversions.h"		/* for M_PI */
#include "dervish.h"
#include "phUtils.h"
#include "phFindPsf.h"
#include "phStar1.h"
#include "phStar1c.h"
#include "phCellFitobj.h"
#include "phMathUtils.h"
#include "phMeasureObj.h"
#include "phBrightObjects.h"
#include "phObjc.h"
#include "phObjectCenter.h"		/* prototype for phObjectCenterFind */
#include "phExtract.h"


 
/***************************************************************************
 * <AUTO EXTRACT>
 *
 * This module determines the best psf for each frame
 */
#define CHECK_OFFSETS 0			/* true to generate diagnostics
					   about offset between astrometric
					   and photometric positions */

#define DUMP_PROFILE 1			/* true to dump comoposite profile
                                           and the best fit PSF to files 
                                           comp_prof.dat and best_fits.dat */
 
#define IGNORE_COMPPROF 0               /* true to use cprof_bar instead of 
                                           comp_prof to fit PSF (2 Gauss only) */ 
#if DUMP_PROFILE
static void dump_psf_fit(FILE *fp, const int jf, const COMP_PROFILE *comp_prof,
		       const DGPSF *psf, float profmin, int medflg);
static void dump_comp_prof(const int jf, const COMP_PROFILE *comp_prof,
		       FILE *fp);
static void dump_wing_slope(FILE *fp, const char *filter, const CHAIN *winglist, 
                       int medflg);
static void dump_wing_prof(FILE *fp, const char *filter, const CHAIN *winglist, 
                       int medflg);
#endif 

static void fill_psf_fit(FRAME_INFORMATION *frame_info, const COMP_PROFILE *comp_prof, 
                       const DGPSF *psf, float profmin, int medflg);	

static void scale_cprof(int type, float *flux, CELL_PROF *cprof);
 
static float get_scale(int type, float *flux, CELL_PROF *cprof_bar);

static void pack_frameinfo(FRAME_INFORMATION *frame_info, int first, int last, int Ngood, 
                       int *badstars);

static int clip_cprof(const CHAIN *starlist, FRAME_INFORMATION *frame_info, 
                       int first, int last, float ncellclip, int nbadcell, float maxdev, 
                       int min_starnum, int badval, int *Ngood, int *badstar);
   
static int clip_stars(const CHAIN *starlist, FRAME_INFORMATION *frame_info,
		      int first, int last,
		      float nsigma_sigma, float nsigma_QU, float nsigma_RMS_rad,
		      int min_starnum, int badval, int niter, int *Ngood, int *badstar);
static int goodPSF(DGPSF *psf);
static float profileFromPSFReg(const PSF_REG *psfReg, int id, float sky, float skysig, 
                         float gain, float dark_var, CELL_PROF *cprof, OBJECT1 *object);
static int cmp(const void *a, const void *b);
static float get_quartile(float *buf, int n, int sorted, float q);
static float get_wing_slope(STAR1 *wing, int medflg);
 

RET_CODE
phSelectPsfStars(const CHAIN *starlist, /* Chains of star1s with stamps */
	  const CHAIN *cframes,		/* I: calib1byframes */
          CHAIN **frameinfolist,        /* info about PSF stars for each frame */
	  int ncolor,			/* number of starlists */
	  int i,		 	/* index of this filter */
          int frame0,                   /* this frame (zero indexed) */
          int first_star,               /* first star to analyze */
          int last_star,                /* last star to analyze (>= first_star) */
          float ncellclip,              /* # of sigma for a cell to be bad */ 
          int nbadcell,                 /* max. # of bad cells for a star to be good */ 
          float maxdev,                 /* max. dev. of a cell to condemn all */ 
	  float nsigma_sigma,		/* how many sigma to clip sigma at */
	  float nsigma_QU,		/* how many sigma to clip QU at */
	  float nsigma_RMS_rad,		/* how many sigma to clip RMS rad. at*/
	  int niter,			/* no. of times to iterate clipping */
	  int min_starnum)		/* min # of stars for PSF fitting*/             
{
    int *badstar;                       /* flags telling which stars survived various rejections */
    int Nstars;
    const char *filter;			  /* filter letter code */  
    FRAME_INFORMATION *frame_info = NULL; /* set to NULL to pacify compilers */    
    int Ngood;                            /* number of good stars */
    int frame;                            /* this frame (not zero indexed) */
    STAR1 *star;
    CALIB1BYFRAME *cal;
    int j;

    /* some sanity checks */
    shAssert(ncolor > 0);
    shAssert(starlist != NULL);
    shAssert(starlist->type == shTypeGetFromName("STAR1"));
    shAssert(cframes != NULL);
    shAssert(cframes->type == shTypeGetFromName("CALIB1BYFRAME"));

    shAssert(first_star <= last_star);
    shAssert(shChainSize(starlist) > last_star);

    Ngood = Nstars = shChainSize(starlist);
    badstar = shMalloc(Nstars * sizeof(int));
    memset(badstar,'\0',Nstars*sizeof(int));

    /* get the letter code for this filter */
    cal = shChainElementGetByPos(cframes, frame0);
    filter = cal->calib[i]->filter;


    /* make and stuff frame_info */       
    frame_info = phFrameInformationNew(Nstars); 
    frame = frame_info->frame = cal->field; 
    frame_info->filter[0] = *filter; 
    frame_info->firststar = first_star;
    frame_info->Nallstars = Ngood = last_star - first_star + 1;
    /* flags: sigma & b rejection is 1-7 by default */
    frame_info->FbadQU = 8; 
    frame_info->FbadCell = 9; 


    /* reset all flags to 0 (unless the old value is 10) */
    for (j = 0; j < shChainSize(starlist); j++) {
	 star = shChainElementGetByPos(starlist,j);
         star->used4coeffs = 0; 
         star->used4photo = 0; 
	 if (star->badstar != 10) {              
	    star->badstar = 0; 
         }   
    }

    /* clipping on the deviation from a median in each cell */
    /* this step throws out grossly discrepant objects */
    if ((Ngood = clip_cprof(starlist, frame_info, first_star, last_star, ncellclip, nbadcell, 
		            maxdev, min_starnum, frame_info->FbadCell, &Ngood, badstar)) < 1) {
               shError("phSelectPsfStars: skipping cell median rejection for filter %s,"
                       " frame %d, have only %d acceptable input stars (need at least %d,"
                       " consider changing ncellclip, nbadcell, and/or maxdev). ", 
                       filter, frame, -Ngood, min_starnum+1);
    }
 
    /* clipping on QU, adaptive moments RMS size, and sigma1/sigma2/b */
    if ((Ngood = clip_stars(starlist, frame_info, first_star, last_star,
			    nsigma_sigma, nsigma_QU, nsigma_RMS_rad,
			    min_starnum, frame_info->FbadQU, niter,
			    &Ngood, badstar)) < 1) {
       shError("phSelectPsfStars: skipping width and shape rejection for filter %s,"
	       " frame %d, have only %d acceptable input stars (need at least %d,"
	       " consider increasing nsigma_sigma, nsigma_QU, and/or psf_nframe)",
	       filter, frame, -Ngood, min_starnum+1);
    }

    /* let each star know if it's good */
    Ngood = 0; 
    for (j = first_star; j <= last_star; j++) {
	 star = shChainElementGetByPos(starlist,j);
         star->PSFflag = 0;
         if (badstar[j] == 0) {
	    /* n.b. flags are reset to 0, except when the old value is 10 */
	    /* the value of 10 is set in prepare_star1 and shows that this star */
	    /* is too faint */    
	    if (star->badstar != 10) {
                Ngood++;
                star->badstar = 0;
	    }
         } else {
	    star->badstar = badstar[j]; 
         }      
    }

    /* save the number of good stars */
    cal->calib[i]->psf_nstar = frame_info->NgoodBasis = Ngood;
    
    shChainElementAddByPos(frameinfolist[i],frame_info,"FRAME_INFORMATION",TAIL,AFTER);

    shFree(badstar); 
 
    return(SH_SUCCESS);

}




RET_CODE
phQAforPsfStars(const CHAIN *starlist,  /* Chains of star1s with stamps */
          PSF_BASIS *KLpsf,	        /* KL PSF for this frame and color */
	  CHAIN *cframes,		/* chain of calib1byframes */
          CHAIN **frameinfolist,        /* info about PSF stars for each frame */
	  int ncolor,			/* number of starlists */
	  int icolor,		 	/* index of this filter */
          int frame,                    /* this frame */
          int nrow,                     /* number of pixel rows per frame */
          int ncol,                     /* number of pixel cols per frame */
	  float gain,		        /* gain for this color */
	  float dark_var,		/* dark variances for this color */
          float sky,                    /* mean sky for this frame and filter */
          float skyerr,                 /* mean sky error for this frame and filter */
          int first_star,               /* first star to analyze */
          int last_star,                /* last star to analyze (>= first_star) */    
          float q_min,	                /* lower parameter for clipping */
 	  float q_max)		        /* upper parameter for clipping */
    
{
    const char *filter;			   /* filter letter code */  
    FRAME_INFORMATION *frame_info = NULL;  /* set to NULL to pacify compilers */    
    STAR1 *star;
    CALIB1BYFRAME *cal, *cal_prev;
    int j;
    int *badstar;                       /* flags telling which stars are bad */
    CELL_PROF *cprof_bar = NULL;	/* CELL_PROF for reconstructed PSF at the frame center */
    CELL_PROF *cprof_loc = NULL;	/* CELL_PROF for reconstructed PSF at a star's position */
    PSF_REG *centerPSF;                 /* recontructed PSF at the frame center */
    OBJECT1 *PSFobject = NULL;
    OBJECT1 *LocalObject = NULL;
    float ap_corr;   /* aperture correction */
    DGPSF *psf_center = NULL;
    DGPSF *psf_local = NULL;
    float neff;
    float rowc, colc;	
    float *apCrat;			/* ratio (in mags) of aperture corr.
					   determined from reconstructed PSF 
					   at a star's position and the
					   aperture correction determined
					   directly from that star's region */
    float *apCratErr;			/* errors in apCrat */
    float *apCratDummy;		        /* for determining apCor error due to error in apC */
    float *wingCorr;			/* ratio of aperture counts enclosed by the 7th and 5th annulus */
    float *widths;  
    float xmed, xq25, xq75; 
    float apCrat_min, apCrat_max, apCerrCorr; 
    int Napcor = 0, Nwidth = 0, Nwingcor =0;  /* used when calling get_quartile */
    int Nstars;                               /* number of stars in this window */
    int Nframestars = 0;                      /* number of stars from this frame */
    int NgoodBasis = 0;                       /* # of good basis stars for this frame */
    int NgoodCoeffs = 0;                      /* # of good coeff stars for this frame */


    shAssert(ncolor > 0);
    shAssert(starlist != NULL);
    shAssert(starlist->type == shTypeGetFromName("STAR1"));
    shAssert(cframes != NULL);
    shAssert(cframes->type == shTypeGetFromName("CALIB1BYFRAME"));

    if (cprof_bar == NULL) {
        cprof_bar = phCellProfNew(NCELL);
    }
    if (cprof_loc == NULL) {
        cprof_loc = phCellProfNew(NCELL);
    }
    if (PSFobject == NULL) {
        PSFobject = phObject1New();
    }
    if (LocalObject == NULL) {
        LocalObject = phObject1New();
    }
    if (psf_center == NULL) {
        psf_center = phDgpsfNew();        
    }
    if (psf_local == NULL) {
        psf_local = phDgpsfNew();        
    }

    Nstars = shChainSize(starlist);

    badstar = shMalloc(Nstars * sizeof(int));
    memset(badstar, '\0', Nstars*sizeof(int));

    apCrat = shMalloc(5*Nstars * sizeof(float));
    apCratErr = apCrat + Nstars;
    widths = apCratErr + Nstars;
    apCratDummy = widths + Nstars;
    wingCorr = apCratDummy + Nstars; 
    memset(apCrat, '\0', 5*Nstars*sizeof(float));

    /* get the letter code for this filter */
    cal = shChainElementGetByPos(cframes, frame);
    filter = cal->calib[icolor]->filter;
    frame_info = shChainElementGetByPos(frameinfolist[icolor], frame);


    /* reconstruct the PSF at the frame center */
    centerPSF = phPsfKLReconstruct(KLpsf, nrow/2, ncol/2, TYPE_PIX);          
    /* get CELL_PROF for the reconstructed PSF at the frame center */
    if (profileFromPSFReg(centerPSF,-1,sky,skyerr,gain,
				 dark_var, cprof_bar, PSFobject) < 0) {
          shError("phQAforPsfStars: cannot get  center PSF CELL_PROF for frame %d," 
                  " filter %s.", frame, filter); 
    }


    /* set up "the mean PSF" for this frame */
    phDgpsfFitFromCellprof(psf_center, cprof_bar);
    neff = phPsfCountsSetupFromDgpsf(icolor,psf_center,1); /* XXX We do this again later XXXX */
    if (neff < 1) {
        shError("phQAforPsfStars: neff < 1 (!!) for frame %d, filter %s.",
                        frame, filter);
        if (neff <= 0) {
            shFatal("phQAforPsfStars: neff <= 0 for frame %d, filter %s.",
                        frame, filter);
        }
    }
 

    /* for each star calculate psfCounts for itself and for reconstructed PSF */
    for (j = first_star; j <= last_star; j++) {
	 star = shChainElementGetByPos(starlist,j);         
	 star->used4photo = 0; 
         /* skip if this star was clipped */
         if (star->badstar != 0) continue;
         NgoodBasis += 1;
         /* need statistics only if this star is used for coeffs. */
         if (star->used4coeffs <= 0) continue;
         NgoodCoeffs += 1;
         /* we want photometric bias estimate based on _all_ coeff stars */
         if (1 || star->frame == frame_info->frame) {
	     /* this star comes from the current frame */ 
             Nframestars += 1;
             /* this star should be used for QA */
             star->PSFflag = 1;
             /* since this star was used for PSF coeffs get some statistics */
             /* position of this star in absolute coordinates */
             colc =  star->object->colc;
             rowc = star->object->rowc;
             /* master row */
             rowc += (star->frame-frame_info->frame)*nrow;
             /* aperture correction evaluated at star's position */
	     ap_corr = phPsfSetAtPoint(icolor, KLpsf,
				       rowc, colc, psf_center->sigma1_2G,NULL);
#define ZI_DEBUG 0
#if ZI_DEBUG
	     {
		static char *dump_filename = NULL; /* write data to this file?
						      For use from gdb */
		int extra = 15;		/* padding for syncreg */
		float ap_corr2;		/* new estimate of ap_corr */
		const REGION *syncreg = star->syncreg;
		REGION *big = shRegNew("", syncreg->nrow + 2*extra,
				       syncreg->ncol + 2*extra, TYPE_PIX);
		REGION *sbig;		/* subreg of big */
		shRegIntSetVal(big, SOFT_BIAS);

		sbig = shSubRegNew("", big, syncreg->nrow, syncreg->ncol,
					   extra, extra, NO_FLAGS);
		shRegIntCopy(sbig, syncreg);
		shRegIntConstAdd(sbig, -FLT2PIX(star->object->sky), 0);
		shRegDel(sbig);
		
		ap_corr2 = phPsfSetFromRegion(icolor, big, NULL);

		if(dump_filename != NULL) {
		   shRegWriteAsFits(big, dump_filename,
				    STANDARD, 2, DEF_NONE, NULL, 0);
		   dump_filename = NULL;
		}

		shRegDel(big);
	     }
#endif
             star->apCorr_PSF = ap_corr;
             /* psf counts for this star (from syncreg) */
             star->psfCounts = phPsfCountsFromRegion(star->syncreg,   
                                 icolor, -1, -1, ap_corr, star->object->sky, 
                                 gain, dark_var, 0.0, &star->psfCountsErr);
             /* aperture corrections ratio as test of KL reconstruction */
             if (star->apCounts > 0 && star->psfCounts > 0) {
		float star_apCorr = star->apCounts/star->psfCounts;
#if 1
                float nsr = star->apCountsErr/star->apCounts;  
#else
                float nsr = star->psfCountsErr/star->psfCounts;
#endif
		star->count_rat = -2.5*log10(star_apCorr);
                star->count_ratErr  = pow(star->apCountsErr/star->apCounts,2);
                star->count_ratErr += pow(star->psfCountsErr/star->psfCounts,2);
                /* 1.086 = 2.5/ln(10) */
                star->count_ratErr  = 1.086*sqrt(star->count_ratErr);
                /** limit on SNR (1/0.005) determined by studying ~20,000 stars from run 94 **/
                /** XXX **/
                if (nsr < 0.02) {
                    apCrat[Napcor] = star->count_rat;
                    apCratErr[Napcor] = star->count_ratErr;
                    apCratDummy[Napcor] = 0;
		    Napcor++;
                    star->used4photo = 1;
		}               
                if (nsr < 0.01) {
                    wingCorr[Nwingcor] = star->wingcorr;
                    Nwingcor++;
		}               
	     } else {
		star->count_rat = 0; 
                star->count_ratErr = 10;
             }
             /* efective width */
             widths[Nwidth++] = star->Eff_width;

#if ZI_DEBUG
/* this QA code commented since it was taking to much time and is not essential  
   it repeats the same computations as for the real stars, but uses
   the KL expansion at the star's position instead 
   the idea is to compare
   1) ap_corr and star->apCounts_PSF/star->psfCounts_PSF
      they better be same as this is simply code testing
   2) the observed star->Eff_width should be similar to modeled star->PSFwidth
   3) the observed star->apCounts/star->psfCounts should be similar to modeled
      ap_corr = star->apCounts_PSF/star->psfCounts_PSF (see 1)
   4) the observed star->apCounts/star->psfCounts should be roughly constant
      with time and show rms scatter no larger than ~2%
*/
       {
         PSF_REG *localPSF = NULL;      
         REGION *tmp_reg = NULL;
         CELL_STATS *cstats_loc = NULL;      
 
         /* and then psfCounts determined from PSF reconstructed at star's position */
         /* reconstruct the local PSF at the star's position */
         localPSF = phPsfKLReconstruct(KLpsf, rowc, colc, TYPE_PIX);          
         /* get CELL_PROF for the local PSF */
         if (profileFromPSFReg(localPSF,star->id,sky,skyerr,gain,
				 dark_var, cprof_loc, LocalObject) < 0) {
              shError("phQAforPsfStars: cannot get the local PSF CELL_PROF for star %d,"
                      " frame %d, filter %s.", j, frame, filter); 
         }  
	 cstats_loc = phProfileGetLast();
         /* for QA determine effective seeing from the reconstructed PSF */
         phDgpsfFitFromCellprof(psf_local, cprof_loc);
         neff = phPsfCountsSetupFromDgpsf(icolor, psf_local, 1);
         star->PSFwidth = 0.663 * 0.400 * sqrt(neff);
         /* determine psf counts for reconstructed PSF using local estimate
	    of Gaussian weights */  
         tmp_reg = shRegNew("",localPSF->reg->nrow,localPSF->reg->ncol,TYPE_PIX);
         shRegIntCopy(tmp_reg, localPSF->reg);
         /* n.b. ap_corr is set to 1, so these are _uncorrected_ psf counts */
         ap_corr = 1.0;
         star->psfCounts_PSF = phPsfCountsFromRegion(tmp_reg, icolor,  
                          LocalObject->rowc, LocalObject->colc, ap_corr,
                          0.0, gain, dark_var, 0.0, NULL); 
         /* and now get the aperture counts for the reconstructed PSF */
         star->apCounts_PSF = phApertureCounts(cstats_loc, star->aperture,
					       0, 0, 0, NULL);
         /* clean */
         shRegDel(tmp_reg);
         phPsfRegDel(localPSF);
       }
#endif 
         /* end of loop over stars from this frame*/ 
         }
    /* end of loop over all stars */ 
    }

    /* find statistics of ap. corr. ratios */
    frame_info->Nphoto = Napcor;
    if (Napcor > 0) {

       apCerrCorr = 0;

#if 0					/* old code */
       frame_info->apCrat_med = get_quartile(apCrat, Napcor, 0, 0.50);	
       frame_info->apCrat_q25 = get_quartile(apCrat, Napcor, 1, 0.25);	
       frame_info->apCrat_q75 = get_quartile(apCrat, Napcor, 1, 0.75);	

#elif 0					/* median without errors */
       phFloatArrayStats(apCrat, Napcor, 0,
			 &frame_info->apCrat_med, NULL,
			 &frame_info->apCrat_q25,
			 &frame_info->apCrat_q75);
#else					/* median including errors */
       phFloatArrayStatsErr(apCrat, apCratErr, Napcor,
			    &frame_info->apCrat_med, NULL,
			    &frame_info->apCrat_q25,
			    &frame_info->apCrat_q75);
       phFloatArrayStatsErr(apCratDummy, apCratErr, Napcor,
			    &xmed, NULL, &xq25, &xq75);
       shAssert(xq75 >= xq25);
       apCerrCorr = xq75-xq25;
#endif

       /* PR 3744 */
       if (Nwingcor > 0) {
           cal->calib[icolor]->psf_wingcorr = get_quartile(wingCorr, Nwingcor, 0, 0.50);	
       } else {
           cal->calib[icolor]->psf_wingcorr = -111; 
       }
       frame_info->wingcorr = cal->calib[icolor]->psf_wingcorr;

       shAssert(frame_info->apCrat_q75 >= frame_info->apCrat_q25);
       frame_info->apCrat_rms = frame_info->apCrat_q75 - frame_info->apCrat_q25;
       /* correct for contribution from error in apCount */
       {
	   const float var = pow(frame_info->apCrat_rms,2) - pow(apCerrCorr,2);	// corrected variance
	   frame_info->apCrat_rms = (var <= 0) ? 0 : sqrt(var);
       }
       frame_info->apCrat_rms *= IQR_TO_SIGMA; 
       /* errors due to PSF mismatch */
       cal->calib[icolor]->psf_ap_correctionBias = frame_info->apCrat_med;
       cal->calib[icolor]->psf_ap_correctionErr = frame_info->apCrat_rms;
    } else {
       frame_info->apCrat_med = 0.0;  
       frame_info->apCrat_q25 = 0.0;
       frame_info->apCrat_q75 = 0.0;
       frame_info->apCrat_rms = 0.0;
       if (frame > 0) {
	   /* there are no stars on this frame, take psf_ap_correctionErr 
              from the previous frame */
           cal_prev = shChainElementGetByPos(cframes, frame-1);
           cal->calib[icolor]->psf_ap_correctionErr = 
                                 cal_prev->calib[icolor]->psf_ap_correctionErr;   
       } else {
	   /* this is the first frame, take a canonical error */
           cal->calib[icolor]->psf_ap_correctionErr = 1.0;
       }   
    }

    /* find statistics of effective widths */
    if (Nwidth > 0) {
       frame_info->width_med = get_quartile(widths, Nwidth, 0, 0.50);	   
       frame_info->width_q25 = get_quartile(widths, Nwidth, 1, 0.25);	   
       frame_info->width_q75 = get_quartile(widths, Nwidth, 1, 0.75); 
    }


    /* clipping on m(aperture) - m(psf): set star->badstar to -2 for 
       all stars whose apCrat is outside q_min - q_max range */
    if (Napcor > 0) {
      apCrat_min = get_quartile(apCrat, Napcor, 0, q_min);	
      apCrat_max = get_quartile(apCrat, Napcor, 0, q_max);	
      for (j = first_star; j <= last_star; j++) {
	 star = shChainElementGetByPos(starlist,j);
         /* skip if this star was not used for QA */
         if (star->PSFflag != 1) continue;
         if (star->used4coeffs < 1) continue; 
         if (star->count_rat < apCrat_min ||
             star->count_rat > apCrat_max) {
             star->badstar = -2;
         }
      }
    } 
  
    /* save the number of good stars */
    frame_info->Nframestars = Nframestars;
    frame_info->NgoodBasis = NgoodBasis;
    cal->calib[icolor]->psf_nstar = frame_info->NgoodCoeffs = NgoodCoeffs;

    /* clean */
    phPsfRegDel(centerPSF);
    phObject1Del(PSFobject);    
    phCellProfDel(cprof_bar);
    phDgpsfDel(psf_center);
    phObject1Del(LocalObject);    
    phCellProfDel(cprof_loc);
    phDgpsfDel(psf_local);
    shFree(badstar);
    shFree(apCrat);
 
    return(SH_SUCCESS);

}


RET_CODE
phCheckCompiler(float x)             
{
    double z[1]; double y[1]; double C;
    int j=0, k=0;

       C = 2.0;
       y[0] = x;
       z[0] = x;
       shError("BEFORE: j=%d, k=%d, y0=%f, z0=%f",j,k,y[0],z[0]);
       /* this breaks when compiled with -opt*/
       y[j++] /= C; 
       /* this seems OK */
       z[k] /= C; 
       k++;
       shError("  Both y0 and z0 should be 5");
       shError("AFTER: j=%d, k=%d, y0=%f, z0=%f",j,k,y[0],z[0]);


     return(SH_SUCCESS);

}
   



DGPSF *
phKLpsf2dgpsf(PSF_BASIS *KLpsf,	        /* given PSF_BASIS */
              int row,                  /* position for evaluating PSF */
              int col,                  /* position for evaluating PSF */
              float sky,                /* sky at (row,col) */
              float skyerr,             /* sky error at (row,col) */
	      float gain,	        
	      float dark_var)
{
    CELL_PROF *cprof = NULL;	        /* CELL_PROF for reconstructed PSF */
    DGPSF *psf = phDgpsfNew();		/* DGPSF to hold results */
    PSF_REG *psfreg;                    /* recontructed PSF */
    OBJECT1 *PSFobject = NULL;          /* aux place holder */
    float neff_psf;

    shAssert(KLpsf != NULL);
 
       if (cprof == NULL) {
           cprof = phCellProfNew(NCELL);
       }
       if (PSFobject == NULL) {
           PSFobject = phObject1New();
       }

       /* reconstruct the PSF at the given position */
       psfreg = phPsfKLReconstruct(KLpsf, row, col, TYPE_PIX);          

       /* get CELL_PROF for the reconstructed PSF */
       if (profileFromPSFReg(psfreg,-1,sky,skyerr,gain,
				 dark_var, cprof, PSFobject) < 0) {
          shError("       phKLpsf2dgpsf: cannot get CELL_PROF from PSF_REG");
          psf->chisq = -1.0;
          phPsfRegDel(psfreg);      
          if (cprof != NULL) {
              phCellProfDel(cprof);
          }   
          if (PSFobject != NULL) {
              phObject1Del(PSFobject);
          } 
          return(NULL);
       }  
       /* 2-Gaussian fit to PSF */
       phDgpsfFitFromCellprof(psf, cprof);

       /* effective seeing (in arcsec) for this PSF 
	  N.b. This routine sets the Gaussian weights used 
	  for determining the raw psfCounts if the last argument is true */  
       neff_psf = phPsfCountsSetupFromDgpsf(0, psf, 1);
       psf->width = 0.663 * 0.400 * sqrt(neff_psf);

       /* store aperture correction as chisq */
       psf->chisq = phPsfSetAtPoint(0, KLpsf, row, col, psf->sigma1_2G, NULL);

       if (!goodPSF(psf)) {
            shError("phKLpsf2dgpsf: could not fit 2 Gaussians to KL PSF");
            psf->chisq = -1.0;
       }

       /* clean */
       phPsfRegDel(psfreg);      
       phCellProfDel(cprof);   
       phObject1Del(PSFobject);
  
 
   return(psf);
}   



RET_CODE
phMeasurePsf(PSF_BASIS *KLpsf,	        /* KL PSF for this frame and color */
          const CHAIN **winglist,	/* Chains to hold star1s with wings */
          const CHAIN **wingmasks,	/* masks from region->mask for wings */
	  const CHAIN *cframes,		/* I: calib1byframes */
          CHAIN **frameinfolist,        /* info about PSF stars for each frame */
	  float gain,		        /* gain for this color */
	  float dark_var,		/* dark variances for this color */
	  int ncolor,			/* number of winglists */
	  int icolor,			/* index of this filter */
          float meansky,                /* mean sky for this frame and filter */
          float meanskyerr,             /* mean sky error for this frame and filter */
          float err_min,                /* min. err. for composite prof. */
          float err_max,                /* max. err. for composite prof. */ 
          int frame,                    /* this frame (zero indexed) */
          int nrow,                     /* number of pixel rows per frame */
          int ncol,                     /* number of pixel cols per frame */
          int wing_options)             /* what to do w/o wing stars */
{
    CELL_PROF *cprof_bar = NULL;	/* CELL_PROF for reconstructed PSF at the frame center */
    COMP_PROFILE *comp_prof = NULL;	/* composite profile for PS and wings*/
    float profmin=0.0, chisq;
     
    int l;
    int medflg = 0;   /* use median profiles for composite profile */

    int  fit_g2, fit_p, fit_sigmap, fit_beta;
    PSF_REG *centerPSF;                 /* recontructed PSF at the frame center */
    REGION *centred_16 = NULL;		/* centred copy of pstamp */
    REGION *centred_32 = NULL;		/* S32 copy of centred_16 */
    CALIB1BYFRAME *cal = NULL;
    CALIB1BYFRAME *prevcal = NULL;
    STAR1 *wing = NULL; /* set to NULL to pacify compilers */
    DGPSF *psf, *initparam, *wingpsf, *psf2G;
    int I0;                             /* scaling factor for comp_prof */
    int wingOK;                         /* are wing stars OK? */ 
    float sig, sig_soft, sigma_2;       /* aux. params for error softening */ 
    const char *filter;			/* filter letter code */  
    FRAME_INFORMATION *frame_info = NULL;   /* set to NULL to pacify compilers */
    int nwings;
    float PSFwidth, neff_psf;
    OBJMASK *PSFmask = NULL, *mask = NULL;
    OBJECT1 *PSFobject = NULL;
    float ap_corr;   /* aperture correction at this point */
    DGPSF *psf_loc = NULL;
    float row_center = nrow/2.0,  col_center = ncol/2.0;	
    float sigfac, wing_slope;
    int ngoodwings = 0; 

#if DUMP_PROFILE
    FILE *fp, *fp2, *fp3, *fp4; 
#endif 

    shAssert(ncolor > 0);
    shAssert(icolor >= 0 && icolor < ncolor);
    if(winglist != NULL) {
       shAssert(winglist[icolor] != NULL);
       shAssert(winglist[icolor]->type == shTypeGetFromName("STAR1"));
    }
    shAssert(cframes != NULL);
    shAssert(cframes->type == shTypeGetFromName("CALIB1BYFRAME"));

    cal = shChainElementGetByPos(cframes, frame);
    psf = cal->calib[icolor]->psf = phDgpsfNew(); 	
    filter = cal->calib[icolor]->filter;
    frame_info = shChainElementGetByPos(frameinfolist[icolor], frame);
    nwings = (winglist == NULL) ? 0 : shChainSize(winglist[icolor]); 

#if DUMP_PROFILE
    /* open files for dumping */
      fp = fopen("best_fits.dat","w");  
      fp2 = fopen("comp_prof.dat","w"); 
      fp3 = fopen("wing_slope.dat","w"); 
      fp4 = fopen("wing_prof.dat","w");
      shAssert(fp != NULL && fp2 != NULL && fp3 != NULL && fp4 != NULL);
      if (nwings > 0) {
          dump_wing_slope(fp3,filter,winglist[icolor],medflg);
          dump_wing_prof(fp4,filter,winglist[icolor],medflg);
      } 
#endif 
   
    if (cprof_bar == NULL) {
        cprof_bar = phCellProfNew(NCELL);
    }
    if (PSFobject == NULL) {
        PSFobject = phObject1New();
    }
    if (psf_loc == NULL) {
        psf_loc = phDgpsfNew();        
    }


    /* reconstruct the PSF at the frame center */
    centerPSF = phPsfKLReconstruct(KLpsf, nrow/2, ncol/2, TYPE_PIX);          

    /* get CELL_PROF for the reconstructed PSF at the frame center */
    if (profileFromPSFReg(centerPSF,-1,meansky,meanskyerr,gain,
				 dark_var, cprof_bar, PSFobject) < 0) {
          shError("phMeasurePsf: cannot get  center PSF CELL_PROF for frame %d, filter %s.",
                        frame, icolor);
    }
    cal->calib[icolor]->psf->psfCounts = centerPSF->counts;
    phPsfRegDel(centerPSF);      

    /* get aperture correction at the frame center */
    phDgpsfFitFromCellprof(psf_loc, cprof_bar);/* XXX We do this many times!! */
    cal->calib[icolor]->psf->sigma1_2G = psf_loc->sigma1_2G;
    ap_corr = phPsfSetAtPoint(icolor, KLpsf, row_center, col_center,
			      cal->calib[icolor]->psf->sigma1_2G, NULL);
    frame_info->ap_corr = ap_corr;
    phDgpsfDel(psf_loc);

    /******************************/
    /* composite profile of PS stars and wing stars */
    comp_prof = cal->calib[icolor]->prof;
    /* minimum composite profile value to consider */
    profmin = 3 * meanskyerr;

    /* add the KL PSF to the composite profile */ 
    if (PSFmask == NULL) {
        PSFmask = phObjmaskNew(0);
    }  
    phCompositeProfileAdd(comp_prof,PSFobject,PSFmask,0,profmin); 
    phObject1Del(PSFobject);
    phObjmaskDel(PSFmask);


    /*********************************************/
           /********* wing part ***********/
        
           /* add wing stars to the composite profile */ 
           if (nwings > 0) {
	      for (l = 0; l < nwings; l++) {
	         mask = shChainElementGetByPos(wingmasks[icolor], l);
                 wing = shChainElementGetByPos(winglist[icolor],l);
                 wing_slope = get_wing_slope(wing,medflg);
                 /* this should not be hard coded (ZI) XXX */ 
                 if (wing_slope > 2.0 && wing_slope < 5.0) {
                   { float auxCounts = wing->object->psfCounts;
                     wing->object->psfCounts = wing->apCounts;                  
                     if (phCompositeProfileAdd(comp_prof,wing->object,mask,1,profmin))
		       ngoodwings += 1;
                     wing->object->psfCounts = auxCounts; 
                   }
                 } else {
                /* This is for testing, should be removed XXX   
                   shError("phMeasurePsf, filter %s, frame %d: wing %d: beta=%6.2f, N= %d",
                              filter,frame,l,wing_slope,wing->object->nprof);
		 */
                 }          
	      } 
              /* here we added nwings stars which will dominate the fit */
              /* since the KL PSF comes in a single profile; to account for */
              /* this decrease its error bars by a factor sqrt(nwings-1) */
              if (ngoodwings > 1) {
                 sigfac = sqrt(ngoodwings);
                 for (l = 0; l < comp_prof->profs[0].nprof; l++) {
                    comp_prof->profs[0].sig[l] /= sigfac;
                 }    
              }
	      /* testing, should be removed XXX 
                 shError("phMeasurePsf, filter %s, frame %d: ngoodwings = %d",
                          filter,frame,ngoodwings);
	       */
	   }

	   /* find composite profile with all stars */
	   phCompositeProfileFind(comp_prof,medflg);
           if (comp_prof->prof_is_set != 1) {
	      shFatal("phMeasurePsf %d:"
		      " filter %s, frame %d: Cannot find composite profile!",
		      __LINE__, filter, frame);
           }

           /* copy composite profile for QA purposes */
           frame_info->Nprof = comp_prof->nprof;
           for(l = 0; l < comp_prof->nprof; l++) {
               frame_info->ptarr[l] = comp_prof->med[l];
           }      
           frame_info->pt1 = comp_prof->med[3];
           if (comp_prof->nprof > 5) {
               frame_info->pt2 = comp_prof->med[5];
           } else {
               frame_info->pt2 = 0.0; 
           }
           frame_info->pt3 = comp_prof->med[comp_prof->nprof-1];


           /* soften errors in composite profile */
	   comp_prof->sig[0] = comp_prof->sig[1];
	   I0 = 1000;   /* arbitrary scaling factor */	   
	   for(l = 0;l < comp_prof->nprof;l++) {
	      comp_prof->mean[l] *= I0;
	      comp_prof->med[l]  *= I0;
	      comp_prof->sig[l]  *= I0;
              /* softening parameter for sigmas: we do not want sigmas
              to range from ~0.1% to ~40% -> soften with err_min and put
              the error upper limit to err_max (fractional errors)  */
              sig = comp_prof->sig[l]; 
              /* soften */
              sig_soft = err_min * comp_prof->med[l];
              sigma_2 =  sig_soft * sig_soft + sig * sig;
              comp_prof->sig[l] = sqrt(sigma_2);
              /* upper limit */
              if (err_max*comp_prof->med[l] < comp_prof->sig[l]) {
                  comp_prof->sig[l] = err_max * comp_prof->med[l];  
              }
	   }                   


	   /***************************
            * find  the best-fit DGPSF
            ***************************/

	   /* set up the initial values of fitted parameters*/
	   initparam = phDgpsfNew();
	   initparam->sigmax1 = initparam->sigmay1 = 1.05; 
	   initparam->b = 0.1; 
	   initparam->sigmax2 = initparam->sigmay2 = 2.55;
	   initparam->p0 = 0.001;
	   initparam->beta = 3.0; initparam->sigmap = 5.0; 
   
	   /* which best fit parameters to fit*/
           fit_g2 = 1;          /* fit a second Gaussian component */
           fit_p = 1;	        /* fit the strength of the power-law component */
           fit_sigmap = 1;      /* fit "sigma" for power law */
           fit_beta = 0;  /* silence compiler */

           /* select the fitting procedure for beta */
           wingOK = 0;
	   if(ngoodwings > 0) {
              /* we have some wing stars */
	      fit_beta = 1;     /* fit index of power law */
              /* it may happen that a wing star is not bright enough in u filter, 
                 i.e. there are no good points to determine wing part, in which 
                 case we should not fit beta. Thus, we will require that at least 
                 one wing star has at least 1 radial data point more than the
                 brightest star. */
	      assert(ngoodwings <= comp_prof->n);
              for (l = 0; l < ngoodwings; l++) {
                   if (comp_prof->profs[comp_prof->n - 1 - l].nprof > 
                                             comp_prof->profs[0].nprof) {
                       wingOK = 1;
                   }
              }
              if (!wingOK) {
                  fit_beta = 0;     /* do not fit power law slope */
                  fit_sigmap = 0;
                  initparam->p0 = 0.00001;
                  if (ngoodwings > 1) {
                      if (frame == 0) shError("phMeasurePsf, filter %s: Although there are %d good wing stars,\n"
                           "none is bright enough to constrain the power-law region.",filter,ngoodwings);
                  } else {
                      if (frame == 0) shError("phMeasurePsf, filter %s: There is a good wing star, \n"
                           "but it is not bright enough to constrain the power-law region.",filter);
                  }
              } 
	   } else {
              /* there are no good wing stars */
              switch(wing_options) {
                case 1:
                   shFatal("phMeasurePsf, filter %s: can't proceed w/o good wing stars.",filter);     
                   break;  
                case 2:
                   if (winglist != NULL && frame == 0) {
		      shError("phMeasurePsf, filter %s: "
			      "there are no good wing stars, fitting only 2 Gaussians and p0 with beta=3.",
                            filter);
		   }
                   initparam->p0 = 0.00001; 
                   fit_beta = 0; /* do not fit the power-law index */
                   fit_sigmap = 0;      /* no need to fit "sigma" either */                   
                   break;  
                default:
                   shFatal("phMeasurePsf, filter %s: selected wing_option not supported!",filter);
                   break;   
              }      
	   }

           /* if there are not enough points do not fit beta, nor sigmap */
           if (comp_prof->nprof < 8 && wingOK == 1) {
               fit_beta = 0;        /* do not fit the power-law index */
               fit_sigmap = 0;
               initparam->p0 = 0.00001;
               shError("phMeasurePsf, filter %s, frame %d: %d < 8 points in comp. prof., setting beta=3",
                        filter,frame,comp_prof->nprof);      
           }
           /******************/
           /* do the fitting */
	   wingpsf = phStarParamsFind(comp_prof,initparam,0,medflg,fit_g2,
				      fit_p,fit_sigmap,fit_beta,&chisq); 
           /* check the results */
	   if (wingpsf != NULL) {
              /* so far OK, copy and ... */
	      *psf = *wingpsf;
              /* ... check the best-fit parameters */
	      if (psf->beta < 2 || psf->beta > 10) { 
                 /* power-law wing is too flat */ 
                 switch(wing_options) {
                   case 1:
                      /* give up */
                      shFatal("phMeasurePsf, filter %s, frame %d: Power-law wing is not good (beta = %f).",
                               filter,frame,psf->beta); 
                      break;   
                   case 2:
                      /* try w/o power-law wing*/ 
                      shError("phMeasurePsf, filter %s, frame %d: "
                              "beta (%f) is not good, setting beta to 3 and refitting.",
                               filter,frame,psf->beta);
                      phDgpsfDel(wingpsf);
		      initparam->beta = 3.0; 
                      fit_beta = 0;  /* do not fit beta */ 
		      wingpsf = phStarParamsFind(comp_prof,initparam,0,medflg, 
				        fit_g2,fit_p,fit_sigmap,fit_beta,&chisq);
		      if (wingpsf == NULL || !goodPSF(wingpsf)) {                           
		          shError("phMeasurePsf, filter %s, frame %d: "
                                  "cannot fit power law to comp. prof, resorting to cprof_bar",
                                   filter,frame);
                          psf->beta = 3.0;
                          psf->sigmap = 1.0;          
                          psf->p0 = 0.0000004;
		          phDgpsfFitFromCellprof(psf, cprof_bar); 
                          /* renormalize to comp_prof scale */
                          if (cprof_bar->data[0] > 0) {
                              psf->a *= comp_prof->med[0]/cprof_bar->data[0];
                          } 
                      }
                      break;  
                   default:
                      shFatal("phMeasurePsf, filter %s, frame %d: " 
                              "selected wing_option not supported!",filter,frame); 
                      break;  
                 }
              
              } 

              if (psf->sigmap <= 0) { 
                 /* radial scale for power-law wing is not good */ 
                 switch(wing_options) {
                   case 1:
                      /* give up */
                      shFatal("phMeasurePsf, filter %s, frame %d: Power-law wing is not good (sigmap = %f).",
                               filter,frame,psf->sigmap); 
                      break;   
                   case 2:
                      /* try w/o power-law wing*/ 
                      shError("phMeasurePsf, filter %s, frame %d: "
                              "sigmap = %f, resorting to cprof_bar.",filter,frame,psf->sigmap);
                      psf->beta = 3.0;
                      psf->sigmap = 1.0;
                      psf->p0 = 0.0000003;
		      phDgpsfFitFromCellprof(psf, cprof_bar); 
                      /* renormalize to comp_prof scale */
                      if (cprof_bar->data[0] > 0) {
                          psf->a *= comp_prof->med[0]/cprof_bar->data[0];
                      } 
                      break;  
                   default:
                      shFatal("phMeasurePsf, filter %s, frame %d: " 
                              "selected wing_option not supported!",filter,frame); 
                      break;  
                 }
              
              } 

              if (psf->p0 <= 0) { 
                 /* power-law wing is not good */
                 switch(wing_options) {
                   case 1:
                      /* give up */
                      shFatal("phMeasurePsf, filter %s, frame %d: Power-law wing is not good (p0 = %f).",
                               filter,frame,psf->p0); 
                      break;   
                   case 2:
                      /* try w/o power-law wing*/ 
                      shError("phMeasurePsf, filter %s, frame %d: "
                              "p0 = %f, resorting to cprof_bar.",filter,frame,psf->p0);
                      psf->beta = 3.0;
                      psf->sigmap = 1.0;
                      psf->p0 = 0.0000002;
		      phDgpsfFitFromCellprof(psf, cprof_bar); 
                      /* renormalize to comp_prof scale */
                      if (cprof_bar->data[0] > 0) {
                          psf->a *= comp_prof->med[0]/cprof_bar->data[0];
                      } 
                      break;  
                   default:
                      shFatal("phMeasurePsf, filter %s, frame %d: " 
                              "selected wing_option not supported!",filter,frame); 
                      break;  
                 }
	      }

              if(psf->b < 0.0001 || psf->b > 100) { 
                 /* second Gaussian is not good */
                 switch(wing_options) {
                   case 1:
                      /* give up */
                      shFatal("phMeasurePsf, filter %s, frame %d: "
                              "Second Gaussian is not good (b = %f).",filter,frame,psf->b); 
                      break;   
                   case 2:
                      /* try w/o second Gaussian */ 
                      shError("phMeasurePsf, filter %s, frame %d: "
                              "b = %f, ignoring the second Gaussian",filter,frame,psf->b);
		      initparam->b = 0.00000001; 
                      fit_g2 = 0;  /* do not fit a second Gaussian component */ 
                      phDgpsfDel(wingpsf);
		      wingpsf = phStarParamsFind(comp_prof,initparam,0,medflg, 
				        fit_g2,fit_p,fit_sigmap,fit_beta,&chisq);
		      if (wingpsf == NULL || !goodPSF(wingpsf)) {                           
		         shError("phMeasurePsf, filter %s, frame %d: "
                                 "bad psf again, resorting to cprof_bar AND ignoring 2nd Gaussian",
                                  filter,frame);
		         psf->p0 = psf->b = 0.0000001; 
                         psf->beta = 3.0; psf->sigmap = 1.0;
                         psf->sigmax2 = psf->sigmay2 = 2.0;
		         phDgpsfFitFromCellprof(psf, cprof_bar); 
                         /* renormalize to comp_prof scale */
                         if (cprof_bar->data[0] > 0) {
                             psf->a *= comp_prof->med[0]/cprof_bar->data[0];
                         } 
		      } else {
		         psf = wingpsf;
		      }
                      break;  
                   default:
                      shFatal("phMeasurePsf, filter %s, frame %d: "
                              "selected wing_option not supported!",filter,frame); 
                      break;  
                 }
	      }
	   } else {
              /* something is wrong (phStarParamsFind returned NULL) */
              switch(wing_options) {
                case 1:
                   /* give up */
                   shFatal("phMeasurePsf, filter %s, frame %d: phStarParamsFind returns NULL.",
                            filter,frame);  
                   break;       
                case 2:
                   /* try w/o fitting sigmap */
                   shError("phMeasurePsf, filter %s, frame %d: phStarParamsFind returns NULL, "
		           "trying w/o fitting sigmap (set to 5).",filter,frame);
                   fit_sigmap = 0; initparam->sigmap = 5.0;
 	           wingpsf = phStarParamsFind(comp_prof,initparam,0,medflg,fit_g2,
				      fit_p,fit_sigmap,fit_beta,&chisq); 
    	           if (wingpsf == NULL || !goodPSF(wingpsf)) {
                       /* try to fit Gaussians only */
                       shError("phMeasurePsf, filter %s, frame %d: fit still not good, resorting to cprof_bar"
		                ,filter,frame);
	               psf->p0 = 0.0000001; psf->beta = 3.0; psf->sigmap = 2.0; 
	               phDgpsfFitFromCellprof(psf, cprof_bar);
                       /* renormalize to comp_prof scale */
                       if (cprof_bar->data[0] > 0) {
                           psf->a *= comp_prof->med[0]/cprof_bar->data[0];
                       } 
                   } else {
	               *psf = *wingpsf;
                   }
                   break;                     
                default:
                   shFatal("phMeasurePsf, filter %s, frame %d: selected wing_option not supported!",
                            filter,frame); 
                   break;  
              }            
	   }   

           /* final sanity check on PSF */
           if (!goodPSF(psf)) { 
               /* forget fancy stuff and force a 2-gaussian fit */
	       psf->p0 = 0.0000001; psf->beta = 3.0; psf->sigmap = 5.0; 
               shError("phMeasurePsf, filter %s, frame %d: final sanity check on PSF fails, "
		           "attempting to fit cprof_bar (w/o wing stars)",filter,frame);
	       phDgpsfFitFromCellprof(psf, cprof_bar);
               /* renormalize to comp_prof scale */
               if (cprof_bar->data[0] > 0) {
                    psf->a *= comp_prof->med[0]/cprof_bar->data[0];
               }                       

               if (!goodPSF(psf)) { 
                    shError("phMeasurePsf: ERROR ERROR ERROR!!!\n"); 
                    shError("  --> filter %s, frame %d: all attempts to fit PSF failed, "
		           "applying default PSF (Kolmogoroff with sigma1 = 1.0)",filter,frame);
                    psf->a = 1.0;
	            psf->sigmax1 = psf->sigmay1 = 1.0; 
                    psf->b = 0.1; 
	            psf->sigmax2 = psf->sigmay2 = 2.0; 
	            psf->p0 = 0.000001; 
                    psf->beta = 3.0; psf->sigmap = 5.0;
               }
           }    

#if IGNORE_COMPPROF     
           phDgpsfFitFromCellprof(psf, cprof_bar); 
           psf->p0 = 0.0000;
           psf->a *= I0;
#endif 
           /* assert that final PSF parameters make sense */           
           shAssert(psf->a > 0 && psf->a == psf->a);
           shAssert(psf->sigmax1 > 0 && psf->sigmax1 == psf->sigmax1);
           shAssert(psf->sigmax2 > 0 && psf->sigmax2 == psf->sigmax2);
           shAssert(psf->sigmax2 >= psf->sigmax1);
           shAssert(psf->b >= 0 && psf->b == psf->b);
           shAssert(psf->p0 >= 0 && psf->p0 == psf->p0);
           shAssert(psf->sigmap > 0 && psf->sigmap == psf->sigmap);
           shAssert(psf->beta > 2 && psf->beta == psf->beta);
           /* copy final PSF parameters to CALIB1 and frame_info */
           *cal->calib[icolor]->psf = *psf;
           frame_info->sig1PSF = psf->sigmax1;
           frame_info->sig2PSF = psf->sigmax2; 
           phDgpsfDel(initparam); 

           if (wingpsf != NULL) phDgpsfDel(wingpsf);

           /******************************/
           /* to find effective seeing we need a 2-Gaussian fit to PSF */
           psf2G = phDgpsfNew();          
           phDgpsfFitFromCellprof(psf2G, cprof_bar);
           if (!goodPSF(psf2G)) {
               shError("phMeasurePsf: could not fit 2 Gaussians to adopted PSF"
		       " for filter %s, frame %d.\n", filter, frame);
               if (frame > 0) {
		   /* take value from the previous frame */
                   shError("   taking 2G values from previous frame.");                   
                   prevcal = shChainElementGetByPos(cframes, frame-1);
                   psf2G->sigma1_2G = prevcal->calib[icolor]->psf->sigma1_2G;
                   psf2G->sigma2_2G = prevcal->calib[icolor]->psf->sigma2_2G;
                   psf2G->b_2G = prevcal->calib[icolor]->psf->b_2G;
               } else {
                   shError("   this is the first frame, taking 2G default values.");        
                   psf2G->sigma1_2G = 1.5;	/* XXX */
                   psf2G->sigma2_2G = 10.0;
                   psf2G->b_2G = 0;
               }
           }
           neff_psf = phPsfCountsSetupFromDgpsf(icolor, psf2G, 1);
           /* save 2-Gaussian fit parameters */
           cal->calib[icolor]->psf->sigma1_2G = psf2G->sigma1_2G;
           cal->calib[icolor]->psf->sigma2_2G = psf2G->sigma2_2G;
           cal->calib[icolor]->psf->b_2G = psf2G->b_2G;
           /* calculate seeing => effective FWHM for this frame's PSF */  
           /* 0.663 for 2.35/sqrt(4*Pi), 0.40 for pixel size in arcsec */  
           /* this hard coding should be OK since this is only a QA quantity */
           /* (and the pixel size and value of Pi are not likely to change) */     
           PSFwidth = 0.663 * 0.400 * sqrt(neff_psf);
           cal->calib[icolor]->psf->width = frame_info->PSFwidth = PSFwidth; 
           phDgpsfDel(psf2G);  

           /* for QA plotting */
           fill_psf_fit(frame_info, comp_prof, psf, 0, medflg);

           /* normalize PSF to 1 */
           cal->calib[icolor]->psf->a = 1.0;


#if DUMP_PROFILE
           if (goodPSF(psf)) {
               dump_psf_fit(fp,frame, comp_prof, psf, 0, 0);
           }
           dump_comp_prof(frame, comp_prof, fp2);	
  
           fclose(fp); fclose(fp2); fclose(fp3); fclose(fp4);
#endif 

   
       /* cleanup */
       phCellProfDel(cprof_bar);
       shRegDel(centred_16); shRegDel(centred_32); /* OK even if NULL */

      return(SH_SUCCESS);

}

 

 

/*****************************************************************************/
/*
 * fit the DGPSF parameters (sigma[xy][12] and b) from a CELL_PROF
 *
 * First a routine for calculating the residuals of a given fit to double
 * gaussian psf:
 *
 *                                x^2                 x^2
 *  double-gaussian = A [ exp(- --------) + b*exp(- --------) ]
 *                               2*s1^2              2*s2^2
 * 
 * params[0] = b
 * params[1] = s1
 * params[2] = s2
 * params[3] = A
 *
 * A seeing-convolved Gaussian isn't quite Gaussian, but it's a pretty
 * good approximation to take N(sigma^2) --> N(sigma^2 + 1/12), i.e. the
 * result of naively adding the variance of the PSF to that of the pixel
 * (it's 1/12 rather than 1/6 as sigma^2 is the PSF's 1-D variance)
 *
 * A still better result is achieved by correcting the resulting variance
 * based on the analytic results
 */
static double psf_amp;			/* amplitude of best-fit PSF */
static const CELL_PROF *cellprof;	/* global for fit_psf_to_cellProf */
static const float *area = NULL;	/* areas of annuli */
static const float *radii;		/* inner and outer radii of annuli */
static double
sigma_eff(double sigma)
{
   sigma = sqrt(pow(sigma,2) + 1/(float)12);
   sigma *= 1 + 0.00286/pow(sigma,3.9);

   return(sigma);
}


static void
fit_psf_to_cellProf(int ndata,		/* Number of data points */
		    int npar,		/* Number of parameters */
		    double params[],	/* Parameters to be fit */
		    double residuals[],	/* Vector of (ndata) residuals */
		    int *iflag)
{
   const float *const data = cellprof->data; /* unpack from global cellprof */
   int i, j, k;
   double model[NCELL];			/* model of PSF */
   float mod_var;			/* == model[]/sigma^2 */
   const float *const sig = cellprof->sig; /* unpack from global cellprof */
   double sr1, sr2, ss1, ss2, b;	/* parameters of PSF */
   float sum_dm, sum_mm;		/* sums of data*model and model^2 */
   float fac1, fac2;              /* multipl. factors in penalty function */
   /* for the penalty function when fitting Gaussians */
   float sig_max = 100.0;
   float sig_min = 0.01;

   
   shAssert(*iflag >= 0 && npar == 3);
   shAssert(ndata <= NCELL);

#if 1   
   ss1 = pow(sigma_eff(params[1]),2);
   ss2 = pow(sigma_eff(params[2]),2);
   b = params[0]*(pow(params[2],2)/ss2)/(pow(params[1],2)/ss1);
#else
   /* avoid sigma=0 */
   ss1 = pow(fabs(params[1])+sig_min,2);
   ss2 = pow(fabs(params[2])+sig_min,2);
     b = params[0];
#endif

/*
 * calculate model. The CELL_PROF has 1 element for the central cell, then
 * NSEC/2 for each outer annulus.
 */
   for(i = j = 0; i < 1 + (int)((ndata - 1)/(NSEC/2)); i++) {
      sr1 = pow(radii[i],2);
      sr2 = pow(radii[i + 1],2);
      model[j] = 2*M_PI*(  ss1*(exp(-sr1/(2*ss1)) - exp(-sr2/(2*ss1))) + 
			 b*ss2*(exp(-sr1/(2*ss2)) - exp(-sr2/(2*ss2))));
#if 0
      /* This doesn't work with -opt compiling: j is incremented, */
      /* but model is not divided by area */
      model[j++] /= area[i]; 
#else
      model[j] /= area[i]; 
      j++;
#endif   

      if(i > 0) {			/* we need a total of NSEC/2 copies */
	 int j0 = j;
	 for(k = 0;k < NSEC/2 - 1;k++) {
	    model[j++] = model[j0 - 1];
	 }
      }
   }
/*
 * calculate best-fitting amplitude
 */
   mod_var = model[0]/(sig[0]*sig[0]);
   sum_dm = data[0]*mod_var;
   sum_mm = model[0]*mod_var;

   for(i = 1;i < ndata;i++) {
      if(sig[i] != 0.0) {
	 mod_var = model[i]/(sig[i]*sig[i]);
	 sum_dm += data[i]*mod_var;
	 sum_mm += model[i]*mod_var;
      }
   }
#if 1					/* debug the assertion failure */
   if(sum_mm > 0.0) {
      ;
   } else {
      volatile int j;
      
      fprintf(stderr,"sum_mm = %g\n", sum_mm);
      fprintf(stderr,"params =");
      for(j = 0; j < npar; j++) {
	 fprintf(stderr, " %g", params[j]);
      }
      fprintf(stderr,"\n");

      fprintf(stderr,"i model data sig\n");
      for(j = 0;j < ndata;j++) {
	 fprintf(stderr,"%d %g %g %g\n", j, model[j], data[j], sig[j]);
      }
      fflush(stderr);

      j = *(int *)0x1;			/* generate core dump */
   }
#endif
   shAssert(sum_mm > 0.0);
   
   psf_amp = sum_dm/sum_mm;
   
   for(i = 0;i < ndata;i++) {
      model[i] *= psf_amp;
   }
/*
 * and calculate residuals
 */
   /* penalty function to prevent extemely small/large sigmas */
   fac1 = 1 + pow(sig_min,2)/ss1 + ss1 / pow(sig_max,2);
   fac2 = 1 + pow(sig_min,2)/ss2 + ss2/  pow(sig_max,2);
   for(i = 0; i < ndata; i++) {
      if(sig[i] == 0.0) {
	 residuals[i] = 0.0;
      } else {
	 residuals[i] = (data[i] - model[i])/sig[i];
      }
      /* penalty function */
#if 1
      residuals[i] *= fac1 * fac2; 
#endif
   }
}

int
phDgpsfFitFromCellprof(DGPSF *psf,
			const CELL_PROF *cprof)
{
   int nfev;				/* number of function evaluation */
   const int nfit = 4;			/* number of fitted params;
					   3 fitted by LM + amplitude */
   double norm=-1e10;			/* returned norm of residual vector */
   int ncell;				/* number of profile points */
   double psfpars[3];			/* fitted parameters */
   int ret;				/* return code from LM minimiser */
   double tol = 1e-6, sumsq_tol;	/* permitted tolerances in LM fit */

   shAssert(cprof != NULL)
   shAssert(cprof->is_median == 0); /* we want the mean profile */

   if(area == NULL) {
      const CELL_STATS *cstats = phProfileGeometry();
      area = cstats->area;
      radii = cstats->radii;
   }

   cellprof = cprof;			/* copy to global */
   ncell = cprof->ncell;

   if(ncell <= nfit + 1) {
      return(-1);
   }
/*
 * time for the fit
 */
   psfpars[0] = 0.1;			/* b */
   psfpars[1] = 1.0;			/* sigma1 */
   psfpars[2] = 3.0;			/* sigma2 */

   sumsq_tol = 0;
   ret = phLMMinimise(fit_psf_to_cellProf, ncell, nfit - 1, psfpars,
		      &norm, &nfev, tol, sumsq_tol, 0, NULL);

   if (ret < 0) {
       shFatal("phDgpsfFitFromCellprof: bogus return from phLMMinimise");
   } else if (ret == 0) {
       shError("phDgpsfFitFromCellprof: bad inputs to phLMMinimise");
       return(-1); 
   } else if (ret & ~07) {		/* couldn't converge, but probably OK*/
      ;
   }
/*
 * Minimizer sometimes returns negative sigmas, but as we only care about
 * the variances, take absolute value
 */
   psfpars[1] = fabs(psfpars[1]);
   psfpars[2] = fabs(psfpars[2]);
/*
 * For convenience, let's guarantee that sigma1 is smaller
 */
   if(psfpars[1] > psfpars[2]) {
      float tmp = psfpars[1];
      psfpars[1] = psfpars[2];
      psfpars[2] = tmp;
      if (psfpars[0] != 0.0) {
          psfpars[0] = 1.0 / psfpars[0]; 
      }
   }

/* take care of some fitting degeneracies */
   /* Gauss 1 is delta function */
   if (psfpars[1] < 1.0e-4) {
       psfpars[1] = psfpars[2];
       psfpars[0] = 1.0e-6;
   }
   /* Gauss 2 is delta function */
   if (psfpars[2] < 1.0e-4) {
       psfpars[2] = psfpars[1];
       psfpars[0] = 1.0e-6;
   }
   /* Gaussians are same => 1 Gaussian */
   if (fabs(psfpars[1]-psfpars[2])/(psfpars[1]+psfpars[2]) < 1.0e-4) {
       psfpars[0] = 1.0e-6;
   }

/*
 * pack the PSF
 */
   psf->a = psf_amp;
   psf->sigma1_2G = psf->sigmax1 = psf->sigmay1 = psfpars[1];
   psf->sigma2_2G = psf->sigmax2 = psf->sigmay2 = psfpars[2];
   psf->b_2G = psf->b = psfpars[0];

#if 0
   psf->chisq = pow(norm,2)/(ncell-nfit);
#else
   psf->chisq = phChisqProb(pow(norm,2), ncell - nfit, 0.0);
#endif

   return(0);
}

 
#if DUMP_PROFILE
/* print best-fit parameters, data from comp_prof and the best fit */
enum {
  I0_1 = 0,			/* narrow central intensity */
  I0_2,				/* wide central intensity */
  I0_P,				/* power law central intensity */
  SIG_1,			/* narrow sigma */
  SIG_2,			/* wide sigma */
  SIG_P,			/* "sigma" of power law */
  BETA,				/* power law index */
  NPAR			        /* number of parameters */
};

static void           
dump_psf_fit(FILE *fp,                /* file for dumping */
             const int j,             /* frame number */
             const COMP_PROFILE *comp_prof, /* composite profile */
	     const DGPSF *psf,              /* best-fit parameters */
             float profmin,                 /* minimum profile value to consider */
	     int medflg)		    /* use median not mean profile? */	
{
   const float *r = NULL;		/* radii for annuli */
   int nprof, i0, i;
   int m;				/* number of data points, same as nprof */
   double prof[NANN];			/* profile */
   double sig[NANN];			/* s.d. of prof[] */
   double fit[NANN];			/* the best fit -> sum of contributions: */
   double fit_g1[NANN];			/* the best fit, first Gaussian */
   double fit_g2[NANN];			/* the best fit, second Gaussian */
   double wing[NANN];			/* power-law-wing values in annuli */
   double p[NPAR];			/* best fit parameters */
   double resid[NANN], area[NANN], ot[NANN];
   float r_ms;			/* mean-square radius for annulus */

   shAssert(psf != NULL);

   if(r == NULL) {
      r = phProfileGeometry()->radii;
      for(i = 0;i < NANN;i++) {
	 area[i] = M_PI*(pow(r[i+1],2) - pow(r[i],2));
      }
   }

   /* print out best-fit parameters */
   fprintf(fp," \n");
   fprintf(fp,"============================================================ \n");
   fprintf(fp,"  Frame: %d, a: %f, width: %f\n", j, psf->a, psf->width); 
   fprintf(fp,"  sigmax1: %f, b: %f, sigmax2: %f\n", psf->sigmax1, psf->b, psf->sigmax2); 
   fprintf(fp," p0: %f, beta: %f, sigmap: %f\n \n", psf->p0, psf->beta, psf->sigmap); 

   /* select proper profile */
   nprof =comp_prof->nprof;
   for(i = 0;i < nprof;i++) {
       prof[i] = medflg ? comp_prof->med[i] : comp_prof->mean[i];
       ot[i] = medflg ? comp_prof->mean[i] : comp_prof->med[i] ;
       sig[i] = comp_prof->sig[i];
   }

   m = nprof;
   
   /* assume all points are good */
   i0 = 0; 
 /*
 * And only consider those parts of the profile with a level of
 * more than profmin counts, so as to reduce our sensitivity to the
 * not-very-well-determined value of the sky
 */
   for(i = 0;i < nprof;i++) {
      if(prof[i] < profmin) {
	 nprof = i;
	 break;
      }
   }
/*
 * initial values for parameters
 */
   p[I0_1] = psf->a / (1 + psf->b + psf->p0); /* narrow peak */
   p[I0_2] = p[I0_1] * psf->b;		      /* wide peak */
   p[I0_P] = p[I0_1] * psf->p0;		/* peak value of powerlaw wing */
   p[SIG_1] = (psf->sigmax1 + psf->sigmay1)/2; /* narrow width */
   p[SIG_2] = (psf->sigmax2 + psf->sigmay2)/2; /* wide width */
   p[SIG_P] = psf->sigmap;		/* "sigma" for powerlaw */
   p[BETA] = psf->beta;			/* powerlaw index */

   if(p[BETA] - 2 == 0) {
      p[BETA] = 2.0001;			/* avoid divide by zero */
   }


   fprintf(fp,"   p[I0_1]    p[I0_2]    p[I0_P]   p[SIG_1]   p[SIG_2]   p[SIG_P]   p[BETA]\n");
   fprintf(fp," %9.6f %9.6f %9.6f %9.6f %9.6f %9.6f %9.6f\n",p[I0_1],p[I0_2],p[I0_P],p[SIG_1],p[SIG_2],p[SIG_P],p[BETA]);   



/*
 * find mean value in annuli for core and wings
 */
   if (medflg) {
      for(i = 0;i < NANN;i++) {
	 r_ms = (pow(r[i],2) + pow(r[i + 1],2))/2;  
         fit_g1[i] = p[I0_1]*exp(-0.5*r_ms*pow(1/p[SIG_1],2));
         fit_g2[i] = p[I0_2]*exp(-0.5*r_ms*pow(1/p[SIG_2],2)); 
	 wing[i] = p[I0_P]*pow(1 + r_ms / pow(p[SIG_P],2)/p[BETA],-p[BETA]/2);  

       }
   } else {
      for(i = 0;i < NANN;i++) {
         fit_g1[i] = 2*M_PI*p[I0_1]*pow(p[SIG_1],2)*(1 - exp(-0.5*pow(r[i+1]/p[SIG_1],2)));
         fit_g2[i] = 2*M_PI*p[I0_2]*pow(p[SIG_2],2)*(1 - exp(-0.5*pow(r[i+1]/p[SIG_2],2))); 
	 wing[i] = 2*p[I0_P]*M_PI*pow(p[SIG_P],2)/(p[BETA] - 2)*
	           (1 - pow(1 + pow(r[i + 1]/p[SIG_P],2)/p[BETA],1 - p[BETA]/2));
      }
      
      for(i = NANN - 1;i > 0;i--) {
	 wing[i] = (wing[i] - wing[i - 1])/area[i];
         fit_g1[i] = (fit_g1[i] - fit_g1[i - 1])/area[i];
         fit_g2[i] = (fit_g2[i] - fit_g2[i - 1])/area[i];   
      }
   }

   fprintf(fp,"-------------------------------------------------------\n");
   fprintf(fp,"   i     radius  comp_prof   best fit    sigma     residual  other prof  Gauss1     Gauss2    Power-law\n");
   for(i = 0;i < m;i++) {
      fit[i] = fit_g1[i+i0] + fit_g2[i+i0] + wing[i+i0];
      resid[i] = (prof[i+i0] - fit[i])/sig[i+i0];
      fprintf(fp," %3d %10.3f %10.3e %10.3e %10.3e %10.3e %10.3e %10.3e %10.3e %10.3e\n",i, r[i], prof[i+i0], fit[i], sig[i+i0], resid[i], ot[i],fit_g1[i+i0],fit_g2[i+i0],wing[i+i0]);
   }
   fprintf(fp,"-------------------------------------------------------\n");

}


static void           
fill_psf_fit(FRAME_INFORMATION *frame_info, /* holding strucuture */
             const COMP_PROFILE *comp_prof, /* composite profile */
	     const DGPSF *psf,              /* best-fit parameters */
             float profmin,                 /* minimum profile value to consider */
	     int medflg)		    /* use median not mean profile? */	
{
   const float *r = NULL;		/* radii for annuli */
   int nprof, i0, i;
   int m;				/* number of data points, same as nprof */
   double prof[NANN];			/* profile */
   double fit[NANN];			/* the best fit -> sum of contributions: */
   double fit_g1[NANN];			/* the best fit, first Gaussian */
   double fit_g2[NANN];			/* the best fit, second Gaussian */
   double wing[NANN];			/* power-law-wing values in annuli */
   double p[NPAR];			/* best fit parameters */
   double area[NANN], ot[NANN], sig[NANN];
   float r_ms;			/* mean-square radius for annulus */

   shAssert(psf != NULL);

   if(r == NULL) {
      r = phProfileGeometry()->radii;
      for(i = 0;i < NANN;i++) {
	 area[i] = M_PI*(pow(r[i+1],2) - pow(r[i],2));
      }
   }

   /* select proper profile */
   nprof =comp_prof->nprof;
   for(i = 0;i < nprof;i++) {
       prof[i] = medflg ? comp_prof->med[i] : comp_prof->mean[i];
       ot[i] = medflg ? comp_prof->mean[i] : comp_prof->med[i] ;
       sig[i] = comp_prof->sig[i];
   }
   /* silence compiler */
   r_ms = ot[i] * sig[i];

   m = nprof;
   
   /* assume all points are good */
   i0 = 0; 
 /*
 * And only consider those parts of the profile with a level of
 * more than profmin counts, so as to reduce our sensitivity to the
 * not-very-well-determined value of the sky
 */
   for(i = 0;i < nprof;i++) {
      if(prof[i] < profmin) {
	 nprof = i;
	 break;
      }
   }
/*
 * initial values for parameters
 */
   p[I0_1] = psf->a / (1 + psf->b + psf->p0); /* narrow peak */
   p[I0_2] = p[I0_1] * psf->b;		      /* wide peak */
   p[I0_P] = p[I0_1] * psf->p0;		/* peak value of powerlaw wing */
   p[SIG_1] = (psf->sigmax1 + psf->sigmay1)/2; /* narrow width */
   p[SIG_2] = (psf->sigmax2 + psf->sigmay2)/2; /* wide width */
   p[SIG_P] = psf->sigmap;		/* "sigma" for powerlaw */
   p[BETA] = psf->beta;			/* powerlaw index */

   if(p[BETA] - 2 == 0) {
      p[BETA] = 2.0001;			/* avoid divide by zero */
   }



/*
 * find mean value in annuli for core and wings
 */
   if (medflg) {
      for(i = 0;i < NANN;i++) {
	 r_ms = (pow(r[i],2) + pow(r[i + 1],2))/2;  
         fit_g1[i] = p[I0_1]*exp(-0.5*r_ms*pow(1/p[SIG_1],2));
         fit_g2[i] = p[I0_2]*exp(-0.5*r_ms*pow(1/p[SIG_2],2)); 
	 wing[i] = p[I0_P]*pow(1 + r_ms / pow(p[SIG_P],2)/p[BETA],-p[BETA]/2);  

       }
   } else {
      for(i = 0;i < NANN;i++) {
         fit_g1[i] = 2*M_PI*p[I0_1]*pow(p[SIG_1],2)*(1 - exp(-0.5*pow(r[i+1]/p[SIG_1],2)));
         fit_g2[i] = 2*M_PI*p[I0_2]*pow(p[SIG_2],2)*(1 - exp(-0.5*pow(r[i+1]/p[SIG_2],2))); 
	 wing[i] = 2*p[I0_P]*M_PI*pow(p[SIG_P],2)/(p[BETA] - 2)*
	           (1 - pow(1 + pow(r[i + 1]/p[SIG_P],2)/p[BETA],1 - p[BETA]/2));
      }
      
      for(i = NANN - 1;i > 0;i--) {
	 wing[i] = (wing[i] - wing[i - 1])/area[i];
         fit_g1[i] = (fit_g1[i] - fit_g1[i - 1])/area[i];
         fit_g2[i] = (fit_g2[i] - fit_g2[i - 1])/area[i];   
      }
   }

   for(i = 0;i < m;i++) {
      fit[i] = fit_g1[i+i0] + fit_g2[i+i0] + wing[i+i0];
      frame_info->ptarr[i] = fit[i];
   }
   frame_info->Nprof = m;

}



static void
dump_comp_prof(const int jf, const COMP_PROFILE *cprof, FILE *fp)		   
{
   int m, i, j, nprof;
   float Med, Mean, Med0, Mean0, Sigma;			 
	
   shAssert(cprof != NULL); shAssert(fp != NULL);
 
   /* numbers of profiles */
   nprof = cprof->n;
   fprintf(fp,"----------------------------------------------------\n");
   fprintf(fp," Frame %d: comp_prof has %d individual profiles.\n",jf,nprof);
   for (i = 0;i < nprof;i++) {
        /* numbers of points in this profile */           
        m = cprof->profs[i].nprof;
        fprintf(fp,"\nProfile number %d, i0:%d, alpha: %f\n",i,cprof->profs[i].i0,cprof->profs[i].alpha);
        fprintf(fp,"  i  Med/Med(0) Mean/Mean(0) Med     Mean      Sigma\n");
        for (j = 0;j < m;j++) {             
             Med = cprof->profs[i].med[j]; 
             Mean = cprof->profs[i].mean[j]; 
             Med0 = Med / cprof->profs[i].med[0]; 
             Mean0 = Mean / cprof->profs[i].mean[0]; 
             Sigma = cprof->profs[i].sig[j]; 
             fprintf(fp,"%3d %10.5f %10.5f %8.1f %8.1f %8.1f\n",j,Med0,Mean0,Med,Mean,Sigma);
        }
   }   

   if (cprof->prof_is_set != 0) {
       fprintf(fp,"\n Composite profile is set, psfCounts: %f\n",cprof->psfCounts);
       fprintf(fp,"  i  Med/Med(0) Mean/Mean(0) Med     Mean      Sigma\n");
       for (j = 0;j < cprof->nprof;j++) {             
            Med = cprof->med[j]; 
            Mean = cprof->mean[j]; 
            Med0 = Med / cprof->med[0]; 
            Mean0 = Mean / cprof->mean[0]; 
            Sigma = cprof->sig[j]; 
            fprintf(fp,"%3d %10.5f %10.5f %8.5f %8.5f %8.5f\n",j,Med0,Mean0,Med,Mean,Sigma);
       }
   } else {
       fprintf(fp," Composite profile is not set.\n");
   }

   fprintf(fp,"----------------------------------------------------\n");

}


/* return a power-law slope determined by the last two points in 
   median (or mean for !medflg) profile. If profile contains a point
   with negative count, return -99 */
static float    
get_wing_slope(STAR1 *wing, int medflg) 	 		   
{
   int j, Npts;
   float prof, prof1, prof2, r1, r2, beta;
   const float *r = NULL;		/* radii for annuli */		 
	
   shAssert(wing != NULL); 
 
   /* radii for profile extraction */
   r = phProfileGeometry()->radii;

   Npts = wing->object->nprof;
   for(j = 0; j < Npts; j++) {
      prof = wing->object->profMean[j]; 
      if(medflg) prof = wing->object->profMed[j];  
      if(prof < 0.0) {
         break;
      }
   }
   /* throw away points with negative counts */
   wing->object->nprof = Npts = j;
   if (Npts < 3) {
      return(-99);
   } 
   /* looks good, calculate the power-law index */
   if (medflg) {
      prof1 = wing->object->profMed[Npts-2]; 
      prof2 = wing->object->profMed[Npts-1]; 
   } else {
      prof1 = wing->object->profMean[Npts-2];
      prof2 = wing->object->profMean[Npts-1];
   }

   if (prof1*prof2 <= 0) {
      return(-99);
   }

   r1 = r[Npts-2];
   r2 = r[Npts-1];
   beta = log(prof1/prof2) / log(r2/r1);

   return(beta);
}






static void      
dump_wing_slope(FILE *fp, const char *filter, const CHAIN *winglist,int medflg) 	 		   
{
   int i, j, n, frame, Npts, id;
   float prof, prof1, prof2, sig1, sig2, r1, r2, beta, error;
   float sky, skyErr, apCounts;
   STAR1 *wing;	
   const float *r = NULL;		/* radii for annuli */		 
	
   shAssert(winglist != NULL); shAssert(fp != NULL);
 
   fprintf(fp,"Filter %s\n",filter);
   fprintf(fp,"  frame  id    r1      r2    prof1     prof2    N  apCounts    sky      skyErr   beta  error  \n");  

   n = shChainSize(winglist);
   r = phProfileGeometry()->radii;

   for (i = 0; i < n; i++) {
      wing = shChainElementGetByPos(winglist,i);
      Npts = wing->object->nprof;
      for(j = 0; j < Npts; j++) {
          prof = wing->object->profMean[j]; 
          if(medflg) prof = wing->object->profMed[j];  
          if(prof < 0.0) {
	        break;
          }
      }
      Npts = j;
      if (Npts > 7) {
        prof1 = wing->object->profMean[5];
        if (medflg) prof1 = wing->object->profMed[5]; 
        prof2 = wing->object->profMean[7];
        if (medflg) prof2 = wing->object->profMed[7]; 
        sig1 = wing->object->profErr[Npts-2]; 
        sig2 = wing->object->profErr[Npts-1]; 
        r1 = r[5];
        r2 = r[7];
        beta = log(prof1/prof2) / log(r2/r1);
        error = sqrt(pow(sig1/prof1,2.0)+pow(sig2/prof2,2.0)) / log(r2/r1);
        if (error > 1.0 && Npts > 3) {
	   /* error too large for the last two points, back off a bit */
           prof1 = wing->object->profMean[4];
           if (medflg) prof1 = wing->object->profMed[4]; 
           prof2 = wing->object->profMean[5];
           if (medflg) prof2 = wing->object->profMed[5]; 
           sig1 = wing->object->profErr[5]; 
           sig2 = wing->object->profErr[5]; 
           r1 = r[4];
           r2 = r[5];
           beta = log(prof1/prof2) / log(r2/r1);
           error = sqrt(pow(sig1/prof1,2.0)+pow(sig2/prof2,2.0)) / log(r2/r1);
        }
        frame = wing->frame;
        id = wing->starid;
        apCounts = wing->apCounts;
        sky = wing->object->sky;
        skyErr = wing->object->skyErr;
        fprintf(fp,"%5d %5d %6.1f  %6.1f %9.2e %9.2e %2d %9.2e %9.2e %9.2e %5.2f %5.2f\n",
                   frame,id,r1,r2,prof1,prof2,Npts,apCounts,sky,skyErr,beta,error);  
      }     
   }

   fprintf(fp,"----------------------------------------------------\n");
}

static void      
dump_wing_prof(FILE *fp, const char *filter, const CHAIN *winglist,int medflg)
{
   int i, j, n, Npts;
   float prof;
   STAR1 *wing;	
	
   shAssert(winglist != NULL); shAssert(fp != NULL);
 
   fprintf(fp,"Filter %s\n",filter);
   fprintf(fp,"  star   prof0   prof1   prof2   prof3   prof4   prof5\
   prof6   prof7   prof8    prof9   prof10\n");  

   n = shChainSize(winglist);
   for (i = 0; i < n; i++) {
      fprintf(fp,"%5d",i);
      wing = shChainElementGetByPos(winglist,i);
      Npts = wing->object->nprof;
      fprintf(fp,"%4d ",Npts);
      for(j = 0; j < 11; j++) {
	  if (j < Npts) {
             prof = wing->object->profMean[j];
          } else {
             prof = -1.0;
          }
          if (medflg) prof = wing->object->profMed[j]; 
          fprintf(fp,"%7.3e ",prof);
      }
      fprintf(fp,"\n");
   }

}



#endif 


/************************************************************ 
 *  Clip array at nsigma niter times. If symmetry = 1 use
 *  use 2*(median - q25)*IQR_TO_SIGMA to estimate sigma, 
 *  otherwise use (q75 - q25)*IQR_TO_SIGMA.
 */  
static void 
get_stats(const float *array,	        /* the array */
	  int n,			/* number of elements */
          float nsigma,		        /* how many sigma to clip at */
	  int niter,			/* no. of times to iterate clipping */
          int symmetry,                 /* which method to determine sigma */
          float *dis_sigma,             /* sigma determined according to
					   symmetry (can be NULL) */
          float q[3])                   /* 25, 50 and 75% quartiles */
{
  int i, j;
  float median;				/* median of array */
  int num_elements;                     /* number of elements after clipping*/
  int sorted;				/* is sort_array sorted? */
  float *sort_array;	                /* the array to sort*/
  float sigma;

  shAssert(array != NULL);
  shAssert(n >= 1);

  num_elements = n; 
  median = sigma = 0;			/* make compiler happy */
  sorted = 0;				/* it isn't sorted yet */

  sort_array = (float *)shMalloc(n * sizeof(float)); 
  memcpy(sort_array,array,n*sizeof(float));

/* 
  for(j = 0; j < n; j++) {   
      sort_array[j] = array[j];
  }
 */

  for(i = 0;i < niter;i++) {
      if(i > 0) {
	 for(j = num_elements - 1;j >= 0;j--) { /* clip at +nsigma */
	     if(sort_array[j] < median + nsigma*sigma) {
		break;
	     }
         }
         if(j <= 2) {
	    num_elements = 2;
            break;
         }
      }
      phFloatArrayStats(sort_array, num_elements, sorted, &q[1], &sigma, &q[0], &q[2]);
      sorted = 1;			/* sort_array is now sorted */
      /* phFloatArrayStats uses q75-q25 to get sigma, we might want 2*(median-q25) */
      if (symmetry == 1) {
          sigma = 2 * (q[1] - q[0]) * IQR_TO_SIGMA;
      }
  }
 
  if (dis_sigma != NULL ) *dis_sigma = sigma; 
  
  /*  clean up  */
  shFree(sort_array);

}





/***************
Given a chain of STAR1s, starlist, beginning and ending positions, first  
and last,array of flags showing which stars not to include, badstar, find
average CELL_PROF. For type==1 take mean profile, for type==2,3,4 median, 
for type==5 average weighted by inverse variances. For type==2 errors are
taken to be same as the spread of data in each cell, for type==3 and 4
error is evaluated as pow(sum(e_i^2/alpha),alpha/2) with alpha=1 and 2, 
respectively. For type==5 error is 1/sqrt(sum(1/err_i^2)). Note that for
type > 1 the profile and and its sigma (for each star) are scaled first,
and then the average profile is found. For type==1 first the cprof_bar 
is found and then it is scaled. There are three scaling options selected
by scaling_type. For scaling_type = 1 the profiles are scaled to 1 at the
center.  For scaling_type = 2, the profiles are scaled by 1/7 * sum of counts
in the first 7 cells. For scaling_type = 3,  the profiles are scaled by 
aperture flux.
If array oddness != NULL, fill it by average chi2 per cell 
for each star, where chi2 measures the deviation from the obtained CELL_PROF 
in units of quoted error (should be <~1 for stars compatible with the average). 
If array maxdev != NULL, fill it by maximum chi2 per cell 
for each star.
****************/  
RET_CODE
phGetCprof(const CHAIN *starlist,	/* Chain of STAR1s to be clipped */
          int first,                    /* first star on this frame */
          int last,                     /* last star on this frame */
          int *badstar,  	        /* flag showing if a star has been clipped */
          int type,                     /* 1 for mean, 2 for median */
          int scaling_type,             /* controls the profile scaling */
          CELL_PROF *cprof_bar,         /* average profile */	
          float *oddness,               /* measures deviation from the average profile */
          float *maxchi2)               /* maximum (cell) deviation from the average profile */		   
{
    STAR1 *star;
    int k, l, i, Ncell=NCELL, Nstars, Ngood = 0;
    int *n_cprof_data;			/* number of stars contributing to
					   cprof_bar in each cell */
    float *values;                      /* value in each cell */  
    float *errors;                      /* quoted error for each cell */
    float *fluxes, flux = 0;                /* flux for each star */ 
    float sig, quart[3];
    float alpha;                        /* fiddle factor */ 
    float val, err, scale;
    float chi2, chi2max, chi2sum, chi2_1, chi2_all = 0; /* aux. quant. for chi2 */


    if(type < 1 || type > 5) {
         shError("phGetCprof: type can be only 1-5");
    }

    if(scaling_type < 1 || scaling_type > 3) {
         shError("phGetCprof: scaling_type can be only 1-3");
    }

    shAssert(starlist != NULL);
    shAssert(badstar != NULL);
    shAssert(cprof_bar != NULL);
 
    /* number of stars */
    Nstars = last - first + 1; 

    /* initialize arrays */
    n_cprof_data = alloca(NCELL*sizeof(int));
    memset(cprof_bar->data,'\0',NCELL*sizeof(float));
    memset(cprof_bar->sig,'\0',NCELL*sizeof(float));
    memset(n_cprof_data,'\0',NCELL*sizeof(int));
    shAssert(cprof_bar->data[0] == 0.0 && n_cprof_data[0] == 0);

    /*** mean profile ***/
    if (type == 1) { 
       /* first add all profiles for surviving stars to cprof_bar */
       for(k = first;k <= last;k++) {
           /* skip if this star was clipped */
           if (badstar[k-first] != 0) continue;
           star = shChainElementGetByPos(starlist, k);
	   shAssert(star->cprof != NULL);
           Ncell = star->cprof->ncell;
           shAssert(NCELL >= Ncell);	   
           for(l = 0;l < Ncell;l++) {
	       cprof_bar->data[l] += star->cprof->data[l];
	       cprof_bar->sig[l] += pow(star->cprof->sig[l],2);
	       n_cprof_data[l]++;
	   }
           flux += star->apCounts;
        }  

        /* average all profiles */
	shAssert(n_cprof_data[0] > 0);
	for(l = 0;l < Ncell;l++) {
	    if(n_cprof_data[l] == 0) {
	         break;
	    }
	    cprof_bar->sig[l] = sqrt(cprof_bar->sig[l]);
	    cprof_bar->sig[l] /= n_cprof_data[l];
	    cprof_bar->data[l] /= n_cprof_data[l];
	}
	cprof_bar->ncell = Ncell;
	cprof_bar->is_median = 0;
        flux /= n_cprof_data[0]; 
        scale_cprof(scaling_type,&flux,cprof_bar);
    }

    /*** median profile ***/
    if (type > 1 && type < 5) { 

        /* arrays for a value and error in each cell */
        values = (float *)shMalloc(Nstars * sizeof(float));
        errors = (float *)shMalloc(Nstars * sizeof(float));
        /* array for PSF fluxes for each star */
        fluxes = (float *)shMalloc(Nstars * sizeof(float));
        Ncell = 0;
        /* loop over stars to get scaling fluxes */
        for(k = first;k <= last;k++) {
            /* skip if this star was clipped */
            if (badstar[k-first] != 0) continue;
            star = shChainElementGetByPos(starlist, k);
            shAssert(star->cprof != NULL);
            if (star->cprof->ncell > Ncell) {
                Ncell = star->cprof->ncell;
            }   
            /* find flux */
            flux = star->apCounts;
            shAssert(flux > 0.0);
            fluxes[k-first] = flux;
        } 
 
        /* fiddle factor for estimating error of the median (1-2) */
        alpha = 1.0;
        if (type == 4) alpha = 2.0;

        /* loop over cells */
        for(l = 0; l < Ncell; l++) { 
            i = 0; 
	    cprof_bar->sig[l] = 0.0;
            /* loop over stars */
            for(k = first;k <= last;k++) {
                /* skip if this star was clipped */
                if (badstar[k-first] != 0) continue;
                star = shChainElementGetByPos(starlist, k);
                shAssert(star->cprof != NULL);
                /* skip if this star does not have data for this cell */ 
                if (l > star->cprof->ncell) continue;
                flux = fluxes[k-first];
                scale = get_scale(scaling_type,&flux,star->cprof);
                values[i] = star->cprof->data[l]/scale;
                errors[i++] = star->cprof->sig[l]/scale;
	        cprof_bar->sig[l] += pow(star->cprof->sig[l]/scale,2/alpha);
            }
            /* find median and sigma for this cell */ 
            /* clipping at 3 sigma, 2 iterations */
            get_stats(values, i, 3.0, 2, 1, &sig, quart);    
            cprof_bar->data[l] = quart[1];
            if (type == 2) { 
               cprof_bar->sig[l] = sig;
            } else {
               cprof_bar->sig[l] = pow(cprof_bar->sig[l],alpha/2);
               cprof_bar->sig[l] /= Ncell; 
            }
        }
	cprof_bar->ncell = Ncell;
	cprof_bar->is_median = 1;
       
        shFree(values);
        shFree(fluxes);
        shFree(errors);

    }


    /*** weighted average profile ***/
    if (type == 5) { 
       /* first add all profiles for surviving stars to cprof_bar */
       for(k = first;k <= last;k++) {
           /* skip if this star was clipped */
           if (badstar[k-first] != 0) continue;
           star = shChainElementGetByPos(starlist, k);
	   shAssert(star->cprof != NULL);
           Ncell = star->cprof->ncell;
           shAssert(NCELL >= Ncell);
           flux = star->apCounts;
           scale = get_scale(scaling_type,&flux,star->cprof);	   
           for(l = 0;l < Ncell;l++) {
               val = star->cprof->data[l]/scale;
               err = star->cprof->sig[l]/scale;
               if (err > 0) {
	           cprof_bar->data[l] += val/err/err;
	           cprof_bar->sig[l] += 1/err/err;
	           n_cprof_data[l]++;
	       }
           }
        }  

        /* find the average */
	shAssert(n_cprof_data[0] > 0);
	for(l = 0;l < Ncell;l++) {
	    if(n_cprof_data[l] == 0) {
	         break;
	    }
	    cprof_bar->data[l] /= cprof_bar->sig[l] ;
	    cprof_bar->sig[l] = 1 / sqrt(cprof_bar->sig[l]);
	}
	cprof_bar->ncell = Ncell;
	cprof_bar->is_median = 0;
    }

    /* find deviations of each star from cprof_bar */
    for(k = first;k <= last;k++) {
         /* skip if this star was clipped */
         if (badstar[k-first] != 0) {
             if (oddness != NULL) oddness[k] = -1.0;
             if (maxchi2 != NULL) maxchi2[k] = -1.0;
             continue;
         } else {
             chi2sum = 0; chi2max = 0; Ngood++;     
             star = shChainElementGetByPos(starlist, k);
	     shAssert(star->cprof != NULL);
             flux = star->apCounts;
             scale = get_scale(scaling_type,&flux,star->cprof);	   
             /* calculate chi2 for each cell */	   
             for(l = 0; l < star->cprof->ncell; l++) {
                  val = star->cprof->data[l]/scale;
                  err = star->cprof->sig[l]/scale;
                  if (err > 0) {
                      chi2 = pow((val-cprof_bar->data[l])/err,2);
                      if (chi2 > chi2max) chi2max = chi2; 
	          } else {
                      chi2 = 0.0;
                  } 
                  chi2sum += chi2;                             
             } 
             /* cell averaged chi2 for this star */
             chi2_1 = chi2sum / star->cprof->ncell;
             if (oddness != NULL) oddness[k] = chi2_1;
             if (maxchi2 != NULL) maxchi2[k] = chi2max;
             /* add this contribution to overall chi2 */
             chi2_all += chi2_1; 
         }
    }
    /* normalize overall chi2 */  
    chi2_all /= Ngood; 
 
    return(SH_SUCCESS);
}


/***************
Fo type = 1 scale cprof to 1 at the center. For type = 2, scale cprof by the
1/7 * sum of counts in the first 7 cells. For type = 3, scale cprof by flux.
***************/ 
static void
scale_cprof(int type, 
            float *flux,                   /* can be NULL for type != 3 */
            CELL_PROF *cprof)    
{
    int l, Ncell; 
    float scale;

    if(type < 1 || type > 3) {
         shError("scale_cprof: type can be only 1-3");
    }

    shAssert(cprof != NULL);    
    Ncell = cprof->ncell;

    scale = get_scale(type,flux,cprof);
 
    for(l = 0;l < Ncell;l++) {
	cprof->data[l] /= scale;
	cprof->sig[l] /= scale;
    }
 
}

/***************
Get normalization constant for scaling CELL_PROF. For type = 1 the scale is 
profile value at the center. For type = 2, the scale is 1/7 * sum of counts 
in the first 7 cells. For type = 3, the scale is the given value of flux.
***************/ 
static float
get_scale(int type, 
          float *flux,                   /* can be NULL for type != 3 */
          CELL_PROF *cprof)          /* can be NULL for type == 3 */  
{
    int l, Ncell; 
    float scale=1.0;

    if(type < 1 || type > 3) {
         shError("get_scale: type can be only 1-3");
    }

    if(type != 3) {
       shAssert(cprof != NULL);    
    }

    if(type == 1) {
       scale = cprof->data[0];
    }
    if(type == 2) {
       scale = 0;
       Ncell = cprof->ncell;
       shAssert(Ncell >= 7);    
       for(l = 0;l < 7;l++) scale += cprof->data[l];
       scale /= 7;
    }    
    if(type == 3) {
       shAssert(flux != NULL);    
       scale = *flux; 
    }
    shAssert(scale > 0);  

   return scale;  
}

/************************************************************ 
 *  Copy relevant info from badstar to frame_info. To insure 
 *  that everything is the way it should be, the number of 
 *  good stars, Ngood, is passed as a control parameter.
 */  
static void 
pack_frameinfo(FRAME_INFORMATION *frame_info,  
	       int first,	        /* first star on this frame */         
	       int last,		/* last star on this frame */
               int Ngood,               /* # of stars with badstars = 0 */               
               int *badstars)           /* flags showing which stars are included */
{
  int i, Nstars, j=0, NbadCell = 0, NbadQU = 0, NbadSSb = 0;
                   
  shAssert(frame_info != NULL);
  shAssert(badstars != NULL);
  shAssert(last >=first);
  
  Nstars = last - first + 1;  

  for(i = 0; i < Nstars; i++) {
      frame_info->starflags[i] = badstars[i];
      if (badstars[i] == 0) j++;
      if (badstars[i] == frame_info->FbadCell) NbadCell++;
      if (badstars[i] > 0 && badstars[i] < 8) NbadSSb++;  /* these are default flags */  
      if (badstars[i] == frame_info->FbadQU) NbadQU++;   
  }
 

  frame_info->NgoodBasis = Ngood;
  frame_info->NbadSSb = NbadSSb;
  frame_info->NbadCell = NbadCell;
  frame_info->NbadQU = NbadQU;
 
  return;
}

 /*********************************************************************
  * This function rejects stars for psf fitting by clipping their 
  * CELL_PROFs. Only the stars with badstar==0 are considered.
  * First the median is found for each cell, and then for each cell all 
  * stars that deviate more than ncellclip*sigma, where sigma is the 
  * star's measurement error (for this cell), are marked.
  * All stars that are marked in this way in at least nbadcell cells
  * are designated 'bad', and their badstar is set to badval. Star
  * that deviate more than maxdev*sigma, in _any_ cell, are also 
  * designated 'bad'
  * If the resulting number of good stars, Ngood, is > min_starnum the
  * return value is 1. If not, this clipping is undone and return value
  * is -Ngood (with the input value of Ngood unchanged).
  */
static int
clip_cprof(const CHAIN *starlist,	/* Chain of STAR1s to be clipped */
            FRAME_INFORMATION *frame_info,  /* info for this frame */
            int first,                  /* first star on this frame */
            int last,                   /* last star on this frame */
	    float ncellclip,		/* how many sigma to clip at */
	    int nbadcell,		/* how many bad cells to condemn a star */
            float maxdev,               /* maximal allowed deviation from the median */
            int min_starnum,            /* minimum number of stars - 1 */
            int badval,                 /* value to set for bad stars */
            int *Ngood,                 /* # of stars which have passed clipping */
	    int *badstar)		/* flag showing if a star has been clipped */			   
{
  STAR1 *star;
  int i, l, k, Ng = 0, Ncell, Ngood_old = 0;
  int Nstars;  
  int scaling_type = 2;            /* scale profiles by the average counts */
                                   /* in the first 7 cells */      
  float scale, flux;         
  float val, err;
  CELL_PROF *med_prof;
  float dev;                       /* deviation from the median for a given star */
  float max_dev;                   /* max. deviation from the median for a given star */
  int *oldclip; /* a copy of badstar for a case when the rejection has to be undone */
  int doubleS;   /* flag showing that this stamp is a repeat */
  float rowc=-1, colc=-1;  /* position of the previous star in the list */

  shAssert(starlist != NULL);
  shAssert(Ngood != NULL ); 

  Nstars = last - first + 1; 
  shAssert(*Ngood <= Nstars);

  if (Nstars < 1) {
     shFatal("clip_cprof: no stars to clip, Nstars = %d???\n",Nstars);
  }
 
  if (*Ngood < min_starnum + 1) {
     /* no clipping is needed */
     pack_frameinfo(frame_info, first, last, *Ngood, badstar); 
     return 1;
  }  

  oldclip = shMalloc(Nstars * sizeof(int)); 
  memcpy(oldclip,badstar,Nstars*sizeof(int)); 

  /* find median profile */
  med_prof = phCellProfNew(NCELL); 
  phGetCprof(starlist,first,last,badstar,2,scaling_type,med_prof, NULL, NULL);

  /* loop over stars */
  for(k = first;k <= last;k++) {
       /* skip if this star was clipped */
       if (badstar[k-first] != 0) continue;
       i = 0; Ngood_old++;
       star = shChainElementGetByPos(starlist, k);
       Ncell = star->cprof->ncell;
       /* exclude if within 5 pixels from the previous star */
       if (fabs(rowc-star->object->rowc)<5 && fabs(colc-star->object->colc)<5) {
          doubleS = 1;
       } else {
          doubleS = 0;
       }
       rowc = star->object->rowc;
       colc = star->object->colc;
       /* loop over cells, count all deviations */
       max_dev = 0.0;
       flux = star->apCounts;
       scale = get_scale(scaling_type,&flux,star->cprof);	 
       for(l = 0; l < Ncell; l++) {   
           /* scaled profile */
           val = star->cprof->data[l]/scale;
           err = star->cprof->sig[l]/scale;
           if (med_prof->data[l] > 0 && 
               fabs(val-med_prof->data[l]) > ncellclip * err) i++;
           if (med_prof->sig[l] != 0 && err != 0.0) {
               dev = fabs(val-med_prof->data[l])/err;             
           } else {
               dev = 0.0;
           }
           if (dev > 0.0 && dev > max_dev) max_dev = dev;
       }
       /* if too many bad cells throw this star out */
       if (i >= nbadcell || max_dev > maxdev || doubleS) {
           badstar[k-first] = badval;
       } else {
           Ng++;  
       }   
  }  

  shAssert(*Ngood == Ngood_old);   
  phCellProfDel(med_prof);

  if (Ng < min_starnum + 1) {
      memcpy(badstar,oldclip,Nstars*sizeof(int));
      shFree(oldclip);
      return -Ng;
  } else {
      /* copy relevant information to frame_info */
      pack_frameinfo(frame_info, first, last, Ng, badstar); 
      *Ngood = Ng;
  }

  shFree(oldclip);

  return Ng;

} 


/********************************************************
  * This function rejects stars for psf fitting by clipping 
  * their width distribution, and their shape distribution 
  * (Q and U parameters). First, stars are clipped on (QU),
  * and then only the remaining stars on widths and b.
  * All distributions are clipped above nsigma*sigma. 
  * Stars are not removed from the chain. Rather, a flag 
  * badstar is set to !0 for each rejected star,
  * where the coding is:
  *  badstar = 1 -> rejected on the width of narrow Gaussian
  *  badstar = 2 -> rejected on the width of wide Gaussian
  *  badstar = 4 -> rejected on the amplitude ratio, b
  *  badstar = 8 -> rejected on U/Q clipping; FbadUQ
  *  badstar = 9 -> rejected on cellprof clipping; FbadCell
  *  badstar = 10 -> rejected on RMS radius clipping
  *
  * Note that the coding is cumulative: a star clipped in 
  * both narrow Gaussian and in b will have badstar = 5.
  *
  * Other values might be set in other clipping routines.
  */
static int
clip_stars(const CHAIN *starlist,	/* Chain of STAR1s to be clipped */
            FRAME_INFORMATION *frame_info,  /* info for this frame */
            int first,                  /* first star on this frame */
            int last,                   /* last star on this frame */
	    float nsigma_sigma,		/* how many sigma to clip sigma at */
	    float nsigma_QU,		/* how many sigma to clip QU at */
	    float nsigma_RMS_rad,	/* how many sigma to clip RMS rad at */
            int min_starnum,            /* minimum number of stars - 1 */
            int badval,                 /* value to set for bad stars */
	    int niter,			/* no. of times to iterate clipping */
            int *Ngood,                 /* # of stars which have passed clipping */
	    int *badstar)		/* flag showing if a star has been clipped */			   
{
  STAR1 *star;
  int i, j = 0, Ng = 0, NgoodQU, Ngood_old = 0;
  int NgoodRMS;				/* number of stars which pass
					   RMS radius clip */
  int Nstars;                    
  float *W1array, *W2array, *Wbarray;	/* arrays */ 
  float *Qarray, *Uarray, *dQUarray;	/*        for */
  float *RMSarray;			/*            sorting */
  float W1quart[3], W2quart[3], Wbquart[3]; /* quartiles */
  float Qquart[3], Uquart[3], dQUquart[3]; /*      for sorted */ 
  float RMSquart[3];			/*                  arrays */ 
  float W1sig=0, W2sig=0, Wbsig=0;	/* sigmas */
  float Qsig=0, Usig=0, dQUsig=0;       /*        determined */
  float RMSsig = 0;			/*                 in get_stats */
  float dx, dy, *oldclip;		/* auxiliary */


  shAssert(starlist != NULL);
  shAssert(Ngood != NULL ); 

  Nstars = last - first + 1; 
  shAssert(*Ngood <= Nstars); 

  if (Nstars < 1) {
     shFatal("clip_stars: no stars to clip, Nstars = %d???\n",Nstars);
  }
   
  if (*Ngood < min_starnum + 1) {
     /* no clipping is needed */
     pack_frameinfo(frame_info, first, last, *Ngood, badstar); 
     return 1;
  }   

  oldclip = shMalloc(Nstars * sizeof(int)); 
  memcpy(oldclip,badstar,Nstars*sizeof(int));

  /* allocate sorting arrays */
  W1array = (float *)shMalloc(7*Nstars * sizeof(float));
  W2array = W1array + Nstars;
  Wbarray = W2array + Nstars;
  Qarray = Wbarray + Nstars;
  Uarray = Qarray + Nstars;
  dQUarray = Uarray + Nstars;
  RMSarray = dQUarray + Nstars;

  /* First clip on QU */
  /* populate sorting arrays */
  for(i = first, j = 0; i <= last; i++) { 
      /* skip if this star was clipped */
      if (badstar[i-first] != 0) continue;

      Ngood_old++;
      star = shChainElementGetByPos(starlist,i);
      Qarray[j] = star->Q;
      Uarray[j++] = star->U;
  }
 
  Ng = j;
  /* get statistics for each sorting array */
  get_stats(Qarray, Ng, nsigma_QU, niter, 0, &Qsig, Qquart); 
  get_stats(Uarray, Ng, nsigma_QU, niter, 0, &Usig, Uquart); 

  /* calculate distances from the median position in (Q,U) plane */
  for(i = first, j = 0; i <= last; i++) { 
      /* skip if this star was clipped */
      if (badstar[i-first] != 0) continue;
      dx = Qquart[1] - Qarray[j];
      dy = Uquart[1] - Uarray[j];
      dQUarray[j++] = sqrt(dx*dx + dy*dy);
  }  
  shAssert(j == Ng);

  /* get statistics for dQUarray */
  get_stats(dQUarray, Ng, nsigma_QU, niter, 1, &dQUsig, dQUquart); 

  /* go through stars again and mark all those that should be clipped */
  for(i = first, j = 0; i <= last; i++) { 
      /* skip if this star was clipped */
      if (badstar[i-first] != 0) continue;
      if (dQUarray[j++] - dQUquart[1] > nsigma_QU*dQUsig) {
          badstar[i-first] = badval;
          Ng--;
      }   
  }
  shAssert(Ng >= 0);
/*
 * Now clip on RMS
 */
  for(i = first, j = 0; i <= last; i++) { 
      /* skip if this star was clipped */
      if (badstar[i-first] != 0) continue;

      star = shChainElementGetByPos(starlist,i);
      RMSarray[j++] = star->M_rms;
  }
  NgoodRMS = j;
  
  get_stats(RMSarray, NgoodRMS, nsigma_sigma, niter, 1, &RMSsig, RMSquart);

  /* go through stars again and mark all those that should be clipped */
  j = 0;
  for(i = 0; i < Nstars; i++) { 
      if (badstar[i] != 0) continue;
      
      if(fabs(RMSarray[j] - RMSquart[1]) > nsigma_RMS_rad*RMSsig) {
	 badstar[i] += 10;
      }
      j++;       
  }  
/*
 * Now clip on sigma1, sigma2 and b
 */
  /* populate sorting arrays */
  for(i = first, j = 0; i <= last; i++) { 
      if (badstar[i-first] != 0) continue;
      
      star = shChainElementGetByPos(starlist,i);
      W1array[j] = star->dgpsf->sigmax1; 
      W2array[j] = star->dgpsf->sigmax2; 
      Wbarray[j++] = star->dgpsf->b;
  }
 
  NgoodQU = j;

  /* get statistics for each sorting array */
  get_stats(W1array, NgoodQU, nsigma_sigma, niter, 1, &W1sig, W1quart);
  get_stats(W2array, NgoodQU, nsigma_sigma, niter, 1, &W2sig, W2quart);
  get_stats(Wbarray, NgoodQU, nsigma_sigma, niter, 1, &Wbsig, Wbquart);

  /* go through stars again and mark all those that should be clipped */
  j = Ng = 0;
  for(i = 0; i < Nstars; i++) { 
      if (badstar[i] != 0) continue;
      if(fabs(W1array[j] - W1quart[1]) > nsigma_sigma*W1sig) badstar[i] += 1; 
      if(fabs(W2array[j] - W2quart[1]) > nsigma_sigma*W2sig) badstar[i] += 2; 
      if(fabs(Wbarray[j] - Wbquart[1]) > nsigma_sigma*Wbsig) badstar[i] += 4; 
      if (badstar[i] == 0) Ng++; 
      j++;       
  }  

  shFree(W1array);

  shAssert(j == NgoodQU);
  shAssert(Ng <= NgoodQU);
  shAssert(*Ngood == Ngood_old);   
/*
 * Store in frame_info the parameters needed for setting the badstar
 * flag in other (already rejected) STAR1s, and also for plotting
 */
  frame_info->sig1med = W1quart[1];   
  frame_info->sig1sig = W1sig;                
  frame_info->sig2med = W2quart[1];                
  frame_info->sig2sig = W2sig; 
  frame_info->bmed = Wbquart[1];                
  frame_info->bsig = Wbsig;                
  frame_info->Qmedian = Qquart[1];             
  frame_info->Qsigma = Qsig;            
  frame_info->Umedian = Uquart[1];           
  frame_info->Usigma = Usig;              
  frame_info->dQUmed = dQUquart[1];         
  frame_info->dQUsig = dQUsig; 
  frame_info->RMSmed = RMSquart[1];
  frame_info->RMSsig = RMSsig; 

  if (Ng < min_starnum + 1) {
      memcpy(badstar,oldclip,Nstars*sizeof(int));
      shFree(oldclip);
      return -Ng;
  } else {
      /* copy relevant information to frame_info */
      pack_frameinfo(frame_info, first, last, Ng, badstar); 
      *Ngood = Ng;
  }

  shFree(oldclip);

  return Ng;
           
}

/*****************************************************************************/
/*
 * Given a set of FRAME_INFORMATIONs (one for each band) with the
 * statistical distribution of e.g. U/Q set by phSelectPsfStars(), go
 * through a set of STAR1Cs setting STAR1.badstar
 */
void
phSetBadstarFlagFromChain(const CHAIN *starlist, /* Chain of STAR1cs */
			  const CHAIN *const *frameinfolist, /* clipping info
							      for each band */
			  float ncellclip, /*# of sigma for a cell to be bad*/ 
			  int nbadcell,	/* max. # of bad cells for a star
					   to be good */ 
			  float maxdev,	/* max dev of a cell to condemn all */ 
			  float nsigma_sigma, /* nsigma to clip sigma at */
			  float nsigma_QU, /* nsigma to clip QU at */
			  float nsigma_RMS_rad)	/* nsigma to clip RMS rad. at*/
{
   int badstar;				/* value of badstar */
   float dQU;				/* how far (Q,U) is from median value*/
   int frame0 = -1;			/* First frame on frameinfolist[] */
   int frame = -1;			/* current frame in frameinfolist[] */
   int i, c;				/* counters */
   int ncolor;				/* number of colours in STAR1C */
   const FRAME_INFORMATION *frame_info = NULL; /* an element of frameinfolist[] */
   STAR1C *star1c;			/* an element of starlist */
   STAR1 *star1;			/* == star1c->star1[] */
   float W1med[NCOLOR];			/* == per band frame_info->sig1med */
   float W1sig[NCOLOR];			/* == per band frame_info->sig1med */
   float W2med[NCOLOR];			/* == per band frame_info->sig2med */
   float W2sig[NCOLOR];			/* == per band frame_info->sig2med */
   float Wbmed[NCOLOR];			/* == per band frame_info->bmed */
   float Wbsig[NCOLOR];			/* == per band frame_info->bmed */
   float Umed[NCOLOR];			/* == per band frame_info->Umedian */
   float Qmed[NCOLOR];			/* == per band frame_info->Qmedian */
   float dQUmed[NCOLOR];		/* == per band frame_info->dQUmed */
   float dQUsig[NCOLOR];		/* == per band frame_info->dQUmed */
   float RMSmed[NCOLOR];		/* == per band frame_info->RMSmed */
   float RMSsig[NCOLOR];		/* == per band frame_info->RMSmed */
   
   shAssert(starlist != NULL && starlist->type == shTypeGetFromName("STAR1C"));
   if(starlist->nElements == 0) {
      return;				/* No stars; no badstars */
   }

   star1c = shChainElementGetByPos(starlist, 0);
   ncolor = star1c->ncolors;
/*
 * Check that the frameinfolist chains make sense
 */
   for(c = 0; c < ncolor; c++) {
      shAssert(frameinfolist[c] != NULL &&
	       frameinfolist[c]->type==shTypeGetFromName("FRAME_INFORMATION"));

      frame_info = shChainElementGetByPos(frameinfolist[c], 0);
      if(frame0 < 0) {
	 frame0 = frame_info->frame;
      }
      shAssert(frame0 == frame_info->frame);
   }
/*
 * Go through starlist, star1c by star1c, setting STAR1.badstar
 */
   for(i = 0; i < starlist->nElements; i++) {
      star1c = shChainElementGetByPos(starlist, i);
      star1 = star1c->star1[0];
      shAssert(star1 != NULL);
/*
 * Do we need to read a new set of FRAME_INFORMATIONs?
 */
      if(star1->frame != frame) {
	 frame = star1->frame;
	 for(c = 0; c < ncolor; c++) {
	    frame_info =
	      shChainElementGetByPos(frameinfolist[c], frame - frame0);
	    shAssert(frame_info->frame == frame);

	    W1med[c] = frame_info->sig1med;
	    W1sig[c] = frame_info->sig1med;
	    W2med[c] = frame_info->sig2med;
	    W2sig[c] = frame_info->sig2med;
	    Wbmed[c] = frame_info->bmed;
	    Wbsig[c] = frame_info->bmed;

	    Umed[c] = frame_info->Umedian;
	    Qmed[c] = frame_info->Qmedian;
	    dQUmed[c] = frame_info->dQUmed;
	    dQUsig[c] = frame_info->dQUmed;

	    RMSmed[c] = frame_info->RMSmed;
	    RMSsig[c] = frame_info->RMSmed;
	 }
      }
/*
 * Set badstar
 */
      for(c = 0; c < ncolor; c++) {
	 star1 = star1c->star1[c];

	 badstar = 0;
	 if(fabs(star1->M_rms - RMSmed[c]) > nsigma_RMS_rad*RMSsig[c]) {
	    badstar += 10;
	 }
      
	 if(star1->dgpsf != NULL) {
	    if(fabs(star1->dgpsf->sigmax1 - W1med[c]) >
					  nsigma_sigma*W1sig[c]) badstar += 1; 
	    if(fabs(star1->dgpsf->sigmax2 - W1med[c]) >
					  nsigma_sigma*W2sig[c]) badstar += 2; 
	    if(fabs(star1->dgpsf->b - Wbmed[c]) >
					  nsigma_sigma*Wbsig[c]) badstar += 4;
	 }

	 dQU = sqrt(pow(star1->Q - Qmed[c], 2) + pow(star1->U - Umed[c], 2));
	 if(dQU - dQUmed[c] > nsigma_QU*dQUsig[c]) {
	    badstar += frame_info->FbadQU;
	 }
/*
 * This routine doesn't bother setting the badcell values
 * to frame_info->FbadCell, so don't override set values
 */
	 if(star1->badstar) { /* it was already set */
	    ;
	 } else {
	    star1->badstar = badstar;
	 }
      }
   }
}

/*****************************************************************************/

/* check that DGPSF contains acceptable values */
static int
goodPSF(DGPSF *psf)
{
   int psfOK = 1;

   shAssert(psf != NULL);  

   /* start with psfOK = 1, any failure will flip it to 0 */

   /* we must have an positive chisq to start with */
   if(psf->chisq < 0) psfOK = 0;
   /* amplitude must be positive */
   if (psf->a <= 0 || psf->a != psf->a) psfOK = 0;  
   /* widths must be positive */   
   if (psf->sigmax1 <= 0 || psf->sigmax1 != psf->sigmax1) psfOK = 0;
   if (psf->sigmax2 <= 0 || psf->sigmax2 != psf->sigmax2) psfOK = 0;
   /* we are using circularly symmetric gaussians */
   if (psf->sigmax1 != psf->sigmay1) psfOK = 0;
   if (psf->sigmax2 != psf->sigmay2) psfOK = 0;
   /* sigma2 must be outer gaussian */   
   if (psf->sigmax2 < psf->sigmax1) psfOK = 0; 
   /* sigma2 must have a sensible value (PR 5288) */   
   if (psf->sigmax2 > 100) psfOK = 0; 
   /* amplitude of outer Gaussian must be nonnegative */
   if (psf->b < 0 || psf->b != psf->b) psfOK = 0; 
   /* amplitude of power-law wing must be nonnegative */
   if (psf->p0 < 0 || psf->p0 != psf->p0) psfOK = 0; 
   /* amplitude of power-law wing must be < 1 */
   if (psf->p0 >= 1) psfOK = 0; 
   /* radial scale of power-law wing must be positive */
   if (psf->sigmap <= 0 || psf->sigmap != psf->sigmap) psfOK = 0;
   /* power-law index must be > 2 */
   if (psf->beta < 2 || psf->beta != psf->beta) psfOK = 0; 
   /* power-law index must be < 10 */
   if (psf->beta > 10 || psf->beta != psf->beta) psfOK = 0; 

   if (psfOK) {
      return 1;
   } else {
      return 0;
   }

}



/*********************************************************************
 * This function takes a PSF_REG and extracts CELL_PROF from its
 * REGION. It returns the object's peak value.
 */
static float 
profileFromPSFReg(const PSF_REG *psfReg,         /* input PSF_REG */
                            int id,              /* ID number */
		          float sky,		 /* sky level */
		          float skysig,	         /* sky sigma */ 
                          float gain,            /* gain of amplifier */
                          float dark_var,        /* variance of dark background */
                      CELL_PROF *cprof,          /* output */
                        OBJECT1 *object)         /* for keeping radial profile and center */
{
   float xc, yc;       /* object center coordinates relative to REGION's origin */ 
   int cpeak, rpeak;   /* center in abs. coord. (row0 and col0 included) */  
   float dxc, dyc, posErr; /* errors in center position */
   float peak;     /* peak */
   float sigsqx, sigsqy, width; /* estimates of the width^2 of the object in x and y */
   float bkgd, annulus_sig, smooth=1.0; 
   CELL_STATS *cellstats;
   float *profile, *profErr, *profileMedian;
   int i;

   
   /* background level in reconstructed PSF */
   bkgd = SOFT_BIAS;

   /* approximate object's center in abs. coord. */
   cpeak = psfReg->reg->col0 + psfReg->reg->ncol/2;
   rpeak = psfReg->reg->row0 + psfReg->reg->nrow/2;
 
   /* refine object's center */
   if (phObjectCenterFind(psfReg->reg, cpeak, rpeak, sky, smooth, bkgd, dark_var, gain,
			  &xc, &yc, &dxc, &dyc, &peak, &sigsqx, &sigsqy) < 0) {
       shError("       profileFromPSFReg: phObjectCenterFind fails");  
       return -1;  
   }       


   /* correct coordinates to be relative to REGION's origin */  
   xc -= psfReg->reg->col0;
   yc -= psfReg->reg->row0;

   /* extract the radial profile */
   if((cellstats = phProfileExtract(id,-1,psfReg->reg,yc,xc,xc,bkgd,skysig,0))
								     == NULL) {
       shError("       profileFromPSFReg: phProfileExtract returns NULL");
       return -1;  
   }

   /* set radial profiles in PSFobject from cellstats */
   profile = object->profMean;
   profErr = object->profErr;
   profileMedian = object->profMed;

   for (i = 0; i < cellstats->nannuli_c; i++) {
       profileMedian[i] = phProfileMedian(cellstats, i, 0, 0, NULL);
       profile[i] = phProfileMean(cellstats, i, 0, 0, &annulus_sig);
       if (i == 0) {
	   profErr[0] = sqrt((profile[0] + sky*cellstats->area[0])/gain +
			   cellstats->area[0]*(dark_var + skysig*skysig));
       } else {
	   profErr[i] = annulus_sig;
       }
   }
   object->nprof = cellstats->nannuli_c;
   object->rowc = yc; object->rowcErr = dyc;  
   object->colc = xc; object->colcErr = dxc;  
   object->psfCounts = psfReg->counts;

   /* extract CELL_PROF from CELL_STATS */
   posErr = sqrt(pow(dxc,2) + pow(dyc,2));
   width =  sqrt(sigsqx + sigsqy);
   phCellProfSet(cprof, cellstats, 0, 1000, gain, dark_var, width, posErr,
									 1, 0);

   return peak;

}
	

/*****************************************************************************/
/*
 * comparison float function for qsort()
 */
static int
cmp(const void *a, const void *b)
{
   float ia = *(float *)a;
   float ib = *(float *)b;

   return((ia < ib) ? -1 : ((ia == ib) ? 0 : 1));
}


/*****************************************************************************/
/*
 * find arbitrary quartile value for values given in buffer buf[].
 * If sorted == 1, assume that the values are sorted.
 */
static float
get_quartile(float *buf,			/* the values */
         int n,			        /* number of values */
         int sorted,			/* see comment above */
         float q)                       /* which quartile (0<q<1) */
{
   int nL, nR;
   double frac, ipart, qn;		 
   float quartile;


   shAssert(n > 0); 
   shAssert(q >= 0 && q <= 1);
   if (n <= 1) {
      return buf[0];
   }

   /* sort if needed */
   if (!sorted) qsort(buf,n,sizeof(buf[0]),cmp); 

   if (q == 0) {
      return buf[0];
   }   
   if (q == 1) {
      return buf[n-1];
   }


   /* position of the required quartile */
   qn = q * (n - 1);
   /* fractional position between the two integer values */
   frac = modf(qn,&ipart);
   /* bracket the position */
   nL = ipart; nR = nL + 1;
   /* the required quartile value */
   if (nR <= n) {
       quartile = buf[nL] * (1-frac) + buf[nR] * frac;
   } else {
       quartile = buf[n];
   }

   return quartile;
}
