#ifndef ATEPHEMERIS_H
#define ATEPHEMERIS_H

#include <math.h>
#include <ctype.h>
#include <string.h>
#include "atSlalib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* RISE_SET procedure options */
#define	RISE		1
#define	SET		2

/* Function prototypes: */

double atMoonPhase(
  double moon_long,	/* Moon's ecliptic longitude (degrees) */
  double moon_lat,	/* Moon's ecliptic latitude (degrees)  */
  double sun_long);	/* Sun's longitude (degrees)           */

double atRiseSet (
  double ra,		/* Right ascension (degrees)                   */
  double dec,		/* declination (degrees)                       */
  double longitude,	/* Earth west longitude (degrees)              */
  double latitude,	/* Earth latitude (degrees)                    */
  double epoch,		/* Julian epoch                                */ 
  int	 time_zone,	/* Time zone                                   */
  double zd,		/* Zenith distance defining rising and setting */
  int	 which);	/* Which phenomenom (RISE or SET)              */

double atMjdRiseSet (
  double ra,		/* Right ascension (degrees)                   */
  double dec,		/* declination (degrees)                       */
  double longitude,	/* Earth west longitude (degrees)              */
  double latitude,	/* Earth latitude (degrees)                    */
  double mjd,		/* Modified Julian date                        */ 
  int	 time_zone,	/* Time zone                                   */
  double zd,		/* Zenith distance defining rising and setting */
  int	 which);	/* Which phenomenom (RISE or SET)              */

double atHourAngle (
  double zd,		/* Zenith distance (degrees from pole) */
  double dec,		/* declination (degrees)               */
  double latitude);	/* Earth latitude (degrees)            */

TSTAMP *atNextNewMoon(
		      TSTAMP *beginTime,
		      float deltaHours
		      );

TSTAMP *atNextFullMoon(
		      TSTAMP *beginTime,
		      float deltaHours
		      );

int atDayOfYear (
		 int	year,			/* Year         */
		 int	month,			/* Month (1-12) */
		 int	day			/* Day of month */
		 );

#ifdef __cplusplus
}
#endif

#endif
