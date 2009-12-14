/*
 * <AUTO>
 *
 * FILE: star1c.c
 * 
 * DESCRIPTION:
 * Support for the star1c structure used by the PSP to hold information 
 * about an object in all colors.
 *
 * AUTHORS:	                         Creation date: September 26, 1996
 *     Rob Simcoe
 * </AUTO>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dervish.h"
#include "astrotools.h"
#include "phCorrectFrames.h"
#include "phStar1.h"
#include "phStar1c.h"
#include "phFocus.h"
#include "phOffset.h"
#include "phMeasureObj.h"
#include "phCellFitobj.h"


/***************************************************************************
 * <AUTO EXTRACT>
 *
 * This module corrects postage stamps and
 * makes a STAR1 structure for each one by fitting a double gaussian
 * psf to each star. It also sets the RA and Dec for the star. All
 * input postage stamps are assumed to be from the same frame
 */


RET_CODE 
phMakeStar1clist(CHAIN *star1clist, /* Chain to hold star1cs */	
                 CHAIN *masks,      /* masks from region */    
		 CHAIN *pstamps,    /* Raw postage stamps */
		 int ifilter,       /* index of the filter in filterlist */
		 int nfilters,      /* no. of colors in the star1c */
		 int id0,	    /* starting starid */
		 int frame,	    /* Current frame */
		 const REGION *flat,	 /* Flat vector for current frame */
		 const REGION *bias,	 /* Bias vector for current frame */
		 const CCDPARS *ccdpars, /* CCD properties */
		 int bscale,	         /* bias scale factor */
		 const CALIB1 *calib,	 /* calib containing left and
					   right bias */
		 int stamp_size,    /* size of postage stamps (=nc=nr) */
		 int aperture,      /* index of profile element for phot */
		 int psf_def_rad,   /* search radius for star peak */
                 float cr_min_s,    /* CRs must be > this many sky-sig above sky */
                 int cr_min_e,      /* CRs must have > this many electrons in peak pixel */
                 float cr_cond3,    /* used in condition 3 for CR; see CR.c code */
                 float cr_cond32,   /* used in condition 3 for CR; see CR.c code */
		 FRAMEPARAMS *fparams,	/* gain, soft_bias, etc. */
		 int threshold,         /* for the object finder in sym test */
                 float sigma_multi,     /* number of sky sigma for multiple */
                 float sigma_badsky,    /* number of sigma for bad sky */
                 float second_peak,     /* max. allowed strength for 2nd peak */
                 int wing,              /* 1 for wing stars => bypasses an assertion */
		 int sym_flag,    /* keep the symmetry test regions? */
		 int regflag)	  /* flag to put a region in the star1 */

{
    CURSOR_T crsr;
    REGION *ps = NULL;
    REGION *rawps;
    STAR1 *star;
    STAR1C *star1c;
    int id = id0;			/* id number of STAR1C */
    int minval;				/* minimum interpolated value */
    int off_chip;			/* is part of pstamp off the chip? */
    int fullWell[2];			/* saved values of fparams->fullWell */
    REGION *scr1 = NULL, *scr2 = NULL;	/* scratch regions for
					   phStar1MakeFromReg() */
    SPANMASK *sm = NULL;
    OBJMASK *satur_mask;

    shAssert(star1clist != NULL);
    shAssert(masks      != NULL);
    shAssert(pstamps    != NULL);
    shAssert(flat       != NULL);
    shAssert(bias       != NULL);
    shAssert(ccdpars    != NULL);
    shAssert(calib      != NULL);
    shAssert(fparams    != NULL);
    shAssert(phDarkVariance(fparams, 0, 0) == calib->dark_variance);
    shAssert(phGain(fparams, 0, 0) == calib->gain);
    shAssert(nfilters == 1 || strchr(atFilternames, *calib->filter) != NULL);

    wing = wing;			/* pacify compiler */
    
    minval = calib->skyraw - 2*calib->skyerr + 0.5;
    fullWell[0] = fparams->fullWell[0]; fullWell[1] = fparams->fullWell[1];

    if(!regflag) {			/* we need a region */
       ps = shRegNew("corrected ps", stamp_size, stamp_size, TYPE_PIX);
    }
    scr1 = shRegNew("", stamp_size, stamp_size, TYPE_PIX);
    scr2 = shRegNew("", stamp_size, stamp_size, TYPE_PIX);

    crsr = shChainCursorNew(pstamps);
    while ((rawps = (REGION *) shChainWalk(pstamps, crsr, NEXT)) != NULL) {

/* check that that pstamp lies on the frame; problems in SSC can create
 * stamps for which this isn't true
 */
	off_chip = 0;
	if(rawps->col0 < 0 || rawps->col0 + rawps->ncol > flat->ncol) {
#if 0
	   shError("phMakeStar1clist: "
		   "postage stamp has pixels off the CCD (cols %d..%d)",
		    rawps->col0,rawps->col0 + rawps->ncol - 1);
#endif
	   off_chip = 1;
	} else {
          /* If want to keep the corrected stamps around in the star1s so
           * we can look at them later, we need to make a new reg for each
           */
	   if (regflag) {
	      ps = shRegNew("corrected ps", stamp_size, stamp_size, TYPE_PIX);
   	   }
	   ps->mask = (MASK *)phSpanmaskNew(ps->nrow, ps->ncol);
	   ps->row0 = rawps->row0;
	   ps->col0 = rawps->col0;
           /* run pstamps through correctFrames; as it can change fullWell[01]
            * reset it to the initial value before each call */
           fparams->fullWell[0] = fullWell[0];
           fparams->fullWell[1] = fullWell[1];
           fparams->sky = phBinregionNewFromConst(calib->skyraw, 1, 1, 1, 1, 0);
           if (strcmp(rawps->name, "fake wing star") != 0) {
               phCorrectFrames(rawps, bias, flat, ccdpars, fparams, bscale,
			       calib->lbias, calib->rbias, 1, ps, minval, 0);
           }
           phBinregionDel((BINREGION *)fparams->sky);
           fparams->sky = NULL;
	}

/*
 * Make star1
 */
        if(!off_chip) {
           star = phStar1MakeFromReg(ps, 0, calib->skyraw, calib->skyerr, 
				     calib->skysig, fparams, scr1, scr2,
				     aperture, psf_def_rad,
				     cr_min_s, cr_min_e, cr_cond3, cr_cond32, 
				     threshold, sigma_multi, sigma_badsky,
				     second_peak);
           if (strcmp(rawps->name, "fake wing star") == 0) {
	       star->flags |= STAR1_FAKEWING;
           }
	   /* (we could just check inside def_rad ...)  */
	   sm = (SPANMASK *)ps->mask;
           if (sm->masks[S_MASK_SATUR]->nElements > 0) {
	       if (phRectIntersectMask(sm->masks[S_MASK_SATUR],ps->col0,ps->row0,
				ps->col0+ps->ncol,ps->row0+ps->nrow)) {
	           star->flags |= STAR1_SATURATED; 
                   star->object->flags |= OBJECT1_SATUR;           
	       }
           }



           { /* get effective width */
         
             DGPSF *psf;
             int neff;             
             if (!(star->flags & STAR1_NOPROFILE) && 
                   star->cprof != NULL && star->cprof->is_median == 0) {
                 psf = phDgpsfNew();          
                 phDgpsfFitFromCellprof(psf, star->cprof);
     	         neff = phPsfCountsSetupFromDgpsf(ifilter,psf, 0);
                 star->Eff_width = 0.663 * 0.400 * sqrt(neff);
 	         phDgpsfDel(psf);  
             } 
           }



        } else {
#if 0
	   shError("phMakeStar1clist: "
		   "stamp off the CCD, making a dummy star1");
#endif
           star = phStar1New();
	   star->flags |= STAR1_OFFCHIP;
           star->object->rowc = rawps->row0;
           star->object->colc = rawps->col0;
           star->object->rowcErr = 0;
           star->object->colcErr = 0;
	}

	star->starid = id++;
	star->frame = frame;	
 
	if(star->sym != NULL) {
	   if(!sym_flag) {
	      shRegDel(star->sym->residual); star->sym->residual = NULL;
	      shRegDel(star->sym->star); star->sym->star = NULL;
	   }
	}

	/* 
	 * Set the RA and Dec for the star; this takes two steps.
	 * First, we convert (row, col) to Great Circle coords (mu, nu).
	 * Second, we convert (mu, nu) to (RA, Dec).
	 */
	if(strchr(atFilternames, *calib->filter) != NULL) { /* an SDSS filter*/
	   double mu, nu;

	   atTransApply(calib->toGCC, *calib->filter,
			star->object->rowc, star->object->rowcErr,
			star->object->colc, star->object->colcErr,
			NULL, NULL, &mu, NULL, &nu, NULL);
	   atGCToEq(mu, nu, &star->ra, &star->dec, calib->node, calib->incl);
	}
/*
 * Add the STAR1 as a field in a Chain of star1c's (analogous to an OBJC
 * in frames, a STAR1C contains STAR1s of all 5 passbands, as well as a
 * common id for the star in all bands).
 *
 * We assume that there are the same number of pstamps in each band, and
 * that they are in the same order
 */
	if(ifilter == 0) {      
	   star1c = phStar1cNew(nfilters);
	   shChainElementAddByPos(star1clist,star1c,"STAR1C",TAIL,AFTER);
	   star1c->starid = star->starid;
	} else {
	   star1c = shChainElementGetByPos(star1clist, star->starid);
	   shAssert(star1c != NULL && star1c->starid == star->starid); 
	}
   
        /* if fake star, make sure this is the only flag set
	   (not even STAR1_NOFLAGS) */
        if (star->flags & STAR1_FAKEWING) {
            star->flags = STAR1_FAKEWING;
        } 
 
	star1c->star1[ifilter] = star;
	star1c->flags |= star1c->star1[ifilter]->flags;

        /* get saturated pixels mask needed for composite profile */
        if(star->object->flags & OBJECT1_SATUR) {
          satur_mask = phObjmaskIntersection(star->object->mask,sm->masks[S_MASK_SATUR]);
        } else {
          satur_mask = phObjmaskNew(0);
        }
        shAssert(satur_mask != NULL);
	shChainElementAddByPos(masks,satur_mask,"OBJMASK",TAIL,AFTER);
        
	if (!regflag) {		
	    star->object->region = NULL; /* so we can reuse region 
					    and delete star1s */
	    /* We can safely delete the spanmask now, since regDel won't  */
	    phSpanmaskDel((SPANMASK *) ps->mask);
	    ps->mask = NULL;  
        }
 
    }
        
    /* Cleanup */
    shChainCursorDel(pstamps,crsr);
    if(!regflag) {
       shRegDel(ps);
    }
    shRegDel(scr1); shRegDel(scr2);

    return(SH_SUCCESS);
}

/********************************************************/

/* <AUTO EXTRACT>
*
*  Allocates space needed for a new Star1c, holding ncolors star1's.
*
*/

STAR1C *
phStar1cNew (int ncolors) 
{
  static int id = 0;
  int i = 0;

  STAR1C *star1c = (STAR1C *) shMalloc(sizeof(STAR1C));

  *(int *)&star1c->id = id++;

  star1c->starid = -1;
  star1c->ncolors = ncolors;
  star1c->flags = STAR1_NOFLAGS;

  /* Usually ncolors is 5, for u g r i z. */ 

  star1c->star1 = shMalloc(ncolors * sizeof(STAR1 *));

  for (i = 0; i < ncolors; i++)
    star1c->star1[i] = NULL;

  return (star1c);
}

/*****************************************************/
/* <AUTO EXTRACT>
*
*   Frees memory allocated by a call to phStar1cNew()
*
*/

void 
phStar1cDel(STAR1C *star) 
{
  if (star == NULL)
    return;

  if(star->star1 != NULL) {
     shFree(star->star1);
  }

  shFree((char *) star);
}
