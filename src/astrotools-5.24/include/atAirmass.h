#ifndef ATAIRMASS_H
#define ATAIRMASS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dervish.h"
#include "atSlalib.h"

RET_CODE atAirmassFind (TSTAMP *time, double ra, double dec, double *airmass);

double atAirmassFromZd(
		       double degzd        /* IN: zenith dist (degrees) */
		       ); 
					      
double atMeridianAirmassFind(
			     double degdec    /*IN: DEC (degrees)*/
			     );

double atAirmassFindHaDec(
			  double hahrs,     /*IN: HA (hours)*/
			  double degdec    /*IN: DEC (degrees)*/
			  );

double atZdDec2Ha(
		  double zddeg,       /*IN: ZD (degrees)*/
		  double decdeg       /*IN: DEC (degrees)*/
		  );

#ifdef __cplusplus
}
#endif

#endif
