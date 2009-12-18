#ifndef ATSLALIB_H
#define ATSLALIB_H


/******************************************************************************
<AUTO>
 *  FILE:  atSlalib.h
 *  Interface to slalib routines and TSTAMP, the time stamp structure
 *
 * <DATE>
 </AUTO>
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include "dervish.h"
#include "atTrans.h"


/* <HTML><PRE> */

typedef struct Tstamp {
  int year;			/* year (1995, for example) */
  int month;			/* month (1 to 12) */
  int day;			/* day (1 to 31) */
  int hour;			/* hour (0 to 23) */
  int minute;			/* minute (0 to 59) */
  double second;		/* second (0 to 59.99999) */
} TSTAMP;
/* </PRE></HTML> */

/* PROTOTYPES FOR C-INTERFACE TO SLALIB */
void tclAtDeclare(Tcl_Interp *interp);

RET_CODE atDayFromTime(char *string, float *day); 
double atRadFromDMS(char *string);
double atRadFromHMS(char *string);
int atRadToDMS(double angle, char *string);
int atRadToHMS(double angle, char *string);
double atRadToDeg(double radians);
double atRadFromDeg(double degrees);
double atXFromxy(TRANS *trans, double x, double y);
double atYFromxy(TRANS *trans, double x, double y);
double atDMSToDeg(char *string);
double atHMSToDeg(char *string);
int atDegToDMS(double angle, char *string);
int atDegToHMS(double angle, char *string);

/* Prototypes for TSTAMPS structure */
TSTAMP *atTstampNew(
		    int year,	/* year including century (1995, NOT 95) */
		    int month,	/* month (1 through 12) */
		    int day,	/* day (1 through 31) */
		    int hour,	/* hour (0 through 23) */
		    int minute, /* minute (0 through 59) */
		    double second /* second (0.0 through 59.999999 */
		    );

TSTAMP *atTstampNow(
		    char *timetype  /* UT (default) or TAI*/
		    );

RET_CODE atTstampDel(
		     TSTAMP *tstamp
		     );

double atTstampToMJD(
		     TSTAMP *tstamp
		     );

TSTAMP *atTstampFromMJD(
			double mjd
			);

RET_CODE atTstampIncr(
		       TSTAMP *tstamp, /* the TSTAMP to increment */
		       double seconds /* number of seconds to increment */
		       );

RET_CODE atTstampCopy(
		      TSTAMP *tstampDestination, 
		      TSTAMP *tstampSource
		      );

RET_CODE atTstampToDate(
			TSTAMP *tstamp, /* the TSTAMP */
			char *date /* char address at least 11 long */
			);

RET_CODE atTstampToTime(
			TSTAMP *tstamp, /* the TSTAMP */
			char *date /* char address at least 14 long */
			);
double atTstampToLst(
		     TSTAMP *tstamp, /* the TSTAMP */
                     double theLongitude,
		     char *timetype  /* UT (default) or TAI*/
		     );

double atTstampToTwilight(
			   TSTAMP *tstampNow,    /* IN: any tstamp in night */
			  double degrees,
			   TSTAMP *tstampSunset, /* OUT: tstamp of sunset */
			   TSTAMP *tstampSunrise /* OUT: tstamp of sunrise */
			   );

void atPlanetRiseSet(
			   TSTAMP *tstampNow,    /* IN: any tstamp in night */
			   TSTAMP *tstampSet, /* OUT: tstamp of sunset */
			   TSTAMP *tstampRise, /* OUT: tstamp of sunrise */
		           int np,
		           double longitude,
		           double lat,
		           int tz,
		           double dz
			   );

void atPlanetRaDec(
			   TSTAMP *tstampNow,    /* IN: any tstamp in night */
		           int np,
		           double longitude,
		           double lat,
		           double *ra, 
			   double *dec
			   );

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
	     TRANS *trans,	/* to get from measure to expected */
	     int itype		/* 4 for solid; 6 allows shears */
	     );

RET_CODE atVTrans(TRANS *trans, VECTOR *vxm, VECTOR *vxme, 
		  VECTOR *vym, VECTOR *vyme);

RET_CODE atVS2tp(VECTOR* ra, VECTOR* dec, double raz, double decz);

RET_CODE atVTp2s(VECTOR* ra, VECTOR* dec, double raz, double decz);

RET_CODE atAberrationApply(
			   double mjd, /* modified Julian date */
			   double epoch, /* Julian epoch (eg 2000) or if 
					    negative, use FK5 at mjd */
			   double *alpha, /* in: ra mean;  out: ra apparent */
			   double *delta /* in: dec mean; out: dec apparent */
			   );

RET_CODE atVAberrationApply(
			   double mjd, /* modified Julian date */
			   double epoch, /* Julian epoch (eg 2000) or if 
					    negative, use FK5 at mjd */
			   VECTOR* alpha, /* in: ra mean;  out: ra apparent */
			   VECTOR* delta /* in: dec mean; out: dec apparent */
			   );

RET_CODE atAberrationGet(
			 double vx, /* x, y, z velocities, in units of c */
			 double vy,
			 double vz,
			 double alpha, /* right ascension (radians) */
			 double delta, /* declination (radians) */
			 double *dela, /* change in alpha */
			 double *deld  /* change in delta */
			 );

RET_CODE atVelocityFromTstamp(
			      TSTAMP* tstamp, /* input tstamp */
			      VECTOR* vel /* output:  vx, vy, vz 
					     (geodetic, apparent, in AU/sec */
			      );

RET_CODE atMeanToApparent(
			  double epoch,	/* epoch of mean equinox (Julian) */
			  double mjd,   /* modified Julian date */
			  double *r,    /* mean->apparent ra (radians)  */
			  double *d    /* mean->apparent dec (radians)  */
			  );
#ifdef __cplusplus
}
#endif

#endif
