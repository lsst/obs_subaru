/*  IMPORTANT:  atCaliApply and atCaliInvert have been converted 
                for use with the JEG photometric equations 
                (see sdss-calib 705).
                The other routines in atCali.c have NOT been changed,
                but the results are equivalent for b = c = 0.
                (DLT, 5 March 2002)
*/


/* <AUTO>
   FILE: 
             atCali.c
<HTML>
C bindings apply photometric calibrations.
</HTML>
</AUTO>
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "dervish.h"
#include "atCali.h"

/*<AUTO EXTRACT>
  ROUTINE: atCaliApply
  DESCRIPTION:
  <HTML>
  Applies, for filter u and color filter g:
  <pre>
  u = u_inst - a'_u - b'_u [(u-g)_inst - (u-g)_inst,0]
                    - c'_u [(u-g)_inst - (u-g)_inst,0] [X - X_0] 
                    - k'_u X 

  where
  (u'-g')_inst,0 is the crossing color
  X_0 is a zeropoint airmass 
      (we generally take it to be the mean airmass of the SDSS imaging 
      survey, or X_0=1.3)

  and where, to second order,
  a'_u = a_u
  b'_u = b_u
  c'_u = c_u
  k'_u = k_u
  (note: for earler forms of the photometric equations,
  a'_u=a_u and k'_u=k_u to only first order and additional
  terms were needed)


  If 3\sigma > Counts, use cosmic color and cosmic scatter
  where 
  Counts is the counts/sec through the color filter
  \sigma is the error on Counts 

  This handles cases where there is no detection through the color filter,
  or the error on the detection is just too high.
  The idea behind using the mean cosmic color is that we are making a 
  correction, hopefully small, and using a reasonable guess is better
  than using a very noisy mesaurement.
 

  </pre>
  </HTML>
  RETURN VALUES:
  <HTML>
  <pre>
 	error codes:
 		0	calibration sucessful
 		1	counts <= 0
 	status codes:
 		0	calibrated using measured quatities
 		1	used cosmic colors for calibration
 
</pre>

</HTML>
</AUTO>*/
int atCaliApply 
(
 double counts,			/* IN: counts in the reference filter */
 double countsErr,		/* IN: error in counts */
 double cCounts,		/* IN: count in the adjacent filter */
 double cCountsErr,		/* IN: error in cCounts */
 double intTime,		/* IN: exposure time in the reference filter */
 double cIntTime,		/* IN: exposure time in the adjacent filter */
 double airmass,		/* IN: airmass  (sec(Z)) */
 double zeropt,			/* IN: zeropoint (a') */
 double extinct,		/* IN: extinction prime (k') */
 double colorTerm,		/* IN: color term (b') */
 double secColorTerm,		/* IN: second order color term (c') */
 int sign,			/* IN: usually +1 for u g r i; -1 for z */
 double cosmicColor,		/* IN: cosmic reference-adjacent color */
 double cosmicError,		/* IN: scatter in cosmicColor */
 double zpAirmass,	        /* IN: zeropoint airmass (for c term) */
 double zpColor,	        /* IN: zeropoint color (for c term) */
 double *calMag,		/* OUT: calibrated magnitude */
 double *calMagErr,		/* OUT: error in calMag */
 int *status			/* OUT: status */
 ) {
  double mag=0, magErr=0;
  double cMag=0, cMagErr=0;
  
  double nSigma = 3.00;		/* if cCounts < nSigma*cCountsErr, 
				   past errorThresh */ 
  double errorThresh=0;		/* past errorThresh, 
				   use cosmic scatter for calibration*/
  
  int returnValue = 0;
  
  /* define status = 0 to be normal completion */
  *status = 0;
  
  /*
   * convert to counts/second 
   *
   */
  counts = counts/intTime;
  countsErr = countsErr/intTime;
  cCounts = cCounts/cIntTime;
  cCountsErr = cCountsErr/cIntTime;
  
  /*
   * convert to magnitudes
   *
   */
  if ( counts > 0) {
    errorThresh = nSigma * cCountsErr;
    
    magErr = ( 2.5/log(10.0) ) * (countsErr / counts);
    mag    = -2.5*log10(counts);
    
    if ( cCounts > 0) {
      cMagErr = ( 2.5/log(10.0) ) * (cCountsErr / cCounts);
      cMag    = -2.5*log10(cCounts);
    } else {
      cCounts = -999.;
      cMagErr = -999.;
      cMag = -999.;
      returnValue = 1;
    }
    
  } else {
    *calMag = -999.;
    *calMagErr = -999.;
    returnValue = 1;
  }
  
  /*
   * check whether to use cosmic colors for transformation
   *
   */
  if ( returnValue == 0) {
    if ( (errorThresh < cCounts) && (cCounts > 0) ) {
      cMag    = sign*(mag - cMag);
      cMagErr = sqrt(pow(cMagErr,2.0) + pow(magErr,2.0));
    } else {
      cMag    = cosmicColor;
      cMagErr = cosmicError;
      *status = 1;
    }
  }
  /*
   * do the transformation
   *
   */
  if ( returnValue == 0) {
    *calMag = 
      mag - zeropt - colorTerm*(cMag-zpColor) 
      - secColorTerm*(cMag-zpColor)*(airmass-zpAirmass) - extinct*airmass;
    *calMagErr = 
      sqrt( pow(magErr,2.0) + pow(colorTerm*cMagErr,2.0) 
	   + pow(secColorTerm*(airmass-zpAirmass)*cMagErr,2.0));
  }
  return returnValue;
}

double lasinh(double x) {
    if (x < -100) {
        return log(-0.5/x + 1./(8.*x*x*x));
    } else {
	return log(x + sqrt(1+x*x));
    }
}

/*<AUTO EXTRACT>
  ROUTINE: atLCaliApply
  DESCRIPTION:
  <HTML>
  Applies, for filter u and color filter g:
  <pre>
  u = u_inst - a'_u - b'_u (u-g)_inst - c'_u (u-g)_inst secZ - k'_u secZ 
  where:
  a'_u = a_u - b_u(a_u-a_g)  
  b'_u = b_u  
  c'_u = c_u  
  k'_u = k_u - b_u(k_u-k_g) - c_u(a_u-a_g)  

  If the color is blueward cast rather than redward,
  (for the i filter, say, redward color cast is (i-z), blueward is (r-i) )
  then sign of every term including color in the above equations must
  change signs.


  If 3\sigma > Counts, use cosmic color and cosmic scatter
  where 
  Counts is the counts/sec through the color filter
  \sigma is the error on Counts 

  This handles cases where there is no detection through the color filter,
  or the error on the detection is just too high.
  The idea behind using the mean cosmic color is that we are making a 
  correction, hopefully small, and using a reasonable guess is better
  than using a very noisy mesaurement.
 
  (Luptitude calculation)

  </pre>
  </HTML>
  RETURN VALUES:
  <HTML>
  <pre>
 	error codes:
 		0	calibration sucessful
 		1	counts <= 0
 	status codes:
 		0	calibrated using measured quatities
 		1	used cosmic colors for calibration
 
</pre>

</HTML>
</AUTO>*/
int atLCaliApply 
(
 double counts,			/* IN: counts in the reference filter */
 double countsErr,		/* IN: error in counts */
 double cCounts,		/* IN: count in the adjacent filter */
 double cCountsErr,		/* IN: error in cCounts */
 double intTime,		/* IN: exposure time in the reference filter */
 double cIntTime,		/* IN: exposure time in the adjacent filter */
 double airmass,		/* IN: airmass  (sec(Z)) */
 double zeropt,			/* IN: zeropoint (a') */
 double extinct,		/* IN: extinction prime (k') */
 double colorTerm,		/* IN: color term (b') */
 double secColorTerm,		/* IN: second order color term (c') */
 int sign,			/* IN: usually +1 for u g r i; -1 for z */
 double cosmicColor,		/* IN: cosmic reference-adjacent color */
 double cosmicError,		/* IN: scatter in cosmicColor */
 double *calMag,		/* OUT: calibrated luptitude */
 double *calMagErr,		/* OUT: error in calMag */
 int *status			/* OUT: status */
 ) {
  double normcnt;
  double mag=0, magErr=0;
  double cMag=0, cMagErr=0;
  
  double nSigma = 3.00;		/* if cCounts < nSigma*cCountsErr, 
				   past errorThresh */ 
  double errorThresh=0;		/* past errorThresh, 
				   use cosmic scatter for calibration*/
  
  int returnValue = 0;
  
  /* define status = 0 to be normal completion */
  *status = 0;
  
  /* If counts error negative, bad measure, so return -1000 indicating bad
   * bad measure. */
  if (countsErr < 0)
    {
      *calMag = -1000.;
      *calMagErr = -1000.;
      return 0;
    }
  /*
   * convert to counts/second 
   *
   */
  counts = counts/intTime;
  countsErr = countsErr/intTime;
  cCounts = cCounts/cIntTime;
  cCountsErr = cCountsErr/cIntTime;
  
  /*
   * convert to magnitudes
   *
   */
	/* this may cause trouble */

    errorThresh = nSigma * cCountsErr;
    
    /*magErr = ( 2.5/log(10.0) ) * (countsErr/counts);*/

    normcnt = counts*counts + LUPTITUDE_SCALE*LUPTITUDE_SCALE;
    magErr = (2.5/log(10.0)) * countsErr /sqrt(normcnt);

    /*mag    = -2.5*log10(counts);*/

    mag    = -2.5/log(10.0)*lasinh(counts/LUPTITUDE_SCALE) + 2.5/log(10.0)*log(2.0/LUPTITUDE_SCALE);
    
    normcnt = cCounts*cCounts + LUPTITUDE_SCALE*LUPTITUDE_SCALE;
    cMagErr = (2.5/log(10.0)) * cCountsErr /sqrt(normcnt);

      /*cMagErr = ( 2.5/log(10.0) ) * (cCountsErr/cCounts);*/
      /*cMag    = -2.5*log10(cCounts);*/
    cMag    = -2.5/log(10.0)*lasinh(cCounts/LUPTITUDE_SCALE) + 2.5/log(10.0)*log(2.0/LUPTITUDE_SCALE);
  
  /*
   * check whether to use cosmic colors for transformation
   *
   */
  if ( returnValue == 0) {
    if ( (errorThresh < cCounts) && (cCounts > 0) ) {
      cMag    = sign*(mag - cMag);
      cMagErr = sqrt(pow(cMagErr,2.0) + pow(magErr,2.0));
    } else {
      cMag    = cosmicColor;
      cMagErr = cosmicError;
      *status = 1;
    }
  }
  /*
   * do the transformation
   *
   */
  if ( returnValue == 0) {
    *calMag = 
      mag - zeropt - colorTerm*cMag 
      - secColorTerm*cMag*airmass - extinct*airmass;
    *calMagErr = 
      sqrt( pow(magErr,2.0) + pow(colorTerm*cMagErr,2.0) 
	   + pow(secColorTerm*airmass*cMagErr,2.0));
  }
/*
printf("c cc c cc %f %f %f %f airmass: %f color:%f c:%f exptime %f b %f a %f k%f sign %d cm %g\n",counts,cCounts,counts*intTime,cCounts*intTime,airmass,cMag,secColorTerm,intTime,colorTerm,zeropt,extinct,sign,*calMag);
*/
  return returnValue;
}

/* new LcaliApply */

int atnLCaliApply 
(
 double counts,			/* IN: counts in the reference filter */
 double countsErr,		/* IN: error in counts */
 double cCounts,		/* IN: count in the adjacent filter */
 double cCountsErr,		/* IN: error in cCounts */
 double intTime,		/* IN: exposure time in the reference filter */
 double cIntTime,		/* IN: exposure time in the adjacent filter */
 double airmass,		/* IN: airmass  (sec(Z)) */
 double zeropt,			/* IN: zeropoint (a') */
 double extinct,		/* IN: extinction prime (k') */
 double colorTerm,		/* IN: color term (b') */
 double secColorTerm,		/* IN: second order color term (c') */
 double cosmicColor,		/* IN: cosmic reference-adjacent color */
 double cosmicError,		/* IN: scatter in cosmicColor */
 double zpAirmass,		/* IN: reference airmass */
 double zpColor,		/* IN: reference color */
 int filterIndex,		/* IN: filter index 0=u', 1=g', 2=r' 3=i' 4=z' etc */
 double faintbprime,		/* IN: if = 1, use an extra faint bprime , default = 0*/
 int maggies,			/* IN: if=1, return maggies instead of mags, if =2, return approx Jy */
 double *calMag,		/* OUT: calibrated luptitude */
 double *calMagErr,		/* OUT: error in calMag */
 int *status			/* OUT: status */
 ) {
  double normcnt;
  int sign = 1.0;
  double mag=0, magErr=0,color;
  double zpref,bcref,flux,bcref2,errorflux;
  double nSigma = 3.00;		/* if cCounts < nSigma*cCountsErr, 
				   past errorThresh */ 
  double errorThresh=0;		/* past errorThresh, 
				   use cosmic scatter for calibration*/
  
  int returnValue = 0;
  double bprime[] = {BPRIME_U,BPRIME_G,BPRIME_R,BPRIME_I,BPRIME_Z,BPRIME_SB};
  double cosmicColors[] = {COSMICCOLOR_U_G,COSMICCOLOR_G_R,COSMICCOLOR_R_I,COSMICCOLOR_I_Z,COSMICCOLOR_I_Z};
  double zpColors[] = {ZPCOLOR_U_G,ZPCOLOR_G_R,ZPCOLOR_R_I,ZPCOLOR_I_Z,ZPCOLOR_I_Z};
  double ptCTerm[] = {PT_C_U,PT_C_G,PT_C_R,PT_C_I,PT_C_Z};
  
  /* define status = 0 to be normal completion */
  *status = 0;

  /* If counts error negative, bad measure, so return -1000 indicating bad
   * bad measure. */
  if (fabs(countsErr+9999.0)  < 0.01)
    {
      *calMag = -9999.;
      *calMagErr = -9999.;
       return 0;
    } 

	/* the default cosmicColor */
	if (cosmicColor < -9) {
		cosmicColor = cosmicColors[filterIndex];
	}
	if (zpColor < -9) {
		zpColor = zpColors[filterIndex];
	}
	if (zpAirmass < 1) {
		zpAirmass = COSMIC_AIRMASS;
	}
	if (secColorTerm < -9) {
		secColorTerm = ptCTerm[filterIndex];
	}

	if(filterIndex == 4) sign = -1.0;

  /*
   * convert to counts/second 
   *
   */
  counts = counts/intTime;
  if (fabs(countsErr+1000.0)  < 0.01) {
  countsErr = -1000.0;
	} else {
  countsErr = countsErr/intTime;
	}
  cCounts = cCounts/cIntTime;
  cCountsErr = cCountsErr/cIntTime;

 

   if (cCounts > 0 && counts > 0) {
	color = -2.5*sign*log10(counts/cCounts);
    }



	/* Note quite right yet, what about a big countsErr? HOW TO USE COSMICERROR?? */

        errorThresh = nSigma * countsErr;
    if ( (errorThresh > counts) || (counts <= 0) ) {
	color = cosmicColor;
      *status = 1;
    }

        errorThresh = nSigma * cCountsErr;
    if ( (errorThresh > cCounts) || (cCounts <= 0) ) {
	color = cosmicColor;
      *status = 1;
    }

  /* calc reference flux */
  zpref = pow(10.,(zeropt+extinct*airmass)/2.5);
	/* watch for cosmic color on cterm if big enough... */
  if(cCounts != 0) {
	/* still need to use ref cosmic color zpColor*/
  bcref = pow(10.,color*colorTerm/2.5);
  bcref2 = pow(10.,(color-zpColor)*secColorTerm*(airmass-zpAirmass)/2.5);
  } else {
    bcref = 1;
    bcref2 = 1;
  }
  flux = counts*zpref*bcref*bcref2;
  errorflux = countsErr*zpref*bcref*bcref2;

	/* DO WE NEED SOMETHING LIKE THE FOLLOWING 
*  	if (*status == 1) {
*	  errorflux = cosmicError*zpref*bcref*bcref2;
*	}
	*/

  /*
   * Print debug
printf("zpref %g bcref %g bcref2 %g airmass: %f color:%f zpCOLOR:%f cosmicC:%f zpA:%f c:%f filter:%d flux:%g errorflux:%g countsErr:%f counts %f cCounts %f exptime %f\n",zpref,bcref,bcref2,airmass,color,zpColor,cosmicColor,zpAirmass,secColorTerm,filterIndex,flux,errorflux,countsErr,counts,cCounts,intTime);
    */
    
	if(faintbprime>0) {
		filterIndex = 5;
		bprime[filterIndex] = faintbprime;
	}

    normcnt = flux*flux+4*bprime[filterIndex]*bprime[filterIndex];
    magErr = POGSON*errorflux/sqrt(normcnt);

    mag = -POGSON*(log(bprime[filterIndex])+lasinh(flux/2.0/bprime[filterIndex]));
	if (maggies > 0) {
	mag=flux;
	magErr=errorflux;
	}
	if (maggies == 2) {
	mag *= ABNU_FLUX0;
	magErr *= ABNU_FLUX0;
	}
    
  if ( returnValue == 0) {
    *calMag = mag;
    *calMagErr = magErr;
  }
	if (fabs(countsErr + 1000.0) < 0.01) {
		*calMagErr = -1000.0;
	}
  return returnValue;
}

/*<AUTO EXTRACT>
	ROUTINE: atnLCaliInverst
	DESCRIPTION: Given a calibrated luptitude get the maggies or approx flux in
	Jy from the luptitude. Getting the DN back is more work, as you then
	need all the a,b,c,k, airmass, exp time etc information.
</AUTO>*/

int atnLCaliInvert(
		 double calMag,	/* calibrated luptitude in this band */
		 double calMagErr, /* calibrated error */
		 int filterIndex, /* u=0, g=1, r=2, i=3, z=4 */
		 int maggies,  /* units of returned quantity 1=maggies (default), 2=flux (Jy)*/
	         double faintbprime, /* if 0 use the filterIndex bprime, else use this one */
	         double flux0, /*  flux zeropoint for this band */
		 double *counts, double *countsErr) {

	double  tmpmag,tmpcnts,tmperrcnts;

  double bprime[] = {BPRIME_U,BPRIME_G,BPRIME_R,BPRIME_I,BPRIME_Z,BPRIME_SB};


	if (faintbprime > 0 ) {
		bprime[5]=faintbprime;
		filterIndex = 5;
	}

	if (flux0 <= 0) {
		flux0 = ABNU_FLUX0;
	}


	tmpmag = -1.0*calMag/POGSON - log(bprime[filterIndex]);
	tmpcnts = 2.0*bprime[filterIndex]*(exp(tmpmag)-exp(-tmpmag))/2.0;
  tmperrcnts = calMagErr/POGSON*sqrt(tmpcnts*tmpcnts+4*bprime[filterIndex]*bprime[filterIndex]);
	
	if (maggies > 1) {
		tmpcnts *= flux0;
		tmperrcnts *= flux0;
	}
	
  *counts = tmpcnts;
  *countsErr = tmperrcnts;
  return 0;
}
/*<AUTO EXTRACT>
  ROUTINE: atCaliInvert
  DESCRIPTION: Given a calibrated magnitude and error with cali constants, return
  the number of counts and error in the counts.
</AUTO>*/

int atCaliInvert(
		 double calMag,	/* calibrated magnitude in this band */
		 double cCalMag, /* calibrated magnitude in adjacent band */
		 double intTime, /* integration time in seconds */
		 double airmass, /* airmass */
		 double zeropt,	/* zeropoint */
		 double extinct, /* extinction */
		 double colorTerm, /* color term */
		 double secColorTerm, /* secondary color term */
		 int sign,	/* sign:  +1 for u g r i; -1 for z */
		 double zpAirmass,  /* IN: zeropoint airmass (for c term) */
		 double zpColor,    /* IN: zeropoint color (for c term) */
		 double *counts) {
  double color, mag;
  color = sign * (calMag - cCalMag);
  mag = 
    calMag +			                         /* typical magnitude is 19 */
    zeropt +			                         /* typical zeropoint is -27.0 */
    colorTerm*(color-zpColor) +		                 /* typical colorTerm is -0.05 */
    secColorTerm*(color-zpColor)*(airmass-zpAirmass) +   /* typical secColor term is -0.03 */
    extinct*airmass;		                         /* typical extinction is +0.2 */
  *counts = intTime * pow (10.0, -0.4*mag);
  return 0;
}

/*<AUTO EXTRACT>
	ROUTINE: atLCaliInverse
	DESCRIPTION: Given a calibrated luptitude and cali constants,
	return the number of counts.
</AUTO>*/

int atLCaliInvert(
		 double calMag,	/* calibrated luptitude in this band */
		 double cCalMag, /* calibrated luptitude in adjacent band */
		 double intTime, /* integration time in seconds */
		 double airmass, /* airmass */
		 double zeropt,	/* zeropoint */
		 double extinct, /* extinction */
		 double colorTerm, /* color term */
		 double secColorTerm, /* secondary color term */
		 int sign,	/* sign:  +1 for u g r i; -1 for z */
		 double *counts) {
  double color, mag;
  double tmpcounts;
  color = sign * (calMag - cCalMag);
  mag = 
    calMag +			 /* typical magnitude is 19 */
    zeropt +			 /* typical zeropoint is -27.0 */
    colorTerm*color +		 /* typical colorTerm is -0.05 */
    secColorTerm*color*airmass + /* typical secColor term is -0.03 */
    extinct*airmass;		 /* typical extinction is +0.2 */
    tmpcounts = -0.4*log(10.0)*(mag-2.5/log(10.0)*log(2.0/LUPTITUDE_SCALE));
  *counts = intTime * LUPTITUDE_SCALE * sinh(tmpcounts);
  return 0;
}

/*<AUTO EXTRACT>
	ROUTINE: atLupApply
	DESCRIPTION: Given counts and counts error, return luptitude and luptitude error.
</AUTO>*/

int atLupApply ( double counts,	/* luptitude */
		 double countsErr,	/* luptitude error */
		 double *lup, double *lupErr) {
  double normcnt;
  normcnt = counts*counts + LUPTITUDE_SCALE*LUPTITUDE_SCALE;
  *lupErr = (2.5/log(10.0)) * countsErr /sqrt(normcnt);
  *lup    = -2.5/log(10.0) * (lasinh(counts/LUPTITUDE_SCALE) - log(2.0/LUPTITUDE_SCALE));
  return 0;
}

/*<AUTO EXTRACT>
	ROUTINE: atLupInvert
	DESCRIPTION: Given a luptitude, return the number of counts.
</AUTO>*/

int atLupInvert( double lup,	/* luptitude */
		 double luperr,	/* luptitude error */
		 double *counts, double *countsErr) {
  double tmpcounts;
  tmpcounts = -log(10.0)/2.5*lup + log(2.0/LUPTITUDE_SCALE);
  *counts = LUPTITUDE_SCALE * sinh(tmpcounts);
  *countsErr = log(10.0) / 2.5 * luperr * sqrt((*counts)*(*counts) + LUPTITUDE_SCALE*LUPTITUDE_SCALE);
  return 0;
}
