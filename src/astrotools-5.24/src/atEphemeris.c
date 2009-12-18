/* <AUTO>
   FILE: atEphemeris
<HTML>
   Ephemeris routines based on Jeff Munn's IRAF routines
</HTML>
  </AUTO>
*/
/*
** Converted to C++ 27-April-1993 Jon Loveday, Fermilab
** Converted to C and radians  14-July-1995  Jon Loveday, Fermilab
** Converted back to degrees !!!  14-July-1995  Jon Loveday, Fermilab
** Replace atDateToEpoch, atEpochToDate, atDayOfYear, atJulday and atMst
**    with slalib calls.  2-Aug-1995  JL, Fermilab
*/

#include "atConversions.h"
#include "atEphemeris.h"
#include "slalib.h"

/*<AUTO EXTRACT>

  ROUTINE: atMoonPhase

  DESCRIPTION:
<HTML>
   Compute the phase of the moon (0.0 = new, 1.0 = full)
 (formula from Jean Meeus's Astronomical Formula for Calculators, 
 3rd Edition, pp. 155).
 This routine was originally called illuminated_fraction
</HTML>
</AUTO>*/

double atMoonPhase(
  double moon_long,	/* Moon's ecliptic longitude (degrees) */
  double moon_lat,	/* Moon's ecliptic latitude (degrees)  */
  double sun_long)	/* Sun's longitude (degrees)           */
{
  double d;

  d = acos (cos (at_deg2Rad*(moon_long - sun_long)) * cos (at_deg2Rad*moon_lat));
  d = at_rad2Deg*(d);
  return ((1 + cos (at_deg2Rad*(180. - d - 0.1468 * sin (at_deg2Rad*d)))) / 2);
}

/*<AUTO EXTRACT>

  ROUTINE: atRiseSet 

  DESCRIPTION:
<HTML> 
 Returns the local time of rise or set for an object with the
 specified ra and dec, at a site with the specified latitide and longitude
 and time zone, for the given Julian epoch.  Rising and setting are defined
 with respect to the specified zenith distance.  Returns -1 if object
 doesn't rise or set (formula from Astronomical Formulae
 for Calculators, 3rd edition, Jean Meeus, p. 47 and
 Astronomical Almanac 1989, p. A12).
</HTML>

</AUTO>*/

double atRiseSet (
  double ra,		/* Right ascension (degrees)                   */
  double dec,		/* declination (degrees)                       */
  double longitude,	/* Earth west longitude (degrees)              */
  double latitude,	/* Earth latitude (degrees)                    */
  double epoch,		/* Julian epoch                                */ 
  int	 time_zone,	/* Time zone                                   */
  double zd,		/* Zenith distance defining rising and setting */
  int	 which)		/* Which phenomenom (RISE or SET)              */
{
  int	 year, month, day, j;
  double mjd, frac;
  double time, ha;


  /* Compute GMST at UT = 0 h */
  mjd = slaEpj2d(epoch);
  /* mjd = epoch; */
  slaDjcl(mjd, &year, &month, &day, &frac, &j);
  if (j != 0) {
    printf("Error %d in slaDjcl called from atRiseSet", j);
    return -1;
  }
  slaCldj(year, month, day, &mjd, &j);
  if (j != 0) {
    printf("Error %d in slaCldj called from atRiseSet", j);
    return -1;
  }
/* comment this out since it isn't used elsewhere and produces a compiler warning
  zepoch = slaEpj(mjd);
*/

  /* Compute the hour angle of rise/set */
  ha = atHourAngle (zd, dec, latitude);
  if (ha < 0)	/* No phenomenom */
    return (-1.);
  if (which == RISE)
    ha = - ha;

  /* Return the appropriate time */
  time = (ra - (360. - longitude) + ha)/15. - at_rad2Hrs*slaGmst(mjd) -
	       time_zone;
  if (time > 24.)
    time = fmod (time, 24.);
  while (time < 0.)
    time = time + 24.;
  return (time);
}

/*<AUTO EXTRACT>

  ROUTINE: atMjdRiseSet 

  DESCRIPTION:
<HTML> 
 Returns the local time of rise or set for an object with the
 specified ra and dec, at a site with the specified latitide and longitude
 and time zone, for the given MJD.  Rising and setting are defined
 with respect to the specified zenith distance.  Returns -1 if object
 doesn't rise or set (formula from Astronomical Formulae
 for Calculators, 3rd edition, Jean Meeus, p. 47 and
 Astronomical Almanac 1989, p. A12).  Default is to return time in
 MJD at Greenwich for night of int(mjd given).
</HTML>

</AUTO>*/

double atMjdRiseSet (
  double ra,		/* Right ascension (degrees)                   */
  double dec,		/* declination (degrees)                       */
  double longitude,	/* Earth west longitude (degrees)              */
  double latitude,	/* Earth latitude (degrees)                    */
  double mjd,		/* Modified Julian Date                        */ 
  int	 time_zone,	/* Time zone (always pos.                      */
  double zd,		/* Zenith distance defining rising and setting */
  int	 which)		/* Which phenomenom (RISE or SET)              */
{
  double time, ha;
  int intmjd;
  double midnight;
  double newtime;

  intmjd=(int)mjd; 
  /* Compute the hour angle of rise/set */
  ha = atHourAngle (zd, dec, latitude);
  if (ha < 0)	/* No phenomenom */
    return (-1.);
  if (which == RISE)
    ha = - ha;

  /* Return the appropriate time in MJD, timezone not important */
  /* time = (ra +longitude + ha)/15. - at_rad2Hrs*slaGmst(intmjd) - time_zone; */
  time = (ra +longitude + ha)/15. - at_rad2Hrs*slaGmst(intmjd);
  /* turn hours into days */
  time = time/24.;
  newtime = intmjd + time;

  midnight = intmjd + time_zone/24.;

  /* make sure time between noon and noon (standard time) for GMT */ 
  while ((midnight - newtime) > 0.5) {
    newtime+=1.;
  }
  while ((midnight - newtime) <= -0.5) {
    newtime+=-1.;
  }

  /* returns mjd for night of int(mjd given) */
  return (newtime);
}


/*<AUTO EXTRACT>

  ROUTINE: atHourAngle

  DESCRIPTION:

<HTML>
 Returns the hour angle (in degrees) for the specified
 declination, earth latitide, and zenith distance.  Returns -1 if
 no solution (object never reaches that hour angle).
 Formula from Astronomical Formulae for Calculators, 3rd edition,
 Jean Meeus, p. 47.
</HTML>

</AUTO>*/

double atHourAngle (
  double zd,		/* Zenith distance (degrees from pole) */
  double dec,		/* declination (degrees)               */
  double latitude)	/* Earth latitude (degrees)            */
{
  double ha, cosha;

  /* Compute the hour angle */
  cosha = (sin(at_deg2Rad*(90. - zd)) -
	   sin(at_deg2Rad*(latitude)) * sin(at_deg2Rad*(dec))) /
	  (cos(at_deg2Rad*(latitude)) * cos(at_deg2Rad*(dec)));
  if (cosha >= 1. || cosha <= -1.)
    return (-1.);
  ha = at_rad2Deg*acos(cosha);
  return (fabs(ha));
}

/*<AUTO EXTRACT>

  ROUTINE: atDateToEpoch

  DESCRIPTION:
<HTML>
 atDateToEpoch -- Convert Gregorian date and solar mean time to
 a Julian epoch.  A Julian epoch has 365.25 days per year and 24
 hours per day.
</HTML>
</AUTO>*/

void atDateToEpoch (
  int	year,			/* Year                                     */
  int	month,			/* Month (1-12)                             */
  int	day,			/* Day of month                             */
  double ut,			/* Universal time for date (mean solar day) */
  double* epoch)		/* Julian epoch                             */
{
  int	yr;
  double utwk;
  if (year < 100)
    yr = 1900 + year;
  else
    yr = year;

  utwk = (int) (ut * 360000. + 0.5) / 360000.;
  *epoch = yr + (atDayOfYear (yr, month, day)-1+utwk/24.) / 365.25;
}

/*<AUTO EXTRACT>

  ROUTINE: atEpochToDate

  DESCRIPTION:
<HTML>
 Convert a Julian epoch to year, month, day, and time.
</HTML>

</AUTO>*/

void atEpochToDate (
  double epoch,			/* Julian epoch            */
  int*	year,			/* Year                    */
  int*	month,			/* Month (1-12)            */
  int*	day,			/* Day of month            */
  double* ut)			/* Universal time for date */
{
  int	d;

  *year = (int) epoch;
  d = (int) ((epoch - *year) * 365.25);
  *ut = ((epoch - *year) * 365.25 - d) * 24.;
  *ut = (int) (*ut * 360000. + 0.5) / 360000.;
  if (*ut >= 24.) {
    d = d + 1;
    *ut = *ut - 24.;
  }

  d = d + 1;
  for (*month=1; d >= atDayOfYear (*year, *month+1, 1); *month=*month+1);
  *day = d - atDayOfYear (*year, *month, 1) + 1;
}

/*<AUTO EXTRACT>

  ROUTINE: atDayOfYear

  DESCRIPTION:
<HTML>
 atDayOfYear -- The day number for the given year is returned.
</HTML>

</AUTO>*/

int atDayOfYear (
  int	year,			/* Year         */
  int	month,			/* Month (1-12) */
  int	day)			/* Day of month */
{
  int	d;
  int	bom[13];
  bom[0] = 1;
  bom[1] = 32;
  bom[2] = 60;
  bom[3] = 91;
  bom[4] = 121;
  bom[5] = 152;
  bom[6] = 182;
  bom[7] = 213;
  bom[8] = 244;
  bom[9] = 274;
  bom[10] = 305;
  bom[11] = 335;
  bom[12] = 366;

  d = bom[month-1] + day - 1;
  if ((month > 2) && ((year%4) == 0) && ((year%400) != 0))
    d = d + 1;
  return (d);
}

/*<AUTO EXTRACT>

  ROUTINE: atJulDay

  DESCRIPTION:
<HTML>
 atJulday -- Convert epoch to Julian day. 
</HTML>
</AUTO>*/

double atJulday (double epoch)
{
  int   year, century;
  double jd;

  year = (int) epoch - 1;
  century = year / 100;
  jd = 1721425.5 + 365 * year - century + (int) (year / 4) + (int) (century / 4);
  jd = jd + (epoch - (int)epoch) * 365.25;
  return (jd);
}

/*<AUTO EXTRACT>

  ROUTINE: atMst

  DESCRIPTION:
<HTML>
 Mean sidereal time (hours) of the epoch at the given longitude.
 This procedure may be used to optain Greenwich Mean Sidereal Time (GMST)
 by setting the longitude to 0.
</HTML>
</AUTO>*/

double atMst (
  double epoch,		/* Epoch                */
  double longitude)	/* Longitude in degrees */
{
  double jd, ut, t, st;

  /* Determine JD and UT, and T (JD in centuries from J2000.0). */
  jd = atJulday (epoch);
  ut = (jd - (int) jd - 0.5) * 24.;
  t = (jd - 2451545.) / 36525.;

  /* The GMST at 0 UT in seconds is a power series in T. */
  st = 24110.54841 + t * (8640184.812866 + t * (0.093104 - t * 6.2e-6));

  /* Correct for longitude and convert to standard hours. */
  st = fmod (st / 3600. + ut - longitude/15.0, 24.0);

  if (st < 0)
    st = st + 24;

  return (st);
}

/*<AUTO EXTRACT>

  ROUTINE: atNextNewMoon

  DESCRIPTION: Return a time stamp of the next new moon
<HTML>
  Start at the time given by beginTime and step by deltaHours.
The returned TSTAMP is the time of 
the first minimum of the separation between the moon and Sun.
deltaHours defaults to 1 hour.

</HTML>
</AUTO>*/
TSTAMP *atNextNewMoon(
		      TSTAMP *beginTime,
		      float deltaHours
		      ) {
  TSTAMP *nnm;
  float rEarth[6], rMoon[6];
  int iy, id, j, i;
  float fd;
  float earthSum, moonSum, cosSum, daySum;
  float cosSumPrev = -1.1;
  int hasRisen = 0;

  int iterationNum = 0;

  nnm = atTstampNew(
		    beginTime->year,
		    beginTime->month,
		    beginTime->day,
		    beginTime->hour,
		    beginTime->minute,
		    beginTime->second);
  
  daySum = deltaHours/24.0;

  slaClyd(nnm->year, nnm->month, nnm->day, &iy, &id, &j);
  fd = (beginTime->hour + beginTime->minute/60.0 + 
        beginTime->second/3600)/24.0;

  while (daySum < 31) {
    slaEarth(iy, id, fd, rEarth);
    slaMoon(iy, id, fd, rMoon);

/* find cos of angle between Earth-Sun and Earth-Moon vectors */
    earthSum = moonSum = 0;
    for (i=0; i<3; i++) {
      rEarth[i] *= -1;
      earthSum += pow(rEarth[i],2.0);
      moonSum += pow(rMoon[i],2.0);
    }
    cosSum = 0;
    for (i=0; i<3; i++) {
      rEarth[i] /= sqrt(earthSum);
      rMoon[i] /= sqrt(moonSum);
      cosSum += rEarth[i]*rMoon[i];
    }

/* if angle starts to increase then stop */
    if ( (cosSum < cosSumPrev) && hasRisen) {
      atTstampIncr(nnm, -deltaHours*3600.0);
      break;
    }

/* set flag if angle is small and increasing */
    if ( (cosSum > cosSumPrev) && (cosSum > 0.995 ) ) {
/* don't set flag if are just after new moon */
      if ( iterationNum > 1 ) {
        hasRisen = 1;
      }
    }

    iterationNum++;
    cosSumPrev = cosSum;
    daySum += deltaHours/24.0;
    atTstampIncr(nnm, deltaHours*3600.0);
    slaClyd(nnm->year, nnm->month, nnm->day, &iy, &id, &j);
    fd = (nnm->hour + nnm->minute/60.0 + nnm->second/3600)/24.0;
  }
  return nnm;
}

/*<AUTO EXTRACT>

  ROUTINE: atNextFullMoon

  DESCRIPTION: Return a time stamp of the next full moon
<HTML>
  Start at the time given by beginTime and step by deltaHours.
The returned TSTAMP is the time of 
the first minimum of the separation between the moon and Sun.
deltaHours defaults to 1 hour.

</HTML>
</AUTO>*/
TSTAMP *atNextFullMoon(
		      TSTAMP *beginTime,
		      float deltaHours
		      ) {
  TSTAMP *nfm;
  float rEarth[6], rMoon[6];
  int iy, id, j, i;
  float fd;
  float earthSum, moonSum, cosSum, daySum;
  float cosSumPrev = 1.1;
  int hasRisen = 0;

  int iterationNum = 0;

  nfm = atTstampNew(
		    beginTime->year,
		    beginTime->month,
		    beginTime->day,
		    beginTime->hour,
		    beginTime->minute,
		    beginTime->second);
  
  daySum = deltaHours/24.0;

  slaClyd(nfm->year, nfm->month, nfm->day, &iy, &id, &j);
  fd = (beginTime->hour + beginTime->minute/60.0 + 
        beginTime->second/3600)/24.0;

  while (daySum < 31) {
    slaEarth(iy, id, fd, rEarth);
    slaMoon(iy, id, fd, rMoon);

/* find cos of angle between Earth-Sun and Earth-Moon vectors */
    earthSum = moonSum = 0;
    for (i=0; i<3; i++) {
      rEarth[i] *= -1;
      earthSum += (rEarth[i]*rEarth[i]);
      moonSum += (rMoon[i]*rMoon[i]);
    }
    cosSum = 0;
    for (i=0; i<3; i++) {
      rEarth[i] /= sqrt(earthSum);
      rMoon[i] /= sqrt(moonSum);
      cosSum += rEarth[i]*rMoon[i];
    }

/* if angle starts to increase then stop */
    if ( (cosSum < cosSumPrev) && hasRisen) {
      atTstampIncr(nfm, -deltaHours*3600.0);
      break;
    }

/* set flag if angle is near 180 and increasing (cosSum decreasing)*/
    if ( (cosSum < cosSumPrev) && (cosSum < -0.995 ) ) {
/* don't set flag if are just after new moon */
      if ( iterationNum > 1 ) {
        hasRisen = 1;
      }
    }

    iterationNum++;
    cosSumPrev = cosSum;
    daySum += deltaHours/24.0;
    atTstampIncr(nfm, deltaHours*3600.0);
    slaClyd(nfm->year, nfm->month, nfm->day, &iy, &id, &j);
    fd = (nfm->hour + nfm->minute/60.0 + nfm->second/3600)/24.0;
  }
  return nfm;
}











