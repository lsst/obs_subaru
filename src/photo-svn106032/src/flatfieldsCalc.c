/*
 * <AUTO>
 * FILE: flatfieldsCalc.c 
 *
 * create 1-D flatfield vector from a 2-D, night-long flatfield frame
 *
 * </AUTO>
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "dervish.h"
#include "phQuartiles.h"
#include "phCorrectFrames.h"
#include "phCorrectFrames_p.h"    /* Private definitions  */
#include "phConsts.h"

#define dump2file 0

/* 
 * if medianBiasDrift is true, use median to get bias drift in overscan,
 * and mean otherwise. Mean was fine until binned scans (see PR 5322).
 */
#define medianBiasDrift 1

/*************************************************************************
 * A little function used to compare two integers for qsort.
 * 
 */
static int 
cmp(const void *a, const void *b) 
{ 
   return(*(int *)a - *(int *)b);
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



/*
 * <AUTO EXTRACT>
 *
 * This is the science module flatfieldsCalc which
 * generates a 1D bias vector, a cleaned, inverse flat 
 * field array and a cleaned sky level array. 
 *
 * returns:
 *   SH_SUCCESS                    always
 */
int
phFlatfieldsCalc(
		 QUARTILES *quartiles,	/* I: the quartile array for the night
					   and passband to be processed */
		 REGION *bias_vec,	/* I: 1D bias vector from db */
		 int bias_scale,	/* I: factor bias is multiplied by */
                 int left_buffer,       /* I: do not include last left_buffer
                                           pixels in left overscan region when
                                           determining bias drift */
                 int right_buffer,      /* I: do not include first right_buffer
                                           pixels in right overscan region when
                                           determining bias drift */ 
		 CCDPARS *ccdpars,	/* I: CCD parameters data structure */

		 REGION *flat2d,	/* O: 2D flat field array. This is 
					   required by phFlatfieldsInterp */
		 REGION *skylev,	/* O: 1D sky (col0) and skysigma (col1)
					   array. Also required
					   by phFlatfieldsInterp */
		 REGION *drift,	        /* O: bias drift correction array */
		 REGION *Odrift,	/* O: odd bias drift correction array */
		 REGION *Edrift		/* O: even bias drift correction array */
		 )
{
	int i, j, k, n, nE, nO, c;
	int scol, ecol;
        int neg_quart;                  /* counts cols with neg. quartiles */
	int b;				/* counter for bad1col[] */
	int bad1col[NBAD1COLMAX + 1];		/* positions of bad columns */
	int imbuf[MAXBUF];
	int biassum=0, Obiassum=0, Ebiassum=0, bval;
        int prebiasscol[2], prebiasecol[2], postbiasscol[2], postbiasecol[2];
        int biasArray[100], ObiasArray[100], EbiasArray[100];
	float fval, sum;
	float iqr;			/* interquartile range */
	int nc;				/* number of cols of data (not bias) */
	int nbad1col;			/* number of single-column defects */
	float quart_to_flat;		/* convert an input quartile intensity
					   to the same scale as a flat field */
	float bias_to_flat;		/* convert a bias intensity to the
					   same scale as a flat field */
	float OEdiff;                   /* difference between odd and even drift */
#if dump2file
        float sum25 = 0, sum50 = 0, sum75 = 0, mode;
        FILE *fp;
     
        fp = fopen("quartiles.dat","w");  
        fprintf(fp,"frame        Q25          Q50          Q75        mode\n");
#endif
/*
 *	Check for improper input parameter values
 */
	shAssert(quartiles->tscal != 0);
	shAssert(bias_scale != 0);
	shAssert(bias_vec->type == TYPE_U16);
	shAssert(drift->type == TYPE_S16);
	shAssert(Odrift->type == TYPE_S16);
	shAssert(Edrift->type == TYPE_S16);
	shAssert(skylev->type == TYPE_FL32);

	quart_to_flat = (float)FSHIFT/quartiles->tscal;
	bias_to_flat = (float)FSHIFT/bias_scale;
/*
 *	Define bias and data regions limits
 *      Assume we use overscan only, not bias region
 */
	prebiasscol[0] = ccdpars->sPrescan0;
	prebiasecol[0] = ccdpars->nPrescan0 + ccdpars->sPrescan0 - 1;
	postbiasscol[0] = ccdpars->sPostscan0;
	postbiasecol[0] = ccdpars->nPostscan0 + ccdpars->sPostscan0 - 1;

	if (ccdpars->namps > 1) {
	  shAssert(ccdpars->namps == 2);
	  prebiasscol[1] = ccdpars->sPrescan1;
	  prebiasecol[1] = ccdpars->nPrescan1 + ccdpars->sPrescan1 - 1;
	  postbiasscol[1] = ccdpars->sPostscan1;
	  postbiasecol[1] = ccdpars->nPostscan1 + ccdpars->sPostscan1 - 1;
/*
 *      Note: scol & ecol are the starting and ending data columns
 *            (ie, they do not include bias areas)
 */

/*      This reasonably assumes there is never a gap between the two data 
 *      sections, but let's assert it just to be safe
 */
	  shAssert(ccdpars->sData1 == (ccdpars->sData0 + ccdpars->nData0));

	  ecol = ccdpars->sData1 + ccdpars->nData1 - 1;
	} else {
	  ecol = ccdpars->sData0 + ccdpars->nData0 - 1;
	}
	scol = ccdpars->sData0;


	nc = ecol - scol + 1;
/*
 *	Compute Overscan Drift Corrections; they are in scaled-bias units.
 *      Note that we ignore any quartile datum < 0; that indicates that
 *      there was an error in calculating the quartiles for some column.
 */
	for (k = 0; k < ccdpars->namps; k++) {
	   for (i = 0; i < quartiles->nr; i++) {
	      if(nc == quartiles->data[0]->ncol) { /* no bias region */
		 drift->rows_s16[i][k] = 0;
		 Odrift->rows_s16[i][k] = 0;
		 Edrift->rows_s16[i][k] = 0;
		 continue;
	      }

	      drift->rows_s16[i][k] = 0;
	      Odrift->rows_s16[i][k] = 0;
	      Edrift->rows_s16[i][k] = 0;
	      biassum = 0;
	      Obiassum = 0;
	      Ebiassum = 0;
              n = 0; nO = 0; nE = 0;
              for (j = prebiasscol[k]; j <= prebiasecol[k]-left_buffer; j++) {
                 if (quartiles->data[1]->rows_s32[i][j] < 0) {
                    continue;
                 }
                 /* drift values will be scaled up by FSHIFT */
		 bval = quartiles->data[1]->rows_s32[i][j]*quart_to_flat - 
                                      bias_vec->rows_u16[0][j]*bias_to_flat;
                 if (j/2*2 == j) {
		    /* even */
                    Ebiassum += bval;
                    EbiasArray[nE] = bval;
                    nE++;   
                 } else {
                    Obiassum += bval;
                    ObiasArray[nO] = bval;
                    nO++;   
                 }
                 biassum += bval;
                 biasArray[n] = bval;
                 n++;   
	      }
              for (j = postbiasscol[k]+right_buffer; j <= postbiasecol[k]; j++) {
                 if (quartiles->data[1]->rows_s32[i][j] < 0) {
                    continue;
                 }
                 /* drift values will be scaled up by FSHIFT */
		 bval = quartiles->data[1]->rows_s32[i][j]*quart_to_flat - 
                                      bias_vec->rows_u16[0][j]*bias_to_flat;
                 if (j/2*2 == j) {
		    /* even */
                    Ebiassum += bval;
                    EbiasArray[nE] = bval;
                    nE++;   
                 } else {
                    Obiassum += bval;
                    ObiasArray[nO] = bval;
                    nO++;   
                 }
                 biassum += bval;
                 biasArray[n] = bval;
                 n++;   
	      }
	      if(n == 0) {		/* no good columns */
		 drift->rows_s16[i][k] = MAX_S16;
	      } else {
		 /* convert from "scaled by FSHIFT" to "scaled by bias_scale"*/
#if medianBiasDrift 
                 float medianB = get_quartile(biasArray,n,0,0.50);
                 int tmp = 0.5 + medianB/bias_to_flat;
#else
		 int tmp = biassum/(float)n/bias_to_flat + 0.5;
#endif

#if 0
		 shAssert(tmp >= MIN_S16 && tmp <= MAX_S16);
#else
		 if (tmp < MIN_S16 || tmp > MAX_S16) {
		     shError("Out-of-range value in %s bias at quartile row %d; "
			     "setting to MAX_S16", (k == 0 ? "left" : "right"), i);
		     nO = nE = 0;
		     tmp = MAX_S16;
		 }

#endif
		 drift->rows_s16[i][k] = tmp;
	      }
	      if(nO == 0) {		/* no good columns */
		 Odrift->rows_s16[i][k] = MAX_S16;
	      } else {
#if medianBiasDrift 
                 float medianB = get_quartile(ObiasArray,nO,0,0.50);
                 int tmpO = 0.5 + medianB/bias_to_flat;
#else
		 int tmpO = Obiassum/(float)nO/bias_to_flat + 0.5;
#endif
		 shAssert(tmpO >= MIN_S16 && tmpO <= MAX_S16);
		 Odrift->rows_s16[i][k] = tmpO;
	      }
	      if(nE == 0) {		/* no good columns */
		 Edrift->rows_s16[i][k] = MAX_S16;
	      } else {
#if medianBiasDrift 
                 float medianB = get_quartile(EbiasArray,nE,0,0.50);
                 int tmpE = 0.5 + medianB/bias_to_flat;
#else
		 int tmpE = Ebiassum/(float)nE/bias_to_flat + 0.5;
#endif
		 shAssert(tmpE >= MIN_S16 && tmpE <= MAX_S16);
		 Edrift->rows_s16[i][k] = tmpE;
	      }
	   }
	}
/*
 * Read the defect structure and prepare to fix them. 
 */
   {
      CHAIN *chain = ccdpars->CCDDefect; /* the chain of defects */
      CURSOR_T curs;			/* chain's cursor */
      CCDDEFECT *defect;		/* a defect */

      shAssert(chain != NULL);
      
      curs = shChainCursorNew(chain);
      nbad1col = 0;

      defect = (CCDDEFECT *)shChainCursorSet(chain,curs,HEAD);
      while((defect = (CCDDEFECT *)shChainWalk(chain,curs,NEXT)) != NULL) {
	if(defect->dfaction == BADCOL || defect->dfaction == FILCOL) {
	    if(nbad1col >= NBAD1COLMAX - 1) {
	       shFatal("phFlatfieldsCalc: "
		       "I can only handle %d bad columns per chip\n",
		       NBAD1COLMAX);
	    }
	    
	    for (c = 0; c < defect->dfncol; c++) {
		i = defect->dfcol0 + scol + c; 
		if(i >= scol && i <= ecol) {
		    bad1col[nbad1col++] = i;
		} else {
		    shFatal("phFlatfieldsCalc: Defect in overclocked column");
		} 
	    }
	 } else {
	   shFatal("phFlatfieldsCalc: Found but can't process a "
		   "type %d defect\n",defect->dfaction);
	 }
      }
      shChainCursorDel(chain,curs);

      qsort(bad1col,nbad1col,sizeof(int),cmp);
      bad1col[nbad1col] = -1;		/* set the end marker */
      
   }
/*
 *	Compute the Flat field and sky level array data
 *
 *	The quartile data are combined to give an estimate of the
 *	mode. The data are bias subtracted and corrected for bias drifts.
 *      The average of every row of corrected mode data is computed
 *      and used as the sky level estimate as well as for the normalization
 *      of the flat field data for that quartile row. The final flat field
 *      data are the inverse of the corrected mode data normalized row by
 *      row using the mean level.
 *
 *	We have to reject bad columns in finding the mean sky, of course
 *      We'll set these to FSHIFT, which means the "normalized" flatfield 
 *      value. In the flatfield vectors written to disk, these columns will
 *      all be pegged at 1, which is ok, since we'll interpolate over them in
 *      correctFrames.
 *
 *      We also reject any columns whose quartile(s) is negative --
 *      this occurs when the quartile-finding code fails; the value <
 *      0 is an error code.  We'll set such columns to have the value
 *      0, so we can detect them in flatfieldsInterp. If we just
 *      treated them the same as the bad columns, they would be
 *      weighted inappropriately in flatfieldsInterp.
 */

	for (i = 0; i < quartiles->nr; i++) {
	   b = 0;
           neg_quart = 0;
	   for (iqr=0, sum = 0, j = scol; j <= ecol; j++) {
              if (ccdpars->namps == 1) {
                 k = 0;
              } else if (j < ccdpars->sData1) {
                 k = 0;
              } else {
                 k = 1;
              }

              /* skip any known-to-be-bad column */
	      if(j == bad1col[b]) {	/* this is a bad column */
		 b++;                   /* increment count of cols we skip */
		 imbuf[j - scol] = 0;
		 continue;		/* so don't include it in the mean */
	      }

              /* also skip any column with NEGATIVE quartile value(s),
		 or a bad drift value */
              if (quartiles->data[0]->rows_s32[i][j] < 0 ||
                  quartiles->data[1]->rows_s32[i][j] < 0 ||
                  quartiles->data[2]->rows_s32[i][j] < 0 ||
		  drift->rows_s16[i][k] == MAX_S16 ||
		  Odrift->rows_s16[i][k] == MAX_S16 ||
		  Edrift->rows_s16[i][k] == MAX_S16) {
                 neg_quart++;  
                 imbuf[j - scol] = -1;
                 continue;
              }

	      /* Now use clipped median */
              /* Note that the bias drift depends on column's parity */
              if (j/2*2 == j) {
		  /* even column */
                  OEdiff = Edrift->rows_s16[i][k] - Odrift->rows_s16[i][k];
              } else {
		  /* odd column */
                  OEdiff = Odrift->rows_s16[i][k] - Edrift->rows_s16[i][k];
              }
              /* N.B. here we explicitly include the odd/even effect into background */
              fval = quartiles->data[1]->rows_s32[i][j] * quart_to_flat -
		     (bias_vec->rows_u16[0][j] + drift->rows_s16[i][k] + 0.5*OEdiff) 
                     * bias_to_flat;

              sum += fval;
	      imbuf[j - scol] = fval;

	      iqr += quartiles->data[2]->rows_s32[i][j] - 
		quartiles->data[0]->rows_s32[i][j];
#if dump2file
   if (j/2*2 == j) { 
       sum25 += quartiles->data[0]->rows_s32[i][j] * quart_to_flat -
		(bias_vec->rows_u16[0][j] + Edrift->rows_s16[i][k]) * bias_to_flat;
       sum50 += quartiles->data[1]->rows_s32[i][j] * quart_to_flat -
		(bias_vec->rows_u16[0][j] + Edrift->rows_s16[i][k]) * bias_to_flat;
       sum75 += quartiles->data[2]->rows_s32[i][j] * quart_to_flat -
		(bias_vec->rows_u16[0][j] + Edrift->rows_s16[i][k]) * bias_to_flat;
   } else {
       sum25 += quartiles->data[0]->rows_s32[i][j] * quart_to_flat -
		(bias_vec->rows_u16[0][j] + Odrift->rows_s16[i][k]) * bias_to_flat;
       sum50 += quartiles->data[1]->rows_s32[i][j] * quart_to_flat -
		(bias_vec->rows_u16[0][j] + Odrift->rows_s16[i][k]) * bias_to_flat;
       sum75 += quartiles->data[2]->rows_s32[i][j] * quart_to_flat -
		(bias_vec->rows_u16[0][j] + Odrift->rows_s16[i][k]) * bias_to_flat;
   }
#endif
           }
	   
	   n = nc - (b + neg_quart);
	   if(n == 0) {			/* _no_ good columns */
	      sum = iqr = 0;
	   } else {
	      sum /= n;
	      iqr /= (n * quartiles->tscal); /* the average SIQR. */
#if dump2file
	      sum25 /= n;
	      sum50 /= n;
	      sum75 /= n;
 	      /* NO Scaling (in DN units) */ 
	      sum25 /= FSHIFT;
	      sum50 /= FSHIFT;
	      sum75 /= FSHIFT;
              mode = sum50 - 2.2 * (sum25 + sum75 - 2 * sum50);
           fprintf(fp," %d   %12.2f %12.2f %12.2f %12.2f\n",i, sum25, sum50, sum75, mode);
#endif
	   }
/*
 *	   Store sky level data with NO Scaling (ie, 1 array unit = 1 DN)
 */
           skylev->rows_fl32[i][0] = sum/FSHIFT;
	   if(n != 0) {
	      skylev->rows_fl32[i][1] = iqr*IQR_TO_SIGMA/sqrt(n);
	   } else {
	      skylev->rows_fl32[i][1] = 0.0;
           }
	   skylev->rows_fl32[i][2] = iqr*IQR_TO_SIGMA;
/*
 *	   Now store NOT inverted, normalized, and scaled flat field data
 */
           for (j = 0; j < nc; j++) {
              fval = imbuf[j];
	      flat2d->rows_u16[i][j] = (sum <= 0) ? 0 :
		(fval > 0) ? FSHIFT*fval/sum : ((fval == 0) ? FSHIFT : 0);
           }
	}
#if dump2file
    fclose(fp);
#endif
	return (SH_SUCCESS);
}

