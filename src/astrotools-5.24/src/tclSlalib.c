/* <AUTO>
   FILE: tclSlalib
<HTML>
   TCL support for SLALIB (astrometry) routines
</HTML>
   </AUTO>
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "smaValues.h"
#include "dervish.h"
#include "tcl.h"
#include "shCUtils.h"
#include "shCErrStack.h"
#include "atSlalib.h"
#include "shTclHandle.h"
#include "shTclRegSupport.h"            /* needed for shTclDeclare prototype */
#include "tclMath.h"
#include "atConversions.h"
#include "slalib.h"
#include "atEphemeris.h"
#include "atSurveyGeometry.h"

static char *module = "astrotools";    /* name of this set of code */
static char *tclFacil = "astrotools";

/**************************************************************/
/*
 * radFromDeg
 */
/*<AUTO EXTRACT>
  TCL VERB: radFromDeg
<HTML>
C ROUTINE CALLED:
<A HREF=atSlalib.html#atRadFromDeg>atRadFromDeg</A> in atSlalib.c
<P>
Convert degrees to radians
</HTML>
  </AUTO>
*/
static char *tclRadFromDeg_use = "USAGE: radFromDeg deg";
static char *tclRadFromDeg_hlp = "Convert degrees to radians";

static int
tclRadFromDeg(ClientData cd, Tcl_Interp *interp, int argc, char **argv) {
  double degrees, radians;
  char result[80];
  if (argc!=2) {
    Tcl_SetResult(interp, tclRadFromDeg_use, TCL_VOLATILE);
    return TCL_ERROR;
  }
  degrees = strtod(argv[1], (void *)NULL);
  radians = atRadFromDeg(degrees);

  sprintf(result, "%13.10f", radians);
  Tcl_SetResult(interp, result, TCL_VOLATILE);

  return TCL_OK;

}

/**************************************************************/
/*
 * radToDeg
 */
/*<AUTO EXTRACT>
  TCL VERB: radToDeg
<HTML>
C ROUTINE CALLED:
<A HREF=atSlalib.html#atRadToDeg>atRadToDeg</A> in atSlalib.c
<P>
Convert radians to degrees
</HTML>
  </AUTO>
*/
static char *tclRadToDeg_use = "USAGE: radToDeg rad";
static char *tclRadToDeg_hlp = "Convert radians to degrees";

static int
tclRadToDeg(ClientData cd, Tcl_Interp *interp, int argc, char **argv) {
  double degrees, radians;
  char result[80];
  if (argc!=2) {
    Tcl_SetResult(interp, tclRadToDeg_use, TCL_VOLATILE);
    return TCL_ERROR;
  }
  radians = strtod(argv[1], (void *)NULL);
  degrees = atRadToDeg(radians);

  sprintf(result, "%13.10f\n", degrees);
  Tcl_SetResult(interp, result, TCL_VOLATILE);

  return TCL_OK;

}

/**************************************************************/
/*
 * Cartesian coordinates from (long, lat)
 */
/*<AUTO EXTRACT>
  TCL VERB: lbToXyz
<HTML>
C ROUTINE CALLED:
slaDcs2c in slalib library
<P>
Convert (long, lat) in degrees to (x, y, z)
</HTML>
  </AUTO>
*/
static char *tclLbToXyz_use = "USAGE: lbToXyz longitude latitude";
static char *tclLbToXyz_hlp = "Convert (long, lat) in degrees to (x,y,z)";

static int
tclLbToXyz(ClientData cd, Tcl_Interp *interp, int argc, char **argv) {
  double longitude, latitude;
  double xyz[3];
  char result[80];
  if (argc!=3) {
    Tcl_SetResult(interp, tclLbToXyz_use, TCL_VOLATILE);
    return TCL_ERROR;
  }
  longitude = strtod(argv[1], (void *)NULL);
  latitude = strtod(argv[2], (void *)NULL);
  longitude = atRadFromDeg(longitude);
  latitude = atRadFromDeg(latitude);
  slaDcs2c(longitude,latitude,xyz);

  sprintf(result, "%13.10f %13.10f %13.10f\n", xyz[0], xyz[1], xyz[2]);
  Tcl_SetResult(interp, result, TCL_VOLATILE);

  return TCL_OK;

}

/**************************************************************/
/*
 * Cartesian coordinates to (long, lat)
 */
/*<AUTO EXTRACT>
  TCL VERB: xyzToLb
<HTML>
C ROUTINE CALLED:
slaDcc2s in slalib library
<P>
Convert (x, y, z) to (long, lat) in degrees
</HTML>
  </AUTO>
*/
static char *tclXyzToLb_use = "USAGE: xyzToLb x y z";
static char *tclXyzToLb_hlp = "Convert (x,y,z) to (long, lat) in degrees";

static int
tclXyzToLb(ClientData cd, Tcl_Interp *interp, int argc, char **argv) {
  double longitude, latitude;
  double xyz[3];
  char result[80];
  if (argc!=4) {
    Tcl_SetResult(interp, tclXyzToLb_use, TCL_VOLATILE);
    return TCL_ERROR;
  }
  xyz[0] = strtod(argv[1], (void *)NULL);
  xyz[1] = strtod(argv[2], (void *)NULL);
  xyz[2] = strtod(argv[3], (void *)NULL);
  slaDcc2s(xyz,&longitude,&latitude);
  longitude = atRadToDeg(longitude);
  latitude = atRadToDeg(latitude);
  if (longitude<0.0) longitude = longitude + 360;

  sprintf(result, "%13.10f %13.10f\n", longitude, latitude);
  Tcl_SetResult(interp, result, TCL_VOLATILE);

  return TCL_OK;

}

/****************************************************************************
**
** ROUTINE: tclVS2tp
**
**<AUTO EXTRACT>
** TCL VERB: vS2tp
<HTML>
C ROUTINE CALLED:
<A HREF=atSlalib.html#atVS2tp>atVS2tp</A> in atSlalib.c
</HTML>
**
**</AUTO>
*/
char g_vS2tp[] = "vS2tp";
ftclArgvInfo g_vS2tpTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Project spherical coordinates to those on a tangential plane. The tangential point is given by ra and dec in degrees.\n" },
  {"<vRa>", FTCL_ARGV_STRING, NULL, NULL, 
   "vector of RAs to use"},
  {"<vDec>", FTCL_ARGV_STRING, NULL, NULL, 
   "vector of DECs to use"},
  {"<ra>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "tangential point ra"},
  {"<dec>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "tangential point dec"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVS2tp(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_vRa = NULL;
  char *a_vDec = NULL;
  double ra=0, dec = 0;
  VECTOR *vRa = NULL, *vDec = NULL;

  g_vS2tpTbl[1].dst = &a_vRa;
  g_vS2tpTbl[2].dst = &a_vDec;
  g_vS2tpTbl[3].dst = &ra;
  g_vS2tpTbl[4].dst = &dec;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_vS2tpTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_vS2tp)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_vRa, (void **) &vRa, "VECTOR") 
      != TCL_OK) {
    Tcl_SetResult(interp, "tclVS2tp:  bad v_ra name", TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (shTclAddrGetFromName(interp, a_vDec, (void **) &vDec, "VECTOR")
      != TCL_OK) {
    Tcl_SetResult(interp, "tclVS2tp:  bad v_dec name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  
  if(atVS2tp(vRa, vDec, ra, dec)!=SH_SUCCESS) {
    shTclInterpAppendWithErrStack(interp);
    return TCL_ERROR;
  }
  
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclVTp2s
**
**<AUTO EXTRACT>
** TCL VERB: vTp2s
<HTML>
<A HREF=atSlalib.html#atVTp2s>atVTp2s</A> in atSlalib.c
</HTML>
**
**</AUTO>
*/
char g_vTp2s[] = "vTp2s";
ftclArgvInfo g_vTp2sTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Project tangential plane to spherical coordinates. The tangential point is given by ra and dec in degrees.\n" },
  {"<vXi>", FTCL_ARGV_STRING, NULL, NULL, 
   "vector of xi's (in degrees) to use (will be modified)"},
  {"<vEta>", FTCL_ARGV_STRING, NULL, NULL, 
   "vector of eta's (in degrees) to use (will be modified)"},
  {"<ra>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "tangential point ra"},
  {"<dec>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "tangential point dec"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVTp2s(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_vXi = NULL;
  char *a_vEta = NULL;
  double ra=0, dec = 0;
  VECTOR *vXi = NULL, *vEta = NULL;

  g_vTp2sTbl[1].dst = &a_vXi;
  g_vTp2sTbl[2].dst = &a_vEta;
  g_vTp2sTbl[3].dst = &ra;
  g_vTp2sTbl[4].dst = &dec;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_vTp2sTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_vTp2s)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_vXi, (void **) &vXi, "VECTOR") 
      != TCL_OK) {
    Tcl_SetResult(interp, "tclVTp2s:  bad v_xi name", TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (shTclAddrGetFromName(interp, a_vEta, (void **) &vEta, "VECTOR")
      != TCL_OK) {
    Tcl_SetResult(interp, "tclVTp2s:  bad v_eta name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  
  if(atVTp2s(vXi, vEta, ra, dec)!=SH_SUCCESS) {
    shTclInterpAppendWithErrStack(interp);
    return TCL_ERROR;
  }
  
  return TCL_OK;
}


/****************************************************************************/
/*
 * dayFromTime
 */

/*<AUTO EXTRACT>
  TCL VERB: dayFromTime
<HTML>
C ROUTINE CALLED:
<A HREF=atSlalib.html#atDayFromTime>atDayFromTime</A> in atSlalib.c
</HTML>
  </AUTO>
*/
static char *tclDayFromTime_use =
  "USAGE: dayFromTime hh:mm:sec";
static char *tclDayFromTime_hlp =
  "Convert the hh:mm:sec string to days";

static int
tclDayFromTime(ClientData cd, Tcl_Interp *interp, int argc, char **argv)
{
  float day;
  char answer[40];
  if (argc != 2) {
     Tcl_SetResult(interp,tclDayFromTime_use,TCL_VOLATILE);
     return(TCL_ERROR);
   }
  if (atDayFromTime(argv[1], &day) == SH_SUCCESS) {
    sprintf(answer,"%9.7f",day);
    Tcl_SetResult(interp,answer,TCL_VOLATILE);
    return(TCL_OK);
  } else {
    Tcl_SetResult(interp,"-1.0",TCL_VOLATILE);
    return(TCL_ERROR);
  }
}

/****************************************************************************/
/*
 * coordPrecess
 */

/*<AUTO EXTRACT>
  TCL VERB: coordPrecess
<HTML>
C ROUTINE CALLED:
slaPreces in slalib library
</HTML>
  </AUTO>
*/
static char *tclCoordPrecess_use =
  "USAGE: coordPrecess raInDegrees decInDegrees startepoch endepoch FK4/FK5";
static char *tclCoordPrecess_hlp =
  "Precess coordinates from one epoch to another, return as a tcl list";

static int
tclCoordPrecess(ClientData cd, Tcl_Interp *interp, int argc, char **argv)
{
  char syst[40];
  char *temp;
  char answer[80];
  double ra, dec, ep0, ep1;
  int a,b;
  if(argc==6) {
    temp = argv[5];
    strcpy(syst, temp);
    a = (strcmp(syst, "FK5")==0) | (strcmp(syst, "fk5")==0);
    b = (strcmp(syst, "FK4")==0) | (strcmp(syst, "fk4")==0);
    if((a==0) && (b==0)) {
      Tcl_SetResult(interp,"must use FK4 or FK5",TCL_VOLATILE);
      return(TCL_ERROR);
    }
    ra = strtod(argv[1], (void *) NULL);
    dec = strtod(argv[2], (void *) NULL);
    ep0 = strtod(argv[3], (void *) NULL);
    ep1 = strtod(argv[4], (void *) NULL);
   } else {
     Tcl_SetResult(interp,tclCoordPrecess_use,TCL_VOLATILE);
     return(TCL_ERROR);
   }

  ra = ra*at_deg2Rad;
  dec = dec*at_deg2Rad;
  slaPreces(syst, ep0, ep1, &ra, &dec);
  ra = ra*at_rad2Deg;
  dec = dec*at_rad2Deg;
  
  sprintf(answer,"{ra %12.10f} {dec %12.10f}", ra, dec);
  Tcl_SetResult(interp,answer,TCL_VOLATILE);
  return(TCL_OK);
}

/****************************************************************************/
/*
 * coordPreNut
 */

/*<AUTO EXTRACT>
  TCL VERB: coordPreNut
<HTML>
C ROUTINES CALLED:
slaDcs2c, slaPrenut, slaDmxv, slaDimxv, and  slaDcc2s in slalib library
<P>
This routine performs both precession and nutation on a coordinate.
Nutation has an amplitude of about 9" and a period of 18.6 years.
<p>
"Epoch" is the epoch of true coordinates
(on the sky, without refraction and aberration). 
"meanEpoch" is the epoch of mean coordinates (catalog). 
The routine takes mean into true coordinates.
Optionally one may reverse the direction, from true to mean.
<p>
The difference between this routine and coordPrecess is that this
one adds nutation. The difference between this routine and atMean2Apparent
is primarily that the latter adds annual aberration and secondarily
that the latter adds light deflection effects.
</HTML>
</AUTO>
*/

char g_coordPreNut[] = "coordPreNut";
ftclArgvInfo g_coordPreNutTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Precess and nutate coordinates from catalog to true coordinates, in FK5.\nreturn a tcl list of coordinates" },
  {"<ra>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "ra in degrees"},
  {"<dec>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "dec in degrees"},
  {"<epoch>", FTCL_ARGV_STRING, NULL, NULL, 
   "the true epoch TSTAMP"},
  {"-meanEpoch", FTCL_ARGV_STRING, NULL, NULL, 
   "the TSTAMP holding the epoch of the mean (catalog) coordinates. (defaults to J2000)"},
  {"-reverse", FTCL_ARGV_INT, NULL, NULL,
   "if 1, reverse the sense, taking true to mean coordinates"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclCoordPreNut(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  char answer[80];
  double ra, dec;
  double cartesian[3];			/* x,y,z coords */
  double rotCartesian[3];		/* rotated x,y,z coords */
  double rotMatrix[3][3];		/* preces. nutation rotation matrix */
  double mjd;				/* epoch in mjd */
  double mjdM;				/* mean epoch in mjd */
  double jEpoch;			/* julian epoch of mean coords */
  int sDefault = 0;			/* use J2000? */
  int reverse = 0;			/* if 1, dir of conv. is true to mean */
  char *a_epoch = NULL;
  char *a_meanEpoch = NULL;
  TSTAMP *epoch = NULL;
  TSTAMP *meanEpoch = NULL;
  int rstat;

  g_coordPreNutTbl[1].dst = &ra;
  g_coordPreNutTbl[2].dst = &dec;
  g_coordPreNutTbl[3].dst = &a_epoch;
  g_coordPreNutTbl[4].dst = &a_meanEpoch;
  g_coordPreNutTbl[5].dst = &reverse;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_coordPreNutTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_coordPreNut)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_epoch, (void **) &epoch, "TSTAMP")
     != TCL_OK) {
     Tcl_SetResult(interp, "coordPreNut:  bad epoch name", TCL_VOLATILE);
     return TCL_ERROR;
  }

  if (a_meanEpoch == NULL) { sDefault = 1;  }

  if (sDefault == 0) {
     if (shTclAddrGetFromName(interp, a_meanEpoch, (void **) &meanEpoch, "TSTAMP")
      != TCL_OK) {
       Tcl_SetResult(interp, "coordPreNut:  bad meanEpoch name", TCL_VOLATILE);
       return TCL_ERROR;
     }
  }

  /* convert times (presumably in gregorian calender) to mjds */
  if (sDefault == 0) {
     mjdM = atTstampToMJD(meanEpoch);
     jEpoch = slaEpj(mjdM);		/* target epoch */
  } else {
     jEpoch = 2000.0;
  }
  mjd = atTstampToMJD(epoch);
     
  /* convert ra,dec to radians */
  ra = ra*at_deg2Rad;
  dec = dec*at_deg2Rad;

  /* get cartesian coords of ra,dec */
  slaDcs2c(ra,dec,cartesian);

  /* find precession and nutation matrix */
  slaPrenut(jEpoch,mjd,rotMatrix);

  /* do the rotation on the vector */
  if (reverse != 1) {	/* go from mean to true (catalog to apparent) */
      slaDmxv(rotMatrix,cartesian,rotCartesian);
  } else {		/* go from true to mean */
      slaDimxv(rotMatrix,cartesian,rotCartesian);
  }

  /* get ra,dec of the cartesian coords */
  slaDcc2s(rotCartesian,&ra,&dec);

  ra = ra*at_rad2Deg; if (ra < 0) { ra = ra + 360.0;  }
  dec = dec*at_rad2Deg;
  
  sprintf(answer,"{ra %12.10f} {dec %12.10f}", ra, dec);
  Tcl_SetResult(interp,answer,TCL_VOLATILE);
  return(TCL_OK);
}

/****************************************************************************/
/*
 * earthPosVel
 */

/*<AUTO EXTRACT>
  TCL VERB: earthPosVel
<HTML>
C ROUTINES CALLED:
slaEpj, slaEvp in slalib library
</HTML>
</AUTO>
*/

char g_earthPosVel[] = "earthPosVel";
ftclArgvInfo g_earthPosVelTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Find the position and velocity of the Earth in barycentric coordinates\nPositions are in mean FK5 coords, units of AU. Velocities are AU/s." },
  {"<mjd>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "modified julian date"},
  {"-epoch", FTCL_ARGV_STRING, NULL, NULL, 
   "the TSTAMP holding the epoch of the mean coordinates. (defaults to J2000)"},
  {"-helio", FTCL_ARGV_INT, NULL, NULL, 
   "if 1,  coordinates returned are heliocentric (defaults to barycentric)"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclEarthPosVel(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  char answer[180];
  double bary[3];			/* x,y,z coords */
  double helio[3];			/* x,y,z coords */
  double baryVel[3];			/* x,y,z coords */
  double helioVel[3];			/* x,y,z coords */
  double mjd;				/* epoch in mjd */
  double mjdM;				/* mean epoch in mjd */
  double jEpoch;			/* julian epoch of mean coords */
  int helioRet = 0;			/* if 1, return helio */
  int sDefault = 0;
  char *a_epoch = NULL;
  TSTAMP *epoch = NULL;
  int rstat;

  g_earthPosVelTbl[1].dst = &mjd;
  g_earthPosVelTbl[2].dst = &a_epoch;
  g_earthPosVelTbl[3].dst = &helioRet;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_earthPosVelTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_earthPosVel)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (a_epoch == NULL) { sDefault = 1;  }

  if (sDefault == 0) {
     if (shTclAddrGetFromName(interp, a_epoch, (void **) &epoch, "TSTAMP")
     != TCL_OK) {
        Tcl_SetResult(interp, "earthPosVel:  bad epoch name", TCL_VOLATILE);
        return TCL_ERROR;
     }
  }

  /* convert times (presumably in gregorian calender) to mjds */
  if (sDefault == 0) {
     mjdM = atTstampToMJD(epoch);
     jEpoch = slaEpj(mjdM);		/* target epoch */
  } else {
     jEpoch = -2000.0;			/* negative defaults c routine */
  }
     
  /* get cartesian coords of earth velocity vector */
  slaEvp(mjd,jEpoch,baryVel,bary,helioVel,helio);

  if (helioRet == 1) {
  	sprintf(answer,"{x %12.10f} {y %12.10f} {z %12.10f} {xdot %12.10f} {ydot %12.10f} {zdot %12.10f}", helio[0],helio[1],helio[2],helioVel[0],helioVel[1],helioVel[2]);
  } else {
     sprintf(answer,"{x %12.10f} {y %12.10f} {z %12.10f} {xdot %12.10f} {ydot %12.10f} {zdot %12.10f}", bary[0],bary[1],bary[2],baryVel[0],baryVel[1],baryVel[2]);
  }
  Tcl_SetResult(interp,answer,TCL_VOLATILE);
  return(TCL_OK);
}

/****************************************************************************/
/*
 * radFromDMS
 */

/*<AUTO EXTRACT>
  TCL VERB: radFromDMS
<HTML>
C ROUTINE CALLED:
<A HREF=atSlalib.html#atRadFromDMS>atRadFromDMS</A> in atSlalib.c
</HTML>
  </AUTO>
*/
static char *tclRadFromDMS_use =
  "USAGE: radFromDMS hh:mm:sec";
static char *tclRadFromDMS_hlp =
  "Convert the dd:mm:sec string to radians";

static int
tclRadFromDMS(ClientData cd, Tcl_Interp *interp, int argc, char **argv)
{
  double rad;
  char answer[40];
  if (argc != 2) {
     Tcl_SetResult(interp,tclRadFromDMS_use,TCL_VOLATILE);
     return(TCL_ERROR);
  }
  if ( (rad = atRadFromDMS(argv[1]))  >= -10.0) {
    sprintf(answer,"%20.17g",rad);
    Tcl_SetResult(interp,answer,TCL_VOLATILE);
    return(TCL_OK);
  } else {
    Tcl_SetResult(interp,"-10.0",TCL_VOLATILE);
    return(TCL_ERROR);
  }
}
/****************************************************************************/
/*
 * radFromHMS
 */

/*<AUTO EXTRACT>
  TCL VERB: radFromHMS
<HTML>
C ROUTINE CALLED:
<A HREF=atSlalib.html#atRadFromHMS>atRadFromHMS</A> in atSlalib.c
</HTML>
  </AUTO>
*/
static char *tclRadFromHMS_use =
  "USAGE: radFromHMS hh:mm:sec";
static char *tclRadFromHMS_hlp =
  "Convert the hh:mm:sec string to radians";

static int
tclRadFromHMS(ClientData cd, Tcl_Interp *interp, int argc, char **argv)
{
  double rad;
  char answer[40];

  if (argc != 2) {
     Tcl_SetResult(interp,tclRadFromHMS_use,TCL_VOLATILE);
     return(TCL_ERROR);
  }
  if ( (rad = atRadFromHMS(argv[1]))  >= -10.0) {
    sprintf(answer,"%20.17g",rad);
    Tcl_SetResult(interp,answer,TCL_VOLATILE);
    return(TCL_OK);
  } else {
    Tcl_SetResult(interp,"-10.0",TCL_VOLATILE);
    return(TCL_ERROR);
  }
}
/****************************************************************************/
/*
 * DMSToDeg
 */

/*<AUTO EXTRACT>
  TCL VERB: DMSToDeg
<HTML>
C ROUTINE CALLED:
<A HREF=atSlalib.html#atDMSToDeg>atDMSToDeg</A> in atSlalib.c
</HTML>
  </AUTO>
*/
static char *tclDMSToDeg_use =
  "USAGE: DMSToDeg hh:mm:sec";
static char *tclDMSToDeg_hlp =
  "Convert the dd:mm:sec string to degrees";

static int
tclDMSToDeg(ClientData cd, Tcl_Interp *interp, int argc, char **argv)
{
  double deg;
  char answer[40];
  if (argc != 2) {
     Tcl_SetResult(interp,tclDMSToDeg_use,TCL_VOLATILE);
     return(TCL_ERROR);
  }
  if ( (deg = atDMSToDeg(argv[1]))  > -1000.0) {
    sprintf(answer,"%20.17g",deg);
    Tcl_SetResult(interp,answer,TCL_VOLATILE);
    return(TCL_OK);
  } else {
    Tcl_SetResult(interp,"err",TCL_VOLATILE);
    return(TCL_ERROR);
  }
}
/****************************************************************************/
/*
 * HMSToDeg
 */

/*<AUTO EXTRACT>
  TCL VERB: HMSToDeg
<HTML>
C ROUTINE CALLED:
<A HREF=atSlalib.html#atHMSToDeg>atHMSToDeg</A> in atSlalib.c
</HTML>
  </AUTO>
*/
static char *tclHMSToDeg_use =
  "USAGE: HMSToDeg hh:mm:sec";
static char *tclHMSToDeg_hlp =
  "Convert the hh:mm:sec string to degrees";

static int
tclHMSToDeg(ClientData cd, Tcl_Interp *interp, int argc, char **argv)
{
  double deg;
  char answer[40];

  if (argc != 2) {
     Tcl_SetResult(interp,tclHMSToDeg_use,TCL_VOLATILE);
     return(TCL_ERROR);
  }
  if ( (deg = atHMSToDeg(argv[1]))  > -1000.0) {
    sprintf(answer,"%20.17g",deg);
    Tcl_SetResult(interp,answer,TCL_VOLATILE);
    return(TCL_OK);
  } else {
    Tcl_SetResult(interp,"err",TCL_VOLATILE);
    return(TCL_ERROR);
  }
}
/****************************************************************************/
/*
 * radToDMS
 */

/*<AUTO EXTRACT>
  TCL VERB: radToDMS
<HTML>
C ROUTINE CALLED:
<A HREF=atSlalib.html#atRadToDMS>atRadToDMS</A> in atSlalib.c
</HTML>
  </AUTO>
*/
static char *tclRadToDMS_use =
  "USAGE: radToDMS radians";
static char *tclRadToDMS_hlp =
  "Convert radians to dd:mm:sec string";

static int
tclRadToDMS(ClientData cd, Tcl_Interp *interp, int argc, char **argv)
{
  double rad;
  char answer[40];
  if (argc != 2) {
    Tcl_SetResult(interp,tclRadToDMS_use,TCL_VOLATILE);
    return(TCL_ERROR);
  }
  rad = strtod(argv[1], NULL);
  if ( atRadToDMS(rad, answer)  == TCL_OK) {
    Tcl_SetResult(interp, answer, TCL_VOLATILE);
    return(TCL_OK);
  } else {
    Tcl_SetResult(interp,"error from in tclRadToDMS atRadToDMS",TCL_VOLATILE);
    return(TCL_ERROR);
  }
}
/****************************************************************************/
/*
 * radToHMS
 */

/*<AUTO EXTRACT>
  TCL VERB: radToHMS
<HTML>
C ROUTINE CALLED:
<A HREF=atSlalib.html#atRadToHMS>atRadToHMS</A> in atSlalib.c
</HTML>
  </AUTO>
*/
static char *tclRadToHMS_use =
  "USAGE: radToHMS radians";
static char *tclRadToHMS_hlp =
  "Convert radians to hh:mm:sec string";

static int
tclRadToHMS(ClientData cd, Tcl_Interp *interp, int argc, char **argv)
{
  double rad;
  char answer[40];
  if (argc != 2) {
    Tcl_SetResult(interp,tclRadToHMS_use,TCL_VOLATILE);
    return(TCL_ERROR);
  }
  rad = strtod(argv[1], NULL);
  if (atRadToHMS(rad, answer)  == TCL_OK) {
    Tcl_SetResult(interp, answer, TCL_VOLATILE);
    return(TCL_OK);
  } else {
    Tcl_SetResult(interp,"Error from atRadToHMS in tclRadToHMS",TCL_VOLATILE);
    return(TCL_ERROR);
  }
}

/****************************************************************************/
/*
 * degToDMS
 */

/*<AUTO EXTRACT>
  TCL VERB: degToDMS
<HTML>
C ROUTINE CALLED:
<A HREF=atSlalib.html#atDegToDMS>atDegToDMS</A> in atSlalib.c
</HTML>
  </AUTO>
*/
static char *tclDegToDMS_use =
  "USAGE: degToDMS degrees";
static char *tclDegToDMS_hlp =
  "Convert degrees to dd:mm:sec string";

static int
tclDegToDMS(ClientData cd, Tcl_Interp *interp, int argc, char **argv)
{
  double deg;
  char answer[40];
  if (argc != 2) {
    Tcl_SetResult(interp,tclDegToDMS_use,TCL_VOLATILE);
    return(TCL_ERROR);
  }
  deg = strtod(argv[1], NULL);
  if ( atDegToDMS(deg, answer)  == TCL_OK) {
    Tcl_SetResult(interp, answer, TCL_VOLATILE);
    return(TCL_OK);
  } else {
    Tcl_SetResult(interp,"error in tclDegToDMS from atDegToDMS",TCL_VOLATILE);
    return(TCL_ERROR);
  }
}
/****************************************************************************/
/*
 * degToHMS
 */

/*<AUTO EXTRACT>
  TCL VERB: degToHMS
<HTML>
C ROUTINE CALLED:
<A HREF=atSlalib.html#atDegToHMS>atDegToHMS</A> in atSlalib.c
</HTML>
  </AUTO>
*/
static char *tclDegToHMS_use =
  "USAGE: degToHMS degrees";
static char *tclDegToHMS_hlp =
  "Convert degrees to hh:mm:sec string";

static int
tclDegToHMS(ClientData cd, Tcl_Interp *interp, int argc, char **argv)
{
  double deg;
  char answer[40];
  if (argc != 2) {
    Tcl_SetResult(interp,tclDegToHMS_use,TCL_VOLATILE);
    return(TCL_ERROR);
  }
  deg = strtod(argv[1], NULL);
  if (atDegToHMS(deg, answer)  == TCL_OK) {
    Tcl_SetResult(interp, answer, TCL_VOLATILE);
    return(TCL_OK);
  } else {
    Tcl_SetResult(interp,"Error from atDegToHMS in tclDegToHMS",TCL_VOLATILE);
    return(TCL_ERROR);
  }
}

/***************************************************************************/
/*
 * sepGet
 */
/*<AUTO EXTRACT>
  TCL VERB: sepGet
<HTML>
C ROUTINE CALLED:
slaDsep in slalib library
</HTML>
  </AUTO>
*/
static char *tclSepGet_use  = "USAGE:  sepGet ra1 dec1 ra2 dec2";
static char *tclSepGet_help = "Given two positions (in degrees) return "
"the separation angle (in degrees)";
static int
tclSepGet(ClientData cd, Tcl_Interp *interp, int argc, char **argv) {
  double ra1, dec1, ra2, dec2, sep;
  char answer[100];
  if (argc != 5) {
    Tcl_SetResult(interp, tclSepGet_use, TCL_VOLATILE);
    return TCL_ERROR;
  }
  ra1  = strtod(argv[1], NULL)*at_deg2Rad;
  dec1 = strtod(argv[2], NULL)*at_deg2Rad;
  ra2  = strtod(argv[3], NULL)*at_deg2Rad;
  dec2 = strtod(argv[4], NULL)*at_deg2Rad;
  sep = slaDsep(ra1, dec1, ra2, dec2)*at_rad2Deg;
  sprintf(answer, "%20.17g", sep);
  Tcl_SetResult(interp, answer, TCL_VOLATILE);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclTstampNew
**
**<AUTO EXTRACT>
** TCL VERB: tstampNew
**<HTML>
** C ROUTINE CALLED:
** <A HREF=atSlalib.html#atTstampNew>atTstampNew</A> in atSlalib.c
**</HTML>
**</AUTO>
*/
char g_tstampNew[] = "tstampNew";
ftclArgvInfo g_tstampNewTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Make a new TSTAMP with the specified date and time.\nRETURN:  handle to the TSTAMP\n" },
  {"<year>", FTCL_ARGV_STRING, NULL, NULL, 
   "year, including centuries:  1995, for example"},
  {"<month>", FTCL_ARGV_STRING, NULL, NULL, 
   "month, from 1 to 12"},
  {"<day>", FTCL_ARGV_STRING, NULL, NULL, 
   "day of the month, from 1 to 31"},
  {"<hour>", FTCL_ARGV_STRING, NULL, NULL, 
   "hour, from 0 to 23"},
  {"<minute>", FTCL_ARGV_STRING, NULL, NULL, 
   "minute, from 0 to 59"},
  {"<second>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "seconds, from 0.0 to 59.999999"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclTstampNew(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  char *ptr;
  int rstat;
  char *s_year = NULL;
  char *s_month = NULL;
  char *s_day = NULL;
  char *s_hour = NULL;
  char *s_minute = NULL;
  int a_year = 0;
  int a_month = 0;
  int a_day = 0;
  int a_hour = 0;
  int a_minute = 0;
  double a_second = 0.0;

  TSTAMP *tstamp=NULL;
  char tstampName[HANDLE_NAMELEN];

  g_tstampNewTbl[1].dst = &s_year;
  g_tstampNewTbl[2].dst = &s_month;
  g_tstampNewTbl[3].dst = &s_day;
  g_tstampNewTbl[4].dst = &s_hour;
  g_tstampNewTbl[5].dst = &s_minute;
  g_tstampNewTbl[6].dst = &a_second;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_tstampNewTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_tstampNew)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  a_year = (int) strtol(s_year, &ptr, 10);
  if (ptr == s_year || *ptr != 0) {
    Tcl_SetResult(interp, "tstampNew: invalid year string", TCL_VOLATILE);
    return TCL_ERROR;
  }
  a_month = (int) strtol(s_month, &ptr, 10);
  if (ptr == s_month || *ptr != 0) {
    Tcl_SetResult(interp, "tstampNew: invalid month string", TCL_VOLATILE);
    return TCL_ERROR;
  }
  a_day = (int) strtol(s_day, &ptr, 10);
  if (ptr == s_day || *ptr != 0) {
    Tcl_SetResult(interp, "tstampNew: invalid day string", TCL_VOLATILE);
    return TCL_ERROR;
  }
  a_hour = (int) strtol(s_hour, &ptr, 10);
  if (ptr == s_hour || *ptr != 0) {
    Tcl_SetResult(interp, "tstampNew: invalid hour string", TCL_VOLATILE);
    return TCL_ERROR;
  }
  a_minute = (int) strtol(s_minute, &ptr, 10);
  if (ptr == s_minute || *ptr != 0) {
    Tcl_SetResult(interp, "tstampNew: invalid minute string", TCL_VOLATILE);
    return TCL_ERROR;
  }

  tstamp = atTstampNew(a_year, a_month, a_day, a_hour, a_minute, a_second);

  if (shTclHandleNew(interp, tstampName, "TSTAMP", tstamp) != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to new tstamp handle", TCL_STATIC);
    return(TCL_ERROR);
  }
  Tcl_SetResult(interp, tstampName, TCL_VOLATILE);
  return TCL_OK;
}


/****************************************************************************
**
** ROUTINE: tclTstampNow
**
**<AUTO EXTRACT>
** TCL VERB: tstampNow
**<HTML>
** C ROUTINE CALLED:
** <A HREF=atSlalib.html#atTstampNow>atTstampNow</A> in atSlalib.c
**</HTML>
**</AUTO>
*/
char g_tstampNow[] = "tstampNow";
ftclArgvInfo g_tstampNowTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Make new TSTAMP with the current GMT.\nRETURN:  handle to the TSTAMP\n" },
  {"-timetype", FTCL_ARGV_STRING, NULL, NULL, "UT(default) or TAI"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclTstampNow(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *timetype = "UT";

  TSTAMP *tstamp=NULL;
  char tstampName[HANDLE_NAMELEN];

  g_tstampNowTbl[1].dst = &timetype;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_tstampNowTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_tstampNow)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  tstamp = atTstampNow(timetype);

  if (shTclHandleNew(interp, tstampName, "TSTAMP", tstamp) != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to new tstamp handle", TCL_STATIC);
    return(TCL_ERROR);
  }
  Tcl_SetResult(interp, tstampName, TCL_VOLATILE);
  return TCL_OK;
}


/****************************************************************************
**
** ROUTINE: tclTstampDel
**
**<AUTO EXTRACT>
** TCL VERB: tstampDel
**<HTML>
** C ROUTINE CALLED:
** <A HREF=atSlalib.html#atTstampDel>atTstampDel</A> in atSlalib.c
**</HTML>
**</AUTO>
*/
char g_tstampDel[] = "tstampDel";
ftclArgvInfo g_tstampDelTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "delete a TSTAMP\n" },
  {"<tstamp>", FTCL_ARGV_STRING, NULL, NULL, 
   "the tstamp to delete"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclTstampDel(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_tstamp = NULL;
  TSTAMP *tstamp=NULL;

  g_tstampDelTbl[1].dst = &a_tstamp;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_tstampDelTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_tstampDel)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_tstamp, (void **) &tstamp, 
			   "TSTAMP") != TCL_OK) {
    Tcl_SetResult(interp, "bad TSTAMP name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  atTstampDel(tstamp);
  p_shTclHandleDel(interp, a_tstamp);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclDeltaTstamp
**
**<AUTO EXTRACT>
** TCL VERB: deltaTstamp
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSlalib.html#atTstampToMJD>atTstampToMJD</A> in atSlalib.c
** </HTML>
**</AUTO>
*/
char g_deltaTstamp[] = "deltaTstamp";
ftclArgvInfo g_deltaTstampTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Return the tstamp1 - tstamp2 in seconds\n" },
  {"<tstamp1>", FTCL_ARGV_STRING, NULL, NULL, 
   "The first tstamp"},
  {"<tstamp2>", FTCL_ARGV_STRING, NULL, NULL, 
   "The tstamp to subtract from the first tstamp"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclDeltaTstamp(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_tstamp1 = NULL;
  char *a_tstamp2 = NULL;
  TSTAMP *tstamp1=NULL;
  TSTAMP *tstamp2=NULL;
  double delta;
  char answer[40];
  g_deltaTstampTbl[1].dst = &a_tstamp1;
  g_deltaTstampTbl[2].dst = &a_tstamp2;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_deltaTstampTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_deltaTstamp)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_tstamp1, (void **) &tstamp1, 
			   "TSTAMP") != TCL_OK) {
    Tcl_SetResult(interp, "bad TSTAMP 1 name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, a_tstamp2, (void **) &tstamp2, 
			   "TSTAMP") != TCL_OK) {
    Tcl_SetResult(interp, "bad TSTAMP 2 name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  delta = 24.0*3600.0*(atTstampToMJD(tstamp1) - atTstampToMJD(tstamp2));
  sprintf(answer,"%20.17f",delta);
  Tcl_SetResult(interp, answer, TCL_VOLATILE);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclTstampFromMJD
**
**<AUTO EXTRACT>
** TCL VERB: tstampFromMJD
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSlalib.c#atTstampFromMJD>atTstampFromMJD</A> in atSlalib.c
**</HTML>
**</AUTO>
*/
char g_tstampFromMJD[] = "tstampFromMJD";
ftclArgvInfo g_tstampFromMJDTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Create a tstamp corresponding to a MJD date\n" },
  {"<MJD>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "the MJD date"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclTstampFromMJD(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_MJD = 0.0;
  TSTAMP *tstamp=NULL;
  char tstampName[HANDLE_NAMELEN];

  g_tstampFromMJDTbl[1].dst = &a_MJD;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_tstampFromMJDTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_tstampFromMJD)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  tstamp = atTstampFromMJD(a_MJD);

  if (shTclHandleNew(interp, tstampName, "TSTAMP", tstamp) != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to new tstamp handle", TCL_STATIC);
    return(TCL_ERROR);
  }
  Tcl_SetResult(interp, tstampName, TCL_VOLATILE);
  return TCL_OK;

}

/****************************************************************************
**
** ROUTINE: tclTstampToMJD
**
**<AUTO EXTRACT>
** TCL VERB: tstampToMJD
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSlalib.html#atTstampToMJD>atTstampToMJD</A> in atSlalib.c
** </HTML>
**</AUTO>
*/
char g_tstampToMJD[] = "tstampToMJD";
ftclArgvInfo g_tstampToMJDTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Return the MJD date corresponding to the TSTAMP\n" },
  {"<tstamp>", FTCL_ARGV_STRING, NULL, NULL, 
   "the TSTAMP"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclTstampToMJD(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_tstamp = NULL;
  TSTAMP *tstamp=NULL;
  double MJD;
  char answer[40];

  g_tstampToMJDTbl[1].dst = &a_tstamp;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_tstampToMJDTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_tstampToMJD)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_tstamp, (void **) &tstamp, 
			   "TSTAMP") != TCL_OK) {
    Tcl_SetResult(interp, "bad TSTAMP name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  MJD = atTstampToMJD(tstamp);
  sprintf(answer,"%17.9f",MJD);
  Tcl_SetResult(interp, answer, TCL_VOLATILE);
  return TCL_OK;


}

/****************************************************************************
**
** ROUTINE: tclTstampIncr
**
**<AUTO EXTRACT>
** TCL VERB: tstampIncr
**<HTML>
** C ROUTINE CALLED:
** <A HREF=atSlalib.html#atTstampIncr>atTstampIncr</A> in atSlalib.c
**</HTML>
**</AUTO>
*/
char g_tstampIncr[] = "tstampIncr";
ftclArgvInfo g_tstampIncrTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Increment TSTAMP by seconds\n" },
  {"<tstamp>", FTCL_ARGV_STRING, NULL, NULL, 
   "the tstamp to increment"},
  {"<seconds>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "number of seconds to increment "},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclTstampIncr(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_tstamp = NULL;
  double a_seconds = 0.0;
  TSTAMP *tstamp=NULL;

  g_tstampIncrTbl[1].dst = &a_tstamp;
  g_tstampIncrTbl[2].dst = &a_seconds;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_tstampIncrTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_tstampIncr)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_tstamp, (void **) &tstamp, 
			   "TSTAMP") != TCL_OK) {
    Tcl_SetResult(interp, "bad TSTAMP name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  atTstampIncr(tstamp, a_seconds);
  Tcl_SetResult(interp, a_tstamp, TCL_VOLATILE);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclTstampToDate
**
**<AUTO EXTRACT>
** TCL VERB: tstampToDate
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSlalib.html#atTstampToDate>atTstampToDate</A> in atSlalib.c
** </HTML>
**</AUTO>
*/
char g_tstampToDate[] = "tstampToDate";
ftclArgvInfo g_tstampToDateTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "return the formatted date from the TSTAMP\n" },
  {"<tstamp>", FTCL_ARGV_STRING, NULL, NULL, 
   "the tstamp"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclTstampToDate(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_tstamp = NULL;
  TSTAMP *tstamp = NULL;
  char answer[11];
  g_tstampToDateTbl[1].dst = &a_tstamp;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_tstampToDateTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_tstampToDate)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_tstamp, (void **) &tstamp, 
			   "TSTAMP") != TCL_OK) {
    Tcl_SetResult(interp, "bad TSTAMP name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  
  atTstampToDate(tstamp, answer);
  Tcl_SetResult(interp, answer, TCL_VOLATILE);
  return TCL_OK;

}
/****************************************************************************
**
** ROUTINE: tclTstampToTime
**
**<AUTO EXTRACT>
** TCL VERB: tstampToTime
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSlalib.html#atTstampToTime>atTstampToTime</A> in atSlalib.c
** </HTML>
**</AUTO>
*/
char g_tstampToTime[] = "tstampToTime";
ftclArgvInfo g_tstampToTimeTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "return the formatted time from the TSTAMP\n" },
  {"<tstamp>", FTCL_ARGV_STRING, NULL, NULL, 
   "the tstamp"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclTstampToTime(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_tstamp = NULL;
  TSTAMP *tstamp = NULL;
  char answer[14];
  g_tstampToTimeTbl[1].dst = &a_tstamp;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_tstampToTimeTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_tstampToTime)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_tstamp, (void **) &tstamp, 
			   "TSTAMP") != TCL_OK) {
    Tcl_SetResult(interp, "bad TSTAMP name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  
  atTstampToTime(tstamp, answer);
  Tcl_SetResult(interp, answer, TCL_VOLATILE);
  return TCL_OK;

}
/****************************************************************************
**
** ROUTINE: tclTstampToLst
**
**<AUTO EXTRACT>
** TCL VERB: tstampToLst
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSlalib.html#atTstampToLst>atTstampToLst</A> in atSlalib.c
** </HTML>
**</AUTO>
*/
char g_tstampToLst[] = "tstampToLst";
ftclArgvInfo g_tstampToLstTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "return the formatted Lst from the TSTAMP\n" },
  {"<tstamp>", FTCL_ARGV_STRING, NULL, NULL, "the tstamp"},
  {"-long", FTCL_ARGV_DOUBLE, NULL, NULL, 
     "observatory longitude, at_site_longitude default"},
  {"-timetype", FTCL_ARGV_STRING, NULL, NULL, "UT(default) or TAI"},  
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclTstampToLst(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_tstamp = NULL;
  TSTAMP *tstamp = NULL;
  char *timetype = "UT";
  char answer[20];
  double lst;
  double a_longitude =  at_site_longitude;

  g_tstampToLstTbl[1].dst = &a_tstamp;
  g_tstampToLstTbl[2].dst = &a_longitude;
  g_tstampToLstTbl[3].dst = &timetype;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_tstampToLstTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_tstampToLst)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_tstamp, (void **) &tstamp, 
			   "TSTAMP") != TCL_OK) {
    Tcl_SetResult(interp, "bad TSTAMP name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  
  lst = atTstampToLst(tstamp, a_longitude, timetype);
  sprintf(answer,"%17.10f", lst);
  Tcl_SetResult(interp, answer, TCL_VOLATILE);
  return TCL_OK;

}

/****************************************************************************
**
** ROUTINE: tclTstampToTwilight
**
**<AUTO EXTRACT>
** TCL VERB: tstampToTwilight
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSlalib.html#atTstampToTwilight> atTstampToTwilight</A> in atSlalib.c
** </HTML>
**</AUTO>
*/
char g_tstampToTwilight[] = "tstampToTwilight";
ftclArgvInfo g_tstampToTwilightTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "return the formatted Lst from the TSTAMP\n" },
  {"<tstamp>", FTCL_ARGV_STRING, NULL, NULL, 
   "the tstamp"},
  {"-degrees", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "degrees below the horizon for darkness"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclTstampToTwilight(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_tstamp = NULL;
  TSTAMP *tstamp = NULL;
  TSTAMP *tstampSunset = NULL;
  TSTAMP *tstampSunrise = NULL;
  char tstampSunriseName[HANDLE_NAMELEN];
  char tstampSunsetName[HANDLE_NAMELEN];
  double a_degrees = 18.0;
  char answer[100];
  double hours;
  g_tstampToTwilightTbl[1].dst = &a_tstamp;
  g_tstampToTwilightTbl[2].dst = &a_degrees;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_tstampToTwilightTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_tstampToTwilight)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_tstamp, (void **) &tstamp, 
			   "TSTAMP") != TCL_OK) {
    Tcl_SetResult(interp, "bad TSTAMP name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  
  tstampSunset = atTstampNew(0,0,0,0,0,0);
  tstampSunrise = atTstampNew(0,0,0,0,0,0);
  hours = atTstampToTwilight(tstamp, a_degrees, tstampSunset, tstampSunrise);

  if (shTclHandleNew(interp, tstampSunsetName, "TSTAMP", tstampSunset) 
      != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to new tstamp handle", TCL_STATIC);
    return(TCL_ERROR);
  }

  if (shTclHandleNew(interp, tstampSunriseName, "TSTAMP", tstampSunrise) 
      != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to new tstamp handle", TCL_STATIC);
    return(TCL_ERROR);
  }

  sprintf(answer,"duration %17.10f", hours); 
  Tcl_AppendElement(interp, answer);
  sprintf(answer,"sunset %s",tstampSunsetName);
  Tcl_AppendElement(interp, answer);
  sprintf(answer,"sunrise %s",tstampSunriseName);
  Tcl_AppendElement(interp, answer);
  return TCL_OK;

}

/****************************************************************************
**
** ROUTINE: tclPlanetRaDec
**/
/*<AUTO EXTRACT>
 TCL VERB: planetRaDec

<HTML>
C ROUTINE CALLED: <A HREF=atSlalib.html#atPlanetRaDec>atPlanetRaDec</A> in atSlalib.c

<P> Finds the RA and DEC at a given time for Sun, Moon, or planet.
The planet numbers (np) are given by:
<LISTING>
1 Mercury
2 Venus
3 Moon
4 Mars
5 Jupiter
6 Saturn 
7 Uranus
8 Neptune
x Pluto (not supported)
10 Sun
</LISTING>
</HTML>

</AUTO>
*/
char g_planetRaDec[] = "planetRaDec";
ftclArgvInfo g_planetRaDecTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "return the RA and DEC of the Sun, Moon, or planet in a keyed list\n" },
  {"<tstamp>", FTCL_ARGV_STRING, NULL, NULL, 
   "the tstamp"},
  {"<np>", FTCL_ARGV_INT, NULL, NULL, 
   "which planet, moon=3, sun=10"},
  {"-longitude", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "west longitude of observatory, at_site_longitude"},
  {"-lat", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "north latitude of observatory, at_site_latitude default"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPlanetRaDec(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_tstamp = NULL;
  TSTAMP *tstamp = NULL;
  int a_np;
  double a_longitude = at_site_longitude;
  double a_lat = at_site_latitude;
  double ra;
  double dec;
  char answer[300];
  /* double hours; */
  g_planetRaDecTbl[1].dst = &a_tstamp;
  g_planetRaDecTbl[2].dst = &a_np;
  g_planetRaDecTbl[3].dst = &a_longitude;
  g_planetRaDecTbl[4].dst = &a_lat;
 
  if ((rstat = shTclParseArgv(interp, &argc, argv, g_planetRaDecTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_planetRaDec)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_tstamp, (void **) &tstamp, 
			   "TSTAMP") != TCL_OK) {
    Tcl_SetResult(interp, "bad TSTAMP name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  
  atPlanetRaDec(tstamp, a_np, a_longitude, a_lat, &ra, &dec);

  /* printf("ra %f dec %f\n",ra,dec); */

  sprintf(answer,"ra %17.10f", ra);
  Tcl_AppendElement(interp, answer);
  sprintf(answer,"dec %17.10f", dec);
  Tcl_AppendElement(interp, answer);
  return TCL_OK;

}

/****************************************************************************
**
** ROUTINE: tclPlanetRiseSet
**/
/*<AUTO EXTRACT>
 TCL VERB: planetRiseSet

<HTML>
C ROUTINE CALLED: 
<A HREF=atSlalib.html#atPlanetRiseSet>atPlanetRiseSet</A> in atSlalib.c
<P> Finds the rise and set times for Sun, Moon, or planet.
The planet numbers (np) are given by:
<LISTING>
1 Mercury
2 Venus
3 Moon
4 Mars
5 Jupiter
6 Saturn 
7 Uranus
8 Neptune
x Pluto (not supported)
10 Sun
</LISTING>
Time zones are hours from GMT.  Zenith distance defaults to 108 degrees.
</HTML>

</AUTO>
*/
char g_planetRiseSet[] = "planetRiseSet";
ftclArgvInfo g_planetRiseSetTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "return the formatted Lst from the TSTAMP\n" },
  {"<tstamp>", FTCL_ARGV_STRING, NULL, NULL, 
   "the tstamp"},
  {"<np>", FTCL_ARGV_INT, NULL, NULL, 
   "which planet, moon=3, sun=10"},
  {"-longitude", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "longitude of observatory, at_site_longitude default"},
  {"-lat", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "latitude of observatory, at_site_latitude default"},
  {"-tz", FTCL_ARGV_INT, NULL, NULL, 
   "timezone, GMT=0, default"},
  {"-zd", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "zenith distance that defines horizon, 108 degrees default"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPlanetRiseSet(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_tstamp = NULL;
  TSTAMP *tstamp = NULL;
  TSTAMP *tstampSet = NULL;
  TSTAMP *tstampRise = NULL;
  char tstampRiseName[HANDLE_NAMELEN];
  char tstampSetName[HANDLE_NAMELEN];
  int a_np;
  double a_longitude = at_site_longitude;
  double a_lat = at_site_latitude;
  int a_tz = 0;
  double a_zd = 108.0;
  char answer[300];
  /* double hours; */
  g_planetRiseSetTbl[1].dst = &a_tstamp;
  g_planetRiseSetTbl[2].dst = &a_np;
  g_planetRiseSetTbl[3].dst = &a_longitude;
  g_planetRiseSetTbl[4].dst = &a_lat;
  g_planetRiseSetTbl[5].dst = &a_tz;
  g_planetRiseSetTbl[6].dst = &a_zd;
 
  if ((rstat = shTclParseArgv(interp, &argc, argv, g_planetRiseSetTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_planetRiseSet)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_tstamp, (void **) &tstamp, 
			   "TSTAMP") != TCL_OK) {
    Tcl_SetResult(interp, "bad TSTAMP name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  
  tstampSet = atTstampNew(0,0,0,0,0,0);
  tstampRise = atTstampNew(0,0,0,0,0,0);
  atPlanetRiseSet(tstamp, tstampSet, tstampRise, a_np, a_longitude, a_lat, a_tz, a_zd);

  if (shTclHandleNew(interp, tstampSetName, "TSTAMP", tstampSet) 
      != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to new tstamp handle", TCL_STATIC);
    return(TCL_ERROR);
  }

  if (shTclHandleNew(interp, tstampRiseName, "TSTAMP", tstampRise) 
      != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to new tstamp handle", TCL_STATIC);
    return(TCL_ERROR);
  }

  /* sprintf(answer,"duration %17.10f", hours); 
  Tcl_AppendElement(interp, answer); */
  sprintf(answer,"Set %s",tstampSetName);
  Tcl_AppendElement(interp, answer);
  sprintf(answer,"Rise %s",tstampRiseName);
  Tcl_AppendElement(interp, answer);
  return TCL_OK;

}

/****************************************************************************/
/*
 * VFitxy
 */
/*<AUTO EXTRACT>
  TCL VERB: vFitxy
<HTML>
C ROUTINE CALLED:
<A HREF=atSlalib.html#atVFitxy>atVFitxy</A> in atSlalib.c
<P>
Fit a linear model to relate the two sets of points that have already
been matched.
(vxe,vye) are the expected positions of the points and 
(vxm,vym) are the measured positions of the points. nfit must be 4 for
solid body fit, or 6 for full linear fit.  The model returned 
in the TRANS structure transforms from the measured points to the expected 
points.  Only the affine terms are fit; the higher-order distortion and color
terms are set to 0.  The error flag is returned - 0 is okay.
</HTML>
  </AUTO>
*/
char g_vFitxy[] = "vFitxy";
ftclArgvInfo g_vFitxyTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
  "Fit a linear model to relate the two sets of points"},
  {"<trans>", FTCL_ARGV_STRING, NULL, NULL, 
   "the trans"},
  {"<vxe>", FTCL_ARGV_STRING, NULL, NULL, 
   "x expected"},
  {"-vxe", FTCL_ARGV_STRING, NULL, NULL, 
   "x expected error"},
  {"<vye>", FTCL_ARGV_STRING, NULL, NULL, 
   "y expected"},
  {"-vyee", FTCL_ARGV_STRING, NULL, NULL, 
   "y expected error"},
  {"<vxm>", FTCL_ARGV_STRING, NULL, NULL, 
   "x measured"},
  {"-vxme", FTCL_ARGV_STRING, NULL, NULL, 
   "x measured error"},
  {"<vym>", FTCL_ARGV_STRING, NULL, NULL, 
   "y measured"},
  {"-vxme", FTCL_ARGV_STRING, NULL, NULL, 
   "x measured error"},
  {"-mask", FTCL_ARGV_STRING, NULL, NULL, 
   "mask values; use pt only if set to 1"},
  {"<nfit>", FTCL_ARGV_INT, NULL, NULL, 
   "4 or 6 to determine type of fit"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVFitxy(ClientData cd, Tcl_Interp *interp, int argc, char **argv) {
  int rstat;
  VECTOR *vxe=NULL, *vye=NULL, *vxm=NULL, *vym=NULL;
  VECTOR *vxee=NULL, *vyee=NULL, *vxme=NULL, *vyme=NULL;
  VECTOR *mask=NULL;
  char *a_vxe=NULL;
  char *a_vye=NULL;
  char *a_vxm=NULL;
  char *a_vym=NULL;
  char *a_vxee=NULL;
  char *a_vyee=NULL;
  char *a_vxme=NULL;
  char *a_vyme=NULL;
  char *a_mask=NULL;
  char *a_trans=NULL;
  char sflag[11];
  int flag, nfit;
  TRANS *trans=NULL;
  shErrStackClear();

  /* get the four VECTOR names and addresses */

  g_vFitxyTbl[1].dst = &a_trans;
  g_vFitxyTbl[2].dst = &a_vxe;
  g_vFitxyTbl[3].dst = &a_vxee;
  g_vFitxyTbl[4].dst = &a_vye;
  g_vFitxyTbl[5].dst = &a_vyee;
  g_vFitxyTbl[6].dst = &a_vxm;
  g_vFitxyTbl[7].dst = &a_vxme;
  g_vFitxyTbl[8].dst = &a_vym;
  g_vFitxyTbl[9].dst = &a_vyme;
  g_vFitxyTbl[10].dst = &a_mask;
  g_vFitxyTbl[11].dst = &nfit;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_vFitxyTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_vFitxy)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  if (shTclAddrGetFromName(interp, a_trans, (void **) &trans, "TRANS")
      != TCL_OK){
    Tcl_SetResult(interp, "tclVFitxy:  bad trans name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, a_vxe, (void **) &vxe, "VECTOR") != TCL_OK){
    Tcl_SetResult(interp, "tclVFitxy:  bad vxe name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, a_vye, (void **) &vye, "VECTOR") != TCL_OK){
    Tcl_SetResult(interp, "tclVFitxy:  bad vye name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, a_vxm, (void **) &vxm, "VECTOR") != TCL_OK){
    Tcl_SetResult(interp, "tclVFitxy:  bad vxm name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, a_vym, (void **) &vym, "VECTOR") != TCL_OK){
    Tcl_SetResult(interp, "tclVFitxy:  bad vym name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  
  /* and the errors */
  
  if(a_vxee!=NULL) {
    if (shTclAddrGetFromName(interp, a_vxee, (void **) &vxee, "VECTOR") 
	!= TCL_OK){
      Tcl_SetResult(interp, "tclVFitxy:  bad vxee name", TCL_VOLATILE);
      return TCL_ERROR;
    }
  }
  if(a_vxee!=NULL) {
    if (shTclAddrGetFromName(interp, a_vyee, (void **) &vyee, "VECTOR")
	!= TCL_OK){
      Tcl_SetResult(interp, "tclVFitxy:  bad vyee name", TCL_VOLATILE);
      return TCL_ERROR;
    }
  }
  if(a_vxee!=NULL) {
    if (shTclAddrGetFromName(interp, a_vxme, (void **) &vxme, "VECTOR")
	!= TCL_OK){
      Tcl_SetResult(interp, "tclVFitxy:  bad vxme name", TCL_VOLATILE);
      return TCL_ERROR;
    }
  }
  if(a_vxee!=NULL) {
    if (shTclAddrGetFromName(interp, a_vyme, (void **) &vyme, "VECTOR") 
	!= TCL_OK){
      Tcl_SetResult(interp, "tclVFitxy:  bad vyme name", TCL_VOLATILE);
      return TCL_ERROR;
    }
  }
  if (a_mask != NULL) {
    if (shTclAddrGetFromName(interp, a_mask, (void **) &mask, "VECTOR") 
	!= TCL_OK){
      Tcl_SetResult(interp, "tclVFitxy:  bad mask name", TCL_VOLATILE);
      return TCL_ERROR;
    }
  }
  if (nfit!=4 && nfit!=6) {
    Tcl_SetResult(interp, "tclVFitxy: nfit must be 4 or 6", TCL_VOLATILE);
    return TCL_ERROR;
  }
  /* make the actual call */
  flag = atVFitxy(vxe, vye, vxm, vym, vxee, vyee, vxme, vyme, 
		  mask, trans, nfit);
  sprintf(sflag,"%d\n", flag);
  Tcl_SetResult(interp, sflag, TCL_VOLATILE);

  if (flag == 0) {
    return TCL_OK;
  } else {
    return TCL_ERROR;
  }
}

/****************************************************************************/
/*
 * VTrans
 */
/*<AUTO EXTRACT>
  TCL VERB: vTrans
<HTML>
   Applies a TRANS to two VECTOR's containing the measured x and y positions.
   Only the affine terms are applied; the higher-order distortion and color
   terms are ignored.

C ROUTINE CALLED:
<A HREF=atSlalib.html#atVTrans>atVTrans</A> in atSlalib.c
</HTML>
  </AUTO>
*/
char g_vTrans[] = "vTrans";
ftclArgvInfo g_vTransTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
  "Use the trans to transform the (x,y) pairs in \n"
  "vxm and vym.  The values in the VECTORs are transformed \n"
  "in the same VECTORs and \n"
  "the errors are set to zero."},
  {"<trans>", FTCL_ARGV_STRING, NULL, NULL, 
   "the trans"},
  {"<vxm>", FTCL_ARGV_STRING, NULL, NULL, 
   "x values"},
  {"-vxme", FTCL_ARGV_STRING, NULL, NULL, 
   "x errors"},
  {"<vym>", FTCL_ARGV_STRING, NULL, NULL, 
   "y values"},
  {"-vyme", FTCL_ARGV_STRING, NULL, NULL, 
   "y errors"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVTrans(ClientData cd, Tcl_Interp *interp, int argc, char **argv) {
  int rstat;
  VECTOR *vxm=NULL, *vym=NULL, *vxme=NULL, *vyme=NULL;
  char *a_vxm=NULL;
  char *a_vym=NULL;
  char *a_vxme=NULL;
  char *a_vyme=NULL;
  TRANS *trans=NULL;
  char *a_trans=NULL;
  shErrStackClear();

  g_vTransTbl[1].dst = &a_trans;
  g_vTransTbl[2].dst = &a_vxm;
  g_vTransTbl[3].dst = &a_vxme;
  g_vTransTbl[4].dst = &a_vym;
  g_vTransTbl[5].dst = &a_vyme;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_vTransTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_vTrans)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, 
			   a_trans, (void **) &trans, "TRANS") != TCL_OK){
    Tcl_SetResult(interp, "tclVTrans:  bad trans name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, a_vxm, (void **) &vxm, "VECTOR") != TCL_OK){
    Tcl_SetResult(interp, "tclVTrans:  bad vxm name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, a_vym, (void **) &vym, "VECTOR") != TCL_OK){
    Tcl_SetResult(interp, "tclVTrans:  bad vym name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if(a_vxme==NULL) {
    vxme=NULL;
  } else {
    if (shTclAddrGetFromName(interp, a_vxme, (void **) &vxme, "VECTOR") 
	!= TCL_OK){
      Tcl_SetResult(interp, "tclVTrans:  bad vxme name", TCL_VOLATILE);
      return TCL_ERROR;
    }
  }
  if(a_vyme==NULL) {
    vyme=NULL;
  } else {
    if (shTclAddrGetFromName(interp, a_vyme, (void **) &vyme, "VECTOR") 
	!= TCL_OK){
      Tcl_SetResult(interp, "tclVTrans:  bad vyme name", TCL_VOLATILE);
      return TCL_ERROR;
    }
  }

  /* make the call and see if it worked */
  if (atVTrans(trans, vxm,vxme, vym,vyme) != SH_SUCCESS) {
    shTclInterpAppendWithErrStack(interp);
    return TCL_ERROR;
  }

  Tcl_SetResult(interp, "", TCL_VOLATILE);
  Tcl_AppendResult(interp, a_vxm, (char *) NULL);
  Tcl_AppendResult(interp, " ", (char *) NULL);
  Tcl_AppendResult(interp, a_vxme, (char *) NULL);
  Tcl_AppendResult(interp, " ", (char *) NULL);
  Tcl_AppendResult(interp, a_vym, (char *) NULL);
  Tcl_AppendResult(interp, " ", (char *) NULL);
  Tcl_AppendResult(interp, a_vyme, (char *) NULL);
  return TCL_OK;
}

/****************************************************************************/
/*
 * Refro: Refraction correction calculation
 */
/*<AUTO EXTRACT>
  TCL VERB: atRefro
<HTML>
C ROUTINE CALLED:
slaRefro in slalib library
</HTML>
  </AUTO>
*/
char g_refro[] = "atRefro";
ftclArgvInfo g_refroTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL, "Calculate a refraction correction"},
  {"<zd>", FTCL_ARGV_DOUBLE, NULL, NULL, "zenith dist (degrees)"},
  {"<wavelength>", FTCL_ARGV_DOUBLE, NULL, NULL, "effective wavelength (angstroms)"},
  {"-height", FTCL_ARGV_DOUBLE, NULL, NULL, "above sea level meters (default 3000m)"},
  {"-temp", FTCL_ARGV_DOUBLE, NULL, NULL, "temp (K) (default 300)"},
  {"-pressure", FTCL_ARGV_DOUBLE, NULL, NULL, "pressure (mbar) (default 700)"},
  {"-humidity", FTCL_ARGV_DOUBLE, NULL, NULL, "humidity (frac) (default 0.3)"},
  {"-latitude", FTCL_ARGV_DOUBLE, NULL, NULL, "latitude (degrees) (default 32.8)"},
  {"-lapse", FTCL_ARGV_DOUBLE, NULL, NULL, "temp lapse K/m (default 0.0065)"},
  {"-err", FTCL_ARGV_DOUBLE, NULL, NULL, "precision of calc (default 1d-10)"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclRefro(ClientData cd, Tcl_Interp *interp, int argc, char **argv) {
  
  int rstat;
  char result[80];
  double zd;
  double wavelength;
  double height=3000.0;
  double temp= 300.0;
  double pressure = 700.0;
  double humidity = 0.3;
  double latitude = 32.8;
  double lapse = 0.0065;
  double err = 1.0e-10;

  double refraction;

  shErrStackClear();

  g_refroTbl[1].dst = &zd;
  g_refroTbl[2].dst = &wavelength;
  g_refroTbl[3].dst = &height;
  g_refroTbl[4].dst = &temp;
  g_refroTbl[5].dst = &pressure;
  g_refroTbl[6].dst = &humidity;
  g_refroTbl[6].dst = &latitude;
  g_refroTbl[7].dst = &lapse;
  g_refroTbl[8].dst = &err;


  if ((rstat = shTclParseArgv(interp, &argc, argv, g_refroTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_refro)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  zd *= at_deg2Rad;
  latitude *= at_deg2Rad;
  wavelength /= 10000.0; /* convert to microns */
  slaRefro(zd,height,temp,pressure,humidity,wavelength,latitude,lapse,err,&refraction);
  refraction *= at_rad2Deg;
  sprintf(result,"%.8f",refraction);
  Tcl_SetResult(interp,result, TCL_VOLATILE);
  return TCL_OK;
}


/****************************************************************************/
/*
 * Oap: Observed to Apparent positions
 */
/*<AUTO EXTRACT>
  TCL VERB: atOap
<HTML>
C ROUTINE CALLED:
slaOap in slalib library
</HTML>
  </AUTO>
*/
char g_oap[] = "atOap";
ftclArgvInfo g_oapTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL, "Calculate apparent position from observed"},
  {"<type>", FTCL_ARGV_STRING, NULL, NULL, "Is first coord RA (R or r), HA (H or h) or Az (A or a)"},
  {"<obs1>", FTCL_ARGV_DOUBLE, NULL, NULL, "RA, HA, or Az (degrees)"},
  {"<obs2>", FTCL_ARGV_DOUBLE, NULL, NULL, "DEC (R|r|H|h) or Zenith Dist (A|a) (degrees)"},
  {"<date>", FTCL_ARGV_DOUBLE, NULL, NULL, "Date (mjd)"},
  {"-delut", FTCL_ARGV_DOUBLE, NULL, NULL, "UT1-UTC (UTC seconds), default=0.0 (which is wrong)"},
  {"-longitude", FTCL_ARGV_DOUBLE, NULL, NULL, "west (+ve) longitude of observatory, at_site_longitude"},
  {"-latitude", FTCL_ARGV_DOUBLE, NULL, NULL, "latitude (degrees) at_site_latitude"},
  {"-height", FTCL_ARGV_DOUBLE, NULL, NULL, "above sea level meters (default at_site_altitude)"},
  {"-xp", FTCL_ARGV_DOUBLE, NULL, NULL, "x polar motion (default = 0.0)"},
  {"-yp", FTCL_ARGV_DOUBLE, NULL, NULL, "y polar motion (default = 0.0)"},
  {"-temp", FTCL_ARGV_DOUBLE, NULL, NULL, "temp (K) (default 300)"},
  {"-pressure", FTCL_ARGV_DOUBLE, NULL, NULL, "pressure (mbar) (default 700)"},
  {"-humidity", FTCL_ARGV_DOUBLE, NULL, NULL, "humidity (frac) (default 0.3)"},
  {"-wavelength", FTCL_ARGV_DOUBLE, NULL, NULL, "effective wavelength, default=4770 A (g')"},

  {"-lapse", FTCL_ARGV_DOUBLE, NULL, NULL, "temp lapse K/m (default 0.0065)"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclOap(ClientData cd, Tcl_Interp *interp, int argc, char **argv) {
  
  int rstat;
  char result[80];
  char *type;
  double obs1;
  double obs2;
  double date;
  double delut = 0.0;
  /* This routine requires E+, unlike the others */
  double a_longitude =  -1.0*at_site_longitude;
  double a_latitude =  at_site_latitude;
  double height=3000.0;
  double xp=0.0;
  double yp=0.0;
  double temp= 300.0;
  double pressure = 700.0;
  double humidity = 0.3;
  double wavelength=4770.0;
  double lapse = 0.0065;

  double ra;
  double dec;

  shErrStackClear();

  g_oapTbl[1].dst = &type;
  g_oapTbl[2].dst = &obs1;
  g_oapTbl[3].dst = &obs2;
  g_oapTbl[4].dst = &date;
  g_oapTbl[5].dst = &delut;
  g_oapTbl[6].dst = &a_longitude;
  g_oapTbl[7].dst = &a_latitude;
  g_oapTbl[8].dst = &height;
  g_oapTbl[9].dst = &xp;
  g_oapTbl[10].dst = &yp;
  g_oapTbl[11].dst = &temp;
  g_oapTbl[12].dst = &pressure;
  g_oapTbl[13].dst = &humidity;
  g_oapTbl[14].dst = &wavelength;
  g_oapTbl[15].dst = &lapse;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_oapTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_oap)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  obs1 *= at_deg2Rad;
  obs2 *= at_deg2Rad;
  a_longitude *= at_deg2Rad;
  a_latitude *= at_deg2Rad;
  wavelength /= 10000.0; /* convert to microns */

  slaOap(type,obs1,obs2,date,delut,a_longitude,a_latitude,height,xp,yp,temp,pressure,humidity,wavelength,lapse,&ra,&dec);

  ra *= at_rad2Deg;
  dec *= at_rad2Deg;
  sprintf(result,"{appRa %.6f} {appDec %.6f}",ra,dec);
  Tcl_SetResult(interp,result, TCL_VOLATILE);
  return TCL_OK;
}


/****************************************************************************/
/*
 * Aop: Apparent to Observed positions
 */
/*<AUTO EXTRACT>
  TCL VERB: atAop
<HTML>
C ROUTINE CALLED:
slaAop in slalib library
</HTML>
  </AUTO>
*/
char g_aop[] = "atAop";
ftclArgvInfo g_aopTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL, "Calculate observed position from apparent"},
  {"<apra>", FTCL_ARGV_DOUBLE, NULL, NULL, "apparent RA (degrees)"},
  {"<apdec>", FTCL_ARGV_DOUBLE, NULL, NULL, "apparent DEC (degrees)"},
  {"<date>", FTCL_ARGV_DOUBLE, NULL, NULL, "Date (mjd)"},
  {"-delut", FTCL_ARGV_DOUBLE, NULL, NULL, "UT1-UTC (UTC seconds), default=0.0 (which is wrong)"},
  {"-longitude", FTCL_ARGV_DOUBLE, NULL, NULL, "west (+ve) longitude of observatory, at_site_longitude"},
  {"-latitude", FTCL_ARGV_DOUBLE, NULL, NULL, "latitude (degrees) at_site_latitude"},
  {"-height", FTCL_ARGV_DOUBLE, NULL, NULL, "above sea level meters (default at_site_altitude)"},
  {"-xp", FTCL_ARGV_DOUBLE, NULL, NULL, "x polar motion (default = 0.0)"},
  {"-yp", FTCL_ARGV_DOUBLE, NULL, NULL, "y polar motion (default = 0.0)"},
  {"-temp", FTCL_ARGV_DOUBLE, NULL, NULL, "temp (K) (default 300)"},
  {"-pressure", FTCL_ARGV_DOUBLE, NULL, NULL, "pressure (mbar) (default 700)"},
  {"-humidity", FTCL_ARGV_DOUBLE, NULL, NULL, "humidity (frac) (default 0.3)"},
  {"-wavelength", FTCL_ARGV_DOUBLE, NULL, NULL, "effective wavelength, default=4770 A (g')"},
  {"-lapse", FTCL_ARGV_DOUBLE, NULL, NULL, "temp lapse K/m (default 0.0065)"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclAop(ClientData cd, Tcl_Interp *interp, int argc, char **argv) {
  
  int rstat;
  char result[200];
  double apra;
  double apdec;
  double date;
  double delut = 0.0;
  /* This routine requires E+, unlike the others */
  double a_longitude =  -1.0*at_site_longitude;
  double a_latitude =  at_site_latitude;
  double height=3000.0;
  double xp=0.0;
  double yp=0.0;
  double temp= 300.0;
  double pressure = 700.0;
  double humidity = 0.3;
  double wavelength=4770.0;
  double lapse = 0.0065;

  double obsaz;
  double obszd;
  double obsha;
  double obsra;
  double obsdec;

  shErrStackClear();

  g_aopTbl[1].dst = &apra;
  g_aopTbl[2].dst = &apdec;
  g_aopTbl[3].dst = &date;
  g_aopTbl[4].dst = &delut;
  g_aopTbl[5].dst = &a_longitude;
  g_aopTbl[6].dst = &a_latitude;
  g_aopTbl[7].dst = &height;
  g_aopTbl[8].dst = &xp;
  g_aopTbl[9].dst = &yp;
  g_aopTbl[10].dst = &temp;
  g_aopTbl[11].dst = &pressure;
  g_aopTbl[12].dst = &humidity;
  g_aopTbl[13].dst = &wavelength;
  g_aopTbl[14].dst = &lapse;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_aopTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_aop)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  apra *= at_deg2Rad;
  apdec *= at_deg2Rad;
  a_longitude *= at_deg2Rad;
  a_latitude *= at_deg2Rad;
  wavelength /= 10000.0; /* convert to microns */

  slaAop(apra,apdec,date,delut,a_longitude,a_latitude,height,xp,yp,temp,pressure,humidity,wavelength,lapse,&obsaz,&obszd,&obsha,&obsdec,&obsra);

  obsaz *= at_rad2Deg;
  obszd *= at_rad2Deg;
  obsha *= at_rad2Deg;
  obsra *= at_rad2Deg;
  obsdec *= at_rad2Deg;

  sprintf(result,"{obsAz %.6f} {obsZd %.6f} {obsHa %.6f} {obsRa %.6f} {obsDec %.6f}",obsaz,obszd,obsha,obsra,obsdec);
  Tcl_SetResult(interp,result, TCL_VOLATILE);
  return TCL_OK;
}


/****************************************************************************/
/*
 * Mean to Apparent Conversion
 */
/*<AUTO EXTRACT>
  TCL VERB: atMean2Apparent
<HTML>
C ROUTINE CALLED:
slaMap in slalib library
</HTML>
  </AUTO>
*/
char g_mean2apparent[] = "atMean2Apparent";
ftclArgvInfo g_mean2apparentTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,"Mean To Apparent coord conversion"},
  {"<ra>", FTCL_ARGV_DOUBLE, NULL, NULL, "mean ra (degrees)"},
  {"<dec>", FTCL_ARGV_DOUBLE, NULL, NULL, "mean dec (degrees)"},
  {"<tstampName>", FTCL_ARGV_STRING, NULL, NULL, "tstamp giving time and date"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclMean2Apparent(ClientData cd, Tcl_Interp *interp, int argc, char **argv) {
  int rstat;
  char result[200];
  char *tstampName;
  double ra,dec,appra,appdec,mjd;
  TSTAMP *tstamp = NULL;
  shErrStackClear();

  g_mean2apparentTbl[1].dst = &ra;
  g_mean2apparentTbl[2].dst = &dec;
  g_mean2apparentTbl[3].dst = &tstampName;


  if ((rstat = shTclParseArgv(interp, &argc, argv, g_mean2apparentTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_mean2apparent)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, 
			   tstampName, (void **) &tstamp, "TSTAMP") != TCL_OK){
    Tcl_SetResult(interp, "mean2apparent:  bad tstamp name", TCL_VOLATILE);
    return TCL_ERROR;
  }

  mjd = atTstampToMJD(tstamp);
	ra *= at_deg2Rad;
	dec *= at_deg2Rad;
  slaMap (ra,dec,0.0,0.0,0.0,0.0,2000.0,mjd,&appra,&appdec);
  sprintf(result,"{appRa %.6f} {appDec %.6f}",at_rad2Deg*appra,at_rad2Deg*appdec);
  Tcl_SetResult(interp, result, TCL_VOLATILE);

  return TCL_OK;
}

/****************************************************************************/
/*
 * Apply annual aberration
 */
/*<AUTO EXTRACT>
  TCL VERB: atAberrationApply
<HTML>
C ROUTINE CALLED:
<A HREF=atSlalib.html#atAberrationApply>atAberrationApply</A> in atSlalib.c
</HTML>
  </AUTO>
*/
char g_aberrationApply[] = "atAberrationApply";
ftclArgvInfo g_aberrationApplyTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL, 
   "Find and apply delta position due to annual aberration"},
  {"<mjd>", FTCL_ARGV_DOUBLE, NULL, NULL, "Modified Julian Date"},
  {"<epoch>", FTCL_ARGV_DOUBLE, NULL, NULL, "epoch (negative for FK5 at mjd)"},
  {"<ra>", FTCL_ARGV_DOUBLE, NULL, NULL, "mean ra (degrees)"},
  {"<dec>", FTCL_ARGV_DOUBLE, NULL, NULL, "mean dec (degrees)"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclAberrationApply(ClientData cd, Tcl_Interp *interp, int argc, char **argv) {
  int rstat;
  char result[200];
  double mjd, epoch, raIn, decIn;
  double ra, dec;
  shErrStackClear();

  g_aberrationApplyTbl[1].dst = &mjd;
  g_aberrationApplyTbl[2].dst = &epoch;
  g_aberrationApplyTbl[3].dst = &raIn;
  g_aberrationApplyTbl[4].dst = &decIn;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_aberrationApplyTbl,
			      FTCL_ARGV_NO_LEFTOVERS, g_aberrationApply)) !=
      FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  ra    = at_deg2Rad * raIn;
  dec   = at_deg2Rad * decIn;
  atAberrationApply(mjd, epoch, &ra, &dec);
  sprintf(result, "{ra %.9f} {dec %.9f}",
	  at_rad2Deg*(ra), at_rad2Deg*(dec));
  Tcl_SetResult(interp, result, TCL_VOLATILE);
  return TCL_OK;
}

/****************************************************************************/
/*
 * Apply annual aberration
 */
/*<AUTO EXTRACT>
  TCL VERB: atVAberrationApply
<HTML>
C ROUTINE CALLED:
<A HREF=atSlalib.html#atVAberrationApply>atVAberrationApply</A> in atSlalib.c
</HTML>
  </AUTO>
*/
char g_VAberrationApply[] = "atVAberrationApply";
ftclArgvInfo g_VAberrationApplyTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL, 
   "Find and apply delta position due to annual aberration"},
  {"<mjd>", FTCL_ARGV_DOUBLE, NULL, NULL, "Modified Julian Date"},
  {"<epoch>", FTCL_ARGV_DOUBLE, NULL, NULL, "epoch (negative for FK5 at mjd)"},
  {"<ra>", FTCL_ARGV_STRING, NULL, NULL, "VECTOR of mean ras (degrees)"},
  {"<dec>", FTCL_ARGV_STRING, NULL, NULL, "VECTOR of mean decs (degrees)"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVAberrationApply(ClientData cd, Tcl_Interp *interp, int argc, char **argv) {
  int rstat;
  double mjd, epoch;

  VECTOR* raIn = NULL;
  VECTOR* decIn = NULL;

  char *a_raIn = NULL;
  char *a_decIn = NULL;

  shErrStackClear();

  g_VAberrationApplyTbl[1].dst = &mjd;
  g_VAberrationApplyTbl[2].dst = &epoch;
  g_VAberrationApplyTbl[3].dst = &a_raIn;
  g_VAberrationApplyTbl[4].dst = &a_decIn;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_VAberrationApplyTbl,
			      FTCL_ARGV_NO_LEFTOVERS, g_VAberrationApply)) !=
      FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_raIn, (void **) &raIn, "VECTOR") 
      != TCL_OK) {
    Tcl_SetResult(interp, "atVAberrationApply:  bad raIn name", TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (shTclAddrGetFromName(interp, a_decIn, (void **) &decIn, "VECTOR") 
      != TCL_OK) {
    Tcl_SetResult(interp, "atVAberrationApply:  bad decIn name", TCL_VOLATILE);
    return TCL_ERROR;
  }

  atVAberrationApply(mjd, epoch, raIn, decIn);
  return TCL_OK;
}

/****************************************************************************/
/*
 * Get annual aberration
 */
/*<AUTO EXTRACT>
  TCL VERB: atAberrationGet
<HTML>
C ROUTINE CALLED:
<A HREF=atSlalib.html#atAberrationGet>atAberrationGet</A> in atSlalib.c
</HTML>
  </AUTO>
*/
char g_aberrationGet[] = "atAberrationGet";
ftclArgvInfo g_aberrationGetTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL, 
   "Find delta position due to annual aberration"},
  {"<vx>", FTCL_ARGV_DOUBLE, NULL, NULL, "x velocity (units of c)"},
  {"<vy>", FTCL_ARGV_DOUBLE, NULL, NULL, "y velocity (units of c)"},
  {"<vz>", FTCL_ARGV_DOUBLE, NULL, NULL, "z velocity (units of c)"},
  {"<ra>", FTCL_ARGV_DOUBLE, NULL, NULL, "mean ra (degrees)"},
  {"<dec>", FTCL_ARGV_DOUBLE, NULL, NULL, "mean dec (degrees)"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclAberrationGet(ClientData cd, Tcl_Interp *interp, int argc, char **argv) {
  int rstat;
  char result[200];
  double vx, vy, vz, raIn, decIn;
  double ra, dec, delRa, delDec;
  shErrStackClear();

  g_aberrationGetTbl[1].dst = &vx;
  g_aberrationGetTbl[2].dst = &vy;
  g_aberrationGetTbl[3].dst = &vz;
  g_aberrationGetTbl[4].dst = &raIn;
  g_aberrationGetTbl[5].dst = &decIn;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_aberrationGetTbl,
			      FTCL_ARGV_NO_LEFTOVERS, g_aberrationGet)) !=
      FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  ra    = at_deg2Rad * raIn;
  dec   = at_deg2Rad * decIn;
  atAberrationGet(vx, vy, vz, ra, dec, &delRa, &delDec);
  sprintf(result, "{delRa %.9f} {delDec %.9f}",
	  at_rad2Deg*(delRa), at_rad2Deg*(delDec));
  Tcl_SetResult(interp, result, TCL_VOLATILE);
  return TCL_OK;
}

/****************************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: atAzEl2HaDec
<HTML>
Convert azimuth and elevation to hour angle and declination.  For example,
if the azimuth and elevation are for an actual pointing of a telescope, this
will return the observed place hour angle and declination.  If the effects of
refraction have been removed from the elevation, then the apparent
topocentric hour angle and declination are returned.  This is simply
a wrapper for SLALIB's slaDH2E().  It returns the hour angle and declination
(both in degrees) as a TCL keyed list (with keys "ha" and "dec",
respectively).
<p>
C ROUTINE CALLED:
slaDh2e in slalib library
</HTML>
  </AUTO>
*/
char g_azEl2HaDec[] = "atAzEl2HaDec";
ftclArgvInfo g_azEl2HaDecTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,"(Azimuth,elevation) to (hour angle,declination) conversion"},
  {"<az>", FTCL_ARGV_DOUBLE, NULL, NULL, "azimuth (degrees, north=0, east=+90)"},
  {"<el>", FTCL_ARGV_DOUBLE, NULL, NULL, "elevation (degrees"},
  {"<latitude>", FTCL_ARGV_DOUBLE, NULL, NULL, "geodetic latitude (degrees)"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclAzEl2HaDec(ClientData cd, Tcl_Interp *interp, int argc, char **argv) {
  int rstat;
  char result[200];
  double az,el,latitude,ha,dec;

  g_azEl2HaDecTbl[1].dst = &az;
  g_azEl2HaDecTbl[2].dst = &el;
  g_azEl2HaDecTbl[3].dst = &latitude;


  if ((rstat = shTclParseArgv(interp, &argc, argv, g_azEl2HaDecTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_azEl2HaDec)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  az *= at_deg2Rad;
  el *= at_deg2Rad;
  latitude *= at_deg2Rad;
  slaDh2e (az,el,latitude,&ha,&dec);
  sprintf(result,"{ha %.6f} {dec %.6f}",at_rad2Deg*ha,at_rad2Deg*dec);
  Tcl_SetResult(interp, result, TCL_VOLATILE);

  return TCL_OK;
}

/****************************************************************************/
/*
 * velFromTstamp
 */

/*<AUTO EXTRACT>
  TCL VERB: velFromTstamp
<HTML>
C ROUTINE CALLED:
<A HREF=atSlalib.html#atVelocityFromTstamp>atVelocityFromTstamp</A> in atSlalib.c
</HTML>
</AUTO>
*/

char g_velFromTstamp[] = "velFromTstamp";
ftclArgvInfo g_velFromTstampTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Find the geocentric apparent velocity of the APO observatory at TSTAMP" },
  {"<tstamp>", FTCL_ARGV_STRING, NULL, NULL, 
   "time to get the velocity"},
  {"<vel>", FTCL_ARGV_STRING, NULL, NULL, 
   "a vector of dimension 6 which will contain the x,y,z geocentric apparent position (in AU) and velocity, in AU/sec at x, y, z, xdot, ydot, zdot"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclvelFromTstamp(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  TSTAMP *tstamp = NULL;
  VECTOR *vel = NULL;
  int rstat;

  char *a_tstamp = NULL;
  char *a_vel = NULL;

  g_velFromTstampTbl[1].dst = &a_tstamp;
  g_velFromTstampTbl[2].dst = &a_vel;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_velFromTstampTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_velFromTstamp)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_tstamp, (void **) &tstamp, "TSTAMP")
      != TCL_OK) {
    Tcl_SetResult(interp, "velFromTstamp:  bad tstamp name", TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (shTclAddrGetFromName(interp, a_vel, (void **) &vel, "VECTOR")
      != TCL_OK) {
    Tcl_SetResult(interp, "velFromTstamp:  bad vel name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if ((rstat = atVelocityFromTstamp(tstamp, vel)) != SH_SUCCESS) {
    return TCL_ERROR;
  } else {
    return TCL_OK;
  }
}

/****************************************************************************/
/*
 * meanToApparent
 */

/*<AUTO EXTRACT>
  TCL VERB: meanToApparent
<HTML>
C ROUTINE CALLED:
<A HREF=atSlalib.html#atMeanToApparent>atMeanToApparent</A> in atSlalib.c
</HTML>
</AUTO>
*/

char g_meanToApparent[] = "meanToApparent";
ftclArgvInfo g_meanToApparentTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Convert mean to apparent place ignoring parallax and proper motion.  Returns a keyed list of ra and dec, the apparent place" },
  {"-epoch", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "epoch of mean equinox to be used (Julian) defaults to J2000.0"},
  {"<mjd>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "when to do the transformation"},
  {"<ra>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "mean ra (degrees)"},
  {"<dec>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "mean dec (degrees)"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclMeanToApparent(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  double mjd = 0.0;
  double epoch = 2000.0;
  double ra = 0.0;
  double dec = 0.0;
  int rstat;
  char answer[100];
  g_meanToApparentTbl[1].dst = &epoch;
  g_meanToApparentTbl[2].dst = &mjd;
  g_meanToApparentTbl[3].dst = &ra;
  g_meanToApparentTbl[4].dst = &dec;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_meanToApparentTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_meanToApparent)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  /* convert to radians */

  ra *= at_deg2Rad;
  dec *= at_deg2Rad;
  /* do the work */
  atMeanToApparent(epoch, mjd, &ra, &dec);
  /* convert to degrees */
  ra *= at_rad2Deg;
  dec *= at_rad2Deg;
  sprintf(answer,"ra %20.17f", ra);

  /* do the tcl stuff */
  Tcl_AppendElement(interp, answer);
  sprintf(answer,"dec %20.17f", dec);
  Tcl_AppendElement(interp, answer);
  return TCL_OK;

}
/****************************************************************************/
/*
 * pmApply
 */

/*<AUTO EXTRACT>
  TCL VERB: pmApply
<HTML>
C ROUTINE CALLED:
slaPm in slalib library
</HTML>
</AUTO>
*/

char g_pmApply[] = "pmApply";
ftclArgvInfo g_pmApplyTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Apply proper motion" },
  {"<r0>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "initial ra (degrees)"},
  {"<d0>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "initial dec (degrees)"},
  {"<pr>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "ra proper motion (degrees/year)"},
  {"<pd>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "dec proper motion (degrees/year)"},
  {"-px", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "parallax (arcsec)"},
  {"-rv", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "radial velocity km/sec (+ve if receding)"},
  {"<mjd0>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "start epoch as an MJD"},
  {"<mjd1>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "end epoch as an MJD"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclpmApply(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  double r0 = 0.0;
  double d0 = 0.0;
  double pr = 0.0;
  double pd = 0.0;
  double px = 0.0;
  double rv = 0.0;
  double mjd0 = 0.0;
  double mjd1 = 0.0;
  double r1 = 0.0;
  double d1 = 0.0;
  double ep0, ep1;

  int rstat;
  char answer[100];
  g_pmApplyTbl[1].dst = &r0;
  g_pmApplyTbl[2].dst = &d0;
  g_pmApplyTbl[3].dst = &pr;
  g_pmApplyTbl[4].dst = &pd;
  g_pmApplyTbl[5].dst = &px;
  g_pmApplyTbl[6].dst = &rv;
  g_pmApplyTbl[7].dst = &mjd0;
  g_pmApplyTbl[8].dst = &mjd1;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pmApplyTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pmApply)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  /* convert to radians */

  r0 *= at_deg2Rad;
  d0 *= at_deg2Rad;
  pr *= at_deg2Rad;
  pd *= at_deg2Rad;

  /* do the work */
  ep0 = slaEpj(mjd0);
  ep1 = slaEpj(mjd1);
  slaPm(r0, d0, pr, pd, px, rv, ep0, ep1, &r1, &d1);
  /* convert to degrees */
  r1 *= at_rad2Deg;
  d1 *= at_rad2Deg;

  /* do the tcl stuff */
  sprintf(answer,"ra %20.17f", r1);
  Tcl_AppendElement(interp, answer);
  sprintf(answer,"dec %20.17f", d1);
  Tcl_AppendElement(interp, answer);
  return TCL_OK;

}

/****************************************************************************/
/*
 * epj2mjd
 */

/*<AUTO EXTRACT>
  TCL VERB: epj2mjd
<HTML>
C ROUTINE CALLED:
slaEpj2d in slalib library
</HTML>
</AUTO>
*/

char g_epj2mjd[] = "epj2mjd";
ftclArgvInfo g_epj2mjdTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Convert Julian Epoch to MJD" },
  {"<epj>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "Julian Epoch"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclepj2mjd(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  double epj = 0.0;
  double mjd;
  int rstat;
  char answer[100];
  g_epj2mjdTbl[1].dst = &epj;
  if ((rstat = shTclParseArgv(interp, &argc, argv, g_epj2mjdTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_epj2mjd)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  mjd = slaEpj2d(epj);
  sprintf(answer,"%.17f", mjd);
  Tcl_AppendElement(interp, answer);
  return TCL_OK;

}

/****************************************************************************/
/*
 * mjd2epj
 */

/*<AUTO EXTRACT>
  TCL VERB: mjd2epj
<HTML>
C ROUTINE CALLED:
slaEpj in slalib library
</HTML>
</AUTO>
*/

char g_mjd2epj[] = "mjd2epj";
ftclArgvInfo g_mjd2epjTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Convert MJD to Julian Epoch" },
  {"<mjd>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "Modified Julian Date"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclmjd2epj(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  double mjd = 0.0;
  double epj;
  int rstat;
  char answer[100];
  g_mjd2epjTbl[1].dst = &mjd;
  if ((rstat = shTclParseArgv(interp, &argc, argv, g_mjd2epjTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_mjd2epj)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  epj = slaEpj(mjd);
  sprintf(answer,"%.17f", epj);
  Tcl_AppendElement(interp, answer);
  return TCL_OK;

}
/****************************************************************************/
/*
 * datFromMjd
 */

/*<AUTO EXTRACT>
  TCL VERB: datFromMjd
<HTML>
C ROUTINE CALLED:
slaDat in slalib library
</HTML>
</AUTO>
*/

char g_datFromMjd[] = "datFromMjd";
ftclArgvInfo g_datFromMjdTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Convert MJD to Julian Epoch" },
  {"<mjd>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "Modified Julian Date"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tcldatFromMjd(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  double mjd = 0.0;
  double dat;
  int rstat;
  char answer[100];
  g_datFromMjdTbl[1].dst = &mjd;
  if ((rstat = shTclParseArgv(interp, &argc, argv, g_datFromMjdTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_datFromMjd)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  dat = slaDat(mjd);
  sprintf(answer,"%.17f", dat);
  Tcl_AppendElement(interp, answer);
  return TCL_OK;

}
/***************************************************************************/
/*
 * Declare tcl verbs
 */
void atTclSlalibDeclare(Tcl_Interp *interp) {
   int flags = FTCL_ARGV_NO_LEFTOVERS;

   shTclDeclare(interp,"radFromDeg", 
		(Tcl_CmdProc *)tclRadFromDeg,
		(ClientData) 0, (Tcl_CmdDeleteProc *)NULL,
		module, tclRadFromDeg_hlp, tclRadFromDeg_use);

   shTclDeclare(interp,"radToDeg", (Tcl_CmdProc *)tclRadToDeg,
		(ClientData) 0, (Tcl_CmdDeleteProc *)NULL,
		module, tclRadToDeg_hlp, tclRadToDeg_use);

   shTclDeclare(interp,"lbToXyz", 
		(Tcl_CmdProc *)tclLbToXyz,
		(ClientData) 0, (Tcl_CmdDeleteProc *)NULL,
		module, tclLbToXyz_hlp, tclLbToXyz_use);

   shTclDeclare(interp,"xyzToLb", 
		(Tcl_CmdProc *)tclXyzToLb,
		(ClientData) 0, (Tcl_CmdDeleteProc *)NULL,
		module, tclXyzToLb_hlp, tclXyzToLb_use);

   shTclDeclare(interp,"dayFromTime", (Tcl_CmdProc *)tclDayFromTime,
                (ClientData) 0, (Tcl_CmdDeleteProc *)NULL,
                module, tclDayFromTime_hlp, tclDayFromTime_use);

   shTclDeclare(interp,"coordPrecess", (Tcl_CmdProc *)tclCoordPrecess,
                (ClientData) 0, (Tcl_CmdDeleteProc *)NULL,
                module, tclCoordPrecess_hlp, tclCoordPrecess_use);

   shTclDeclare(interp,g_coordPreNut, 
		(Tcl_CmdProc *)tclCoordPreNut,
                (ClientData) 0, (Tcl_CmdDeleteProc *)NULL, tclFacil, 
		shTclGetArgInfo(interp, g_coordPreNutTbl, flags, g_coordPreNut),
		shTclGetUsage(interp, g_coordPreNutTbl, flags, g_coordPreNut));

   shTclDeclare(interp,g_earthPosVel, 
		(Tcl_CmdProc *)tclEarthPosVel,
                (ClientData) 0, (Tcl_CmdDeleteProc *)NULL, tclFacil, 
		shTclGetArgInfo(interp, g_earthPosVelTbl, flags, g_earthPosVel),
		shTclGetUsage(interp, g_earthPosVelTbl, flags, g_earthPosVel));

   shTclDeclare(interp,"radFromDMS", (Tcl_CmdProc *)tclRadFromDMS,
                (ClientData) 0, (Tcl_CmdDeleteProc *)NULL,
                module, tclRadFromDMS_hlp, tclRadFromDMS_use);

   shTclDeclare(interp,"radFromHMS", (Tcl_CmdProc *)tclRadFromHMS,
                (ClientData) 0, (Tcl_CmdDeleteProc *)NULL,
                module, tclRadFromHMS_hlp, tclRadFromHMS_use);

   shTclDeclare(interp,"radToDMS", (Tcl_CmdProc *)tclRadToDMS,
                (ClientData) 0, (Tcl_CmdDeleteProc *)NULL,
                module, tclRadToDMS_hlp, tclRadToDMS_use);

   shTclDeclare(interp,"radToHMS", (Tcl_CmdProc *)tclRadToHMS,
                (ClientData) 0, (Tcl_CmdDeleteProc *)NULL,
                module, tclRadToHMS_hlp, tclRadToHMS_use);

   shTclDeclare(interp,"degToDMS", (Tcl_CmdProc *)tclDegToDMS,
                (ClientData) 0, (Tcl_CmdDeleteProc *)NULL,
                module, tclDegToDMS_hlp, tclDegToDMS_use);

   shTclDeclare(interp,"degToHMS", (Tcl_CmdProc *)tclDegToHMS,
                (ClientData) 0, (Tcl_CmdDeleteProc *)NULL,
                module, tclDegToHMS_hlp, tclDegToHMS_use);

   shTclDeclare(interp,"DMSToDeg", (Tcl_CmdProc *)tclDMSToDeg,
                (ClientData) 0, (Tcl_CmdDeleteProc *)NULL,
                module, tclDMSToDeg_hlp, tclDMSToDeg_use);

   shTclDeclare(interp,"HMSToDeg", (Tcl_CmdProc *)tclHMSToDeg,
                (ClientData) 0, (Tcl_CmdDeleteProc *)NULL,
                module, tclHMSToDeg_hlp, tclHMSToDeg_use);

   shTclDeclare(interp,"sepGet", (Tcl_CmdProc *)tclSepGet,
		(ClientData) 0, (Tcl_CmdDeleteProc *)NULL,
		module, tclSepGet_help, tclSepGet_use);

   shTclDeclare(interp, g_tstampNew, 
               (Tcl_CmdProc *) tclTstampNew,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_tstampNewTbl, flags, g_tstampNew),
               shTclGetUsage(interp, g_tstampNewTbl, flags, g_tstampNew));

   shTclDeclare(interp, g_tstampNow, 
               (Tcl_CmdProc *) tclTstampNow,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_tstampNowTbl, flags, g_tstampNow),
               shTclGetUsage(interp, g_tstampNowTbl, flags, g_tstampNow));

  shTclDeclare(interp, g_tstampDel, 
               (Tcl_CmdProc *) tclTstampDel,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_tstampDelTbl, flags, g_tstampDel),
               shTclGetUsage(interp, g_tstampDelTbl, flags, g_tstampDel));

  shTclDeclare(interp, g_deltaTstamp, 
               (Tcl_CmdProc *) tclDeltaTstamp,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_deltaTstampTbl, flags, g_deltaTstamp),
               shTclGetUsage(interp, g_deltaTstampTbl, flags, g_deltaTstamp));

   shTclDeclare(interp, g_tstampFromMJD, 
		(Tcl_CmdProc *) tclTstampFromMJD,
		(ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
		shTclGetArgInfo(interp, g_tstampFromMJDTbl, 
				flags, g_tstampFromMJD),
		shTclGetUsage(interp, g_tstampFromMJDTbl, 
			      flags, g_tstampFromMJD));
   shTclDeclare(interp, g_tstampToMJD, 
		(Tcl_CmdProc *) tclTstampToMJD,
		(ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
		shTclGetArgInfo(interp, g_tstampToMJDTbl, 
				flags, g_tstampToMJD),
		shTclGetUsage(interp, g_tstampToMJDTbl, 
			      flags, g_tstampToMJD));
  shTclDeclare(interp, g_tstampIncr, 
               (Tcl_CmdProc *) tclTstampIncr,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_tstampIncrTbl, flags, g_tstampIncr),
               shTclGetUsage(interp, g_tstampIncrTbl, flags, g_tstampIncr));

  shTclDeclare(interp, g_tstampToDate, 
               (Tcl_CmdProc *) tclTstampToDate,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_tstampToDateTbl, 
			       flags, g_tstampToDate),
               shTclGetUsage(interp, g_tstampToDateTbl, 
			     flags, g_tstampToDate));

  shTclDeclare(interp, g_tstampToTime, 
               (Tcl_CmdProc *) tclTstampToTime,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_tstampToTimeTbl, 
			       flags, g_tstampToTime),
               shTclGetUsage(interp, g_tstampToTimeTbl, 
			     flags, g_tstampToTime));

  shTclDeclare(interp, g_tstampToLst, 
               (Tcl_CmdProc *) tclTstampToLst,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_tstampToLstTbl, 
			       flags, g_tstampToLst),
               shTclGetUsage(interp, g_tstampToLstTbl, 
			     flags, g_tstampToLst));

  shTclDeclare(interp, g_tstampToTwilight, 
               (Tcl_CmdProc *) tclTstampToTwilight,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_tstampToTwilightTbl, 
			       flags, g_tstampToTwilight),
               shTclGetUsage(interp, g_tstampToTwilightTbl, 
			     flags, g_tstampToTwilight));

  shTclDeclare(interp, g_planetRaDec, 
               (Tcl_CmdProc *) tclPlanetRaDec,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_planetRaDecTbl, 
			       flags, g_planetRaDec),
               shTclGetUsage(interp, g_planetRaDecTbl, 
			     flags, g_planetRaDec));

  shTclDeclare(interp, g_planetRiseSet, 
               (Tcl_CmdProc *) tclPlanetRiseSet,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_planetRiseSetTbl, 
			       flags, g_planetRiseSet),
               shTclGetUsage(interp, g_planetRiseSetTbl, 
			     flags, g_planetRiseSet));

   shTclDeclare(interp, g_vFitxy,
		(Tcl_CmdProc *)tclVFitxy,
                (ClientData) 0, (Tcl_CmdDeleteProc *)NULL, tclFacil,
		shTclGetArgInfo(interp, g_vFitxyTbl, flags, g_vFitxy),
		shTclGetUsage(interp, g_vFitxyTbl, flags, g_vFitxy));

   shTclDeclare(interp, g_vTrans,
		(Tcl_CmdProc *)tclVTrans,
                (ClientData) 0, (Tcl_CmdDeleteProc *)NULL,
                tclFacil,
		shTclGetArgInfo(interp, g_vTransTbl, 
				flags, g_vTrans),
		shTclGetUsage(interp, g_vTransTbl, 
			      flags, g_vTrans));

    shTclDeclare(interp, g_vS2tp, 
               (Tcl_CmdProc *) tclVS2tp,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_vS2tpTbl, flags, g_vS2tp),
               shTclGetUsage(interp, g_vS2tpTbl, flags, g_vS2tp));

    shTclDeclare(interp, g_vTp2s, 
               (Tcl_CmdProc *) tclVTp2s,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_vTp2sTbl, flags, g_vTp2s),
               shTclGetUsage(interp, g_vTp2sTbl, flags, g_vTp2s));

    shTclDeclare(interp, g_refro, 
               (Tcl_CmdProc *) tclRefro,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_refroTbl, flags, g_refro),
               shTclGetUsage(interp, g_refroTbl, flags, g_refro));

    shTclDeclare(interp, g_oap, 
               (Tcl_CmdProc *) tclOap,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_oapTbl, flags, g_oap),
               shTclGetUsage(interp, g_oapTbl, flags, g_oap));

    shTclDeclare(interp, g_aop, 
               (Tcl_CmdProc *) tclAop,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_aopTbl, flags, g_aop),
               shTclGetUsage(interp, g_aopTbl, flags, g_aop));

    shTclDeclare(interp, g_mean2apparent, 
               (Tcl_CmdProc *) tclMean2Apparent,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_mean2apparentTbl, flags, g_mean2apparent),
               shTclGetUsage(interp, g_mean2apparentTbl, flags, g_mean2apparent));

    shTclDeclare(interp, g_azEl2HaDec, 
               (Tcl_CmdProc *) tclAzEl2HaDec,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_azEl2HaDecTbl, flags, g_azEl2HaDec),
               shTclGetUsage(interp, g_azEl2HaDecTbl, flags, g_azEl2HaDec));

    shTclDeclare(interp, g_aberrationApply,
               (Tcl_CmdProc *) tclAberrationApply,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_aberrationApplyTbl, 
			       flags, g_aberrationApply),
               shTclGetUsage(interp, g_aberrationApplyTbl, 
			     flags, g_aberrationApply));

    shTclDeclare(interp, g_VAberrationApply,
               (Tcl_CmdProc *) tclVAberrationApply,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_VAberrationApplyTbl, 
			       flags, g_VAberrationApply),
               shTclGetUsage(interp, g_VAberrationApplyTbl, 
			     flags, g_VAberrationApply));

    shTclDeclare(interp, g_aberrationGet,
               (Tcl_CmdProc *) tclAberrationGet,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_aberrationGetTbl, 
			       flags, g_aberrationGet),
               shTclGetUsage(interp, g_aberrationGetTbl, 
			     flags, g_aberrationGet));

   shTclDeclare(interp,g_velFromTstamp, 
		(Tcl_CmdProc *)tclvelFromTstamp,
                (ClientData) 0, (Tcl_CmdDeleteProc *)NULL, tclFacil, 
		shTclGetArgInfo(interp, g_velFromTstampTbl, 
				flags, g_velFromTstamp),
		shTclGetUsage(interp, g_velFromTstampTbl, 
			      flags, g_velFromTstamp));

   shTclDeclare(interp,g_meanToApparent, 
		(Tcl_CmdProc *)tclMeanToApparent,
                (ClientData) 0, (Tcl_CmdDeleteProc *)NULL, tclFacil, 
		shTclGetArgInfo(interp, g_meanToApparentTbl, 
				flags, g_meanToApparent),
		shTclGetUsage(interp, g_meanToApparentTbl, 
			      flags, g_meanToApparent));

   shTclDeclare(interp,g_pmApply, 
		(Tcl_CmdProc *)tclpmApply,
                (ClientData) 0, (Tcl_CmdDeleteProc *)NULL, tclFacil, 
		shTclGetArgInfo(interp, g_pmApplyTbl, 
				flags, g_pmApply),
		shTclGetUsage(interp, g_pmApplyTbl, 
			      flags, g_pmApply));

   shTclDeclare(interp,g_epj2mjd, 
		(Tcl_CmdProc *)tclepj2mjd,
                (ClientData) 0, (Tcl_CmdDeleteProc *)NULL, tclFacil, 
		shTclGetArgInfo(interp, g_epj2mjdTbl, 
				flags, g_epj2mjd),
		shTclGetUsage(interp, g_epj2mjdTbl, 
			      flags, g_epj2mjd));

   shTclDeclare(interp,g_mjd2epj, 
		(Tcl_CmdProc *)tclmjd2epj,
                (ClientData) 0, (Tcl_CmdDeleteProc *)NULL, tclFacil, 
		shTclGetArgInfo(interp, g_mjd2epjTbl, 
				flags, g_mjd2epj),
		shTclGetUsage(interp, g_mjd2epjTbl, 
			      flags, g_mjd2epj));

   shTclDeclare(interp,g_datFromMjd, 
		(Tcl_CmdProc *)tcldatFromMjd,
                (ClientData) 0, (Tcl_CmdDeleteProc *)NULL, tclFacil, 
		shTclGetArgInfo(interp, g_datFromMjdTbl, 
				flags, g_datFromMjd),
		shTclGetUsage(interp, g_datFromMjdTbl, 
			      flags, g_datFromMjd));

}
