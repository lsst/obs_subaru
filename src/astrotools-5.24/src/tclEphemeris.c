/* <AUTO>
   FILE: tclEphemeris
   ABSTRACT:
<HTML>
   Routines to calculate ephemeris
</HTML>
   </AUTO>
*/
/***************************************************************
 *
 * TCL verbs to calculate the ephemeris
 *
 *              tclMoonPhase
 *		tclRiseSet
 *		tclHourAngle
 *
 *		installed in astrotools: July 31, 1995
					 Heidi Newberg
 *
 **************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "slalib.h"
#include "atEphemeris.h"

static char *module = "astrotools";   /* name of this set of code */

/**************************************************************
                      tclMoonPhase
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: moonPhase

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atEphemeris.html#atMoonPhase>atMoonPhase</A> in atEphemeris.c

<P>
Compute the phase of the moon (0.0 = new, 1.0 = full)
(formula from Jean Meeus's Astronomical Formula for Calculators,
3rd Edition, pp. 155).
This routine was originally called illuminated_fraction.
  </HTML>

</AUTO>*/

static ftclArgvInfo moonPhaseTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL, "Find the phase of the moon."},
  {"<long>", FTCL_ARGV_DOUBLE, NULL, NULL, "Moon's ecliptic longitude (degrees)"},
  {"<lat>", FTCL_ARGV_DOUBLE, NULL, NULL, "Moon's ecliptic latitude (degrees)"},
  {"<sun>", FTCL_ARGV_DOUBLE, NULL, NULL, "Sun's longitude (degrees)"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *moonPhase_name = "moonPhase";

static int
tclMoonPhase(	ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                ) {
                                /* INPUT */
  double longitude,lat,sunlong;
                                /* OUTPUT */
  double moonphase;		/* the moon's phase */
  char answer[40];              /* sent back to Tcl */
  int returnValue=0;            /* info on atAirmassFind performance */
                                /* 0= ok, 1= wrong */

/* Parse the command */

  moonPhaseTable[1].dst = &longitude;
  moonPhaseTable[2].dst = &lat;
  moonPhaseTable[3].dst = &sunlong;

  if ((returnValue = shTclParseArgv(
        interp, &argc, argv, moonPhaseTable, 0, moonPhase_name))
                                                != FTCL_ARGV_SUCCESS) {
        return(returnValue);
  }

/************************************
 * 
 *   Do the actual call 
 * 
 ************************************
 */

/* send back the answer */
  moonphase = atMoonPhase (longitude,lat,sunlong);

  Tcl_SetResult(interp, "", TCL_VOLATILE);
  sprintf(answer, "%f", moonphase); 
  Tcl_AppendElement(interp, answer);

  return TCL_OK;
}

/**************************************************************
                      tclRiseSet
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: riseSet

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atEphemeris.html#atRiseSet>atRiseSet</A> in atEphemeris.c

<P>
 Returns the local time of rise or set for an object with the
 specified ra and dec, at a site with the specified latitide and longitude
 and time zone, for the given Julian epoch.  Rising and setting are defined
 with respect to the specified zenith distance.  Returns -1 if object
 doesn't rise or set (formula from Astronomical Formulae
 for Calculators, 3rd edition, Jean Meeus, p. 47 and
 Astronomical Almanac 1989, p. A12).
  </HTML>

</AUTO>*/

static ftclArgvInfo riseSetTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL, "Return local rise/set time."},
  {"<ra>", FTCL_ARGV_DOUBLE, NULL, NULL, "Right ascension (degrees)"},
  {"<dec>", FTCL_ARGV_DOUBLE, NULL, NULL, "declination (degrees)"},
  {"<long>", FTCL_ARGV_DOUBLE, NULL, NULL, "Earth west longitude (degrees)"},
  {"<lat>", FTCL_ARGV_DOUBLE, NULL, NULL, "Earth latitude (degrees)"},
  {"<epoch>", FTCL_ARGV_DOUBLE, NULL, NULL, "Julian epoch"},
  {"<timezone>", FTCL_ARGV_INT, NULL, NULL, "Time zone"},
  {"<zenithdist>", FTCL_ARGV_DOUBLE, NULL, NULL, "Zenith distance defining rising and setting"},
  {"<which>", FTCL_ARGV_STRING, NULL, NULL, "Which phenomenom (RISE or SET)"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *riseSet_name = "riseSet";

static int
tclRiseSet(	ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                ) {
                                /* INPUT */
  double ra,dec,longitude,lat,epoch,zd;
  int timezone, which;
  char *cwhich;
                                /* OUTPUT */
  double riseset;		/* the rise or set time */
  char answer[40];              /* sent back to Tcl */
  int returnValue=0;            /* info on atRiseSet performance */
                                /* 0= ok, 1= wrong */

/* Parse the command */

  riseSetTable[1].dst = &ra;
  riseSetTable[2].dst = &dec;
  riseSetTable[3].dst = &longitude;
  riseSetTable[4].dst = &lat;
  riseSetTable[5].dst = &epoch;
  riseSetTable[6].dst = &timezone;
  riseSetTable[7].dst = &zd;
  riseSetTable[8].dst = &cwhich;

  if ((returnValue = shTclParseArgv(
        interp, &argc, argv, riseSetTable, 0, riseSet_name))
                                                != FTCL_ARGV_SUCCESS) {
        return(returnValue);
  }

  if (!strcmp(cwhich,"rise")) { which = RISE; }
  else if (!strcmp(cwhich, "set")) { which = SET; }
  else {
    Tcl_SetResult(interp, "which must be rise or set", TCL_VOLATILE);
    return TCL_ERROR;
  }

/************************************
 * 
 *   Do the actual call 
 * 
 ************************************
 */

/* send back the answer */
  riseset = atRiseSet (ra,dec,longitude,lat,epoch,timezone,zd,which);

  Tcl_SetResult(interp, "", TCL_VOLATILE);
  sprintf(answer, "%f", riseset); 
  Tcl_AppendElement(interp, answer);

  return TCL_OK;
}


/**************************************************************
                      tclMjdRiseSet
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: mjdRiseSet

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atEphemeris.html#atMjdRiseSet>atMjdRiseSet</A> in atEphemeris.c

<P>
 Returns the local time of rise or set for an object with the
 specified ra and dec, at a site with the specified latitide and longitude
 and time zone, for the given Julian epoch.  Rising and setting are defined
 with respect to the specified zenith distance.  Returns -1 if object
 doesn't rise or set (formula from Astronomical Formulae
 for Calculators, 3rd edition, Jean Meeus, p. 47 and
 Astronomical Almanac 1989, p. A12).
  </HTML>

</AUTO>*/

static ftclArgvInfo mjdRiseSetTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL, "Return local rise/set time."},
  {"<ra>", FTCL_ARGV_DOUBLE, NULL, NULL, "Right ascension (degrees)"},
  {"<dec>", FTCL_ARGV_DOUBLE, NULL, NULL, "declination (degrees)"},
  {"<longitude>", FTCL_ARGV_DOUBLE, NULL, NULL, "Earth west longitude (degrees)"},
  {"<lat>", FTCL_ARGV_DOUBLE, NULL, NULL, "Earth latitude (degrees)"},
  {"<mjd>", FTCL_ARGV_DOUBLE, NULL, NULL, "Modified Julian date"},
  {"<timezone>", FTCL_ARGV_INT, NULL, NULL, "Time zone"},
  {"<zenithdist>", FTCL_ARGV_DOUBLE, NULL, NULL, "Zenith distance defining rising and setting"},
  {"<which>", FTCL_ARGV_STRING, NULL, NULL, "Which phenomenom (RISE or SET)"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *mjdRiseSet_name = "mjdRiseSet";

static int
tclMjdRiseSet(	ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                ) {
                                /* INPUT */
  double ra,dec,longitude,lat,mjd,zd;
  int timezone, which;
  char *cwhich;
                                /* OUTPUT */
  double riseset;		/* the rise or set time */
  char answer[40];              /* sent back to Tcl */
  int returnValue=0;            /* info on atMjdRiseSet performance */
                                /* 0= ok, 1= wrong */

/* Parse the command */

  mjdRiseSetTable[1].dst = &ra;
  mjdRiseSetTable[2].dst = &dec;
  mjdRiseSetTable[3].dst = &longitude;
  mjdRiseSetTable[4].dst = &lat;
  mjdRiseSetTable[5].dst = &mjd;
  mjdRiseSetTable[6].dst = &timezone;
  mjdRiseSetTable[7].dst = &zd;
  mjdRiseSetTable[8].dst = &cwhich;

  if ((returnValue = shTclParseArgv(
        interp, &argc, argv, mjdRiseSetTable, 0, mjdRiseSet_name))
                                                != FTCL_ARGV_SUCCESS) {
        return(returnValue);
  }

  if (!strcmp(cwhich,"rise")) { which = RISE; }
  else if (!strcmp(cwhich, "set")) { which = SET; }
  else {
    Tcl_SetResult(interp, "which must be rise or set", TCL_VOLATILE);
    return TCL_ERROR;
  }

/************************************
 * 
 *   Do the actual call 
 * 
 ************************************
 */

/* send back the answer */
  riseset = atMjdRiseSet (ra,dec,longitude,lat,mjd,timezone,zd,which);

  Tcl_SetResult(interp, "", TCL_VOLATILE);
  sprintf(answer, "%f", riseset); 
  Tcl_AppendElement(interp, answer);

  return TCL_OK;
}

/**************************************************************
                      tclHourAngle
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: hourAngle

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atEphemeris.html#atHourAngle>atHourAngle</A> in atEphemeris.c

<P>
 atHourAngle -- Returns the hour angle (in degrees) for the specified
 declination, earth latitide, and zenith distance.  Returns -1 if
 no solution (object never reaches that hour angle).
 Formula from Astronomical Formulae for Calculators, 3rd edition,
 Jean Meeus, p. 47.
  </HTML>

</AUTO>*/

static ftclArgvInfo hourAngleTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL, "Find hour angle"},
  {"<zenithdist>", FTCL_ARGV_DOUBLE, NULL, NULL, "Zenith distance (degrees from pole)"},
  {"<dec>", FTCL_ARGV_DOUBLE, NULL, NULL, "declination (degrees)"},
  {"<lat>", FTCL_ARGV_DOUBLE, NULL, NULL, "Earth latitude (degrees)"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *hourAngle_name = "hourAngle";

static int
tclHourAngle(	ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                ) {
                                /* INPUT */
  double zd,dec,lat;
                                /* OUTPUT */
  double hourangle;		/* hour angle */
  char answer[40];              /* sent back to Tcl */
  int returnValue=0;            /* info on atHourAngle performance */
                                /* 0= ok, 1= wrong */

/* Parse the command */

  hourAngleTable[1].dst = &zd;
  hourAngleTable[2].dst = &dec;
  hourAngleTable[3].dst = &lat;

  if ((returnValue = shTclParseArgv(
        interp, &argc, argv, hourAngleTable, 0, hourAngle_name))
                                                != FTCL_ARGV_SUCCESS) {
        return(returnValue);
  }

/************************************
 * 
 *   Do the actual call 
 * 
 ************************************
 */

/* send back the answer */
  hourangle = atHourAngle (zd,dec,lat);

  Tcl_SetResult(interp, "", TCL_VOLATILE);
  sprintf(answer, "%f", hourangle); 
  Tcl_AppendElement(interp, answer);

   return TCL_OK;
}

/**************************************************************
                      tclNextNewMoon
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: nextNewMoon

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atEphemeris.html#atNextNewMoon>atNextNewMoon</A> in atEphemeris.c

<P>
 Start at the time given in beginTime, and step by deltaHours.
The TSTAMP returned is the time of the first minimum of the separation
between the moon and the Sun.
deltaHours defaults to 1 hour.
  </HTML>

 </AUTO>*/

 static ftclArgvInfo nextNewMoonTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL, "Find next new moon"},
  {"<beginTime>", FTCL_ARGV_STRING, NULL, NULL, "Beginning time"},
  {"-deltaHours", FTCL_ARGV_DOUBLE, NULL, NULL, "time step in hours"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *nextNewMoon_name = "nextNewMoon";

static int
tclNextNewMoon(	ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                ) {
                                /* INPUT */
  char *a_beginTime=NULL;
  TSTAMP *beginTime = NULL;
  double a_deltaHours = 1;
                                /* OUTPUT */
  TSTAMP *nextNewMoon = NULL;
  char nextNewMoonName[HANDLE_NAMELEN];
  int returnValue;
/* Parse the command */

  nextNewMoonTable[1].dst = &a_beginTime;
  nextNewMoonTable[2].dst = &a_deltaHours;

  if ((returnValue = shTclParseArgv(
        interp, &argc, argv, nextNewMoonTable, 0, nextNewMoon_name))
                                                != FTCL_ARGV_SUCCESS) {
        return(returnValue);
  }
  if (shTclAddrGetFromName(interp, a_beginTime, (void **) &beginTime, 
			   "TSTAMP") != TCL_OK) {
    Tcl_SetResult(interp, "bad TSTAMP name for beginTime", TCL_VOLATILE);
    return TCL_ERROR;
  }

/************************************
 * 
 *   Do the actual call 
 * 
 ************************************
 */


  nextNewMoon = atNextNewMoon (beginTime, a_deltaHours);

  if (shTclHandleNew(interp, nextNewMoonName, "TSTAMP", nextNewMoon) 
      != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to nextNewMoonName handle", TCL_STATIC);
    return(TCL_ERROR);
  }
    Tcl_SetResult(interp, nextNewMoonName, TCL_VOLATILE);

  return TCL_OK;
}

/**************************************************************
                      tclNextFullMoon
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: nextFullMoon

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atEphemeris.html#atNextFullMoon>atNextFullMoon</A> in atEphemeris.c

<P>
 Start at the time given in beginTime, and step by deltaHours.
The TSTAMP returned is the time of the first minimum of the separation
between the moon and the Sun.
deltaHours defaults to 1 hour.
  </HTML>

 </AUTO>*/

 static ftclArgvInfo nextFullMoonTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL, "Find next Full moon"},
  {"<beginTime>", FTCL_ARGV_STRING, NULL, NULL, "Beginning time"},
  {"-deltaHours", FTCL_ARGV_DOUBLE, NULL, NULL, "time step in hours"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *nextFullMoon_name = "nextFullMoon";

static int
tclNextFullMoon(	ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                ) {
                                /* INPUT */
  char *a_beginTime=NULL;
  TSTAMP *beginTime = NULL;
  double a_deltaHours = 1;
                                /* OUTPUT */
  TSTAMP *nextFullMoon = NULL;
  char nextFullMoonName[HANDLE_NAMELEN];
  int returnValue;
/* Parse the command */

  nextFullMoonTable[1].dst = &a_beginTime;
  nextFullMoonTable[2].dst = &a_deltaHours;

  if ((returnValue = shTclParseArgv(
        interp, &argc, argv, nextFullMoonTable, 0, nextFullMoon_name))
                                                != FTCL_ARGV_SUCCESS) {
        return(returnValue);
  }
  if (shTclAddrGetFromName(interp, a_beginTime, (void **) &beginTime, 
			   "TSTAMP") != TCL_OK) {
    Tcl_SetResult(interp, "bad TSTAMP name for beginTime", TCL_VOLATILE);
    return TCL_ERROR;
  }

/************************************
 * 
 *   Do the actual call 
 * 
 ************************************
 */


  nextFullMoon = atNextFullMoon (beginTime, a_deltaHours);

  if (shTclHandleNew(interp, nextFullMoonName, "TSTAMP", nextFullMoon) 
      != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to nextFullMoonName handle", TCL_STATIC);
    return(TCL_ERROR);
  }
    Tcl_SetResult(interp, nextFullMoonName, TCL_VOLATILE);

  return TCL_OK;
}

/*********************************************************************/
void atTclEphemerisDeclare(

Tcl_Interp *interp)
{
  shTclDeclare(
               interp,
               riseSet_name,
               (Tcl_CmdProc *)tclRiseSet,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, riseSetTable, 0,
				riseSet_name),
               shTclGetUsage(interp,   riseSetTable, 0,
				riseSet_name));

  shTclDeclare(
               interp,
               mjdRiseSet_name,
               (Tcl_CmdProc *)tclMjdRiseSet,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, mjdRiseSetTable, 0,
				mjdRiseSet_name),
               shTclGetUsage(interp,   mjdRiseSetTable, 0,
				mjdRiseSet_name));

  shTclDeclare(
               interp,
               moonPhase_name,
               (Tcl_CmdProc *)tclMoonPhase,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, moonPhaseTable, 0,
				moonPhase_name),
               shTclGetUsage(interp,   moonPhaseTable, 0,
				moonPhase_name));
  shTclDeclare(
               interp,
               hourAngle_name,
               (Tcl_CmdProc *)tclHourAngle,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, hourAngleTable, 0,
				hourAngle_name),
               shTclGetUsage(interp,   hourAngleTable, 0,
				hourAngle_name));
  shTclDeclare(
               interp,
               nextNewMoon_name,
               (Tcl_CmdProc *)tclNextNewMoon,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, nextNewMoonTable, 0,
				nextNewMoon_name),
               shTclGetUsage(interp,   nextNewMoonTable, 0,
				nextNewMoon_name));
  shTclDeclare(
               interp,
               nextFullMoon_name,
               (Tcl_CmdProc *)tclNextFullMoon,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, nextFullMoonTable, 0,
				nextFullMoon_name),
               shTclGetUsage(interp,   nextFullMoonTable, 0,
				nextFullMoon_name));
}
