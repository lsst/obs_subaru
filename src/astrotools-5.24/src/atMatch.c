/* <AUTO>
   FILE: atMatch
<HTML>
   Routines which Match lists of objects
</HTML>
   </AUTO>
*/

#include <float.h>
#include <math.h>
#include <string.h>
#include "slalib.h"
#include "dervish.h"
#include "atMatch.h"

#define CUT1SIGMA 3
#define CUT2SIGMA 1
#define MATCH_FRACTION 0.3
#define MAX_ARRAY_ELEM 1000
#define delGet(x1, y1, x2, y2) (sqrt(pow((double)((x1)-(x2)),2.0)+pow((double)((y1)-(y2)),2.0)))

/******** static functions, etc. used by atVCloseMatch *********************/
struct AT_PT {
  int e;
  int m;
};


/***************************************************************************
   nextpoint
   Find the next matching pair                                             
   Set point->m to -1 to get the first m value in row point->e          
*/
static RET_CODE nextpoint(struct AT_PT *point, int **array, int npe, int npm) {

  if (point->e<0) point->e = 0;
  if (point->m<0) point->m = -1;
  if (point->e>=npe) return SH_GENERIC_ERROR;
  for (point->m += 1; point->m < npm; point->m += 1)
    if (array[point->e][point->m]>0) return SH_SUCCESS;
  for (point->e += 1; point->e < npe; point->e +=1) {
    for (point->m = 0; point->m < npm; point->m += 1)
      if (array[point->e][point->m]>0) return SH_SUCCESS;
  }
  return SH_GENERIC_ERROR;
}


/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atVCloseMatch

  DESCRIPTION:
<HTML>
   Match vectors of (x, y) positions.
   <P>
   The VECTOR inputs are made with shVectorNew - they are the expected and
   measured (x, y) values.
   xdist and ydist describe the maximum distance two matching objects are
   separated by, in whatever units the VECTORs use.
   nfit can be 4 or 6.  4 gives solid body rotation and 6 gives the full
   linear fit.  The 6 parameters returned when nfit=4 are degenerate
   The returned TRANS struct contains the size constants in a linear 
   transformation from the measured coordinate to the expected coordinates.
   The higher-order distortion and color terms are set to 0.
   <P>
   The algorithm works by making an array (roughmatches) which says which
   objects match within xdist and ydist.  All sets of three possible matched
   objects (based on the rough matches) are tried.  The match that matches
   the most objects within CUT1SIGMA*error (in each direction) is chosen 
   as the winner.  So, 
   <strong>IT IS IMPORTANT THAT THE VECTORS PASSED INTO THIS ROUTINE
   HAVE NON-ZERO ERROR.</strong>
   The new TRANS struct is the best fit to this larger
   set of matched points.
   <P>
   Finally, outlying points are rejected one at a time to improve the fit.
   The worst fitting point is found.  If it is more than CUT2SIGMA away in
   either direction, it is eliminated and a new TRANS is calculated.  This
   process is repeated until there are only three points left, or until
   the worst fit points are withing CUT2SIGMA of each other.

</HTML>

  RETURN VALUES:
<HTML>
   Returns the number of objects matched.  A negative number returned
   is an error flag:

<LISTING>
	-1 = illegal nfit
	-2 = insufficient data
	-3 = singular solution
</LISTING>

</HTML>
</AUTO>*/
int atVCloseMatch(
                   VECTOR *vxe,     /* VECTOR of expected x values */
		   VECTOR *vye,     /* VECTOR of expected y values */
		   VECTOR *vxm,	    /* VECTOR of measured x values */
		   VECTOR *vym,	    /* VECTOR of measured y values */
                   VECTOR *vxeErr,  /* VECTOR of expected x values */
		   VECTOR *vyeErr,  /* VECTOR of expected y values */
		   VECTOR *vxmErr,  /* VECTOR of measured x values */
		   VECTOR *vymErr,  /* VECTOR of measured y values */
		   double xdist,    /* x distance within which the
					two sets of points must match to
					be considered possible matches */
		   double ydist,    /* y distance within which the
					two sets of points must match to
					be considered possible matches */
		   int nfit,        /* number of parameters in fit -
					must be 4 or 6.  4 is a solid
					body fit, and 6 includes squash and
					shear */
		   TRANS *trans	    /* Returned TRANS struct */
  ) {
  int **roughmatches;
  int npe, npm, targetmatch;
  int i,j,flag,worsti;
  int nmatch, bestnmatch;
  int bestjforthisi=0;
  double dx, dy, xm, ym, xerror=0, yerror=0;
  double bestxerrorforthisi, bestyerrorforthisi;
  double dist2, maxdist2;
  double xeErrVal=0, yeErrVal=0, xmErrVal=0, ymErrVal=0, xtransm, ytransm;
  TRANS besttrans, trytrans;
  VECTOR *vxbeste=NULL, *vxbestm=NULL, *vybeste=NULL, *vybestm=NULL;
  VECTOR *vxbesteErr=NULL, *vxbestmErr=NULL, *vybesteErr=NULL, *vybestmErr=NULL;
  VECTOR *vxcopye=NULL, *vxcopym=NULL, *vycopye=NULL, *vycopym=NULL;
  VECTOR *vxcopyeErr=NULL, *vxcopymErr=NULL, *vycopyeErr=NULL, *vycopymErr=NULL;
  VECTOR *vx3e, *vx3m, *vy3e, *vy3m;
  VECTOR *vx3eErr, *vx3mErr, *vy3eErr, *vy3mErr;
  double coeff[6];

  struct AT_PT p1, p2, p3;

/* verify inputs */
  shAssert( vxe->dimen == vye->dimen );
  shAssert( vxm->dimen == vym->dimen );
  shAssert( vxe->dimen == vxeErr->dimen );
  shAssert( vye->dimen == vyeErr->dimen );
  shAssert( vxe->dimen != 0 );
  shAssert( vxm->dimen != 0 );
  shAssert( xdist > 0 );
  shAssert( ydist > 0 );
  shAssert( (nfit == 4) || (nfit == 6) );

  npe = vxe->dimen;
  npm = vxm->dimen;
  roughmatches = (int **)shMalloc(npe*sizeof(int *));
  for (i=0; i<npe; i++) {roughmatches[i] = (int *)shMalloc(npm*sizeof(int));}

  /* Find all points which match within the xdist, ydist criteria */
  for (i=0; i< npe; i++) {
    for (j=0; j<npm; j++) {
      dx = fabs( vxe->vec[i] - vxm->vec[j] );
      dy = fabs( vye->vec[i] - vym->vec[j] );

      if ((dx<xdist) && (dy<ydist)) roughmatches[i][j] = 1;
      else roughmatches[i][j] = -1;
    }
  }

  /* Find all possible sets of three points, compute the transformation,
       and save the transformation that matches the most points within
       CUT1SIGMA sigma of their positions. */

  vx3e = shVectorNew( 3 );
  vy3e = shVectorNew( 3 );
  vx3m = shVectorNew( 3 );
  vy3m = shVectorNew( 3 );
  vx3eErr = shVectorNew( 3 );
  vy3eErr = shVectorNew( 3 );
  vx3mErr = shVectorNew( 3 );
  vy3mErr = shVectorNew( 3 );

  bestnmatch = -1;
  if (npe>npm) {
    targetmatch = (int)(MATCH_FRACTION*npm);
  } else {
    targetmatch = (int)(MATCH_FRACTION*npe);
  }
  if (targetmatch<3) targetmatch=3;
  p1.e = 0; p1.m = -1;
  while ((nextpoint(&p1, roughmatches, npe, npm)==SH_SUCCESS)&&(bestnmatch<targetmatch)) {
    p2.e = p1.e+1; p2.m = -1;
    while ((nextpoint(&p2, roughmatches, npe, npm)==SH_SUCCESS)&&(bestnmatch<targetmatch)) {
      p3.e = p2.e+1; p3.m = -1;
      while ((nextpoint(&p3, roughmatches, npe, npm)==SH_SUCCESS)&&(bestnmatch<targetmatch)) {

        /* Find transformation, apply it to the measured array, and
                see how many match within CUT1SIGMA */

         vx3e->vec[0] = vxe->vec[p1.e];
         vx3e->vec[1] = vxe->vec[p2.e];
         vx3e->vec[2] = vxe->vec[p3.e];
         vx3m->vec[0] = vxm->vec[p1.m];
         vx3m->vec[1] = vxm->vec[p2.m];
         vx3m->vec[2] = vxm->vec[p3.m];
         vy3e->vec[0] = vye->vec[p1.e];
         vy3e->vec[1] = vye->vec[p2.e];
         vy3e->vec[2] = vye->vec[p3.e];
         vy3m->vec[0] = vym->vec[p1.m];
         vy3m->vec[1] = vym->vec[p2.m];
         vy3m->vec[2] = vym->vec[p3.m];

         vx3eErr->vec[0] = vxeErr->vec[p1.e];
         vx3eErr->vec[1] = vxeErr->vec[p2.e];
         vx3eErr->vec[2] = vxeErr->vec[p3.e];
         vx3mErr->vec[0] = vxmErr->vec[p1.m];
         vx3mErr->vec[1] = vxmErr->vec[p2.m];
         vx3mErr->vec[2] = vxmErr->vec[p3.m];
         vy3eErr->vec[0] = vyeErr->vec[p1.e];
         vy3eErr->vec[1] = vyeErr->vec[p2.e];
         vy3eErr->vec[2] = vyeErr->vec[p3.e];
         vy3mErr->vec[0] = vymErr->vec[p1.m];
         vy3mErr->vec[1] = vymErr->vec[p2.m];
         vy3mErr->vec[2] = vymErr->vec[p3.m];

/* atVFitxy needs a mask, here this is passed as a NULL */
         flag = atVFitxy( vx3e, vy3e, vx3m, vy3m, vx3eErr, vy3eErr, 
                          vx3mErr, vy3mErr, NULL, &trytrans, nfit);
        nmatch = 0;
        for (i=0; i<npm; i++) {

          xm = vxm->vec[i];
          ym = vym->vec[i];
	  coeff[0] = trytrans.a;
	  coeff[1] = trytrans.b;
	  coeff[2] = trytrans.c;
	  coeff[3] = trytrans.d;
	  coeff[4] = trytrans.e;
	  coeff[5] = trytrans.f;
	  slaXy2xy(xm, ym, coeff, &xtransm, &ytransm);
          bestxerrorforthisi = DBL_MAX;
          bestyerrorforthisi = DBL_MAX;
          for (j=0; j<npe; j++) {

            dx = fabs( vxe->vec[j] - xtransm );
            dy = fabs( vye->vec[j] - ytransm );
            xeErrVal = fabs( vxeErr->vec[j] );
            xmErrVal = fabs( vxmErr->vec[i] );
            yeErrVal = fabs( vyeErr->vec[j] ); 
            ymErrVal = fabs( vymErr->vec[i] );

            xerror = sqrt( xeErrVal * xeErrVal + xmErrVal * xmErrVal );
            yerror = sqrt( yeErrVal * yeErrVal + ymErrVal * ymErrVal );           

            if (dx<bestxerrorforthisi && dy<bestyerrorforthisi) {
              bestxerrorforthisi = dx;
              bestyerrorforthisi = dy;
            }
          }
          if (bestxerrorforthisi<CUT1SIGMA*xerror &&
	    bestyerrorforthisi<CUT1SIGMA*yerror) {
            nmatch++;
          }
        }
        if (nmatch>bestnmatch) {
          bestnmatch = nmatch;
          besttrans.a = trytrans.a;
          besttrans.b = trytrans.b;
          besttrans.c = trytrans.c;
          besttrans.d = trytrans.d;
          besttrans.e = trytrans.e;
          besttrans.f = trytrans.f;
        }
      }
    }
  }

  shVectorDel( vx3e );
  shVectorDel( vy3e );
  shVectorDel( vx3m );
  shVectorDel( vy3m );
  shVectorDel( vx3eErr );
  shVectorDel( vy3eErr );
  shVectorDel( vx3mErr );
  shVectorDel( vy3mErr );

  if (bestnmatch<3) {
    shErrStackPush("atVCloseMatch:   less than 3 points match after first cut");
    for (i=0; i<npe; i++) {shFree(roughmatches[i]);}
    shFree(roughmatches);
    return bestnmatch;
  }

  /* Make new VECTORs, find the transformation, then delete them */

  vxbeste = shVectorNew( (unsigned int)bestnmatch );
  vybeste = shVectorNew( (unsigned int)bestnmatch );
  vxbestm = shVectorNew( (unsigned int)bestnmatch );
  vybestm = shVectorNew( (unsigned int)bestnmatch );
  vxbesteErr = shVectorNew( (unsigned int)bestnmatch );
  vybesteErr = shVectorNew( (unsigned int)bestnmatch );
  vxbestmErr = shVectorNew( (unsigned int)bestnmatch );
  vybestmErr = shVectorNew( (unsigned int)bestnmatch );

  nmatch = 0;
  for (i=0; i<npm; i++) {
    xm = vxm->vec[i];
    ym = vym->vec[i];

    coeff[0] = besttrans.a;
    coeff[1] = besttrans.b;
    coeff[2] = besttrans.c;
    coeff[3] = besttrans.d;
    coeff[4] = besttrans.e;
    coeff[5] = besttrans.f;
    slaXy2xy(xm, ym, coeff, &xtransm, &ytransm);
    bestxerrorforthisi = DBL_MAX;
    bestyerrorforthisi = DBL_MAX;
    for (j=0; j<npe; j++) {

      dx = fabs( vxe->vec[j] - xtransm );
      dy = fabs( vye->vec[j] - ytransm );
      xeErrVal = fabs( vxe->vec[j] );
      xmErrVal = fabs( vxm->vec[i] );
      yeErrVal = fabs( vye->vec[j] );
      ymErrVal = fabs( vym->vec[i] );
      xerror = sqrt( xeErrVal * xeErrVal + xmErrVal * xmErrVal );
      yerror = sqrt( yeErrVal * yeErrVal + ymErrVal * ymErrVal );

      if (dx<bestxerrorforthisi && dy<bestyerrorforthisi) {
        bestxerrorforthisi = dx;
        bestyerrorforthisi = dy;
        bestjforthisi = j;
      }
    }
    if (bestxerrorforthisi<CUT1SIGMA*xerror &&
      bestyerrorforthisi<CUT1SIGMA*yerror) {

      vxbestm->vec[nmatch] = xm;
      vybestm->vec[nmatch] = ym;
      vxbeste->vec[nmatch] = vxe->vec[ bestjforthisi ];
      vybeste->vec[nmatch] = vye->vec[ bestjforthisi ];
      vxbestmErr->vec[nmatch] = xmErrVal;
      vybestmErr->vec[nmatch] = ymErrVal;
      vxbesteErr->vec[nmatch] = xeErrVal;
      vybesteErr->vec[nmatch] = yeErrVal;

      nmatch++;
    }
  }

  flag = atVFitxy( vxbeste, vybeste, vxbestm, vybestm, vxbesteErr, vybesteErr,
                   vxbestmErr, vybestmErr, NULL, &besttrans, nfit );


  /* Throw out the worst fit point, and if it is worse than CUT2SIGMA,
        recalulate the trans */

  while (bestnmatch>3) {

/* create temporary VECTORs of size bestmatch, after 
   first destroying any from a previous iteration */

    if ( vxcopye != NULL ) { shVectorDel( vxcopye ); }
    if ( vycopye != NULL ) { shVectorDel( vycopye ); }
    if ( vxcopym != NULL ) { shVectorDel( vxcopym ); }
    if ( vycopym != NULL ) { shVectorDel( vycopym ); }
    if ( vxcopyeErr != NULL ) { shVectorDel( vxcopyeErr ); }
    if ( vycopyeErr != NULL ) { shVectorDel( vycopyeErr ); }
    if ( vxcopymErr != NULL ) { shVectorDel( vxcopymErr ); }
    if ( vycopymErr != NULL ) { shVectorDel( vycopymErr ); }

    vxcopye = shVectorNew( (unsigned int)bestnmatch );
    vycopye = shVectorNew( (unsigned int)bestnmatch );
    vxcopym = shVectorNew( (unsigned int)bestnmatch );
    vycopym = shVectorNew( (unsigned int)bestnmatch );
    vxcopyeErr = shVectorNew( (unsigned int)bestnmatch );
    vycopyeErr = shVectorNew( (unsigned int)bestnmatch );
    vxcopymErr = shVectorNew( (unsigned int)bestnmatch );
    vycopymErr = shVectorNew( (unsigned int)bestnmatch );

    worsti = -1;
    maxdist2 = 0;
    for (i=0;i<bestnmatch;i++) {
      xm = vxbestm->vec[i];
      ym = vybestm->vec[i];

      coeff[0] = besttrans.a;
      coeff[1] = besttrans.b;
      coeff[2] = besttrans.c;
      coeff[3] = besttrans.d;
      coeff[4] = besttrans.e;
      coeff[5] = besttrans.f;
      slaXy2xy(xm, ym, coeff, &xtransm, &ytransm);

      dx = fabs( vxbeste->vec[i] - xtransm );
      dy = fabs( vybeste->vec[i] - ytransm );

      xeErrVal = fabs( vxbesteErr->vec[i] );
      xmErrVal = fabs( vxbestmErr->vec[i] );
      yeErrVal = fabs( vybesteErr->vec[i] );
      ymErrVal = fabs( vybestmErr->vec[i] );

      vxcopye->vec[i] = vxbeste->vec[i];
      vxcopym->vec[i] = vxbestm->vec[i];
      vycopye->vec[i] = vybeste->vec[i];
      vycopym->vec[i] = vybestm->vec[i];
      vxcopyeErr->vec[i] = xeErrVal; 
      vxcopymErr->vec[i] = xmErrVal;
      vycopyeErr->vec[i] = yeErrVal;
      vycopymErr->vec[i] = ymErrVal;
      
      xerror = sqrt( xeErrVal * xeErrVal + xmErrVal * xmErrVal );
      yerror = sqrt( yeErrVal * yeErrVal + ymErrVal * ymErrVal );

      if (dx>CUT2SIGMA*xerror || dy>CUT2SIGMA*yerror) {
        dist2 = dx*dx + dy*dy;
        if (dist2>maxdist2) {
          maxdist2 = dist2;
          worsti = i;
        }
      }
    }
    if (worsti==-1) break;
    bestnmatch--;

/* create temporary VECTORs of size bestmatch, after 
   first destroying any from a previous iteration */

    if ( vxbeste != NULL ) { shVectorDel( vxbeste ); }
    if ( vybeste != NULL ) { shVectorDel( vybeste ); }
    if ( vxbestm != NULL ) { shVectorDel( vxbestm ); }
    if ( vybestm != NULL ) { shVectorDel( vybestm ); }
    if ( vxbesteErr != NULL ) { shVectorDel( vxbesteErr ); }
    if ( vybesteErr != NULL ) { shVectorDel( vybesteErr ); }
    if ( vxbestmErr != NULL ) { shVectorDel( vxbestmErr ); }
    if ( vybestmErr != NULL ) { shVectorDel( vybestmErr ); }

    vxbeste = shVectorNew( (unsigned int)bestnmatch );
    vybeste = shVectorNew( (unsigned int)bestnmatch );
    vxbestm = shVectorNew( (unsigned int)bestnmatch );
    vybestm = shVectorNew( (unsigned int)bestnmatch );
    vxbesteErr = shVectorNew( (unsigned int)bestnmatch );
    vybesteErr = shVectorNew( (unsigned int)bestnmatch );
    vxbestmErr = shVectorNew( (unsigned int)bestnmatch );
    vybestmErr = shVectorNew( (unsigned int)bestnmatch );

    j=0;
    for (i=0;i<=bestnmatch;i++) {
      if (i!=worsti) {

        vxbeste->vec[j] = vxcopye->vec[i];        
        vybeste->vec[j] = vycopye->vec[i];
        vxbestm->vec[j] = vxcopym->vec[i];
        vybestm->vec[j] = vycopym->vec[i];

        vxbesteErr->vec[j] = vxcopyeErr->vec[i];
        vybesteErr->vec[j] = vxcopyeErr->vec[i];
        vxbestmErr->vec[j] = vxcopymErr->vec[i];
        vxbestmErr->vec[j] = vycopymErr->vec[i];

        j++;
      }
    }
    flag = atVFitxy( vxbeste, vybeste, vxbestm, vybestm, vxbesteErr, vybesteErr,
                     vxbestm, vybestm, NULL, &besttrans, nfit );
  }

  shVectorDel( vxbeste );
  shVectorDel( vybeste );
  shVectorDel( vxbestm );
  shVectorDel( vybestm );
  shVectorDel( vxcopye );
  shVectorDel( vycopye );
  shVectorDel( vxcopym );
  shVectorDel( vycopym );

  shVectorDel( vxbesteErr );
  shVectorDel( vybesteErr );
  shVectorDel( vxbestmErr );
  shVectorDel( vybestmErr );
  shVectorDel( vxcopyeErr );
  shVectorDel( vycopyeErr );
  shVectorDel( vxcopymErr );
  shVectorDel( vycopymErr );

  trans->a = besttrans.a;
  trans->b = besttrans.b;
  trans->c = besttrans.c;
  trans->d = besttrans.d;
  trans->e = besttrans.e;
  trans->f = besttrans.f;
  trans->dRow0 = 0;
  trans->dRow1 = 0;
  trans->dRow2 = 0;
  trans->dRow3 = 0;
  trans->dCol0 = 0;
  trans->dCol1 = 0;
  trans->dCol2 = 0;
  trans->dCol3 = 0;
  trans->csRow = 0;
  trans->csCol = 0;
  trans->ccRow = 0;
  trans->ccCol = 0;
  trans->riCut = 0;

/* return the number of matches or an error code */
  if (flag<0) {
    for (i=0; i<npe; i++) {shFree(roughmatches[i]);}
    shFree(roughmatches);
    return flag;
    }
  for (i=0; i<npe; i++) {shFree(roughmatches[i]);}
  shFree(roughmatches);
  return (bestnmatch);

}


/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atVDiMatch

  DESCRIPTION:
<HTML>
	Matches two sets of objects.  The matching is done by looking 
	for similar differences in length, so it is dependent on scale
	but not on offset of the objects.
</HTML>

  RETURN VALUES:
<HTML>
        Returns the linear transformation to be applied to the measured
	positions to produce the expected positions.
	The higher-order distortion and color terms are set to 0.
        A value less than 2 is an error.

</HTML>
</AUTO>*/
int atVDiMatch(VECTOR *vxe,	/* VECTOR of expected x values */
		VECTOR *vye,	/* VECTOR of expected y values */
		int ne,         /* use first ne expected points -
				   if set to 0 use all points */
		VECTOR *vxm,   	/* VECTOR of measured x values */
		VECTOR *vym,   	/* VECTOR of measured y values */
		int nm, 	/* use first nm measured points -
				   if set to 0 use all points */
		double xSearch,	/* x distance within which the
				   two sets of points must match to
				   be considered possible matches */
		double ySearch,	/* y distance within which the
				   two sets of points must match to
				   be considered possible matches */
		double delta,   /* width of each bin for voting */
		int nfit,       /* number of parameters in fit -
				   must be 4 or 6.  4 is a solid
				   body fit, and 6 includes squash and
				   shear */
		TRANS *trans	/* Returned TRANS struct */
		
  ) {
  int ivote, jvote;
  int **vote;
  int nBinX, nBinY, nGoodMatch, bestj;
  float xmin, ymin, xv, yv, bestDelta, thisDelta;
  int *eMatch, *mMatch;
  VECTOR *vxt=NULL, *vyt=NULL, *vxe2=NULL, *vye2=NULL, *vxm2=NULL, *vym2=NULL;

  int neLocal, nmLocal;
  int i,j,bestx=0,besty=0,bestv=0;
  float delX, delY;

#ifdef DIDEBUG
  float xmax, ymax
#endif

  /* See if there is enough input to do anything useful */
  if ( (vxe==NULL) || (vye==NULL) || (vxm==NULL) || (vym==NULL) ) {
    shErrStackPush("atVDiMatch:  must have four non-null VECTORs input");
    return 0;
  }
  if (ne==0) {neLocal=vxe->dimen;} else {neLocal=ne;}
  if (nm==0) {nmLocal=vxm->dimen;} else {nmLocal=nm;}
  if ( (vxe->dimen != vye->dimen) || (vxe->dimen < neLocal) ) {
    shErrStackPush("atVDiMatch:  bad number of values in expected VECTORs");
    return 0;
  }
  if ( (vxm->dimen != vym->dimen) || (vxm->dimen < nmLocal) ) {
    shErrStackPush("atVDiMatch:  bad number of values in measured VECTORs");
    return 0;
  }

  /* Allocate and stuff vectors */
  nBinX = ceil(xSearch/delta)*2;
  nBinY = ceil(ySearch/delta)*2;
  xmin = -(xSearch);
  ymin = -(ySearch);

#ifdef DIDEBUG
  xmax = xSearch;
  ymax = ySearch;
  printf("nBinX, nBinY, xmin, xmax, ymin, ymax: %d %d %f %f %f %f\n", 
	 nBinX, nBinY, xmin, xmax, ymin, ymax);
#endif

  vote = (int **)shMalloc(nBinY*sizeof(int *));  
  for(i=0; i<nBinY; i++) {
    vote[i]=(int *)shMalloc(nBinX*sizeof(int*));
  }
  
  for (i=0; i<nBinY; i++) {
    for (j=0; j<nBinX; j++) {
      vote[i][j]=0;
    }
  }
  
  vxt = shVectorNew(vxm->dimen);
  memcpy((char *) vxt->vec, 
	 (char *) vxm->vec, 
	 vxt->dimen*sizeof(VECTOR_TYPE));

  vyt = shVectorNew(vym->dimen);
  memcpy((char *) vyt->vec, 
	 (char *) vym->vec, 
	 vyt->dimen*sizeof(VECTOR_TYPE));

  atVTrans(trans,vxt,NULL,vyt,NULL);

  
#ifdef DIDEBUG
  printf("after applying first guess:\n");
  printf("vxe and vye\n");
  for (i=0; i<vxe->dimen; i++) {
    printf("i xe ye %3d %6.2f %6.2f\n",
  	   i, vxe->vec[i], vye->vec[i]);
  }
  printf("vxt and vyt\n");
  for (i=0; i<vxt->dimen; i++) {
    printf("i xt yt %3d %6.2f %6.2f\n",
  	   i, vxt->vec[i], vyt->vec[i]);
  }
#endif

  for (i=0; i<neLocal; i++) {
    for (j=0; j<nmLocal; j++) {
      xv = vxe->vec[i]-vxt->vec[j];
      yv = vye->vec[i]-vyt->vec[j];
      ivote = (int) floor((yv-ymin)/delta);
      if  ( (ivote >= 0) && (ivote<nBinY) ) {
	jvote = (int) floor((xv-xmin)/delta);
	if  ( (jvote >= 0) && (jvote<nBinX) ) {
	  vote[ivote][jvote]++;
#ifdef DIDEBUG
	  printf("i=%d j=%d ivote=%d jvote=%d vote=%d\n",
		 i, j, ivote, jvote, vote[ivote][jvote]);
#endif
	}
      }
    }
  }
  for (i=0; i<nBinY; i++) {
    for (j=0; j<nBinX; j++) {
      if(bestv < vote[i][j]) {
	bestv = vote[i][j];
	besty = i;
	bestx = j;
      }
    }
  }
#ifdef DIDEBUG
  printf("after vote:  bestv besty bestx %d %d %d\n",bestv, besty, bestx);
#endif
  /*
  ** find out how much to shift the trans
  ** find the location starting from min, 
  ** then subtract max to account for negatives 
  ** then then subtract 1/2 delta to get to middle of bin
  */
  delX = xmin + (bestx+0.5)*delta;
  delY = ymin + (besty+0.5)*delta;
#ifdef DIDEBUG
  printf("delX Y are %f %f \n",delX, delY);
#endif
  for (i=0; i<nmLocal; i++) {
    vxt->vec[i] = delX+vxt->vec[i];
    vyt->vec[i] = delY+vyt->vec[i];
  }
  
  
#ifdef DIDEBUG
  printf("after shift:\n");
  printf("vxe and vye\n");
  for (i=0; i<vxe->dimen; i++) {
    printf("i xe ye %3d %6.2f %6.2f\n",
  	   i, vxe->vec[i], vye->vec[i]);
  }
  printf("vxt and vyt\n");
  for (i=0; i<vxt->dimen; i++) {
    printf("i xt yt %3d %6.2f %6.2f\n",
  	   i, vxt->vec[i], vyt->vec[i]);
  }
#endif
  
  nGoodMatch = 0;
  if (nmLocal > neLocal) {
    eMatch = (int *) shMalloc(nmLocal*sizeof(int));
    mMatch = (int *) shMalloc(nmLocal*sizeof(int));
  } else {
    eMatch = (int *) shMalloc(neLocal*sizeof(int));
    mMatch = (int *) shMalloc(neLocal*sizeof(int));
  }


  for (i=0; i<neLocal; i++) {
    bestDelta = delGet(vxe->vec[i],vye->vec[i], 
		       vxt->vec[0],vyt->vec[0]);
    bestj =0;
    for (j=1; j<nmLocal; j++) {
      thisDelta = delGet(vxe->vec[i],vye->vec[i], 
			 vxt->vec[j],vyt->vec[j]);
      if (thisDelta < bestDelta) {
	bestDelta = thisDelta;
	bestj = j;
      }
    }
    if (bestDelta < delta) {
      eMatch[nGoodMatch] = i;
      mMatch[nGoodMatch] = bestj;
      nGoodMatch++;
    }
  }

  if ((nGoodMatch<3) && (nfit==6)) {
      shErrStackPush("atVDiMatch:  not enough matches to try fit at nfit=6");
      return nGoodMatch; 
  } 
  if ((nGoodMatch<2) && (nfit==4)) {
      shErrStackPush("atVDiMatch:  not enough matches to try fit");
      return nGoodMatch;
  } 
  vxe2 = shVectorNew(nGoodMatch);
  vye2 = shVectorNew(nGoodMatch);
  vxm2 = shVectorNew(nGoodMatch);
  vym2 = shVectorNew(nGoodMatch);
  for (i=0; i<nGoodMatch; i++) {
     vxe2->vec[i] = vxe->vec[eMatch[i]];
     vye2->vec[i] = vye->vec[eMatch[i]];
     vxm2->vec[i] = vxm->vec[mMatch[i]];
     vym2->vec[i] = vym->vec[mMatch[i]];
#ifdef DIDEBUG
     printf("i=%d eMatch=%d mMatch=%d\n", i, eMatch[i], mMatch[i]);
#endif
  }
  atVFitxy(vxe2, vye2, vxm2, vym2,
	   NULL, NULL, NULL, NULL, NULL,
	   trans, nfit);
  trans->dRow0 = 0;
  trans->dRow1 = 0;
  trans->dRow2 = 0;
  trans->dRow3 = 0;
  trans->dCol0 = 0;
  trans->dCol1 = 0;
  trans->dCol2 = 0;
  trans->dCol3 = 0;
  trans->csRow = 0;
  trans->csCol = 0;
  trans->ccRow = 0;
  trans->ccCol = 0;
  trans->riCut = 0;
  /* clean up the temporary vectors */
  for(i=0; i<nBinY; i++) {
    shFree(vote[i]);
  }
  shFree(vote);  
  shVectorDel(vxt);
  shVectorDel(vyt);
  shFree(eMatch);
  shFree(mMatch);

 return nGoodMatch;
}

/***************************************************************************/
/*<AUTO EXTRACT>

  ROUTINE: atVDiMatch2

  DESCRIPTION:
<HTML>
	Matches two sets of objects.  The matching is done by looking 
	for similar differences in length, so it is dependent on scale
	but not on offset of the objects.  Use the magnitude as a third
	dimension.
</HTML>

  RETURN VALUES:
<HTML>
        Returns the linear transformation to be applied to the measured
	positions to produce the expected positions.
	The higher-order distortion and color terms are set to 0.
        A value less than 2 is an error.

</HTML>
</AUTO>*/
RET_CODE atVDiMatch2(
		     VECTOR *vxe,	/* VECTOR of expected x values */
		     VECTOR *vye,	/* VECTOR of expected y values */
		     VECTOR *vme,	/* VECTOR of expected magnitudes */
		     int ne,         /* use first ne expected points -
					if set to 0 use all points */
		     VECTOR *vxm,   	/* VECTOR of measured x values */
		     VECTOR *vym,   	/* VECTOR of measured y values */
		     VECTOR *vmm,	/* VECTOR of measured magnitudes */
		     int nm, 	/* use first nm measured points -
				   if set to 0 use all points */
		     double xSearch,	/* x distance within which the
					   two sets of points must match to
					   be considered possible matches */
		     double ySearch,	/* y distance within which the
					   two sets of points must match to
					   be considered possible matches */
		     double delta,   /* width of each bin for voting */
		     double magSearch,
		     double deltaMag,
		     double zeroPointIn,
		     int nfit,       /* number of parameters in fit -
					must be 4 or 6.  4 is a solid
					body fit, and 6 includes squash and
					shear */
		     
		     TRANS *trans,	/* Returned TRANS struct */
		     VECTOR *vmatche,    /* filled in with matches used for
					   fit if it is not NULL */
		     VECTOR *vmatchm,    /* filled in with matches used for
					   fit if it is not NULL */
		     double *zeroPointOut,
		     int *nMatch
		     ) {
  int xvote, yvote, mvote;
  int ***vote;
  int nBinX, nBinY, nBinM, nGoodMatch, bestj;
  float xmin, ymin, mmin, xv, yv, mv, bestDelta, thisDelta;
  double zpSum=0;
  int *eMatch, *mMatch;
  VECTOR *vxt=NULL, *vyt=NULL, *vxe2=NULL, *vye2=NULL, *vxm2=NULL, *vym2=NULL;

  int neLocal, nmLocal;
  int i,j,k, bestx=0,besty=0,bestv=0;
  float delX, delY;

#ifdef DIDEBUG
  float xmax, ymax
#endif

  /* See if there is enough input to do anything useful */
  if ( (vxe==NULL) || (vye==NULL) || (vxm==NULL) || (vym==NULL) ) {
    shErrStackPush("atVDiMatch:  must have four non-null VECTORs input");
    return SH_GENERIC_ERROR;
  }
  if (ne==0) {neLocal=vxe->dimen;} else {neLocal=ne;}
  if (nm==0) {nmLocal=vxm->dimen;} else {nmLocal=nm;}
  if ( (vxe->dimen != vye->dimen) || (vxe->dimen < neLocal) ) {
    shErrStackPush("atVDiMatch:  bad number of values in expected VECTORs");
    return SH_GENERIC_ERROR;
  }
  if ( (vxm->dimen != vym->dimen) || (vxm->dimen < nmLocal) ) {
    shErrStackPush("atVDiMatch:  bad number of values in measured VECTORs");
    return SH_GENERIC_ERROR;
  }

  if ( (vmatchm != NULL) ) {
    if (vmatchm->dimen != vym->dimen) {
      shErrStackPush("atVDiMatch:  bad number of values in vmatchm");
    }
  }
  if ( (vmatche != NULL) ) {
    if (vmatche->dimen != vye->dimen) {
      shErrStackPush("atVDiMatch:  bad number of values in vmatche");
    }
  }
  /* Allocate and stuff vectors */
  nBinX = ceil(xSearch/delta)*2;
  nBinY = ceil(ySearch/delta)*2;
  nBinM = ceil(magSearch/deltaMag)*2;
  xmin = -(xSearch);
  ymin = -(ySearch);
  mmin = -magSearch;

#ifdef DIDEBUG
  xmax = xSearch;
  ymax = ySearch;
  printf("nBinX, nBinY, xmin, xmax, ymin, ymax: %d %d %f %f %f %f\n", 
	 nBinX, nBinY, xmin, xmax, ymin, ymax);
#endif

  vote = (int ***)shMalloc(nBinM*sizeof(int ));
  for (k=0; k<nBinM; k++) {
    vote[k] = (int **)shMalloc(nBinY*sizeof(int *));  
    for(j=0; j<nBinY; j++) {
      vote[k][j]=(int *)shMalloc(nBinX*sizeof(int*));
    }
  }
  
  for (i=0; i<nBinX; i++) {
    for (j=0; j<nBinY; j++) {
      for (k=0; k<nBinM; k++) {
	vote[k][j][i]=0;
      }
    }
  }
  
  vxt = shVectorNew(vxm->dimen);
  memcpy((char *) vxt->vec, 
	 (char *) vxm->vec, 
	 vxt->dimen*sizeof(VECTOR_TYPE));

  vyt = shVectorNew(vym->dimen);
  memcpy((char *) vyt->vec, 
	 (char *) vym->vec, 
	 vyt->dimen*sizeof(VECTOR_TYPE));

  atVTrans(trans,vxt,NULL,vyt,NULL);

  
#ifdef DIDEBUG
  printf("after applying first guess:\n");
  printf("vxe and vye\n");
  for (i=0; i<vxe->dimen; i++) {
    printf("i xe ye %3d %6.2f %6.2f\n",
  	   i, vxe->vec[i], vye->vec[i]);
  }
  printf("vxt and vyt\n");
  for (i=0; i<vxt->dimen; i++) {
    printf("i xt yt %3d %6.2f %6.2f\n",
  	   i, vxt->vec[i], vyt->vec[i]);
  }
#endif

  for (i=0; i<neLocal; i++) {
    for (j=0; j<nmLocal; j++) {
      yv = vye->vec[i]-vyt->vec[j];
      yvote = (int) floor((yv-ymin)/delta);
      if  ( (yvote >= 0) && (yvote<nBinY) ) {
	xv = vxe->vec[i]-vxt->vec[j];
	xvote = (int) floor((xv-xmin)/delta);
	if  ( (xvote >= 0) && (xvote<nBinX) ) {
	  mv = vme->vec[i]-vmm->vec[j]-zeroPointIn;
	  mvote = (int) floor((mv-mmin)/deltaMag);
	  if  ( (mvote >= 0) && (mvote<nBinM) ) {
	    vote[mvote][yvote][xvote]++;
	  }
	}
      }
    }
  }
  for (k=0; k<nBinM; k++) {
    for (j=0; j<nBinY; j++) {
      for (i=0; i<nBinX; i++) {
	if(bestv < vote[k][j][i]) {
	  bestv = vote[k][j][i];
/*        bestm = k;          removed as this is never referenced */
	  besty = j;
	  bestx = i;
	}
      }
    }
  }
#ifdef DIDEBUG
  printf("after vote:  bestv besty bestx %d %d %d\n",bestv, besty, bestx);
#endif
  /*
  ** find out how much to shift the trans
  ** find the location starting from min, 
  ** then subtract max to account for negatives 
  ** then then subtract 1/2 delta to get to middle of bin
  */
  delX = xmin + (bestx+0.5)*delta;
  delY = ymin + (besty+0.5)*delta;
#ifdef DIDEBUG
  printf("delX Y are %f %f \n",delX, delY);
#endif
  for (i=0; i<nmLocal; i++) {
    vxt->vec[i] = delX+vxt->vec[i];
    vyt->vec[i] = delY+vyt->vec[i];
  }
  
  
#ifdef DIDEBUG
  printf("after shift:\n");
  printf("vxe and vye\n");
  for (i=0; i<vxe->dimen; i++) {
    printf("i xe ye %3d %6.2f %6.2f\n",
  	   i, vxe->vec[i], vye->vec[i]);
  }
  printf("vxt and vyt\n");
  for (i=0; i<vxt->dimen; i++) {
    printf("i xt yt %3d %6.2f %6.2f\n",
  	   i, vxt->vec[i], vyt->vec[i]);
  }
#endif

  if (vmatche != NULL) {
    for (i=0; i<vmatche->dimen; i++) vmatche->vec[i] = -1;
  }

  if (vmatchm != NULL) {
    for (j=0; j<vmatchm->dimen; j++) vmatchm->vec[j] = -1;
  }

  nGoodMatch = 0;
  if (nmLocal > neLocal) {
    eMatch = (int *) shMalloc(nmLocal*sizeof(int));
    mMatch = (int *) shMalloc(nmLocal*sizeof(int));
  } else {
    eMatch = (int *) shMalloc(neLocal*sizeof(int));
    mMatch = (int *) shMalloc(neLocal*sizeof(int));
  }


  for (i=0; i<neLocal; i++) {
    bestDelta = delGet(vxe->vec[i],vye->vec[i], 
		       vxt->vec[0],vyt->vec[0]);
    bestj =0;
    for (j=1; j<nmLocal; j++) {
      thisDelta = delGet(vxe->vec[i],vye->vec[i], 
			 vxt->vec[j],vyt->vec[j]);
      if (thisDelta < bestDelta) {
	bestDelta = thisDelta;
	bestj = j;
      }
    }
    if (bestDelta < delta) {
      eMatch[nGoodMatch] = i;
      mMatch[nGoodMatch] = bestj;
      nGoodMatch++;
      if (vmatche != NULL) {
	vmatche->vec[i] = bestj;
      }
      if (vmatchm != NULL) {
	vmatchm->vec[bestj] = i;
      }
    }
  }

    if ((nGoodMatch<3) && (nfit==6)) {
      shErrStackPush("atVDiMatch:  not enough matches to try fit at nfit=6");
      *nMatch = nGoodMatch;
      return SH_GENERIC_ERROR;
  } 
  if ((nGoodMatch<2) && (nfit==4)) {
      shErrStackPush("atVDiMatch:  not enough matches to try fit");
      *nMatch = nGoodMatch;
      return SH_GENERIC_ERROR;
    } 
  vxe2 = shVectorNew(nGoodMatch);
  vye2 = shVectorNew(nGoodMatch);
  vxm2 = shVectorNew(nGoodMatch);
  vym2 = shVectorNew(nGoodMatch);
  
  for (i=0; i<nGoodMatch; i++) {
     vxe2->vec[i] = vxe->vec[eMatch[i]];
     vye2->vec[i] = vye->vec[eMatch[i]];
     vxm2->vec[i] = vxm->vec[mMatch[i]];
     vym2->vec[i] = vym->vec[mMatch[i]];
     zpSum += vme->vec[eMatch[i]] - vmm->vec[mMatch[i]];
   }
  *zeroPointOut = zpSum/nGoodMatch;

  atVFitxy(vxe2, vye2, vxm2, vym2,
	   NULL, NULL, NULL, NULL, NULL,
	   trans, nfit);
  trans->dRow0 = 0;
  trans->dRow1 = 0;
  trans->dRow2 = 0;
  trans->dRow3 = 0;
  trans->dCol0 = 0;
  trans->dCol1 = 0;
  trans->dCol2 = 0;
  trans->dCol3 = 0;
  trans->csRow = 0;
  trans->csCol = 0;
  trans->ccRow = 0;
  trans->ccCol = 0;
  trans->riCut = 0;

  /* clean up the temporary vectors */
    for (k=0; k<nBinM; k++) {
      for(j=0; j<nBinY; j++) {
	shFree(vote[k][j]);
      }
      shFree(vote[k]);
    }
    
    shFree(vote);  
    shVectorDel(vxt);
    shVectorDel(vyt);
    shFree(eMatch);
    shFree(mMatch);
    *nMatch = nGoodMatch;
    return SH_SUCCESS;
}




