/* Copyright (c) 1994 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* doqtreedec.c		read qtree codes from infile and construct array
 *
 * Programmer: R. White		Date: 27 July 1994
 */
#include <stdio.h>
#include "region.h"
#include "dervish_msg_c.h"
#include "phCompUtils.h"

extern int qread (MYFILE *infile, char *a, int n);
extern void dodecode(MYFILE *infile, int a[], int nx, int ny, unsigned char nbitplanes[3]);


extern void
doqtreedec(MYFILE *infile, int a[], int nx, int ny)
{
unsigned char nbitplanes[3];

	/* get # bits in quadrants	*/
	qread(infile, (char *) nbitplanes, sizeof(nbitplanes));
	/* do the decoding */
	dodecode(infile, a, nx, ny, nbitplanes);
}
