/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* dodecode.c	Decode stream of characters on infile and return array
 *
 * This version encodes the different quadrants separately
 *
 * Programmer: R. White		Date: 16 June 1994
 */

#include <stdio.h>
#include <stdlib.h>
#include "region.h"
#include "dervish_msg_c.h"
#include "phCompUtils.h"

#define input_nybble(infile)	input_nbits(infile,4)

extern void
qtree_decode(MYFILE *infile, int a[], int n, int nqx, int nqy, int nbitplanes);

extern void start_inputing_bits(void);
extern int input_bit(MYFILE *infile);
extern int input_nbits(MYFILE *infile, int n);

extern void
dodecode(MYFILE *infile, int a[], int nx, int ny, unsigned char nbitplanes[3])
{
int i, nel, nx2, ny2;

	nel = nx*ny;
	nx2 = (nx+1)/2;
	ny2 = (ny+1)/2;
	/*
	 * Initialize bit input
	 */
	start_inputing_bits();
	/*
	 * read bit planes for each quadrant
	 */
	qtree_decode(infile, &a[0],          ny, nx2,  ny2,  nbitplanes[0]);
	qtree_decode(infile, &a[ny2],        ny, nx2,  ny/2, nbitplanes[1]);
	qtree_decode(infile, &a[ny*nx2],     ny, nx/2, ny2,  nbitplanes[1]);
	qtree_decode(infile, &a[ny*nx2+ny2], ny, nx/2, ny/2, nbitplanes[2]);
	/*
	 * make sure there is an EOF symbol (nybble=0) at end
	 */
	if (input_nybble(infile) != 0) {
		fprintf(stderr, "dodecode: bad bit plane values\n");
		exit(-1);
	}
	/*
	 * now get the sign bits
	 * Re-initialize bit input
	 */
	start_inputing_bits();
	for (i=0; i<nel; i++) {
		if (a[i] != 0) {
			if (input_bit(infile) != 0) a[i] = -a[i];
		}
	}
}
