/*
 * Utilities for extract; slightly modified from mirage
 */
#include <stdio.h>
#include <math.h>
#include <alloca.h>
#include "dervish.h"
#include "atConversions.h"
#include "phConsts.h"
#include "phExtract.h"
#include "phExtract_p.h"

/*****************************************************************************/
/*
 * Given a float *d, calculate *d and i s.t. i + *d == del,
 * where *d is in range [-0.5,0.5], and return i
 */
int
reduce_shift(float del, float *d)
{
   int i;

   del += 0.5;				/* so should be in [0,1] */
   if(del < 0) {
      i = -(int)(-del) - 1;
   } else {
      i = (int)del;
   }
   del -= i;				/* reduced to [0,1] */
   del -= 0.5;				/* reduced to [-0.5, 0.5] */
   
   shAssert(del >= -0.5 && del <= 0.5);
     
   *d = del;
   return(i);
}

/********************** COSYGEN ********************************/
/* 
 * generate a cos_belled sync filter to move an image from del to 
 * zero; del in the range -0.5 to +0.5; the filter is tapered with 
 * a cosine bell of half-length lbell (filter length 2*lbell - 1). 
 * lbell is silently truncated to 50 if you are silly enough to try 
 * to use one that big. filt is an array which dimension >= 2*lbell-1
 */

void 
get_sync_with_cosbell(float *filt, int lbell, double del)
{
    int len;
    int k,j;
    float sum;
    int joff;
    double sgn;
    float bellf;
    float jd;
    const double flbell = M_PI/(double)lbell;

   shAssert(del >= -0.5 && del <= 0.5);    
/*
 * generate the coefficients
 */
    if(lbell > 50) lbell = 50;
    if(fabs(del) < 1.e-6) del = 1.e-6;
    
    len = 2*lbell-1;
    joff = lbell-1;
    sgn = (joff & 1) ? del : -del;

    for(k=0;k<len;k++){
        j=k-joff;       /* j = 0 at center of array, -/+ lbell-1 at ends */
        jd = (float)j - del;
        bellf = 0.5*(cos(jd*flbell) + 1);
        filt[k] = (sgn/jd)*bellf; 
        sgn = -sgn;
    }
/*
 * normalise filter to unity
 */
    sum = 0;
    for(k=0;k<len;k++){
        sum += filt[k];
    }

    for(k=0;k<len;k++){
        filt[k] /= sum;
    }
}

/******* HISTOGRAMMING/SORTING STUFF ********************************/


/*************** SHSHSORT() *****************************************/
/* sort a PIX array in place using Shell's method. Adapted from
 * Numerical Recipes
 */

static void
shshsort(PIX *arr, int n)
{
    unsigned i, j, inc;
    unsigned t;
    inc=1;
    do{
        inc *= 3;
        inc++;
    }while(inc <= n);
    do{
        inc /= 3;
        for(i=inc;i<n;i++){
            t = arr[i];
            j=i;
            while(arr[j-inc] > t){
                arr[j] = arr[j-inc];
                j -= inc;
                if(j<inc) break;
            }
            arr[j] = t;
        }
    }while(inc > 1);
}

/***************** SHINSORT() *****************************************/
/* sort a PIX array in place using straight insertion.
 * From Numerical recipes.
 */
 
static void
shinsort(PIX *arr,int n)
{
    register int i,j;
    register unsigned a;
    
    for(j=1;j<n;j++){
        a = arr[j];
        i = j-1;
        while(i>=0 && arr[i] > a){
            arr[i+1] = arr[i];
            i--;
        }
        arr[i+1] = a;
    }
}        

/********************** QHIST() *************************************/
/* makes histogram from a PIX array data of length n. The histogram
 * is placed in the PIX array hist, nominally of size 0xffff shorts.
 * The user is responsible for clearing (the relevant part of) the histogram
 * array before calling this routine, AND for ensuring that there are no
 * more than 65535 elements of any given value.
 */
 
static void
qhist(register PIX *data,register int n,register U16 *hist)
{
   register PIX *end;

   for(end = data + n;data < end;data++) {
      hist[PIX2INT(*data)]++;
   }
}

/***************** QMAXMEAN() **********************************************/
/*
 * Calculate unclipped min/mean/range/sum of a U16 array.
 *
 * READS ntot,data.
 * WRITES rng,mean,min,sum. 
 *
 * This used to be a routine which tried to more efficient, at the expense 
 * of occasionally getting the bounds a bit wide. See version 1.25 and 
 * earlier for details.
 */
 
void
region_maxmean_crude(struct pstats *ps)
{
    register U16 c;
    register U16 *ptr = ps->data, *pend;
    register U16 min, max;
    register int sum;
    int n = ps->ntot;
    
    shAssert(n > 0);

    min = max = *ptr;
    sum = 0;
    for(pend = ptr + n;ptr < pend;ptr++){
        c = *ptr;
        sum += c;
        if (c < min) min = c;
        if (c > max) max = c;
    }
    ps->min = min;
    ps->sum = sum;
    ps->mean = (float)sum/(float)n;
    ps->rng = max - min;

    ps->rng++;				/* min == max => upper quartile == max + 025, so need extra value */

    if (ps->min + ps->rng > MAX_U16) {
	ps->rng--;
    }
    shAssert(ps->min + ps->rng <= MAX_U16);
}

/********************** PSHQTILE() *************************************/
/*
 * Calculates QUARTILES from a PIX histogram, which it builds.
 * READS ntot,min,rng from pstat
 * WRITES quartiles and sigma and modifies flg
 */

/*#define HDEBUG*/

void 
get_quartiles_from_array(U16 *hist,struct pstats *ps)
{
    register U16 *sp ;
    register int b1, sum;
    int i, bound;
    float fbound;
    
    int n = ps->ntot;
    PIX min = ps->min;
    U16 *histo = hist + ps->min;
    float *qt = ps->qt;
    
    /* build histogram */
    qhist(ps->data,n,hist);
        
    sp = histo-1;
    sum = 0;   /* NB!!! the ASSUMPTION is that this is a good approximation*/
    for(i=0;i<3;i++){
        fbound = 0.25*((float)((i+1)*n)); 
        bound = fbound;
        b1 = bound+1;
        if(b1 > n) b1 = n;    /* do not look for more than we have */
        
        while( sum < b1 ) {   /* find the rough quartile */
            sum += *(++sp) ;
        }
        /* at this point, the histogram first sums to a number which is 
         * greater than the bound, being at least equal to bound + 1, AND 
         * the sum is greater than that in the previous cell. Thus the 
         * sum at the previous cell is less than or equal to bound, and 
         * the quartile MUST lie in the current cell. We linearly 
         * interpolate to find the float qtile. Note that the cells are 
         * deemed to span the range (value - 1/2,value + 1/2), so that 
         * if the bound is exactly met by the sum at some cell, the qtile
         * is at the index + 0.5.
         */

        qt[i] = (float)(min + (sp-histo)) + 0.5 
                           - ((float)sum-fbound)/((float)(*sp));

        /* have evaluated quartile. to begin next one, GO BACK one
         * step; this is necessary to handle 'spike' histograms, for which
         * all quartiles lie in the same cell.
         */

        sum -= *sp; sp--;
    }

    ps->sig = IQR_TO_SIGMA*(qt[2] - qt[0]);
    ps->flg |= (EXTRACT_HIST_SORT | EXTRACT_SIGIQR);
}


/********************** PSHMQTILE() *************************************/
/*
 * QUARTILES the 'main body' histogram (+/- PROFILE_NSIGMA sigma; i.e.
 * the central 98% if PROFILE_NSIGMA == 2.3 and the data's Gaussian)
 * of an array. The lower limit is set at the PROFILE_NTILE quantile,
 * so we tacitly assume that the serious deviations from normality are
 * for positive fluctuations, though if the PROFILE_NSIGMA sigma point
 * is above the PROFILE_NTILE quantile, we use it instead. (if BELOW,
 * we do NOT correct, but use the quantile.) This routine can be used
 * safely for small histograms, but in this case the second pass
 * usually does nothing and is skipped.
 *
 * The sigma clipping is done using a real sigma, not one based on
 * the quartiles (think about a histogram with a sharp core containing
 * 51% of the pixels and wide wings; almost all of the wing pixels
 * would be clipped by a sigma based on the interquartile range)
 *
 * READS ntot,min,rng from *ps
 * WRITES quartiles, mean, min, rng, nel and modifies flg
 */
void 
get_quartiles_from_array_clipped(U16 *hist,struct pstats *ps,
				 int band, /* band data's taken in */
				 float fxc, float fyc, /* centre of obj */
				 const struct cellgeom *cgeom) /* geom */

{
    register U16 *sp ;
    register int b1, sum;
    int i, bound;
    float fbound;
    float clipval;			/* how many DN from median to clip */
    int mosum,tsum;
    U16 *mhist,*top,*bottom;
    int pass;
    int n = ps->ntot;
#if 0
    const int initial_min = ps->min;	/* yes; the initial value of min */
    const int initial_rng = ps->rng;	/* miracles! the initial value of rng*/
#endif
    int min = ps->min;
    float *qt = ps->qt;
    
    /* build histogram */
    qhist(ps->data,n,hist);
    
    sp = hist + min;
    sum = 0;
    
    /* look for PROFILE_NTILE quantile */
    fbound = PROFILE_NTILE*(float)n;
    b1 = fbound + 1;

    while(sum < b1) sum += *(sp++);
    /* sum is now sum through cell containing PROFILE_NTILE quantile
     * and sp is ptr to next cell; we want sum not including that cell
     * and pointer to that cell, so
     */
    sp--;
    sum -= *sp; 
    mhist = sp;   /* pointer to histog cell containing PROFILE_NTILE quantile*/
    mosum = sum;  /* sum to this point */  
        
    top = NULL;			/* make compiler happy; it's set if pass==0 */
    for(pass=0;pass < 2; pass++){   
    
        /* find quartiles */
        for(i=0; i<3; i++){
            fbound = 0.25*((float)((i+1)*n)); 
            bound = fbound;
            b1 = bound+1;
            if(b1 > n) b1 = n;    /* do not look for more than we have */
        
            while( sum < b1 ) {   /* find the rough quartile */
                   sum += *(sp++) ;
            }
            /* at this point, the histogram first sums to a number which is 
             * greater than the bound, being at least equal to bound + 1, AND 
             * the sum is greater than that in the previous cell. Thus the 
             * sum at the previous cell is less than or equal to bound, and 
             * the quartile MUST lie in the current cell. We linearly 
             * interpolate to find the float qtile. Note that the cells are 
             * deemed to span the range (value - 1/2,value + 1/2), so that 
             * if the bound is exactly met by the sum at some cell, the qtile
             * is at the index + 0.5. Note that sum is the sum THROUGH the
             * current cell (pointed to by sp-1; the pointer stays one ahead
             * of the sum, and points to the NEXT cell to be summed).
             */
	    shAssert(sp[-1] > 0);
            qt[i] = (float)(sp - hist -1) + 0.5 - ((float)sum - fbound)/sp[-1];
		
            /* have evaluated quartile. to begin next one, GO BACK one
             * step; this is necessary to handle very narrow histograms,
             * for which more than one quartile may lie in one cell.
             */
            sp--;
            sum -= *sp;
        }

	if (min + ps->rng < qt[2]) {	/* ensure that rng covers the upper quartile */
	    fprintf(stderr, "min + rng doesn't cover upper quartile: %d %g\n", min + ps->rng, qt[2]);
	    ps->rng = (int)(qt[2] + 1) - min;
	}

        if(pass == 0){
            /*
	     * 'trim' the histogram to the range which would
             * correspond to +/- PROFILE_NSIGMA sigma if Gaussian; we
             * have done this (provisionally) on the low side by
             * finding the PROFILE_NTILE quantile. We do it on the
             * high side by going to the histogram cell which would be
             * PROFILE_NSIGMA sigma for a gaussian, calculated by use
             * of the interquartile range.
	     *
	     * When processing children, the other children are replaced
	     * by the value SOFT_BIAS; this can confuse the estimation of
	     * `sigma' so set that element of the histogram to the average
	     * of its neighbours.
             */
#if 1
	   float mean;			/* naive mean of histogram */
	   double dsum;			/* a floating point accumulator */
	   int i;
	   int nhist;			/* number of pixels with this DN */
	   int npix;			/* number of pixels in histogram */
	   int rng = ps->rng;

	   shAssert(n > 1);
	   
	   dsum = 0; npix = 0;
	   for(i = min; i < min + rng; i++) {
	      nhist = (i != SOFT_BIAS) ?
				       hist[i] : (hist[i - 1] + hist[i + 1])/2;
	      npix += nhist;
	      dsum += i*nhist;
	   }

	   if(npix == 0) {
	       mean = ps->data[0];
	   } else {
	      mean = dsum/npix;
	   }

	   dsum = 0;
	   if(npix > 1) {
	      for(i = min; i < min + rng; i++) {
		 nhist = (i != SOFT_BIAS) ?
				       hist[i] : (hist[i - 1] + hist[i + 1])/2;
		 dsum += (i - mean)*(i - mean)*nhist;
	      }
	      dsum /= (npix - 1);
	   }
	   /*
	    * If almost all the pixels are SOFT_BIAS ignoring them isn't a smart thing
	    * to do, and indeed gets us into trouble (cf. PR 7609)
	    */
	   if (npix < 0.01*n) {		/* XXX 0.01 */
	       clipval = PROFILE_NSIGMA*IQR_TO_SIGMA*(qt[2] - qt[0]);
	       mean = qt[1];
	   } else {
	       clipval = PROFILE_NSIGMA*sqrt(dsum);
	   }
	   top = hist + 1 + (int)(mean + clipval + 0.5);
#else
	    clipval = PROFILE_NSIGMA*IQR_TO_SIGMA*(qt[2] - qt[0]);
            top = hist + 1 + (int)(qt[1] + clipval + 0.5);
#endif
            /* top points to the first cell BEYOND the cell containing
             * the PROFILE_NSIGMA sigma point. Note that cell i spans
             * i-0.5 to i+0.5, so the value 0.51 lies in cell i+1, and
             * the round is appropriate.  we will sum cells BELOW top.
             */
	    if(top < sp) {		/* don't clip pixels that we've
					   already counted; PR 4983 */
	       top = sp;
	    }
	   
            while(sp < top && sum < n) sum += *(sp++);
            tsum = sum;
            if(sum == n){
                /* have exhausted histogram */
                top = sp; /* all there is */
                if(mosum == 0){
                    /* histogram is entirely contained between 
                     * PROFILE_NTILE quantile and +PROFILE_NSIGMA sig pt
                     */ 
#if 0					/* JEG correction; fails assertion */
                    ps->nel = top - mhist;
#else
                    ps->nel = n;
#endif
                    ps->sig = IQR_TO_SIGMA*(qt[2] - qt[0]);
                    break;              /* nothing more to do; whole
                                           hist is in reduced range */
                }
            }
            /* check for lower tail */
#if 1
            bottom = hist + (int)(mean - clipval + 0.5);
#else
            bottom = hist + (int)(qt[1] - clipval + 0.5);
#endif
            /* cell containing -PROFILE_NSIGMA sigma point */
            if(bottom > mhist){  /* there is a lower tail; fixup */
                sp = mhist;
                sum = mosum;
                while(sp < bottom) sum += *(sp++);
                sp--;
                sum -= *sp ;
                mosum = sum ;
                    /* sum below cell containing -PROFILE_NSIGMA sigma point */
                mhist = sp ;
                    /* pointer to cell containing -PROFILE_NSIGMA sigma point*/
            }

            /* update structure */
	    ps->rng = top - mhist;
            ps->min = min = mhist - hist;
	    n = ps->nel = tsum-mosum;
            /* go back around with trimmed histogram */
            sp = hist + min;
            sum = 0;
        }else{
	   /* successful trim; modify flags and reevaluate sigma */
#if 0
	   if(cgeom != NULL && ps->ntot - ps->nel > 0.005*ps->ntot) {
 	      fprintf(stderr, "show_sector \""
		      "%d  %7.3f %7.3f  %5.1f %5.1f %7.4f %7.4f  %d %d %.1f"
		      "\"\n", band, fyc, fxc, cgeom->inner, cgeom->outer,
		      cgeom->cw, cgeom->ccw,
		      ps->ntot - ps->nel, ps->ntot, ps->qt[1] - SOFT_BIAS);
	   }
#else
	   shAssert(band ==band && fxc == fxc && fyc == fyc && cgeom == cgeom);
#endif
	   ps->flg |= EXTRACT_CLIPPED;
	   ps->sig = IQR_TO_SIGMA_NSIGMA_CLIPPED*(qt[2] - qt[0]);
        }
    }
    /* evaluate sum, mean */
    n = sum = 0;    
    for(i = min; i < top - hist; i++) {
	n += hist[i];
	sum += i*hist[i];
    }
    ps->sum = sum;
    ps->mean = (float)sum/(float)n;
    ps->flg |= (EXTRACT_HIST_SORT | EXTRACT_SIGIQR);
    shAssert(n == ps->nel);
}

/******************* SHINSTATS() ******************************************/
/*
 * sorts an array by Shell or insertion sort, whicever is deemed faster, and
 * finds the mean and quartiles.
 *
 * If the number of points is less than MBTHRESH (or MBTHRESH is -ve),
 * it uses all the data; otherwise it finds the quartiles for the main
 * body of the histogram, and reevaluates the mean for the main body.
 *
 * This routine is normally used in the inner cells, where no trimming is
 * used as there are too few points to play rejection games, or the outer
 * cells if it is deemed that Shell sort would be faster than the histogram
 * sort. In that case, the mean would already have been determined with
 * region_maxmean_crude, and WE DO NOT RECOMPUTE IT HERE; if ps->sum is
 * nonzero, the mean calculation is SKIPPED.
 *
 * The quartile algorithm is the same as used in
 * get_quartiles_from_array() and returns identical results; the ansatz is
 * that the data are distributed uniformly in the histogram cells, and
 * the quartiles are thus linearly interpolated in the cumulative histogram.
 * This is about as good as one can do with dense histograms, and for sparse
 * ones is as good as anything.
 *
 *
 * READS only data pointer and ntot from the pstats;
 * WRITES quartiles,min,rng,and mean, and modifies flg, sorts data
 *
 * If it trims the data, also modifies nel
 */
  
void
shinstats(struct pstats *ps)   
{
    int i;
    double sum;
    register int np;
    register PIX *p;
    int ldex,udex;
    float fdex;
    float fldex;
    int npass;				/* how many passes? 2 => trim */
    int pass;				/* which pass through array? */
    int cdex;    
    int dcell;
    int dlim;
    float *qt = ps->qt;			/* unaliased variables */
    int n = ps->ntot;
    PIX *data = ps->data;
    
    npass = (MBTHRESH < 0 || n < MBTHRESH) ? 1 : 2;

    if(n<NINSERT){
        shinsort(data,n);
    }else{
        shshsort(data,n);
        ps->flg |= EXTRACT_SHELL_SORT;
    }

    for(pass=0;pass < npass;pass++){
       ps->min = data[0];
       ps->rng = data[n-1] - data[0];

       for(i = 0;i < 3;i++) {
	  fdex = 0.25*(float)((i+1)*n);	/*float index*/
	  cdex = fdex;
	  dcell = data[cdex];
	  ldex = cdex;
	  if(ldex > 0) {
	     while(data[--ldex] == dcell && ldex > 0) continue;
	     /* ldex is now the last index for which data<cdex */
	     
	     if(ldex > 0 || data[ldex] != dcell) {
		/* we stopped before the end or we stopped at the
		 * end but would have stopped anyway, so bump it up; 
		 */
		ldex++;
	     }
	  }
	  /* The value of the cumulative histogram at the left edge of the
	   * dcell cell is ldex; ie exactly ldex values lie strictly below
	   * dcell, and data=dcell BEGINS at ldex.
	   */
	  udex = cdex;
	  while(udex < n && data[++udex] == dcell) continue;
	  /* first index for which data>cdex or the end of the array, 
	   * whichever comes first. This can run off the end of
	   * the array, but it does not matter; if data[n] is accidentally
	   * equal to dcell, udex == n on the next go and it falls out 
	   * before udex is incremented again. */
	  
	  /* now the cumulative histogram at the right edge of the dcell
	   * cell is udex-1, and the number of instances for which the data
	   * are equal to dcell exactly is udex-ldex. Thus if we assume
	   * that the data are distributed uniformly within a histogram
	   * cell, the quartile can be computed:
	   */
	  fldex = ldex; 
	  
	  shAssert(udex != ldex);
	  qt[i] = dcell - 1 + 0.5 + (fdex - fldex)/(float)(udex-ldex);
	  
	  /* The above is all OK except for one singular case: if the
	   * quartile is EXACTLY at a histogram cell boundary (a half-integer) 
	   * as computed above AND the previous histogram cell is empty, the
	   * result is not intuitively correct, though the 'real' answer 
	   * is formally indeterminate even with the unform-population-in-
	   * cells ansatz. The cumulative histogram has a segment of zero
	   * derivative in this cell, and intuitively one would place the
	   * quartile in the center of this segment; the algorithm above
	   * places it always at the right end. This code, which can be
	   * omitted, fixes this case.
	   *
	   * We only have to do something if the quartile is exactly at a cell
	   * boundary; in this case ldex cannot be at either end of the array,
	   * so we do not need to worry about the array boundaries .
	   */
	  if(4*ldex == (i+1)*n) {
	     int zext = dcell - data[ldex-1] - 1;
	     
	     if(zext > 0) {
		/* there is at least one empty cell in the histogram
		 * prior to the first data==dcell one
		 */
		qt[i] -= 0.5*zext;
	     }
	  }
       }

       if(npass == 1) {			/* no trimming to be done */
	  ps->sig = IQR_TO_SIGMA*(qt[2] - qt[0]);
       } else {
	  /*
	   * trim the histogram if possible to the first percentile below
	   * and the +PROFILE_NSIGMA sigma point above
	   */
	  if(pass==0){
	     /* terminate data array--there must be room */
	     data[n] = 0x7fff;
	     /* trim histogram */
	     ldex = PROFILE_NTILE*n;	/* index in sorted data array at
					   quantile corresponding to
					   -PROFILE_NSIGMA */
	     dlim = qt[1] + PROFILE_NSIGMA*IQR_TO_SIGMA*(qt[2] - qt[0]) + 0.5;
	     if(dlim >= data[n-1] || udex >= n) {  /* off top of data or
						      already at end */
		if(ldex == 0) {
		   ps->sig = IQR_TO_SIGMA*(qt[2] - qt[0]);
		   break;		/* histogram is too small; we're done*/
		}
	     } else {
		/* find the index corresponding to PROFILE_NSIGMA
		 * sigma; this should be done by a binary search */
		udex--; 
		while(data[++udex] <= dlim){;}
		n = ps->nel = udex - ldex;
		data = data + ldex;
		ps->flg |= EXTRACT_CLIPPED;
	     }
	  }else{   /* have trimmed hist and recomputed quartiles */
	     ps->sig = IQR_TO_SIGMA_NSIGMA_CLIPPED*(qt[2] - qt[0]);
	  }
       }
    }

    ps->flg |= EXTRACT_SIGIQR;

    /* do the mean if we do not already have it or we have trimmed the hist */
    if(ps->sum == 0 || (npass == 1 || pass == 1)) {
        sum = 0;
        np = n;
        p = data;
        while(np--){
            sum += *p++;
        }
        ps->sum = sum;
        ps->mean = (float)sum/n;
    }
}    

/********************** SIMPLESTATS() **************************************/
/*
 * routine to do simple inner cells, with numbers (4 or fewer) too small
 * for quartiling. Here we do straight moments, and fake the quartiling.
 * the standard deviation and iqr are set to zero for cells with only one
 * element.
 *
 * READS only data pointer and ntot from the pstats;
 * WRITES quartiles,min,rng,and mean, and modifies flg.
 *
 */
void
simplestats(struct pstats *ps)
{
    const int npt = ps->ntot;
    const float fnpt = (float)npt;
    register int n;
    register PIX *p;
    register float sum=0,sum2=0;
    register PIX c;
    float mom2,mean;
    PIX max=0, min=MAX_U16;
    float var;
    
    n = npt;
    p = ps->data;
    while(n--){
        sum += (c=*p++);
        sum2 += c*c;
        if(c<min) min = c;
        if(c>max) max = c;
    }
    ps->min = min;
    ps->rng = max-min;
    ps->mean = mean = (float)sum/fnpt;
    mom2 = (float)sum2/fnpt;
    if(npt>1){
        var = mom2-mean*mean;
        if(var < 0.) var = 0.;
        mom2 = sqrt((var)*fnpt/(fnpt-1.));
    }else{
        mom2 = 0;
    }
    ps->sig = mom2;
    ps->qt[0] = mean - IQR_TO_SIGMA/2*mom2;
    ps->qt[1] = mean;
    ps->qt[2] = mean + IQR_TO_SIGMA/2*mom2;
    ps->nel = npt;
    ps->sum = sum;
    ps->flg |= EXTRACT_FAKEQT;
}

/*****************************************************************************/
/*
 * set all values high for a bad cell (usually too close to edge of frame)
 */
void
badstats(struct pstats *ps)
{
   int big = (1 << 16) - 1;
    
   ps->min = ps->rng = big;
   ps->mean = ps->sig = ps->qt[1] = ps->qt[2] = big;
   ps->qt[0] = -big;			/* invalidate IQR too */
   ps->area = ps->nel = -1;
   ps->flg |= EXTRACT_BAD;
}
