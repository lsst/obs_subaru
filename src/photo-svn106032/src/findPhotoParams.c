
/***************************************************************************************************************************************************************
** <AUTO>
** FILE:
**
**	findPhotoParams.c
**
** ABSTRACT:
**	The routines in this module will take the MT pipeline output
**	and produce photometric parameters per frame.
**  
** </AUTO>
**
******************************************************************************
Assumptions:  (check these are up to date)

EXTINCTIONlist sorted by time
can have a patch containing only 1 starc
each calib1bytime contains values for all filters
Filter order and index is fixed, even if a particular struct has no value for a particular color. (ie if and extinction has no u' value, then the u=null and I call still refer to the others by the right order in the array.
*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <alloca.h>
#include "dervish.h"
#include "sdssmath.h"
#include "phFindPhotoParams.h"
#include "phFilter.h"
#include "phFramestat.h"
#include "phUtils.h"
#include "phMathUtils.h"

#define INTERP_ONE_COLOR 0	/* flags for how much extinction to interp. */
#define INTERP_ALL_COLORS 1

#define SEC_PER_DEGREE 3600


#define DUMP_STARS 1           /* true to dump info on calibration stars */

/*********************** statics local to this module ************************/

static double *ext_time[MAX_FILTERS];  /* Arrays containg t in seconds  */
static float *ext_coeff[MAX_FILTERS]; /* Array containing K for this color */
static float *ext_err[MAX_FILTERS];   /* Array containing errK for this color*/
static int nk[MAX_FILTERS];	      /* Number of element in ext arrays */

static double *cal_time[MAX_FILTERS];
static float *cal_flux20[MAX_FILTERS];
static float *cal_flux20Err[MAX_FILTERS];
static int ncal[MAX_FILTERS];

static float m_flux20[MAX_FILTERS];    /* Slope m for flux20(t) = m*t + c */
static float m_err_flux20[MAX_FILTERS];    /* Error for slope m */
static float c_flux20[MAX_FILTERS];    /* Intercept c for  flux20(t) = m*t + c */
static float c_err_flux20[MAX_FILTERS];    /* Error for intercept c */

static void 
find_clipped_mean(const float *data, const float *errors, int ndata, float nsigma,
		  int maxiter, float *mean, float *sigma, float *chi2);

/*****************************************************************************/

/*
 * This function fits a straight line to the data, y(x) = mx + c, determines
 * errors for m and c, and calculates chi2 when errors are given, or 
 * correlation coefficient when errors are NULL.
 */

static void 
fit_straight_line(const double *x,	/* the x array of data*/
                  const float *y,	/* the y array of data */
		  const float *errors,	/* data (y) errors, can be NULL */
		  int n,		/* number of data points */
		  float *m,             /* slope */
		  float *m_err,         /* slope error*/
		  float *c,             /* intercept */   
		  float *c_err,         /* intercept error */          
		  float *chi2)		/* chi2 if errors are given, */     
                                        /* if no error then correlation coefficient */
{ 
   int i;              
   double alpha, beta, gamma, p, q, z;	/* follows notation by Sivia (ZI)*/
   float weight;			/* weight of a data point */
   double det, s = 0.0;

   if (n == 1) {
      *m = *m_err = 0;
      *c = y[0];
      *c_err = errors[0];
      *chi2 = 0;
      return; 
   }
 
   /* make sure there are no errors equal to 0 */
   if (errors != NULL) {
       for (i = 0; i < n; i++) {    
           if (errors[i] == 0) {
              errors = NULL; 
              break;
           }
       }
   }   

   alpha = beta = gamma = p = q = z = 0.0;	
   for (i = 0; i < n; i++) {
        weight = errors == NULL ? 1 : 1/pow(errors[i],2);
        alpha += weight*x[i]*x[i];
        beta += weight;
        gamma += weight*x[i];
        p += weight*x[i]*y[i];
        q += weight*y[i];
        z += weight*y[i]*y[i];  
   }

   /* calculate the slope and intercept */
   det = alpha * beta - gamma * gamma;
   /* EPSILON is the "smallest double" */
   if (fabs(det) <= EPSILON) {
     /*  shFatal("fit_straight_line: |det| is < EPSILON.\n"); */
     /* temp hack XXX */ 
     find_clipped_mean(y,errors,n,3.0,2,c,c_err,chi2);
     *m = 0.0; 
     *m_err = 0.0;    
   } 
   *m = (beta * p - gamma * q) / det;
   *c = (alpha * q - gamma * p) / det;
      
   /* calculate the slope and intercept errors */ 
   if (errors == NULL) {
      *m_err = sqrt(((n*z*z-q*q)/(n*alpha-gamma*gamma) - (*m) * (*m))/(n-1));
      *c_err = *m_err * sqrt(alpha/n);
      /* calculate correlation coefficient */
      s = sqrt(n*alpha-gamma*gamma) * sqrt(n*z-q*q);  
      s = (n * p - q * gamma) * n / (n-1) / s;
   } else {
      *m_err = sqrt(beta / det / 2);
      *c_err = sqrt(alpha / det / 2);
      /* calculate chi2 */
      s = 0;
      for (i = 0; i < n; i++) {
         s += pow((*m * x[i] + *c - y[i])/errors[i],2);         
      }
      s /= n - 1;
   } 
   *chi2 = s;

   return;
  
}
   
 
/*
 * This function calculates a clipped weighted average of the data in
 * the input array.
 */

static void 
find_clipped_mean(const float *data,	/* the array of data to be averaged */
		  const float *errors,	/* data errors, if NULL don't weight */
		  int ndata,		/* number of data points */
		  float nsigma,		/* number of sigma for clipping */
		  int maxiter,		/* maximum number of iterations */
		  float *mean,		/* O: clipped and possibly weighted
					   mean */
		  float *sigma,		/* O: error in mean */
		  float *chi2)		/* O: chi2 for the clipped sample, (if errors != NULL) */
{
   int i;
   int num;				/* number of acceptable data points */
   int niter;				/* number of iterations */
   int lastnum = 0;			/* value of num on last iteration */
   double s=1.0;			/* estimate of population s.d. */
   double sum, sum2;			/* double as calculation of variance
					   subtracts large numbers */
   double sumw, sumw2;			/* sum(weights), sum(weights^2) */
   float max=FLT_MAX, min=-FLT_MAX;	/* allowable range of values of data */
   float weight;			/* weight of a data point */
   double xbar;				/* estimate of population mean */
   float *clipped_array;                /* array of surviving values */
   float *sort_array;	                /* array to hold sorted values */
   float median, sigma_quart = 0;           /* statistics for clipped values */
   float aux=1.0;

   if (ndata == 1) {
      *mean = data[0];
      *sigma = errors[0];
      if (errors[0] > 0) {
         *chi2 = data[0]/errors[0];
      } else {
         *chi2 = -1;
      }
      return; 
   }

   sort_array = (float *)shMalloc(ndata * sizeof(float)); 
   clipped_array = (float *)shMalloc(ndata * sizeof(float)); 

   for(niter = 0;niter < maxiter;niter++) {
      sum = sum2 = sumw = sumw2 = num = 0;
      for (i = 0; i < ndata; i++) {
	 if(data[i] <= max && data[i] >= min) {
            clipped_array[num] = data[i];
	    num++;
	    
	    weight = errors == NULL ? 1 : 1/(1e-10 + pow(errors[i],2));
	    sumw += weight;
	    sumw2 += weight*weight;
	    sum += weight*data[i];
	    sum2 += weight*data[i]*data[i];
	 }
      }
      
      if(niter >= 1 && (num == lastnum || num == 1)) { /* no change */
	 break;
      }

      if(num == 0 || fabs(sumw2 - pow(sumw,2)) < EPSILON) {
	 if(niter == 0) {		/* illegal inputs */
	    shFatal("find_clipped_mean: input data has <= 1 effective point"
		    "sum(w) = %g",sumw);
	 }
	 break;				/* cannot estimate sigma */
      }
      
      xbar = sum/sumw;	/* n.b. xbar is a double */
		
      if (niter == 0 || num > 1) { 
         s = sum2/sumw - xbar*xbar;	/* biased variance */
         s /= 1 - sumw2/pow(sumw,2);	/* unbiased variance */
         s = sqrt(s);
         if (s != s) {
            if (sumw2 > 0) aux = sumw / sqrt(sumw2);
            if (aux != aux) {
	       shError("find_clipped_mean: something is not kosher with calibration.\n");
               s = sigma_quart;
            } else {
               s = *sigma * aux;
            }
         }
      }

      if (errors != NULL && chi2 != NULL && num > 1) {
          for (i = 0; i < ndata; i++) {
             aux = 0;
	     if(data[i] <= max && data[i] >= min ) {
                aux += (data[i]-xbar)*(data[i]-xbar)/errors[i]/errors[i];
	     }
          }
          *chi2 = aux / (num-1);
      }

#if 1
      memcpy(sort_array,clipped_array,num*sizeof(float));
      phFloatArrayStats(sort_array, num, 0, &median, &sigma_quart, NULL, NULL);
      max = median + nsigma*sigma_quart;
      min = median - nsigma*sigma_quart;
#else 
      max = xbar + nsigma*s;
      min = xbar - nsigma*s;
#endif
      
/*
 * we want the error in the mean, not the s.d. of the underlying population.
 */
      *mean = xbar;
      *sigma = s*sqrt(sumw2)/sumw;
      
      lastnum = num;
   }
/*
 * we should really correct sigma for the clipping, but this is a small effect
 */
   ;

   /*  clean up  */
   shFree(sort_array);
   shFree(clipped_array);
}

/*----------------------------------------------------------------------*/
/* This function gets the first patch from the patch chain.
   The mtindex array will have ncolors elements set. Each element is
   the location in the arrays in MT patch  of the
   data for the corresponding filter in the starlist. 
   The extinctions selected to match the starlist when read in in tcl.
*/

static int
get_mtindex(int *mtindex, int *refindex, int ncolors,char *filters[], 
	    char *ref_filters[], CHAIN *patches) 
{
    int i,j,foundit;
    CHAIN *patch;
    STARC *starc;

    patch = (CHAIN *) shChainElementGetByPos(patches,0);
    if ((starc = (STARC *) shChainElementGetByPos(patch,0)) == NULL) {
	shError("phFndPhotoParams: first patch contains no stars");
	return 0;
    }
    for (i = 0; i < ncolors; i++) {

	/* Get index for starc main color */
	foundit = 0;
	for (j = 0; j < starc->ncolors; j++) {
	    if (!strcmp(starc->mag[j]->passBand,filters[i])) {
		mtindex[i] = j;
		foundit = 1;
		continue;		
	    }
	}
	if (!foundit) {
	    shError("findPhotoParams: cannot find filter %s in starclist",
		    filters[i]);
	      return 0;
	}

	/* Get index for starc refcolor */
	foundit = 0;
	for (j = 0; j < starc->ncolors; j++) {
	    if (!strcmp(starc->mag[j]->passBand,ref_filters[i])) {
		refindex[i] = j;
		foundit = 1;
		continue;		
	    }
	}
	if (!foundit) {
	    shError("findPhotoParams: cannot find filter %s in starclist",
		    ref_filters[i]);
	      return 0;
	}
    }
    return 1;
}
/*----------------------------------------------------------------------*/
/* Returns the frameinfo in the chain that has the field=id */

static FRAMEINFO *
findFrameinfo (int field, CHAIN *frames)
{

    CURSOR_T crsr;
    FRAMEINFO *frame;

    crsr = shChainCursorNew(frames);
    while((frame = (FRAMEINFO *) shChainWalk(frames,crsr,NEXT)) != NULL) {
       if(frame->field == field) {
	  break;
       }
    }
    shChainCursorDel(frames,crsr);

    if(frame == NULL) {
       shError("ffp: no frameinfo with field %d in input chain",field);
    }
    
    return (frame);
}

/*---------------------------------------------------------------------*/
/* Linear interpolation with error estimated from error in input data
 * points Currently we assume the error is in y only. The calculated
 * error is compared with the difference between the linear
 * interpolation and a 4-point polynomial interpolation. If the
 * difference is larger than the calcuated error, a warning is
 * printed. */
static void
interp_with_errors(double *xx,	/* array of x data */
		   float *yy,   /* array of y data */
		   float *sigy, /* errors in y data */
		   int n,	/* number of points in array */
		   double x,	/* point at which to interpolate */
		   float *y,	/* result */
		   float *dy	/* error in result */
		   )
{
    
    int high, low, mid;
    float w;


    low = 0;
    high = n-1;

    if (x < xx[0]) {		/* We're off the bottom, must extrapolate */

	high = 1;
    /* it is safer to take the boundary value (ZI) */
        *y = yy[low];
        *dy = sigy[low];
        return;

    } else if (x > xx[n-1]) {	/* We're off the top, must extrapolate*/

	low = n-2;
    /* it is safer to take the boundary value (ZI) */
        *y = yy[high];
        *dy = sigy[high];
        return;

    } else { 	/* Now xx[low] <= x < xx[high] */

	
	while (high - low > 1) {
	    mid = (high + low) / 2;
	    if (xx[mid] > x) {
		high = mid;
	    } else { 
		low = mid;
	    }
	}
    }
    /* Sometimes during testing, we may accidently use an oldcal that was 
     *  output from the current run. This will catch it 
     */
    if(fabs(xx[high] - xx[low]) <= 1e-10*xx[low]) {
       w = 1;
    } else {
       w = (x - xx[high])/(xx[low] - xx[high]);
    }
    *y = w * yy[low] + (1-w) * yy[high];
    *dy = sqrt(pow(w * sigy[low],2) + pow((1-w) * sigy[high],2));

}


/*---------------------------------------------------------------------*/
/*
 * <AUTO EXTRACT>
 *
 * Matches star1pc's and starc's with separations < starsep
 * Doesn't check for duplicates. 
 *
 * returns:
 *   pointer to CHAIN of STAR1STARCMERGE    contains matched stars
 *   NULL                                   if fails
 */

CHAIN *
matchPatch(CHAIN *patch,     /* points to list of STARCs */
	   CHAIN *pstamps,   /* post stamps in a particular band */
	   CHAIN *frames,    /* frameinfos (for time and color */
	   int  color,       /* index of star1 color in starc array */
	   int  color2,      /* index of refcolor in starc array */
 	                     /* If neither have std mags, dont try to match */
	   char *filter,     /* Passband corresponding to color (to check) */
	   char *filter2,    /* Passband corresponding to color (to check) */
	   float starsep,    /* distance allowed in matching stars */
	   float maxErr,     /* max. error in any mag. allowed for MT stars */
	   double *mergetime  /* average 2.5 meter obs time for matches */
           )
{
    double dra, ddec, sep, starsepSq;
    double time=0;
    CHAIN *mergedlist=shChainNew("STAR1STARCMERGE");
    STAR1STARCMERGE *match;
    STAR1PC *star1;
    STARC *starc;
    FRAMEINFO *frame = NULL;
    CURSOR_T patch_crsr, stamp_crsr;
    int matchcount=0;

    shAssert(patch != NULL && patch->type == shTypeGetFromName("STARC"));
    shAssert(pstamps != NULL && pstamps->type == shTypeGetFromName("STAR1PC"));
    shAssert(frames != NULL && frames->type == shTypeGetFromName("FRAMEINFO"));

    starsepSq = starsep*starsep;
    stamp_crsr = shChainCursorNew(pstamps);
    patch_crsr = shChainCursorNew(patch);
    while((starc = (STARC *)shChainWalk(patch,patch_crsr,NEXT)) != NULL) {
	/* Make sure the starc's do all have colors in the right order */
	shAssert(!strcmp(starc->mag[color]->passBand, filter));
	shAssert(!strcmp(starc->mag[color2]->passBand, filter2));

	if (starc->mag[color]->magErr < 0) {
	    shDebug(FPP_ERRLEVEL,
		    " matchPatch: no mag in color %d for starc %d\n",
		    color,starc->id);
	} else if (starc->mag[color2]->magErr < 0) { 
	    shDebug(FPP_ERRLEVEL,
		    " matchPatch: no mag in refcolor band for starc %d\n",
		     starc->id);
	} else if (starc->mag[color]->magErr > maxErr || starc->mag[color2]->magErr > maxErr) { 
	    shDebug(FPP_ERRLEVEL,
		    " matchPatch: mag. errors > %f for starc %d\n", maxErr, starc->id);
	} else { 

	    shChainCursorSet(pstamps,stamp_crsr,HEAD);
	    while((star1 = (STAR1PC *) shChainWalk(pstamps, stamp_crsr, 
						 NEXT)) != NULL) {
       
                dra = starc->ra*cos(starc->dec/180*M_PI) - star1->ra*cos(star1->dec/180*M_PI);
                dra *= SEC_PER_DEGREE;
                ddec = (starc->dec - star1->dec)*SEC_PER_DEGREE; 
                sep  = dra*dra+ddec*ddec;
 
		if (sep <= starsepSq) {
 
		    /* Skip bad stars */
		    if ((star1->flags & STAR1_SATURATED) || 
		      (star1->flags & STAR1_NOAPMAG) || star1->apCounts <= 0)  {
		      continue;
	     	    }
 		    if (star1->apCountsErr > maxErr * star1->apCounts)  {
		      continue;
	     	    }

		    /* Get frame info for star1 */
		    frame=findFrameinfo(star1->frame,frames);
                    if (frame == NULL) {
                        shError("matchPatch: can't get frameInfo for frame %d",
                                    star1->frame);
                        shError("skipping star1 with ID %d", star1->id);
                        continue;
                    }
                    

		    /* Make a STAR1STARCMERGE */
		    match=phStar1starcmergeNew();
		    match->star1=star1;
		    match->starc=starc;
		    match->starcMagPos=color;
		    match->starcRefMagPos=color2;
		    match->airmass=frame->airmass;
		    match->mjd=frame->mjd;
		    match->sep = sqrt(sep);
		    /* Put it on list */
		    shChainElementAddByPos(mergedlist, match, 
					   "STAR1STARCMERGE", TAIL, AFTER);
		    matchcount++;
		    time += frame->mjd;
		}
	    }
	}
    }
    shChainCursorDel(patch, patch_crsr);
    shChainCursorDel(pstamps, stamp_crsr);
    shDebug(FPP_ERRLEVEL2,"Matchcount = %d",matchcount);

    if (matchcount > 0) {
        shAssert(frame != NULL);
	*mergetime = time/matchcount;
	shDebug(FPP_ERRLEVEL,"matchPatch: found matches in frame %d\n",
		frame->field);
	return mergedlist;
    } else {
	shDebug(FPP_ERRLEVEL,"matchPatch: No matches found for band %d",color);
	shChainDel(mergedlist);
	return NULL;
    }
}

/*----------------------------------------------------------------------*/
/* Assumes extinction list is sorted in time */


static void 
extlistToArrays(CHAIN *extinction, /* Input from MT */
		int ncolors,	   /* Number of colors with data */
		char *filters[]	   /* The filters we expect in the exts */
			   )
{
    int color;
    EXTINCTION *k;
    CURSOR_T crsr;
    
    crsr = shChainCursorNew(extinction);
    while((k = (EXTINCTION *) shChainWalk(extinction, crsr, NEXT)) != NULL) {

	for (color = 0; color < ncolors; color++) {
	    shAssert(k->k[color] != NULL);
	    shAssert(strcmp(k->k[color]->passBand, filters[color]) == 0);

	    if (k->k[color] != NULL && k->k[color]->magErr != BADVALUE) {
		
		ext_coeff[color][nk[color]] = k->k[color]->mag;
		ext_time[color][nk[color]] = k->mjd;
		ext_err[color][nk[color]] = k->k[color]->magErr;
		nk[color]++;
	    } 
	}
    }
    shChainCursorDel(extinction, crsr);
}

/*----------------------------------------------------------------------*/
    /* Interpolate through a list of extinction structs to make a new one 
     * for the input time. This assumes that the input list is sorted by time.
     * If only one struct is on the list, the procedure returns that struct
     * with a warning. It interpolates each color seperately and allows for
     * a null value in a particular color and struct. 
     */
static EXTINCTION *
extinctInterp(double mjd, int ncolor, int flag)
{

    int i;
    float mag, magerr;
    EXTINCTION *k=phExtinctionNew(MAX_FILTERS);

    /* ERRORS(NAN): ext err from interpolation */
    
    if (flag) {			/* Interpolate extinction for all bands */
	for (i = 0; i < ncolor; i++) {

	    if (nk[i] == 1) {

		k->k[i]->mag = ext_coeff[i][0];
		k->k[i]->magErr = ext_err[i][0];
		shDebug(FPP_ERRLEVEL,
			"extinctInterp: only 1 value for filter %d",i);

	    } else if (nk[i] == 0) {

		shError("fpp: No good extinction values in color %d",i);
		k->k[i]->mag = BADVALUE;
		k->k[i]->magErr = BADVALUE;

	    } else {
		interp_with_errors(ext_time[i], ext_coeff[i], ext_err[i],
				   nk[i], mjd, &mag, &magerr);
		k->k[i]->mag=mag;
		k->k[i]->magErr=magerr;
	    }
	}
    } else {			/* Just interpolate in one band */
	i = ncolor;
	if (nk[i] == 1) {
	    
	    k->k[i]->mag=ext_coeff[i][0];
	    shDebug(FPP_ERRLEVEL,
		    "extinctInterp: only 1 value for filter %d",i);
	    
	} else if (nk[i] == 0) {
	    
	    shError("fpp: No good extinction values in color %d",i);
	    k->k[i]->mag = BADVALUE;
	    k->k[i]->magErr = BADVALUE;

	} else {
	    interp_with_errors(ext_time[i], ext_coeff[i], ext_err[i],
			       nk[i], mjd, &mag, &magerr);
	    k->k[i]->mag=mag;
	    k->k[i]->magErr=magerr;
	}
    }
    k->mjd = mjd;
    return(k);
}

/*----------------------------------------------------------------------*/


/* Assumes calib1bytime list is sorted in time and that if data is missing 
 * for any color the pointer to its CALIB1 is NULL 
 */


static void
patchcalToArrays(CHAIN *calibs, /* Input from MT */
		 int ncolors	/* number of  */
			   )
{

    int color;
    CALIB1BYTIME *cal;
    CURSOR_T crsr;
    
    crsr = shChainCursorNew(calibs);
    while((cal = (CALIB1BYTIME *) shChainWalk(calibs, crsr, NEXT)) != NULL) {

	for (color = 0; color < ncolors; color++) {
	    if (cal->calib[color] != NULL &&
		cal->calib[color]->flux20Err != BADVALUE) { 

		cal_flux20[color][ncal[color]] = cal->calib[color]->flux20;
		cal_flux20Err[color][ncal[color]] = 
		  cal->calib[color]->flux20Err;
		cal_time[color][ncal[color]] = cal->mjd;
		ncal[color]++;
	    } 
	}
    }

    shChainCursorDel(calibs, crsr);
}

/*----------------------------------------------------------------------*/
    /* Interpolate through a list of extinction structs to make a new one 
     * for the input time. This assumes that the input list is sorted by time.
     * If only one struct is on the list, the procedure returns that struct
     * with a warning. It interpolates each color seperately and allows for
     * a null value in a particular color and struct. 
     */
static CALIB1BYTIME *
calbytimeInterp(double mjd,
		const char *filters)
{

    int i;
    float flux20, flux20Err;
    int ncolors = strlen(filters);
    CALIB1BYTIME *tcal=phCalib1bytimeNew(filters);

    /* ERRORS(NAN): ext err from interpolation */

    for (i=0;i<ncolors;i++) {
	if (ncal[i] == 1) {

	    tcal->calib[i]->flux20 = cal_flux20[i][0];
	    tcal->calib[i]->flux20Err = cal_flux20Err[i][0];
	    shDebug(FPP_ERRLEVEL,
		    "calbytimeInterp: only 1 value for filter %d",i);

	} else if (ncal[i] == 0) {

	    tcal->calib[i]->flux20 = BADVALUE;
	    tcal->calib[i]->flux20Err = BADVALUE;
	    shDebug(FPP_ERRLEVEL,"calbytimeInterp: no value for filter %d",i);

	} else {

 /* #if 0 */
            /* piecewise interpolation */
	    interp_with_errors(cal_time[i], cal_flux20[i], cal_flux20Err[i],
			       ncal[i], mjd, &flux20, &flux20Err);
 /* #else */
            /* overall linear fit */
            flux20 =  m_flux20[i] * mjd + c_flux20[i]; 
            flux20Err = pow(m_err_flux20[i] * mjd, 2);
            flux20Err = sqrt(flux20Err + pow(c_err_flux20[i],2));
 /* #endif */
 
	    tcal->calib[i]->flux20=flux20;

	    tcal->calib[i]->flux20Err = flux20Err;
	}
    } 
    return(tcal);
}

/*----------------------------------------------------------------------*/
/* This function fills one CALIB1 structure in a CALIB1BYTIME using
 * the information mergpatch and and extinction struct interpolated
 * (in one color) to the time the stamps overlaping the patch were
 * observed. The order of colors in the extinction is assumed to be
 * the same as that in the starlist (checked by assertion) so that
 * "color" indexes the right point in the array.  "starc_color" and
 * "starc_color" are the positions in the starc of the main band and the
 * band to use for the color term. These have already been checked in
 * match matchpatch. Refcolor is the fiducial color for which the 
 * flux20 number is calculated.
 */
static RET_CODE
patchCal(CHAIN *mergepatch,	/* list of star1starc merges for this color */
	 int  color,		/* number of current PS filter, in PS array  */
	 float refcolor,	/* reference color for calibration */
	 int starc_color,	/* position of color in starc */
	 int starc_color2,	/* position of band for color index in starc */
	 float colorterm,	/* slope of log(flux20)-color relation */
	 float colortermErr,	/* error in the color term */
	 float min_err,	        /* systematic photometric error */
	 CALIB1BYTIME *tcal)	/* structure holding calib for this patch */
{

    STAR1STARCMERGE *merge;
    EXTINCTION *k;
    int n, i;
    float extinction, extinctionErr;
    float flux, fluxErr, mag, magErr, logflux20, logflux20ErrSq;
    float colorindex, err1, err2, colorErrSq;
    float a=0, aErrSq;
    float flux20, flux20Err;
    float *data=NULL, *errors=NULL;

    merge = (STAR1STARCMERGE *) shChainElementGetByPos(mergepatch, HEAD);

    /* Determine extinction for this patch and color */

    k = extinctInterp(tcal->mjd,color,INTERP_ONE_COLOR);

    if (k->k[color]->magErr == BADVALUE) { 
	/* There is no good extinction measured in this band */
	tcal->calib[color]->flux20 = BADVALUE;
	tcal->calib[color]->flux20Err = BADVALUE;
	return(SH_GENERIC_ERROR);
    }

    /* We assume that all stars in a patch are from the 
       same frame and therefore have the same airmass */
      
    extinction = k->k[color]->mag * merge->airmass;
    extinctionErr = k->k[color]->magErr * merge->airmass;
    
    phExtinctionDel(k);

    /* ERRORS(NAN): extcor err due to exterr and airmass? patchcal err */

    /* Get flux20 for this patch and color. 
     * We assume that the there is a linear relation between log(flux20) 
     * and color
     *             log(flux20) = a + b * color
     * 
     * and that b = colorterm is constant and known. b will be determined 
     * for each color during the test year and, presumably, will be checked
     * and updated as needed as part of the post-photo calibration.
     * We determine a, then, by a weighted average:
     *
     *           a = log(flux20) - b * color
     *           w^2 = (sigma_flux20)^2 + 
     *                 (b*color)^2 * [(sigma_b/b)^2 + (sigma_color/color)^2]
     *
     *           a = sum(w_i*a_i)/sum(w_i)
     *           sigma_a = sqrt(1/sum(w_i))
     *
     */

    n = mergepatch->nElements;
    if (n > 0) {
	data = (float *)shMalloc(n*sizeof(float));
	errors = (float *)shMalloc(n*sizeof(float));
	for (i = 0; i < n; i++) {

	    merge = shChainElementGetByPos(mergepatch,i);

	    flux = merge->star1->apCounts;
	    fluxErr = merge->star1->apCountsErr;

	    mag = merge->starc->mag[merge->starcMagPos]->mag;
	    magErr = merge->starc->mag[merge->starcMagPos]->magErr;

	    logflux20 = log10(flux) + (mag-20.)/2.5;
	    logflux20ErrSq = 
	      pow(fluxErr/(flux*log(10)),2) + pow(magErr/2.5,2);
		   

	    colorindex = merge->starc->mag[starc_color]->mag -
	      merge->starc->mag[starc_color2]->mag;
	    err1 = merge->starc->mag[starc_color]->magErr;
	    err2 = merge->starc->mag[starc_color2]->magErr;
	    colorErrSq = err1 * err1 + err2 * err2;


	    data[i] = logflux20 - colorterm * colorindex;
	    if (colorterm == 0 || colorindex == 0) {
		errors[i] = sqrt(logflux20ErrSq);
	    } else {
		errors[i] = sqrt(logflux20ErrSq + pow(colorterm*colorindex,2)*
				 (pow(colortermErr/colorterm,2) +
						colorErrSq/pow(colorindex,2)));
	    }
	}
    } else {
	shError("findphotoparams: patchCal passed patch with no matches");
	shError("findphotoparams: this should never happen");
	tcal->calib[color]->flux20 = BADVALUE;
	tcal->calib[color]->flux20Err = BADVALUE;
	return(SH_GENERIC_ERROR);
    }

    /* XXX NAN: weighted average/error rejecting too many points, 
       What is correct sigma to use for weighted mean? */

    /* ZI: changed nsigma from 2.5 to 1.0 */
    /* this clipping should be done more cleverly */
    find_clipped_mean(data,errors,n,2.0,4,&a,&aErrSq,NULL);
    /* find_clipped_mean returns sigma, not sigmaSq, ZI */
    aErrSq = aErrSq * aErrSq;
    shFree(data);
    shFree(errors);
	
    /* Now calculate flux20 at zero airmass */

    logflux20 = a + colorterm * refcolor + 0.4 * extinction;

    /* Error here comes from a, colorterm, and extinction. refcolor is fixed */
    logflux20ErrSq = aErrSq + pow(refcolor * colortermErr,2) 
      + pow(0.4*extinctionErr,2);
    
    flux20 = (float) pow(10, logflux20);
    flux20Err = flux20 * log(10) * sqrt(logflux20ErrSq);

    /* limit flux20Err from below by a patch systematic error */
    flux20Err = sqrt (pow(flux20Err,2) + pow(min_err*flux20,2));
 

    tcal->calib[color]->flux20 = flux20;
    tcal->calib[color]->flux20Err = flux20Err;

    return(SH_SUCCESS);
}
/*----------------------------------------------------------------------*/
static RET_CODE
frameCalFromOneCal(FRAMEINFO *frame,     /* Current frame info*/
		   CALIB1BYFRAME *fcal,  /* calib1byframe for this frame*/
		   CALIB1BYTIME *tcal,   /* 1st choice cal to use */
		   CALIB1BYTIME *oldcal, /* from previous run */
		   int *fcal_flags	/* flags for a good cal in all bands*/
	       )
{
    EXTINCTION *k;
    int i;
    float extcorr, extcorrErr;

    k = extinctInterp(frame->mjd,fcal->ncolors,INTERP_ALL_COLORS);

    for (i=0;i<fcal->ncolors;i++) {

	if (k->k[i]->magErr == BADVALUE) {
	    /* There is no good extinction measurement */
	    /* in this band, we can't even use oldcal */
	    shError("fpp: No good extinction values in color %d\n",i);
	    fcal->calib[i]->flux20 = BADVALUE;
	    fcal->calib[i]->flux20Err = BADVALUE;
	    fcal_flags[i] = 0;
	    continue;
	}

	/* We are neglecting errors in the airmass */
	extcorr = pow(10., 0.4 * k->k[i]->mag * frame->airmass);
	extcorrErr = extcorr * 0.4 * log(10) * k->k[i]->magErr * frame->airmass;

	if (tcal == NULL || tcal->calib[i]==NULL || 
	    tcal->calib[i]->flux20Err == BADVALUE) {
	    shDebug(FPP_ERRLEVEL,
		    "frameCalFromOneCal: using oldcal for color %d",i);
	    fcal->calib[i]->flux20  = 
		oldcal->calib[i]->flux20/extcorr;

	    fcal->calib[i]->flux20Err = fcal->calib[i]->flux20 *
	      sqrt(pow(oldcal->calib[i]->flux20Err/oldcal->calib[i]->flux20,2)
		   + pow(extcorrErr/extcorr,2));
		   
	    fcal_flags[i] = 0;
	    /* ERRORS(NAN): extcor err due to exterr and airmass? */

	} else {
	    
	    fcal->calib[i]->flux20 =
		tcal->calib[i]->flux20/extcorr;

	    fcal->calib[i]->flux20Err = fcal->calib[i]->flux20 *
	      sqrt(pow(tcal->calib[i]->flux20Err/tcal->calib[i]->flux20,2)
		   + pow(extcorrErr/extcorr,2));

	}
    }
    phExtinctionDel(k);
    return(SH_SUCCESS);			/* Add error checking sometime */
}
/*----------------------------------------------------------------------*/
static int 
frameCal(FRAMEINFO *frame,      /* Current frame info*/
	 CALIB1BYFRAME *fcal,	/* calib1byframe for this frame */
	 CALIB1BYTIME *oldcal,	/* old cal to use for any empty bands */
	 const char *filtersStr,	/* filters expected */
	 int *fcal_flags)	/* flags for a good calib in all bands*/
{

    CALIB1BYTIME *tcal;

    tcal=calbytimeInterp(frame->mjd,filtersStr);
    frameCalFromOneCal(frame,fcal,tcal,oldcal,fcal_flags);

    phCalib1bytimeDel(tcal);
    return(1);			/* Add error catching sometime */

}


/*----------------------------------------------------------------------*/
static int 
find_nearest_frame(double time, CHAIN *frames, CHAIN * framecal)
{
   CURSOR_T crsr;
   CALIB1BYFRAME *fcal;
   FRAMEINFO *frame=NULL;
   int nearest_frame;

   crsr = shChainCursorNew(framecal);
   while((fcal = (CALIB1BYFRAME *)shChainWalk(framecal,crsr,NEXT))
	  != NULL) {
	 /* Find the corresponding FRAMEINFO */
	 frame = findFrameinfo(fcal->field,frames);
	 if (frame->mjd > time) break;
   }
   if (frame != NULL) {
       nearest_frame = frame->field;
   } else {
       nearest_frame = -1;
   }
   shChainCursorDel(framecal, crsr);

   return nearest_frame;
}



/*********************** C entry points in this module ***********************/

   /*
    * <AUTO EXTRACT>
    *
    * ROUTINE: phFindPhotoParams
    *
    * DESCRIPTION:
    * phFindPhotoParams takes the input produced by the monitor telescope
    * (calibration patches as a list of STARCs, extinction(t), most recent
    * run calibration) and combines it with measurements of the postage stamps
    * cut out by the online system to produce a list of calibrations for each 
    * patch and a calibration for each frame. 
    *
    * The starc and extinction structures should contain data for the
    * same set of filters in the same order, which will be checked by
    * assertion. Bad values for extinction coefficients and starc
    * magnitudes from the MT should be indicated by negative error values.
    *
    * Does not presently calculate errors.
    *
    * returns:
    *   SH_SUCCESS                   if all goes well
    *   SH_GENERIC_ERROR             if there's a problem
    *
    * </AUTO> */

RET_CODE 
phFindPhotoParams(int ncolors,         /* I: number of colors (& lists of pstamps) */
		  char *filters[],     /* I: Array of filternames */
		  char *ref_filters[], /* I: Array of color-index filternames*/
		  float *colorterms,   /* I: Array of color terms for calib */
		  float *colortermErrs, /* I: Array of errors for above */
		  CHAIN *pstamps[],    /* I: 1 chain of STAR1PCs per color */
		  CHAIN *patches,      /* I: chain of STARCs */
		  CHAIN *extinctions,  /* I: chain of EXTINCTIONs */
		  CHAIN *frames,       /* I: chain of FRAMEINFOs (time, z) */
		  CALIB1BYTIME *oldcal,/* I: last good CALIB1BYTIME */
                  float starsep,       /* I: max dist in "match" of stars */
                  float sys_ph_err,    /* I: systematic photometric error */
	          float maxErr,         /* max. error in any mag. allowed for MT stars */
                  int n_max,           /* I: max polyn. for flux20(t) fit */
                  float *flux20_m,     /* O: slope for flux20(t) */
                  float *flux20_c,     /* O: intercept for flux20(t) */
                  float *flux20_chi2,  /* O: chi2 for the fit flux20(t) */
		  CHAIN *patchcal,     /* O: chain of CALIB1BYTIMEs */
		  CHAIN *framecal,     /* O: list of CALIB1BYFRAMEs */
		  int *patchcal_flags, /* O: flags for setting return list */
		  int *fcal_flags)     /* O: flags for setting return list */
{
    char *filtersStr;			/* filters, converted to a string */
    int npatches=0, color, calcount;
    int mt_color_lookup[MAX_FILTERS], ref_color_lookup[MAX_FILTERS];
    int nks, ntcal, ext_allocated = 0, tcal_allocated = 0, oldcal_added = 0, ip;
    float refcolor=REFCOLOR;
    double date, mergetime, besttime;
    int i;
    CHAIN *mergepatch;
    FRAMEINFO *frame;
    CHAIN *patch;
    CALIB1BYTIME *tcal, *tcal2;
    CALIB1BYFRAME *fcal;
    STAR1STARCMERGE *merge;
    CURSOR_T crsr;
    float m, m_err, c, c_err, chi2;
    int Npatch = -1;
/*
 * convert that list of filters into a string for
 */
    filtersStr = alloca(ncolors + 1);
    for(i = 0; i < ncolors; i++) {
       filtersStr[i] = filters[i][0];
    }
    filtersStr[i] = '\0';
    

    /* we'll check to see if it's ever set before we use it ... */
    tcal = NULL;   

    /* grab date from first frameinfo */
    frame = (FRAMEINFO *) shChainElementGetByPos(frames, HEAD);
    date = frame->mjd;

    /* We must have at least one extinction struct to run */
    if (extinctions->nElements < 1) {
	shError("phFindPhotoParams: extinction chain is empty");
	return (SH_GENERIC_ERROR);
    } else {

	nks = extinctions->nElements;

	/* Make the extinction arrays for interpolation. Only do this once */
	for (color = 0; color < ncolors; color++) {
	    ext_time[color] = (double *) shMalloc(nks*sizeof(double));
	    ext_coeff[color] = (float *) shMalloc(nks*sizeof(float));
	    ext_err[color] = (float *) shMalloc(nks*sizeof(float));
	    nk[color] = 0;
	}
	ext_allocated = 1;
	extlistToArrays(extinctions,ncolors,filters);

    }


    /* Check if we have any input patches */
    if ((patches == NULL) || (shChainElementGetByPos(patches, HEAD) 
				== NULL)) {
	/* Set all pathcal_flags to 0 */
	for (color = 0; color < ncolors; color++) {
	    patchcal_flags[color] = 0;
	}

	/* Use oldcal for all frames */
	shDebug(FPP_ERRLEVEL,"fpp: no patches from mt. Using oldcal");

        crsr = shChainCursorNew(framecal);
	while((fcal = (CALIB1BYFRAME *) shChainWalk(framecal, crsr, NEXT)) !=
	      NULL) {
	    /* find the corresponding FRAMEINFO */
	    frame=findFrameinfo(fcal->field,frames);

	    /* interpolate to get extinction for current frame */
	    frameCalFromOneCal(frame,fcal,oldcal,oldcal,fcal_flags);

	}    
        shChainCursorDel(framecal, crsr);

	/* If oldcal is out of date, set all fcal_flags to 0 */
	if (floor(date) != floor(oldcal->mjd)) {
	    for (color = 0; color < ncolors; color++) {
		fcal_flags[color] = 0;
	    }
	}

    } else {

	/* First create an index array for the mt data so we can get data
	 * for a given filter. 
	 */

	if (get_mtindex(mt_color_lookup,ref_color_lookup,ncolors,filters,
			ref_filters,patches) != 1) {
	    return (SH_GENERIC_ERROR);
	}

	npatches = patches->nElements;

	/* Calibrate each patch:  */
	crsr = shChainCursorNew(patches);
	while ((patch = (CHAIN *) shChainWalk(patches, crsr, NEXT)) 
	       != NULL) {

	    calcount=0;	besttime = 0;
            Npatch++;

	    /* Make a Calib1bytime for this patch */
	    tcal=phCalib1bytimeNew(filtersStr);

	    if (shChainElementGetByPos(patch,HEAD)==NULL) {
		shError("phFindPhotoParams: current patch contains no starcs");
		return (SH_GENERIC_ERROR);
	    }

	    /* For each color */
	    for (color = 0;color < ncolors; color++) {
     		/* Match postage stamps with patch */
                if ((mergepatch = matchPatch(patch,
					     pstamps[color], frames, 
					     mt_color_lookup[color], 
					     ref_color_lookup[color],
					     filters[color],ref_filters[color],
                                             starsep, maxErr, &mergetime)) 
		    != NULL) {
		    calcount++;
		    tcal->mjd = mergetime;

                    /* Best time so far */
		    besttime += (mergetime-besttime)/calcount;
     
 
                    /* require at least 3 stars for a patch to be valid */
 		    if (mergepatch->nElements > 2) {                   
		        /* Get calib for this color */
		        if (patchCal(mergepatch,color,refcolor,
			        mt_color_lookup[color],ref_color_lookup[color],
			        colorterms[color], colortermErrs[color], sys_ph_err, tcal)
			        == SH_SUCCESS) {
			    /* We got a cal, set its filtername */
			    strcpy(tcal->calib[color]->filter,filters[color]);
		        } else  {
		   	    /* Something went wrong with this band */
   		            tcal->calib[color]->flux20= BADVALUE;
		            tcal->calib[color]->flux20Err = BADVALUE;
			    patchcal_flags[color]-- ;
			    calcount--;
		        }
                    } else {
		        /* less than 3 matched stars */
		        tcal->calib[color]->flux20= BADVALUE;
		        tcal->calib[color]->flux20Err = BADVALUE;
		        patchcal_flags[color]-- ;
		        calcount--;
                    }
	      	    /* Cleanup mergelist */
		    while ((merge = (STAR1STARCMERGE *) 
			    shChainElementRemByPos(mergepatch, HEAD))
			    != NULL) {
		        phStar1starcmergeDel(merge);
		    }
		    shChainDel(mergepatch);
		} else {	/* No matches this band */
		    tcal->calib[color]->flux20= BADVALUE;
		    tcal->calib[color]->flux20Err = BADVALUE;
		    patchcal_flags[color]--;
		}
            }
	    if (calcount > 0) {
		tcal->mjd = besttime;
		tcal->ncolors=ncolors; 
		tcal->MTpatch_number = Npatch;
                /* determine nearest frame */
                tcal->nearest_frame = find_nearest_frame(besttime,frames,framecal);
		shChainElementAddByPos(patchcal, tcal, "CALIB1BYTIME", 
				       TAIL, AFTER);
	    } else {
		phCalib1bytimeDel(tcal);
		shDebug(FPP_ERRLEVEL,
			"findPhotoParams: no good calib for current patch");
		npatches--;
	    }
	}
        shChainCursorDel(patches, crsr); 

	/* Add oldcal to front of (time sorted) patchlist 
	   if it came from same date  */
        /* ZI: only if there are no other patches */
	if (floor(date) == floor(oldcal->mjd) && npatches == 0){
            shChainElementAddByPos(patchcal, oldcal, 
                "CALIB1BYTIME", HEAD, BEFORE);
	    npatches++;
            oldcal_added = 1;
	}

	/*-------------------------------------------------------------  
	 * Calibrate each frame:
	 * Interpolate over time to get calibs for each frame
	 * Assuming that there were at least two successful calib1bytimes 
	 */
	if (npatches >=2) {
	    shDebug(FPP_ERRLEVEL,"Interpolating between ctimes");

            /* Time sort the patchlist */ 
            if (shChainSort(patchcal,"mjd",1) != SH_SUCCESS)  {
                shError("phFindPhotoParams: cannot sort patch chain");
            }

	    /* Copy patch cal to arrays for interpolation */
	    ntcal = patchcal->nElements;
	    for (color = 0; color < ncolors; color++) {		
		cal_time[color] = (double *) shMalloc(ntcal*sizeof(double));
		cal_flux20[color] = (float *) shMalloc(ntcal*sizeof(float));
		cal_flux20Err[color] = (float *) shMalloc(ntcal*sizeof(float));
		ncal[color] = 0;
	    } 
	    tcal_allocated = 1;
	    patchcalToArrays(patchcal,ncolors);
          
            /* fit a straight line to flux20(t), in each color*/
	    for (color = 0; color < ncolors; color++) {
	       if(ncal[color] == 0) {
		  shError("phFindPhotoParams: "
			  "no calibration points in band %d\n", color);
		  continue;
	       }
                 /* find the mean value and sigma for cal_flux20[color] */
                 find_clipped_mean(cal_flux20[color],cal_flux20Err[color],ncal[color],3.0,5,
                                   &c, &c_err, &chi2);               
                 m_flux20[color] = m_err_flux20[color] = m = 0.0;
                 c_flux20[color] = c;
                 c_err_flux20[color] = c_err;

                 if (n_max && chi2 > 2) {	
                     fit_straight_line(cal_time[color],cal_flux20[color],cal_flux20Err[color],
                                   ncal[color],&m,&m_err,&c,&c_err,&chi2);
                     m_flux20[color] = m;
                     m_err_flux20[color] = m_err;
                     c_flux20[color] = c;
                     c_err_flux20[color] = c_err;
                 }
                 flux20_m[color] = m;
                 flux20_c[color] = c;
                 flux20_chi2[color] = chi2;

                 if (chi2 > 2) {
                     shError("phFindPhotoParams: chi2 > 2!");
                 }
#if 0
                 if (chi2 > 10.0) {
                     shFatal("phFindPhotoParams: chi2 > 10, that's just too much...");
                 }
#endif
            }

	    /* Loop over all frames calib1byframelist */

            crsr = shChainCursorNew(framecal);
	    while((fcal = (CALIB1BYFRAME *)shChainWalk(framecal,crsr,NEXT))
		  != NULL) {

		/* Find the corresponding FRAMEINFO */
		frame = findFrameinfo(fcal->field,frames);
		
		frameCal(frame,fcal,oldcal,filtersStr,fcal_flags);
	    }
            shChainCursorDel(framecal, crsr);

	} else if (npatches == 1) {

	    /* Use the single patch for all frames */
	    shDebug(FPP_ERRLEVEL,"fpp: only 1 patch, use for all frames");

	    /* Loop over all frames calib1byframelist */
            crsr = shChainCursorNew(framecal);
	    while((fcal = (CALIB1BYFRAME *)shChainWalk(framecal,crsr,NEXT))
		  != NULL) {

	       tcal2 = (CALIB1BYTIME *) shChainElementGetByPos(patchcal, HEAD);

		/* Find the corresponding FRAMEINFO */
	       frame = findFrameinfo(fcal->field,frames);

	       frameCalFromOneCal(frame,fcal,tcal2, oldcal,fcal_flags);
	    }    
            shChainCursorDel(framecal, crsr);

	} else {		
	    /* Use oldcal for all frames*/
	    shDebug(FPP_ERRLEVEL,
		    "phFindPhotoParams: No matched patches and no same-date oldcal, using oldcal anyway");

	    /* Loop over all frames calib1byframelist */
            crsr = shChainCursorNew(framecal);
	    while((fcal = (CALIB1BYFRAME *) shChainWalk(framecal, crsr, NEXT))
		  						     != NULL) {
                tcal2 = oldcal;
		/* find the corresponding FRAMEINFO */
		frame = findFrameinfo(fcal->field,frames);
		/* calibrate this frame */
		frameCalFromOneCal(frame,fcal,NULL, oldcal,fcal_flags);
	    }    
            shChainCursorDel(framecal, crsr);
	    
	}
    }


    /* If we added it, remove oldcal from patchcal list so we can
       delete it at the end of the pipeline. */
    if (oldcal_added){
         for (ip=0; ip<npatches; ip++) {
              tcal = shChainElementGetByPos(patchcal,ip);
              if (oldcal == tcal) {
	          shChainElementRemByPos(patchcal, ip);
                  break;
              }
         }   
    }


    /* Free interpolation arrays  */
    if (ext_allocated) {
	for (color = 0; color < ncolors; color++) {
	    shFree(ext_time[color]);
	    shFree(ext_coeff[color]);
	    shFree(ext_err[color]);
	}
    }
    if (tcal_allocated) {
	for (color = 0; color < ncolors; color++) {
	    shFree(cal_time[color]);
	    shFree(cal_flux20[color]);
	    shFree(cal_flux20Err[color]);
	}
    }
    return(SH_SUCCESS);
}


