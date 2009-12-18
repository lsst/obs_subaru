/* <AUTO>
   FILE: tclGalaxy
   ABSTRACT:
<HTML>
   Routines to calculate galactic quantities
</HTML>
   </AUTO>
*/
/***************************************************************
 *
 * TCL verbs to calculate galactic quantites
 *
 *              atGalaxy
 *
 *              written: June 6, 1995
 *			 Heidi Newberg
 *
 *
 **************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "astrotools.h"
#include "slalib.h"

static char *module = "astrotools";   /* name of this set of code */

/**************************************************************
                      tclStarCountsFind
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: starCountsFind

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atGalaxy.html#atStarCountsFind>atStarCountsFind</A> in atGalaxy.c

<P>
Calculate the stellar density, given the galactic longitude, latitude, and
limiting magnitude

  </HTML>

</AUTO>*/

static ftclArgvInfo starCountsFindTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL,
  "Calculate the stellar density"},
  {"<l>", FTCL_ARGV_DOUBLE, NULL, NULL, "galactic longitude in decimal degrees"},
  {"<b>", FTCL_ARGV_DOUBLE, NULL, NULL, "galactic latitude in decimal degrees"},
  {"<m>", FTCL_ARGV_DOUBLE, NULL, NULL, "limiting V magnitude"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};
static char *starCountsFind_name = "starCountsFind";

static int
tclStarCountsFind(
                ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                )
{
                                /* INPUT */
  double l,b,m;               /* position of object */
                                /* OUTPUT */
  double starcounts;		/* the zenith distance */

  int returnValue=0;            /* info on atStarCountsFind performance */
                                /* 0= ok, 1= wrong */

  char answer[40];              /* sent back to Tcl */


/* Parse the command */

  starCountsFindTable[1].dst = &l;
  starCountsFindTable[2].dst = &b;
  starCountsFindTable[3].dst = &m;

  if ((returnValue = shTclParseArgv(
        interp, &argc, argv, starCountsFindTable, FTCL_ARGV_NO_LEFTOVERS, starCountsFind_name))
                                                != FTCL_ARGV_SUCCESS) {
        return(returnValue);
  }

  if (b<20 || b>90) {
    Tcl_SetResult(interp, "Galaxic latitude is must be in 20<=b<=90", TCL_VOLATILE);
    return TCL_ERROR;
  }

/************************************
 * 
 *   Do the actual call 
 * 
 ************************************
 */

/* send back the answer */
  atStarCountsFind (l,b,m,&starcounts);

  Tcl_SetResult(interp, "", TCL_VOLATILE);
  sprintf(answer, "%f", starcounts); Tcl_AppendElement(interp, answer);

  return TCL_OK;
}

/*********************************************************************/
void atTclGalaxyDeclare(

Tcl_Interp *interp)
{
  shTclDeclare(
               interp,
               starCountsFind_name,
               (Tcl_CmdProc *)tclStarCountsFind,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, starCountsFindTable, 
				FTCL_ARGV_NO_LEFTOVERS, starCountsFind_name),
               shTclGetUsage(interp,   starCountsFindTable, 
				FTCL_ARGV_NO_LEFTOVERS, starCountsFind_name));
}
