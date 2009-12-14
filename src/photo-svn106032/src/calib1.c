/* Support for the CALIB1{,BYTIME,BYFRAME} structures */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dervish.h"
#include "phCalib1.h"

CALIB1 *
phCalib1New(const char filtername)
{
  CALIB1 *new = shMalloc(sizeof(CALIB1));

  sprintf(new->filter, "%c", filtername);
  new->status = PSP_FIELD_UNKNOWN;
  new->sky = 0;
  new->skyerr = 0;
  new->skysig = 0;
  new->skyslope = 0;
  new->lbias = 0;
  new->rbias = 0;
  new->lbiasOEdiff = 0;
  new->rbiasOEdiff = 0;
  new->flux20 = -1;
  new->flux20Err = 0;
  new->psf_nstar = 0;
  new->psf_ap_correctionErr = new->psf_ap_correctionBias = 0;
  new->psf_wingcorr = -999;
  new->nann_ap_frame = 0; 
  new->nann_ap_run = 0; 
  new->ap_corr_run = 0; 
  new->ap_corr_runErr = 0; 
  new->psf = NULL;
  new->prof = phCompositeProfileNew(); 
  new->node = new->incl = 0.0;
  new->toGCC = NULL;
  new->gain = 0;
  new->dark_variance = 0;
  return(new);

}

void
phCalib1Del(CALIB1 *calib1)
{
  if (calib1->psf!=NULL) phDgpsfDel(calib1->psf);
  if (calib1->prof!=NULL) phCompositeProfileDel(calib1->prof); 
  if (calib1->toGCC!=NULL) atTransDel(calib1->toGCC);
  shFree(calib1);
}


CALIB1BYTIME *
phCalib1bytimeNew(const char *colors)
{
  int i;
  int ncolors;
  CALIB1BYTIME *new;

  shAssert(colors != NULL);
  ncolors = strlen(colors);

  new = shMalloc(sizeof(CALIB1BYTIME));

  new->mjd = -1.0;
  new->nearest_frame = -1;
  new->ncolors = ncolors;
  new->calib= (CALIB1 **)shMalloc(ncolors*sizeof(CALIB1 *));
  for (i=0; i<ncolors; i++) {
      new->calib[i] = phCalib1New(colors[i]);
  }

  return(new);
}

void
phCalib1bytimeDel(CALIB1BYTIME *calib1bytime)
{
  int i;

  for (i=0; i<calib1bytime->ncolors; i++) {
      if (calib1bytime->calib[i]!=NULL) {
	  phCalib1Del(calib1bytime->calib[i]);
      }
  }
  shFree(calib1bytime->calib);
  shFree(calib1bytime);
}

/*****************************************************************************/
/* Make a CALIB_IO from a calib1bytime*/
CALIB_IO *
phCalibIoNewFromCalib1bytime(const CALIB1BYTIME *ctime)
{
    int c;
    CALIB_IO *new;
    
    shAssert(ctime != NULL);

    new = phCalibIoNew(ctime->ncolors);
    
    new->mjd = ctime->mjd;
    for (c=0; c < ctime->ncolors; c++) {
	new->flux20[c] = ctime->calib[c]->flux20;
	new->flux20Err[c] = ctime->calib[c]->flux20Err;
    }
    return(new);
}
/*****************************************************************************/
/* Make a CALIB1BYTIME from a CALIB_IO */

CALIB1BYTIME *
phCalib1bytimeNewFromCalibIo(const CALIB_IO *calibio, 
			     const char *filters,
			     const int *index) 
{
    int c;
    CALIB1BYTIME *new;
    int ncolor;

    shAssert(filters != NULL);
    ncolor = strlen(filters);

    new = phCalib1bytimeNew(filters);

    new->mjd = calibio->mjd;
    for (c = 0; c < ncolor; c++) {
	sprintf(new->calib[c]->filter, "%c", filters[c]);
	new->calib[c]->flux20 = calibio->flux20[index[c]];
	new->calib[c]->flux20Err = calibio->flux20Err[index[c]];

    }
    return(new);
}

/*****************************************************************************/

CALIB1BYFRAME *
phCalib1byframeNew(const char *colors)
{
  int i;
  int ncolors;
  CALIB1BYFRAME *new;

  shAssert(colors != NULL);
  ncolors = strlen(colors);
  
  new = shMalloc(sizeof(CALIB1BYFRAME));
  new->field = -1;
  new->ncolors = ncolors;
  new->psp_status = PSP_FIELD_UNKNOWN;
  new->calib = (CALIB1 **)shMalloc(ncolors*sizeof(CALIB1 *));
  for (i=0; i<ncolors; i++) {
     int cc = colors[i];
     if(ncolors == 1) {
	;				/* any filtername is allowed */
     } else {
	shAssert(strchr(atFilternames, cc) != NULL); /* only SDSS filters */
     }

     new->calib[i] = phCalib1New(cc);
  }

  return(new);
}

void
phCalib1byframeDel(CALIB1BYFRAME *calib1byframe)
{
  int i;

  if(calib1byframe == NULL) {
     return;
  }
  
  for (i=0; i<calib1byframe->ncolors; i++) {
      if (calib1byframe->calib[i] != NULL) {
	  phCalib1Del(calib1byframe->calib[i]);
      }
  }
  shFree(calib1byframe->calib);
  shFree(calib1byframe);
}
/*****************************************************************************/
/* Make a CALIB_IO */

CALIB_IO *
phCalibIoNew(int ncolor) 
{
    CALIB_IO *new;

    shAssert(ncolor <= NCOLOR);
    new = shMalloc(sizeof(CALIB_IO));
    *(int *)&new->ncolor = ncolor;
    
    return(new);
}

/*****************************************************************************/
/* Delete a CALIB_IO */

void 
phCalibIoDel(CALIB_IO *calibio) 
{
    if (calibio == NULL) return;
    shFree(calibio);
}


/*****************************************************************************/
/* Make a CALIB_IO from a calib1byframe*/
CALIB_IO 
*phCalibIoNewFromCalib1byframe(const CALIB1BYFRAME *cframe)
{
    int c;
    CALIB_IO *new;
    
    shAssert(cframe != NULL);
    
    new = phCalibIoNew(cframe->ncolors);
    
    new->field = cframe->field;
    new->mjd = -1.;
    new->psp_status = cframe->psp_status;
    new->node = cframe->calib[0]->node; /* below we check they're same */
    new->incl = cframe->calib[0]->incl;	/* for all filters. */
    for (c=0; c < cframe->ncolors; c++) {
	shAssert(cframe->calib[c]->psf != NULL);
	shAssert(cframe->calib[c]->toGCC != NULL);
	shAssert(new->node == cframe->calib[c]->node);
	shAssert(new->incl == cframe->calib[c]->incl);

	new->status[c] = cframe->calib[c]->status;
	new->sky[c] = cframe->calib[c]->sky;
	new->skyerr[c] = cframe->calib[c]->skyerr;
	new->skysig[c] = cframe->calib[c]->skysig;
	new->skyslope[c] = cframe->calib[c]->skyslope;
	new->lbias[c] = cframe->calib[c]->lbias;
	new->rbias[c] = cframe->calib[c]->rbias;
	new->flux20[c] = cframe->calib[c]->flux20;
	new->flux20Err[c] = cframe->calib[c]->flux20Err;
	new->psf_ap_correctionErr[c] = cframe->calib[c]->psf_ap_correctionErr;
        new->nann_ap_frame[c] = cframe->calib[c]->nann_ap_frame; 
        new->nann_ap_run[c] = cframe->calib[c]->nann_ap_run; 
        new->ap_corr_run[c] = cframe->calib[c]->ap_corr_run; 
        new->ap_corr_runErr[c] = cframe->calib[c]->ap_corr_runErr; 
	new->psf_nstar[c] = cframe->calib[c]->psf_nstar;
	new->psf_sigma1[c] = cframe->calib[c]->psf->sigmax1;
	new->psf_sigma2[c] = cframe->calib[c]->psf->sigmax2;
	new->psf_sigmax1[c] = cframe->calib[c]->psf->sigmay1;
	new->psf_sigmax2[c] = cframe->calib[c]->psf->sigmay2; 
	new->psf_sigmay1[c] = cframe->calib[c]->psf->sigmay1;
	new->psf_sigmay2[c] = cframe->calib[c]->psf->sigmay2; 
	new->psf_b[c] = cframe->calib[c]->psf->b;
	new->psf_p0[c] = cframe->calib[c]->psf->p0;
	new->psf_beta[c] = cframe->calib[c]->psf->beta;
	new->psf_sigmap[c] = cframe->calib[c]->psf->sigmap;
	new->psf_width[c] = cframe->calib[c]->psf->width;
	new->psf_psfCounts[c] = cframe->calib[c]->psf->psfCounts;
	new->psf_sigma1_2G[c] = cframe->calib[c]->psf->sigma1_2G;
	new->psf_sigma2_2G[c] = cframe->calib[c]->psf->sigma2_2G;
	new->psf_b_2G[c] = cframe->calib[c]->psf->b_2G;


        new->psfCounts[c] = cframe->calib[c]->prof->psfCounts;
        new->prof_nprof[c] = cframe->calib[c]->prof->nprof;
	memcpy(new->prof_mean[c],cframe->calib[c]->prof->mean,
	                       sizeof(new->prof_mean[c]));
	memcpy(new->prof_med[c],cframe->calib[c]->prof->med,
	                       sizeof(new->prof_med[c]));
	memcpy(new->prof_sig[c],cframe->calib[c]->prof->sig,
	                       sizeof(new->prof_sig[c]));

	new->gain[c] = cframe->calib[c]->gain;
	new->dark_variance[c] = cframe->calib[c]->dark_variance;
    }
    return(new);
}
/*****************************************************************************/
/* Make a CALIB1BYFRAME from a CALIB_IO */

CALIB1BYFRAME 
*phCalib1byframeNewFromCalibIo(const CALIB_IO *calibio, 
			       const char *filters,
			       const int *index) 
{
    int c;
    int ncolor;
    CALIB1BYFRAME *new;

    shAssert(filters != NULL);
    ncolor = strlen(filters);

    new = phCalib1byframeNew(filters);

    new->field = calibio->field;
    for (c = 0; c < ncolor; c++) {
	new->psp_status = calibio->psp_status;
	new->calib[c]->status = calibio->status[index[c]];
        sprintf(new->calib[c]->filter, "%c", filters[c]);
	new->calib[c]->sky = calibio->sky[index[c]];
	new->calib[c]->skyerr = calibio->skyerr[index[c]];
	new->calib[c]->skysig = calibio->skysig[index[c]];
	new->calib[c]->skyslope = calibio->skyslope[index[c]];
	new->calib[c]->lbias = calibio->lbias[index[c]];
	new->calib[c]->rbias = calibio->rbias[index[c]];
	new->calib[c]->flux20 = calibio->flux20[index[c]];
	new->calib[c]->flux20Err = calibio->flux20Err[index[c]];
	new->calib[c]->psf_ap_correctionErr = calibio->psf_ap_correctionErr[index[c]];
	new->calib[c]->nann_ap_frame = calibio->nann_ap_frame[index[c]];
	new->calib[c]->nann_ap_run = calibio->nann_ap_run[index[c]];
	new->calib[c]->ap_corr_run = calibio->ap_corr_run[index[c]];
	new->calib[c]->ap_corr_runErr = calibio->ap_corr_runErr[index[c]];
	new->calib[c]->psf_nstar = calibio->psf_nstar[index[c]];
	new->calib[c]->psf = phDgpsfNew();
	new->calib[c]->psf->a = 1.0;
	new->calib[c]->psf->sigmax1 = calibio->psf_sigma1[index[c]];
	new->calib[c]->psf->sigmax2 = calibio->psf_sigma2[index[c]];
	new->calib[c]->psf->sigmay1 = calibio->psf_sigma1[index[c]];
	new->calib[c]->psf->sigmay2 = calibio->psf_sigma2[index[c]];
	new->calib[c]->psf->b = calibio->psf_b[index[c]];
	new->calib[c]->psf->p0 = calibio->psf_p0[index[c]];
	new->calib[c]->psf->beta = calibio->psf_beta[index[c]];
	new->calib[c]->psf->sigmap = calibio->psf_sigmap[index[c]];
	new->calib[c]->psf->width = calibio->psf_width[index[c]];
	new->calib[c]->psf->psfCounts = calibio->psf_psfCounts[index[c]];
	new->calib[c]->psf->sigma1_2G = calibio->psf_sigma1_2G[index[c]];
	new->calib[c]->psf->sigma2_2G = calibio->psf_sigma2_2G[index[c]];
	new->calib[c]->psf->b_2G = calibio->psf_b_2G[index[c]];

        new->calib[c]->prof->psfCounts =  calibio->psfCounts[index[c]]; 
        new->calib[c]->prof->nprof = calibio->prof_nprof[index[c]];  
	memcpy(new->calib[c]->prof->mean, calibio->prof_mean[index[c]],
						     sizeof(calibio->prof_mean[c]));
	memcpy(new->calib[c]->prof->med, calibio->prof_med[index[c]],
						      sizeof(calibio->prof_med[c]));
	memcpy(new->calib[c]->prof->sig, calibio->prof_sig[index[c]],
						      sizeof(calibio->prof_sig[c]));
 
	new->calib[c]->gain = calibio->gain[index[c]];
	new->calib[c]->dark_variance = calibio->dark_variance[index[c]]; 

    }
    return(new);
}
