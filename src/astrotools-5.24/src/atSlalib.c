/* <AUTO>
   FILE: atSlalib
<HTML>
   These wrappers convert common structs in our tools (like TRANS and 
      ARRAY structs) to forms that the SLALIB package can understand.
</HTML>
   </AUTO>
*/
/*
   atDMSToDeg	        given deg:min:sec string, return degrees
   atDayFromTime        given HH:MM:SS string, return the day.
   atDegToDMS	        given degrees, return deg:min:sec string
   atDegToHMS           given degrees, return HH:MM:SS string
   atHMSToDeg	        given HH:MM:SS string, return degrees
   atPlanetRaDec
   atPlanetRiseSet
   atRadFromDeg     	given degrees, return radian equivalent.
   atRadFromDMS         given deg:min:sec string, return radians.
   atRadFromHMS         given hh:min:sec string, return radians.
   atRadToDeg	        given radians, return degree equivalent.
   atRadToDMS           given radians, return deg:min:sec string.
   atRadToHMS           given radians, return hh:min:sec string.
   atTstampCopy
   atTstampDel
   atTstampIncr
   atTstampNew
   atTstampNow
   atTstampToDate
   atTstampToLst
   atTstampToMJD
   atTstampToTime
   atTstampToTwilight
   atVFitxy
   atVS2tp
   atVTp2s
   atVTrans             tranform the vxm,vym pairs using TRANS
   atAberrationGet      return apparent alpha,delta given actual alpha,delta
				*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include "dervish.h"
#include "smaValues.h"
#include "slalib.h"
#include "atSlalib.h"
#include "atSlalib_c.h"
#include "atConversions.h"
#include "atSurveyGeometry.h"
#include "atEphemeris.h"

/**************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atRadToDeg

  DESCRIPTION:
<HTML>
	Converts radians to degrees
</HTML>

  RETURN VALUES:
<HTML>
	Returns degrees
</HTML>
</AUTO>*/
double atRadToDeg(double radians) {
  return (radians*at_rad2Deg);
}

/**************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atRadFromDeg

  DESCRIPTION:
<HTML>
	Convergs degrees to radians
</HMTL>

  RETURN VALUES:
<HTML>
	Returns radians
</HTML>
</AUTO>*/
double atRadFromDeg(double degrees) {
  return (degrees*at_deg2Rad);
}


/**************************************************************/
/**** Spherical to Tangential Plane: vector */

/*<AUTO EXTRACT>

  ROUTINE: atVS2tp  

  DESCRIPTION:
<HTML>
        All angles must be in decimal degrees.  The tangent
	plane coordinates are also returned in degrees.

  Calls SLA_DS2TP.
</HTML>

  RETURN VALUES:
<HTML>
   Returns either SH_SUCCESS or SH_GENERIC_ERROR
</HTML>
</AUTO>*/
RET_CODE atVS2tp(VECTOR* ra, VECTOR* dec, double raz, double decz)
{
  
  double ra_tmp, dec_tmp, xi =0., eta =0.;
  int i, nvalue, status = 0;
  
  if(ra == NULL || dec ==NULL)
    {
      shErrStackPush("atVS2tp: ra and dec vectors are null");
      return SH_GENERIC_ERROR;
    }
  
  nvalue = ra->dimen;
  if(nvalue!=dec->dimen) {
    shErrStackPush("atVS2tp: VECTORs don't have same number of elems");
    return SH_GENERIC_ERROR;
  }
  
  /* now transform */
  
  for(i=0; i < nvalue; i++) {
    ra_tmp = at_deg2Rad*ra->vec[i];
    dec_tmp = at_deg2Rad*dec->vec[i];
    slaDs2tp(ra_tmp, dec_tmp, raz*at_deg2Rad, decz*at_deg2Rad, &xi, &eta, &status);
    xi = at_rad2Deg*xi;
    eta = at_rad2Deg*eta;
    switch(status) {
    case 0: ra->vec[i] = xi; dec->vec[i] = eta; break;
    case 1: shErrStackPush("star too far from axis : %d", i); break;
    case 2: shErrStackPush("antistar too far from axis: %d", i); break;
    case 3: shErrStackPush("antistar on tangent plane: %d", i); break;
    default: shErrStackPush("Unknown status returned by slaDs2tp");
    }
    if(status!=0) return SH_GENERIC_ERROR;
    ra->vec[i] = xi;
    dec->vec[i] = eta;       
  }
  
  return SH_SUCCESS;
}

/**************************************************************/
/**** Tangential Plane to Spherical: vector */

/*<AUTO EXTRACT>

  ROUTINE: atVTp2s 

  DESCRIPTION:
<HTML>
        All angles must be in decimal degrees, including the tangent
	plane coordinates.  Spherical coordinates are returned in
	degrees

  Calls SLA_DTP2S.
</HTML>

  RETURN VALUES:
<HTML>
   Returns either SH_SUCCESS or SH_GENERIC_ERROR
</HTML>
</AUTO>*/
RET_CODE atVTp2s(VECTOR* ra, VECTOR* dec, double raz, double decz)
{
  
  double xi_tmp, eta_tmp, sra =0., sdec =0.;
  int i, nvalue;
  
  if(ra == NULL || dec ==NULL)
    {
      shErrStackPush("atVTp2s: ra and dec vectors are null");
      return SH_GENERIC_ERROR;
    }
  
  nvalue = ra->dimen;
  if(nvalue!=dec->dimen) {
    shErrStackPush("atVTp2s: VECTORs don't have same number of elems");
    return SH_GENERIC_ERROR;
  }
  
  /* now transform */
  
  for(i=0; i < nvalue; i++) {
    xi_tmp = at_deg2Rad*ra->vec[i];
    eta_tmp = at_deg2Rad*dec->vec[i];
    slaDtp2s(xi_tmp, eta_tmp, raz*at_deg2Rad, decz*at_deg2Rad, &sra, &sdec);
    sra = at_rad2Deg*sra;
    sdec = at_rad2Deg*sdec;
    ra->vec[i] = sra;
    dec->vec[i] = sdec;       
  }
  
  return SH_SUCCESS;
}

/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atDayFromTime

  DESCRIPTION:
<HTML>
  Calls SLA_CTF2D.  Converts a time string (hh:mm:ss) to decimal days.
</HTML>

  RETURN VALUES:
<HTML>
   Returns the time in days, or -1 if an error occurred.
</HTML>
</AUTO>*/
RET_CODE atDayFromTime(char *string, float *days) {
  int nstrt;
  long hour, minute;
  float second;
  int hourFlag, minuteFlag, secondFlag;
  int jflag;

  nstrt = 1;
  slaIntin(string, &nstrt, &hour, &hourFlag);
  nstrt++;
  slaIntin(string, &nstrt, &minute, &minuteFlag);
  nstrt++;
  slaFlotin(string, &nstrt, &second, &secondFlag);

  if ( (hourFlag==0) && (minuteFlag==0) && (secondFlag==0) ) {
    slaCtf2d((int)hour, (int)minute, second, days, &jflag);
  } else {
    *days = -1.0;
    return SH_GENERIC_ERROR;
  }
  if (jflag!=0) return SH_GENERIC_ERROR;
  return SH_SUCCESS;
}
/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atRadFromDMS

  DESCRIPTION:
<HTML>
  Calls SLA_DAF2R.  Converts an angle string (dd:mm:ss) to decimal radians.
</HTML>

  RETURN VALUES:
<HTML>
   Returns the angle in radians, or -10 if an error occurred.
</HTML>
</AUTO>*/
double atRadFromDMS(char *string) {
  int nstrt;
  long deg, min;
  double sec, radian;
  int degFlag, minFlag, secFlag, jflag;
  int sign;
  nstrt = 1; slaIntin(string, &nstrt, &deg, &degFlag);
  nstrt++; slaIntin(string, &nstrt, &min, &minFlag);
  nstrt++; slaDfltin(string, &nstrt, &sec, &secFlag);

  /* Deal with the sign */
  if (strncmp(string,"-",1)==0) {
    sign = -1;
    deg = -deg;
  } else {
    sign = 1;
  }
  if ( (degFlag<=0) && (minFlag<=0) && (secFlag<=0) ) {
    slaDaf2r(deg, min, sec, &radian, &jflag);
    if (sign == -1) {radian = -radian;}
  } else {
    radian = -10.0;
  }
  return radian;
}
/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atRadFromHMS

  DESCRIPTION:
<HTML>
  Calls SLA_DTF2R.  Converts an angle string (hh:mm:ss) to decimal radians.
</HTML>

  RETURN VALUES:
<HTML>
   Returns the angle in radians, or -10 if an error occurred.
</HTML>
</AUTO>*/
double atRadFromHMS(char *string) {
  int nstrt, hour = 0, min = 0;
  double sec, radian;
  int hourFlag, minFlag, secFlag, jflag;

  nstrt = 1; slaIntin(string, &nstrt, (long*) &hour, &hourFlag);
  nstrt++;   slaIntin(string, &nstrt, (long*) &min, &minFlag);
  nstrt++;   slaDfltin(string, &nstrt, &sec, &secFlag);

  if ( (hourFlag<=0) && (minFlag<=0) && (secFlag<=0) && (strncmp(string,"-",1)!=0) ) {
    slaDtf2r(hour, min, sec, &radian, &jflag);
  } else {
    radian = -10.0;
  }
  return radian;
}
/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atRadToDMS

  DESCRIPTION:
<HTML>
  Calls SLA_DR2AF.  Converts an angle in decimal radians to a string (dd:mm:ss).
  The string must already be allocated.
</HTML>

  RETURN VALUES:
<HTML>
   Returns TCL_OK
</HTML>
</AUTO>*/
int atRadToDMS(double angle, char *string) {
  int DMS[4];
  char sign[2];
  int ndp;
  char temp[20];
  ndp = 9;
  string[0] = '\0';
  slaDr2af(ndp,angle,sign,DMS);
  sign[1] = '\0';
  strcat(string,sign);
  sprintf(temp,"%02d:",DMS[0]);
  strcat(string,temp);
  sprintf(temp,"%02d:",DMS[1]);
  strcat(string,temp);
  sprintf(temp,"%02d.",DMS[2]);
  strcat(string,temp);
  sprintf(temp,"%0*d",ndp,DMS[3]);
  strcat(string,temp);
  return TCL_OK;
}
/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atRadToHMS

  DESCRIPTION:
<HTML>
  Calls SLA_DR2TF.  Converts an angle in decimal radians to a string (hh:mm:ss).
  The string must already be allocated.
</HTML>

  RETURN VALUES:
<HTML>
   Returns TCL_OK
</HTML>
</AUTO>*/
int atRadToHMS(double angle, char *string) {
  int HMS[4];
  char sign[2];
  int ndp;
  char temp[20];
  ndp = 9;
  string[0] = '\0';
  if (angle<0) {angle = angle +2*M_PI;}
  slaDr2tf(ndp,angle,sign,HMS);
  sign[1] = '\0';
  strcat(string,sign);
  sprintf(temp,"%02d:",HMS[0]);
  strcat(string,temp);
  sprintf(temp,"%02d:",HMS[1]);
  strcat(string,temp);
  sprintf(temp,"%02d.",HMS[2]);
  strcat(string,temp);
  sprintf(temp,"%0*d",ndp,HMS[3]);
  strcat(string,temp);
  return TCL_OK;
}

/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atDMSToDeg

  DESCRIPTION:
<HTML>
  Calls SLA_DAF2R.  Converts an angle string (dd:mm:ss) to decimal degrees.
</HTML>

  RETURN VALUES:
<HTML>
   Returns the angle in degrees, or -1000 if an error occurred.
</HTML>
</AUTO>*/
double atDMSToDeg(char *string) {
  int nstrt;
  long deg, min;
  double sec, radian, degrees;
  int degFlag, minFlag, secFlag, jflag;
  int sign;
  nstrt = 1; slaIntin(string, &nstrt, &deg, &degFlag);
  nstrt++; slaIntin(string, &nstrt, &min, &minFlag);
  nstrt++; slaDfltin(string, &nstrt, &sec, &secFlag);

  /* Deal with the sign */
  if (strncmp(string,"-",1)==0) {
    sign = -1;
    deg = -deg;
  } else {
    sign = 1;
  }
  if ( (degFlag<=0) && (minFlag<=0) && (secFlag<=0) ) {
    slaDaf2r(deg, min, sec, &radian, &jflag);
    if (sign == -1) {radian = -radian;}
    degrees = radian*at_rad2Deg;
  } else {
    degrees = -1000.0;
  }
  return degrees;
}
/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atHMSToDeg

  DESCRIPTION:
<HTML>
  Calls SLA_DTF2R.  Converts an angle string (hh:mm:ss) to decimal degrees.
</HTML>

  RETURN VALUES:
<HTML>
   Returns the angle in degrees, or -1000 if an error occurred.
</HTML>
</AUTO>*/
double atHMSToDeg(char *string) {
  int nstrt, hour = 0, min = 0;
  double sec, radian, degrees;
  int hourFlag, minFlag, secFlag, jflag;

  nstrt = 1; slaIntin(string, &nstrt, (long*) &hour, &hourFlag);
  nstrt++;   slaIntin(string, &nstrt, (long*) &min, &minFlag);
  nstrt++;   slaDfltin(string, &nstrt, &sec, &secFlag);

  if ( (hourFlag<=0) && (minFlag<=0) && (secFlag<=0) && (strncmp(string,"-",1)!=0) ) {
    slaDtf2r(hour, min, sec, &radian, &jflag);
    degrees = radian*at_rad2Deg;
  } else {
    degrees = -1000.0;
  }
  return degrees;
}
/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atDegToDMS

  DESCRIPTION:
<HTML>
  Calls SLA_DR2AF.  Converts an angle in decimal degrees to a string (dd:mm:ss).
  The string must already be allocated.
</HTML>

  RETURN VALUES:
<HTML>
   Returns TCL_OK
</HTML>
</AUTO>*/
int atDegToDMS(double angle, char *string) {
  int DMS[4];
  double radians;
  char sign[2];
  int ndp;
  char temp[20];
  ndp = 9;
  string[0] = '\0';
  radians = angle*at_deg2Rad;
  slaDr2af(ndp,radians,sign,DMS);
  sign[1] = '\0';
  strcat(string,sign);
  sprintf(temp,"%02d:",DMS[0]);
  strcat(string,temp);
  sprintf(temp,"%02d:",DMS[1]);
  strcat(string,temp);
  sprintf(temp,"%02d.",DMS[2]);
  strcat(string,temp);
  sprintf(temp,"%0*d",ndp,DMS[3]);
  strcat(string,temp);
  return TCL_OK;
}
/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atDegToHMS

  DESCRIPTION:
<HTML>
  Calls SLA_DR2TF.  Converts an angle in decimal degrees to a string (hh:mm:ss).
  The string must already be allocated.
</HTML>

  RETURN VALUES:
<HTML>
   Returns TCL_OK
</HTML>
</AUTO>*/
int atDegToHMS(double angle, char *string) {
  double radian;
  int HMS[4];
  char sign[2];
  int ndp;
  char temp[20];
  ndp = 9;
  string[0] = '\0';
  radian = angle*at_deg2Rad;
  if (radian<0) {radian = radian +2*M_PI;}
  slaDr2tf(ndp,radian,sign,HMS);
  sign[1] = '\0';
  strcat(string,sign);
  sprintf(temp,"%02d:",HMS[0]);
  strcat(string,temp);
  sprintf(temp,"%02d:",HMS[1]);
  strcat(string,temp);
  sprintf(temp,"%02d.",HMS[2]);
  strcat(string,temp);
  sprintf(temp,"%0*d",ndp,HMS[3]);
  strcat(string,temp);
  return TCL_OK;
}

/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atTstampNew

  DESCRIPTION:
<HTML>
Make a new TSTAMP structure with the given values
</HTML>

  RETURN VALUES:
<HTML>
   address of the new TSTAMP
</HTML>
</AUTO>*/
TSTAMP *atTstampNew(
		    int year,	/* year including century (1995, NOT 95) */
		    int month,	/* month (1 through 12) */
		    int day,	/* day (1 through 31) */
		    int hour,	/* hour (0 through 23) */
		    int minute, /* minute (0 through 59) */
		    double second /* second (0.0 through 59.999999 */
		    ) {
  TSTAMP *tstamp = (TSTAMP *) shMalloc(sizeof(TSTAMP));
  tstamp->year = year;
  tstamp->month = month;
  tstamp->day = day;
  tstamp->hour = hour;
  tstamp->minute = minute;
  tstamp->second = second;
  return tstamp;
}
/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atTstampNow

  DESCRIPTION:
<HTML>
Make a new TSTAMP structure with current gm time or tai time.
</HTML>

  RETURN VALUES:
<HTML>
   address of the new TSTAMP
</HTML>
</AUTO>*/
TSTAMP *atTstampNow(
		    char *timetype  /* UT (default) or TAI*/
		    ) {

  TSTAMP *tstamp = NULL;
  time_t *tloc=NULL;
  time_t tret;
  struct tm *gmt;
  double djm, days, julian, leapsec;
  int j;

  tret = time(tloc);
  gmt = gmtime(&tret);
  gmt->tm_year += 1900;

/* Bug discovered by S. Kent, 1/13/2001: gmt->tm_mon has range 0-11,
 * whereas slalib needs range 1-12.  Whowever decided that the Unix month
 * range should be 0 based (unlike the other time fields) should be
 * shot AND sent to the Russian front.
*/

  gmt->tm_mon++;

  if (strcmp(timetype,"TAI") == 0) {
    slaCldj(gmt->tm_year, gmt->tm_mon, gmt->tm_mday, &djm, &j);
    slaDtf2d(gmt->tm_hour, gmt->tm_min, gmt->tm_sec, &days, &j);
    julian = djm + days;
    leapsec = slaDat(julian);
    printf("leap seconds = %g\n", leapsec);

/* The following logic too flawed to be useful.  Instead, I will recomput
 * gmt from scratch.
    gmt->tm_sec += (int)leapsec;
    if (gmt->tm_sec >= 60) {
      gmt->tm_sec -= 60;
      gmt->tm_min += 1;
	if (gmt->tm_min >=60) {
      		gmt->tm_min -= 60;
	       gmt->tm_hour += 1;
	}
    }
*/
      tret+= (int)leapsec;
      gmt = gmtime(&tret);
      gmt->tm_year += 1900;
      gmt->tm_mon++;
  }
  tstamp = atTstampNew(
		       gmt->tm_year,
/*		       1+gmt->tm_mon,  */
		       gmt->tm_mon,
		       gmt->tm_mday,
		       gmt->tm_hour,
		       gmt->tm_min,
		       gmt->tm_sec
		       );
  return tstamp;
}

/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atTstampDel

  DESCRIPTION:
<HTML>
Delete a TSTAMP structure.
</HTML>

  RETURN VALUES:
<HTML>
   Always returns SH_SUCCESS
</HTML>
</AUTO>*/
RET_CODE atTstampDel(TSTAMP *tstamp) {
  shFree(tstamp);
  return SH_SUCCESS;
}
/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atTstampToMJD

  DESCRIPTION:
<HTML>
Return the Modified Julian Date of the TSTAMP.
</HTML>

  RETURN VALUES:
<HTML>
The Julian Date
</HTML>
</AUTO>*/
double atTstampToMJD(
		     TSTAMP *tstamp
		     ) {
  double djm, days, julian;
  int j;
  slaCldj(tstamp->year, tstamp->month, tstamp->day, &djm, &j);
  slaDtf2d(tstamp->hour, tstamp->minute, tstamp->second, &days, &j);
  julian = djm + days;
  return julian;
}
/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atTstampFromMJD

  DESCRIPTION:
<HTML>
Return a TSTAMP equivalent to the Modified Julian Date.
</HTML>

  RETURN VALUES:
<HTML>
Address of a TSTAMP
</HTML>
</AUTO>*/
TSTAMP *atTstampFromMJD(
			double mjd /* the Modified Julian Date */
			) {
  int year, month, day, hour, minute, j;
  int ihmsf[4];
  int ndp = 9;
  char sign;
  double fraction, second;
  TSTAMP *tstamp;
  slaDjcl(mjd, &year, &month, &day, &fraction, &j);
  slaDd2tf(ndp, fraction, &sign, ihmsf);
  
  hour = ihmsf[0];
  minute = ihmsf[1];
  second = ihmsf[2] + (double) ihmsf[3] / (double) pow(10.0, (double)ndp);

  tstamp = atTstampNew(year, month, day, hour, minute, second);
    
  return tstamp;
}
/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atTstampIncr

  DESCRIPTION:
<HTML>
Increment the TSTAMP by seconds
</HTML>

  RETURN VALUES:
<HTML>
SH_SUCCESS
</HTML>
</AUTO>*/
RET_CODE atTstampIncr(
		       TSTAMP *tstamp, /* the TSTAMP to increment */
		       double seconds /* number of seconds to increment */
		       ) {
  double julian;
  TSTAMP *temp;
  julian = atTstampToMJD(tstamp);
  julian += seconds / (3600.0 * 24.0);
  temp = atTstampFromMJD(julian);
  atTstampCopy(tstamp, temp);
  atTstampDel(temp);
  return SH_SUCCESS;
}
/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atTstampCopy

  DESCRIPTION:
<HTML>
Copy the values from tstampSource to tstampDestination
</HTML>

  RETURN VALUES:
<HTML>
SH_SUCCESS
</HTML>
</AUTO>*/
RET_CODE atTstampCopy(
		      TSTAMP *tstampDestination, 
		      TSTAMP *tstampSource
		      ) {
  tstampDestination->year = tstampSource->year;
  tstampDestination->month = tstampSource->month;
  tstampDestination->day = tstampSource->day;
  tstampDestination->hour = tstampSource->hour;
  tstampDestination->minute = tstampSource->minute;
  tstampDestination->second = tstampSource->second;

  return SH_SUCCESS;
}

/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atTstampToDate

  DESCRIPTION:
<HTML>
Return the formatted date in the string.  Note that the address must
have at least 11 char's allocated before calling this function.

</HTML>

  RETURN VALUES:
<HTML>
SH_SUCCESS
</HTML>
</AUTO>*/
RET_CODE atTstampToDate(
			TSTAMP *tstamp, /* the TSTAMP */
			char *date /* char address at least 11 long */
			) {
  sprintf(date,"%04d/%02d/%02d",tstamp->year, tstamp->month, tstamp->day);
  return SH_SUCCESS;
}

/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atTstampToTime

  DESCRIPTION:
<HTML>
Return the formatted time in the string.  Note that the address must
have at least 14 char's allocated before calling this function.

</HTML>

  RETURN VALUES:
<HTML>
SH_SUCCESS
</HTML>
</AUTO>*/
RET_CODE atTstampToTime(
			TSTAMP *tstamp, /* the TSTAMP */
			char *date /* char address at least 14 long */
			) {
  /* Original code is what is in the "else" half.
   * The sprintf to %07.4f unknowningly rounds up seconds of 59.9999 to 60.0000, thus the kludge.
   * Fix for PR 6180
   */
  if (tstamp->second >= 59.9999) {
    sprintf(date,"%02d:%02d:59.9999",
	    tstamp->hour, tstamp->minute);
  } else {
    sprintf(date,"%02d:%02d:%07.4f",
	  tstamp->hour, tstamp->minute, tstamp->second);
  }
  return SH_SUCCESS;
}
/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atTstampToLst

  DESCRIPTION:
<HTML>
Return the Local Sidereal Time in decimal degrees for an observatory
at an arbitrary WEST longitude
</HTML>

  RETURN VALUES:
<HTML>
the LST in degrees as a double
</HTML>
</AUTO>*/
double atTstampToLst(
		     TSTAMP *tstamp, /* the TSTAMP */
                     double theLongitude, /* at_site_longitude, for exammple.
					  (degrees) */
		     char *timetype /* UT (default) or TAI*/
		     ) {

  int stat;
  double mjd, gmst, lst, days;
  
  slaCldj (tstamp->year, tstamp->month, tstamp->day, 
	   &mjd, &stat);   /* get Modified Julian Date */
  if (stat != 0) {
    printf("slaCldj returned status %d\n", stat);
  }
  slaDtf2d(tstamp->hour, tstamp->minute, tstamp->second, &days, &stat);
  mjd += days;
  if (strcmp(timetype,"TAI")==0) {

  /* correct for difference between TAI and UTC, slaDat gives diff in seconds,
   **    so divide by number of seconds in a day.
   **
   ** slaDat goes out of date whenever leapseconds are added, so
   ** this module should be rebuilt against new version of SLALIB
   ** whenever available.
   **
   ** Technically slaDat should be called with the UTC ModJulDate, it gives
   **    the increment to be added to UTC to get TAI
   ** Going backwards here, shouldn't matter for virtually any application
   */
     mjd -= slaDat(mjd)/24./3600.;
  } 

 
  
  /* Fixed on May 8, 1998 at Jeff Pier's suggestion */
  /* gmst = slaGmst(mjd) * at_rad2Deg; */
  gmst = (slaGmst(mjd) + slaEqeqx(mjd)) * at_rad2Deg;

  /* Use the minus sign here since it is a WEST longitude */
  lst = gmst - theLongitude; 
  if (lst <    0.) lst= lst + 360.;
  if (lst >= 360.) lst= lst - 360.;
  
  return lst;
  
}

/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atTstampToTwilight

  DESCRIPTION:
<HTML>
Step  backward from the given tstamp to find the time 
of the sunrise that started this night and forward to find
the time that will end this night.

If you get a short duration you input a tstamp when the sun 
was already up.
</HTML>

  RETURN VALUES:
  The number of hours in this night
<HTML>
SH_SUCCESS
</HTML>
</AUTO>*/
double atTstampToTwilight(
			  TSTAMP *tstampNow,    /* IN: any tstamp in night */
			  double degrees, /* number of degrees below horizon for darkness */
			  TSTAMP *tstampSunset, /* OUT: tstamp of sunset */
			  TSTAMP *tstampSunrise /* OUT: tstamp of sunrise */
			  ) {
  double dvb[3], dpb[3], dvh[3], dph[3];
  double ra, dec;
  double slong, slat;
  double az, el, pa;
  double deqx = 2000.0;
  double date;
  double hours;

  /* Step forward until sun has elevation > -degrees */
  atTstampCopy(tstampSunrise, tstampNow);
  el = -90;
  while (el < -degrees) {
    atTstampIncr(tstampSunrise, 60.0);
    date = atTstampToMJD(tstampSunrise);
    slaEvp(date, deqx, dvb, dpb, dvh, dph);
    dph[0] *= -1.0;
    dph[1] *= -1.0;
    dph[2] *= -1.0;
    slaDcc2s(dph, &ra, &dec);
    ra *= at_rad2Deg;
    dec *= at_rad2Deg;
    atEqToSurvey(ra, dec, &slong, &slat);
    atSurveyToAzelpa(slong, slat, tstampSunrise, &az, &el, &pa);
  }

  /* Step backward until sun has elevation > -degrees */
  atTstampCopy(tstampSunset, tstampNow);
  el = -90;
  while (el < -degrees) {
    atTstampIncr(tstampSunset, -60.0);
    date = atTstampToMJD(tstampSunset);
    slaEvp(date, deqx, dvb, dpb, dvh, dph);
    dph[0] *= -1.0;
    dph[1] *= -1.0;
    dph[2] *= -1.0;
    slaDcc2s(dph, &ra, &dec);
    ra *= at_rad2Deg;
    dec *= at_rad2Deg;
    atEqToSurvey(ra, dec, &slong, &slat);
    atSurveyToAzelpa(slong, slat, tstampSunset, &az, &el, &pa);
  }
  hours = 24.0*(atTstampToMJD(tstampSunrise) - atTstampToMJD(tstampSunset));
  return hours;
}

/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atPlanetRaDec

  DESCRIPTION: <HTML> 
  Determines the Ra and Dec of the Sun, Moon, or Planet at a particular
  time and date using slaRdplan, which gives positions for MJD+1.  To
  find times for a given night, give correct GMT int(MJD) for APO after
  1am on said night.

</HTML>

  RETURN VALUES:
  None

<HTML>
SH_SUCCESS
</HTML>
</AUTO>*/
void atPlanetRaDec(
		     TSTAMP *tstampNow,    /* IN: any tstamp in night */
		     int np, /* IN: number that indicates which planet Moon=3, Sun=10 */
		     double longitude,  /* IN: longitude of obs. (west) */
		     double lat,   /* IN: latitude of obs. */
		     double *ra, /* OUT: ra of planet */
		     double *dec /* OUT: dec of planet */
		     ) {
  double diam;
  double elong;
  double date;
  char obj[10];

  if (np==3) {
    (void)strcpy(obj,"Moon");
  } else if (np==10) {
    (void)strcpy(obj,"Sun");
  } else {
    (void)strcpy(obj,"Planet");
  }

  /* convert tstamp date */
  date = atTstampToMJD(tstampNow);
  elong = (360.-longitude);

  /* Find ra, dec of planet */
  slaRdplan(date, np, elong*at_deg2Rad, lat*at_deg2Rad, ra, dec, &diam);

  *ra = *ra/at_deg2Rad;
  *dec = *dec/at_deg2Rad;

  /* printf("%s initial ra %f dec %f\n",obj,*ra,*dec); */
  
}

/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atPlanetRiseSet

  DESCRIPTION: <HTML> 
  Determines the Ra and Dec of the Sun, Moon, or Planet at a particular
  time and date using slaRdplan, which gives positions for MJD+1.  To
  find times for a given night, give correct GMT int(MJD) for APO after
  1am on said night.  From RA, DEC it figures out when that object will
  rise or set that night, given a certain definition for the horizon,
  using atMjdRiseSet.  Time is given in hours from GMT.

</HTML>

  RETURN VALUES:
  None

<HTML>
SH_SUCCESS
</HTML>
</AUTO>*/
void atPlanetRiseSet(
		     TSTAMP *tstampNow,    /* IN: any tstamp in night */
		     TSTAMP *tstampSet,    /* OUT: tstamp of set */
		     TSTAMP *tstampRise,   /* OUT: tstamp of rise */
		     int np, /* IN: number that indicates which planet Moon=3, Sun=10 */
		     double longitude,  /* IN: longitude of obs. (west) */
		     double lat,   /* IN: latitude of obs. */
		     int tz,  /* IN: time zone, 0==GMT, always pos.*/
		     double zd /* IN: distance from zenith in degrees that defs horizon */
		     ) {
  double ra, dec, diam;
  double elong;
  double date;
  double rmjd, smjd;
  double nrmjd, nsmjd;
  int intmjd;
  double rra, rdec;
  double sra, sdec;
  double rtime,stime;
  TSTAMP *tstampTemp;

  /* convert tstamp date */
  date = atTstampToMJD(tstampNow);
  elong = (360.-longitude);

  /* Force date to local midnight */
  intmjd = (int)date;
  date = intmjd + tz/24.;
  
  /* Find ra, dec of planet */
  slaRdplan(date, np, elong*at_deg2Rad, lat*at_deg2Rad, &ra, &dec, &diam);

  /* Calculate rise time.  GMT if time_zone=0 */
  rmjd = atMjdRiseSet(ra*at_rad2Deg, dec*at_rad2Deg, longitude, lat, date, tz, zd, RISE);

  if (np!=3) {
    rtime = ((rmjd - (int)rmjd)*24. - tz);
    if (rtime < 0) {rtime+=24.;}
  }

  tstampTemp = atTstampFromMJD(rmjd);
  atTstampCopy(tstampRise,tstampTemp);
  atTstampDel(tstampTemp);
  
  /* Calculate set time.  GMT if time_zone=0 */
  smjd = atMjdRiseSet(ra*at_rad2Deg, dec*at_rad2Deg, longitude, lat, date, tz, zd, SET);  
  if (np!=3) {
    stime = ((smjd - (int)smjd)*24. - tz);
    if (stime<0) {stime+=24.;}
  }

  tstampTemp = atTstampFromMJD(smjd);
  atTstampCopy(tstampSet,tstampTemp);
  atTstampDel(tstampTemp);

  if (np==3) {

    /* Because the moon moves a significant amount in a night, first
       determine the approx times, then plug in these times and redo 
       first the positions, and then the times. */
      
    /* This is a bit of a hack */
    /* If first iteration is before 00 GMT mjdRiseSet 
       will chop off fraction, and day will be wrong */
    if (rmjd < intmjd) {
      rmjd = intmjd;
    }
    if (smjd < intmjd) {
      smjd = intmjd;
    }

    /* Calculate rise time.  GMT if time_zone=0 */
    slaRdplan(rmjd, np, elong*at_deg2Rad, lat*at_deg2Rad, &rra, &rdec, &diam);
    nrmjd = atMjdRiseSet(rra*at_rad2Deg, rdec*at_rad2Deg, longitude, lat, rmjd, tz, zd, RISE);
    rtime = ((nrmjd - (int)nrmjd)*24. - tz);
    if (rtime<0) {rtime+=24.;}

    tstampTemp = atTstampFromMJD(nrmjd);
    atTstampCopy(tstampRise,tstampTemp);
    atTstampDel(tstampTemp);

    /* Calculate set time.  GMT if time_zone=0 */
    slaRdplan(smjd, np, elong*at_deg2Rad, lat*at_deg2Rad, &sra, &sdec, &diam);
    nsmjd = atMjdRiseSet(sra*at_rad2Deg, sdec*at_rad2Deg, longitude, lat, date, tz, zd, SET);  
    stime = ((nsmjd - (int)nsmjd)*24. - tz);
    if (stime<0) {stime+=24.;}

    tstampTemp = atTstampFromMJD(nsmjd);
    atTstampCopy(tstampSet,tstampTemp);
    atTstampDel(tstampTemp);
  }  
}

/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atVFitxy

  DESCRIPTION:
<HTML>
   Calls the SLALIB function SLA_FITXY, with VECTOR's and TRANSs.
   Only the affine terms are fit; the higher-order distortion and color terms
   are set to 0.
</HTML>

  RETURN VALUES:
<HTML>
   Returns the jflag error flag:
<LISTING>
	0 = okay
	-1 = illegal itype
	-2 = insufficient data
	-3 = singular solution
</LISTING>
</HTML>
</AUTO>*/
int atVFitxy(
	     VECTOR *vxe,	/* expected x positions */
	     VECTOR *vye,	/* expected y positions */
	     VECTOR *vxm,	/* measured x positions */
	     VECTOR *vym,	/* measured y positions */
	     VECTOR *vxee,	/* error in vxe */
	     VECTOR *vyee,	/* error in vye */
	     VECTOR *vxme,	/* error in vxm */
	     VECTOR *vyme,	/* error in vym */
	     VECTOR *mask,	/* if mask!=NULL, use only pts set to 1 */
	     TRANS *trans,	/* to get from measured to expected */
	     int itype		/* 4 for solid; 6 allows shears */
	     ) {
  /* the number of points must be the same for all the VECTORs, and >= 3 */
  int np;			/* number of points to fit */
  int i,j;			/* index through the points */
  double (*xye)[2];		/* expected points */
  double (*xym)[2];		/* measured points */
  double coeffs[6];		/* temporary place to keep the trans */
  int jflag;			/* success flag that slaFitxy returns */

  /* See if there is enough input information to do anything useful */
  if ( (itype!=4) && (itype!=6) ) {
    shErrStackPush("atFitxy:  itype must be either 4 or 6");
    return -1;
  }

  if ( (vxe==NULL) || (vye==NULL) || (vxm==NULL) || (vym==NULL) ) {
    shErrStackPush("atFitxy:  must have four non-null VECTORs input");
    return -2;
  }

  np = vxe->dimen;
  if ((vye->dimen != np) || ( vxm->dimen != np) || ( vym->dimen != np)) {
    shErrStackPush("atFitxy:  all VECTORs must have same size");
    return -2;
  }

  if (mask == NULL) {
    np = vxe->dimen;
  } else {
    np = 0;
    for (i=0; i<vxe->dimen; i++) {
      if (mask->vec[i] == 1) np++;
    }
  }

  if ( (itype==4) && (np < 2) ) {
    shErrStackPush("atFitxy:  must have at least two points for itype=4");
    return -2;
  }
  if ( (itype==6) && (np < 3) ) {
    shErrStackPush("atFitxy:  must have at least three points for itype=6");
    return -2;
  }


  /* errors not used in calculation, so ignore if there is a problem */
  if (( (vxee!=NULL) && (vxee->dimen != np) ) || 
      ( (vyee!=NULL) && (vyee->dimen != np) ) || 
      ( (vxme!=NULL) && (vxme->dimen != np) ) ||
      ( (vyme!=NULL) && (vyme->dimen != np) )
      ) {
/*
    shErrStackPush("atFitxy:  all error VECTORs must be same size or NULL");
    return -2;
*/
  }

  /* Allocate storage and stuff the coordinates into xye and xym */
  xye = (double (*)[2]) shMalloc(2*np*sizeof(VECTOR_TYPE));
  xym = (double (*)[2]) shMalloc(2*np*sizeof(VECTOR_TYPE));
  j=0;
  for (i=0; i<vxe->dimen; i++) {
    if ( (mask == NULL) || (mask->vec[i] == 1) ) {
      xye[j][0] = vxe->vec[i];
      xye[j][1] = vye->vec[i];
      xym[j][0] = vxm->vec[i];
      xym[j][1] = vym->vec[i];
      j++;
    }
  }
  /* Make the actual call to Fitxy */
  slaFitxy(itype, np, xye, xym, coeffs, &jflag);

  /* Stuff the answers into the TRANS struct */
  trans->a = coeffs[0];
  trans->b = coeffs[1];
  trans->c = coeffs[2];
  trans->d = coeffs[3];
  trans->e = coeffs[4];
  trans->f = coeffs[5];
  trans->dRow0 = 0;
  trans->dRow1 = 0;
  trans->dRow2 = 0;
  trans->dRow3 = 0;
  trans->dCol0 = 0;
  trans->dCol1 = 0;
  trans->dCol2 = 0;
  trans->dCol3 = 0;
  trans->csRow = 0;
  trans->csCol = 0;
  trans->ccRow = 0;
  trans->ccCol = 0;
  trans->riCut = 0;

  /* clean up the temporary arrays */
  shFree(xye); 
  shFree(xym);

  /* see if it worked and send back error if it failed*/
  if (jflag < 0 ) {
    shErrStackPush("atFitxy: singular solution\n");
  }

  /* send back the TRANS */
  return jflag;
}

/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atVTrans

  DESCRIPTION:
<HTML>
   Applies a TRANS to two VECTOR's containing the measured x and y positions.
   Only the affine terms are applied; the higher-order distortion and color
   terms are ignored.
</HTML>

  RETURN VALUES:
<HTML>
   Returns either SH_SUCCESS or SH_GENERIC_ERROR
</HTML>
</AUTO>*/
RET_CODE atVTrans(TRANS *trans, VECTOR *vxm, VECTOR *vxme, VECTOR *vym, VECTOR *vyme) {
  double xm, ym, dxm, dym, dxe2, dye2, dxe, dye, coeff[6];
  unsigned int i;
  if ((trans==NULL) || (vxm==NULL) || (vym==NULL)) {
    shErrStackPush("atVTrans:  need two non-null VECTORs and a non-null TRANS");
    return SH_GENERIC_ERROR;
  }
  if (vxm->dimen != vym->dimen) {
    shErrStackPush("atVTrans:  both VECTORs need to be the same size");
    return SH_GENERIC_ERROR;
  }
  if ((vxme!=NULL) && (vxme->dimen != vxm->dimen)) {
    shErrStackPush("atVTrans:  x error VECTOR must be size of x vector");
    return SH_GENERIC_ERROR;
  }
  if ((vyme!=NULL) && (vyme->dimen != vym->dimen)) {
    shErrStackPush("atVTrans:  y error VECTOR must be size of y vector");
    return SH_GENERIC_ERROR;
  }
  for (i=0; i<vxm->dimen; i++) {
    xm = vxm->vec[i];
    ym = vym->vec[i];
    if(vxme!=NULL) {
      dxm = vxme->vec[i];
    } else {
      dxm=0;
    }      
    if(vyme!=NULL) {
      dym = vyme->vec[i];
    } else {
      dym=0;
    }
    dxe2 = trans->b*trans->b*dxm*dxm + trans->c*trans->c*dym*dym;
    dye2 = trans->e*trans->e*dxm*dxm + trans->f*trans->f*dym*dym;
    if (dxe2>0) dxe = sqrt(dxe2); else dxe = 0;
    if (dye2>0) dye = sqrt(dye2); else dye = 0;
    coeff[0] = trans->a;
    coeff[1] = trans->b;
    coeff[2] = trans->c;
    coeff[3] = trans->d;
    coeff[4] = trans->e;
    coeff[5] = trans->f;
    slaXy2xy(xm, ym, coeff, &vxm->vec[i], &vym->vec[i]);
    if(vxme!=NULL) {
      vxme->vec[i] = dxe;
    }
    if(vyme!=NULL) {
      vyme->vec[i] = dye;
    }
  }
  return SH_SUCCESS;
}

/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atAberrationApply

  DESCRIPTION:
<HTML>
Apply the aberration (annual plus diurnal) at time MJD for the mean position 
(alpha, delta), returning the apparent place.
</HTML>

  RETURN VALUES:
<HTML>
   Returns either SH_SUCCESS or SH_GENERIC_ERROR
</HTML>
</AUTO>*/
RET_CODE atAberrationApply(
			   double mjd, /* modified Julian date */
			   double epoch, /* Julian epoch (eg 2000) or if 
					    negative, use FK5 at mjd */
			   double *alpha, /* in: ra mean;  out: ra apparent */
			   double *delta /* in: dec mean; out: dec apparent */
			   ) {
  
  double vb[3], pb[3], vh[3], ph[3];
  double dela, deld;
  TSTAMP *tstamp;
  VECTOR *vel;

  /* Find the velocity of the earth */
  slaEvp(mjd, epoch, vb, pb, vh, ph);

  /*  printf(" annual velocity (AU/sec) x=%10.5g y=%10.5g z=%10.5g\n",
	 vb[0],vb[1],vb[2]); */

  /* Find the geocentric apparent velocity of APO */
  tstamp = atTstampFromMJD(mjd);
  vel = shVectorNew(6);
  atVelocityFromTstamp(tstamp,vel);

  /*  printf("diurnal velocity (AU/sec) x=%10.5g y=%10.5g z=%10.5g\n",
	 vel->vec[3],vel->vec[4],vel->vec[5]); */
  /* Add this velocity to vb */
  vb[0] += vel->vec[3];
  vb[1] += vel->vec[4];
  vb[2] += vel->vec[5];

  /* Clean up */
  atTstampDel(tstamp);
  shVectorDel(vel);

  /* The velocities vb are in au/second.  Convert them to units of c */
  /* printf("  total velocity (AU/sec) x=%10.5g y=%10.5g z=%10.5g\n",
	 vb[0],vb[1],vb[2]); */

  vb[0] *= 1495.9787/2.99792458;
  vb[1] *= 1495.9787/2.99792458;
  vb[2] *= 1495.9787/2.99792458;

  /* printf("  total velocity      (c) x=%10.5g y=%10.5g z=%10.5g\n",
	 vb[0],vb[1],vb[2]); */
  /* Get the delta a and d */
  atAberrationGet(vb[0], vb[1], vb[2], *alpha, *delta, &dela, &deld);

  /* Apply these corrections and return */

  *alpha += dela;
  if (*alpha < 0.0) {
    *alpha += 2*M_PI;
  }
  if (*alpha > 2*M_PI) {
    *alpha -= 2*M_PI;
  }
  *delta += deld;

  return SH_SUCCESS;
}
/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atVAberrationApply

  DESCRIPTION:
<HTML>
Apply the aberration at time MJD for the vectors of mean positions 
(alpha, delta), returning the apparent place.
</HTML>

  RETURN VALUES:
<HTML>
   Returns either SH_SUCCESS or SH_GENERIC_ERROR
</HTML>
</AUTO>*/
RET_CODE atVAberrationApply(
			   double mjd, /* modified Julian date */
			   double epoch, /* Julian epoch (eg 2000) or if 
					    negative, use FK5 at mjd */
			   VECTOR* alpha, /* in: ra mean;  out: ra apparent */
			   VECTOR* delta /* in: dec mean; out: dec apparent */
			   ) {
  int i;
  double vb[3], pb[3], vh[3], ph[3];
  double dela, deld;
  TSTAMP *tstamp;
  VECTOR *vel;

  /* Sanity check on the incoming vectors */
  if (alpha->dimen != delta->dimen) {
    shErrStackPush("atVAberrationApply: vectors must have the same dimension");
    return SH_GENERIC_ERROR;
  }

  /* Find the velocity of the earth */
  slaEvp(mjd, epoch, vb, pb, vh, ph);

  /* Find the geocentric apparent velocity of APO */
  tstamp = atTstampFromMJD(mjd);
  vel = shVectorNew(6);
  atVelocityFromTstamp(tstamp,vel);

  /* Add this velocity to vb */
  vb[0] += vel->vec[3];
  vb[1] += vel->vec[4];
  vb[2] += vel->vec[5];

  /* Clean up */
  atTstampDel(tstamp);
  shVectorDel(vel);

  /* The barycentric velocities vb are in au/second.  Convert them to 
     units of c */
  vb[0] *= 1495.9787/2.99792458;
  vb[1] *= 1495.9787/2.99792458;
  vb[2] *= 1495.9787/2.99792458;

  for (i=0; i<alpha->dimen; i++) {
    /* Convert to radians */
    alpha->vec[i] *= at_deg2Rad;
    delta->vec[i] *= at_deg2Rad;

    /* Get the delta a and d */
    atAberrationGet(vb[0], vb[1], vb[2], 
		    alpha->vec[i], delta->vec[i], &dela, &deld);

    /* Apply these corrections */
    alpha->vec[i] += dela;
    delta->vec[i] += deld;

    /* Convert to degrees */
    alpha->vec[i] *= at_rad2Deg;
    delta->vec[i] *= at_rad2Deg;

  }
  return SH_SUCCESS;
}

/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atAberrationGet

  DESCRIPTION:
<HTML>
Return the aberration for the input velocit at the position (alpha, delta), in
the sense that (dAlpha, dDelta) is the apparent place minus mean place.
</HTML>

  RETURN VALUES:
<HTML>
   Returns either SH_SUCCESS or SH_GENERIC_ERROR
</HTML>
</AUTO>*/
RET_CODE atAberrationGet(
			 double vx, /* x, y, z velocities, in units of c */
			 double vy,
			 double vz,
			 double alpha, /* right ascension (radians) */
			 double delta, /* declination (radians) */
			 double *dela, /* change in alpha */
			 double *deld  /* change in delta */
			 ) {

  double cosa, sina, cosd, sind;

  /* Calculate the sin and cos of the angles */
  cosa = cos(alpha); sina = sin(alpha); 
  cosd = cos(delta); sind = sin(delta);

  /* Apply formula 3.253-1 of the Explanatory Supplement... */

  *dela = -vx*sina + vy*cosa + (vx*sina - vy*cosa)*(vx*cosa + vy*sina)/cosd;
  *dela /= cosd;

  *deld = -vx*cosa*sind - vy*sina*sind + vz*cosd -
    0.5*pow((vx*sina - vy*cosa),2)*sind/cosd +
	(vx*cosd*cosa + vy*cosd*sina + vz*sind) * 
	 (vx*sind*cosa + vy*sind*sina - vz*cosd);

  return SH_SUCCESS;
}





/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atVelocityFromTstamp

  DESCRIPTION:
<HTML>
Fill in the values for the geocentric apparent velocity of the
telescope given the Tstamp.
</HTML>

  RETURN VALUES:
<HTML>
   Returns either SH_SUCCESS or SH_GENERIC_ERROR
</HTML>
</AUTO>*/
RET_CODE atVelocityFromTstamp(
			      TSTAMP* tstamp, /* input tstamp */
			      VECTOR* vel /* output:  x, y, z, (in AU),
					     vx, vy, vz 
					     (geodetic, apparent, in AU/sec */
			      ) {
  double pv[6];
  double pxyz[3], vxyz[3], pm[3][3];
  double lst;			/* local apparent sidereal time (radians) */
  double mjd, jepoch;
  char *timetype = "TAI";
  int i;
  if (vel->dimen != 6) {
    shErrStackPush("atVelocityFromTstamp:  vel must have dimension 6");
    return SH_GENERIC_ERROR;
  }
  lst = atTstampToLst(tstamp, at_site_longitude, timetype);
  slaPvobs(
	   at_site_latitude*at_deg2Rad,
	   at_site_altitude,
	   lst*at_deg2Rad,
	   pv);
  /* Now precess this to J2000 */
  mjd = atTstampToMJD(tstamp);
  jepoch = slaEpj(mjd);
  /* Generate the precession matrix for FK5 */
  slaPrec(jepoch, 2000.0, pm);
  /* Precess the position then the velocity */
  slaDmxv(pm, &pv[0], pxyz);
  slaDmxv(pm, &pv[3], vxyz);
  for (i=0; i<3; i++) {
    vel->vec[i]   = pxyz[i];
    vel->vec[i+3] = vxyz[i];
  }
  return SH_SUCCESS;
}

/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atMeanToApparent

  DESCRIPTION:
<HTML>
Transform (alpha,delta) from mean place to geocentric apparent place, 
assuming zero parallax and proper motion.  No guarantees for stars closer
than 300 arcsec of the center of the Sun.
</HTML>

  RETURN VALUES:
<HTML>
   Returns SH_SUCCESS no matter what.
</HTML>
</AUTO>*/
RET_CODE atMeanToApparent(
			  double epoch,	/* epoch of mean equinox (Julian) */
			  double mjd,   /* modified Julian date */
			  double *r,    /* mean->apparent ra (radians)  */
			  double *d    /* mean->apparent dec (radians)  */
			  ) {
  double amprms[21];
  /* Get the star-independent parameters */
  slaMappa(epoch, mjd, amprms);

  /* Do the work */
  slaMapqkz(*r, *d, amprms, r, d);
  return SH_SUCCESS;
}


