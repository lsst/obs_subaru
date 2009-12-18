#ifndef ATMATCH_H
#define ATMATCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dervish.h"
#include "atSlalib.h"

#define SM_MAX_LINFIT_VAR 100

int atVCloseMatch(
                   VECTOR *vxe, 
                   VECTOR *vye, 
                   VECTOR *vxm, 
                   VECTOR *vym, 
                   VECTOR *vxeErr, 
                   VECTOR *vyeErr, 
                   VECTOR *vxmErr, 
                   VECTOR *vymErr, 
                   double xdist, 
                   double ydist, 
                   int nfit, 
                   TRANS *trans);

int atVDiMatch(
                VECTOR *vxe,	/* VECTOR of expected x values */
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
	     );

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
		     );
#ifdef __cplusplus
}
#endif

#endif






