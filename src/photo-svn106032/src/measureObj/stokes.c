/*
 * Use coefficients calculated by make_photom_coeffs to calculate the
 * contributions of cells to Q and U
 */
#include <math.h>
#include "dervish.h"
#include "alloca.h"
#include "phExtract.h"
#include "phMeasureObj.h"

/*
 * here are the coefficients
 */
#include "stokes_coeffs.c"

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Evaluate the Stokes coefficient Q or U in a given annulus for the given
 * region. The object is assumed to be at the centre of the specified pixel,
 * so you may have to sinc shift first
 *
 * If normalise is false, don't actually calculate Q or U, but rather return
 * its numerator; it is assumed that the caller will divide by the total
 * intensity in the annulus. Note that you often already know the annular
 * flux, or need it for both U and Q, so you probably don't usually want to
 * have normalise true
 *
 * Return 0 if all is well, else -1.
 */
int
phStokesParamEval(const REGION *reg,	/* input data */
		  int ann,		/* the desired annulus */
		  int bkgd,		/* the REGION's background level */
		  int rowc, int colc,	/* central pixel of object */
		  int which,		/* 'Q' or 'U' */
		  int normalise,	/* if true, normalise the answer */
		  float *val)		/* the answer */
{
   COEFF *coeffs = NULL;
   int ret;				/* return code */
   
   switch(which) {
    case 'Q':
      if(ann < 0 || ann >= sizeof(Qcoeffs)/sizeof(Qcoeffs[0])) {
	 *val = 0;
	 return(-1);
      }
      coeffs = Qcoeffs;
      break;
    case 'U':
      if(ann < 0 || ann >= sizeof(Ucoeffs)/sizeof(Ucoeffs[0])) {
	 *val = 0;
	 return(-1);
      }
      coeffs = Ucoeffs;
      break;
    default:
      shFatal("phStokesParamEval: Illegal parameter %d (%c)",which,which);
   }
/*
 * do the work
 */
   if((ret = phSincApertureEvalFromCoeffs(reg, &coeffs[ann],
						bkgd, rowc, colc, val)) != 0) {
      return(ret);
   }
   if(normalise) {
      int c;				/* cell counter */
      float flux;			/* total flux in annulus */

      if(ann == 0) {			/* central disk */
	 ret = phSincApertureEval(reg,0,bkgd,rowc,colc,&flux);
	 shAssert(ret == 0);		/* we know that it succeeded for Q/U */
      } else {
	 float cell_flux;		/* flux in a cell */
	 
	 flux = 0;
	 for(c = ann*NSEC + 1;c <= (ann + 1)*NSEC;c++) {
	    ret = phSincApertureEval(reg,c,bkgd,rowc,colc,&cell_flux);
	    shAssert(ret == 0);		/* we know that it succeeded for Q/U */
	    flux += cell_flux;
	 }
      }
      if(flux != 0.0) {
	 *val /= flux;
      }
   }
   
   return(0);
}
