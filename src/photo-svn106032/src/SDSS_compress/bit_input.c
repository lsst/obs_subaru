/* Copyright (c) 1993 Association of Universities for Research
 * in Astronomy. All rights reserved. Produced under National
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */

/* BIT INPUT ROUTINES */

#include <stdio.h>
#include <stdlib.h>
#include "region.h"
#include "dervish_msg_c.h"
#include "phCompUtils.h"


/* THE BIT BUFFER */

static int buffer;					/* Bits waiting to be input				*/
static int bits_to_go;				/* Number of bits still in buffer		*/

extern void erreof(char *str);

/* INITIALIZE BIT INPUT */

extern void
start_inputing_bits(void)
{
	/*
	 * Buffer starts out with no bits in it
	 */
	bits_to_go = 0;
}


/* INPUT A BIT */

extern int
input_bit(MYFILE *infile)
{
	if (bits_to_go == 0) {
		/* Read next byte if no bits left in buffer */
		buffer = getCB(infile);
		if (buffer == EOF) erreof("input_bit");
		bits_to_go = 8;
	}
	/*
	 * Return the next bit
	 */
	bits_to_go -= 1;
	return((buffer>>bits_to_go) & 1);
}


/* INPUT N BITS (N must be <= 24) */

extern int
input_nbits(MYFILE *infile, int n)
{
int c;
/* local copies */
int lbuffer;
int lbits_to_go;

	lbuffer = buffer;
	lbits_to_go = bits_to_go;
	while (lbits_to_go < n) {
		/*
		 * need another byte's worth of bits
		 */
		lbuffer <<= 8;
		c = getCB(infile);
		if (c == EOF) erreof("input_nbits");
		lbuffer |= c;
		lbits_to_go += 8;
	}
	/*
	 * now pick off the first n bits
	 */
	lbits_to_go -= n;
	c = (lbuffer>>lbits_to_go) & ((1<<n)-1);

	bits_to_go = lbits_to_go;
	buffer = lbuffer;
	return( c );
}

extern void
erreof(char *str)
{
	/*
	 * end of file is an error for this application
	 */
	fprintf(stderr, "%s: unexpected end-of-file\n", str);
	exit(-1);
}
