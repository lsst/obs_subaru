#include <stdlib.h>
#include <math.h>

/*
 * Do we have a Gaussian deviate squirreled away?
 */
static int phGaussdev_iset = 0;

/*****************************************************************************/
/*
 * return a variate in [0,1).
 */
float
phRandomUniformdev(void)
{
	return drand48();
}

/*****************************************************************************/
/*
 * return an N(0,1) variate.  Note that this saves an `extra' random
 * number internally, and uses it if phGaussdev_iset is true
 */
float
phGaussdev(void)
{
   static float gset;			/* saved Gaussian deviate */
   float fac,r = 0,v1,v2;		/* initialise r to appease
					   the IRIX 5.3 cc */

   if(phGaussdev_iset) {
      phGaussdev_iset = 0;
      return(gset);
   } else {
      do {
		  // dstn
		  v1 = phRandomUniformdev() * 2 - 1;
		  v2 = phRandomUniformdev() * 2 - 1;
	 r = v1*v1+v2*v2;
      } while (r >= 1.0);
      fac = sqrt(-2.0*log(r)/r);
      gset = v1*fac;
      phGaussdev_iset = 1;

      return(v2*fac);
   }
}

