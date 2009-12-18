#ifndef ATCALI_H
#define ATCALI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dervish.h"

/* This defines the 'A' constant in the arcsinh magnitude definition */
/* Changing from 0.001 as of Feb 99 */
#define LUPTITUDE_SCALE  1.14

/* b' from Lupton, Szalay and Gunn paper, units are Jy/3631 */
#define BPRIME_U 1.4e-10
#define BPRIME_G 0.9e-10
#define BPRIME_R 1.2e-10
#define BPRIME_I 1.8e-10
#define BPRIME_Z 7.4e-10
#define BPRIME_SB 1e-12
/* in Janskys */
#define ABNU_FLUX0 3630.78
#define POGSON 1.0857362
#define COSMIC_AIRMASS 1.3
/* 2.5m Mosaic camera Instrumental cosmic colors */
#define COSMICCOLOR_U_G 2.29
#define COSMICCOLOR_G_R 0.70
#define COSMICCOLOR_R_I 0.10
#define COSMICCOLOR_I_Z -1.30
#define ZPCOLOR_U_G 2.29
#define ZPCOLOR_G_R 0.70
#define ZPCOLOR_R_I 0.10
#define ZPCOLOR_I_Z -1.3
#define PT_C_U -0.021
#define PT_C_G -0.016
#define PT_C_R -0.004
#define PT_C_I 0.006
#define PT_C_Z 0.003


int atCaliApply
  (
   double counts,	/* IN: counts in the reference filter */
   double countsErr,	/* IN: error in counts */
   double cCounts,	/* IN: count in the adjacent filter */
   double cCountsErr,	/* IN: error in cCounts */
   double intTime,	/* IN: exposure time in the reference filter */
   double cIntTime,	/* IN: exposure time in the adjacent filter */
   double airmass,	/* IN: airmass  (sec(Z)) */
   double zeropt,	/* IN: zeropoint (a') */
   double extinct,	/* IN: extinction prime (k') */
   double colorTerm,	/* IN: color term (b') */
   double secColorTerm, /* IN: second order color term (c') */
   int sign,		/* IN: usually +1 for u g r i; -1 for z */
   double cosmicColor,  /* IN: cosmic reference-adjacent color */
   double cosmicError,	/* IN: scatter in cosmicColor */
   double zpAirmass,	/* IN: zeropoint airmass (for c term) */
   double zpColor,	/* IN: zeropoint color (for c term) */
   double *calMag,	/* OUT: calibrated magnitude */
   double *calMagErr,	/* OUT: error in calMag */
   int *status		/* OUT: status */
   );


int atLCaliApply
  (
   double counts,	/* IN: counts in the reference filter */
   double countsErr,	/* IN: error in counts */
   double cCounts,	/* IN: count in the adjacent filter */
   double cCountsErr,	/* IN: error in cCounts */
   double intTime,	/* IN: exposure time in the reference filter */
   double cIntTime,	/* IN: exposure time in the adjacent filter */
   double airmass,	/* IN: airmass  (sec(Z)) */
   double zeropt,	/* IN: zeropoint (a') */
   double extinct,	/* IN: extinction prime (k') */
   double colorTerm,	/* IN: color term (b') */
   double secColorTerm, /* IN: second order color term (c') */
   int sign,		/* IN: usually +1 for u g r i; -1 for z */
   double cosmicColor,  /* IN: cosmic reference-adjacent color */
   double cosmicError,	/* IN: scatter in cosmicColor */
   double *calMag,	/* OUT: calibrated luptitude */
   double *calMagErr,	/* OUT: error in calMag */
   int *status		/* OUT: status */
   );
int atnLCaliApply
  (
   double counts,	/* IN: counts in the reference filter */
   double countsErr,	/* IN: error in counts */
   double cCounts,	/* IN: count in the adjacent filter */
   double cCountsErr,	/* IN: error in cCounts */
   double intTime,	/* IN: exposure time in the reference filter */
   double cIntTime,	/* IN: exposure time in the adjacent filter */
   double airmass,	/* IN: airmass  (sec(Z)) */
   double zeropt,	/* IN: zeropoint (a') */
   double extinct,	/* IN: extinction prime (k') */
   double colorTerm,	/* IN: color term (b') */
   double secColorTerm, /* IN: second order color term (c') */
   double cosmicColor,  /* IN: cosmic reference-adjacent color */
   double cosmicError,	/* IN: scatter in cosmicColor */
   double zpAirmass,	/* IN: scatter in cosmicColor */
   double zpColor,	/* IN: X0=1.3 */
   int fluxIndex,	/* IN: fluxIndex */
   double faintbprime,	/* IN: alternative */
   int maggies,		/* IN: do you want maggies, lmags or flux */
   double *calMag,	/* OUT: calibrated luptitude */
   double *calMagErr,	/* OUT: error in calMag */
   int *status		/* OUT: status */
   );

int atnLCaliInvert(
                 double calMag, /* calibrated luptitude in this band */
                 double calMagErr, /* calibrated error */
                 int filterIndex, /* u=0, g=1, r=2, i=3, z=4 */
                 int maggies,  /* units of returned quantity 1=maggies (default), 2=flux (Jy)*/
                 double faintbprime, /* if 0 use the filterIndex bprime, else use this one */
                 double flux0, /*  flux zeropoint for this band */
                 double *counts,  /* OUT: returned maggies or Jy */
		double *countsErr /* OUT: returned error */
	);


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
		 double *counts);

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
		 double *counts);

int atLupApply ( double counts, /* luptitude */
                 double countsErr,      /* luptitude error */
                 double *lup, double *lupErr);

int atLupInvert( double lup,    /* luptitude */
                 double luperr, /* luptitude error */
                 double *counts, double *countsErr);

#ifdef __cplusplus
}
#endif

#endif
