#if !defined(PHEXTINCTION_H)
#define PHEXTINCTION_H
#include "phMag.h"

/* This struct contains a single determination of the extinction in all colors
 */

typedef struct {
	double mjd;
	int ncolors;
	MAG **k;		/* coefficients are of type
						MAG; there is one for every
						color, stored as an array */
} EXTINCTION; /* pragma SCHEMA */

EXTINCTION * phExtinctionNew(int ncolors);
void phExtinctionDel(EXTINCTION *extinction);

#endif
