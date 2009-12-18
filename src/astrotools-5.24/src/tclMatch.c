/* <AUTO>
   FILE: tclMatch
   ABSTRACT:
<HTML>
   Routines for matching two lists of objects
</HTML>
   </AUTO>
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "tcl.h"
#include "shCUtils.h"
#include "shCErrStack.h"
#include "atSlalib.h"
#include "shTclHandle.h"
#include "shTclRegSupport.h"            /* needed for shTclDeclare prototype */
#include "tclMath.h"
#include "atMatch.h"
static char *module = "astrotools";    /* name of this set of code */


/****************************************************************************/
/*
 * vCloseMatch
 */
/*<AUTO EXTRACT>
  TCL VERB: vCloseMatch
  <HTML>
  C ROUTINE CALLED:
  <A HREF=atMatch.html#atVCloseMatch>atAfCloseMatch</A> in atMatch.c
<P>
Fit a linear model to relat two sets of points.
The expected  (x, y) positions are given by the vectors (vxe, vye)
with the associated errors (vxeErr, vyeErr).  The measured positions
are (vxm, vym) with errors (vxmErr, vymErr).  Points are 
considered possible matches if their x and y positions are less than xdist
and ydist, respectively.  nfit is the number of parameters fit - 4 for
solid body fits and 6 to include squash and shear.  The model returned in
the TRANS structure transforms from the measured points to the expected
points. The higher-order distortion and color terms are set to 0.
The number of point matched is returned - a number smaller than 3
is an error and the returned trans may not be correct.
</HTML>
</AUTO>
*/
static char *g_vCloseMatch = "vCloseMatch";
static ftclArgvInfo vCloseMatchArgTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Fit a linear model to relate the two sets of points.\n"},
   {"<trans>", FTCL_ARGV_STRING, NULL, NULL,
    "returned transformation"},
   {"<vxe>", FTCL_ARGV_STRING, NULL, NULL,
    "VECTOR with x positions for expected points"},
   {"<vye>", FTCL_ARGV_STRING, NULL, NULL,
    "VECTOR with y positions for expected points"},
   {"<vxm>", FTCL_ARGV_STRING, NULL, NULL,
    "VECTOR with x positions for measured points"},
   {"<vym>", FTCL_ARGV_STRING, NULL, NULL,
    "VECTOR with y positions for measured points"},
   {"<vxeErr>", FTCL_ARGV_STRING, NULL, NULL,
    "VECTOR with error in x positions for expected points"},
   {"<vyeErr>", FTCL_ARGV_STRING, NULL, NULL,
    "VECTOR with error in y positions for expected points"},
   {"<vxmErr>", FTCL_ARGV_STRING, NULL, NULL,
    "VECTOR with error in x positions for measured points"},
   {"<vymErr>", FTCL_ARGV_STRING, NULL, NULL,
    "VECTOR with error in y positions for measured points"},
   {"[xdist]", FTCL_ARGV_DOUBLE, NULL, NULL,
    "maximum x distance between expected and measured points (default=100)"},
   {"[ydist]", FTCL_ARGV_DOUBLE, NULL, NULL,
    "maximum y distance between expected and measured points (default=100)"},
   {"[nfit]", FTCL_ARGV_INT, NULL, NULL,
    "number of parameters in the fit - 4 or 6 (default=6)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};


static int
tclVCloseMatch(ClientData cd, Tcl_Interp *interp, int argc, char **argv) {

  VECTOR *vxe=NULL, *vye=NULL, *vxm=NULL, *vym=NULL;
  VECTOR *vxeErr=NULL, *vyeErr=NULL, *vxmErr=NULL, *vymErr=NULL;
  TRANS *trans=(TRANS *)0;
  int nmatch, nfit=6;
  double xdist=100, ydist=100;
  char vxeName[HANDLE_NAMELEN], *vxeNamePtr;
  char vyeName[HANDLE_NAMELEN], *vyeNamePtr;
  char vxmName[HANDLE_NAMELEN], *vxmNamePtr;
  char vymName[HANDLE_NAMELEN], *vymNamePtr;
  char vxeErrName[HANDLE_NAMELEN], *vxeErrNamePtr;
  char vyeErrName[HANDLE_NAMELEN], *vyeErrNamePtr;
  char vxmErrName[HANDLE_NAMELEN], *vxmErrNamePtr;
  char vymErrName[HANDLE_NAMELEN], *vymErrNamePtr;

  char transName[HANDLE_NAMELEN], *transNamePtr;
  char snmatch[11];

  shErrStackClear();

  vCloseMatchArgTbl[1].dst  = &transNamePtr;
  vCloseMatchArgTbl[2].dst  = &vxeNamePtr;
  vCloseMatchArgTbl[3].dst  = &vyeNamePtr;
  vCloseMatchArgTbl[4].dst  = &vxmNamePtr;
  vCloseMatchArgTbl[5].dst  = &vymNamePtr;
  vCloseMatchArgTbl[6].dst  = &vxeErrNamePtr;
  vCloseMatchArgTbl[7].dst  = &vyeErrNamePtr;
  vCloseMatchArgTbl[8].dst  = &vxmErrNamePtr;
  vCloseMatchArgTbl[9].dst  = &vymErrNamePtr;
  vCloseMatchArgTbl[10].dst = &xdist;
  vCloseMatchArgTbl[11].dst = &ydist;
  vCloseMatchArgTbl[12].dst = &nfit;

  if (shTclParseArgv(interp, &argc, argv, vCloseMatchArgTbl,
                FTCL_ARGV_NO_LEFTOVERS, g_vCloseMatch)!=FTCL_ARGV_SUCCESS) {
    return TCL_ERROR;
  }

/* make sure the input TRANS and VECTORs have valid handles */

  strncpy(transName,transNamePtr,HANDLE_NAMELEN);
  if (shTclAddrGetFromName(interp, transName, (void **) &trans, "TRANS")!=TCL_OK){
    Tcl_SetResult(interp, "tclAfCloseMatch:  bad trans name", TCL_VOLATILE);
    return TCL_ERROR;
  }

  strncpy(vxeName,vxeNamePtr,HANDLE_NAMELEN);
  if (shTclAddrGetFromName(interp, vxeName, (void **) &vxe, "VECTOR") != TCL_OK){
    Tcl_SetResult(interp, "tclVCloseMatch:  bad vxe name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  strncpy(vyeName,vyeNamePtr,HANDLE_NAMELEN);
  if (shTclAddrGetFromName(interp, vyeName, (void **) &vye, "VECTOR") != TCL_OK){
    Tcl_SetResult(interp, "tclVCloseMatch:  bad vye name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  strncpy(vxmName,vxmNamePtr,HANDLE_NAMELEN);
  if (shTclAddrGetFromName(interp, vxmName, (void **) &vxm, "VECTOR") != TCL_OK){
    Tcl_SetResult(interp, "tclVCloseMatch:  bad vxm name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  strncpy(vymName,vymNamePtr,HANDLE_NAMELEN);
  if (shTclAddrGetFromName(interp, vymName, (void **) &vym, "VECTOR") != TCL_OK){
    Tcl_SetResult(interp, "tclVCloseMatch:  bad vym name", TCL_VOLATILE);
    return TCL_ERROR;
  }

  strncpy(vxeErrName,vxeErrNamePtr,HANDLE_NAMELEN);
  if (shTclAddrGetFromName(interp, vxeErrName, (void **) &vxeErr, "VECTOR") != TCL_OK){
    Tcl_SetResult(interp, "tclVCloseMatch:  bad vxeErr name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  strncpy(vyeErrName,vyeErrNamePtr,HANDLE_NAMELEN);
  if (shTclAddrGetFromName(interp, vyeErrName, (void **) &vyeErr, "VECTOR") != TCL_OK){
    Tcl_SetResult(interp, "tclVCloseMatch:  bad vyeErr name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  strncpy(vxmErrName,vxmErrNamePtr,HANDLE_NAMELEN);
  if (shTclAddrGetFromName(interp, vxmErrName, (void **) &vxmErr, "VECTOR") != TCL_OK){
    Tcl_SetResult(interp, "tclVCloseMatch:  bad vxmErr name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  strncpy(vymErrName,vymErrNamePtr,HANDLE_NAMELEN);
  if (shTclAddrGetFromName(interp, vymErrName, (void **) &vymErr, "VECTOR") != TCL_OK){
    Tcl_SetResult(interp, "tclVCloseMatch:  bad vymErr name", TCL_VOLATILE);
    return TCL_ERROR;
  }

/* do some error checking on the inputs */

  if (nfit!=4 && nfit!=6) {
    Tcl_SetResult(interp, "tclVCloseMatch: nfit must be 4 or 6", TCL_VOLATILE);
  }

  if ( (vxm->dimen != vym->dimen) || (vxe->dimen != vye->dimen) ) {
    Tcl_SetResult(interp, "tclVCloseMatch: x and y arrays must be same size",
      TCL_VOLATILE);
    return TCL_ERROR;
  } 

  if ( (vxe->dimen != vxeErr->dimen) || (vye->dimen != vyeErr->dimen) ||
       (vxm->dimen != vxmErr->dimen) || (vym->dimen != vymErr->dimen) ) {
    Tcl_SetResult(interp, 
      "tclVCloseMatch: Error arrays must be the same size as position array",
      TCL_VOLATILE);
    return TCL_ERROR;
  } 


 /* make the actual call */

      nmatch = atVCloseMatch( vxe, vye, vxm, vym, vxeErr, vyeErr, 
                              vxmErr, vymErr, xdist, ydist, nfit, trans);

  sprintf(snmatch,"%d", nmatch);
  Tcl_SetResult(interp, snmatch, TCL_VOLATILE);

/* check to see if atVCloseMatch returned an error */

  if ( nmatch < 3 ) {
    shTclInterpAppendWithErrStack(interp);
    return TCL_ERROR;
  }

  return TCL_OK;
}

/****************************************************************************/
/*
 * VDiMatch
 */
/*<AUTO EXTRACT>
  TCL VERB: vDiMatch
  <HTML>
  C ROUTINE CALLED:
  <A HREF=atMatch.html#atVDiMatch>atVDiMatch</A> in atMatch.c
<P>
Fit a linear model to relate the two sets of
points.  (vxe,vye) are the expected positions of the points and 
(vxm,vym) are the measured positions of the points.  Points must not be
more than xSearch and ySearch apart either positively or negatively.  
Delta determines how wide a bin will be for voting on the best fit. Small
bins give more accurate fits if the points are close. nfit is the number
of parameters fit - 4 for solid body fits and 6 to include squash and 
shear.  The model returned in the TRANS structure transforms from the
measured points to the expected points.  The higher-order distortion and
color terms are set to 0.  The number of point matched 
is returned - a number smaller than 3 is an error and the trans may 
not be correct
</HTML>
</AUTO>
*/
static char *g_vDiMatch = "vDiMatch";
static ftclArgvInfo vDiMatchArgTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Fit a linear model to relate the two sets of points.\n"},
   {"<trans>", FTCL_ARGV_STRING, NULL, NULL,
    "returned transformation"},
   {"<vxe>", FTCL_ARGV_STRING, NULL, NULL,
    "VECTOR with x positions for expected points"},
   {"<vye>", FTCL_ARGV_STRING, NULL, NULL,
    "VECTOR with y positions for expected points"},
   {"<vxm>", FTCL_ARGV_STRING, NULL, NULL,
    "VECTOR with x positions for measured points"},
   {"<vym>", FTCL_ARGV_STRING, NULL, NULL,
    "VECTOR with y positions for measured points"},
   {"[xSearch]", FTCL_ARGV_DOUBLE, NULL, NULL,
    "maximum amount of variation in x distance (default=100)"},
   {"[ySearch]", FTCL_ARGV_DOUBLE, NULL, NULL,
    "maximum amount of variation in y distance (default=100)"},
   {"[delta]", FTCL_ARGV_DOUBLE, NULL, NULL,
    "width of a bin for voting on winners (default=20)"},
   {"-ne", FTCL_ARGV_INT, NULL, NULL,
    "number of expected points (default=0)"},
   {"-nm", FTCL_ARGV_INT, NULL, NULL,
    "number of measured points (default=0)"},
   {"-nfit", FTCL_ARGV_INT, NULL, NULL,
    "number of parameters in fit - must be 4 or 6."},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};


static int
tclVDiMatch(ClientData cd, Tcl_Interp *interp, int argc, char **argv) {
  VECTOR *vxe= (VECTOR *)0, *vye= (VECTOR *)0, *vxm= (VECTOR *)0, *vym= (VECTOR *)0;
  TRANS *trans = (TRANS *)0;
  int ne=0, nm=0, nfit=4, nmatch;
  double xSearch=100, ySearch=100, delta=20;
  char *vxeName;
  char *vyeName;
  char *vxmName;
  char *vymName;
  char *transName;
  char snmatch[11];

  shErrStackClear();

  vDiMatchArgTbl[1].dst = &transName;
  vDiMatchArgTbl[2].dst = &vxeName;
  vDiMatchArgTbl[3].dst = &vyeName;
  vDiMatchArgTbl[4].dst = &vxmName;
  vDiMatchArgTbl[5].dst = &vymName;
  vDiMatchArgTbl[6].dst = &xSearch;
  vDiMatchArgTbl[7].dst = &ySearch;
  vDiMatchArgTbl[8].dst = &delta;
  vDiMatchArgTbl[9].dst = &ne;
  vDiMatchArgTbl[10].dst = &nm;
  vDiMatchArgTbl[11].dst = &nfit;

  if (shTclParseArgv(interp, &argc, argv, vDiMatchArgTbl,
		FTCL_ARGV_NO_LEFTOVERS, g_vDiMatch)!=FTCL_ARGV_SUCCESS) {
    return TCL_ERROR;
  }

  if (shTclAddrGetFromName(interp, transName, (void **) &trans, "TRANS")!=TCL_OK){
    Tcl_SetResult(interp, "tclVDiMatch:  bad trans name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, vxeName, (void **) &vxe, "VECTOR") != TCL_OK
){
    Tcl_SetResult(interp, "tclVDiMatch:  bad vxe name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, vyeName, (void **) &vye, "VECTOR") != TCL_OK){
    Tcl_SetResult(interp, "tclVDiMatch:  bad vye name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, vxmName, (void **) &vxm, "VECTOR") != TCL_OK){
    Tcl_SetResult(interp, "tclVDiMatch:  bad vxm name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, vymName, (void **) &vym, "VECTOR") != TCL_OK){
    Tcl_SetResult(interp, "tclVDiMatch:  bad vym name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (nfit!=4 && nfit!=6) {
    Tcl_SetResult(interp, "tclVDiMatch: nfit must be 4 or 6", TCL_VOLATILE);
    return TCL_ERROR;
  }

  /* Check to make sure vectors are the correct size */
  if ((vxm->dimen != vym->dimen) || (vxe->dimen!=vye->dimen)) {
    Tcl_SetResult(interp, "tclVDiMatch: x and y vectors must be same size",
      TCL_VOLATILE);
    return TCL_ERROR;
  }

  /* make the actual call */
  nmatch = atVDiMatch(vxe, vye, ne, vxm, vym, nm, 
		       xSearch, ySearch, delta, nfit, trans);

  sprintf(snmatch,"%d", nmatch);
  Tcl_SetResult(interp, snmatch, TCL_VOLATILE);

  /* see if it worked */
  if (nmatch<3) {
    shTclInterpAppendWithErrStack(interp);
    return TCL_ERROR;
  }

  return TCL_OK;
}

/****************************************************************************/
/*
 * VDiMatch2
 */
/*<AUTO EXTRACT>
  TCL VERB: vDiMatch2
  <HTML>
  C ROUTINE CALLED:
  <A HREF=atMatch.html#atVDiMatch2>atVDiMatch2</A> in atMatch.c
<P>
Fit a linear model to relate the two sets of
points.  (vxe,vye) are the expected positions of the points and 
(vxm,vym) are the measured positions of the points.  Points must not be
more than xSearch and ySearch apart either positively or negatively.  
Delta determines how wide a bin will be for voting on the best fit. Small
bins give more accurate fits if the points are close. nfit is the number
of parameters fit - 4 for solid body fits and 6 to include squash and 
shear.  The model returned in the TRANS structure transforms from the
measured points to the expected points.  The higher-order distortion and
color terms are set to 0.  The number of point matched 
is returned - a number smaller than 3 is an error and the trans may 
not be correct.  Use the magnitudes as a third dimension.
</HTML>
</AUTO>
*/

static char *g_vDiMatch2 = "vDiMatch2";
static ftclArgvInfo vDiMatch2ArgTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Fit a linear model to relate the two sets of points.\n"},
   {"<trans>", FTCL_ARGV_STRING, NULL, NULL,
    "returned transformation"},
   {"<vxe>", FTCL_ARGV_STRING, NULL, NULL,
    "VECTOR with x positions for expected points"},
   {"<vye>", FTCL_ARGV_STRING, NULL, NULL,
    "VECTOR with y positions for expected points"},
   {"<vme>", FTCL_ARGV_STRING, NULL, NULL,
    "VECTOR with magnitudes for expected points"},

   {"<vxm>", FTCL_ARGV_STRING, NULL, NULL,
    "VECTOR with x positions for measured points"},
   {"<vym>", FTCL_ARGV_STRING, NULL, NULL,
    "VECTOR with y positions for measured points"},
   {"<vmm>", FTCL_ARGV_STRING, NULL, NULL,
    "VECTOR with magnitudes for measured points"},

   {"[xSearch]", FTCL_ARGV_DOUBLE, NULL, NULL,
    "maximum amount of variation in x distance (default=100)"},
   {"[ySearch]", FTCL_ARGV_DOUBLE, NULL, NULL,
    "maximum amount of variation in y distance (default=100)"},
   {"[delta]", FTCL_ARGV_DOUBLE, NULL, NULL,
    "width of a magnitude bin for voting on winners (default=20)"},
   {"[magSearch]", FTCL_ARGV_DOUBLE, NULL, NULL,
    "maximum amount of variation in magnitudes (default=20)"},
   {"[deltaMag]", FTCL_ARGV_DOUBLE, NULL, NULL,
    "width of a bin for voting on winners (default=1)"},
   {"-zeroPointIn", FTCL_ARGV_DOUBLE, NULL, NULL,
    "first guess at zero point;  expected = measured+zeroPoint (default=0)"},

   {"-ne", FTCL_ARGV_INT, NULL, NULL,
    "number of expected points (default=0)"},
   {"-nm", FTCL_ARGV_INT, NULL, NULL,
    "number of measured points (default=0)"},
   {"-nfit", FTCL_ARGV_INT, NULL, NULL,
    "number of parameters in fit - must be 4 or 6.  Default is 4"},

   {"-vmatche", FTCL_ARGV_STRING, NULL, NULL,
    "VECTOR mask set for expected points"},
   {"-vmatchm", FTCL_ARGV_STRING, NULL, NULL,
    "VECTOR mask set for measured points"},

   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};


static int
tclVDiMatch2(ClientData cd, Tcl_Interp *interp, int argc, char **argv) {
  VECTOR *vxe=NULL, *vye=NULL, *vme=NULL, *vxm=NULL, *vym=NULL, *vmm=NULL;
  VECTOR *vmatche=NULL, *vmatchm=NULL;
  TRANS *trans = NULL;
  int ne=0, nm=0, nfit=4, nmatch;
  double xSearch=100, ySearch=100, delta=20;
  double magSearch=20, deltaMag=1, zeroPointIn=0, zeroPointOut=0;
  char *vxeName, *vyeName, *vmeName, *vxmName, *vymName, *vmmName, *transName;
  char *vmatcheName=NULL, *vmatchmName=NULL;
  char snmatch[50];
  RET_CODE rcode;

  shErrStackClear();
  vDiMatch2ArgTbl[ 1].dst = &transName;
  vDiMatch2ArgTbl[ 2].dst = &vxeName;
  vDiMatch2ArgTbl[ 3].dst = &vyeName;
  vDiMatch2ArgTbl[ 4].dst = &vmeName;
  vDiMatch2ArgTbl[ 5].dst = &vxmName;
  vDiMatch2ArgTbl[ 6].dst = &vymName;
  vDiMatch2ArgTbl[ 7].dst = &vmmName;
  vDiMatch2ArgTbl[ 8].dst = &xSearch;
  vDiMatch2ArgTbl[ 9].dst = &ySearch;
  vDiMatch2ArgTbl[10].dst = &delta;
  vDiMatch2ArgTbl[11].dst = &magSearch;
  vDiMatch2ArgTbl[12].dst = &deltaMag;
  vDiMatch2ArgTbl[13].dst = &zeroPointIn;
  vDiMatch2ArgTbl[14].dst = &ne;
  vDiMatch2ArgTbl[15].dst = &nm;
  vDiMatch2ArgTbl[16].dst = &nfit;
  vDiMatch2ArgTbl[17].dst = &vmatcheName;
  vDiMatch2ArgTbl[18].dst = &vmatchmName;

  if (shTclParseArgv(interp, &argc, argv, vDiMatch2ArgTbl,
		FTCL_ARGV_NO_LEFTOVERS, g_vDiMatch2)!=FTCL_ARGV_SUCCESS) {
    return TCL_ERROR;
  }

  if (shTclAddrGetFromName(interp, transName, (void **) &trans, "TRANS")
      !=TCL_OK){
    Tcl_SetResult(interp, "tclVDiMatch2:  bad trans name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, vxeName, (void **) &vxe, "VECTOR") 
      != TCL_OK){
    Tcl_SetResult(interp, "tclVDiMatch2:  bad vxe name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, vyeName, (void **) &vye, "VECTOR") 
      != TCL_OK){
    Tcl_SetResult(interp, "tclVDiMatch2:  bad vye name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, vmeName, (void **) &vme, "VECTOR") 
      != TCL_OK){
    Tcl_SetResult(interp, "tclVDiMatch2:  bad vme name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, vxmName, (void **) &vxm, "VECTOR") 
      != TCL_OK){
    Tcl_SetResult(interp, "tclVDiMatch2:  bad vxm name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, vymName, (void **) &vym, "VECTOR") 
      != TCL_OK){
    Tcl_SetResult(interp, "tclVDiMatch2:  bad vym name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (shTclAddrGetFromName(interp, vmmName, (void **) &vmm, "VECTOR") 
      != TCL_OK){
    Tcl_SetResult(interp, "tclVDiMatch2:  bad vmm name", TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (nfit!=4 && nfit!=6) {
    Tcl_SetResult(interp, "tclVDiMatch2: nfit must be 4 or 6", TCL_VOLATILE);
    return TCL_ERROR;
  }

  if (vmatcheName != NULL) {
    if (shTclAddrGetFromName(interp, vmatcheName, (void **) &vmatche, 
			     "VECTOR") != TCL_OK) {
      Tcl_SetResult(interp, "tclVDiMatch2:  bad vmatche name", TCL_VOLATILE);
      return TCL_ERROR;
    }
  }

  if (vmatchmName != NULL) {
    if (shTclAddrGetFromName(interp, vmatchmName, (void **) &vmatchm, 
			     "VECTOR") != TCL_OK) {
      Tcl_SetResult(interp, "tclVDiMatch2:  bad vmatchm name", TCL_VOLATILE);
      return TCL_ERROR;
    }
  }

  /* Check to make sure vectors are the correct size */
  if ((vxm->dimen != vym->dimen) || (vxe->dimen!=vye->dimen)) {
    Tcl_SetResult(interp, "tclVDiMatch2: x and y vectors must be same size",
      TCL_VOLATILE);
    return TCL_ERROR;
  }

  /* make the actual call */
  rcode = atVDiMatch2(vxe, vye, vme, ne, vxm, vym, vmm, nm, 
		      xSearch, ySearch, delta, 
		      magSearch, deltaMag, zeroPointIn, 
		      nfit, trans, vmatche, vmatchm,
		      &zeroPointOut, &nmatch);

  if (rcode != SH_SUCCESS) {
    shTclInterpAppendWithErrStack(interp);
    return TCL_ERROR;
  }
  
  sprintf(snmatch,"zeroPointOut %f", zeroPointOut); 
  Tcl_AppendElement(interp, snmatch);
  sprintf(snmatch,"nMatch %d", nmatch); 
  Tcl_AppendElement(interp, snmatch);

  return TCL_OK;
}


/***************************************************************************/
/*
 * Declare tcl verbs
 */
void atTclMatchDeclare(Tcl_Interp *interp) {

   shTclDeclare(interp, g_vCloseMatch, (Tcl_CmdProc *)tclVCloseMatch,
                (ClientData) 0, (Tcl_CmdDeleteProc *)NULL,
                module,
                shTclGetArgInfo(interp, vCloseMatchArgTbl,
                        FTCL_ARGV_NO_LEFTOVERS, g_vCloseMatch),
                shTclGetUsage(interp, vCloseMatchArgTbl,
                        FTCL_ARGV_NO_LEFTOVERS, g_vCloseMatch)
                );

  shTclDeclare(interp, g_vDiMatch, (Tcl_CmdProc *)tclVDiMatch,
		(ClientData) 0, (Tcl_CmdDeleteProc *)NULL,
		module,
		shTclGetArgInfo(interp, vDiMatchArgTbl, 
			FTCL_ARGV_NO_LEFTOVERS, g_vDiMatch),
		shTclGetUsage(interp, vDiMatchArgTbl, 
			FTCL_ARGV_NO_LEFTOVERS, g_vDiMatch)
		);

  shTclDeclare(interp, g_vDiMatch2, (Tcl_CmdProc *)tclVDiMatch2,
		(ClientData) 0, (Tcl_CmdDeleteProc *)NULL,
		module,
		shTclGetArgInfo(interp, vDiMatch2ArgTbl, 
			FTCL_ARGV_NO_LEFTOVERS, g_vDiMatch2),
		shTclGetUsage(interp, vDiMatch2ArgTbl, 
			FTCL_ARGV_NO_LEFTOVERS, g_vDiMatch2)
		);
}


