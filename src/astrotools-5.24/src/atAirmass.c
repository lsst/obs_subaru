/* <AUTO>
  FILE: atAirmass
<HTML>
  Routines to calculate airmass
</HTML>
  </AUTO>
*/
/***************************************************************
 *
 *  C routine to calculate airmass
 *
 *  Extracted from Jim Annis's code May 24, 1995
 *
 **************************************************************/

#include "dervish.h"
#include "slalib.h"
#include "atAirmass.h"
#include "atSlalib.h"
#include "atConversions.h"
#include "atSurveyGeometry.h"

/*<AUTO EXTRACT>

  ROUTINE: atAirmassFind

  DESCRIPTION:
<HTML>
	This routine will calculate the airmass given a UT 
	time in a TSTAMP struct, and the RA and DEC in degrees
	at the current epoch.
	<P>
	The routine uses Hardie's (1962) polynomial fit to 
	Bemporad's data for the relative air mass, X, in 
	units of the thickness at the zenith as tabulated by 
	Schoenberg (1929). This is adequate as it is accurate 
	to better than 0.1% up to X=6.8. Bemporad's tabulated 
	values are unlikely to be trustworthy to such 
	accuracy because of variations in density pressure, 
	and other conditions of the atmosphere assumed in 
	his work
</HTML>

  RETURN VALUES:
<HTML>
	Returns the airmass
</HTML>
</AUTO>*/

RET_CODE atAirmassFind(
		       TSTAMP *time, 
		       double degra, 
		       double degdec, 
		       double *airmass
		       ) {
  double ra,dec;		/* ra and dec in radians */
  double longitude;             /* position of the observatory, radians west  */
  double latitude;              /* position of the observatory, radians north */
  double mjd;                   /* modified julian date */
  double mjd_hours;             /* hr,min,sec in units of days */
  double gmst;                  /* CMST at 0h, in radians */
  double last;                  /* local apparent sidereal time in radians */
  double hour_angle;            /* the hour angle */
  double zenith_distance;       /* the zenith distance */
  int status;
  
  ra = at_deg2Rad*degra;
  dec = at_deg2Rad*degdec;
  
  /* find APO latitude */
  /* slaObs(0,"APO3.5",obsName,&longitude,&latitude,&height); */
  longitude = at_site_longitude * at_deg2Rad;
  latitude  = at_site_latitude  * at_deg2Rad;

  /* find modified julian date */
  slaCaldj(time->year,time->month,time->day,&mjd,&status);
  if (status > 0) {
    *airmass = 0;
    return SH_GENERIC_ERROR;
  }
  
  /* make mjd up to the second */
  slaDtf2d(time->hour,time->minute,time->second,&mjd_hours,&status);
  if (status > 0) {
    *airmass = 0;
    return SH_GENERIC_ERROR;
  }
  mjd = mjd + mjd_hours;
  
  /* GMST at 0 hr, in radians  */
  gmst = slaGmst(mjd);
  
  /* convert to local apparent ST using equation of the equinoxes */
  /* and (east) longitide of APO; last is in radians  */
  last = gmst + slaEqeqx(mjd) - longitude ;             
  
  /* find hour angle, in radians  */
  hour_angle = last - ra;
  
  /* find zenith distance, in radians  */
  zenith_distance = slaZd(hour_angle,dec,latitude);
  
  /* find airmass */
  *airmass = slaAirmas(zenith_distance);
  
  /* The routine uses Hardie's (1962) polynomial fit to Bemporad's data for
     the relative air mass, X, in units of the thickness at the zenith as
     tabulated by Schoenberg (1929). This is adequate as it is accurate to
     better than 0.1% up to X=6.8. Bemporad's tabulated values are unlikely
     to be trustworthy to such accuracy because of variations in density
     pressure, and other conditions of the atmosphere assumed in his work */
  
  return SH_SUCCESS;
  
}

/*<AUTO EXTRACT>

  ROUTINE: atAirmassFromZd

  DESCRIPTION: 
<HTML> 
        Nothing but a wrapper to sla_Airmas which takes
        a zenith distance.  This is specifically for use when one
        might be tempted to simply use sec(z), which will incur 0.25%
        errors at the Survey limit without any real good reason for
        doing so.
	<P>
	The routine uses Hardie's (1962) polynomial fit to 
	Bemporad's data for the relative air mass, X, in 
	units of the thickness at the zenith as tabulated by 
	Schoenberg (1929). This is adequate as it is accurate 
	to better than 0.1% up to X=6.8. Bemporad's tabulated 
	values are unlikely to be trustworthy to such 
	accuracy because of variations in density pressure, 
	and other conditions of the atmosphere assumed in 
	his work
</HTML>

  RETURN VALUES:
<HTML>
	Returns the airmass
</HTML>
</AUTO>*/

double atAirmassFromZd(
		       double degzd 
		       ) 
{
  
  double zenith_distance;       /* the zenith distance */
  double airmass;

  /* find zenith distance, in radians  */  
  zenith_distance = at_deg2Rad*degzd;
  
  /* find airmass */
  airmass = slaAirmas(zenith_distance);
  
  /* The routine uses Hardie's (1962) polynomial fit to Bemporad's data for
     the relative air mass, X, in units of the thickness at the zenith as
     tabulated by Schoenberg (1929). This is adequate as it is accurate to
     better than 0.1% up to X=6.8. Bemporad's tabulated values are unlikely
     to be trustworthy to such accuracy because of variations in density
     pressure, and other conditions of the atmosphere assumed in his work */
  
  return airmass;
  
}

/*<AUTO EXTRACT>

  ROUTINE: atMeridianAirmassFind

  DESCRIPTION:
<HTML>

Simply finds the airmass with respect to the 2.5m at APO at the
meridian given the DEC.  Everything is as for atAirmassFind above.<P>

</HTML>

  RETURN VALUES:
<HTML>
	Returns the airmass at the meridian (at APO 2.5m).
</HTML>
</AUTO>*/

double atMeridianAirmassFind(
			     double degdec    /*IN: DEC (degrees)*/
			     ) 
{
  double dec;		        /* dec in radians */
  double latitude;              /* 2.5m latitude in radians */
  double zenith_distance;       /* the zenith distance in radians */
  double airmass;               /* airmass at the meridian */
  double hour_angle;            /* hour angle (=0 at merdian) */
  
  dec = at_deg2Rad*degdec;
  latitude = at_deg2Rad*at_site_latitude;

  /* hour angle is 0 on the meridian */
  hour_angle = 0.0;
  
  /* find zenith distance, in radians  */
  zenith_distance = slaZd(hour_angle,dec,latitude);
  
  /* find airmass */
  airmass = slaAirmas(zenith_distance);
  
  return airmass;
}

/*<AUTO EXTRACT>

  ROUTINE: atAirmassFindHaDec

  DESCRIPTION:
<HTML>

Simply finds the airmass with respect to the 2.5m at APO given the HA
and DEC.  Everything is as for atAirmassFind above.<P>

</HTML>

  RETURN VALUES:
<HTML>
	Returns the airmass for a given HA and DEC (at APO 2.5m).
</HTML>
</AUTO>*/

double atAirmassFindHaDec(
			  double hahrs,     /*IN: HA (hours)*/
			  double degdec    /*IN: DEC (degrees)*/
			  ) 
{
  double dec;		        /* dec in radians */
  double latitude;              /* 2.5m latitude in radians */
  double zenith_distance;       /* the zenith distance in radians */
  double airmass;               /* airmass at the meridian */
  double hour_angle;            /* hour angle (=0 at merdian) */
  
  dec = at_deg2Rad*degdec;
  latitude = at_deg2Rad*at_site_latitude;

  /* hour angle in radians */
  hour_angle = hahrs*at_hrs2Deg*at_deg2Rad;
  
  /* find zenith distance, in radians  */
  zenith_distance = slaZd(hour_angle,dec,latitude);
  
  /* find airmass */
  airmass = slaAirmas(zenith_distance);
  
  return airmass;
}

/*<AUTO EXTRACT>

  ROUTINE: atZdDec2Ha

  DESCRIPTION:
<HTML>

Takes a zenith distance (deg) and DEC (degrees) and converts it to an
Hour Angle (hours). <P>

</HTML>

  RETURN VALUES:
<HTML>
Return HA (hours) given zenith distance (deg) and DEC (deg).
</HTML>
</AUTO>*/

double atZdDec2Ha(
		  double zddeg,       /*IN: ZD (degrees)*/
		  double decdeg       /*IN: DEC (degrees)*/
		  ) 
{
  double dec;		        /* dec in radians */
  double phi;                   /* 2.5m latitude in radians */
  double zd;                    /* zenith distance in radians */
  double hahrs;                 /* hour angle (hours) */
  double harad;                 /* hour angle (radians) */
  double cd,cp;
  double sd,sp;
  double cz;

  zd = at_deg2Rad*zddeg;
  dec = at_deg2Rad*decdeg;
  phi = at_deg2Rad*at_site_latitude;

  cd = cos(dec);
  sd = sin(dec);
  cp = cos(phi);
  sp = sin(phi);
  cz = cos(zd);

  harad = acos((cz-sd*sp)/(cd*cp));
  
  hahrs = harad*at_rad2Deg*at_deg2Hrs;

  return hahrs;
}





