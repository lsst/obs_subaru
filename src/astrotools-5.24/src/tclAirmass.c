/* <AUTO>
   FILE: tclAirmass
   ABSTRACT:
<HTML>
   Routines to calculate airmass
</HTML>
   </AUTO>
*/
/***************************************************************
 *
 * TCL verbs to calculate the airmass
 *
 *              atAirmass
 *
 *		installed in astrotools: May 24, 1995
 *                                       Heidi Newberg
 *              written: May 11, 1995
 *			 james annis
 *
 *
 **************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "atAirmass.h"
#include "slalib.h"

static char *module = "astrotools";   /* name of this set of code */

/**************************************************************
                      tclAirmassFind
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: airmassFind

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atAirmass.html#atAirmassFind>atAirmassFind</A> in atAirmass.c

<P>
Calculate the airmass at APO given the RA in degrees, the 
DEC in degrees, a UT timestamp.

  </HTML>

</AUTO>*/

static ftclArgvInfo airmassFindTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL,
  "Find the airmass of an object at RA,Dec, at time UT.\n"
  "Finds airmass at epoch of coordinates, at the latitude of APO."},
  {"<ra>", FTCL_ARGV_DOUBLE, NULL, NULL, "right ascension in decimal degrees"},
  {"<dec>", FTCL_ARGV_DOUBLE, NULL, NULL, "declination in decimal degrees"},
  {"<ut>", FTCL_ARGV_STRING, NULL, NULL, "tstamp object"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *airmassFind_name = "airmassFind";

static int
tclAirmassFind(
                ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                )
{
                                /* INPUT */
  char *timeName;               /* timeStamp name */
  TSTAMP *time = (TSTAMP *)0;     /* pointer to the timeStamp */
  double ra, dec;               /* position of object */
                                /* OUTPUT */
  double airmass;		/* the zenith distance */
  int returnValue=0;            /* info on atAirmassFind performance */
                                /* 0= ok, 1= wrong */

  char answer[40];              /* sent back to Tcl */


/* Parse the command */

  airmassFindTable[1].dst = &ra;
  airmassFindTable[2].dst = &dec;
  airmassFindTable[3].dst = &timeName;

  if ((returnValue = shTclParseArgv(
        interp, &argc, argv, airmassFindTable, FTCL_ARGV_NO_LEFTOVERS, airmassFind_name))
                                                != FTCL_ARGV_SUCCESS) {
        return(returnValue);
  }

/* check timestamp */
    if (shTclAddrGetFromName
		(interp, timeName, (void **) &time, "TSTAMP") != TCL_OK) {
      Tcl_SetResult
        	(interp, "airmassFind: bad timeStamp name", TCL_VOLATILE);
      return TCL_ERROR;
    }


/************************************
 * 
 *   Do the actual call 
 * 
 ************************************
 */

/* send back the answer */
  if (atAirmassFind (time, ra, dec, &airmass) != SH_SUCCESS) {
      Tcl_SetResult (interp, "airmassFind: error in atAirmassFind", TCL_VOLATILE);
      return TCL_ERROR;
  }

  Tcl_SetResult(interp, "", TCL_VOLATILE);
  sprintf(answer, "%f", airmass); Tcl_AppendElement(interp, answer);

  return TCL_OK;
}

/**************************************************************
                      tclAirmassFromZd
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: airmassFromZd

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atAirmass.html#atAirmassFromZd>atAirmassFromZd</A> in atAirmass.c

<P>
Nothing but a wrapper to sla_Airmas which takes a zenith distance.
This is specifically for use when one might be tempted to simply use
sec(z), which will incur 0.25% errors at the Survey limit without any
real good reason for doing so.
<P>

  </HTML>

</AUTO>*/

static ftclArgvInfo airmassFromZdTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL,
   "Find the airmass of an object at Zd.\n"},
  {"<zd>", FTCL_ARGV_DOUBLE, NULL, NULL, "zenith distance in decimal degrees"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *airmassFromZd_name = "airmassFromZd";

static int
tclAirmassFromZd(
                ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                )
{
                                /* INPUT */
  double zd;                    /* zenith distance (degrees) */
                                /* OUTPUT */
  double airmass;		/* airmass */
  int returnValue=0;            /* info on performance */
                                /* 0= ok, 1= wrong */

  char answer[40];              /* sent back to Tcl */


/* Parse the command */

  airmassFromZdTable[1].dst = &zd;

  if ((returnValue = shTclParseArgv(
        interp, &argc, argv, airmassFromZdTable, FTCL_ARGV_NO_LEFTOVERS, airmassFromZd_name))
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
  airmass = atAirmassFromZd(zd);


  Tcl_SetResult(interp, "", TCL_VOLATILE);
  sprintf(answer, "%f", airmass); Tcl_AppendElement(interp, answer);

  return TCL_OK;
}

/**************************************************************
                      tclMeridianAirmassFind
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: meridianAirmassFind

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atAirmass.html#atMeridianAirmassFind>atMeridianAirmassFind</A> in atAirmass.c

<P>
Calculate the meridian airmass at APO the DEC in degrees.

  </HTML>

</AUTO>*/

static ftclArgvInfo meridianAirmassFindTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL,
  "Find the meridian airmass of an object at Dec.\n"
  "Finds airmass at epoch of coordinates, at the latitude of APO."},
  {"<dec>", FTCL_ARGV_DOUBLE, NULL, NULL, "declination in decimal degrees"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *meridianAirmassFind_name = "meridianAirmassFind";

static int
tclMeridianAirmassFind(
                ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                )
{
                                /* INPUT */
  double dec;                   /* position of object */
                                /* OUTPUT */
  double airmass;		/* the zenith distance */
  int returnValue=0;            /* info on atAirmassFind performance */
                                /* 0= ok, 1= wrong */

  char answer[40];              /* sent back to Tcl */


/* Parse the command */

  meridianAirmassFindTable[1].dst = &dec;

  if ((returnValue = shTclParseArgv(
        interp, &argc, argv, meridianAirmassFindTable, FTCL_ARGV_NO_LEFTOVERS, meridianAirmassFind_name))
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
  airmass = atMeridianAirmassFind(dec);


  Tcl_SetResult(interp, "", TCL_VOLATILE);
  sprintf(answer, "%f", airmass); Tcl_AppendElement(interp, answer);

  return TCL_OK;
}

/**************************************************************
                      tclAirmassFindHaDec
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: airmassFindHaDec

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atAirmass.html#atAirmassFindHaDec>atAirmassFindHaDec</A> in atAirmass.c

<P> 
Calculate the airmass at APO from the HA in hours and the DEC in
degrees.

  </HTML>

</AUTO>*/

static ftclArgvInfo airmassFindHaDecTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL,
  "Find the airmass of an object at Ha,Dec.\n"
  "Finds airmass at epoch of coordinates, at the latitude of APO."},
  {"<ha>", FTCL_ARGV_DOUBLE, NULL, NULL, "hour angle in hours"},
  {"<dec>", FTCL_ARGV_DOUBLE, NULL, NULL, "declination in decimal degrees"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *airmassFindHaDec_name = "airmassFindHaDec";

static int
tclAirmassFindHaDec(
		    ClientData clientData,
		    Tcl_Interp *interp,
		    int argc,
		    char **argv
		    )
{
                                /* INPUT */
  double ha;                    /* ha of object */
  double dec;                   /* position of object */
                                /* OUTPUT */
  double airmass;		/* the zenith distance */
  int returnValue=0;            /* info on atAirmassFind performance */
                                /* 0= ok, 1= wrong */

  char answer[40];              /* sent back to Tcl */


/* Parse the command */

  airmassFindHaDecTable[1].dst = &ha;
  airmassFindHaDecTable[2].dst = &dec;

  if ((returnValue = shTclParseArgv(
				    interp, &argc, argv, airmassFindHaDecTable, FTCL_ARGV_NO_LEFTOVERS, airmassFindHaDec_name))
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
  airmass = atAirmassFindHaDec(ha,dec);


  Tcl_SetResult(interp, "", TCL_VOLATILE);
  sprintf(answer, "%f", airmass); Tcl_AppendElement(interp, answer);

  return TCL_OK;
}

/**************************************************************
                      tclZdDec2Ha
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: zdDec2Ha

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atAirmass.html#atZdDec2Ha>atZdDec2Ha</A> in atAirmass.c

<P> 
Calculate the HA (hours) from the zenith distance (degrees) and dec (degrees).

  </HTML>

</AUTO>*/

static ftclArgvInfo zdDec2HaTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL,
  "Find the HA given ZD and DEC\n"
  "Finds HA at epoch of coordinates, at the latitude of APO."},
  {"<zd>", FTCL_ARGV_DOUBLE, NULL, NULL, "zenith distance in degrees "},
  {"<dec>", FTCL_ARGV_DOUBLE, NULL, NULL, "declination in degrees"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *zdDec2Ha_name = "zdDec2Ha";

static int
tclZdDec2Ha(
		    ClientData clientData,
		    Tcl_Interp *interp,
		    int argc,
		    char **argv
		    )
{
                                /* INPUT */
  double zd;                    /* zd of object */
  double dec;                   /* position of object */
                                /* OUTPUT */
  double ha;		        /* the hour angle */
  int returnValue=0;            /* info on atAirmassFind performance */
                                /* 0= ok, 1= wrong */

  char answer[40];              /* sent back to Tcl */


/* Parse the command */

  zdDec2HaTable[1].dst = &zd;
  zdDec2HaTable[2].dst = &dec;

  if ((returnValue = shTclParseArgv(
				    interp, &argc, argv, zdDec2HaTable, FTCL_ARGV_NO_LEFTOVERS, zdDec2Ha_name))
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
  ha = atZdDec2Ha(zd,dec);


  Tcl_SetResult(interp, "", TCL_VOLATILE);
  sprintf(answer, "%f", ha); Tcl_AppendElement(interp, answer);

  return TCL_OK;
}

/*********************************************************************/
void atTclAirmassDeclare(
			 Tcl_Interp *interp
			 )
{
  shTclDeclare(
               interp,
               airmassFind_name,
               (Tcl_CmdProc *)tclAirmassFind,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, airmassFindTable, FTCL_ARGV_NO_LEFTOVERS,
			       airmassFind_name),
               shTclGetUsage(interp,   airmassFindTable, FTCL_ARGV_NO_LEFTOVERS,
			     airmassFind_name));

  shTclDeclare(
               interp,
               airmassFromZd_name,
               (Tcl_CmdProc *)tclAirmassFromZd,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, airmassFromZdTable, FTCL_ARGV_NO_LEFTOVERS,
			       airmassFromZd_name),
               shTclGetUsage(interp,   airmassFromZdTable, FTCL_ARGV_NO_LEFTOVERS,
			     airmassFromZd_name));

  shTclDeclare(
               interp,
               meridianAirmassFind_name,
               (Tcl_CmdProc *)tclMeridianAirmassFind,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, meridianAirmassFindTable, FTCL_ARGV_NO_LEFTOVERS,
			       meridianAirmassFind_name),
               shTclGetUsage(interp,   meridianAirmassFindTable, FTCL_ARGV_NO_LEFTOVERS,
			     meridianAirmassFind_name));

  shTclDeclare(
               interp,
               airmassFindHaDec_name,
               (Tcl_CmdProc *)tclAirmassFindHaDec,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, airmassFindHaDecTable, FTCL_ARGV_NO_LEFTOVERS,
			       airmassFindHaDec_name),
               shTclGetUsage(interp,   airmassFindHaDecTable, FTCL_ARGV_NO_LEFTOVERS,
			     airmassFindHaDec_name));

  shTclDeclare(
               interp,
               zdDec2Ha_name,
               (Tcl_CmdProc *)tclZdDec2Ha,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, zdDec2HaTable, FTCL_ARGV_NO_LEFTOVERS,
			       zdDec2Ha_name),
               shTclGetUsage(interp,   zdDec2HaTable, FTCL_ARGV_NO_LEFTOVERS,
			     zdDec2Ha_name));

}
