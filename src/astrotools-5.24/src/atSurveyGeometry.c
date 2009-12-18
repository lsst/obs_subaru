/* <AUTO>
   FILE: atSurveyGeometry
<HTML>
   Routines which convert to and from survey-specific coodinate systems
   <p>
   There are five coordinate systems:
   <ul>
   <li> Equatorial -- ra, dec
   <li> Galactic -- galactic longitude, galactic latitude
   <li> Great Circle -- mu, nu (with node and inclination also specified)
   <li> Survey -- survey longitude, survey latitude (lambda, eta)
   <li> Azelpa -- azimuth, elevation, and position angle of the telescope
   </ul>
   The functions here convert Equatorial coordinates to and from 
   Galactic, Great Circle, and Survey coordinates; also between Survey
   and Azelpa.
   <P>
   Survey coordinates are (lambda, eta).  Lines of constant lambda are
   parallels, and lines of constant eta are meridians which go through
   the survey pole.  The center of a great circle scan will be a line
   of constant eta.
   <P>
   Great circle coordinates (mu, nu) are defined so that the line down the
   center of a stripe (which is a meridian in survey coordinates) is the
   parallel nu=0.  So, lines of constant mu are meridians and lines of
   constant nu are parallels.  Great circle coordinates are specific to a
   survey stripe.
   <p>
   To convert to and from Great Circle coordinates, you must input the
   node and inclination of the reference great circle.  For "normal" drift
   scan great circles, use <code>node=at_surveyCenterRa - 90</code>degrees,
   and <code>inc=survey latitude + at_surveyCenterDec</code>.
   <p>
   The survey latitudes for SDSS stripes are <code> +/- n*at_stripeSeparation
   </code>.
   <p>
   The limits on these coordinates are:
   <ul>
   <li> 0 <= (ra, glong, mu) < 360.0
   <li> -180.0 <= lambda < 180.0
   <li> -90 <= (dec, glat, nu, eta) < 90.0
   </ul>
   <p>
   The survey center is defined with the external const double values
   <ul>
   <li><code> at_surveyCenterRa = 185.0</code>
   <li><code> at_surveyCenterDec = 32.5</code>
   </ul>
   This (ra,dec) transforms to:
   <ul>
   <li><bf>galactic</bf> gLong=172.79876542 gLat=81.32406952
   <li><bf>great circle</bf>(with node=95.0, inclination=32.5) mu=185.0 nu=0.0
   <li><bf>survey</bf> lambda=0.0 eta=0.0
   </ul>
</HTML>
   </AUTO>
*/

#include <math.h>
#include <string.h>
#include "atSlalib.h"
#include "atConversions.h"
#include "atSurveyGeometry.h"
#include "slalib.h"

/*<AUTO EXTRACT>

  ROUTINE: atEqToGC

  DESCRIPTION:
<HTML>
        Converts Equatorial coordinates to Great Circle coordinates.
</HTML>

  RETURN VALUES:
<HTML>
  void
</HTML>
</AUTO>*/
void atEqToGC (
	       double ra,	/* IN -- ra in degrees*/
	       double dec,	/* IN -- dec in degrees*/
	       double *amu,	/* OUT -- mu in degrees*/
	       double *anu,	/* OUT -- nu in degrees*/
	       double anode,	/* IN -- node in degrees*/
	       double ainc	/* IN -- inclination in degrees*/
	       ) {
  double x1, y1, z1, x2, y2, z2;
  /* Convert to radians */
  ra = ra * at_deg2Rad;
  dec = dec * at_deg2Rad;
  anode = anode * at_deg2Rad;
  ainc = ainc * at_deg2Rad;
  /* Rotation */
  x1 = cos(ra-anode)*cos(dec);
  y1 = sin(ra-anode)*cos(dec);
  z1 = sin(dec);
  x2 = x1;
  y2 = y1*cos(ainc) + z1*sin(ainc);
  z2 =-y1*sin(ainc) + z1*cos(ainc);

  *amu = atan2 (y2, x2)+anode;
  *anu = asin(z2);
  /* Convert back to degrees */
  *amu = *amu * at_rad2Deg;
  *anu = *anu * at_rad2Deg;
  atBound2(anu, amu);
  return;
}

/*<AUTO EXTRACT>

  ROUTINE: atGCToEq

  DESCRIPTION:
<HTML>
        Converts Great Circle to Equatorial coordinates.
</HTML>

  RETURN VALUES:
<HTML>
  void
</HTML>
</AUTO>*/

void atGCToEq (
	       double amu,	/* IN -- mu in degrees */
	       double anu,	/* IN -- nu in degrees */
	       double *ra,	/* OUT -- ra in degrees */
	       double *dec,	/* OUT -- dec in degrees */
	       double anode,	/* IN -- node in degrees */
	       double ainc	/* IN -- inclination in degrees */
	       )
{
  double x1, y1, z1, x2, y2, z2;
  /* Convert to radians */

  amu = amu * at_deg2Rad;
  anu = anu * at_deg2Rad;
  anode = anode * at_deg2Rad;
  ainc = ainc * at_deg2Rad;
  /* Rotation */
  x2 = cos(amu-anode)*cos(anu);
  y2 = sin(amu-anode)*cos(anu);
  z2 = sin(anu);
  x1 = x2;
  y1 = y2*cos(ainc) - z2*sin(ainc);
  z1 = y2*sin(ainc) + z2*cos(ainc);

  *ra = atan2 (y1, x1)+anode;
  *dec = asin(z1);
  /* Convert back to degrees */
  *ra = *ra * at_rad2Deg;
  *dec = *dec * at_rad2Deg;
  atBound2(dec, ra);
  return;
}

/*<AUTO EXTRACT>

  ROUTINE: atEqToGal

  DESCRIPTION:
<HTML>
        Converts Equatorial to Galactic coordinates.
</HTML>

  RETURN VALUES:
<HTML>
  void
</HTML>
</AUTO>*/
void atEqToGal (
		double ra,	/* IN -- ra in degrees */
		double dec,	/* IN -- dec in degrees */
		double *glong,	/* OUT -- Galactic longitude in degrees */
		double *glat	/* OUT -- Galactic latitude in degrees */
		)
{
  /* Convert to radians */
   ra = ra * at_deg2Rad;
   dec = dec * at_deg2Rad;
  /* Use SLALIB to do the actual conversion */
  slaEqgal(ra, dec, glong, glat);
  /* Convert back to degrees */
  *glong = *glong * at_rad2Deg;
  *glat = *glat * at_rad2Deg;
  atBound2(glat, glong);
  return;
}


/*<AUTO EXTRACT>

  ROUTINE: atGalToEq

  DESCRIPTION:
<HTML>
        Converts Galactic to Equatorial coordinates.
</HTML>

  RETURN VALUES:
<HTML>
  void
</HTML>
</AUTO>*/

void atGalToEq (
		double glong,	/* IN -- Galactic longitude in degrees*/
		double glat,	/* IN -- Galactic latitude in degrees*/
		double *ra,	/* OUT -- ra in degrees*/
		double *dec	/* OUT -- dec in degrees*/
		)
{
  /* Convert to radians */
  glong = glong * at_deg2Rad;
  glat = glat * at_deg2Rad;
  /* Use SLALIB to do the actual conversion */
  slaGaleq(glong, glat, ra, dec);
  /* Convert back to deggrees */
  *ra = *ra * at_rad2Deg;
  *dec = *dec * at_rad2Deg;
  atBound2(dec, ra);
  return;
}

/*<AUTO EXTRACT>

  ROUTINE: atEqToSurvey

  DESCRIPTION:
<HTML>
        Converts Equatorial to Survey coordinates.
</HTML>

  RETURN VALUES:
<HTML>
  void
</HTML>
</AUTO>*/
void atEqToSurvey (
		   double ra,	   /* IN -- ra in degrees*/
		   double dec,	   /* IN -- dec in degrees*/
		   double *slong,  /* OUT -- survey longitude in degrees */
		   double *slat   /* OUT -- survey latitude in degrees */
		   )
{
  double x1, y1, z1;
  double anode, etaPole;
  /* Convert to radians */
  ra = ra * at_deg2Rad;
  dec = dec * at_deg2Rad;
  anode = at_surveyCenterRa - 90.;
  anode = anode * at_deg2Rad;
  etaPole = at_surveyCenterDec * at_deg2Rad;
  /* Rotation */
  x1 = cos(ra-anode)*cos(dec);
  y1 = sin(ra-anode)*cos(dec);
  z1 = sin(dec);
  *slong = -asin(x1);
  *slat = atan2(z1, y1) - etaPole;
  /* Convert to degrees */
  *slong = *slong * at_rad2Deg;
  *slat = *slat * at_rad2Deg;
  atBound2(slong, slat);
  atBound(slat, -180.0, 180.0);

	if (*slat > 90-at_surveyCenterDec) {
		*slat -= 180;
		*slong = 180 - *slong;
	}


	// Added in response to PR 4668
	if (*slat < -90) {
	  *slat += 180;
	  *slong = 180 - *slong;
	  
	}
	

  atBound(slong, -180.0, 180.0);
  
  return;
}

/*<AUTO EXTRACT>

  ROUTINE: atSurveyToEq

  DESCRIPTION:
<HTML>
        Converts Survey to Equatorial coordinates.
</HTML>

  RETURN VALUES:
<HTML>
  void
</HTML>
</AUTO>*/
void atSurveyToEq (
		   double slong,	/* IN -- survey longitude in degrees */
		   double slat,		/* IN -- survey latitude in degrees */
		   double *ra,		/* OUT -- ra in degrees */
		   double *dec)		/* OUT -- dec in degrees */
{
  double x1, y1, z1;
  double anode, etaPole;
  /* Convert to radians */
  slong = slong * at_deg2Rad;
  slat = slat * at_deg2Rad;
  anode = at_surveyCenterRa - 90.;
  anode = anode * at_deg2Rad;
  etaPole = at_surveyCenterDec * at_deg2Rad;
  /* Rotation */
  x1 = -sin(slong);
  y1 = cos(slat+etaPole) * cos(slong);
  z1 = sin(slat+etaPole) * cos(slong);
  *ra = atan2 (y1, x1)+anode;
  *dec = asin(z1);
  *ra = *ra * at_rad2Deg;
  *dec = *dec * at_rad2Deg;
  atBound2(dec, ra);
  return;
}

/*<AUTO EXTRACT>

  ROUTINE: atGCToSurvey

  DESCRIPTION:
  <HTML>
    Converts Great Circle to Survey Coordinates
  </HTML>

  RETURN VALUES:
  <HTML>
     void
  </HTML>
*/

/*
 * This does nothing fancy, the code from atGCToEq and atEqToSurvey
 * has been glued together to convert GC to Survey coordinates
 */


void atGCToSurvey (
                    double amu,      
                    double anu,     
                    double anode,  
                    double ainc,  
                    double *slong,  
                    double *slat   
                  )

{
double x1, y1, z1, x2, y2, z2, etaPole;
double ra, dec;
double *raPtr=NULL, *decPtr=NULL;

/* from atGCToEq, convert GC to (ra, dec) */

/* Convert to radians */
  amu = amu * at_deg2Rad;
  anu = anu * at_deg2Rad;
  anode = anode * at_deg2Rad;
  ainc = ainc * at_deg2Rad;

/* Rotation */
  x2 = cos(amu-anode)*cos(anu);
  y2 = sin(amu-anode)*cos(anu);
  z2 = sin(anu);
  x1 = x2;
  y1 = y2*cos(ainc) - z2*sin(ainc);
  z1 = y2*sin(ainc) + z2*cos(ainc);

  ra = atan2 (y1, x1)+anode;
  dec = asin(z1);

/* Convert back to degrees and check bounds */
  ra = ra * at_rad2Deg;
  dec = dec * at_rad2Deg;
  raPtr = &ra;
  decPtr = &dec;
  atBound2(decPtr, raPtr);

/* from atEqToSurvey convert (ra, dec) to Survey */

/* Convert back to radians */
  ra = ra * at_deg2Rad;
  dec = dec * at_deg2Rad;
  anode = at_surveyCenterRa - 90.;
  anode = anode * at_deg2Rad;
  etaPole = at_surveyCenterDec * at_deg2Rad;

/* Rotation */
  x1 = cos(ra-anode)*cos(dec);
  y1 = sin(ra-anode)*cos(dec);
  z1 = sin(dec);
  *slong = -asin(x1);
  *slat = atan2(z1, y1) - etaPole;

/* Convert to degrees */
  *slong = *slong * at_rad2Deg;
  *slat = *slat * at_rad2Deg;
  atBound2(slong, slat);
  atBound(slat, -180.0, 180.0);
	if (*slat > 90-at_surveyCenterDec) {
		*slat -= 180;
		*slong = 180 - *slong;
	}
  atBound(slong, -180.0, 180.0);

  return;
} 

/*<AUTO EXTRACT>

  ROUTINE: atSurveyToGC

  DESCRIPTION:
<HTML>
      Converts Survey to Great Circle coordinates
 </HTML>

  RETURN VALUES:
<HTML>
      void 
</HTML>
*/

/* This does nothing fancy, atSurveyToEq and atEqToGC have been glued together */

void atSurveyToGC (
                    double slong,
                    double slat,
                    double anode,
                    double ainc,
                    double *amu,
                    double *anu
                  )
{

  double x1=0, y1=0, z1=0, x2=0, y2=0, z2=0; 
  double anodeSurvey=0, etaPole=0;
  double ra, dec;
  double *raPtr=NULL, *decPtr=NULL;

/* from atSurveyToEq */

  /* Convert to radians */
  slong = slong * at_deg2Rad;
  slat = slat * at_deg2Rad;
  anodeSurvey = at_surveyCenterRa - 90.0;
  anodeSurvey = anodeSurvey * at_deg2Rad;
  etaPole = at_surveyCenterDec * at_deg2Rad;

  /* Rotation */
  x1 = -sin(slong);
  y1 = cos(slat+etaPole) * cos(slong);
  z1 = sin(slat+etaPole) * cos(slong);

  /* convert to (ra, dec) and check bounds */

  ra = atan2 (y1, x1)+anodeSurvey;
  dec = asin(z1);
  ra = ra * at_rad2Deg;
  dec = dec * at_rad2Deg;
  raPtr = &ra;
  decPtr = &dec;
  atBound2(decPtr, raPtr);

/* from atEqToGC */

  /* Convert back to radians */
  ra = ra * at_deg2Rad;
  dec = dec * at_deg2Rad;
  anode = anode * at_deg2Rad;
  ainc = ainc * at_deg2Rad;

  /* Rotation */
  x1 = cos(ra-anode)*cos(dec);
  y1 = sin(ra-anode)*cos(dec);
  z1 = sin(dec);
  x2 = x1;
  y2 = y1*cos(ainc) + z1*sin(ainc);
  z2 =-y1*sin(ainc) + z1*cos(ainc);

  *amu = atan2 (y2, x2)+anode;
  *anu = asin(z2);

  /* Convert back to degrees */
  *amu = *amu * at_rad2Deg;
  *anu = *anu * at_rad2Deg;
  atBound2(anu, amu);

return;
}


/*<AUTO EXTRACT>

  ROUTINE: atSurveyToAzelpaOld

  DESCRIPTION:
<HTML>
        Convert from survey to azelpa coordinates
	
</HTML>

  RETURN VALUES:
<HTML>
  void
</HTML>
</AUTO>*/
void atSurveyToAzelpaOld(double slong, /* IN: survey longitude (lambda) */
		      double slat, /* IN: survey latitude (eat) */
		      TSTAMP *tstamp, /* IN: time stamp of the observation */
		      double *az,	/* OUT:  azimuth  */
		      double *el,	/* OUT: elevation */
		      double *pa	/* OUT: position angle */
		      ) {
  /* This code was originally written in FORTRAN by Steve Kent, 
     translated to C by Jeff Pier, and transplanted into astrotools
     by Chris Stoughton on 28-Apr-95

   ** IMPORTANT NOTE:  changed old definition of survey latitude and
   **	longtidue to new definition (i.e., swapped them)

   ** Survey poles are at intersection of galactic plane and celestial
   ** equator.  Hey, this is like the old L(I) and B(I) Galactic ccords!
   ** x2,y2,z2 coordinate system has x2 axis pointing towards survey pole
   ** in West, y2 axis in survey equator pointing to DEC=0 in a plane
   ** with NCP and NGP, and z2 axis aligned with NCP.  SLONG=0  for survey
   ** meridian that passes through NGP.  slat increases to the N.
   ** slong is 90 deg at easternmost survey pole.  Drift scans are along line
   ** slat=const, slong increasing.
   */

  double  x,  y,  z;
  double x1, y1, z1; 
  double x2, y2, z2; 
  double xi, yi, zi;
  double xl, zl;
  double phi = 90.0 - at_site_latitude; /* co-latitude of the site */
  char *timetype = "TAI";

  double hap = atTstampToLst(tstamp,at_site_longitude, timetype) - 
    at_surveyCenterRa;

  x2 = -sin(slong * at_deg2Rad);
  y2 = cos(slong * at_deg2Rad)*cos((slat+at_surveyCenterDec) * 
				      at_deg2Rad);
  z2 = cos(slong * at_deg2Rad)*sin((slat+at_surveyCenterDec) * 
				      at_deg2Rad);
  /*
   ** x1,y1,z1 coordinates are standard celestial hour angle and decl.
   ** rotate x2... system by hap=hour angle of NGP
   */
  x1 = x2*cos(hap * at_deg2Rad)+y2*sin(hap * at_deg2Rad); 
  /* = sin(dec)*sin(ha) */
  y1 = y2*cos(hap * at_deg2Rad)-x2*sin(hap * at_deg2Rad); 
  /* = sin(dec)*cos(ha) */
  z1 = z2;					
  /* = cos(dec) */
  /*
   ** x,y,z coordinates are standard AZ, EL at the telescope.  x points West
   ** y points S, Z points to zenith
   ** tilt about x1 axis by phi = colatitude of APO
   */
  x = x1;					
  /* = -sin(az)*cos(el) */
  y = y1*cos(phi * at_deg2Rad)-z1*sin(phi * at_deg2Rad);	
  /* = -cos(az)*cos(el) */
  z = z1*cos(phi * at_deg2Rad) + y1*sin(phi * at_deg2Rad);
  /* = sin(el */
  /*
   ** Now get AZ, z
   */
  *az = atan2(-x,-y) * at_rad2Deg;
  if (*az < 0. ) *az += 360.;
  *el = asin(z) * at_rad2Deg;
  /*
   ** NOW GET PA OF A VECTOR IN THE +LAT DIRECTION RELATIVE TO VERTICAL AS SEEN
   ** BY THE INSTRUMENT ROTATOR.
   */
  x2 = -cos(slong * at_deg2Rad);
  y2 = -sin(slong * at_deg2Rad)*cos((slat+at_surveyCenterDec) * 
				       at_deg2Rad);
  z2 = -sin(slong * at_deg2Rad)*sin((slat+at_surveyCenterDec) * 
				       at_deg2Rad);
  /*
   ** X1,Y1,Z1 COORDINATES ARE STANDARD CELESTIAL HOUR ANGLE AND DECL.
   ** ROTATE X2... SYSTEM BY HP=HOUR ANGLE OF NGP
   */
  x1 = x2*cos(hap * at_deg2Rad) + y2*sin(hap * at_deg2Rad); 
  /* = sin(dec)*sin(HA) */
  y1 = y2*cos(hap * at_deg2Rad) - x2*sin(hap * at_deg2Rad); 
  /* = sin(dec)*cos(HA) */
  z1=z2;                                   	  
  /* = cos(dec) */
  /*
   ** X,Y,Z COORDINATES ARE STANDARD AZ, EL AT THE TELESCOPE.  X POINTS WEST
   ** Y POINTS S, Z POINTS TO ZENITH
   ** TILT ABOUT X1 AXIS BY phi = COLATITUDE OF APO
   */
  x = x1;					  
  /* = -sin(az)*cos(el) */
  y = y1*cos(phi * at_deg2Rad) - z1*sin(phi * at_deg2Rad);  
  /* = -cos(az)*cos(el) */
  z = z1*cos(phi * at_deg2Rad) + y1*sin(phi * at_deg2Rad);  
  /* = sin(el) */
  /*
   ** ROTATE ABOUT VERTICAL SO YI POINTS IN THE AZ DIRECTION FOUND ABOVE
   */
  xi = -x*cos(*az * at_deg2Rad) + y*sin(*az * at_deg2Rad);
  yi = -y*cos(*az * at_deg2Rad) - x*sin(*az * at_deg2Rad);
  zi = z;
  /*
   ** NOW GET LOCAL COORDS CENTERED ON AZ, EL.
   */
  xl = xi;
  /* yl = yi*cos(*el * at_deg2Rad) + zi*sin(*el * at_deg2Rad); <- unused variable */
  zl = zi*cos(*el * at_deg2Rad) - yi*sin(*el * at_deg2Rad);
  /*
   ** ROTATOR ANGLE: POSITIVE FOR N THRU EAST
   */
  *pa = atan2(-xl,zl) * at_rad2Deg;
  
  return;
}
/* <AUTO>
   FILE: asatSurveyGeometry
<HTML>
   Routines which convert to and from survey-specific coodinate systems
   <p>
   There are five coordinate systems:
   <ul>
   <li> Equatorial -- ra, dec
   <li> Galactic -- galactic longitude, galactic latitude
   <li> Great Circle -- mu, nu (with node and inclination also specified)
   <li> Survey -- survey longitude, survey latitude (lambda, eta)
   <li> Azelpa -- azimuth, elevation, and position angle of the telescope
   </ul>
   The functions here convert Equatorial coordinates to and from 
   Galactic, Great Circle, and Survey coordinates; also between Survey
   and Azelpa.
   <P>
   Survey coordinates are (lambda, eta).  Lines of constant lambda are
   parallels, and lines of constant eta are meridians which go through
   the survey pole.  The center of a great circle scan will be a line
   of constant eta.
   <P>
   Great circle coordinates (mu, nu) are defined so that the line down the
   center of a stripe (which is a meridian in survey coordinates) is the
   parallel nu=0.  So, lines of constant mu are meridians and lines of
   constant nu are parallels.  Great circle coordinates are specific to a
   survey stripe.
   <p>
   To convert to and from Great Circle coordinates, you must input the
   node and inclination of the reference great circle.  For "normal" drift
   scan great circles, use <code>node=at_surveyCenterRa - 90</code>degrees,
   and <code>inc=survey latitude + at_surveyCenterDec</code>.
   <p>
   The survey latitudes for SDSS stripes are <code> +/- n*at_stripeSeparation
   </code>.
   <p>
   The limits on these coordinates are:
   <ul>
   <li> 0 <= (ra, glong, mu) < 360.0
   <li> -180.0 <= lambda < 180.0
   <li> -90 <= (dec, glat, nu, eta) < 90.0
   </ul>
   <p>
   The survey center is defined with the external const double values
   <ul>
   <li><code> at_surveyCenterRa = 185.0</code>
   <li><code> at_surveyCenterDec = 32.5</code>
   </ul>
   This (ra,dec) transforms to:
   <ul>
   <li><bf>galactic</bf> gLong=172.79876542 gLat=81.32406952
   <li><bf>great circle</bf>(with node=95.0, inclination=32.5) mu=185.0 nu=0.0
   <li><bf>survey</bf> lambda=0.0 eta=0.0
   </ul>
</HTML>
   </AUTO>
*/

#include <math.h>
#include "atSlalib.h"
#include "atConversions.h"
#include "atSurveyGeometry.h"
#include "slalib.h"


/*<AUTO EXTRACT>

  ROUTINE: atSurveyToAzelpa

  DESCRIPTION:
<HTML>
        Convert from survey to azelpa coordinates
	
</HTML>

  RETURN VALUES:
<HTML>
  void
</HTML>
</AUTO>*/
void atSurveyToAzelpa(double slong, /* IN: survey longitude (lambda) */
		      double slat, /* IN: survey latitude (eat) */
		      TSTAMP *tstamp, /* IN: time stamp of the observation */
		      double *az,	/* OUT:  azimuth  */
		      double *el,	/* OUT: elevation */
		      double *pa	/* OUT: position angle */
		      ) {
  /* This code was originally written in FORTRAN by Steve Kent, 
     translated to C by Jeff Pier, and transplanted into astrotools
     by Chris Stoughton on 28-Apr-95

   ** IMPORTANT NOTE:  changed old definition of survey latitude and
   **	longitude to new definition (i.e., swapped them)

   ** Survey poles are at intersection of galactic plane and celestial
   ** equator.  Hey, this is like the old L(I) and B(I) Galactic ccords!
   ** x2,y2,z2 coordinate system has x2 axis pointing towards survey pole
   ** in West, y2 axis in survey equator pointing to DEC=0 in a plane
   ** with NCP and NGP, and z2 axis aligned with NCP.  SLONG=0  for survey
   ** meridian that passes through NGP.  slat increases to the N.
   ** slong is 90 deg at easternmost survey pole.  Drift scans are along line
   ** slat=const, slong increasing.
   */

  double  x,  y,  z;
  double x1, y1, z1; 
  double x2, y2, z2; 
  double v3[3];
  double xi, yi, zi;
  double xl, zl;
  double phi = 90.0 - at_site_latitude; /* co-latitude of the site */

  double MJDtai, preNutMat[3][3];
  double hap, last, lastCMP, decCMP;
  char *timetype = "TAI";

/*rbh 25 Feb 1998 - next line added, following modified .
 */
  MJDtai = atTstampToMJD(tstamp);
  last = atTstampToLst(tstamp,at_site_longitude, timetype) 
    + slaEqeqx(MJDtai)*at_rad2Deg;

/* rbh 25 Feb 1998 - make a vector of zenith true coord, so can precess 
 *  & nutate to mean eqtr & eqnx of J2000
 */

  v3[2] = sin(at_site_latitude * at_deg2Rad);
  v3[1] = cos(at_site_latitude * at_deg2Rad)*sin(last*at_deg2Rad);
  v3[0] = cos(at_site_latitude * at_deg2Rad)*cos(last*at_deg2Rad);


  slaPrenut(2000., MJDtai, preNutMat);

  slaDimxv(preNutMat, v3, v3);
  slaDcc2s(v3, &lastCMP, &decCMP);
  lastCMP *= at_rad2Deg;
  decCMP *= at_rad2Deg;

  hap = lastCMP - at_surveyCenterRa;
  phi = 90.0 - decCMP;

/* rbh 25 Feb 1998- this system defined by y2 at intersection of
 *  survey and celest equators, with x2 pointing at west survey
 *  pole, and z2 is tilted 32.5 deg from NCP
 */

  x2 = slat+at_surveyCenterDec;
  x2 = -sin(slong * at_deg2Rad);
  y2 = cos(slong * at_deg2Rad)*cos((slat+at_surveyCenterDec) * 
				      at_deg2Rad);
  z2 = cos(slong * at_deg2Rad)*sin((slat+at_surveyCenterDec) * 
				      at_deg2Rad);

  /*
   ** x1,y1,z1 coordinates are standard celestial hour angle and decl.
   ** rotate x2... system by hap=hour angle of NGP
   */
  x1 = x2*cos(hap * at_deg2Rad)+y2*sin(hap * at_deg2Rad); 
  /* = sin(dec)*sin(ha) */
  y1 = y2*cos(hap * at_deg2Rad)-x2*sin(hap * at_deg2Rad); 
  /* = sin(dec)*cos(ha) */
  z1 = z2;					
  /* = cos(dec) */
  /*
   ** x,y,z coordinates are standard AZ, EL at the telescope.  x points West
   ** y points S, Z points to zenith
   ** tilt about x1 axis by phi = colatitude of APO
   */
  x = x1;					
  /* = -sin(az)*cos(el) */
  y = y1*cos(phi * at_deg2Rad)-z1*sin(phi * at_deg2Rad);	
  /* = -cos(az)*cos(el) */
  z = z1*cos(phi * at_deg2Rad) + y1*sin(phi * at_deg2Rad);
  /* = sin(el */
  /*
   ** Now get AZ, z
   */
  *az = atan2(-x,-y) * at_rad2Deg;
  if (*az < 0. ) *az += 360.;
  *el = asin(z) * at_rad2Deg;
  /*
   ** NOW GET PA OF A VECTOR IN THE +LAT DIRECTION RELATIVE TO VERTICAL AS SEEN
   ** BY THE INSTRUMENT ROTATOR.
   */
  x2 = -cos(slong * at_deg2Rad);
  y2 = -sin(slong * at_deg2Rad)*cos((slat+at_surveyCenterDec) * 
				       at_deg2Rad);
  z2 = -sin(slong * at_deg2Rad)*sin((slat+at_surveyCenterDec) * 
				       at_deg2Rad);
  /*
   ** X1,Y1,Z1 COORDINATES ARE STANDARD CELESTIAL HOUR ANGLE AND DECL.
   ** ROTATE X2... SYSTEM BY HP=HOUR ANGLE OF NGP
   */
  x1 = x2*cos(hap * at_deg2Rad) + y2*sin(hap * at_deg2Rad); 
  /* = sin(dec)*sin(HA) */
  y1 = y2*cos(hap * at_deg2Rad) - x2*sin(hap * at_deg2Rad); 
  /* = sin(dec)*cos(HA) */
  z1=z2;                                   	  
  /* = cos(dec) */
  /*
   ** X,Y,Z COORDINATES ARE STANDARD AZ, EL AT THE TELESCOPE.  X POINTS WEST
   ** Y POINTS S, Z POINTS TO ZENITH
   ** TILT ABOUT X1 AXIS BY phi = COLATITUDE OF APO
   */
  x = x1;					  
  /* = -sin(az)*cos(el) */
  y = y1*cos(phi * at_deg2Rad) - z1*sin(phi * at_deg2Rad);  
  /* = -cos(az)*cos(el) */
  z = z1*cos(phi * at_deg2Rad) + y1*sin(phi * at_deg2Rad);  
  /* = sin(el) */
  /*
   ** ROTATE ABOUT VERTICAL SO YI POINTS IN THE AZ DIRECTION FOUND ABOVE
   */
  xi = -x*cos(*az * at_deg2Rad) + y*sin(*az * at_deg2Rad);
  yi = -y*cos(*az * at_deg2Rad) - x*sin(*az * at_deg2Rad);
  zi = z;
  /*
   ** NOW GET LOCAL COORDS CENTERED ON AZ, EL.
   */
  xl = xi;
/* commented the following line, as yl is never used anywhere
  yl = yi*cos(*el * at_deg2Rad) + zi*sin(*el * at_deg2Rad);
*/
  zl = zi*cos(*el * at_deg2Rad) - yi*sin(*el * at_deg2Rad);
  /*
   ** ROTATOR ANGLE: POSITIVE FOR N THRU EAST
   */
  *pa = atan2(-xl,zl) * at_rad2Deg;
  
  return;
}

/*<AUTO EXTRACT>
  
  ROUTINE: atBound
  
  DESCRIPTION:
  <HTML>
  Set the angle within the specified bounds
</HTML>

  RETURN VALUES:
<HTML>
  void
</HTML>
</AUTO>*/
void atBound (
	      double *angle,	/* MODIFIED -- the angle to bound in degrees*/
	      double min,	/* IN -- inclusive minimum value */
	      double max	/* IN -- exclusive maximum value */
	      ) {
  while (*angle<min) {
    *angle += 360.0;
  }
  while (*angle>=max) {
    *angle -= 360.0;
  }
  return;
}

/*<AUTO EXTRACT>

  ROUTINE: atBound2

  DESCRIPTION:
<HTML>
        Set the two angles within legal bounds
</HTML>

  RETURN VALUES:
<HTML>
  void
</HTML>
</AUTO>*/
void atBound2(
	      double *theta,	/* MODIFIED -- the -90 to 90 angle */
	      double *phi	/* MODIFIED -- the 0 to 360 angle */
	      ) {
  atBound(theta, -180.0, 180.0);
  if (fabs(*theta) > 90.0) {
    *theta = 180.0 - *theta;
    *phi += 180;
  }
  atBound(theta, -180.0, 180.0);
  atBound(phi, 0.0, 360.0);
  if (fabs(*theta)==90.0) *phi=0.;
  return;
}


/*<AUTO EXTRACT>

  ROUTINE: atVEqToGC

  DESCRIPTION:
<HTML>
        Converts Equatorial coordinates to Great Circle coordinates.
</HTML>

  RETURN VALUES:
<HTML>
  void
</HTML>
</AUTO>*/
void atVEqToGC (
	       VECTOR *vRaMu,	/* IN -- ra in degrees OUT -- mu in degrees*/
	       VECTOR *vDecNu,	/* IN -- dec in degrees OUT -- nu in degrees*/
	       VECTOR_TYPE anode,	/* IN -- node in degrees*/
	       VECTOR_TYPE ainc	/* IN -- inclination in degrees*/
	       ) {
  VECTOR_TYPE x1, y1, z1, x2, y2, z2, raOut, decOut;
  int i;
  /* Convert to radians */
  anode = anode * at_deg2Rad;
  ainc = ainc * at_deg2Rad;

  for(i=0;i<vRaMu->dimen;i++) {
    raOut = vRaMu->vec[i] * at_deg2Rad;
    decOut = vDecNu->vec[i] * at_deg2Rad;
    /* Rotation */
    x1 = cos(raOut-anode)*cos(decOut);
    y1 = sin(raOut-anode)*cos(decOut);
    z1 = sin(decOut);
    x2 = x1;
    y2 = y1*cos(ainc) + z1*sin(ainc);
    z2 =-y1*sin(ainc) + z1*cos(ainc);
    
    vRaMu->vec[i] = atan2 (y2, x2)+anode;
    vDecNu->vec[i] = asin(z2);
    /* Convert back to degrees */
    vRaMu->vec[i] = vRaMu->vec[i] * at_rad2Deg;
    vDecNu->vec[i] = vDecNu->vec[i] * at_rad2Deg;
    atBound2(&vDecNu->vec[i], &vRaMu->vec[i]);
  }
  return;
}



/*<AUTO EXTRACT>

  ROUTINE: atVGCToEq

  DESCRIPTION:
<HTML>
        Converts Great Circle to Equatorial coordinates.
</HTML>

  RETURN VALUES:
<HTML>
  void
</HTML>
</AUTO>*/
void atVGCToEq (
	       VECTOR *vMuRa,	/* IN -- mu in degrees OUT -- ra in degrees*/
	       VECTOR *vNuDec,	/* IN -- nu in degrees OUT -- dec in degrees*/
	       VECTOR_TYPE anode,	/* IN -- node in degrees*/
	       VECTOR_TYPE ainc	/* IN -- inclination in degrees*/
	       ) {
  VECTOR_TYPE x1, y1, z1, x2, y2, z2, muOut, nuOut;
  int i;
  /* Convert to radians */
  anode = anode * at_deg2Rad;
  ainc = ainc * at_deg2Rad;

  for(i=0;i<vMuRa->dimen;i++) {
    muOut = vMuRa->vec[i] * at_deg2Rad;
    nuOut = vNuDec->vec[i] * at_deg2Rad;
    /* Rotation */
    x2 = cos(muOut-anode)*cos(nuOut);
    y2 = sin(muOut-anode)*cos(nuOut);
    z2 = sin(nuOut);
    x1 = x2;
    y1 = y2*cos(ainc) - z2*sin(ainc);
    z1 = y2*sin(ainc) + z2*cos(ainc);
    vMuRa->vec[i] = atan2 (y1, x1)+anode;
    vNuDec->vec[i] = asin(z1);

    /* Convert back to degrees */
    vMuRa->vec[i] = vMuRa->vec[i] * at_rad2Deg;
    vNuDec->vec[i] = vNuDec->vec[i] * at_rad2Deg;
    atBound2(&vNuDec->vec[i], &vMuRa->vec[i]);
  }
  return;
}


/*<AUTO EXTRACT>

  ROUTINE: atVEqToGal

  DESCRIPTION:
<HTML>
        Converts Equatorial to Galactic coordinates.
</HTML>

  RETURN VALUES:
<HTML>
  void
</HTML>
</AUTO>*/
void atVEqToGal (
		VECTOR *vRaGLong,/* IN -- ra in degrees OUT -- Galactic longitude in degrees */
		VECTOR *vDecGLat  /* IN -- dec in degrees OUT -- Galactic latitude in degrees */
		) {
  unsigned int i;
  for(i=0;i<vRaGLong->dimen;i++) {
    /* Convert to radians */
    vRaGLong->vec[i] *= at_deg2Rad;
    vDecGLat->vec[i] *= at_deg2Rad;
    /* Use SLALIB to do the actual conversion */
    slaEqgal(vRaGLong->vec[i], vDecGLat->vec[i],
	     &vRaGLong->vec[i], &vDecGLat->vec[i]);
    /* Convert back to degrees */
    vRaGLong->vec[i] *= at_rad2Deg;
    vDecGLat->vec[i] *= at_rad2Deg;
    atBound2(&vDecGLat->vec[i], &vRaGLong->vec[i]);
  }
  return;
}

/*<AUTO EXTRACT>

  ROUTINE: atVGalToEq

  DESCRIPTION:
<HTML>
        Converts Galactic to Equatorial coordinates.
</HTML>

  RETURN VALUES:
<HTML>
  void
</HTML>
</AUTO>*/

void atVGalToEq (
		VECTOR *vGLongRa,/* IN -- Galactic longitude in degrees OUT -- ra in degrees */
		VECTOR *vGLatDec  /* IN -- Galactic latitude in degrees OUT -- dec in degrees */
		) {
  unsigned int i;
  for(i=0;i<vGLongRa->dimen;i++) {
    /* Convert to radians */
    vGLongRa->vec[i] *= at_deg2Rad;
    vGLatDec->vec[i] *= at_deg2Rad;
    /* Use SLALIB to do the actual conversion */
    slaGaleq(vGLongRa->vec[i], vGLatDec->vec[i],
	     &vGLongRa->vec[i], &vGLatDec->vec[i]);
    /* Convert back to degrees */
    vGLongRa->vec[i] *= at_rad2Deg;
    vGLatDec->vec[i] *= at_rad2Deg;
    atBound2(&vGLatDec->vec[i], &vGLongRa->vec[i]);
  }
  return;
}

/*<AUTO EXTRACT>

  ROUTINE: atVEqToSurvey

  DESCRIPTION:
<HTML>
        Converts Equatorial to Survey coordinates.
</HTML>

  RETURN VALUES:
<HTML>
  void
</HTML>
</AUTO>*/
void atVEqToSurvey (
		    VECTOR *vRaSLong,/* IN -- ra in degrees OUT -- Survey longitude in degrees */
		    VECTOR *vDecSLat  /* IN -- dec in degrees OUT -- Survey latitude in degrees */
		    ) {
  unsigned int i;
  VECTOR_TYPE x1, y1, z1;
  VECTOR_TYPE anode, etaPole;

  /* Convert to radians */
  anode = at_surveyCenterRa - 90.;
  anode = anode * at_deg2Rad;
  etaPole = at_surveyCenterDec * at_deg2Rad;

  for(i=0;i<vRaSLong->dimen;i++) {
    /* Convert to radians */
    vRaSLong->vec[i] *= at_deg2Rad;
    vDecSLat->vec[i] *= at_deg2Rad;
 
    /* Rotation */
    x1 = cos(vRaSLong->vec[i]-anode)*cos(vDecSLat->vec[i]);
    y1 = sin(vRaSLong->vec[i]-anode)*cos(vDecSLat->vec[i]);
    z1 = sin(vDecSLat->vec[i]);
   
    vRaSLong->vec[i] = -asin(x1);
    vDecSLat->vec[i] = atan2(z1, y1) - etaPole;
    /* Convert to degrees */
    vRaSLong->vec[i] *= at_rad2Deg;
    vDecSLat->vec[i] *= at_rad2Deg;
    atBound2(&vRaSLong->vec[i], &vDecSLat->vec[i]);
    atBound(&vDecSLat->vec[i], -180.0, 180.0);
 if (vDecSLat->vec[i] > 90-at_surveyCenterDec) {
                vDecSLat->vec[i] -= 180;
                vRaSLong->vec[i] = 180 - vRaSLong->vec[i];
        }
  atBound(&vRaSLong->vec[i], -180.0, 180.0);


  }
  return;
}

/*<AUTO EXTRACT>

  ROUTINE: atVSurveyToEq

  DESCRIPTION:
<HTML>
        Converts Survey to Equatorial coordinates.
</HTML>

  RETURN VALUES:
<HTML>
  void
</HTML>
</AUTO>*/
void atVSurveyToEq (
		    VECTOR *vSLongRa,/* OUT -- ra in degrees IN -- Survey longitude in degrees */
		    VECTOR *vSLatDec  /* OUT -- dec in degrees IN -- Survey latitude in degrees */
		    ) {
  unsigned int i;  
  VECTOR_TYPE x1, y1, z1;
  VECTOR_TYPE anode, etaPole;

  /* Convert to radians */
  anode = at_surveyCenterRa - 90.;
  anode = anode * at_deg2Rad;
  etaPole = at_surveyCenterDec * at_deg2Rad;
 
  for(i=0;i<vSLongRa->dimen;i++) {
    /* Convert to radians */
    vSLongRa->vec[i] *= at_deg2Rad;
    vSLatDec->vec[i] *= at_deg2Rad;
 
    /* Rotation */
    x1 = -sin(vSLongRa->vec[i]);
    y1 = cos(vSLatDec->vec[i]+etaPole) * cos(vSLongRa->vec[i]);
    z1 = sin(vSLatDec->vec[i]+etaPole) * cos(vSLongRa->vec[i]);

    vSLongRa->vec[i] =  atan2 (y1, x1)+anode;
    vSLatDec->vec[i] = asin(z1);
  
    /* Convert to degrees */
    vSLongRa->vec[i] *= at_rad2Deg;
    vSLatDec->vec[i] *= at_rad2Deg;
    atBound2(&vSLatDec->vec[i], &vSLongRa->vec[i]);
  }
  return;
}



/*<AUTO EXTRACT>

  ROUTINE: atDefineGC

  DESCRIPTION:
<HTML>
        Given two points on a great circle (in decimal degrees), compute
	the node and inclination.
	
</HTML>

  RETURN VALUES:
<HTML>
  void
</HTML>
</AUTO>*/
void atDefineGC(double ra1,		/* IN: */
		double dec1,		/* IN: */
		double ra2,		/* IN: */
		double dec2,		/* IN: */
		double *anode,		/* OUT: */
		double *ainc		/* OUT: */
		) {
  double r1[3], r2[3], p[3], n[3], temp[3], z[3];
  double  decn, mod, acosinc;
  shAssert(ra1!=ra2 || dec1!=dec2);
  /* Convert the two coordinate points to Cartesian */
  slaDcs2c(ra1, dec1, r1);
  slaDcs2c(ra2, dec2, r2);
  /* The GC pole is the normalized cross product of the vector points */
  slaDvxv(r1,r2,temp);
  slaDvn(temp,p,&mod);
  /* Find the inclination angle between the pole and the z axis */
  z[0] = 0;
  z[1] = 0;
  z[2] = 1;
  acosinc = slaDvdv(p,z);
  *ainc = acos(acosinc);
  /* the node is the z axis crossed with the pole */
  temp[0] = - p[1];
  temp[1] = p[0];
  temp[2] = 0;
  slaDvn(temp,n,&mod);
  slaDcc2s(n,anode,&decn);
  if (*anode<0.0) *anode = *anode+2*M_PI;
}


/*<AUTO EXTRACT>

  ROUTINE: atGCDecFromRa

  DESCRIPTION:
<HTML>
        Given an RA, find a corresponding dec on the great circle.
	If the pole of the great circle lies on the equator, this is
	an undefined operation.
	
</HTML>

  RETURN VALUES:
<HTML>
  void
</HTML>
</AUTO>*/
void atGCDecFromRa(double ra,	/* IN: */
		double anode,	/* IN: */
		double ainc,	/* IN: */
		double *dec	/* OUT: */
		) {

  double r[3], temp[3], p2[3], p[3];
  double mod, tempra;

  shAssert(ainc!=M_PI/2.0);

  /* Find the pole, p2, of the longitude line that goes through the
	input RA */
  slaDcs2c(ra, 0, r);
  temp[0] = - r[1];
  temp[1] = r[0];
  temp[2] = 0;
  slaDvn(temp,p2,&mod);

  /* Find the pole of the great circle */
  slaDcs2c(anode-M_PI/2,M_PI/2-ainc,p);
  /*
  p[0] = sin(ainc)*sin(anode);
  p[1] = - sin(ainc)*cos(anode);
  p[2] = cos(ainc);
  */

  /* The intersection is perpendicular to both poles */
  slaDvxv(p2,p,temp);
  slaDvn(temp,r,&mod);
  
  /* Return the declination in angle */;
  slaDcc2s(r,&tempra,dec);
  if (p[2]<0.0) *dec = -*dec;

}



/*<AUTO EXTRACT>

  ROUTINE: atAngleGCToRa

  DESCRIPTION:
<HTML>
  Given a point on the great circle, determine the angle between the
  great circle and increasing RA.
	
</HTML>

  RETURN VALUES:
<HTML>
  void
</HTML>
</AUTO>*/
void atAngleGCToRa(double ra,			/* IN: */
		double dec,			/* IN: */
		double anode,			/* IN: */
		double ainc,			/* IN: */
		 double *angle			/* OUT: */
		){

  double R[3],r1[3], r2[3], temp[3], z[3], p[3];
  double mod, ra1, dec1, ra2, dec2;

  /* Convert point to Cartesian */
  slaDcs2c(ra, dec, R);

  /* Find direction of tangent to great circle */
  slaDcs2c(anode-M_PI/2,M_PI/2-ainc,p);
  /*
  p[0] = sin(ainc)*sin(anode);
  p[1] = - sin(ainc)*cos(anode);
  p[2] = cos(ainc);
  */
  slaDvxv(p,R,temp);
  slaDvn(temp,r1,&mod);

  /* Find direction of tangent to RA */
  z[0] = 0;
  z[1] = 0;
  z[2] = 1;
  slaDvxv(z,R,temp);
  slaDvn(temp,r2,&mod);

  /* Find angle between these vectors */
  slaDcc2s(r1,&ra1,&dec1);
  slaDcc2s(r2,&ra2,&dec2);
  *angle = slaDsep(ra1,dec1,ra2,dec2);
  
}


/*<AUTO EXTRACT>

  ROUTINE: atEtaToStripeNum


  DESCRIPTION:
<HTML>
Determine stripe number, given eta
</HTML>

  RETURN VALUES:
<HTML>
  void
</HTML>
</AUTO>*/
void atEtaToStripeNum(
		      double eta,	       	/* IN: */
		      int *num			/* OUT: */
		      )
{

  double stripeSep = 2.5;
  
  *num = (int)(eta+58.75)/stripeSep;
  
}

void cobecp_(float *longitude, float *latitude,
	     char *coord, int *resolution, int *pixel
	     );
/*<AUTO EXTRACT>
  
  ROUTINE: atCoordToCobePix

  DESCRIPTION:  transform sky coordinates to COBE pixel number
</AUTO>*/
void atCoordToCobePix(
		      float longitude,
		      float latitude,
		      char *coord, /* q for equatorial; 
				      g for galactic; e for ecliptic */
		      int resolution, /* 9 for DIRBE data */
		      int *pixel /* the pixel number */
		      ) {
  cobecp_(&longitude, &latitude, coord, &resolution, pixel);
  return;
}
void cobepc_(int *pixel, char *coord, int *resolution, 
		   float *longitude, float *latitude);
/*<AUTO EXTRACT>
  
  ROUTINE: atCobePixToCoord

  DESCRIPTION:  transform COBE pixel numbers to sky coordinates
</AUTO>*/
void atCobePixToCoord(
		      int pixel, /* the input pixel number */
		      char *coord, /* q for equatorial; 
				      g for galactic; e for ecliptic */
		      int resolution, /* 9 for DIRBE data */
		      float *longitude,
		      float *latitude
		      ) {
  cobepc_(&pixel, coord, &resolution, longitude, latitude);
  return;
}

/*<AUTO EXTRACT>
  
  ROUTINE: atVCoordToCobePix

  DESCRIPTION:  transform sky coordinates to COBE pixel number for VECTORS
</AUTO>*/
void atVCoordToCobePix(
		      VECTOR *longitude,
		      VECTOR * latitude,
		      char *coord, /* q for equatorial; 
				      g for galactic; e for ecliptic */
		      int resolution, /* 9 for DIRBE data */
		      VECTOR *pixel /* the pixel number */
		      ) {
  int i, pix;

  for (i=0; i<longitude->dimen; i++) {
    atCoordToCobePix(longitude->vec[i], latitude->vec[i], coord, resolution, 
		     &pix);
    pixel->vec[i] = pix;
  }
  return;
}

/*<AUTO EXTRACT>
  
  ROUTINE: atVCobePixToCoord

  DESCRIPTION:  transform COBE pixel number to sky coordinates for VECTORS
</AUTO>*/
void atVCobePixToCoord(
		       VECTOR *pixel, /* the pixel number */
		       char *coord, /* q for equatorial; 
				       g for galactic; e for ecliptic */
		       int resolution, /* 9 for DIRBE data */
		       VECTOR *longitude,
		       VECTOR * latitude
		       ) {
  int i;
  float tempLong, tempLat;
  for (i=0; i<longitude->dimen; i++) {
    atCobePixToCoord(pixel->vec[i], coord, resolution, 
		     &tempLong, &tempLat);
    longitude->vec[i] = tempLong;
    latitude->vec[i] = tempLat;
  }
  return;
}

/* <AUTO>
   FILE: atParallacticAngle
<HTML>
   <p>
   Based on an experimental version of atSurveyToAzelpa, this gets the 
     parallactic angle wrt scanning direction  correctly- 5 March 1998
   <p>
   This version converts (LMST,lat) to (LAST,lat), which are zenith
    coordinates referred to true equator and equinox of date, then 
    applies precession/nutation to convert to zenith coordinates referred 
    to mean eqtr & eqnx of J2000.  From that, can convert to GC coordinates, 
    and determine difference between zenith direction and direction of 
    scanning.
    This could all be done as well in survey coordinates, but still need
    to know the node and inclination of GC path.  The survey coordinates
    of a star do not tell you what great circle is being scanned, which
    is what is needed to get mu & nu components of refraction.
   <P>
   Survey coordinates are (lambda, eta).  Lines of constant lambda are
   parallels, and lines of constant eta are meridians which go through
   the survey poles.  The center of a great circle scan will be a line
   of constant eta.
   <P>
   Great circle coordinates (mu, nu) are defined so that the line down the
   center of a stripe (which is a meridian in survey coordinates) is the
   parallel nu=0.  So, lines of constant mu are meridians and lines of
   constant nu are parallels.  Great circle coordinates are specific to a
   survey stripe.
   <p>
   To convert to and from Great Circle coordinates, you must input the
   node and inclination of the reference great circle.  For "normal" drift
   scan great circles, use <code>node=at_surveyCenterRa - 90</code>degrees,
   and <code>inc=survey latitude + at_surveyCenterDec</code>.
   <p>
   The survey latitudes for SDSS stripes are <code> +/- n*at_stripeSeparation
   </code>.
   <p>
   The limits on these coordinates are:
   <ul>
   <li> 0 <= (ra, glong, mu) < 360.0
   <li> -180.0 <= lambda < 180.0
   <li> -90 <= (dec, glat, nu, eta) < 90.0
   </ul>
   <p>
   The survey center is defined with the external const double values
   <ul>
   <li><code> at_surveyCenterRa = 185.0</code>
   <li><code> at_surveyCenterDec = 32.5</code>
   </ul>
   This (ra,dec) transforms to:
   <ul>
   <li><bf>galactic</bf> gLong=172.79876542 gLat=81.32406952
   <li><bf>great circle</bf>(with node=95.0, inclination=32.5) mu=185.0 nu=0.0
   <li><bf>survey</bf> lambda=0.0 eta=0.0
   </ul>
</HTML>
   </AUTO>
*/



/*<AUTO EXTRACT>

  ROUTINE: atParallacticAngleGC

  DESCRIPTION:
<HTML>
        Calculate the parallactic angle in GC, ie direction to
         zenith wrt (mu,nu) axes

	
</HTML>

  RETURN VALUES:
<HTML>
  void
</HTML>
</AUTO>*/
void atParallacticAngleGC(
	double smu,     /* IN: star mu */
        double snu,     /* IN: star nu */
	TSTAMP *tstamp, /* IN: TAI time stamp of the observation */
	double anode,   /* IN: mu value of GC node */
	double ainc,    /* IN: inclination of GC */
	double *pa	/* OUT: pier-allactic angle */
		      ) {
  /* Written by Bob Hindsley 5 March 1998 */

  double  x,  y;
  double x1, y1, z1; 
  double v3[3];

  double MJDtai, last, preNutMat[3][3];
  double lastCMP, decCMP, amuZen, anuZen;
  char *timetype = "TAI";


/* First part of function is like rbh version of atSurveyToAzelpa, because 
 *  both have to find the zenith position.  First find wrt true eqr & eqnx 
 *  of date.
 */

  MJDtai = atTstampToMJD(tstamp);
  last = atTstampToLst(tstamp,at_site_longitude, timetype) 
    + slaEqeqx(MJDtai)*at_rad2Deg;

/* Make a vector of zenith true coord, so can precess 
 *  & nutate to mean eqtr & eqnx of J2000.
 */

  v3[2] = sin(at_site_latitude * at_deg2Rad);
  v3[1] = cos(at_site_latitude * at_deg2Rad)*sin(last*at_deg2Rad);
  v3[0] = cos(at_site_latitude * at_deg2Rad)*cos(last*at_deg2Rad);


  slaPrenut(2000., MJDtai, preNutMat);

  slaDimxv(preNutMat, v3, v3);
  slaDcc2s(v3, &lastCMP, &decCMP);
  lastCMP *= at_rad2Deg;
  decCMP *= at_rad2Deg;

  /* Now convert (lastCMP, decCMP) = zenith position in equatorial coord
   *  to (mu,nu) of zenith 
   */

  atEqToGC(lastCMP, decCMP, &amuZen, &anuZen, anode, ainc);

  /* Want a coordinate system with z1 on GC, mu=smu; x1 at GC pole;
  **  y1 on GC with mu = smu+90
  */

  x1 = sin(anuZen * at_deg2Rad);
  y1 = cos(anuZen * at_deg2Rad)*sin((amuZen-smu) * at_deg2Rad);
  z1 = cos(anuZen * at_deg2Rad)*cos((amuZen-smu) * at_deg2Rad);

  /* Now rotate about y1 to get z at star, x toward GC pole. */

  y = y1;
  x = x1*cos(snu * at_deg2Rad) - z1*sin(snu * at_deg2Rad);
  /*  z = x1*sin(snu * at_deg2Rad) + z1*cos(snu * at_deg2Rad); <- unused variable */

  /* Finally, pa is 90 - atan(y,x) */

  *pa = 90.0000 - (atan2(y,x) * at_rad2Deg);

}

