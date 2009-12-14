/*
 * <AUTO>
 *
 * FILE: makeStarlist.c
 * 
 * DESCRIPTION:
 * routine to make star1s with fitted dgpsf's for all good postage stamps
 * on input list. 
 *
 * </AUTO>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dervish.h"
#include "phStar1.h"
#include "phStar1c.h"
#include "phExtract.h"
#include "phMath.h"
#include "phRandom.h"
#include "phObjects.h"
#include "phCellFitobj.h"        /* needed for phFitCellPsfCoeffsFromCellProf*/
#include "phMathUtils.h"		/* here so as not to re#def M_SQRT */

#define REJECT_STATS 0      /* if true print out percentage of rejected stars in psfReject */


/*
 * <AUTO EXTRACT>
 *
 * Creates a chain of STAR1s of color ncolor (0 = u, 1 = g, 2 = r, etc)
 * from a chain of STAR1Cs.  Also rejects bad candidates for psf fitting
 * or wing stars (depending on the input data)
 *
 * Flags are checked for the STAR1C as well as the individual STAR1s;
 * flags may be either required or forbidden; all the required bits
 * must be set, and none of the reject bits.
 *
 * Examples of star1c_require_mask would be STAR1_NOFLAGS (always
 * set; no constraint) and STAR1_SATURATED for wing stars. It's clear
 * that star1c_ignore_mask must always include star1c_require_mask,
 * but it can also contain bits that tell us that a star's bad in
 * all bands (e.g. bad width); a common value would be ~STAR1_NOFLAGS
 *
 * The corresponding star_*masks are for conditions that only apply
 * per-band, for example, STAR1_SATURATED is not allowed for KL-psf stars
 *
 * The star1c_ constraints are actually applied to the logical OR of
 * all the star1 flags, as not all per-band bits are actually propagated up
 *
 * N.b.  All bits in star1_ignore_mask are always also ignored in STAR1c->flags
 * and STAR1_NOFLAGS is handled for you
 */
RET_CODE
phMakeStarlist(CHAIN *star1clist,         /* Input chain of star1c's */
	       CHAIN *starlist,           /* Output chain of star1s for psf fit */
	       CHAIN *ps_rejects,         /* Output star1 chain of rejects */
	       int ifilter,               /* The current filter */
	       int first_frame,           /* Start from this position in *star1clist */
	       int last_frame,            /* End at this position in *star1clist */
               STAR1_FLAGS star1c_require_mask, /* require these STAR1c bits */
               STAR1_FLAGS star1c_ignore_mask,  /* ignore these STAR1c bits */
               STAR1_FLAGS star1_require_mask, /* require these STAR1 bits */
               STAR1_FLAGS star1_ignore_mask)  /* ignore these STAR1 bits */
{
   CHAIN *chain;			/* list to put a given star on */
   STAR1C *star1c;
   STAR1_FLAGS star1c_flags = STAR1_NOFLAGS; /* OR of all STAR1->flags */
   int i, j;
   int nframes;
   
   shAssert(star1clist != NULL);
   shAssert(starlist   != NULL);
   shAssert(ps_rejects != NULL);

   star1c_ignore_mask |= STAR1_NOFLAGS; /* always set */
   star1c_require_mask |= STAR1_NOFLAGS; /* as it makes the logic simpler */
   star1_ignore_mask |= STAR1_NOFLAGS;	
   star1_require_mask |= STAR1_NOFLAGS;
   
   star1c_ignore_mask |= star1_ignore_mask; /* if ignored in STAR, ignore
					       in STAR1c too */

   nframes = shChainSize(star1clist);
   shAssert(first_frame >= 0);  
   shAssert(last_frame < nframes);  
/*
 * Separate star1clist into starlist and ps_rejects
 */
   for(i = first_frame; i <= last_frame; i++) {
      star1c = (STAR1C *)shChainElementGetByPos(star1clist, i);
      /*
       * Check that all STAR1 bits are propagated to the STAR1C
       */
      if(star1c_require_mask != STAR1_NOFLAGS) { /* we need a per-STAR1C
						    decision */
	 star1c_flags = STAR1_NOFLAGS;
	 for(j = 0; j < star1c->ncolors; j++) {
	    shAssert(star1c->star1[j] != NULL);
	    star1c_flags |= star1c->star1[j]->flags;
	 }
	 if(star1c_flags != star1c->flags) {
	    fprintf(stderr,"Expected 0x%x saw 0x%x\n",
		    star1c_flags, star1c->flags);
	 }
      }
      /*
       * Check for required and forbidden flags
       */
      if ((star1c_flags & star1c_require_mask) != star1c_require_mask ||
	  (star1c->star1[ifilter]->flags & star1_require_mask)
						       != star1_require_mask) {
	 chain = ps_rejects;		/* required bits are missing */
      } else if((star1c_flags & ~star1c_ignore_mask) != 0 ||
		(star1c->star1[ifilter]->flags & ~star1_ignore_mask) != 0) {
	 chain = ps_rejects;		/* naughty bits are set */
      } else if (star1c->star1[ifilter]->cprof == NULL) {
	 chain = ps_rejects;		/* good stars must have profiles */
      } else { 
	 chain = starlist;		/* An acceptable star */
      }
      
      shChainElementAddByPos(chain, star1c->star1[ifilter],
			     "STAR1", TAIL, AFTER);
   }
   
   return(SH_SUCCESS);
}



/*
 * <AUTO EXTRACT>
 *
 * Creates a chain of STAR1PCs of color ncolor (0 = u, 1 = g, 2 = r, etc)
 * from a chain of STAR1Cs.  Also rejects bad candidates.
 *
 * If !STAR1_NOFLAGS, we reject all colors (e.g. bad width, asymmetric)
 * but if the _STAR1_ is bad (e.g. saturated, poorly sampled) in 1 color,
 * we only reject it in that color
 */
RET_CODE
phMakeStarPClist(CHAIN *star1clist,         /* Input chain of star1c's */
	         CHAIN *starlist,           /* Output chain of STAR1PCs */
	         CHAIN *rejects,            /* Output star1 chain of rejects */
	         int ifilter,               /* The current filter */
	         int first_frame,           /* Start from this position in *star1clist */
	         int last_frame,            /* End at this position in *star1clist */
                 STAR1_FLAGS flags2ignore)  /* ignore these flags */   
{
   STAR1C *star1c;
   STAR1 *star;
   STAR1PC *star1pc;
   int j, nframes;
   
   shAssert(star1clist != NULL);
   shAssert(starlist   != NULL);
   shAssert(rejects != NULL);
 
   /* separate star1clist to starlist and rejects */
   nframes = shChainSize(star1clist);
   shAssert(first_frame >= 0);  
   shAssert(last_frame < nframes);  
   for (j = first_frame; j <= last_frame; j++) {
       star1c=(STAR1C *)shChainElementGetByPos(star1clist,j);
       /* PC stars should be rejected on a band by band basis (PR 5086) */
       if (((1 || star1c->flags == STAR1_NOFLAGS) && 
             star1c->star1[ifilter]->flags == STAR1_NOFLAGS) ||
	     !(star1c->star1[ifilter]->flags & ~flags2ignore) ) {
          star = star1c->star1[ifilter];
          /* stars without cprof can be also discarded */
          if (star->cprof != NULL) {
             star1pc = phStar1pcNew();
             /* copy the relevant information */
             star1pc->starid = star->starid;
             star1pc->frame = star->frame;
             star1pc->ra = star->ra;
             star1pc->dec = star->dec;
             star1pc->aperture = star->aperture; 
             star1pc->apCounts = star->apCounts; 
             star1pc->apCountsErr = star->apCountsErr; 
             star1pc->aperture = star->aperture; 
             star1pc->flags = star->flags;
             /* add this star to the chain */
	     shChainElementAddByPos(starlist, star1pc,
				"STAR1PC", TAIL, AFTER);
          } else {
	     shChainElementAddByPos(rejects, star1c->star1[ifilter],
				   "STAR1", TAIL, AFTER);   
          }
       } else {
	  shChainElementAddByPos(rejects, star1c->star1[ifilter],
				"STAR1", TAIL, AFTER);   
       }
   }

   return(SH_SUCCESS);
}


/********************************************************************/
/* Return a DGPSF with parameters a preset function of scanrow */

#define ROWS_PER_FRAME 1361		/* only used in this routine */
#define M_SIGMA1 0.94			/* and phMakeTestStarlist() */
#define M_SIGMA2 2.70

static DGPSF *
psfpars(float scanrow, int nframes, int amp, float change) 
{

    float a1, b1, a2, b2, variation;
    float b, sigma1, sigma2;
    DGPSF *psf = phDgpsfNew();

    /* Define how psf varies */
    /* First pass, all params vary the same, linear way:
     * sigma1 = a1 + b1*scanrow
     * sigma2 = a2 + b2*scanrow
     * b = .1 = constant
     *
     * centered at sigma1 = 0.94, sigma2 = 2.7.
     */
    variation = change/100./nframes;  /* Fractional variation of param/frame */

    b = 0.1;
    b1 = variation*M_SIGMA1/ROWS_PER_FRAME;
    b2 = variation*M_SIGMA2/ROWS_PER_FRAME;

    a1 = M_SIGMA1 - (b1 * nframes * ROWS_PER_FRAME / 2.0);
    a2 = M_SIGMA2 - (b2 * nframes * ROWS_PER_FRAME / 2.0);

    sigma1 = a1 + b1 * scanrow;
    sigma2 = a2 + b2 * scanrow;

    psf->sigmax1 = sigma1;
    psf->sigmay1 = sigma1;
    psf->sigmax2 = sigma2;
    psf->sigmay2 = sigma2;
    psf->b = b;

    psf->a = amp;
    
    return(psf);
}

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * This module takes simulates a starlist.
 */

RET_CODE 
phMakeTestStarlist(int nframes,       /* Number of frames in simulation */
		   int nstars,	      /* Average number of stars/frame */
		   int amp,		/* amplitude of stars */
		   float change,      /* % change in width over nframes */
		   CHAIN *starlist,   /* O: Chain to hold star1s */
		   CHAIN *truepsfs,   /* O: list of true psfs for star1s */
		   CHAIN *framepsfs,   /* O: list of psfs at frame centers */
		   int stampsize)     /* size of stamps to simulate */
{

    int i, j;
    FRAMEPARAMS *fparams = phFrameparamsNew('0');
    int ns;
    float gdev, row, col, scanrow, ranrow;
    DGPSF *psf;
    REGION *reg = shRegNew("testMakeStarlist",stampsize,stampsize,TYPE_U16);
    STAR1 *star;
    float gain;
    RANDOM *rand = phRandomNew("10000:2", 0);
    float sky = 100;
    float skyerr = 1;
    float skysig = 10;
    int skyrad = 0;		/* use default */
/*
 * This is just test code so this hardcoding's marginally OK. RHL
 */
    fparams->gain = phBinregionNewFromConst(3.0, 1, 1, 1, 1, MAX_U16);
    fparams->dark_variance = phBinregionNewFromConst(pow(7, 2), 1, 1, 1, 1, MAX_U16);
/*
 * For each frame ...
 */
    for (i = 0; i < nframes; i++) {

	/* Calculate psf at frame center for comparison with findPsf output */
	scanrow = i*ROWS_PER_FRAME/2.;
	psf = psfpars(scanrow, nframes, amp, change);
	shChainElementAddByPos(framepsfs, psf, "DGPSF", TAIL, AFTER);

	/* Get number of stars per frame */
	gdev = phGaussdev();
	ns = nstars + sqrt((double)nstars)*gdev + 0.5;

	/* For each star ... */
	for (j = 0; j < ns; j++) {
	    ranrow = 0.5*(1 + phRandom()/((float)(1U<<(8*sizeof(int)-1))));
	    scanrow = ROWS_PER_FRAME * (i+ranrow);
	    psf = psfpars(scanrow, nframes, amp, change);
	    row = stampsize/2 - 0.5 +
			 0.5*(1 + phRandom()/((float)(1U<<(8*sizeof(int)-1))));
	    col = stampsize/2 - 0.5 +
			 0.5*(1 + phRandom()/((float)(1U<<(8*sizeof(int)-1))));

	    shRegIntSetVal(reg, SOFT_BIAS + sky);
	    phDgpsfAdd(reg, psf, -1, row, col, psf->a);
	    phRegIntNoiseAdd(reg, rand, SOFT_BIAS, gain, 0);
	    star = phStar1MakeFromReg(reg, skyrad, sky, skyerr, skysig,
				      fparams, NULL, NULL, 3, stampsize/2,  
			              6.0, 150, 3.0, 0.6, -1, 5.0, 3.0, 0.05);
	    star->object->region = NULL;

	    star->frame = i;
	    star->object->rowc += scanrow;
	    shChainElementAddByPos(starlist, star, "STAR1", TAIL, AFTER);
	    shChainElementAddByPos(truepsfs, psf, "DGPSF", TAIL, AFTER);
	}
/* 
 * add noise to parameters?
 */
    }

    phRandomDel(rand);
    phFrameparamsDel(fparams);
    shRegDel(reg);

    return(SH_SUCCESS);
}

#undef ROWS_PER_FRAME			/* I said that they were only used */
#undef M_SIGMA1				/* in these two routines, didn't I? */
#undef M_SIGMA2

/********************************************************
  * <AUTO EXTRACT>
  *
  * This module rejects stars for psf fitting using the rough widths from
  * fitting a single gaussian (using findsigma from the jeg focus code). 
  * The distribution of star widths should be gaussian at worst, so
  * stars above nsigma*sigma (of the distribution, that is) are rejected.
  * Note that the stars are not removed from the chain.  Rather, flags are set
  * for later rejection.
  */

RET_CODE
phPsfReject(CHAIN *star1clist,		/* Chain of star1c's to be flagged */
            char *filters[],            /* letter codes for filters*/
	    float nsigma,		/* how many sigma to clip at */
	    int niter,			/* no. of times to iterate clipping */
	    int min_starnum,		/* min # of stars we'd like to do a 
					   meaningful rejection. */
	    int critical_amplitude)	/* minimum DGPSF->a value for a bad
					   width to condemn all bands */
{
  STAR1C *star1c;
  CURSOR_T crsr;
  int i, j;
  float median;				/* median of array */
  int num_elements;     /* number of star1c's in star1clist */
  float *sort_array;    /* We find the median and quartiles by sorting this */ 
  int sorted;				/* is sort_array sorted? */
  float sigma;          /* sigma is width of distribution of psf widths */
  int ifilter;          /* what filter we're on in the loop */ 
  int ncolors;		/* number of colours in a STAR1C */
  int ngood;            /* number of unrejected stamps in a color */
  char filter[FILTER_MAXNAME];   /* filter letter code */ 

#if REJECT_STATS
  int nbad = 0;      /* number of outliers */ 
  float perc_bad; /* percentage of bad stars */
#endif

  shAssert(star1clist != NULL);

  if(nsigma <= 0) {			/* no clipping is desired */
     return(SH_SUCCESS);
  }
/*
 * see how many filters we are using
 */
  star1c = (STAR1C *)shChainElementGetByPos(star1clist, HEAD);
  ncolors = star1c->ncolors;

  num_elements = star1clist->nElements;
  sort_array = (float *)shMalloc(num_elements * sizeof(float));
  crsr = shChainCursorNew(star1clist);

  for (ifilter = 0; ifilter < ncolors; ifilter++) { 
    shChainCursorSet(star1clist,crsr,HEAD);
    
    i = 0;
    strcpy(filter,(char *)filters[ifilter]);
 
    while ((star1c = (STAR1C *) shChainWalk(star1clist, crsr, NEXT)) != NULL) {
       shAssert(star1c->ncolors == ncolors);
       if ( (star1c->star1[ifilter]->flags == STAR1_NOFLAGS) ||
                !(star1c->star1[ifilter]->flags & ~STAR1_CR)) {
	  shAssert(star1c->star1[ifilter]->dgpsf != NULL);
	  sort_array[i++] = star1c->star1[ifilter]->dgpsf->width;       
       }
    }

    /*  sort_array is only populated with "healthy" star1 widths, that is,
	from star1s which had no flags.  There are i of these, so we now 
	set num elements to be the number of healthy widths, rather than
	the length of star1clist, i.e, number of elements in sort_array. */

    num_elements = i;    

    if (num_elements == 0) {
      shError("WARNING: NO good stars available for PSF fitting in filter %s; moving to next filter"
	      , filter);
      continue;
    } else if (num_elements < min_starnum) {
      shError("WARNING: skipping width rejection for filter %s, have only %d acceptable input stars (need at least %d)", filter, num_elements, min_starnum);
      continue;
    }

    shAssert(num_elements >= 1);
/*
 * clip the list at nsigma niter times
 */
    median = sigma = 0;			/* make compiler happy */
    sorted = 0;				/* it isn't sorted yet */
    for(i = 0;i < niter;i++) {
       if(i > 0) {
	  for(j = num_elements - 1;j >= 0;j--) { /* clip at +nsigma */
	     if(sort_array[j] < median + nsigma*sigma) {
		break;
	     }
	  }
	  if(j <= 0) {
	     num_elements = 0;
	     break;
	  }
       }

       phFloatArrayStats(sort_array, num_elements, sorted, &median, &sigma, NULL, NULL);
       sorted = 1;			/* sort_array is now sorted */
       sigma *= IQR_TO_SIGMA;
    }
    
    /* This shouldn't happen, but it conceivably _could_ with a small,
       uniform sample of stars.  This prevents us from rejecting everything. */
    if (sigma <= 1e-10) {
      shError("phPsfReject: Sigma of distribution of star widths = 0 in filter %s; skipping width rejection", filter);
      continue;
    }

    if(num_elements == 0) {
      shError("phPsfReject: "
	      "no stars remain after clipping histogram in filter %s",filter);
       continue;
    }
/*
 * Do the actual rejection
 */
    ngood = 0;
    num_elements = star1clist->nElements;
    for(i = 0; i < num_elements;i++) {
       star1c = (STAR1C *)shChainElementGetByPos(star1clist, i);

      /* Here, we want to use only star1s with no potential problems.
	 This is because we don't want to dereference a null pointer
	 to a dgpsf that doesn't exist.  */
       if(star1c->star1[ifilter]->dgpsf != NULL &&
	  fabs(star1c->star1[ifilter]->dgpsf->width - median) > nsigma*sigma) {
#if REJECT_STATS
          if (star1c->star1[ifilter]->flags == STAR1_NOFLAGS) { 
              nbad++;
          }
#endif
	  star1c->star1[ifilter]->flags |= STAR1_BADWIDTH;
	  if(star1c->star1[ifilter]->object->profMean[0] > critical_amplitude&&
			  !(star1c->star1[ifilter]->flags & STAR1_SATURATED)) {
	     star1c->flags |= STAR1_BADWIDTH;
	  }
       }
       
       if (star1c->star1[ifilter]->flags == STAR1_NOFLAGS) {
	  ngood++;
  printf(" %8.4f  %8.4f  %8.4f   %8.4f\n",star1c->star1[ifilter]->dgpsf->width,
  star1c->star1[ifilter]->dgpsf->sigmax1,star1c->star1[ifilter]->dgpsf->b, 
  star1c->star1[ifilter]->dgpsf->sigmax2);
       }
    }
    
#if REJECT_STATS
    printf("Total number of stars in filter %s: %d\n",filter,num_elements);
    printf("  Median width: %6.2f, sigma: %6.2f\n",median,sigma); 
    perc_bad = 100.0 * ngood / num_elements;
    printf("   Percentage of stars with no flags: %6.1f\n",perc_bad);   
    perc_bad = 100.0 * nbad / num_elements;
    printf("   Percentage of stars with bad widths: %6.1f\n",perc_bad);
#endif

    if (ngood == 0) {
      shError("WARNING: NO stars passed ps rejection stage in filter %s",
	      filter);
    } else if (ngood < min_starnum) {
      shError("WARNING: Only %d stars passed ps rejection stage in filter %s", ngood, filter);
    }
  }

  /*  clean up  */
  shFree(sort_array);
  shChainCursorDel(star1clist,crsr);

  return(SH_SUCCESS);
}


