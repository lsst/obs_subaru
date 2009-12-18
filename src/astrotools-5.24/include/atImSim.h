#ifndef ATIMSIM_H
#define ATIMSIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dervish.h"

RET_CODE atExpDiskAdd (
		       REGION *region, 
		       double row, 
		       double col, 
		       double Izero, 
		       double rs, 
		       double theta, 
		       double axisRatio,
		       int peak,
		       int fast,
		       double *countsAdded
		       );

RET_CODE atDeVaucAdd (
		      REGION *region, 
		      double row, 
		      double col, 
		      double Izero, 
		      double rs, 
		      double theta,
		      double axisRatio,
		      int peak,
		      int fast,
		      double *countsAdded
		      );

RET_CODE atDeVaucAddSlow (
			  REGION *region, 
			  double row, 
			  double col, 
			  double counts, 
			  double rs, 
			  double maxRad,
			  double theta,
			  double axisRatio,
			  int ngrid,
			  double *countsOut
			  );

RET_CODE atExpDiskAddSlow(
			  REGION *region, /* region to add profile to */
			  double row, /* row position */
			  double col, /* column position */
			  double counts, /* total counts */
			  double rs, /* scale length */
			  double maxRad, /* maximum distance from center */
			  double theta,	/* position angle */
			  double axisRatio, /* b/a */
			  int ngrid, /* size of grid to sample pixels */
			  double *countsOut /* counts actually added */
			  );

RET_CODE atDeltaAdd (
		     REGION *region, 
		     double row, 
		     double col, 
		     double Izero, 
		     double *countsAdded
		     );
	
RET_CODE atWingAdd (
		    REGION *region, 
		    double row, 
		    double col, 
		    double Izero, 
		    double alpha, 
		    double theta,
		    double axisRatio,
		    double minR,
		    int peak,
		    int fast,
		    double *countsAdded
		    );

RET_CODE atRegStatNoiseAdd( REGION *reg);

#ifdef __cplusplus
}
#endif

#endif
