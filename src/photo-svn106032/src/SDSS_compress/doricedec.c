/* Copyright (c) 1994 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* rice_decode.c		Read Rice codes from infile and construct array
 *
 * Programmer: R. White		Date: 27 July 1994
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "region.h"
#include "dervish_msg_c.h"
#include "phCompUtils.h"

static void rdquad( MYFILE *infile, int a[], int nx, int ny, int ndim);
extern void start_inputing_bits(void);
extern int  input_bit(MYFILE *infile);
extern int  input_nbits(MYFILE *infile, int n);

extern void
doricedec(MYFILE *infile, int a[], int nx, int ny)
{
int nx2, ny2;

	/*
	 * Initialize bit input
	 */
	start_inputing_bits();
	/*
	 * do the Rice decoding for each quadrant
	 */
	nx2 = (nx+1)/2;
	ny2 = (ny+1)/2;
	rdquad( infile,  a,                  nx2,      ny2, ny);
	rdquad( infile, &a[ny2],             nx2, ny - ny2, ny);
	rdquad( infile, &a[ny*nx2],     nx - nx2,      ny2, ny);
	rdquad( infile, &a[ny*nx2+ny2], nx - nx2, ny - ny2, ny);
	/*
	 * make sure there is an EOF symbol (nybble=0) at end
	 */
	if (input_nbits(infile,4) != 0) {
		fprintf(stderr, "rice_decode: bad bit plane values\n");
		exit(-1);
	}
}

/*
 * decode Rice output for this array section
 *
 * Note that right now I'm not using the ability to do coding in blocks; I pick
 * a single value for the bit-splitting parameter and use that for the entire
 * image.  Eventually will want to change this.
 */
static void
rdquad( MYFILE *infile, int a[], int nx, int ny, int ndim)
{
int i, *p, v, fs;

	fs = input_nbits(infile,4);
	for (i=0; i<nx; i++) {
		for (p = &a[i*ndim]; p < &a[i*ndim+ny]; p++) {
			/*
			 * number of leading zeros is code for top part of value
			 */
			v = 0;
			while (input_bit(infile) == 0) v++ ;
			/*
			 * bottom FS bits are written without coding
			 */
			if (fs > 0) v = (v<<fs) | input_nbits(infile, fs);
			/*
			 * map v back to positive, negative numbers
			 */
			if (v & 1) {
				/* odd values are positive */
				*p = (v+1)>>1;
			} else {
				/* even values are negative */
				*p = -(v>>1);
			}
		}
	}
}
