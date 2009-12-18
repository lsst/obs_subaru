#ifndef ATSURVEYGEOMETRY_H
#define ATSURVEYGEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "atSlalib.h"

/* The VALUES of all of these are in atExternalSet.c */

extern const double at_galacticPoleRa; /* RA of galactic pole (J2000) */
extern const double at_galacticPoleDec; /* DEC of galactic pole (J2000) */
extern const double at_galacticNode   ; /* galactic longitude of ascending 
					    of galactic plane */
extern const double at_surveyCenterRa ; /* RA of survey center (degrees) */
extern const double at_surveyCenterDec; /* Dec of survey center (degrees) */
extern const double at_surveyEquinox  ; /* equinox (years) */
extern const double at_lambdaMin      ; /* Survey longitude minimum(deg) */
extern const double at_lambdaMax      ; /* Survey longitude maximum(deg) */
extern const double at_etaMin         ; /* Survey latitude minimum (deg) */
extern const double at_etaMax         ; /* Survey latitude maximum (deg) */
extern const double at_stripeWidth    ; /* Width of stripes (deg) */

extern const double at_scanSeparation; /* Scanline separation in degrees */
extern const double at_stripeSeparation; /* Stripe separaion in degrees */

extern const double at_brickLength    ; /* Length of imaging bricks (deg) */
extern const double at_ccdColSep      ; /* CCD column separation (deg) */

/* Survey Ellipse Parameters */
extern const double at_northMajor; /* Major axis radius of the north area */
extern const double at_northMinor; /* Minor axis radius of the north area */ 
extern const double at_northPA   ; /* Position angle (degrees) of north area */
/*
 *  Here are coordinates for two APO telescopes
 */
extern const double at_APO_25_longitude ; /* APO geodetic longitude */
extern const double at_APO_25_latitude  ; /* APO geodetic latitude */
extern const double at_APO_25_altitude ; /*altitude (meters) */

extern const double at_APO_35_longitude ; /* APO geodetic longitude */
extern const double at_APO_35_latitude  ; /* APO geodetic latitude */
extern const double at_APO_35_altitude ; /*altitude (meters) */

extern const double at_FS_longitude ; /* geodetic longitude */
extern const double at_FS_latitude  ; /* geodetic latitude */
extern const double at_FS_altitude ; /*altitude (meters) */

extern const double at_CTIO_longitude ; /* geodetic longitude */
extern const double at_CTIO_latitude  ; /* geodetic latitude */
extern const double at_CTIO_altitude ; /*altitude (meters) */

extern double at_site_longitude;
extern double at_site_latitude;
extern double at_site_altitude;
extern char at_site_name[100];
/*
 * Useful unit conversions are now found in atConversions.h
 */

/*
 * Function prototypes
 */
void atEqToGC (
	       double ra,	/* IN */
	       double dec,	/* IN */
	       double *amu,	/* OUT */
	       double *anu,	/* OUT */
	       double anode,	/* IN */
	       double ainc	/* IN */
	       );

void atGCToEq (
	       double amu,	/* IN */
	       double anu,	/* IN */
	       double *ra,	/* OUT */
	       double *dec,	/* OUT */
	       double anode,	/* IN */
	       double ainc	/* IN */
	       );

void atEqToGal (
		double ra,	/* IN */
		double dec,	/* IN */
		double *glong,	/* OUT: Galactic longitude */
		double *glat	/* OUT: Galactic latitude */
		);

void atGalToEq (
		double glong,	/* IN: Galactic longitude */
		double glat,	/* IN: Galactic latitude */
		double *ra,	/* OUT */
		double *dec	/* OUT */
		);

void atEqToSurvey (
		   double ra,	   /* IN */
		   double dec,	   /* IN */
		   double *slong,  /* OUT: Survey longitude */
		   double *slat   /* OUT: Survey latitude */
		   );

void atGCToSurvey (
                    double amu,
                    double anu,
                    double anode,
                    double ainc,
                    double *slong,
                    double *slat
                  );
void atSurveyToGC (
                    double slong,
                    double slat,
                    double anode,
                    double ainc,
                    double *amu,
                    double *anu
                  );

void atSurveyToEq (
		   double slong, /* IN -- survey longitude in degrees */
		   double slat,	/* IN -- survey latitude in degrees */
		   double *ra,	/* OUT -- ra in degrees */
		   double *dec); /* OUT -- dec in degrees */

void atSurveyToAzelpa(double slong, /* IN: survey longitude (lambda) */
		      double slat, /* IN: survey latitude (eat) */
		      TSTAMP *tstamp, /* IN: time stamp of the observation */
		      double *az,	/* OUT:  azimuth  */
		      double *el,	/* OUT: elevation */
		      double *pa	/* OUT: position angle */
		      );

void atSurveyToAzelpaOld(double slong, /* IN: survey longitude (lambda) */
		      double slat, /* IN: survey latitude (eat) */
		      TSTAMP *tstamp, /* IN: time stamp of the observation */
		      double *az,	/* OUT:  azimuth  */
		      double *el,	/* OUT: elevation */
		      double *pa	/* OUT: position angle */
		      );

void atBound (
	      double *angle,	/* MODIFIED -- the angle to bound */
	      double min,	/* IN -- inclusive minimum value */
	      double max	/* IN -- exclusive maximum value */
	      );

void atBound2(
	      double *theta,	/* MODIFIED -- the -90 to 90 angle */
	      double *phi	/* MODIFIED -- the 0 to 360 angle */
	      );

void atVEqToGC (
	       VECTOR *vRaMu,	/* I/O */
	       VECTOR *vDecNu,	/* I/O */
	       VECTOR_TYPE anode,	/* IN */
	       VECTOR_TYPE ainc	/* IN */
	       );

void atVGCToEq (
	       VECTOR *vMuRa,	/* I/O */
	       VECTOR *vNuDec,	/* I/O */
	       VECTOR_TYPE anode,	/* IN */
	       VECTOR_TYPE ainc	/* IN */
	       );

void atVEqToGal (
		 VECTOR *vRaGLong,	/* I/OUT: Ra / Galactic longitude */
		 VECTOR *vDecGLat  /* I/OUT: Dec / Galactic latitude */
		 );

void atVGalToEq (
		 VECTOR *vGLongRa,	/* I/OUT: Galactic longitude / Ra*/
		 VECTOR *vGLatDec  /* I/OUT: Galactic latitude / Dec */
		);

void atVEqToSurvey (
		    VECTOR *vRaSLong,/* IN -- ra in degrees OUT -- Survey longitude in degrees */
		    VECTOR *vDecSLat  /* IN -- dec in degrees OUT -- Survey latitude in degrees */
		   );

void atVSurveyToEq (
		    VECTOR *vSLongRa,/* OUT -- ra in degrees IN -- Survey longitude in degrees */
		    VECTOR *vSLatDec  /* OUT -- dec in degrees IN -- Survey latitude in degrees */
		    );

void atVSurveyToAzelpa(VECTOR_TYPE slong, /* IN: survey longitude (lambda) */
		      VECTOR_TYPE slat, /* IN: survey latitude (eat) */
		      TSTAMP *tstamp, /* IN: time stamp of the observation */
		      VECTOR_TYPE *az,	/* OUT:  azimuth  */
		      VECTOR_TYPE *el,	/* OUT: elevation */
		      VECTOR_TYPE *pa	/* OUT: position angle */
		      );

void atDefineGC(double ra1,             /* IN: */
                double dec1,            /* IN: */
                double ra2,             /* IN: */
                double dec2,            /* IN: */
                double *anode,          /* OUT: */
                double *ainc            /* OUT: */
                );

void atGCDecFromRa(double ra,   /* IN: */
                double anode,   /* IN: */
                double ainc,    /* IN: */
                double *dec     /* OUT: */
                );

void atAngleGCToRa(double ra,                   /* IN: */
                double dec,                     /* IN: */
                double anode,                   /* IN: */
                double ainc,                    /* IN: */
                 double *angle                  /* OUT: */
                );

void atEtaToStripeNum(
		      double eta,                   /* IN: */
		      int *num                  /* OUT: */
		      );


void atCoordToCobePix(
		      float longitude,
		      float latitude,
		      char *coord, /* q for equatorial; 
				      g for galactic; e for ecliptic */
		      int resolution, /* 9 for DIRBE data */
		      int *pixel /* the pixel number */
		      );

void atCobePixToCoord(
		      int pixel, /* the input pixel number */
		      char *coord, /* q for equatorial; 
				      g for galactic; e for ecliptic */
		      int resolution, /* 9 for DIRBE data */
		      float *longitude,
		      float *latitude
		      );

void atVCoordToCobePix(
		      VECTOR *longitude,
		      VECTOR * latitude,
		      char *coord, /* q for equatorial; 
				      g for galactic; e for ecliptic */
		      int resolution, /* 9 for DIRBE data */
		      VECTOR *pixel /* the pixel number */
		      );

void atVCobePixToCoord(
		       VECTOR *pixel, /* the pixel number */
		       char *coord, /* q for equatorial; 
				       g for galactic; e for ecliptic */
		       int resolution, /* 9 for DIRBE data */
		       VECTOR *longitude,
		       VECTOR * latitude
		       );

void atParallacticAngleGC(
			  double smu,     /* IN: star mu */
			  double snu,     /* IN: star nu */
			  TSTAMP *tstamp, /* IN: TAI time stamp */
			  double anode,   /* IN: mu value of GC node */
			  double ainc,    /* IN: inclination of GC */
			  double *pa	/* OUT: pier-allactic angle */
		      );

char *atSiteSetByName(
		      Tcl_Interp *interp,
		      char *name
		      );

void atSiteSet(
	       Tcl_Interp *interp,
	       double longitude,
	       double latitude,
	       double altitude,
	       char *name
	       );

#ifdef __cplusplus
}
#endif

#endif
