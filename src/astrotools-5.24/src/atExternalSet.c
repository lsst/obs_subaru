#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dervish.h"
#include "atSlalib.h"
#include "atSurveyGeometry.h"
#include "atConversions.h"

/* atExternalSet.c 
   set the VALUES of all the external variables that are in
   atSurveyGeometry.h and atConversions.h */


/* To add a NEW variable, you have to put it in three places:
   
   1 -- a "const double at_variableName = 1.2345" statement in the 
              first half of this file;

   2 -- a "SETVAR(at_variableName)" statement near the bottom of this file;

   3 -- a "extern const double at_variableName" statement in a .h file
              which is included in one of the next lines.
*/

void atTclExternalDeclare(Tcl_Interp *interp);

/*<HTML><H1>
 * atExternalSet.c <BR></H1><H4>
 * set the values of external variables in atSurveyGeometry.h and atConversions.h
 * </H4><PRE>
 */
const double at_galacticPoleRa  = 192.85933333; /* RA galactic pole (J2000) */
const double at_galacticPoleDec = 27.12825278; /* DEC galactic pole (J2000) */
const double at_galacticNode    = 33.07; /* galactic longitude of ascending 
					    of galactic plane */
const double at_surveyCenterRa  =  185.0; /* RA of survey centre (degrees) */
const double at_surveyCenterDec =   32.5; /* Dec of survey centre (degrees) */
const double at_surveyEquinox   = 2000.0; /* Survey equinox (years) */
const double at_lambdaMin       =  -65.0; /* Survey longitude minimum(deg) */
const double at_lambdaMax       =   65.0; /* Survey longitude maximum(deg) */
const double at_etaMin          =  -57.5; /* Survey latitude minimum (deg) */
const double at_etaMax          =   55.0; /* Survey latitude maximum (deg) */
const double at_stripeWidth     =    2.5; /* Width of stripes (deg) */

/* 
 * Scanline separation in degrees, from raytrace 
 */
#define ATSCANSEPARATION  62.93233/(5.0*60.0)
const double at_scanSeparation  = ATSCANSEPARATION;

/*
 * Stripe separation in degrees -- 12 scanlines minus 2 arcmin, gives
 * 3 arcmin overlap between stripes 
 */
const double at_stripeSeparation = 2.5;
const double at_brickLength     =    2.0; /* Length of imaging bricks (deg) */
const double at_ccdColSep       = 25.1/60.0; /* CCD column separation (deg) */

/* 
 * Survey Ellipse Parameters 
 */
const double at_northMajor = 65.0; /* Major axis radius of the north area */
const double at_northMinor = 55.0; /* Minor axis radius of the north area */ 
const double at_northPA    = 20.0; /* Position angle (degrees) of north area */


/*
 * Here are APOs coordinates
 */
/* Change made March 27, 1998: to root out the use of these variables names:  

   at_APOLongitude2_5m 
   at_APOLatitude2_5m  
   at_APOLongitude3_5m 
   at_APOLatitude3_5m  
   at_APOAltitude      

   in code and replace them with the at_site variables below.
   */

const double at_APO_25_longitude =  105.8198305; /* geodetic WEST longitude */
const double at_APO_25_latitude  =   32.7797556; /* geodetic NORTH latitude */
const double at_APO_25_altitude  = 2797.0;       /* altitude (meters) */

const double at_APO_35_longitude =  105.8198778; /* geodetic WEST longitude */
const double at_APO_35_latitude  =   32.7805444; /* geodetic NORTH latitude */
const double at_APO_35_altitude  = 2797.0;       /* altitude (meters) */

const double at_FS_longitude =   111.7382917; /* geodetic WEST longitude */
const double at_FS_latitude  =    35.1843250; /* geodetic NORTH latitude */
const double at_FS_altitude  =  2274.2;       /* altitude (meters) */

const double at_CTIO_longitude =   70.814999; /* geodetic WEST longitude */
const double at_CTIO_latitude  =  -30.164999; /* geodetic NORTH latitude */
const double at_CTIO_altitude  = 2215.0;      /* altitude (meters) */

const double at_MDO_longitude =  104.0216667; /* geodetic WEST longitude */
const double at_MDO_latitude  =   30.6716667; /* geodetic NORTH latitude */
const double at_MDO_altitude  = 2075.0;      /* altitude (meters) */


/* Actual values defined once here (not in include) */
double at_site_longitude = 0;
double at_site_latitude = 0;
double at_site_altitude = 0;
char at_site_name[100];


/* 
 * Hours <--> Radians 
 */
const double at_hrs2Rad = M_PI/12.0; 
const double at_rad2Hrs = 12.0/M_PI; 

/* 
 * Time minutes <--> Radians
 */
const double at_tmin2Rad = M_PI/(12.0 * 60.0); 
const double at_rad2Tmin = (12.0 * 60.0)/M_PI;

/*
 * Time seconds <--> Radians 
 */
const double at_tsec2Rad = M_PI/(12.0 * 3600.0);	
const double at_rad2Tsec = (12.0 * 3600.0)/M_PI;

/* 
 * Degrees <--> Radians 
 */
const double at_deg2Rad = M_PI/180.0; 
const double at_rad2Deg = 180.0/M_PI;

/* 
 * Arc Minutes <--> Radians 
 */
const double at_amin2Rad = M_PI/(180.0 * 60.0); 
const double at_rad2Amin = (180.0 * 60.0)/M_PI;

/* 
 * Arc Seconds <--> Radians 
 */
const double at_asec2Rad = M_PI/(180.0 * 3600.0); 
const double at_rad2Asec = (180.0 * 3600.0)/M_PI;

/* 
 * Hours <--> Degrees 
 */
const double at_hrs2Deg = 180.0 / 12.0; 
const double at_deg2Hrs = 12.0/ 180.0; 

/* 
 * Time minutes <--> Degrees 
 */
const double at_tmin2Deg = 180.0 /(12.0 * 60.0); 
const double at_deg2Tmin = (12.0 * 60.0)/ 180.0;

/* 
 * Time seconds <--> Degrees 
 */
const double at_tsec2Deg = 180.0 / (12.0 * 3600.0);	
const double at_deg2Tsec = (12.0 * 3600.0)/ 180.0;

/* 
 * Arc Minutes <--> Degrees 
 */
const double at_amin2Deg = 1.0 / 60.0;
const double at_deg2Amin = 60.0;

/* 
 * Arc Seconds <--> Degrees 
 */
const double at_asec2Deg = 1.0 / 3600.0; 
const double at_deg2Asec = 3600.0;

/* </PRE>
  </HTML>*/

#define SETVAR(variable) \
sprintf(s, "%22.17e", variable); \
Tcl_SetVar(interp, #variable , s, TCL_LEAVE_ERR_MSG)

#define SETVARC(variable) \
sprintf(s, "%s", variable); \
Tcl_SetVar(interp, #variable , s, TCL_LEAVE_ERR_MSG)


void atTclExternalDeclare(Tcl_Interp *interp) {
  char s[100];
  /*
   * Here are the generic coordinates set by default to the APO 2.5m telescope
   */
  at_site_longitude = at_APO_25_longitude;
  at_site_latitude  = at_APO_25_latitude;
  at_site_altitude  = at_APO_25_altitude;
  strcpy(at_site_name, "APO_25");
  
  SETVAR(at_site_longitude);
  SETVAR(at_site_latitude);
  SETVAR(at_site_altitude);
  SETVARC(at_site_name);

  SETVAR(at_galacticPoleRa);
  SETVAR(at_galacticPoleDec);
  SETVAR(at_galacticNode);
  SETVAR(at_surveyCenterRa);
  SETVAR(at_surveyCenterDec);
  SETVAR(at_surveyEquinox);
  SETVAR(at_lambdaMin);
  SETVAR(at_lambdaMax);
  SETVAR(at_etaMin);
  SETVAR(at_etaMax);
  SETVAR(at_stripeWidth);
  SETVAR(at_scanSeparation);
  SETVAR(at_stripeSeparation);
  SETVAR(at_brickLength);
  SETVAR(at_ccdColSep);
  SETVAR(at_northMajor);
  SETVAR(at_northMinor);
  SETVAR(at_northPA);

  SETVAR(at_APO_25_longitude);
  SETVAR(at_APO_25_latitude);
  SETVAR(at_APO_25_altitude);

  SETVAR(at_APO_35_longitude);
  SETVAR(at_APO_35_latitude);
  SETVAR(at_APO_35_altitude);

  SETVAR(at_FS_longitude);
  SETVAR(at_FS_latitude);
  SETVAR(at_FS_altitude);

  SETVAR(at_CTIO_longitude);
  SETVAR(at_CTIO_latitude);
  SETVAR(at_CTIO_altitude);

  SETVAR(at_hrs2Rad);
  SETVAR(at_rad2Hrs);
  SETVAR(at_tmin2Rad);
  SETVAR(at_rad2Tmin);
  SETVAR(at_tsec2Rad);
  SETVAR(at_rad2Tsec);
  SETVAR(at_deg2Rad);
  SETVAR(at_rad2Deg);
  SETVAR(at_amin2Rad);
  SETVAR(at_rad2Amin);
  SETVAR(at_asec2Rad);
  SETVAR(at_rad2Asec);
  SETVAR(at_hrs2Deg);
  SETVAR(at_deg2Hrs);
  SETVAR(at_tmin2Deg);
  SETVAR(at_deg2Tmin);
  SETVAR(at_tsec2Deg);
  SETVAR(at_deg2Tsec);
  SETVAR(at_amin2Deg);
  SETVAR(at_deg2Amin);
  SETVAR(at_asec2Deg);
  SETVAR(at_deg2Asec);
}

/*<AUTO EXTRACT>

  ROUTINE: atSiteSetByName

  DESCRIPTION:
<HTML>
Set the global c and TCL variables which define the location of the observatory
</HTML>

  RETURN VALUES:
<HTML>
char NULL if successful; list of legal names otherwise.
</HTML>
</AUTO>*/
char *atSiteSetByName(
		      Tcl_Interp *interp,
		      char *name
		      ) {
  char s[100];
  char *sites="APO_25 APO_35 FS CTIO MDO";
  
  if (!strcmp(name,"APO_25")) {
    at_site_longitude = at_APO_25_longitude;
    at_site_latitude  = at_APO_25_latitude;
    at_site_altitude  = at_APO_25_altitude;
    strcpy(at_site_name, name);
    SETVAR(at_site_longitude);
    SETVAR(at_site_latitude);
    SETVAR(at_site_altitude);
    SETVARC(at_site_name);
    return (char *) NULL;
  }
  
  if (!strcmp(name,"APO_35")) {
    at_site_longitude = at_APO_35_longitude;
    at_site_latitude  = at_APO_35_latitude;
    at_site_altitude  = at_APO_35_altitude;
    strcpy(at_site_name, name);
    SETVAR(at_site_longitude);
    SETVAR(at_site_latitude);
    SETVAR(at_site_altitude);
    SETVARC(at_site_name);
    return (char *) NULL;
  }

  if (!strcmp(name,"FS")) {
    at_site_longitude = at_FS_longitude;
    at_site_latitude  = at_FS_latitude;
    at_site_altitude  = at_FS_altitude;
    strcpy(at_site_name, name);
    SETVAR(at_site_longitude);
    SETVAR(at_site_latitude);
    SETVAR(at_site_altitude);
    SETVARC(at_site_name);
    return (char *) NULL;
  }

  if (!strcmp(name,"CTIO")) {
    at_site_longitude = at_CTIO_longitude;
    at_site_latitude  = at_CTIO_latitude;
    at_site_altitude  = at_CTIO_altitude;
    strcpy(at_site_name, name);
    SETVAR(at_site_longitude);
    SETVAR(at_site_latitude);
    SETVAR(at_site_altitude);
    SETVARC(at_site_name);
    return (char *) NULL;
  }

  if (!strcmp(name,"MDO")) {
    at_site_longitude = at_MDO_longitude;
    at_site_latitude  = at_MDO_latitude;
    at_site_altitude  = at_MDO_altitude;
    strcpy(at_site_name, name);
    SETVAR(at_site_longitude);
    SETVAR(at_site_latitude);
    SETVAR(at_site_altitude);
    SETVARC(at_site_name);
    return (char *) NULL;
  }

  return sites;
}

/*<AUTO EXTRACT>

  ROUTINE: atSiteSet

  DESCRIPTION:
<HTML>
Set the global c and TCL variables which define the location of the observatory
</HTML>

  RETURN VALUES:
<HTML>
none
</HTML>
</AUTO>*/
void atSiteSet(
	       Tcl_Interp *interp,
	       double longitude,
	       double latitude,
	       double altitude,
	       char *name
	       ) {
  char s[100];
  at_site_longitude = longitude;
  at_site_latitude  = latitude;
  at_site_altitude  = altitude;
  strcpy(at_site_name, name);
  SETVAR(at_site_longitude);
  SETVAR(at_site_latitude);
  SETVAR(at_site_altitude);
  SETVARC(at_site_name);
  return;
}
