/* <AUTO>
  FILE: atGalaxy
<HTML>
  Routines which calculate quantities in our galaxy
</HTML>
  </AUTO>
*/

#include <math.h>
#include "dervish.h"
#include "atGalaxy.h"
#include "atConversions.h"

/*<AUTO EXTRACT>

  ROUTINE: atStarCountsFind

  DESCRIPTION:
<HTML>
This program calculates the number of stars per square degree
in V from an approximation to the Bahcall-Soneira model
that is good to fifteen percent.  Do not use this below 20 degrees
galactic latitude.
</HTML>
</AUTO>*/
void atStarCountsFind(double degl,/* galactic longitude, in degrees */
		double degb, 	/* galactic latitude, in degrees */
		double m,	/* limiting magnitude in V */
		double *density	/* returned stellar density, in stars/sq. deg.*/
		) {

  double l,b; /* galactic longitude and latitude in radians */
  double C1,C2,alpha,kappa,beta,eta,delta,lambda,mstar,mdagger;
  double mu,gamma,sigma,temp1,temp2,temp3,temp4,temp5,temp6;

  shAssert(degb>=20 && degb<=90);

  l = degl*at_deg2Rad;
  b = fabs(degb*at_deg2Rad);

  C1 = 925.;
  C2 = 1050.;
  alpha = -0.132;
  kappa = -0.180;
  beta = 0.035;
  eta = 0.087;
  delta = 3.0;
  lambda = 2.5;
  mstar = 15.75;
  mdagger = 17.5;

/* These would calculate stars/sq.deg./mag
  C1 = 200.;
  C2 = 400.;
  alpha = -0.2;
  kappa = -0.26;
  beta = 0.01;
  eta = 0.065;
  delta = 2.0;
  lambda = 1.5;
  mstar = 15.;
  mdagger = 17.5;
*/

  if (m<=12) {
    mu = 0.03;
    gamma = 0.36;
  } else if (m>=20) {
    mu = 0.09;
    gamma = 0.04;
  } else {
    mu = 0.0075*(m-12.)+0.03;
    gamma = 0.04*(12.-m)+0.36;
  }
  sigma = 1.45-0.20*cos(b)*cos(l);

  temp1 = C1*pow(10.0,(beta*(m-mstar)));
  temp2 = pow(1+pow(10.0,(alpha*(m-mstar))),delta);
  temp3 = pow(sin(b)*(1-mu*cos(b)*cos(l)/sin(b)),(3.0-5*gamma));
  temp4 = C2*pow(10.0,(eta*(m-mdagger)));
  temp5 = pow(1+pow(10.0,(kappa*(m-mdagger))),lambda);
  temp6 = pow(1-cos(b)*cos(l),sigma);

  *density = (temp1/temp2)*(1/temp3) + (temp4/temp5)*(1/temp6);
}
