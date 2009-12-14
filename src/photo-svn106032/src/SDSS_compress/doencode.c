/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* doencode.c	Encode 2-D array and write stream of characters on outfile
 *
 * This version assumes that A is positive.
 *
 * Programmer: R. White		Date: 7 May 1991
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "ftcl.h"
#include "dervish.h"
#include "region.h"
#include "math.h"
#include "dervish_msg_c.h"
#include "errStack.h"
#include "phStructs.h"
#include "phCompUtils.h"

#define COMP_DEBUG 10

#define output_nybble(outfile, c)	output_nbits(outfile,c,4)

extern int qtree_encode(MYFILE *outfile, int a[], int n, int nqx, int nqy,
			int nbitplanes);
extern int start_outputing_bits(void);
extern int done_outputing_bits(MYFILE *outfile);
extern int output_nbits(MYFILE *outfile, int bits, int n);

/*
 * MYFILE *outfile;   output data stream	
 * int a[];	   Array of values to encode	
 * int nx,ny;	  Array dimensions [nx][ny]	
 * unsigned char nbitplanes[3];   Number of bit planes in quadrants	
 */

extern int
doencode(MYFILE *outfile, int a[], int nx, int ny,unsigned char nbitplanes[3])
{
  int nx2, ny2;
  
  nx2 = (nx+1)/2;
  ny2 = (ny+1)/2;
  /*
   * Initialize bit output
   */
  start_outputing_bits();
  /*
   * write out the bit planes for each quadrant
   */
  qtree_encode(outfile, &a[0],          ny, nx2,  ny2,  nbitplanes[0]);
  qtree_encode(outfile, &a[ny2],        ny, nx2,  ny/2, nbitplanes[1]);
  qtree_encode(outfile, &a[ny*nx2],     ny, nx/2, ny2,  nbitplanes[1]);
  qtree_encode(outfile, &a[ny*nx2+ny2], ny, nx/2, ny/2, nbitplanes[2]);
  /*
   * Add zero as an EOF symbol
   */
  output_nybble(outfile, 0);
  done_outputing_bits(outfile);
  return (SH_SUCCESS);
}




