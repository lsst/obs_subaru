#ifndef ATTRANS_H
#define ATTRANS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dervish.h"

/* STRUCTURE */
/*
  TRANS structures are SDSS-specific coordinate transformations from frame
  coordinates (row,col) to great circle coordinates (mu, nu).
  The transformation is of the form:

  r'-i' < riCut:

      rowm = row + dRow0 + dRow1*col + dRow2*(col^2) + dRow3*(col^3) + csRow*c
      colm = col + dCol0 + dCol1*col + dCol2*(col^2) + dCol3*(col^3) + csCol*c

  r'-i' >= riCut

      rowm = row + dRow0 + dRow1*col + dRow2*(col^2) + dRow3*(col^3) + ccRow
      colm = col + dCol0 + dCol1*col + dCol2*(col^2) + dCol3*(col^3) + ccCol

  mu = a + b * rowm + c * colm
  nu = d + e * rowm + f * colm

  (row, col) are the measured frame coordinates (measured in pixels).  dRow0,
  dRow1, dRow2, dRow3, dCol0, dCol1, dCol2, and dCol3 are the coefficients
  to correct for high-order optical distortions in the camera (because TRANS
  structures are set up for drift scan data, high-order optical distortions
  are a function of column only). c is the color of the star (which color
  depends on which filter this TRANS applies to), so csRow and csCol correct
  for differential chromatic refraction (DCR, where the other terms in the
  TRANS are set up such that DCR = 0 for c = 0).  If the r'-i' color is
  greater than or equal to a given value (riCut), then a constant value is
  used for DCR (ccRow in row, ccCol in col; for some filters this is not the
  case, which is handled by setting riCut to a very large value which will
  never be exceeded in practice).  (rowm, colm) are thus the frame coordinates
  corrected for optical distortions and DCR (in pixels).  A simple affine
  transformation then converts (rowm, colm) to great circle coordinates
  (mu,nu, in degrees) on the sky.

  The errors, muErr and nuErr, are the errors in the transformation (that is,
  the rms systematic errors) in great circle longitude and latitude,
  respectively (in arcsec). For the r' chips, this is the error in the
  absolute astrometry (r' coordinates to sky). For the u', g', i', and z'
  chips, this is the error in the relative astrometry (u', g', i', or z'
  to r'). For example, to transform g' pixel coordinates to r' pixel
  coordinates, one applies the g' TRANS structure to the g' coordinates, and
  then the inverse r' TRANS structure to the intermediate sky coordinates.
  The total error in this transformation is just the errors listed in the g'
  TRANS structure. Those errors are in arcsec; to convert to errors in the
  transformed r' coordinates just divide by the pixel scale for the r' CCD.   
*/

typedef struct Trans {
  int id;		     /* identifier */
  double a,b,c,d,e,f;        /* affine transformation terms */
  double dRow0, dRow1, dRow2, dRow3; /* row optical distortion terms */
  double dCol0, dCol1, dCol2, dCol3; /* column optical distortion terms */
  double csRow, csCol;	     /* DCR slope terms (used when r'-i' < riCut) */
  double ccRow, ccCol;	     /* DCR constant terms (used when r'-i' >= riCut)*/
  double riCut;		     /* r'-i' cutoff value(>= which use constant DCR)*/
  double mjd;                /* MJD(TAI) when row 0 of the frame was read */
  double airmass;            /*Airmass for star at frame center(mid exposure)*/
  double muErr;              /* Mu error in transformation (arcsec) */
  double nuErr;              /* Nu error in transformation (arcsec) */
} TRANS;

#define AT_NBAND 5			/* number of bands used; u g r i z */
extern const char *atFilternames;	/* names of valid filters */

/* Prototypes for the TRANS Manipulation Routines */

TRANS *atTransNew(void);
RET_CODE atTransDel(TRANS *trans);
const float *atCosmicMagSet(const float *mag);
const float *atCosmicMagScatterSet(const float *magScatter);

int
atTransApply(const TRANS *trans,	/* TRANS structure for the frame */
	     char filter,		/* filter in which row, and col are
					   measured */
	     double row,		/* frame row position (pixels) */
	     double rowErr,		/* error in row position (pixels) */
	     double col,		/* frame column position (pixels) */
	     double colErr,		/* error in column position (pixels) */
	     const float *mag,		/* magnitudes for this object, or NULL
					   (only the colours are used)*/
	     const float *magErr,	/* error in mag, or NULL */
	     double *mu,		/* OUT: great circle longitude
					   (degrees) */
	     double *muErr,		/* OUT: error in mu, or NULL */
	     double *nu,		/* OUT: great circle latitude
					   (degrees) */
	     double *nuErr);		/* OUT: error in nu, or NULL */

int
atTransInverseApply(const TRANS *trans,	/* TRANS structure for the frame */
		    char filter,	/* filter in which row, and col are
					   measured */
		    double mu,		/* great circle longitude (degrees) */
		    double muErr,	/* error in mu */
		    double nu,		/* great circle latitude (degrees) */
		    double nuErr,	/* error in nu */
		    const float *mag,	/* magnitudes for this object, or NULL
					   (only the colours are used)*/
		    const float *magErr, /* error in mag, or NULL */
		    double *row,	/* OUT: frame row position (pixels) */
		    double *rowErr,	/* OUT: error in row, or NULL */
		    double *col,	/* OUT: frame column position (pixels)*/
		    double *colErr);	/* OUT: error in col, or NULL */

int
atTransInverseApplyNowrap(const TRANS *trans,	/* TRANS structure for the frame */
		    char filter,	/* filter in which row, and col are
					   measured */
		    double mu,		/* great circle longitude (degrees) */
		    double muErr,	/* error in mu */
		    double nu,		/* great circle latitude (degrees) */
		    double nuErr,	/* error in nu */
		    const float *mag,	/* magnitudes for this object, or NULL
					   (only the colours are used)*/
		    const float *magErr, /* error in mag, or NULL */
		    double *row,	/* OUT: frame row position (pixels) */
		    double *rowErr,	/* OUT: error in row, or NULL */
		    double *col,	/* OUT: frame column position (pixels)*/
		    double *colErr);	/* OUT: error in col, or NULL */

#ifdef __cplusplus
}
#endif

#endif
