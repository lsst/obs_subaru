/* <AUTO>
   FILE: tclCali.c
   ABSTRACT:
<HTML>
   TCL bindings for applying photometric calibrations
</HTML>
   </AUTO>
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "dervish.h"
#include "atCali.h"

static char *tclFacil = "astrotools";    /* name of this set of code */
void alTclCaliDeclare(Tcl_Interp *interp);

/****************************************************************************
**
** ROUTINE: tclCaliApply
**
**<AUTO EXTRACT>
** TCL VERB: caliApply
** 
** <HTML>
** C ROUTINE CALLED:
** <A HREF=atCali.html#atCaliApply>atCaliApply</A> in atCali.c
** </HTML>
**</AUTO>
*/
char g_caliApply[] = "caliApply";
ftclArgvInfo g_caliApplyTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Apply the calibration to the observed counts to yield a magnitude:  \nRETURN:  a keyed list of calMag, calMagErr, status, and retInt\n" },
  {"<counts>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "number of counts in the reference filter"},
  {"<countsErr>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "error in counts"},
  {"<cCounts>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "number of counts in the adjacent filter"},
  {"<cCountsErr>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "error in cCounts"},
  {"<intTime>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "exposure time in the reference filter (seconds)"},
  {"<cIntTime>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "exposure time in the adjacent filter (seconds)"},
  {"<airmass>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "airmass:  sec of the zenith angle"},
  {"<zeropt>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "magnitude zeropoint for the reference filter (a')"},
  {"<extinct>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "extinction  for the reference filter (k')"},
  {"<colorTerm>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "color term (b')"},
  {"<secColorTerm>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "second order color term (c')"},
  {"<sign>", FTCL_ARGV_INT, NULL, NULL, 
   "usually +1 for u g r i and -1 for z"},
  {"<cosmicColor>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "typical color (reference-adjacent)"},
  {"<cosmicError>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "error in cosmicColor"},
  {"-zpAirmass", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "zeropoint airmass for c term; default=0.0"},
  {"-zpColor", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "zeropoint color for c term; default=0.0"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclCaliApply(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_counts = 0.0;
  double a_countsErr = 0.0;
  double a_cCounts = 0.0;
  double a_cCountsErr = 0.0;
  double a_intTime = 0.0;
  double a_cIntTime = 0.0;
  double a_airmass = 0.0;
  double a_zeropt = 0.0;
  double a_extinct = 0.0;
  double a_colorTerm = 0.0;
  double a_secColorTerm = 0.0;
  int a_sign = 0;
  double a_cosmicColor = 0.0;
  double a_cosmicError = 0.0;
  double a_zpAirmass = 0.0;
  double a_zpColor = 0.0;
  double calMag = 0.0, calMagErr = 0.0;
  int status = 0.0;
  int retInt;
  char answer[100];

  g_caliApplyTbl[1].dst = &a_counts;
  g_caliApplyTbl[2].dst = &a_countsErr;
  g_caliApplyTbl[3].dst = &a_cCounts;
  g_caliApplyTbl[4].dst = &a_cCountsErr;
  g_caliApplyTbl[5].dst = &a_intTime;
  g_caliApplyTbl[6].dst = &a_cIntTime;
  g_caliApplyTbl[7].dst = &a_airmass;
  g_caliApplyTbl[8].dst = &a_zeropt;
  g_caliApplyTbl[9].dst = &a_extinct;
  g_caliApplyTbl[10].dst = &a_colorTerm;
  g_caliApplyTbl[11].dst = &a_secColorTerm;
  g_caliApplyTbl[12].dst = &a_sign;
  g_caliApplyTbl[13].dst = &a_cosmicColor;
  g_caliApplyTbl[14].dst = &a_cosmicError;
  g_caliApplyTbl[15].dst = &a_zpAirmass;
  g_caliApplyTbl[16].dst = &a_zpColor;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_caliApplyTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_caliApply)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  retInt = atCaliApply(a_counts, a_countsErr, a_cCounts, a_cCountsErr,
		       a_intTime, a_cIntTime, a_airmass, a_zeropt, a_extinct,
		       a_colorTerm, a_secColorTerm, a_sign,
		       a_cosmicColor, a_cosmicError,
		       a_zpAirmass, a_zpColor, 
		       &calMag, &calMagErr, &status);

  sprintf(answer,"calMag %f"   , calMag);    Tcl_AppendElement(interp, answer);
  sprintf(answer,"calMagErr %f", calMagErr); Tcl_AppendElement(interp, answer);
  sprintf(answer,"status %d"   , status);    Tcl_AppendElement(interp, answer);
  sprintf(answer,"retInt %d"   , status);    Tcl_AppendElement(interp, answer);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclLCaliApply
**
**<AUTO EXTRACT>
** TCL VERB: LcaliApply
**
**<HTML>
** C ROUTINE CALLED:
**  <A HREF=atCali.html#atLCaliApply>atLCaliApply</A> in atCali.c
**</HTML>
**</AUTO>
*/
char g_LcaliApply[] = "lcaliApply";
ftclArgvInfo g_LcaliApplyTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Apply the calibration to the observed counts to yield a luptitude:  \nRETURN:  a keyed list of calMag, calMagErr, status, and retInt\n" },
  {"<counts>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "number of counts in the reference filter"},
  {"<countsErr>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "error in counts"},
  {"<cCounts>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "number of counts in the adjacent filter"},
  {"<cCountsErr>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "error in cCounts"},
  {"<intTime>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "exposure time in the reference filter (seconds)"},
  {"<cIntTime>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "exposure time in the adjacent filter (seconds)"},
  {"<airmass>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "airmass:  sec of the zenith angle"},
  {"<zeropt>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "magnitude zeropoint for the reference filter (a')"},
  {"<extinct>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "extinction  for the reference filter (k')"},
  {"<colorTerm>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "color term (b')"},
  {"<secColorTerm>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "second order color term (c')"},
  {"<sign>", FTCL_ARGV_INT, NULL, NULL, 
   "usually +1 for u g r i and -1 for z"},
  {"<cosmicColor>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "typical color (reference-adjacent)"},
  {"<cosmicError>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "error in cosmicColor"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclLCaliApply(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_counts = 0.0;
  double a_countsErr = 0.0;
  double a_cCounts = 0.0;
  double a_cCountsErr = 0.0;
  double a_intTime = 0.0;
  double a_cIntTime = 0.0;
  double a_airmass = 0.0;
  double a_zeropt = 0.0;
  double a_extinct = 0.0;
  double a_colorTerm = 0.0;
  double a_secColorTerm = 0.0;
  int a_sign = 0;
  double a_cosmicColor = 0.0;
  double a_cosmicError = 0.0;
  double calMag = 0.0, calMagErr = 0.0;
  int status = 0.0;
  int retInt;
  char answer[100];

  g_LcaliApplyTbl[1].dst = &a_counts;
  g_LcaliApplyTbl[2].dst = &a_countsErr;
  g_LcaliApplyTbl[3].dst = &a_cCounts;
  g_LcaliApplyTbl[4].dst = &a_cCountsErr;
  g_LcaliApplyTbl[5].dst = &a_intTime;
  g_LcaliApplyTbl[6].dst = &a_cIntTime;
  g_LcaliApplyTbl[7].dst = &a_airmass;
  g_LcaliApplyTbl[8].dst = &a_zeropt;
  g_LcaliApplyTbl[9].dst = &a_extinct;
  g_LcaliApplyTbl[10].dst = &a_colorTerm;
  g_LcaliApplyTbl[11].dst = &a_secColorTerm;
  g_LcaliApplyTbl[12].dst = &a_sign;
  g_LcaliApplyTbl[13].dst = &a_cosmicColor;
  g_LcaliApplyTbl[14].dst = &a_cosmicError;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_LcaliApplyTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_LcaliApply)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  retInt = atLCaliApply(a_counts, a_countsErr, a_cCounts, a_cCountsErr,
		       a_intTime, a_cIntTime, a_airmass, a_zeropt, a_extinct,
		       a_colorTerm, a_secColorTerm, a_sign,
		       a_cosmicColor, a_cosmicError,
		       &calMag, &calMagErr, &status);

  sprintf(answer,"calMag %f"   , calMag);    Tcl_AppendElement(interp, answer);
  sprintf(answer,"calMagErr %f", calMagErr); Tcl_AppendElement(interp, answer);
  sprintf(answer,"status %d"   , status);    Tcl_AppendElement(interp, answer);
  sprintf(answer,"retInt %d"   , status);    Tcl_AppendElement(interp, answer);
  return TCL_OK;
}


/****************************************************************************
**
** ROUTINE: tclnLCaliApply
**
**<AUTO EXTRACT>
** TCL VERB: nLcaliApply
**
**<HTML>
** C ROUTINE CALLED:
**  <A HREF=atCali.html#atnLCaliApply>atnLCaliApply</A> in atCali.c
**</HTML>
**</AUTO>
*/
char g_nLcaliApply[] = "nlcaliApply";
ftclArgvInfo g_nLcaliApplyTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Apply the calibration to the observed counts to yield a luptitude:  \nRETURN:  a keyed list of calMag, calMagErr, status, and retInt\n" },
  {"<counts>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "number of counts in the reference filter"},
  {"<countsErr>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "error in counts"},
  {"<cCounts>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "number of counts in the adjacent filter"},
  {"<cCountsErr>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "error in cCounts"},
  {"<intTime>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "exposure time in the reference filter (seconds)"},
  {"<cIntTime>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "exposure time in the adjacent filter (seconds)"},
  {"<airmass>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "airmass:  sec of the zenith angle"},
  {"<zeropt>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "magnitude zeropoint for the reference filter (a)"},
  {"<extinct>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "extinction  for the reference filter (k)"},
  {"<colorTerm>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "color term (b)"},
  {"<filterIndex>", FTCL_ARGV_INT, NULL, NULL, 
   "determines which luptitude b prime to use; 0=u', 1=g', 2=r', 3=i', 4=z'"},
  {"-secColorTerm", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "second order color term (c), (default(fI)= -0.021,-0.016,-0.004,0.006,0.003"},
  {"-cosmicColor", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "typical instrumental color (reference-adjacent), default(i)=2.29,0.70,0.10,-1.3,-1.3"},
  {"-cosmicError", FTCL_ARGV_DOUBLE, NULL, NULL, "error in cosmicColor"},
  {"-zpAirmass", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "zeropoint airmass for c term; default=1.3"},
  {"-zpColor", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "zeropoint instrumental color for c term; default(filterIndex)=2.29,0.70,0.10,-1.3,-1.3"},
  {"-faintbprime", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "if > 0, then use the given bprime default is filter specific"},
  {"-maggies", FTCL_ARGV_INT, NULL, NULL, 
   "if =1, then return maggies (linear flux units where maggie=1 <--> mag=0.0), if = 2, then return approximate Jy value"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclnLCaliApply(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_counts = 0.0;
  double a_countsErr = 0.0;
  double a_cCounts = 0.0;
  double a_cCountsErr = 0.0;
  double a_intTime = 0.0;
  double a_cIntTime = 0.0;
  double a_airmass = 0.0;
  double a_zeropt = 0.0;
  double a_extinct = 0.0;
  double a_colorTerm = 0.0;
  double a_secColorTerm = -1000.0;
  double a_zpAirmass = 1.3;
  double a_zpColor = -1000.0;
  int a_filterIndex = 2;
  double a_faintbprime = 0;
  int a_maggies = 0;
  double a_cosmicColor = -1000.0;
  double a_cosmicError = 0.0;
  double calMag = 0.0, calMagErr = 0.0;
  int status = 0.0;
  int retInt;
  char answer[100];

  g_nLcaliApplyTbl[1].dst = &a_counts;
  g_nLcaliApplyTbl[2].dst = &a_countsErr;
  g_nLcaliApplyTbl[3].dst = &a_cCounts;
  g_nLcaliApplyTbl[4].dst = &a_cCountsErr;
  g_nLcaliApplyTbl[5].dst = &a_intTime;
  g_nLcaliApplyTbl[6].dst = &a_cIntTime;
  g_nLcaliApplyTbl[7].dst = &a_airmass;
  g_nLcaliApplyTbl[8].dst = &a_zeropt;
  g_nLcaliApplyTbl[9].dst = &a_extinct;
  g_nLcaliApplyTbl[10].dst = &a_colorTerm;
  g_nLcaliApplyTbl[11].dst = &a_filterIndex;
  g_nLcaliApplyTbl[12].dst = &a_secColorTerm;
  g_nLcaliApplyTbl[13].dst = &a_cosmicColor;
  g_nLcaliApplyTbl[14].dst = &a_cosmicError;
  g_nLcaliApplyTbl[15].dst = &a_zpAirmass;
  g_nLcaliApplyTbl[16].dst = &a_zpColor;
  g_nLcaliApplyTbl[17].dst = &a_faintbprime;
  g_nLcaliApplyTbl[18].dst = &a_maggies;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_nLcaliApplyTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_nLcaliApply)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (a_filterIndex < 0 || a_filterIndex > 4) {
	calMag = -1000;
	calMagErr = -1000;
	status = 2;
  } else {
  retInt = atnLCaliApply(a_counts, a_countsErr, a_cCounts, a_cCountsErr,
		       a_intTime, a_cIntTime, a_airmass, a_zeropt, a_extinct,
		       a_colorTerm, a_secColorTerm,
		       a_cosmicColor, a_cosmicError,a_zpAirmass,a_zpColor,
			a_filterIndex, a_faintbprime,a_maggies,
		       &calMag, &calMagErr, &status);
  }

	if (a_maggies>0) {
  sprintf(answer,"calMag %g"   , calMag);    Tcl_AppendElement(interp, answer);
  sprintf(answer,"calMagErr %g", calMagErr); Tcl_AppendElement(interp, answer);
	} else {
  sprintf(answer,"calMag %f"   , calMag);    Tcl_AppendElement(interp, answer);
  sprintf(answer,"calMagErr %f", calMagErr); Tcl_AppendElement(interp, answer);
	}
  sprintf(answer,"status %d"   , status);    Tcl_AppendElement(interp, answer);
  sprintf(answer,"retInt %d"   , status);    Tcl_AppendElement(interp, answer);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclnLCaliInvert
**
**<AUTO EXTRACT>
** TCL VERB: nLcaliInvert
**
**<HTML>
** C ROUTINE CALLED:
**  <A HREF=atCali.html#atnLCaliInvert>atnLCaliInvert</A> in atCali.c
**</HTML>
**</AUTO>
*/
char g_nLcaliInvert[] = "nlcaliInvert";
ftclArgvInfo g_nLcaliInvertTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Apply the inverse luptitude to get a maggies:  \nRETURN:  maggies, maggiesErr\n" },
  {"<calMag>", FTCL_ARGV_DOUBLE, NULL, NULL, "asinh magnitude"},
  {"<calMagErr>", FTCL_ARGV_DOUBLE, NULL, NULL, "asinh magnitude error (in mags)"},
  {"<filterIndex>", FTCL_ARGV_INT, NULL, NULL, "determines which luptitude b' knee to use; 0=u', 1=g', 2=r', 3=i', 4=z'"},
  {"-faintbprime", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "if > 0, then use the given b' knee, the default is filter specific"},
  {"-Jy", FTCL_ARGV_CONSTANT, (void *)1, NULL, 
   "if present then return Janskys instead of maggies (linear flux units where maggie=1 <--> mag=0.0)"},
  {"-flux0", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "if >0 and -Jy on, then return a flux in Jy which is maggies * this flux0 val (default 3630.78Jy = 1.0 maggie)"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclnLCaliInvert(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_calmag = 0.0;
  double a_calmagerr = 0.0;
  int a_filterIndex = 2;
  double a_faintbprime = 0.0;
  int a_Jy = 0;
  int a_maggies = 1;
  double a_flux0 = 0.0;
  int status = 0.0;
  int retInt;
  double counts,countserr;
  char answer[100];

  g_nLcaliInvertTbl[1].dst = &a_calmag;
  g_nLcaliInvertTbl[2].dst = &a_calmagerr;
  g_nLcaliInvertTbl[3].dst = &a_filterIndex;
  g_nLcaliInvertTbl[4].dst = &a_faintbprime;
  g_nLcaliInvertTbl[5].dst = &a_Jy;
  g_nLcaliInvertTbl[6].dst = &a_flux0;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_nLcaliInvertTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_nLcaliInvert)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  if (a_Jy) a_maggies = 2;

  if (a_filterIndex < 0 || a_filterIndex > 4) {
	counts = -1000;
	countserr = -1000;
	status = 2;
  } else {
  retInt = atnLCaliInvert(a_calmag,a_calmagerr,a_filterIndex,a_maggies,a_faintbprime,a_flux0, &counts,&countserr);
  }

  sprintf(answer,"flux %g"   , counts);    Tcl_AppendElement(interp, answer);
  sprintf(answer,"fluxErr %g", countserr); Tcl_AppendElement(interp, answer);
  sprintf(answer,"filter %d"   , a_filterIndex);    Tcl_AppendElement(interp, answer);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclCaliInvert
**
**<AUTO EXTRACT>
** TCL VERB: caliInvert
**
**<HTML>
** C ROUTINE CALLED:
**  <A HREF=atCali.html#atCaliInvert>atCaliInvert</A> in atCali.c
**</HTML>
**</AUTO>
*/
char g_caliInvert[] = "caliInvert";
ftclArgvInfo g_caliInvertTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Invert the calibration to the observed counts to yield a magnitude:  \nRETURN:  a keyed list of calMag, calMagErr, status, and retInt\n" },
  {"<calMag>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "calibrated magnitude in this band"},
  {"<cCalMag>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "calibrated magniude in the adjacent band"},
  {"<intTime>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "exposure time in the reference filter (seconds)"},
  {"<airmass>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "airmass:  sec of the zenith angle"},
  {"<zeropt>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "magnitude zeropoint for the reference filter (a')"},
  {"<extinct>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "extinction  for the reference filter (k')"},
  {"<colorTerm>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "color term (b')"},
  {"<secColorTerm>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "second order color term (c')"},
  {"<sign>", FTCL_ARGV_INT, NULL, NULL, 
   "usually +1 for u g r i and -1 for z"},
  {"-zpAirmass", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "zeropoint airmass for c term; default=0.0"},
  {"-zpColor", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "zeropoint color for c term; default=0.0"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclCaliInvert(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_calMag = 0.0;
  double a_cCalMag = 0.0;
  double a_intTime = 0.0;
  double a_airmass = 0.0;
  double a_zeropt = 0.0;
  double a_extinct = 0.0;
  double a_colorTerm = 0.0;
  double a_secColorTerm = 0.0;
  int a_sign = 0;
  double a_zpAirmass = 0.0;
  double a_zpColor = 0.0;
  double counts = 0.0;
  int retInt;
  char answer[100];

  g_caliInvertTbl[1].dst = &a_calMag;
  g_caliInvertTbl[2].dst = &a_cCalMag;
  g_caliInvertTbl[3].dst = &a_intTime;
  g_caliInvertTbl[4].dst = &a_airmass;
  g_caliInvertTbl[5].dst = &a_zeropt;
  g_caliInvertTbl[6].dst = &a_extinct;
  g_caliInvertTbl[7].dst = &a_colorTerm;
  g_caliInvertTbl[8].dst = &a_secColorTerm;
  g_caliInvertTbl[9].dst = &a_sign;
  g_caliInvertTbl[10].dst = &a_zpAirmass;
  g_caliInvertTbl[11].dst = &a_zpColor;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_caliInvertTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_caliInvert)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  retInt = atCaliInvert(a_calMag, a_cCalMag, a_intTime, 
			a_airmass, a_zeropt, a_extinct,
			a_colorTerm, a_secColorTerm, a_sign,
			a_zpAirmass, a_zpColor,
			&counts);
  sprintf(answer,"%g"   , counts);    Tcl_AppendElement(interp, answer);
  return TCL_OK;
}


/****************************************************************************
**
** ROUTINE: tclLCaliInvert
**
**<AUTO EXTRACT>
** TCL VERB: LcaliInvert
**
**<HTML>
** C ROUTINE CALLED:
**  <A HREF=atCali.html#atLCaliInvert>atLCaliInvert</A> in atCali.c
**</HTML>
**</AUTO>
*/
char g_LcaliInvert[] = "lcaliInvert";
ftclArgvInfo g_LcaliInvertTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Invert the calibration on the luptitude to yield the observed counts:  \nRETURN:  a keyed list of calMag, calMagErr, status, and retInt\n" },
  {"<calMag>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "calibrated luptitude in this band"},
  {"<cCalMag>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "calibrated luptitude in the adjacent band"},
  {"<intTime>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "exposure time in the reference filter (seconds)"},
  {"<airmass>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "airmass:  sec of the zenith angle"},
  {"<zeropt>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "magnitude zeropoint for the reference filter (a')"},
  {"<extinct>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "extinction  for the reference filter (k')"},
  {"<colorTerm>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "color term (b')"},
  {"<secColorTerm>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "second order color term (c')"},
  {"<sign>", FTCL_ARGV_INT, NULL, NULL, 
   "usually +1 for u g r i and -1 for z"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclLCaliInvert(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_calMag = 0.0;
  double a_cCalMag = 0.0;
  double a_intTime = 0.0;
  double a_airmass = 0.0;
  double a_zeropt = 0.0;
  double a_extinct = 0.0;
  double a_colorTerm = 0.0;
  double a_secColorTerm = 0.0;
  int a_sign = 0;
  double counts = 0.0;
  int retInt;
  char answer[100];

  g_LcaliInvertTbl[1].dst = &a_calMag;
  g_LcaliInvertTbl[2].dst = &a_cCalMag;
  g_LcaliInvertTbl[3].dst = &a_intTime;
  g_LcaliInvertTbl[4].dst = &a_airmass;
  g_LcaliInvertTbl[5].dst = &a_zeropt;
  g_LcaliInvertTbl[6].dst = &a_extinct;
  g_LcaliInvertTbl[7].dst = &a_colorTerm;
  g_LcaliInvertTbl[8].dst = &a_secColorTerm;
  g_LcaliInvertTbl[9].dst = &a_sign;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_LcaliInvertTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_LcaliInvert)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  retInt = atLCaliInvert(a_calMag, a_cCalMag, a_intTime, 
			a_airmass, a_zeropt, a_extinct,
			a_colorTerm, a_secColorTerm, a_sign,
			&counts);
  sprintf(answer,"%g"   , counts);    Tcl_AppendElement(interp, answer);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclLupApply
**
**<AUTO EXTRACT>
** TCL VERB: cnts2Lup
**
**<HTML>
** C ROUTINE CALLED:
**  <A HREF=atCali.html#atLupApply>atLupApply</A> in atCali.c
**</HTML>
**</AUTO>
*/
char g_lupApply[] = "cnts2Lup";
ftclArgvInfo g_lupApplyTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Convert counts to Luptitudes:  \nRETURN:  a keyed list of Luptitude and error\n" },
  {"<counts>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "counts"},
  {"<countserr>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "error in the counts"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclLupApply(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_counts = 0.0;
  double a_countsErr = 0.0;
  double lup, lupErr;
  int retInt;
  char answer[100];

  g_lupApplyTbl[1].dst = &a_counts;
  g_lupApplyTbl[2].dst = &a_countsErr;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_lupApplyTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_lupApply)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  retInt = atLupApply(a_counts, a_countsErr,
			&lup, &lupErr);
  sprintf(answer,"{luptitude %g} {error %g}", lup, lupErr);
  Tcl_SetResult(interp, answer, TCL_VOLATILE);
  return TCL_OK;
}

/****************************************************************************
**
** ROUTINE: tclLupInvert
**
**<AUTO EXTRACT>
** TCL VERB: lup2Cnts
**
**<HTML>
** C ROUTINE CALLED:
**  <A HREF=atCali.html#atLupApply>atLupApply</A> in atCali.c
**</HTML>
**</AUTO>
*/
char g_lupInvert[] = "lup2Cnts";
ftclArgvInfo g_lupInvertTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Convert Luptitudes to counts:  \nRETURN:  a keyed list of counts and error\n" },
  {"<Luptitude>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "Luptitude"},
  {"<LuptitudeErr>", FTCL_ARGV_DOUBLE, NULL, NULL, 
   "error in the Luptitude"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclLupInvert(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_luptitude = 0.0;
  double a_luptitudeErr = 0.0;
  double counts, countsErr;
  int retInt;
  char answer[100];

  g_lupInvertTbl[1].dst = &a_luptitude;
  g_lupInvertTbl[2].dst = &a_luptitudeErr;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_lupInvertTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_lupInvert)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  retInt = atLupInvert(a_luptitude, a_luptitudeErr,
			&counts, &countsErr);
  sprintf(answer,"{counts %g} {error %g}", counts, countsErr);
  Tcl_SetResult(interp, answer, TCL_VOLATILE);
  return TCL_OK;
}

void atTclCaliDeclare(Tcl_Interp *interp) {

  int flags = FTCL_ARGV_NO_LEFTOVERS;
  
  shTclDeclare(interp, g_caliApply, 
               (Tcl_CmdProc *) tclCaliApply,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_caliApplyTbl, flags, g_caliApply),
               shTclGetUsage(interp, g_caliApplyTbl, flags, g_caliApply));

  shTclDeclare(interp, g_caliInvert, 
               (Tcl_CmdProc *) tclCaliInvert,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_caliInvertTbl, flags, g_caliInvert),
               shTclGetUsage(interp, g_caliInvertTbl, flags, g_caliInvert));

  shTclDeclare(interp, g_LcaliApply, 
               (Tcl_CmdProc *) tclLCaliApply,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_LcaliApplyTbl, flags, g_LcaliApply),
               shTclGetUsage(interp, g_LcaliApplyTbl, flags, g_LcaliApply));

  shTclDeclare(interp, g_nLcaliApply, 
               (Tcl_CmdProc *) tclnLCaliApply,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_nLcaliApplyTbl, flags, g_nLcaliApply),
               shTclGetUsage(interp, g_nLcaliApplyTbl, flags, g_nLcaliApply));

  shTclDeclare(interp, g_nLcaliInvert, 
               (Tcl_CmdProc *) tclnLCaliInvert,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_nLcaliInvertTbl, flags, g_nLcaliInvert),
               shTclGetUsage(interp, g_nLcaliInvertTbl, flags, g_nLcaliInvert));

  shTclDeclare(interp, g_LcaliInvert, 
               (Tcl_CmdProc *) tclLCaliInvert,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_LcaliInvertTbl, flags, g_LcaliInvert),
               shTclGetUsage(interp, g_LcaliInvertTbl, flags, g_LcaliInvert));

  shTclDeclare(interp, g_lupApply, 
               (Tcl_CmdProc *) tclLupApply,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_lupApplyTbl, flags, g_lupApply),
               shTclGetUsage(interp, g_lupApplyTbl, flags, g_lupApply));

  shTclDeclare(interp, g_lupInvert, 
               (Tcl_CmdProc *) tclLupInvert,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclFacil,
               shTclGetArgInfo(interp, g_lupInvertTbl, flags, g_lupInvert),
               shTclGetUsage(interp, g_lupInvertTbl, flags, g_lupInvert));

}
