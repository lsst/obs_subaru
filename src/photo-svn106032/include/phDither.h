/* Function prototypes in img module */

#ifndef PHDITHER_H
#define PHDITHER_H

typedef unsigned short UDATA;

void  phSqrt (UDATA **rows, int nr, int nc, double rnoise, double gain,
	double quant);

void phSqrDither (UDATA **rows, int nr, int nc, double rnoise, double gain,
	double quant);

void phSim (UDATA **rows, int nr, int nc, double rnoise, double gain,
	double sky);
#endif
