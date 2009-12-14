/*
 * <AUTO>
 * FILE: flatfieldsInterp.c 
 *
 * This is the science module flatfieldsInterp which
 * generates a 1D flatfield vector from the input 2D
 * flatfield array (produced by flatfieldsCalc.c)
 *
 * </AUTO>
 */

#include <stdio.h>
#include <math.h>
#include "region.h"
#include "phCorrectFrames.h"
#include "phCorrectFrames_p.h"    /* Private definitions  */

#define SIQR_TO_SIGMA 0.6744897502
/*****************************************************************************/
/*
 * comparison function for qsort()
 */
static int
cmp(const void *a, const void *b)
{
   int ia = *(int *)a;
   int ib = *(int *)b;

   return((ia < ib) ? -1 : ((ia == ib) ? 0 : 1));
}


/*****************************************************************************/
/*
 * find arbitrary quartile value for values given in buffer buf[].
 * If sorted == 1, assume that the values are sorted.
 */
static float
get_quartile(int *buf,			/* the values */
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

   if (!sorted) qsort(buf,n,sizeof(buf[0]),cmp); 

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

/*****************************************************************************/
/*
 * interpolate in the buffer buf[]
 *
 * If beta is an integer greater than or equal to one, then weighting 
 * function used is:
 *    Weight = 1/(delta_row + 1)**(beta-1)
 * where delta_row = abs(row - rowid).
 *
 * If beta is zero, 1D vector is median of nadj columns.
 */
static S32
interp(int *buf,			/* the values in question */
       const int *good,			/* which values are good */
       int nval,			/* number of values */
       int beta,			/* see comment above */
       float sigrej,			/* how many sigma to clip at */
       float *err,                      /* standard deviation of good values */
       float *err_12)                   /* deviation of the median between the
                                           first and second half of the sample */
                                        /* err and err_12 are evaluated only for */
                                        /* beta=0 and both can be NULL */


{
   int i, j;
   float avflatlev, sigflatlev, flatval, wsum, weight, median;
   int ngood;				/* number of good values */
   int nclip;				/* number of min and max values to clip */
   double sum, sum2;			/* double, as variance is sum2-sum^2,
					   the difference of two big numbers */
   float aux1, aux2;
   int nL=0, nR=0;
   int *bufL, *bufR;	

   /* this is really needed only if beta == 0 && err_12 != NULL, 
      but need to silence compiler */
   nL = nval/2;
   nR = nval - nL;
   bufL = (int *)shMalloc(nL * sizeof(int)); 
   bufR = (int *)shMalloc(nR * sizeof(int)); 
         
/*
 * first remove the bad values 
 */
   for(i = j = 0; i < nval; i++) {
       if (good[i]) buf[j++] = buf[i]; 
   }
   nval = j;

   if (nval <= 1) {
      if (err != NULL) {
         *err = 0.0;
      }
      if (err_12 != NULL) {
         *err_12 = 0.0;
      }
      shFree(bufL); shFree(bufR);
      return buf[0];
   } 
  
   /* if needed split the sample in half */
   if (beta == 0 && err_12 != NULL) {
      if(nL <= 1) {			/* too few values */
         shFree(bufL); shFree(bufR);
	 return 0;
      }
      
      for(i = 0; i < nL; i++) {
          bufL[i] = buf[i]; 
      }     
      for(i = 0; i < nR; i++) {
          bufR[i] = buf[i+nL]; 
      } 
      qsort(bufL,nL,sizeof(bufL[0]),cmp);      
      qsort(bufR,nR,sizeof(bufR[0]),cmp);           
   }     
  

/*
 * then qsort the buffer --- massive overkill, but who cares?
 */
   qsort(buf,nval,sizeof(buf[0]),cmp);
 
   if (beta == 0) {
      /* median filter */
      median = get_quartile(buf,nval,1,0.5);	
      if (err != NULL) {
         aux1 = get_quartile(buf,nval,1,0.25);	
         aux2 = get_quartile(buf,nval,1,0.75);
         *err = SIQR_TO_SIGMA * (aux2 - aux1);	
      }
      if (err_12 != NULL) {
         aux1 = get_quartile(bufL,nL,0,0.5);	
         aux2 = get_quartile(bufR,nR,0,0.5);	
         *err_12 = abs(aux2-aux1) / median;
      }
      shFree(bufL); shFree(bufR);
      return median;

   } else {
      ngood = 0; nclip = 0;
      sum = sum2 = 0.0;
      if (nval > 2) nclip = 1; 
      /* do not include the smallest and the largest value 
        (no bias, this is used only for clipping on sigma) */
      for (i = nclip; i < nval-nclip; i++) { 
	  ngood++;
	  flatval = buf[i];
	  sum += flatval;
          sum2 += flatval * flatval;
      }
      shAssert(ngood > 0);

      avflatlev = sum/ngood;
      
      if(nval == 1) {
	 sigflatlev = 0;
      } else {
	 sigflatlev = sqrt((ngood*sum2 - sum*sum)/(ngood*(ngood - 1)));
      }
      sum = wsum = 0.0;
      for (i = 0; i < nval; i++) {
	 if(fabs(avflatlev - buf[i]) > sigrej*sigflatlev) {
	    weight = 0.0;
	 } else {
	    int delta = fabs(i - 0.5*nval);
	    weight = 1.0/(pow(delta, beta - 1) + 1.0);
	 }
	 sum += (weight*buf[i]);
	 wsum += weight;
      }
      shFree(bufL); shFree(bufR);

      if(wsum == 0) {
	 return(0);
      }
      sum /= wsum;
      return(sum >= 0 ? sum + 0.5 : -(-sum + 0.5));
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * This is the science module flatfieldsInterp which
 * generates a median 1D flatfield vector from the input 2D
 * flatfield array (produced by flatfieldsCalc.c)

 * Returns:
 *   SH_SUCCESS	Always (but may write error message)
 */
int
phFlatfieldsInterp(const REGION *flat2d, /* I: flat field from phFlatfieldsCalc */
		   REGION *flat_vec)	 /* O: 1-d flat field vector and errors */
{
   int buf[MAXBUF];			/* values to interpolate */
   int good[MAXBUF];			/* flag for if they're good */
   int ngood;				/* number of good values */
   int i, j;
   int nrow;				/* number of rows in input */
   float sum = 0.0;                     /* sum of flat-field values for all col. */
   int nsum = 0;                        /* number of good flat-field values */
   float ff1=0;				/* aux. storage for flat-field */    
   float err;				/* standard deviation of good values */
   float err_12;			/* deviation of the median between the
                                           first and second half of the sample */

   shAssert(flat2d != NULL && flat2d->type == TYPE_U16);
   nrow = flat2d->nrow;
    
   /* for each column find the median of flat-field values for all frames */
   for (i = 0; i < flat2d->ncol; i++) {
      ngood = 0;
      /* copy flat-field values to buffer and mark all bad ones */
      for(j = 0; j < nrow; j++) {
          buf[j] = flat2d->rows_u16[j][i];
  	  if(buf[j] <= 0) {
  	       good[j] = 0;
          } else {
  	       ngood++;
               if (ngood == 1) ff1 = buf[j];
               good[j] = 1;
	  }
      }
      
      if (ngood == 0) {			/* No good value for this column */
	  flat_vec->rows_u16[0][i] = 0;
          /* do not consider this column in normalization */
	  continue;
      } else if(ngood == 1) {		/* not enough to median filter */
	  flat_vec->rows_u16[0][i] = ff1;
	  err = err_12 = 0;
      } else {
          /* find median (clipped at 3 sigma) for this column */
          flat_vec->rows_u16[0][i] = interp(buf,good,nrow,0,3.0,&err,&err_12);
      }
      sum += flat_vec->rows_u16[0][i]; 
      nsum++;
      if (flat_vec->rows_u16[0][i] != 0) {
         /* fractional standard deviation */
         flat_vec->rows_u16[1][i] = 1.0e6 * err / flat_vec->rows_u16[0][i];
         if (nrow > 0) flat_vec->rows_u16[1][i] /= sqrt(nrow);
         /* consistency between the two halfs */
         /* err_12 is the fractional difference, no division by the value */
         flat_vec->rows_u16[2][i] = 1.0e6 * err_12;
      } else {
         flat_vec->rows_u16[1][i] = 0;
         flat_vec->rows_u16[2][i] = 0;
      }

        
   } /* loop over columns */

   /* renormalize flat_vec and change to inverted flat field */
   shAssert(nsum > 0);
   sum /= nsum;
   for (i = 0; i < flat2d->ncol; i++) {
       if (flat_vec->rows_u16[0][i] != 0) {
           flat_vec->rows_u16[0][i] = FSHIFT * sum / flat_vec->rows_u16[0][i];
       }
   }

   /* Put flat scaling factor, FSHIFT, in the header */
   if(shHdrInsertInt(&flat_vec->hdr,"TSHIFT",FSHIFT,
		     "Scale factor to increase precision.") != SH_SUCCESS) {
       shError("shFlatfieldsInterp: could not write to flat_vec->hdr");
   }

   return (SH_SUCCESS);
}



/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * This is the science module skybiasInterp which
 * interpolates (filters) sky and bias values produced by 
 * flatfieldsCalc.c.
 *
 * The interpolation scheme is specified by beta (see interp())
 *
 * Returns:
 *   SH_SUCCESS	Always (but may write error message)
 */
int
phSkyBiasInterp(const REGION *sky_vec, /* 1-d sky/skyErr from "   " */
		   const REGION *drift_vec,  /* bias drift from "   " */
		   const REGION *Odrift_vec, /* odd col. bias drift from "   " */
		   const REGION *Edrift_vec, /* even col. bias drift from "  " */
		   int rowid,		/* current frame in the run, if -1
                                           do flat-field only */
		   int nadj,		/* number of adjacent rows to average
					 over */
		   float sigrej,	/* reject values that deviate by more
					   than sigrej s.d. of sky and bias */
		   int beta,		/* how to smooth; see comment above */
		   const CCDPARS *ccdpars, /* CCD parameters data structure */
		   CALIB1 *calib)	/* saved bias drift, sky, and slope */
{
   int buf[MAXBUF];			/* values to interpolate */
   int bufOEdiff[MAXBUF];			/* values to interpolate */
   int err[MAXBUF];	                /* corresponding errors */
   int sig[MAXBUF];	                /* corresponding sigmas */
   int good[MAXBUF];			/* flag for if they're good */
   int bias_drift[2];			/* bias drift for 2 amps */
   int bias_driftOEdiff[2];			/* bias drift for 2 amps */
   int ngood;				/* number of good values */
   int nval;				/* number of rows to use */
   int nrow;				/* number of rows in input */
   int i, j, k, jL, jR, nW; 

   shAssert(sky_vec != NULL && sky_vec->type == TYPE_FL32 && sky_vec->ncol==3);
   shAssert(drift_vec != NULL && drift_vec->type == TYPE_S16);
   shAssert(Odrift_vec != NULL && Odrift_vec->type == TYPE_S16);
   shAssert(Edrift_vec != NULL && Edrift_vec->type == TYPE_S16);
   shAssert(ccdpars != NULL && calib != NULL);
   shAssert(beta >= 0);
   shAssert(sky_vec->nrow == Odrift_vec->nrow);
   shAssert(sky_vec->nrow == Edrift_vec->nrow);
   shAssert(drift_vec->ncol == 2 && Odrift_vec->ncol == 2 && Edrift_vec->ncol == 2);
 
   /* number of data points in array */
   nrow = sky_vec->nrow;

/*
 * Interpolate Sky data
 */
   ngood = 0;
   /* the filter width */
   nW = 2*nadj + 1;

   /* the window edges */
   jL = rowid - nadj;
   jR = rowid + nadj;
   /* check the left boundary */
   if (jL < 0) {
      jL = 0; 
      jR = nW;
   }
   /* check the right boundary */
   if (jR > nrow) {
      jR = nrow;      
      jL = nrow - nW;
      if (jL < 0) jL = 0; 
   }

   for(k = 0,j = jL; j<=jR; j++, k++) {
       if(j < 0 || j >= nrow) {
           buf[k] = MAX_U16;
           err[k] = MAX_U16; 
           sig[k] = MAX_U16; 
           good[k] = 0;
       } else {
           buf[k] = 32 * sky_vec->rows_fl32[j][0] + 0.5;
           err[k] = 128 * sky_vec->rows_fl32[j][1] + 0.5;
           sig[k] = 128 * sky_vec->rows_fl32[j][2] + 0.5;
           if(buf[k] <= 0) {
	      good[k] = 0;
           } else {
	      ngood++;
	      good[k] = 1;
           }
       }
   }    

   nval = jR - jL + 1;
  
   if (ngood == 0) {		/* No good value for this column */
	calib->sky = 0;
        calib->skyerr = -1.0;
        calib->skysig = -1.0;
        shError("phSkyBiasInterp: failed to interpolate sky for frame %d.",rowid);
   } else {
        if (nval == 1) {	
            calib->sky = buf[0] / 32.0;  
            calib->skyerr = err[0] / 128.0; 
            calib->skysig = sig[0] / 128.0; 
        } else {    
	    /* interpolate sky, sky error, and sky sigma */  
            calib->sky = interp(buf,good,nval,beta,sigrej,NULL,NULL) / 32.0; 
	    /* OLD */
             /* if no error for this row then interpolate to avoid skyerr = 0 */
             if (sky_vec->rows_fl32[rowid][1] <= 0) {
                calib->skyerr = interp(err,good,nval,beta,sigrej,NULL,NULL) / 128.0;  
             } else {
                calib->skyerr = sky_vec->rows_fl32[rowid][1];
             }
             /* if no sigma for this row then interpolate to avoid skysig = 0 */
             if (sky_vec->rows_fl32[rowid][2] <= 0) {
                calib->skysig = interp(sig,good,nval,beta,sigrej,NULL,NULL) / 128.0;  
             } else {
                calib->skysig = sky_vec->rows_fl32[rowid][2];
             }


         /* NEW:   calib->skyerr = interp(err,good,nval,beta,sigrej,NULL,NULL) / 128.0;  
            calib->skysig = interp(sig,good,nval,beta,sigrej,NULL,NULL) / 128.0;  */   
        }
   }

   /* raw sky values (skyslope will be calculated at Tcl level) */
   calib->skyraw = sky_vec->rows_fl32[rowid][0];
   /* use skysig as a storage for interpolated sky values to be plotted later */
 /*  sky_vec->rows_fl32[rowid][2] = calib->sky; */

/*
 * Get drift correction data
 *      
 * Also determine gain and dark_variance for frame
 * It'd be nice to use the measured dark variance, but this will do for now
 */
   for(i = 0;i < ccdpars->namps;i++) {
      ngood = 0;
      for(k = 0,j = jL; j <= jR; j++, k++) {
	 if(j < 0 || j >= nrow) {
	    good[k] = 0;
	 } else {
	    buf[k] = drift_vec->rows_s16[j][i];
	    bufOEdiff[k] = Odrift_vec->rows_s16[j][i] - Edrift_vec->rows_s16[j][i];
	    if(buf[k] == MAX_S16 ||
	       Odrift_vec->rows_s16[j][i] == MAX_S16 ||
	       Edrift_vec->rows_s16[j][i] == MAX_S16) {
	       good[k] = 0;
	    } else {
	       ngood++;
	       good[k] = 1;
	    }
	 }
      }

      if(ngood == 0) {
	 bias_drift[i] = MAX_S16;
	 bias_driftOEdiff[i] = 0;
      } else {
         if (nval == 1) {	
	     bias_drift[i] =  buf[0];  
	     bias_driftOEdiff[i] =  bufOEdiff[0];  
         } else {      
	     bias_drift[i] = interp(buf,good,nval,beta,sigrej,NULL,NULL);
	     bias_driftOEdiff[i] = interp(bufOEdiff,good,nval,beta,sigrej,NULL,NULL);
         }
      }
   }
   calib->lbias = bias_drift[0];
   calib->lbiasOEdiff = bias_driftOEdiff[0];
   
   if (ccdpars->namps == 1) {
      calib->rbias = calib->lbias;
      calib->rbiasOEdiff = calib->lbiasOEdiff;
      
      calib->dark_variance = pow(ccdpars->readNoise0,2);
      calib->gain = ccdpars->gain0; 
   } else {
      calib->rbias = bias_drift[1];
      calib->rbiasOEdiff = bias_driftOEdiff[1];
      
      calib->dark_variance =
			  pow((ccdpars->readNoise0 + ccdpars->readNoise1)/2,2);
      calib->gain = (ccdpars->gain0 + ccdpars->gain1)/2;
   }
   
   return (SH_SUCCESS);
}



/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * This is the science module regGetOddEven which
 * computes the median values of counts in odd and even 
 * columns for row1 <= row <= row2
 *
 * Returns:
 *   SH_SUCCESS	Always (but may write error message)
 */
int
phRegGetOddEven(const REGION *reg, /* input region */
		int row1,	   /* starting row (>= 0)*/
		int row2,	   /* ending row (< reg.ncol) */
                float *oddMed,     /* median count value for odd columns */
                float *oddMedErr,  /* error for oddMed */
                float *evenMed,    /* median count value for odd columns */
                float *evenMedErr) /* error for evenMed  */
{
   int odd[MAXBUF];			/* values to median */
   int even[MAXBUF];	                /* values to median */
   int ncol;				/* number of columns */
   int i, j, nval; 
   float aux1, aux2; 
   int tshift = 32;

   shAssert(reg != NULL && reg->type == TYPE_U16 && reg->ncol>1);
   shAssert(oddMed != NULL && evenMed != NULL);
 
   /* number of columns */
   ncol = reg->ncol;

   nval = 0;
   for (j = 0; j < ncol/2; j++) {
      for(i = row1; i <= row2; i++) {
         if (i == row1) {
            odd[j] = 0;
            even[j] = 0;
            nval++;
         }
         odd[j] += tshift*reg->rows_u16[i][2*j+1]; 
         even[j] += tshift*reg->rows_u16[i][2*j];
      }
   }

   /* there are (row2-row1+1) added values */
   if (row2 > row1) {
       tshift *= (row2-row1+1);
   }   

   *oddMed = get_quartile(odd,nval,0,0.5) / tshift;	
   if (oddMedErr != NULL) {
       aux1 = get_quartile(odd,nval,1,0.25);	
       aux2 = get_quartile(odd,nval,1,0.75);
       *oddMedErr = SIQR_TO_SIGMA * (aux2 - aux1) / tshift / sqrt(nval);	
   }
   *evenMed = get_quartile(even,nval,0,0.5) / tshift;	
   if (evenMedErr != NULL) {
       aux1 = get_quartile(even,nval,1,0.25);	
       aux2 = get_quartile(even,nval,1,0.75);
       *evenMedErr = SIQR_TO_SIGMA * (aux2 - aux1) / tshift / sqrt(nval);	
   }
  
   return (SH_SUCCESS);
}
