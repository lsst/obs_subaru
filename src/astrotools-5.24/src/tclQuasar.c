/* <AUTO>
   FILE: tclQuasar
   ABSTRACT:
<HTML>
   Routines to select quasars.
</HTML>
   </AUTO>
*/
/***************************************************************
 *
 *
 **************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "dervish.h"
#include "atQuasar.h"

#define MAXLABELSIZE 100

static char *module = "astrotools";   /* name of this set of code */

/**************************************************************
                      tclQuLocusCheck
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: quLocusCheck

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atQuasar.html#atQuLocusRemove>atQuLocusRemove</A> in atQuasar.c

<P>
Given a file of locus points and a single point with variances/covariances, determine
whether or not it is in the locus.

  </HTML>

</AUTO>*/

static ftclArgvInfo atQuLocusCheckTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL,
  "Check whether a given object is outside a 3D stellar locus"},
  {"<coordlist>", FTCL_ARGV_STRING, NULL, NULL, "{x y z}"},
  {"<variancelist>", FTCL_ARGV_STRING, NULL, NULL, "{var(x) cov(xy) cov(xz) var(y) cov(yz) var(z)} (NULL if none available)"},
  {"<locusfilename>", FTCL_ARGV_STRING, NULL, NULL, "locus file"},
  {"<maxnpts>", FTCL_ARGV_INT, NULL, NULL, "maximum number of locus points"},
  {"<mink>", FTCL_ARGV_DOUBLE, NULL, NULL, "minimum k in locus"},
  {"<minkdist>", FTCL_ARGV_DOUBLE, NULL, NULL, "k width of end ellipse"},
  {"<maxk>", FTCL_ARGV_DOUBLE, NULL, NULL, "maximum k in locus"},
  {"<maxkdist>", FTCL_ARGV_DOUBLE, NULL, NULL, "k width of end ellipse"},
  {"<nsigma>", FTCL_ARGV_DOUBLE, NULL, NULL, "number of sigma of errors to convolve with locus"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static char *atQuLocusCheck_name = "quLocusCheck";

static int
tclQuLocusCheck(
                ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                )
{
                               /* INPUT */
  char *coordlist;              /* list of ascii files of points */
  char *variancelist;         /* list of ascii files of associated errors */
  char *locusfilename;         /* list of ascii files of locus points */
  int l_argc;
  char**l_argv;			/* number and names of the list elements */
  char *ptr = NULL;
  int counter, axis;
  char errors[80], result[80];
  double *xt,*yt,*zt,*kx,*ky,*kz,*distmax,*distmin,*theta;
  int npts, maxnpts;
  double maxk, maxkdist, mink, minkdist, nsigma;
  int returnValue = 0;
  FILE *currentfile;
  double datapoint[3];
  double dataerrors[6];
  int inlocus;
  int closestpoint;
  double ellipsedist;
  double koffset;
  int ninbin;

/*  static char *function_name = "quLocusCheck"; */

/* Parse the command */

  atQuLocusCheckTable[1].dst = &coordlist;
  atQuLocusCheckTable[2].dst = &variancelist;
  atQuLocusCheckTable[3].dst = &locusfilename;
  atQuLocusCheckTable[4].dst = &maxnpts;
  atQuLocusCheckTable[5].dst = &mink;
  atQuLocusCheckTable[6].dst = &minkdist;
  atQuLocusCheckTable[7].dst = &maxk;
  atQuLocusCheckTable[8].dst = &maxkdist;
  atQuLocusCheckTable[9].dst = &nsigma;

  if ((returnValue = shTclParseArgv(
    interp, &argc, argv, atQuLocusCheckTable, 0, atQuLocusCheck_name))
                                          != FTCL_ARGV_SUCCESS) {
    return(returnValue);
  }

  xt = (double*)shMalloc(maxnpts*sizeof(double));	
  yt = (double*)shMalloc(maxnpts*sizeof(double));	
  zt = (double*)shMalloc(maxnpts*sizeof(double));	
  kx = (double*)shMalloc(maxnpts*sizeof(double));	
  ky = (double*)shMalloc(maxnpts*sizeof(double));	
  kz = (double*)shMalloc(maxnpts*sizeof(double));	
  distmax = (double*)shMalloc(maxnpts*sizeof(double));	
  distmin = (double*)shMalloc(maxnpts*sizeof(double));	
  theta = (double*)shMalloc(maxnpts*sizeof(double));	

  /* Read the data lists into arrays */
  if (((Tcl_SplitList(interp, coordlist, &l_argc, &l_argv)) == TCL_ERROR)||(l_argc!=3)){
    sprintf(errors, "Coordlist must contain three elements");
    Tcl_SetResult(interp, errors, TCL_VOLATILE);
    shFree(xt); shFree(yt); shFree(zt);
    shFree(kx); shFree(ky); shFree(kz);
    shFree(distmax); shFree(distmin); shFree(theta);
    return (TCL_ERROR);
  }
  for (axis = 0; axis<3; axis++) {
    datapoint[axis] = strtod(l_argv[axis], &ptr);
  }

  if (!strcmp(variancelist, "NULL")) {
    for (axis = 0; axis<6; axis++) {
      dataerrors[axis] = 0;
    }
  } else {
    if (((Tcl_SplitList(interp, variancelist, &l_argc, &l_argv)) == TCL_ERROR)||(l_argc!=6)){
      sprintf(errors, "Variancelist must contain six elements");
      Tcl_SetResult(interp, errors, TCL_VOLATILE);
      shFree(xt); shFree(yt); shFree(zt);
      shFree(kx); shFree(ky); shFree(kz);
      shFree(distmax); shFree(distmin); shFree(theta);
      return (TCL_ERROR);
    }
    for (axis = 0; axis<6; axis++) {
      dataerrors[axis] = strtod(l_argv[axis], &ptr);
    }
  }

  if ((currentfile = fopen(locusfilename, "r")) == NULL){
    sprintf(errors, "Can't open %s\n", locusfilename);
    Tcl_SetResult(interp, errors, TCL_VOLATILE);
    shFree(xt); shFree(yt); shFree(zt);
    shFree(kx); shFree(ky); shFree(kz);
    shFree(distmax); shFree(distmin); shFree(theta);
    return (TCL_ERROR);
  }
  for (counter = 0; counter < maxnpts; counter ++) {
    if (fscanf(currentfile, "%lg %d %lg %lg %lg %lg %lg %lg %lg %lg %lg\n", 
      &koffset, &ninbin, &(xt[counter]), &(yt[counter]), &(zt[counter]), 
      &(kx[counter]), &(ky[counter]), &(kz[counter]), &(distmax[counter]), 
      &(distmin[counter]), &(theta[counter]) ) == EOF) {
      break;
    }
  }
  fclose(currentfile);
  npts = counter;
  inlocus = atQuLocusRemove(xt, yt, zt, kx, ky, kz, distmax, distmin, theta, npts, datapoint[0], datapoint[1], datapoint[2], dataerrors[0], dataerrors[1], dataerrors[2], dataerrors[3], dataerrors[4], dataerrors[5], mink, minkdist, maxk, maxkdist, nsigma, &closestpoint, &ellipsedist);

  sprintf(result, "{inlocus %3d} {closestpoint %d} {ellipsedist %g}", inlocus, closestpoint, ellipsedist);
  Tcl_SetResult(interp, result, TCL_VOLATILE);

  shFree(xt); shFree(yt); shFree(zt);
  shFree(kx); shFree(ky); shFree(kz);
  shFree(distmax); shFree(distmin); shFree(theta);

  return TCL_OK;

}


/**************************************************************
                      tclQuLocusKlmFromXyz
 **************************************************************/
/*<AUTO EXTRACT>
  TCL VERB: quLocusKlmFromXyz

  <HTML>
  C ROUTINE CALLED:
		<A HREF=atQuasar.html#atQuLocusKlmFromXyz>atQuLocusKlmFromXyz</A> in atQuasar.c

<P>
Given a file of locus points and a single point (<x,y,z>), determine
the <k,l,m> values.

  </HTML>

</AUTO>*/

static ftclArgvInfo atQuLocusKlmFromXyzTable[] = {
  {NULL,          FTCL_ARGV_HELP, NULL, NULL,
  "Calculate x,y,z from k,l,m given a 3D locus parameterization"},
  {"<coordlist>", FTCL_ARGV_STRING, NULL, NULL, "{x y z}"},
  {"<locusfilename>", FTCL_ARGV_STRING, NULL, NULL, "locus file"},
  {"<maxnpts>", FTCL_ARGV_INT, NULL, NULL, "maximum number of locus points"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static char *atQuLocusKlmFromXyz_name = "quLocusKlmFromXyz";

static int
tclQuLocusKlmFromXyz(
                ClientData clientData,
                Tcl_Interp *interp,
                int argc,
                char **argv
                )
{
                               /* INPUT */
  char *coordlist;             /* list of ascii files of points */
  char *locusfilename;         /* list of ascii files of locus points */
  int l_argc;
  char**l_argv;			/* number and names of the list elements */
  char *ptr = NULL;
  int counter, axis;
  char errors[80], result[80];
  double *xt,*yt,*zt,*kx,*ky,*kz,*distmax,*distmin,*theta;
  int npts, maxnpts;
  int returnValue;
  FILE *currentfile;
  double datapoint[3];
  double koffset;
  int ninbin;
  double k,l,m;

/*  static char *function_name = "quLocusKlmFromXyz"; */

/* Parse the command */

  atQuLocusKlmFromXyzTable[1].dst = &coordlist;
  atQuLocusKlmFromXyzTable[2].dst = &locusfilename;
  atQuLocusKlmFromXyzTable[3].dst = &maxnpts;

  if ((returnValue = shTclParseArgv(
    interp, &argc, argv, atQuLocusKlmFromXyzTable, 0, atQuLocusKlmFromXyz_name))
                                          != FTCL_ARGV_SUCCESS) {
    return(returnValue);
  }

  xt = (double*)shMalloc(maxnpts*sizeof(double));	
  yt = (double*)shMalloc(maxnpts*sizeof(double));	
  zt = (double*)shMalloc(maxnpts*sizeof(double));	
  kx = (double*)shMalloc(maxnpts*sizeof(double));	
  ky = (double*)shMalloc(maxnpts*sizeof(double));	
  kz = (double*)shMalloc(maxnpts*sizeof(double));	
  distmax = (double*)shMalloc(maxnpts*sizeof(double));	
  distmin = (double*)shMalloc(maxnpts*sizeof(double));	
  theta = (double*)shMalloc(maxnpts*sizeof(double));	

  /* Read the data lists into arrays */
  if (((Tcl_SplitList(interp, coordlist, &l_argc, &l_argv)) == TCL_ERROR)||(l_argc!=3)){
    sprintf(errors, "Coordlist must contain three elements");
    Tcl_SetResult(interp, errors, TCL_VOLATILE);
    shFree(xt); shFree(yt); shFree(zt);
    shFree(kx); shFree(ky); shFree(kz);
    shFree(distmax); shFree(distmin); shFree(theta);
    return (TCL_ERROR);
  }
  for (axis = 0; axis<3; axis++) {
    datapoint[axis] = strtod(l_argv[axis], &ptr);
  }

  if ((currentfile = fopen(locusfilename, "r")) == NULL){
    sprintf(errors, "Can't open %s\n", locusfilename);
    Tcl_SetResult(interp, errors, TCL_VOLATILE);
    shFree(xt); shFree(yt); shFree(zt);
    shFree(kx); shFree(ky); shFree(kz);
    shFree(distmax); shFree(distmin); shFree(theta);
    return (TCL_ERROR);
  }
  for (counter = 0; counter < maxnpts; counter ++) {
    if (fscanf(currentfile, "%lg %d %lg %lg %lg %lg %lg %lg %lg %lg %lg\n", 
      &koffset, &ninbin, &(xt[counter]), &(yt[counter]), &(zt[counter]), 
      &(kx[counter]), &(ky[counter]), &(kz[counter]), &(distmax[counter]), 
      &(distmin[counter]), &(theta[counter]) ) == EOF) {
      break;
    }
  }
  fclose(currentfile);
  npts = counter;

  atQuLocusKlmFromXyz(xt, yt, zt, kx, ky, kz, theta, npts, datapoint[0], datapoint[1], datapoint[2], &k, &l, &m);

  sprintf(result, "{k %g} {l %g} {m %g}", k, l, m);
  Tcl_SetResult(interp, result, TCL_VOLATILE);

  shFree(xt); shFree(yt); shFree(zt);
  shFree(kx); shFree(ky); shFree(kz);
  shFree(distmax); shFree(distmin); shFree(theta);

  return TCL_OK;

}

/*********************************************************************/
void atTclQuasarDeclare(Tcl_Interp *interp)
{
  shTclDeclare(
               interp,
               atQuLocusCheck_name,
               (Tcl_CmdProc *)tclQuLocusCheck,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, atQuLocusCheckTable, 0,
				atQuLocusCheck_name),
               shTclGetUsage(interp,   atQuLocusCheckTable, 0,
				atQuLocusCheck_name));
  shTclDeclare(
               interp,
               atQuLocusKlmFromXyz_name,
               (Tcl_CmdProc *)tclQuLocusKlmFromXyz,
               (ClientData) 0,
               (Tcl_CmdDeleteProc *)NULL,
               module,
               shTclGetArgInfo(interp, atQuLocusKlmFromXyzTable, 0,
				atQuLocusKlmFromXyz_name),
               shTclGetUsage(interp,   atQuLocusKlmFromXyzTable, 0,
				atQuLocusKlmFromXyz_name));
}
