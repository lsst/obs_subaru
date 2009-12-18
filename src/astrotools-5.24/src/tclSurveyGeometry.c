/* <AUTO>
   FILE: atSurveyGeometry
   ABSTRACT: coordinate transformations:  equatorial to and from
             great circle, galactic, and great circle.  Also a
             verb to access useful values of the survey geometry and
             conversion constants.
<HTML>
   Routines for coordinate conversions and survey geometry
</HTML>
   </AUTO>
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "dervish.h"
#include "atSlalib.h"
#include "atConversions.h"
#include "atSurveyGeometry.h"

static char *tclFacil = "astrotools";    /* name of this set of code */
char answer[100];

void atTclSurveyGeometryDeclare(Tcl_Interp *interp);
/****************************************************************************
**
** ROUTINE: tclEqToGC
**
**<AUTO EXTRACT>
** TCL VERB: eqToGC
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atEqToGC>atEqToGC</A> in atSurveyGeometry.c
** </HTML>
**</AUTO>
*/
char g_eqToGC[] = "eqToGC";
ftclArgvInfo g_eqToGCTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "convert equatorial to great circle coordinates, in decimal degrees.\nRETURN a keyed list of mu and nu.\n" },
  {"<ra>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "right ascension"},
  {"<dec>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "declination"},
  {"-node", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "node of the reference great circle"},
  {"-inclination", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "inclination of the reference great circle"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclEqToGC(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_ra = 0.0;
  double a_dec = 0.0;
  double a_node = at_surveyCenterRa - 90.0;
  double a_inclination = at_surveyCenterDec;
  double mu = 0.0;
  double nu = 0.0;

  g_eqToGCTbl[1].dst = &a_ra;
  g_eqToGCTbl[2].dst = &a_dec;
  g_eqToGCTbl[3].dst = &a_node;
  g_eqToGCTbl[4].dst = &a_inclination;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_eqToGCTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_eqToGC)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  atEqToGC(a_ra, a_dec, &mu, &nu, a_node, a_inclination);
  sprintf(answer,"mu %13.8f",mu); Tcl_AppendElement(interp, answer);
  sprintf(answer,"nu %13.8f",nu); Tcl_AppendElement(interp, answer);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclGCToEq
**
**<AUTO EXTRACT>
** TCL VERB: GCToEq
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atGCToEq>atGCToEq</A> in atSurveyGeometry.c
** </HTML>
**</AUTO>
*/
char g_GCToEq[] = "GCToEq";
ftclArgvInfo g_GCToEqTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Convert great circle to equatorial coordinates, in decimal degrees\n" },
  {"<mu>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "mu on the great circle"},
  {"<nu>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "nu on the great circle"},
  {"-node", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "node of the great circle"},
  {"-inclination", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "inclination of the great circle"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclGCToEq(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_mu = 0.0;
  double a_nu = 0.0;
  double a_node = at_surveyCenterRa - 90.0;
  double a_inclination = at_surveyCenterDec;
  double ra = 0.0;
  double dec = 0.0;
  g_GCToEqTbl[1].dst = &a_mu;
  g_GCToEqTbl[2].dst = &a_nu;
  g_GCToEqTbl[3].dst = &a_node;
  g_GCToEqTbl[4].dst = &a_inclination;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_GCToEqTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_GCToEq)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  atGCToEq(a_mu, a_nu, &ra, &dec, a_node, a_inclination);
  sprintf(answer,"ra %13.8f",ra); Tcl_AppendElement(interp, answer);
  sprintf(answer,"dec %13.8f",dec); Tcl_AppendElement(interp, answer);
  return TCL_OK;

}

/****************************************************************************
**
** ROUTINE: tclEqToGal
**
**<AUTO EXTRACT>
** TCL VERB: eqToGal
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atEqToGal>atEqToGal</A> in atSurveyGeometry.c
** </HTML>
**</AUTO>
*/
char g_eqToGal[] = "eqToGal";
ftclArgvInfo g_eqToGalTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "convert equatorial coordinates to galactic coordinates, in decimal degrees.\nRETURN: keyed list with gLong and gLat \n" },
  {"<ra>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "right ascension"},
  {"<dec>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "declination"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclEqToGal(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_ra = 0.0;
  double a_dec = 0.0;
  double glong = 0.0;
  double glat = 0.0;
  g_eqToGalTbl[1].dst = &a_ra;
  g_eqToGalTbl[2].dst = &a_dec;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_eqToGalTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_eqToGal)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  atEqToGal(a_ra, a_dec, &glong, &glat);
  sprintf(answer,"gLong %13.8f",glong); Tcl_AppendElement(interp, answer);
  sprintf(answer,"gLat %13.8f",glat); Tcl_AppendElement(interp, answer);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclGalToEq
**
**<AUTO EXTRACT>
** TCL VERB: galToEq
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atGalToEq>atGalToEq</A> in atSurveyGeometry.c
** </HTML>
**</AUTO>
*/
char g_galToEq[] = "galToEq";
ftclArgvInfo g_galToEqTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "convert galactic coordinates to equatorial coordinates, in decimal degrees.\nRETURN: keyed list with ra and dec\n" },
  {"<gLong>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "galactic longitude"},
  {"<gLat>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "galactic latitude"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclGalToEq(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_gLong = 0.0;
  double a_gLat = 0.0;
  double ra = 0.0;
  double dec = 0.0;
  g_galToEqTbl[1].dst = &a_gLong;
  g_galToEqTbl[2].dst = &a_gLat;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_galToEqTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_galToEq)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  atGalToEq(a_gLong, a_gLat, &ra, &dec);
  sprintf(answer,"ra %13.8f",ra); Tcl_AppendElement(interp, answer);
  sprintf(answer,"dec %13.8f",dec); Tcl_AppendElement(interp, answer);
  return TCL_OK;
}
/****************************************************************************
**
** ROUTINE: tclEqToSurvey
**
**<AUTO EXTRACT>
** TCL VERB: eqToSurvey
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atEqToSurvey>atEqToSurvey</A> in atSurveyGeometry.c
** </HTML>
**</AUTO>
*/
char g_eqToSurvey[] = "eqToSurvey";
ftclArgvInfo g_eqToSurveyTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "convert equatorial to survey coordinates, in decimal degrees.\nRETURN: keyed list with lambda, eta\n" },
  {"<ra>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "right ascension"},
  {"<dec>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "declination"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclEqToSurvey(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_ra = 0.0;
  double a_dec = 0.0;
  double lambda = 0.0;
  double eta = 0.0;

  g_eqToSurveyTbl[1].dst = &a_ra;
  g_eqToSurveyTbl[2].dst = &a_dec;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_eqToSurveyTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_eqToSurvey)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  atEqToSurvey(a_ra, a_dec, &lambda, &eta);
  sprintf(answer,"lambda %13.8f",lambda); Tcl_AppendElement(interp, answer);
  sprintf(answer,"eta %13.8f",eta); Tcl_AppendElement(interp, answer);
  return TCL_OK;
}
/****************************************************************************
**
** ROUTINE: tclSurveyToEq
**
**<AUTO EXTRACT>
** TCL VERB: surveyToEq
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atSurveyToEq>atSurveyToEq</A> in atSurveyGeometry.c
** </HTML>
**</AUTO>
*/
char g_surveyToEq[] = "surveyToEq";
ftclArgvInfo g_surveyToEqTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "convert survey to equatorial coordinates, in decimal degrees.\nRETURN: keyed list with ra,dec\n" },
  {"<lambda>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "survey longitude"},
  {"<eta>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "survey latitude"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclSurveyToEq(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_lambda = 0.0;
  double a_eta = 0.0;
  double ra = 0.0;
  double dec = 0.0;

  g_surveyToEqTbl[1].dst = &a_lambda;
  g_surveyToEqTbl[2].dst = &a_eta;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_surveyToEqTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_surveyToEq)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  atSurveyToEq(a_lambda, a_eta, &ra, &dec);
  sprintf(answer,"ra %13.8f",ra); Tcl_AppendElement(interp, answer);
  sprintf(answer,"dec %13.8f",dec); Tcl_AppendElement(interp, answer);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclGCToSurvey
**
**<AUTO EXTRACT>
** TCL VERB: GCToSurvey
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atGCToSurvey>atGCToSurvey</A> in atSurveyGeometry.c
** </HTML>
**</AUTO>
*/
char g_GCToSurvey[] = "GCToSurvey";
ftclArgvInfo g_GCToSurveyTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "convert Great Circle to survey coordinates, in decimal degrees\n" },
   {"<mu>", FTCL_ARGV_DOUBLE, NULL, NULL, "mu on the great circle"},
   {"<nu>", FTCL_ARGV_DOUBLE, NULL, NULL, "nu on the great circle"},
   {"-node", FTCL_ARGV_DOUBLE, NULL, NULL, "node of the great circle"},
   {"-inclination", FTCL_ARGV_DOUBLE, NULL, NULL,  
         "inclination of the great circle"},
   { (char *)NULL, FTCL_ARGV_END, NULL, NULL, (char *)NULL }
}; 

static int
tclGCToSurvey(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)

{
  int rstat;
  double a_mu = 0.0;
  double a_nu = 0.0;
  double lambda = 0.0;
  double eta = 0.0;
  double a_node = at_surveyCenterRa - 90.0;
  double a_inclination = at_surveyCenterDec;
  g_GCToSurveyTbl[1].dst = &a_mu;
  g_GCToSurveyTbl[2].dst = &a_nu;
  g_GCToSurveyTbl[3].dst = &a_node;
  g_GCToSurveyTbl[4].dst = &a_inclination;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_GCToSurveyTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_GCToEq)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  atGCToSurvey( a_mu, a_nu, a_node, a_inclination, &lambda, &eta ); 

  sprintf(answer,"lambda %13.8f",lambda); Tcl_AppendElement(interp, answer);
  sprintf(answer,"eta %13.8f",eta); Tcl_AppendElement(interp, answer);

return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclSurveyToGC
**
**<AUTO EXTRACT>
** TCL VERB: surveyToGC
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atSurveyToGC>atSurveyToGC</A> in at SurveyGeometry.c
** </HTML>
**</AUTO>
*/
char g_surveyToGC[] = "surveyToGC";
ftclArgvInfo g_surveyToGCTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "convert survey to Great Circle coordinates, in decimal degrees\n" },
   {"<lambda>", FTCL_ARGV_DOUBLE, NULL, NULL, "survey latitude"},
   {"<eta>", FTCL_ARGV_DOUBLE, NULL, NULL, "survey longitude"},
   {"-node", FTCL_ARGV_DOUBLE, NULL, NULL, "node of the great circle"},
   {"-inclination", FTCL_ARGV_DOUBLE, NULL, NULL,  
         "inclination of the great circle"},
   { (char *)NULL, FTCL_ARGV_END, NULL, NULL, (char *)NULL }
}; 

static int
tclSurveyToGC(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{

  int rstat;
  double a_lambda = 0.0;
  double a_eta = 0.0;
  double a_node = at_surveyCenterRa - 90.0;
  double a_inclination = at_surveyCenterDec;
  double mu = 0.0;
  double nu = 0.0;

  g_surveyToGCTbl[1].dst = &a_lambda;
  g_surveyToGCTbl[2].dst = &a_eta;
  g_surveyToGCTbl[3].dst = &a_node;
  g_surveyToGCTbl[4].dst = &a_inclination;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_surveyToGCTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_surveyToEq)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  atSurveyToGC( a_lambda, a_eta, a_node, a_inclination, &mu, &nu );

  sprintf(answer,"mu %13.8f",mu); Tcl_AppendElement(interp, answer);
  sprintf(answer,"nu %13.8f",nu); Tcl_AppendElement(interp, answer);

  return TCL_OK;
}


/****************************************************************************
**
** ROUTINE: tclSurveyToAzelpa
**
**<AUTO EXTRACT>
** TCL VERB: surveyToAzelpa
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atSurveyToAzelpa>atSurveyToAzelpa</A> in atSurveyGeometry.c</A>
** </HTML>
**</AUTO>
*/
char g_surveyToAzelpa[] = "surveyToAzelpa";
ftclArgvInfo g_surveyToAzelpaTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "convert survey to azelpa coordinates, in decimal degrees.\nRETURN: keyed list of az, el, and pa\n" },
  {"<lambda>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "survey longitude"},
  {"<eta>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "survey latitude"},
  {"<tstamp>", FTCL_ARGV_STRING, NULL, NULL, 
   "TAI time stamp"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclSurveyToAzelpa(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_lambda = 0.0;
  double a_eta = 0.0;
  double az = 0.0;
  double el = 0.0;
  double pa = 0.0;
  char *a_tstamp = NULL;
  TSTAMP *tstamp=NULL;

  g_surveyToAzelpaTbl[1].dst = &a_lambda;
  g_surveyToAzelpaTbl[2].dst = &a_eta;
  g_surveyToAzelpaTbl[3].dst = &a_tstamp;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_surveyToAzelpaTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_surveyToAzelpa)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_tstamp, (void **) &tstamp, 
			   "TSTAMP") != TCL_OK) {
    Tcl_SetResult(interp, "bad TSTAMP name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  atSurveyToAzelpa(a_lambda, a_eta, tstamp, &az, &el, &pa);
  sprintf(answer,"az %13.8f",az); Tcl_AppendElement(interp, answer);
  sprintf(answer,"el %13.8f",el); Tcl_AppendElement(interp, answer);
  sprintf(answer,"pa %13.8f",pa); Tcl_AppendElement(interp, answer);
  return TCL_OK;
}


/****************************************************************************
**
** ROUTINE: tclVEqToGC
**
**<AUTO EXTRACT>
** TCL VERB: vEqToGC
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atVEqToGC>atVEqToGC</A> in atSurveyGeometry.c
** </HTML>
**</AUTO>
*/
char g_vEqToGC[] = "vEqToGC";
ftclArgvInfo g_vEqToGCTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "convert equatorial to great circle coordinates, in decimal degrees.\nRETURN a keyed list of mu and nu.\n" },
  {"<vRaMu>", FTCL_ARGV_STRING, NULL, NULL, 
   "right ascensions to change to mu"},
  {"<vDecNu>", FTCL_ARGV_STRING, NULL, NULL, 
   "declinations to change to nu"},
  {"-node", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "node of the reference great circle"},
  {"-inclination", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "inclination of the reference great circle"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVEqToGC(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_vRaMu = NULL; 
  char *a_vDecNu = NULL;
  VECTOR_TYPE a_node = at_surveyCenterRa - 90.0;
  VECTOR_TYPE a_inclination = at_surveyCenterDec;
  VECTOR *vRaMu = NULL;
  VECTOR *vDecNu = NULL;

  g_vEqToGCTbl[1].dst = &a_vRaMu;
  g_vEqToGCTbl[2].dst = &a_vDecNu;
  g_vEqToGCTbl[3].dst = &a_node;
  g_vEqToGCTbl[4].dst = &a_inclination;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_vEqToGCTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_vEqToGC)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  if (shTclAddrGetFromName
      (interp, a_vRaMu, (void **) &vRaMu, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVEqToGC:  bad VECTOR name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName
      (interp, a_vDecNu, (void **) &vDecNu, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVEqToGC:  bad VECTOR name", TCL_VOLATILE);
    return TCL_ERROR;
  }

  atVEqToGC(vRaMu, vDecNu, a_node, a_inclination);
  sprintf(answer,"mu %s",a_vRaMu); Tcl_AppendElement(interp, answer);
  sprintf(answer,"nu %s",a_vDecNu); Tcl_AppendElement(interp, answer);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclVGCToEq
**
**<AUTO EXTRACT>
** TCL VERB: vGCToEq
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atVGCToEq>atVGCToEq</A> in atSurveyGeometry.c
** </HTML>
**</AUTO>
*/
char g_vGCToEq[] = "vGCToEq";
ftclArgvInfo g_vGCToEqTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "convert equatorial to great circle coordinates, in decimal degrees.\nRETURN a keyed list of mu and nu.\n" },
  {"<vMuRa>", FTCL_ARGV_STRING, NULL, NULL, 
   "mu on the great circle to change to right ascensions"},
  {"<vNuDec>", FTCL_ARGV_STRING, NULL, NULL, 
   "nu on the great circle to change to right ascensions"},
  {"-node", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "node of the great circle"},
  {"-inclination", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "inclination of the great circle"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVGCToEq(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_vMuRa = NULL; 
  char *a_vNuDec = NULL;
  VECTOR_TYPE a_node = at_surveyCenterRa - 90.0;
  VECTOR_TYPE a_inclination = at_surveyCenterDec;
  VECTOR *vMuRa = NULL;
  VECTOR *vNuDec = NULL;

  g_vGCToEqTbl[1].dst = &a_vMuRa;
  g_vGCToEqTbl[2].dst = &a_vNuDec;
  g_vGCToEqTbl[3].dst = &a_node;
  g_vGCToEqTbl[4].dst = &a_inclination;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_vGCToEqTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_vGCToEq)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  if (shTclAddrGetFromName
      (interp, a_vMuRa, (void **) &vMuRa, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVGCToEq:  bad VECTOR name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName
      (interp, a_vNuDec, (void **) &vNuDec, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVGCToEq:  bad VECTOR name", TCL_VOLATILE);
    return TCL_ERROR;
  }

  atVGCToEq(vMuRa, vNuDec, a_node, a_inclination);
  Tcl_AppendElement(interp, answer);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclVEqToGal
**
**<AUTO EXTRACT>
** TCL VERB: vEqToGal
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atVEqToGal>atVEqToGal</A> in atSurveyGeometry.c
** </HTML>
**</AUTO>
*/
char g_vEqToGal[] = "vEqToGal";
ftclArgvInfo g_vEqToGalTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "convert equatorial coordinates to galactic coordinates, in decimal degrees.\nRETURN: keyed list with gLong and gLat \n" },
  {"<vRaGLong>", FTCL_ARGV_STRING, NULL, NULL, 
   "right ascension"},
  {"<vDecGLat>", FTCL_ARGV_STRING, NULL, NULL, 
   "declination"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVEqToGal(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_vRaGLong = NULL;
  char *a_vDecGLat = NULL;
  VECTOR *vRaGLong = NULL, *vDecGLat = NULL;
  g_vEqToGalTbl[1].dst = &a_vRaGLong;
  g_vEqToGalTbl[2].dst = &a_vDecGLat;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_vEqToGalTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_vEqToGal)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  if (shTclAddrGetFromName
      (interp, a_vRaGLong, (void **) &vRaGLong, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVEqToGal:  bad VECTOR name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName
      (interp, a_vDecGLat, (void **) &vDecGLat, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVEqToGal:  bad VECTOR name", TCL_VOLATILE);
    return TCL_ERROR;
  }

  atVEqToGal(vRaGLong, vDecGLat);
  sprintf(answer,"gLong %s",a_vRaGLong); Tcl_AppendElement(interp, answer);
  sprintf(answer,"gLat %s",a_vDecGLat); Tcl_AppendElement(interp, answer);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclVGalToEq
**
**<AUTO EXTRACT>
** TCL VERB: vGalToEq
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atVGalToEq>atVGalToEq</A> in atSurveyGeometry.c
** </HTML>
**</AUTO>
*/
char g_vGalToEq[] = "vGalToEq";
ftclArgvInfo g_vGalToEqTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "convert galactic coordinates to equatorial coordinates, in decimal degrees.\nRETURN: keyed list with ra and dec\n" },
  {"<vGLongRa>", FTCL_ARGV_STRING, NULL, NULL, 
   "galactic longitude"},
  {"<vGLatDec>", FTCL_ARGV_STRING, NULL, NULL, 
   "galactic latitude"},  
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVGalToEq(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_vGLongRa = NULL;
  char *a_vGLatDec = NULL;
  VECTOR *vGLongRa = NULL, *vGLatDec = NULL;
  g_vGalToEqTbl[1].dst = &a_vGLongRa;
  g_vGalToEqTbl[2].dst = &a_vGLatDec;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_vGalToEqTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_vGalToEq)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  if (shTclAddrGetFromName
      (interp, a_vGLongRa, (void **) &vGLongRa, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVGalToEq:  bad VECTOR name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName
      (interp, a_vGLatDec, (void **) &vGLatDec, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVGalToEq:  bad VECTOR name", TCL_VOLATILE);
    return TCL_ERROR;
  }

  atVGalToEq(vGLongRa, vGLatDec);
  sprintf(answer,"ra %s",a_vGLongRa); Tcl_AppendElement(interp, answer);
  sprintf(answer,"dec %s",a_vGLatDec); Tcl_AppendElement(interp, answer);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclVEqToSurvey
**
**<AUTO EXTRACT>
** TCL VERB: vEqToSurvey
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atVEqToSurvey>atVEqToSurvey</A> in atSurveyGeometry.c
** </HTML>
**</AUTO>
*/
char g_vEqToSurvey[] = "vEqToSurvey";
ftclArgvInfo g_vEqToSurveyTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "convert equatorial to survey coordinates, in decimal degrees.\nRETURN: keyed list with lambda, eta\n" },
  {"<vRaLambda>", FTCL_ARGV_STRING, NULL, NULL, 
   "right ascension"},
  {"<vDecEta>", FTCL_ARGV_STRING, NULL, NULL, 
   "declination"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVEqToSurvey(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_vRaLambda = NULL;
  char *a_vDecEta = NULL;
  VECTOR *vRaLambda = NULL, *vDecEta = NULL;
  g_vEqToSurveyTbl[1].dst = &a_vRaLambda;
  g_vEqToSurveyTbl[2].dst = &a_vDecEta;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_vEqToSurveyTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_vEqToSurvey)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  if (shTclAddrGetFromName
      (interp, a_vRaLambda, (void **) &vRaLambda, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVEqToSurvey:  bad VECTOR name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName
      (interp, a_vDecEta, (void **) &vDecEta, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVEqToSurvey:  bad VECTOR name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  atVEqToSurvey(vRaLambda, vDecEta);
  sprintf(answer,"lambda %s",a_vRaLambda); Tcl_AppendElement(interp, answer);
  sprintf(answer,"eta %s",a_vDecEta); Tcl_AppendElement(interp, answer);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclVSurveyToEq
**
**<AUTO EXTRACT>
** TCL VERB: vSurveyToEq
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atVSurveyToEq>atVSurveyToEq</A> in atSurveyGeometry.c
** </HTML>
**</AUTO>
*/
char g_vSurveyToEq[] = "vSurveyToEq";
ftclArgvInfo g_vSurveyToEqTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "convert survey to equatorial coordinates, in decimal degrees.\nRETURN: keyed list with ra,dec\n" },
  {"<vLambdaRa>", FTCL_ARGV_STRING, NULL, NULL, 
   "right ascension"},
  {"<vEtaDec>", FTCL_ARGV_STRING, NULL, NULL, 
   "declination"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVSurveyToEq(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_vLambdaRa = NULL;
  char *a_vEtaDec = NULL;
  VECTOR *vLambdaRa = NULL, *vEtaDec = NULL;
  g_vSurveyToEqTbl[1].dst = &a_vLambdaRa;
  g_vSurveyToEqTbl[2].dst = &a_vEtaDec;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_vSurveyToEqTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_vSurveyToEq)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  if (shTclAddrGetFromName
      (interp, a_vLambdaRa, (void **) &vLambdaRa, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVSurveyToEq:  bad VECTOR name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName
      (interp, a_vEtaDec, (void **) &vEtaDec, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVSurveyToEq:  bad VECTOR name", TCL_VOLATILE);
    return TCL_ERROR;
  }

  atVSurveyToEq(vLambdaRa, vEtaDec);
  sprintf(answer,"ra %s",a_vLambdaRa); Tcl_AppendElement(interp, answer);
  sprintf(answer,"dec %s",a_vEtaDec); Tcl_AppendElement(interp, answer);
  return TCL_OK;
}



/****************************************************************************
**
** ROUTINE: tclDefineGC
**
**<AUTO EXTRACT>
** TCL VERB: defineGC
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atDefineGC>atDefineGC</A> in atSurveyGeometry.c
** </HTML>
**</AUTO>
*/
char g_defineGC[] = "defineGC";
ftclArgvInfo g_defineGCTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Given two points on a great circle, compute the node and incl.\n" },
  {"<ra1>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "right ascension"},
  {"<dec1>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "declination"},
  {"<ra2>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "right ascension"},
  {"<dec2>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "declination"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclDefineGC(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_ra1, a_dec1, a_ra2, a_dec2, a_node, a_inclination;

  g_defineGCTbl[1].dst = &a_ra1;
  g_defineGCTbl[2].dst = &a_dec1;
  g_defineGCTbl[3].dst = &a_ra2;
  g_defineGCTbl[4].dst = &a_dec2;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_defineGCTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_defineGC)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (a_ra1==a_ra2 && a_dec1==a_dec2) {
    Tcl_SetResult(interp, "The two points must be different", TCL_VOLATILE);
    return TCL_ERROR;
  }

  a_ra1 = a_ra1*at_deg2Rad;
  a_dec1 = a_dec1*at_deg2Rad;
  a_ra2 = a_ra2*at_deg2Rad;
  a_dec2 = a_dec2*at_deg2Rad;

  atDefineGC(a_ra1, a_dec1, a_ra2, a_dec2, &a_node, &a_inclination);
  a_node = a_node*at_rad2Deg;
  a_inclination = a_inclination*at_rad2Deg;
  sprintf(answer,"node %13.8f",a_node); Tcl_AppendElement(interp, answer);
  sprintf(answer,"inc %13.8f",a_inclination); Tcl_AppendElement(interp, answer);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclGCDecFromRa
**
**<AUTO EXTRACT>
** TCL VERB: GCDecFromRa
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atGCDecFromRa>atGCDecFromRa</A> in atSurveyGeometry.c
** </HTML>
**</AUTO>
*/
char g_GCDecFromRa[] = "GCDecFromRa";
ftclArgvInfo g_GCDecFromRaTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Given an RA, find a corresponding dec on the great circle.\n" },
  {"<ra>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "right ascension"},
  {"-node", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "node of the reference great circle"},
  {"-inclination", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "inclination of the reference great circle"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclGCDecFromRa(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_ra = 0.0;
  double a_node = at_surveyCenterRa - 90.0;
  double a_inclination = at_surveyCenterDec;
  double a_dec;

  g_GCDecFromRaTbl[1].dst = &a_ra;
  g_GCDecFromRaTbl[2].dst = &a_node;
  g_GCDecFromRaTbl[3].dst = &a_inclination;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_GCDecFromRaTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_GCDecFromRa)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  a_ra = a_ra*at_deg2Rad;
  a_node = a_node*at_deg2Rad;
  a_inclination = a_inclination*at_deg2Rad;
  atGCDecFromRa(a_ra, a_node, a_inclination, &a_dec);
  a_dec = a_dec*at_rad2Deg;
  sprintf(answer,"%13.8f",a_dec); Tcl_SetResult(interp, answer, TCL_VOLATILE);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclAngleGCToRa
**
**<AUTO EXTRACT>
** TCL VERB: angleGCToRa
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atAngleGCToRa>atAngleGCToRa</A> in atSurveyGeometry.c
** </HTML>
**</AUTO>
*/
char g_angleGCToRa[] = "angleGCToRa";
ftclArgvInfo g_angleGCToRaTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Given a point on the great circle, determine the angle between\n the great circle and increasing ra.\n" },
  {"<ra>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "right ascension"},
  {"<dec>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "declination"},
  {"-node", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "node of the reference great circle"},
  {"-inclination", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "inclination of the reference great circle"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclAngleGCToRa(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_ra = 0.0;
  double a_dec = 0.0;
  double a_node = at_surveyCenterRa - 90.0;
  double a_inclination = at_surveyCenterDec;
  double angle;

  g_angleGCToRaTbl[1].dst = &a_ra;
  g_angleGCToRaTbl[2].dst = &a_dec;
  g_angleGCToRaTbl[3].dst = &a_node;
  g_angleGCToRaTbl[4].dst = &a_inclination;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_angleGCToRaTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_angleGCToRa)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  a_ra = a_ra*at_deg2Rad;
  a_dec = a_dec*at_deg2Rad;
  a_node = a_node*at_deg2Rad;
  a_inclination = a_inclination*at_deg2Rad;
  atAngleGCToRa(a_ra, a_dec, a_node, a_inclination, &angle);
  angle = angle*at_rad2Deg;
  sprintf(answer,"%13.8f",angle); Tcl_SetResult(interp, answer, TCL_VOLATILE);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclCoordToCobePix
**
**<AUTO EXTRACT>
** TCL VERB: coordToCobePix
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atCoordToCobePix>atCoordToCobePix</A> in atSurveyGeometry.c
** </HTML>
**</AUTO>
*/
char g_coordToCobePix[] = "coordToCobePix";
ftclArgvInfo g_coordToCobePixTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Given a sky position, coordinate projection, and resolution, return the COBE pixel number."},
  {"<longitude>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "sky position longitude"},
  {"<longitude>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "sky position latitude"},
  {"<coord>", FTCL_ARGV_STRING, NULL, NULL, 
   "coordinate system:  q for Equatorial; g for galactic; e for ecliptic"},
  {"-resolution", FTCL_ARGV_INT, NULL, NULL, 
   "on each face; 9 for DIRBE data"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclCoordToCobePix(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_longitude=0.0;
  double a_latitude=0.0;
  char *a_coord = NULL;
  char a_resolution = 9;
  float longitude=0.0, latitude=0.0;
  int pixel=0;

  g_coordToCobePixTbl[1].dst = &a_longitude;
  g_coordToCobePixTbl[2].dst = &a_latitude;
  g_coordToCobePixTbl[3].dst = &a_coord;
  g_coordToCobePixTbl[4].dst = &a_resolution;
  
  if ((rstat = shTclParseArgv(interp, &argc, argv, g_coordToCobePixTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_coordToCobePix)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  longitude = (float) a_longitude;
  latitude  = (float) a_latitude;

  atCoordToCobePix(longitude, latitude, a_coord, a_resolution, &pixel);
  sprintf(answer,"%d", pixel); 
  Tcl_SetResult(interp, answer, TCL_VOLATILE);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclCobePixToCoord
**
**<AUTO EXTRACT>
** TCL VERB: cobePixToCoord
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atCobePixToCoord>atCobePixToCoord</A> in atSurveyGeometry.c
** </HTML>
**</AUTO>
*/
char g_cobePixToCoord[] = "cobePixToCoord";
ftclArgvInfo g_cobePixToCoordTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Given a pixel number, coordinate projection, and resolution, return the coordinates of the center of the pixel."},
  {"<pixel>", FTCL_ARGV_INT, NULL, NULL, 
   "the pixel number"},
  {"<coord>", FTCL_ARGV_STRING, NULL, NULL, 
   "coordinate system:  q for Equatorial; g for galactic; e for ecliptic"},
  {"-resolution", FTCL_ARGV_INT, NULL, NULL, 
   "on each face; 9 for DIRBE data"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclCobePixToCoord(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  int a_pixel = 0;
  char *a_coord = NULL;
  char a_resolution = 9;
  float longitude=0.0, latitude=0.0;

  g_cobePixToCoordTbl[1].dst = &a_pixel;
  g_cobePixToCoordTbl[2].dst = &a_coord;
  g_cobePixToCoordTbl[3].dst = &a_resolution;
  
  if ((rstat = shTclParseArgv(interp, &argc, argv, g_cobePixToCoordTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_cobePixToCoord)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  atCobePixToCoord(a_pixel, a_coord, a_resolution, &longitude, &latitude);
  sprintf(answer,"longitude %13.8f", longitude); 
  Tcl_AppendElement(interp, answer);
  sprintf(answer,"latitude %13.8f", latitude); 
  Tcl_AppendElement(interp, answer);

  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclVCoordToCobePix
**
**<AUTO EXTRACT>
** TCL VERB: vCoordToCobePix
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atVCoordToCobePix>atVCoordToCobePix</A> in atSurveyGeometry.c
** </HTML>
**</AUTO>
*/
char g_vCoordToCobePix[] = "vCoordToCobePix";
ftclArgvInfo g_vCoordToCobePixTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "convert vectors of coordinates to COBE pixel numbers."},
  {"<vLongitude>", FTCL_ARGV_STRING, NULL, NULL, 
   "longitudes"},
  {"<vLatitude>", FTCL_ARGV_STRING, NULL, NULL, 
   "latitudes"},
  {"<coord>", FTCL_ARGV_STRING, NULL, NULL, 
   "coordinate system:  q for Equatorial; g for galactic; e for ecliptic"},
  {"-resolution", FTCL_ARGV_INT, NULL, NULL, 
   "on each face; 9 for DIRBE data"},
  {"<vPixel>", FTCL_ARGV_STRING, NULL, NULL, 
   "ouput pixel values"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVCoordToCobePix(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_longitude = NULL;
  char *a_latitude = NULL;
  char *a_pixel = NULL;
  VECTOR *vLongitude = NULL, *vLatitude = NULL, *vPixel = NULL;
  char *a_coord=NULL;
  int a_resolution=9;

  g_vCoordToCobePixTbl[1].dst = &a_longitude;
  g_vCoordToCobePixTbl[2].dst = &a_latitude;
  g_vCoordToCobePixTbl[3].dst = &a_coord;
  g_vCoordToCobePixTbl[4].dst = &a_resolution;
  g_vCoordToCobePixTbl[5].dst = &a_pixel;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_vCoordToCobePixTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_vCoordToCobePix)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  if (shTclAddrGetFromName
      (interp, a_longitude, (void **) &vLongitude, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVCoordToCobePix:  bad longitude VECTOR name", 
		  TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName
      (interp, a_latitude, (void **) &vLatitude, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVCoordToCobePix:  bad latitude VECTOR name", 
		  TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName
      (interp, a_pixel, (void **) &vPixel, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVCoordToCobePix:  bad pixel VECTOR name", 
		  TCL_VOLATILE);
    return TCL_ERROR;
  }

  atVCoordToCobePix(vLongitude, vLatitude, a_coord, a_resolution, vPixel);
  return TCL_OK;
}


/****************************************************************************
**
** ROUTINE: tclVCobePixToCoord
**
**<AUTO EXTRACT>
** TCL VERB: vCobePixToCoord
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atVCobePixToCoord>atVCobePixToCoord</A> in atSurveyGeometry.c
** </HTML>
**</AUTO>
*/
char g_vCobePixToCoord[] = "vCobePixToCoord";
ftclArgvInfo g_vCobePixToCoordTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "convert vectors of coordinates to COBE pixel numbers."},
  {"<vPixel>", FTCL_ARGV_STRING, NULL, NULL, 
   "ouput pixel values"},
  {"<coord>", FTCL_ARGV_STRING, NULL, NULL, 
   "coordinate system:  q for Equatorial; g for galactic; e for ecliptic"},
  {"-resolution", FTCL_ARGV_INT, NULL, NULL, 
   "on each face; 9 for DIRBE data"},
  {"<vLongitude>", FTCL_ARGV_STRING, NULL, NULL, 
   "longitudes"},
  {"<vLatitude>", FTCL_ARGV_STRING, NULL, NULL, 
   "latitudes"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclVCobePixToCoord(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_longitude = NULL;
  char *a_latitude = NULL;
  char *a_pixel = NULL;
  VECTOR *vLongitude = NULL, *vLatitude = NULL, *vPixel = NULL;
  char *a_coord=NULL;
  int a_resolution=9;

  g_vCobePixToCoordTbl[1].dst = &a_pixel;
  g_vCobePixToCoordTbl[2].dst = &a_coord;
  g_vCobePixToCoordTbl[3].dst = &a_resolution;
  g_vCobePixToCoordTbl[4].dst = &a_longitude;
  g_vCobePixToCoordTbl[5].dst = &a_latitude;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_vCobePixToCoordTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_vCobePixToCoord)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  if (shTclAddrGetFromName
      (interp, a_longitude, (void **) &vLongitude, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVCobePixToCoord:  bad longitude VECTOR name", 
		  TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName
      (interp, a_latitude, (void **) &vLatitude, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVCobePixToCoord:  bad latitude VECTOR name", 
		  TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName
      (interp, a_pixel, (void **) &vPixel, "VECTOR") != TCL_OK) {
    Tcl_SetResult(interp, "tclVCobePixToCoord:  bad pixel VECTOR name", 
		  TCL_VOLATILE);
    return TCL_ERROR;
  }

  atVCobePixToCoord(vPixel, a_coord, a_resolution, vLongitude, vLatitude);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclparallacticAngleGC
**
**<AUTO EXTRACT>
** TCL VERB: parallacticAngleGC
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atparallacticAngleGC>atparallacticAngleGC</A> in atSurveyGeometry.c</A>
** </HTML>
**</AUTO>
*/
char g_parallacticAngleGC[] = "parallacticAngleGC";
ftclArgvInfo g_parallacticAngleGCTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Calculate the parallactice angle to the zenith wrt mu,nu axes" },
  {"<mu>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "great circle longitude"},
  {"<nu>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "great circle latitude latitude"},
  {"<tstamp>", FTCL_ARGV_STRING, NULL, NULL, 
   "TAI time stamp"},
  {"<node>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "node of the great circle"},
  {"<incl>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "inclination of the great circle"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclparallacticAngleGC(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double mu = 0.0;
  double nu = 0.0;
  double node = 0.0;
  double incl = 0.0;
  double pa = 0.0;
  char *a_tstamp = NULL;
  TSTAMP *tstamp=NULL;

  g_parallacticAngleGCTbl[1].dst = &mu;
  g_parallacticAngleGCTbl[2].dst = &nu;
  g_parallacticAngleGCTbl[3].dst = &a_tstamp;
  g_parallacticAngleGCTbl[4].dst = &node;
  g_parallacticAngleGCTbl[5].dst = &incl;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_parallacticAngleGCTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_parallacticAngleGC)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (shTclAddrGetFromName(interp, a_tstamp, (void **) &tstamp, 
			   "TSTAMP") != TCL_OK) {
    Tcl_SetResult(interp, "bad TSTAMP name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  atParallacticAngleGC(mu, nu, tstamp, node, incl, &pa);
  sprintf(answer,"%.9f",pa); Tcl_AppendElement(interp, answer);
  return TCL_OK;
}


/****************************************************************************
**
** ROUTINE: tclsiteSetByName
**
**<AUTO EXTRACT>
** TCL VERB: siteSetByName
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atsiteSetByName>atsiteSetByName</A> in atSurveyGeometry.c</A>
** </HTML>
**</AUTO>
*/
char g_siteSetByName[] = "siteSetByName";
ftclArgvInfo g_siteSetByNameTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Set the location of the observatory" },
  {"<name>", FTCL_ARGV_STRING, NULL, NULL, 
   "name of observatory.  A legal list of name is returned on error"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclsiteSetByName(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *name = NULL;
  char *answer;

  g_siteSetByNameTbl[1].dst = &name;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_siteSetByNameTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_siteSetByName)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  answer = atSiteSetByName(interp, name);
  if (answer == NULL) {
    return TCL_OK;
  } else {
    Tcl_AppendElement(interp, answer);
    return TCL_ERROR;
  }
}
/****************************************************************************
**
** ROUTINE: tclsiteSet
**
**<AUTO EXTRACT>
** TCL VERB: siteSet
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atSurveyGeometry.html#atsiteSet>atsiteSet</A> in atSurveyGeometry.c</A>
** </HTML>
**</AUTO>
*/
char g_siteSet[] = "siteSet";
ftclArgvInfo g_siteSetTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Set the location of the observatory" },
  {"<longitude>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "geodetic longitude of observatory in degrees"},
  {"<latitude>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "geodetic latitude of observatory in degrees"},
  {"<altitude>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "altitude of observatory in meters"},
  {"<name>", FTCL_ARGV_STRING, NULL, NULL, 
   "name of the site"},

  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclsiteSet(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double longitude=0.0, latitude=0.0, altitude=0.0;
  char *name=NULL;

  g_siteSetTbl[1].dst = &longitude;
  g_siteSetTbl[2].dst = &latitude;
  g_siteSetTbl[3].dst = &altitude;
  g_siteSetTbl[4].dst = &name;
  if ((rstat = shTclParseArgv(interp, &argc, argv, g_siteSetTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_siteSet)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }
  atSiteSet(interp, longitude, latitude, altitude, name);
  return TCL_OK;
}




/***********************************************************************/

void atTclSurveyGeometryDeclare(Tcl_Interp *interp) {
  int flags = FTCL_ARGV_NO_LEFTOVERS;
  shTclDeclare(interp, g_eqToGC, 
	       (Tcl_CmdProc *) tclEqToGC,
	       (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
	       shTclGetArgInfo(interp, g_eqToGCTbl, flags, g_eqToGC),
	       shTclGetUsage(interp, g_eqToGCTbl, flags, g_eqToGC));
  shTclDeclare(interp, g_GCToEq, 
               (Tcl_CmdProc *) tclGCToEq,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_GCToEqTbl, flags, g_GCToEq),
               shTclGetUsage(interp, g_GCToEqTbl, flags, g_GCToEq));
 shTclDeclare(interp, g_eqToGal, 
               (Tcl_CmdProc *) tclEqToGal,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_eqToGalTbl, flags, g_eqToGal),
               shTclGetUsage(interp, g_eqToGalTbl, flags, g_eqToGal));
 shTclDeclare(interp, g_galToEq, 
               (Tcl_CmdProc *) tclGalToEq,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_galToEqTbl, flags, g_galToEq),
               shTclGetUsage(interp, g_galToEqTbl, flags, g_galToEq));
  shTclDeclare(interp, g_eqToSurvey, 
               (Tcl_CmdProc *) tclEqToSurvey,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_eqToSurveyTbl, flags, g_eqToSurvey),
               shTclGetUsage(interp, g_eqToSurveyTbl, flags, g_eqToSurvey));
  shTclDeclare(interp, g_surveyToEq, 
               (Tcl_CmdProc *) tclSurveyToEq,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_surveyToEqTbl, flags, g_surveyToEq),
               shTclGetUsage(interp, g_surveyToEqTbl, flags, g_surveyToEq));
  shTclDeclare(interp, g_GCToSurvey, 
               (Tcl_CmdProc *) tclGCToSurvey,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_GCToSurveyTbl, flags, g_GCToSurvey),
               shTclGetUsage(interp, g_GCToSurveyTbl, flags, g_GCToSurvey));
  shTclDeclare(interp, g_surveyToGC, 
               (Tcl_CmdProc *) tclSurveyToGC,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_surveyToGCTbl, flags, g_surveyToGC),
               shTclGetUsage(interp, g_surveyToGCTbl, flags, g_surveyToGC));
  shTclDeclare(interp, g_surveyToAzelpa, 
               (Tcl_CmdProc *) tclSurveyToAzelpa,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_surveyToAzelpaTbl, flags, 
			       g_surveyToAzelpa),
               shTclGetUsage(interp, g_surveyToAzelpaTbl, flags, 
			     g_surveyToAzelpa));
  shTclDeclare(interp, g_vEqToGC, 
	       (Tcl_CmdProc *) tclVEqToGC,
	       (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
	       shTclGetArgInfo(interp, g_vEqToGCTbl, flags, g_vEqToGC),
	       shTclGetUsage(interp, g_vEqToGCTbl, flags, g_vEqToGC));
  shTclDeclare(interp, g_vGCToEq, 
               (Tcl_CmdProc *) tclVGCToEq,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_vGCToEqTbl, flags, g_vGCToEq),
               shTclGetUsage(interp, g_vGCToEqTbl, flags, g_vGCToEq));
 shTclDeclare(interp, g_vEqToGal, 
               (Tcl_CmdProc *) tclVEqToGal,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_vEqToGalTbl, flags, g_vEqToGal),
               shTclGetUsage(interp, g_vEqToGalTbl, flags, g_vEqToGal));
 shTclDeclare(interp, g_vGalToEq, 
               (Tcl_CmdProc *) tclVGalToEq,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_vGalToEqTbl, flags, g_vGalToEq),
               shTclGetUsage(interp, g_vGalToEqTbl, flags, g_vGalToEq));
  shTclDeclare(interp, g_vEqToSurvey, 
               (Tcl_CmdProc *) tclVEqToSurvey,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_vEqToSurveyTbl, flags, g_vEqToSurvey),
               shTclGetUsage(interp, g_vEqToSurveyTbl, flags, g_vEqToSurvey));
  shTclDeclare(interp, g_vSurveyToEq, 
               (Tcl_CmdProc *) tclVSurveyToEq,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_vSurveyToEqTbl, flags, g_vSurveyToEq),
               shTclGetUsage(interp, g_vSurveyToEqTbl, flags, g_vSurveyToEq));
  shTclDeclare(interp, g_defineGC, 
               (Tcl_CmdProc *) tclDefineGC,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_defineGCTbl, flags, 
			       g_defineGC),
               shTclGetUsage(interp, g_defineGCTbl, flags, 
			     g_defineGC));
  shTclDeclare(interp, g_GCDecFromRa, 
               (Tcl_CmdProc *) tclGCDecFromRa,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_GCDecFromRaTbl, flags, 
			       g_GCDecFromRa),
               shTclGetUsage(interp, g_GCDecFromRaTbl, flags, 
			     g_GCDecFromRa));
  shTclDeclare(interp, g_angleGCToRa, 
               (Tcl_CmdProc *) tclAngleGCToRa,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_angleGCToRaTbl, flags, 
			       g_angleGCToRa),
               shTclGetUsage(interp, g_angleGCToRaTbl, flags, 
			     g_angleGCToRa));
  shTclDeclare(interp, g_cobePixToCoord, 
               (Tcl_CmdProc *) tclCobePixToCoord,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_cobePixToCoordTbl, flags, 
			       g_cobePixToCoord),
               shTclGetUsage(interp, g_cobePixToCoordTbl, flags, 
			     g_cobePixToCoord));
  shTclDeclare(interp, g_coordToCobePix, 
               (Tcl_CmdProc *) tclCoordToCobePix,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_coordToCobePixTbl, flags, 
			       g_coordToCobePix),
               shTclGetUsage(interp, g_coordToCobePixTbl, flags, 
			     g_coordToCobePix));
  shTclDeclare(interp, g_vCoordToCobePix, 
               (Tcl_CmdProc *) tclVCoordToCobePix,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_vCoordToCobePixTbl,
			       flags, g_vCoordToCobePix),
               shTclGetUsage(interp, g_vCoordToCobePixTbl, 
			     flags, g_vCoordToCobePix));
  shTclDeclare(interp, g_vCobePixToCoord, 
               (Tcl_CmdProc *) tclVCobePixToCoord,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_vCobePixToCoordTbl,
			       flags, g_vCobePixToCoord),
               shTclGetUsage(interp, g_vCobePixToCoordTbl, 
			     flags, g_vCobePixToCoord));
  shTclDeclare(interp, g_parallacticAngleGC, 
               (Tcl_CmdProc *) tclparallacticAngleGC,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_parallacticAngleGCTbl, flags, 
			       g_parallacticAngleGC),
               shTclGetUsage(interp, g_parallacticAngleGCTbl, flags, 
			     g_parallacticAngleGC));
  shTclDeclare(interp, g_siteSetByName, 
               (Tcl_CmdProc *) tclsiteSetByName,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_siteSetByNameTbl, flags, 
			       g_siteSetByName),
               shTclGetUsage(interp, g_siteSetByNameTbl, flags, 
			     g_siteSetByName));
  shTclDeclare(interp, g_siteSet, 
               (Tcl_CmdProc *) tclsiteSet,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_siteSetTbl, flags, 
			       g_siteSet),
               shTclGetUsage(interp, g_siteSetTbl, flags, 
			     g_siteSet));
}
