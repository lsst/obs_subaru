/* <AUTO>
   FILE: atTrans
<HTML>
  TRANS structures are SDSS-specific coordinate transformations from frame
  coordinates (row,col) to great circle coordinates (mu, nu).
  The transformation is of the form:
<pre>
  r'-i' < riCut:

      rowm = row + dRow0 + dRow1*col + dRow2*(col^2) + dRow3*(col^3) + csRow*c
      colm = col + dCol0 + dCol1*col + dCol2*(col^2) + dCol3*(col^3) + csCol*c

  r'-i' >= riCut

      rowm = row + dRow0 + dRow1*col + dRow2*(col^2) + dRow3*(col^3) + ccRow
      colm = col + dCol0 + dCol1*col + dCol2*(col^2) + dCol3*(col^3) + ccCol

  mu = a + b * rowm + c * colm
  nu = d + e * rowm + f * colm
</pre>
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
<p>
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
</HTML>
   </AUTO> */
/*
   MODULE Transutil - Coordinate Transform Utilities

   DESCRIPTION:
     These routines manipulate TRANS objects:

       * atTransNew           - Create a new TRANS object
       * atTransDel           - Delete a TRANS object
       * atTransApply	      - Apply a TRANS, converting (row,col) to (mu,nu)
       * atTransInverseApply  - Apply the inverse of a TRANS, converting
			        (mu,nu) to (row,col)

 *--
*/

#include <stddef.h>
#include <math.h>
#include <string.h>
#include "atTrans.h"
#include "slalib.h"
#include "atConversions.h"

const char *atFilternames = "ugrizolts"; /* names of valid filters */
static float cosmicMag[AT_NBAND] = {	/* default cosmic magnitudes */
   3.05, 1.03, 0.33, 0.11, 0.00		/* for a K3V star from Gunn-Stryker */
};
static float cosmicMagScatter[AT_NBAND] = { /* scatter in cosmicMag */
   0.55, 0.35, 0.15, 0.12, 0.00		/* when added in quadrature, the error
					   in e.g. u-g is half the difference
					   in u-g between a K0V and M0V star */
};

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Set the TRANS routine's ideas of cosmic magnitudes, returning the old
 * values; if mag is NULL the current values will be unchanged
 */
const float *
atCosmicMagSet(const float *mag)
{
   int i;
   static float old[AT_NBAND];

   for(i = 0; i < AT_NBAND; i++) {
      old[i] = cosmicMag[i];
      if(mag != NULL) {
	 cosmicMag[i] = mag[i];
      }
   }

   return(old);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Set the TRANS routine's ideas of the scatter in cosmic magnitudes,
 * returning the old values; if mag is NULL the current values will be unchanged
 */
const float *
atCosmicMagScatterSet(const float *magScatter)
{
   int i;
   static float old[AT_NBAND];

   for(i = 0; i < AT_NBAND; i++) {
      old[i] = cosmicMagScatter[i];
      if(magScatter != NULL) {
	 cosmicMagScatter[i] = magScatter[i];
      }
   }

   return(old);
}

/*----------------------------------------------------------------------------*/
/*<AUTO EXTRACT>

  ROUTINE: atTransNew

  DESCRIPTION:
<HTML>
	Make a new TRANS struct.  The structure in initialized to give a
	unity transformation in the affine terms, with all higher-order
	distortion and color terms set to 0.
</HTML>

  RETURN VALUES:
<HTML>
        Returns a new TRANS struct.
</HTML>
</AUTO>*/
  TRANS *atTransNew(void)

    {  /* Function atTransNew */

      static int id = 0;
      TRANS *t_ptr;

/* Allocate a TRANS-sized block */

      t_ptr = (TRANS *)shMalloc(sizeof(TRANS));

/* Initialize the header */

      t_ptr->id = id++;

/* Initialize elements to a unity transformation */

      t_ptr->a = 0.0;
      t_ptr->b = 1.0;
      t_ptr->c = 0.0;
      t_ptr->d = 0.0;
      t_ptr->e = 0.0;
      t_ptr->f = 1.0;
      t_ptr->dRow0 = 0.0;
      t_ptr->dRow1 = 0.0;
      t_ptr->dRow2 = 0.0;
      t_ptr->dRow3 = 0.0;
      t_ptr->dCol0 = 0.0;
      t_ptr->dCol1 = 0.0;
      t_ptr->dCol2 = 0.0;
      t_ptr->dCol3 = 0.0;
      t_ptr->csRow = 0.0;
      t_ptr->csCol = 0.0;
      t_ptr->ccRow = 0.0;
      t_ptr->ccCol = 0.0;
      t_ptr->riCut = 100.0;
      t_ptr->mjd   = 0.0;
      t_ptr->airmass = 1.0;
      t_ptr->muErr = 0.0;
      t_ptr->nuErr = 0.0;

      return (t_ptr);

    }  /* Function atTransNew */

/*----------------------------------------------------------------------------*/

/*<AUTO EXTRACT>

  ROUTINE: atTransDel

  DESCRIPTION:
<HTML>
	Delete a TRANS struct
</HTML>

  RETURN VALUES:
<HTML>
        Returns SH_SUCCESS
</HTML>
</AUTO>*/
  RET_CODE atTransDel(TRANS *t_ptr)

    {  /* Function atTransDel */

    if (t_ptr != NULL)
      {
      shFree(t_ptr);
      }

    return SH_SUCCESS;

    }  /* Function atTransDel */

/*****************************************************************************/
/*
 * given a filtername (an element of atFilternames), return the index
 * to be used for the other band in calculating colours for astrometric
 * transformations using TRANSs. If it's redder, the caller will have
 * to switch 
 */
static void
get_filter_indices(const char filter,	/* name of filter, e.g. 'r' */
		   int *rfilter,	/* index of redder band */
		   int *bfilter)	/* index of bluer band */
{

   shAssert(strlen(atFilternames) == AT_NBAND + 4); /* olts are in AT_NBAND */
   shAssert(strchr(atFilternames, filter) != NULL);
   shAssert(strncmp(atFilternames, "ugriz", 5) == 0); /* I assume I know
							 bands' indices */

   switch (filter) {
    case 'u':
      *bfilter = 0; *rfilter = 1;	/* u-g */
      break;
    case 'g':
      *bfilter = 1; *rfilter = 2;	/* g-r */
      break;
    case 'l':
    case 'o':
    case 't':
    case 's':
    case 'r':
      *bfilter = 2; *rfilter = 3;	/* r-i */
      break;
    case 'i':
      *bfilter = 2; *rfilter = 3;	/* r-i */
      break;
    case 'z':
      *bfilter = 2; *rfilter = 3;	/* r-i */
      break;
    default:
      shFatal("You cannot get here");
      break;				/* NOTREACHED */
   }
}

/*----------------------------------------------------------------------------*/

/*
 * <AUTO EXTRACT>
 *
 * Apply a TRANS structure to convert CCD coordinates (row,col) in
 * pixels to great circle coordinates (mu,nu) in degrees.
 *
 * The colour of the object is specified by the AT_NBAND element
 * array mag (with associated error array magErr), and the filter
 * which must be one of u, g, r, i , or z. If the errors are too
 * large the colour will be taken from the cosmic values, which
 * may be set (or retrieved) using atCosmicMagSet and atCosmicMagScatterSet
 *
 * If mag is NULL, the cosmic values are used.
 *
 * Each TRANS structure has an element "riCut"; stars with r'-i'
 * colors redder than or equal to the value of "riCut" have their
 * differential chromatic refraction (DCR) set to a constant (i.e.,
 * independent of the color of the star), while stars whose r'-i'
 * color is bluer than "riCut" have their DCR calculated based on the
 * value of "color" (where the color used for "color" depends on the
 * TRANS structure, and can be r'-i').  If "riCut" in the TRANS
 * structure is greater than or equal to 10, then the r'-i' color is
 * ignored, and DCR is based solely on "color".  If the error on r'-i'
 * exceeds 0.5 mag (and "riCut" is less than 10), or if the error on
 * "color" exceeds 0.5 mag and r'-i' is less than "riCut" (or "riCut"
 * is greater than or equal to 10), then the cosmic color is used to
 * calculate DCR.  The cosmic color is always in the color system of
 * "color" (i.e., the cosmic color is always assumed to be for a star
 * whose r'-i' is less than "riCut").
 *
 * muErr and/or nuErr may be NULL, in which case no error is returned
 * for that quantity
 *
 * RETURN VALUES:
 *	0	calibrated using measured colors
 *	1	calibrated using the cosmic color
 */
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
	     double *nuErr)		/* OUT: error in nu, or NULL */
{
  double color=0;			/* appropriate color for this filter */
  double colorErr=0;			/* error in color */
  double cosmicColor, cosmicScatter;	/* cosmic colour and scatter for obj.*/
  double maxColorErr = 0.5;
  double dcrRow, dcrRowErr, dcrCol, dcrColErr, x, xErr2, y, yErr2;
  int rfilter, bfilter;			/* indices corresponding to
					   redder and bluer filters */
  double ri=0;				/* r'-i' color */
  double riErr;				/* error in r'-i' color */
  int status;				/* return value */

  get_filter_indices(filter, &rfilter, &bfilter);

  if(mag == NULL) {
     shAssert(magErr == NULL);
     mag = cosmicMag; magErr = cosmicMagScatter;
  }
  shAssert(mag != NULL && magErr != NULL);

  /* Check for negative mag errors, indicating bad measurement.  If so,
   * set colorErr large to force use of cosmic color. */
  if (magErr[2] < 0 || magErr[3] < 0)
    {
      riErr = 10 * maxColorErr;
    }
  else
    {
      ri = mag[2] - mag[3];
      riErr = sqrt(magErr[3]*magErr[3] + magErr[2]*magErr[2]);
    }
  if (magErr[bfilter] < 0 || magErr[rfilter] < 0)
    {
      colorErr = 10 * maxColorErr;
    }
  else
    {
      color = mag[bfilter] - mag[rfilter];
      colorErr = sqrt(magErr[bfilter]*magErr[bfilter] +
		      magErr[rfilter]*magErr[rfilter]);
    }
  cosmicColor = cosmicMag[bfilter] - cosmicMag[rfilter];
  cosmicScatter = sqrt(cosmicMagScatter[bfilter]*cosmicMagScatter[bfilter] +
		       cosmicMagScatter[rfilter]*cosmicMagScatter[rfilter]);

  /* Determine DCR correction */
  if (trans->riCut >= 10)
    {
      /* Ignore r'-i' */
      if (colorErr > maxColorErr)
	{
	  status = 1;
	  dcrRow = trans->csRow * cosmicColor;
	  dcrCol = trans->csCol * cosmicColor;
	  dcrRowErr = trans->csRow * cosmicScatter;
	  dcrColErr = trans->csCol * cosmicScatter;
	}
      else
	{
	  status = 0;
	  dcrRow = trans->csRow * color;
	  dcrCol = trans->csCol * color;
	  dcrRowErr = trans->csRow * colorErr;
	  dcrColErr = trans->csCol * colorErr;
	}
    }
  else if (riErr > maxColorErr)
    {
      /* r'-i' error too large; use cosmic color */
      status = 1;
      dcrRow = trans->csRow * cosmicColor;
      dcrCol = trans->csCol * cosmicColor;
      dcrRowErr = trans->csRow * cosmicScatter;
      dcrColErr = trans->csCol * cosmicScatter;
    }
  else if (ri >= trans->riCut)
    {
      /* Use constant DCR */
      status = 0;
      dcrRow = trans->ccRow;
      dcrCol = trans->ccCol;
      dcrRowErr = 0.;
      dcrColErr = 0.;
    }
  else if (colorErr > maxColorErr)
    {
      /* "color" error too large; use cosmic color */
      status = 1;
      dcrRow = trans->csRow * cosmicColor;
      dcrCol = trans->csCol * cosmicColor;
      dcrRowErr = trans->csRow * cosmicScatter;
      dcrColErr = trans->csCol * cosmicScatter;
    }
  else
    {
      /* Use "color" for DCR */
      status = 0;
      dcrRow = trans->csRow * color;
      dcrCol = trans->csCol * color;
      dcrRowErr = trans->csRow * colorErr;
      dcrColErr = trans->csCol * colorErr;
    }

  /* Calculate modified (x,y=row,col) coordinates, correcting for
   * higher-order optical distortion terms and DCR */
  x = row + trans->dRow0 + trans->dRow1 * col + trans->dRow2 * col * col +
    trans->dRow3 * col * col * col + dcrRow;
  y = col + trans->dCol0 + trans->dCol1 * col + trans->dCol2 * col * col +
    trans->dCol3 * col * col * col + dcrCol;
  xErr2 = rowErr * rowErr + colErr * colErr * (pow(trans->dRow1, 2) + 
    pow(2 * trans->dRow2 * col, 2) + pow(3 * trans->dRow3 * col * col, 2)) +
    dcrRowErr * dcrRowErr;
  yErr2 = colErr * colErr + colErr * colErr * (pow(trans->dCol1, 2) + 
    pow(2 * trans->dCol2 * col, 2) + pow(3 * trans->dCol3 * col * col, 2)) +
    dcrColErr * dcrColErr;

  /* Calculate great circle coordinates */
  *mu = trans->a + trans->b * x + trans->c * y;
  *nu = trans->d + trans->e * x + trans->f * y;
  if(muErr != NULL) {
     *muErr = sqrt(trans->b * trans->b * xErr2 + trans->c * trans->c * yErr2 +
		   trans->muErr * trans->muErr * at_asec2Deg * at_asec2Deg);
  }
  if(nuErr != NULL) {
     *nuErr = sqrt(trans->e * trans->e * xErr2 + trans->f * trans->f * yErr2 +
		   trans->nuErr * trans->nuErr * at_asec2Deg * at_asec2Deg);
  }

  /* Return the status code */
  return (status);
}

/*----------------------------------------------------------------------------*/

/*<AUTO EXTRACT>

  ROUTINE: atTransInverseApply

  DESCRIPTION:
<HTML>
	Apply the inverse of a TRANS structure to convert great circle
	coordinates (mu,nu) in degrees to CCD coordinates (row,col) in pixels.
	Each TRANS structure
	has an element "riCut"; stars with r'-i' colors redder than or equal to
	the value of "riCut" have their differential chromatic refraction (DCR)
	set to a constant (i.e., independent of the color of the star), while
	stars whose r'-i' color is bluer than "riCut" have their DCR
	calculated based on the	value of "color" (where the color used for
	"color" depends on the TRANS structure, and can be r'-i').  If "riCut"
	in the TRANS structure is greater than or equal to 10, then the r'-i'
	color is ignored, and DCR is based solely on "color".  If the error on
	r'-i' exceeds 0.5 mag (and "riCut" is less than 10), or if the error
	on "color" exceeds 0.5 mag and r'-i' is less than "riCut" (or "riCut"
	is greater than or equal to 10), then the cosmic color is used to
	calculate DCR.  The cosmic color is always in the color system of
	"color" (i.e., the cosmic color is always assumed to be for a star
	whose r'-i' is less than "riCut").
</HTML>
  RETURN VALUES:
  <HTML>
  <pre>
       -1       error inverting the TRANS
	0	calibrated using measured colors
	1	calibrated using the cosmic color
  </pre>
  </HTML>
</AUTO>*/
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
		    double *colErr)	/* OUT: error in col, or NULL */
{
  double color=0;			/* appropriate color for this filter */
  double colorErr=0;			/* error in color */
  double cosmicColor, cosmicScatter;	/* cosmic colour and scatter for obj.*/
  const double maxColorErr = 0.5;
  double dcrRow, dcrRowErr, dcrCol, dcrColErr, x, xErr2, y, yErr2, inv[6];
  int rfilter, bfilter;			/* indices corresponding to
					   redder and bluer filters */
  double ri=0;				/* r'-i' color */
  double riErr;				/* error in r'-i' color */
  int status, jflag, nIterations, i;
  double dy;

  get_filter_indices(filter, &rfilter, &bfilter);

  if(mag == NULL) {
     shAssert(magErr == NULL);
     mag = cosmicMag; magErr = cosmicMagScatter;
  }
  shAssert(mag != NULL && magErr != NULL);

  /* Check for negative mag errors, indicating bad measurement.  If so,
   * set colorErr large to force use of cosmic color. */
  if (magErr[2] < 0 || magErr[3] < 0)
    {
      riErr = 10 * maxColorErr;
    }
  else
    {
      ri = mag[2] - mag[3];
      riErr = sqrt(magErr[3]*magErr[3] + magErr[2]*magErr[2]);
    }
  if (magErr[bfilter] < 0 || magErr[rfilter] < 0)
    {
      colorErr = 10 * maxColorErr;
    }
  else
    {
      color = mag[bfilter] - mag[rfilter];
      colorErr = sqrt(magErr[bfilter]*magErr[bfilter] +
		      magErr[rfilter]*magErr[rfilter]);
    }
  cosmicColor = cosmicMag[bfilter] - cosmicMag[rfilter];
  cosmicScatter = sqrt(cosmicMagScatter[bfilter]*cosmicMagScatter[bfilter] +
		       cosmicMagScatter[rfilter]*cosmicMagScatter[rfilter]);

  /*
   * Invert the affine portion of the TRANS structure
   *
   * Note that we cannot simply treat &(trans->a) as an array of 6 doubles;
   * it is probably true on all current compilers, but need not be --- and the
   * TRANS structure could be changed in the future
   */
  {
     double tr[6];
     tr[0] = trans->a;
     tr[1] = trans->b;
     tr[2] = trans->c;
     tr[3] = trans->d;
     tr[4] = trans->e;
     tr[5] = trans->f;
     slaInvf(tr, inv, &jflag);
  }
  if (jflag != 0) return (-1);

  /* Determine DCR correction */
  if (trans->riCut >= 10)
    {
      /* Ignore r'-i' */
      if (colorErr > maxColorErr)
	{
	  status = 1;
	  dcrRow = trans->csRow * cosmicColor;
	  dcrCol = trans->csCol * cosmicColor;
	  dcrRowErr = trans->csRow * cosmicScatter;
	  dcrColErr = trans->csCol * cosmicScatter;
	}
      else
	{
	  status = 0;
	  dcrRow = trans->csRow * color;
	  dcrCol = trans->csCol * color;
	  dcrRowErr = trans->csRow * colorErr;
	  dcrColErr = trans->csCol * colorErr;
	}
    }
  else if (riErr > maxColorErr)
    {
      /* r'-i' error too large; use cosmic color */
      status = 1;
      dcrRow = trans->csRow * cosmicColor;
      dcrCol = trans->csCol * cosmicColor;
      dcrRowErr = trans->csRow * cosmicScatter;
      dcrColErr = trans->csCol * cosmicScatter;
    }
  else if (ri >= trans->riCut)
    {
      /* Use constant DCR */
      status = 0;
      dcrRow = trans->ccRow;
      dcrCol = trans->ccCol;
      dcrRowErr = 0.;
      dcrColErr = 0.;
    }
  else if (colorErr > maxColorErr)
    {
      /* "color" error too large; use cosmic color */
      status = 1;
      dcrRow = trans->csRow * cosmicColor;
      dcrCol = trans->csCol * cosmicColor;
      dcrRowErr = trans->csRow * cosmicScatter;
      dcrColErr = trans->csCol * cosmicScatter;
    }
  else
    {
      /* Use "color" for DCR */
      status = 0;
      dcrRow = trans->csRow * color;
      dcrCol = trans->csCol * color;
      dcrRowErr = trans->csRow * colorErr;
      dcrColErr = trans->csCol * colorErr;
    }

  /* Treat periodic boundary conditions */
  if(trans->a - mu >= 180.0)
    mu += 360.0;
  else if(trans->a - mu < -180.0)
    mu -= 360.0;

  /* Calculate corrected pixel positions */
  x = inv[0] + inv[1] * mu + inv[2] * nu;
  y = inv[3] + inv[4] * mu + inv[5] * nu;
  xErr2 = inv[1] * inv[1] * muErr * muErr + inv[2] * inv[2] * nuErr * nuErr;
  yErr2 = inv[4] * inv[4] * muErr * muErr + inv[5] * inv[5] * nuErr * nuErr;

  /* Calculate raw (row,col) coordinates, removing corrections for
   * higher-order optical distortion terms and DCR.
   * Iterate a few times on the column to improve the estimate of the higher
   * order distortion terms.  However, test to make sure the correction
   * is smaller than 2 pixels.
   */
  nIterations = 10;
  *col = y;
  for (i = 0; i < nIterations; i++) {
    dy = trans->dCol0 + trans->dCol1 * *col + 
		 trans->dCol2 * *col * *col +
		 trans->dCol3 * *col * *col * *col + dcrCol;
    if(dy > 2.) break;
    *col = y - dy;
  }
  *row = x - (trans->dRow0 + trans->dRow1 * *col + trans->dRow2 * *col * *col +
    trans->dRow3 * *col * *col * *col + dcrRow);
  if(rowErr != NULL) {
     *rowErr = sqrt(xErr2 + yErr2 * (pow(trans->dRow1, 2) + 
    pow(2 * trans->dRow2 * *col, 2) + pow(3 * trans->dRow3 * *col * *col, 2)) +
    dcrRowErr * dcrRowErr);
     *rowErr = sqrt(xErr2 + yErr2 * (pow(trans->dRow1, 2) + 
    pow(2 * trans->dRow2 * *col, 2) + pow(3 * trans->dRow3 * *col * *col, 2)) +
    dcrRowErr * dcrRowErr + trans->muErr * trans->muErr 
    * at_asec2Deg * at_asec2Deg * inv[1] * inv[1]);
  }
  if(colErr != NULL) {
     *colErr = sqrt(yErr2 + yErr2 * (pow(trans->dCol1, 2) + 
    pow(2 * trans->dCol2 * *col, 2) + pow(3 * trans->dCol3 * *col * *col, 2)) +
    dcrColErr * dcrColErr + trans->nuErr * trans->nuErr
    * at_asec2Deg * at_asec2Deg * inv[5] * inv[5]);
  }

  /* Return the status code */
  return (status);
}
/*----------------------------------------------------------------------------*/

/*<AUTO EXTRACT>

  ROUTINE: atTransInverseApplyNowrap

  DESCRIPTION:
<HTML>
	Apply the inverse of a TRANS structure to convert great circle
	coordinates (mu,nu) in degrees to CCD coordinates (row,col) in pixels.
	Do not apply +/-180 wrapping on mu.
	Each TRANS structure
	has an element "riCut"; stars with r'-i' colors redder than or equal to
	the value of "riCut" have their differential chromatic refraction (DCR)
	set to a constant (i.e., independent of the color of the star), while
	stars whose r'-i' color is bluer than "riCut" have their DCR
	calculated based on the	value of "color" (where the color used for
	"color" depends on the TRANS structure, and can be r'-i').  If "riCut"
	in the TRANS structure is greater than or equal to 10, then the r'-i'
	color is ignored, and DCR is based solely on "color".  If the error on
	r'-i' exceeds 0.5 mag (and "riCut" is less than 10), or if the error
	on "color" exceeds 0.5 mag and r'-i' is less than "riCut" (or "riCut"
	is greater than or equal to 10), then the cosmic color is used to
	calculate DCR.  The cosmic color is always in the color system of
	"color" (i.e., the cosmic color is always assumed to be for a star
	whose r'-i' is less than "riCut").
</HTML>
  RETURN VALUES:
  <HTML>
  <pre>
       -1       error inverting the TRANS
	0	calibrated using measured colors
	1	calibrated using the cosmic color
  </pre>
  </HTML>
</AUTO>*/
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
		    double *colErr)	/* OUT: error in col, or NULL */
{
  double color=0;		/* appropriate color for this filter */
  double colorErr;			/* error in color */
  double cosmicColor, cosmicScatter;	/* cosmic colour and scatter for obj.*/
  const double maxColorErr = 0.5;
  double dcrRow, dcrRowErr, dcrCol, dcrColErr, x, xErr2, y, yErr2, inv[6];
  int rfilter, bfilter;			/* indices corresponding to
					   redder and bluer filters */
  double ri=0;				/* r'-i' color */
  double riErr;				/* error in r'-i' color */
  int status, jflag, nIterations, i;

  get_filter_indices(filter, &rfilter, &bfilter);

  if(mag == NULL) {
     shAssert(magErr == NULL);
     mag = cosmicMag; magErr = cosmicMagScatter;
  }
  shAssert(mag != NULL && magErr != NULL);

  /* Check for negative mag errors, indicating bad measurement.  If so,
   * set colorErr large to force use of cosmic color. */
  if (magErr[2] < 0 || magErr[3] < 0)
    {
      riErr = 10 * maxColorErr;
    }
  else
    {
      ri = mag[2] - mag[3];
      riErr = sqrt(magErr[3]*magErr[3] + magErr[2]*magErr[2]);
    }
  if (magErr[bfilter] < 0 || magErr[rfilter] < 0)
    {
      colorErr = 10 * maxColorErr;
    }
  else
    {
      color = mag[bfilter] - mag[rfilter];
      colorErr = sqrt(magErr[bfilter]*magErr[bfilter] +
		      magErr[rfilter]*magErr[rfilter]);
    }
  cosmicColor = cosmicMag[bfilter] - cosmicMag[rfilter];
  cosmicScatter = sqrt(cosmicMagScatter[bfilter]*cosmicMagScatter[bfilter] +
		       cosmicMagScatter[rfilter]*cosmicMagScatter[rfilter]);

  /*
   * Invert the affine portion of the TRANS structure
   *
   * Note that we cannot simply treat &(trans->a) as an array of 6 doubles;
   * it is probably true on all current compilers, but need not be --- and the
   * TRANS structure could be changed in the future
   */
  {
     double tr[6];
     tr[0] = trans->a;
     tr[1] = trans->b;
     tr[2] = trans->c;
     tr[3] = trans->d;
     tr[4] = trans->e;
     tr[5] = trans->f;
     slaInvf(tr, inv, &jflag);
  }
  if (jflag != 0) return (-1);

  /* Determine DCR correction */
  if (trans->riCut >= 10)
    {
      /* Ignore r'-i' */
      if (colorErr > maxColorErr)
	{
	  status = 1;
	  dcrRow = trans->csRow * cosmicColor;
	  dcrCol = trans->csCol * cosmicColor;
	  dcrRowErr = trans->csRow * cosmicScatter;
	  dcrColErr = trans->csCol * cosmicScatter;
	}
      else
	{
	  status = 0;
	  dcrRow = trans->csRow * color;
	  dcrCol = trans->csCol * color;
	  dcrRowErr = trans->csRow * colorErr;
	  dcrColErr = trans->csCol * colorErr;
	}
    }
  else if (riErr > maxColorErr)
    {
      /* r'-i' error too large; use cosmic color */
      status = 1;
      dcrRow = trans->csRow * cosmicColor;
      dcrCol = trans->csCol * cosmicColor;
      dcrRowErr = trans->csRow * cosmicScatter;
      dcrColErr = trans->csCol * cosmicScatter;
    }
  else if (ri >= trans->riCut)
    {
      /* Use constant DCR */
      status = 0;
      dcrRow = trans->ccRow;
      dcrCol = trans->ccCol;
      dcrRowErr = 0.;
      dcrColErr = 0.;
    }
  else if (colorErr > maxColorErr)
    {
      /* "color" error too large; use cosmic color */
      status = 1;
      dcrRow = trans->csRow * cosmicColor;
      dcrCol = trans->csCol * cosmicColor;
      dcrRowErr = trans->csRow * cosmicScatter;
      dcrColErr = trans->csCol * cosmicScatter;
    }
  else
    {
      /* Use "color" for DCR */
      status = 0;
      dcrRow = trans->csRow * color;
      dcrCol = trans->csCol * color;
      dcrRowErr = trans->csRow * colorErr;
      dcrColErr = trans->csCol * colorErr;
    }

  /* Treat periodic boundary conditions */
  /*
  if(trans->a - mu >= 180.0)
    mu += 360.0;
  else if(trans->a - mu < -180.0)
    mu -= 360.0;
    */

  /* Calculate corrected pixel positions */
  x = inv[0] + inv[1] * mu + inv[2] * nu;
  y = inv[3] + inv[4] * mu + inv[5] * nu;
  xErr2 = inv[1] * inv[1] * muErr * muErr + inv[2] * inv[2] * nuErr * nuErr;
  yErr2 = inv[4] * inv[4] * muErr * muErr + inv[5] * inv[5] * nuErr * nuErr;

  /* Calculate raw (row,col) coordinates, removing corrections for
   * higher-order optical distortion terms and DCR.
   * Iterate a few times on the column to improve the estimate of the higher
   * order distortion terms.
   */
  nIterations = 10;
  *col = y;
  for (i = 0; i < nIterations; i++) {
     *col = y - (trans->dCol0 + trans->dCol1 * *col + 
		 trans->dCol2 * *col * *col +
		 trans->dCol3 * *col * *col * *col + dcrCol);
  }
  *row = x - (trans->dRow0 + trans->dRow1 * *col + trans->dRow2 * *col * *col +
    trans->dRow3 * *col * *col * *col + dcrRow);
  if(rowErr != NULL) {
     *rowErr = sqrt(xErr2 + yErr2 * (pow(trans->dRow1, 2) + 
    pow(2 * trans->dRow2 * *col, 2) + pow(3 * trans->dRow3 * *col * *col, 2)) +
    dcrRowErr * dcrRowErr + trans->muErr * trans->muErr
    * at_asec2Deg * at_asec2Deg * inv[1] * inv[1]);
  }
  if(colErr != NULL) {
     *colErr = sqrt(yErr2 + yErr2 * (pow(trans->dCol1, 2) + 
    pow(2 * trans->dCol2 * *col, 2) + pow(3 * trans->dCol3 * *col * *col, 2)) +
    dcrColErr * dcrColErr + trans->nuErr * trans->nuErr
    * at_asec2Deg * at_asec2Deg * inv[5] * inv[5]);
  }

  /* Return the status code */
  return (status);
}


