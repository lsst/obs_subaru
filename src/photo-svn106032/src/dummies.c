#include <stdlib.h>
#include <stdio.h>

void
phOffsetDo(const FIELDPARAMS *fiparams,	/* NOTUSED */
	   float row0,			/* NOTUSED */
	   float col0,			/* NOTUSED */
	   int band0,			/* NOTUSED */
	   int band1,			/* NOTUSED */
	   int relativeErrors,		/* NOTUSED */
	   const float *mag,		/* NOTUSED */
	   const float *magErr,		/* NOTUSED */
	   float *drow,			/* NOTUSED */
	   float *drowErr,		/* NOTUSED */
	   float *dcol,			/* NOTUSED */
	   float *dcolErr);		/* NOTUSED */
{
   fprintf(stderr,"called phOffsetDo");
   abort();
}

float phGaussdev(void) { return(0.0); }
